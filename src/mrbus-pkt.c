/*************************************************************************
Title:    MRBus Common Packet Handling Functions
Authors:  Michael Petersen <railfan@drgw.net>
          Nathan Holmes <maverick@drgw.net>
File:     mrbus-pkt.c
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2014 Nathan Holmes and Michael Petersen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License along 
    with this program. If not, see http://www.gnu.org/licenses/
    
*************************************************************************/

#include <avr/eeprom.h>
#include "mrbus.h"

#ifdef __AVR__

// FIXME: EEPROM addresses are limited to 255 + length of MRBus packet.  Should there be an optional extended write command with 16-bit addressing?

uint8_t mrbusPktHandler(uint8_t* rxBuffer, uint8_t* txBuffer, uint8_t mrbus_dev_addr)
{
	// Loopback Test - did we send it?  If so, we probably want to ignore it
	if (rxBuffer[MRBUS_PKT_SRC] == mrbus_dev_addr) 
		return 0;

	// Destination Test - is this for us or broadcast?  If not, ignore
	if (0xFF != rxBuffer[MRBUS_PKT_DEST] && mrbus_dev_addr != rxBuffer[MRBUS_PKT_DEST]) 
		return 0;
	
	// CRC16 Test - is the packet intact?
	if (!mrbusIsCrcValid(rxBuffer))
		return 0;

	if ('A' == rxBuffer[MRBUS_PKT_TYPE])
	{
		// PING packet
		txBuffer[MRBUS_PKT_DEST] = rxBuffer[MRBUS_PKT_SRC];
		txBuffer[MRBUS_PKT_SRC] = mrbus_dev_addr;
		txBuffer[MRBUS_PKT_LEN] = 6;
		txBuffer[MRBUS_PKT_TYPE] = 'a';
		return (MRBUS_HANDLER_DONE);
	} 
	else if ('W' == rxBuffer[MRBUS_PKT_TYPE]) 
	{
		// EEPROM WRITE Packet
		uint8_t numBytes, i;
		if( !((0xFF == rxBuffer[MRBUS_PKT_DEST]) && (MRBUS_EE_DEVICE_ADDR == rxBuffer[6])) && (rxBuffer[MRBUS_PKT_LEN] > 7) )
		{
			// Exclude global writes to device address
			// Exclude packets with no data
			txBuffer[MRBUS_PKT_DEST] = rxBuffer[MRBUS_PKT_SRC];
			txBuffer[MRBUS_PKT_SRC] = mrbus_dev_addr;

			// Write specified number of bytes.
			// Limit maximum to avoid writing garbage if beyond MRBus buffer size.
			numBytes = rxBuffer[MRBUS_PKT_LEN] - 7;
			if(numBytes > (MRBUS_BUFFER_SIZE - 7))
				numBytes = MRBUS_BUFFER_SIZE - 7;

			txBuffer[MRBUS_PKT_LEN] = numBytes + 7;
			txBuffer[MRBUS_PKT_TYPE] = 'w';
			txBuffer[6] = rxBuffer[6];
			for(i=0; i<numBytes; i++)
			{
				eeprom_write_byte((uint8_t*)(uint16_t)(rxBuffer[6]+i), rxBuffer[7+i]);
				txBuffer[7+i] = rxBuffer[7+i];
			}
			return (MRBUS_HANDLER_EEPROM);
		}
	}
	else if ('R' == rxBuffer[MRBUS_PKT_TYPE]) 
	{
		// EEPROM READ Packet
		uint8_t numBytes, i;
		txBuffer[MRBUS_PKT_DEST] = rxBuffer[MRBUS_PKT_SRC];
		txBuffer[MRBUS_PKT_SRC] = mrbus_dev_addr;
		if((rxBuffer[MRBUS_PKT_LEN] > 7) && (rxBuffer[7] > 0))
		{
			// Read specified number of bytes.  Limit to avoid overflowing MRBus buffer.
			numBytes = rxBuffer[7];
			if(numBytes > (MRBUS_BUFFER_SIZE - 7))
				numBytes = MRBUS_BUFFER_SIZE - 7;
		}
		else
		{
			// Default to 1 byte
			numBytes = 1;
		}
		txBuffer[MRBUS_PKT_LEN] = numBytes + 7;
		txBuffer[MRBUS_PKT_TYPE] = 'r';
		txBuffer[6] = rxBuffer[6];
		for(i=0; i<numBytes; i++)
		{
			txBuffer[7+i] = eeprom_read_byte( (uint8_t*)(uint16_t)(rxBuffer[6]+i) );
		}
		return (MRBUS_HANDLER_DONE);
	}
	else if ('V' == rxBuffer[MRBUS_PKT_TYPE]) 
	{
		// Version
		txBuffer[MRBUS_PKT_DEST] = rxBuffer[MRBUS_PKT_SRC];
		txBuffer[MRBUS_PKT_SRC] = mrbus_dev_addr;
		txBuffer[MRBUS_PKT_LEN] = 16;
		txBuffer[MRBUS_PKT_TYPE] = 'v';
		txBuffer[6]  = MRBUS_VERSION_WIRED;
		txBuffer[7]  = ((uint32_t)SWREV >> 16) & 0xFF;
		txBuffer[8]  = ((uint32_t)SWREV >> 8) & 0xFF;
		txBuffer[9]  = (uint32_t)SWREV & 0xFF;
		txBuffer[10]  = HWREV_MAJOR;
		txBuffer[11]  = HWREV_MINOR;
		// Application inserts ASCII descrption string starting at txBuffer[12]
		return (MRBUS_HANDLER_VERSION);
	}
	else if ('X' == rxBuffer[MRBUS_PKT_TYPE]) 
	{
		return (MRBUS_HANDLER_RESET);
	}
	else
	{
		return (MRBUS_HANDLER_CUSTOM);
	}
	return 0;
}
#endif  // End of __AVR__ PKT routines

