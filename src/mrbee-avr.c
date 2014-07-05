#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/wdt.h>

#include "mrbee.h"

#define MRBEE_UART_TX_BUFFER_SIZE  64
#define MRBEE_UART_RX_BUFFER_SIZE  64

//static volatile uint8_t mrbeeActivity;

// While mrbuxRxBuffer is volatile, it's only accessed within the ISR
static uint8_t mrbeeRxBuffer[MRBEE_UART_RX_BUFFER_SIZE];
static volatile uint8_t mrbeeRxIndex=0;
static volatile uint8_t mrbeeTxBuffer[MRBEE_UART_TX_BUFFER_SIZE];
static volatile uint8_t mrbeeTxIndex=0, mrbeeTxEnd;

MRBusPktQueue mrbeeRxQueue;
MRBusPktQueue mrbeeTxQueue;

#define RX_ISR_ESCAPE_NEXT_BYTE 0x01
#define RX_ISR_PROCESSING_PKT   0x02

ISR(MRBEE_UART_RX_INTERRUPT)
{
	uint8_t data;
	static uint8_t rxFlags = 0;
	static uint8_t expectedPktLen;


	if (MRBEE_UART_SCR_A & MRBEE_RX_ERR_MASK)
	{
		// Handle framing errors - these are likely arbitration bytes
		mrbeeRxIndex = MRBEE_UART_DATA;
		mrbeeRxIndex = 0; // Reset receive buffer
		rxFlags = 0;
	}

	data = MRBEE_UART_DATA;

	switch(data)
	{
		case 0x7E:  // Start of XBee frame
			mrbeeRxIndex = 0;
			memset(mrbeeRxBuffer, 0, sizeof(mrbeeRxBuffer));
			rxFlags |= RX_ISR_PROCESSING_PKT;
			mrbeeRxBuffer[mrbeeRxIndex++] = data;
			break;

		case 0x7D: // XBee escape character
			if (!(rxFlags & RX_ISR_PROCESSING_PKT))
				break;

			rxFlags |= RX_ISR_ESCAPE_NEXT_BYTE;
			break;
			
		default:
			if (!(rxFlags & RX_ISR_PROCESSING_PKT))
				break;

			if (rxFlags & RX_ISR_ESCAPE_NEXT_BYTE)
			{
				data ^= 0x20;
				rxFlags &= ~(RX_ISR_ESCAPE_NEXT_BYTE);
			}

			if (mrbeeRxIndex < sizeof(mrbeeRxBuffer)-1)
			{
				mrbeeRxBuffer[mrbeeRxIndex++] = data;
			}
			else
			{
				// Overflow, reset everything and go back to not processing pkt
				rxFlags &= ~(RX_ISR_PROCESSING_PKT);
				mrbeeRxIndex = 0;
				memset(mrbeeRxBuffer, 0, sizeof(mrbeeRxBuffer));
				break;
			}

			if (4 == mrbeeRxIndex)
			{
				expectedPktLen = mrbeeRxBuffer[2] + 4; // length is 3 bytes of header + 1 byte of check + data len
			}

			if ((mrbeeRxIndex > 4) && (mrbeeRxIndex == expectedPktLen))
			{
				// Theoretically, we have enough bytes for the packet now
				uint8_t pktChecksum = 0;

				for (mrbeeRxIndex=3; mrbeeRxIndex<expectedPktLen; mrbeeRxIndex++)
					pktChecksum += mrbeeRxBuffer[mrbeeRxIndex];

				if (0xFF == pktChecksum)
				{
					mrbeeRxIndex = 8; // Abuse the rx index for the packet offset 

					switch(mrbeeRxBuffer[3])
					{
						case 0x80: // 64 bit addressing frame
							mrbeeRxIndex = 14;
							// Intentional fall-through
						case 0x81: // 16 bit addressing frame
							// 0xFF is a passing checksum, load packet into mrbeeRxQueue
							mrbusPktQueuePush(&mrbeeRxQueue, mrbeeRxBuffer + mrbeeRxIndex, min(mrbeeRxBuffer[mrbeeRxIndex + MRBUS_PKT_LEN], MRBUS_BUFFER_SIZE));
							break;

						// All other cases ignored - these are packet types from the XBee we don't understand	
					}
				
				}

				memset(mrbeeRxBuffer, 0, sizeof(mrbeeRxBuffer));
				mrbeeRxIndex = 0;
				rxFlags = 0;
			}
			break;			
	}
}

ISR(MRBEE_UART_TX_INTERRUPT)
{
#ifndef MRBEE_IGNORE_FLOW
	do {
		wdt_reset();
	} while (MRBEE_PIN & _BV(MRBEE_CTS));  // Watchdog aware loop
#endif

	MRBEE_UART_DATA = mrbeeTxBuffer[mrbeeTxIndex++];  //  Get next byte and write to UART

	if ( (mrbeeTxIndex >= mrbeeTxEnd) || (mrbeeTxIndex >= MRBEE_UART_TX_BUFFER_SIZE) )
	{
		//  Done sending data to UART, disable UART interrupt
		MRBEE_UART_SCR_B &= ~_BV(MRBEE_UART_UDRIE);
		mrbeeTxIndex = 0;
	}
}

void mrbusSetPriority(uint8_t priority)
{
	return;
}

#ifndef MRBEE_BAUD
#define MRBEE_BAUD 115200
#endif


void mrbeeInit(void)
{
	MRBEE_DDR |= _BV(MRBEE_RTS);
	MRBEE_PORT &= ~_BV(MRBEE_RTS);
	MRBEE_DDR &= ~(_BV(MRBEE_RX) | _BV(MRBEE_TX) | _BV(MRBEE_CTS));

	mrbeeRxIndex = 0;
	memset(mrbeeRxBuffer, 0, sizeof(mrbeeRxBuffer));

	mrbeeTxIndex = 0;
	memset((uint8_t*)mrbeeTxBuffer, 0, sizeof(mrbeeTxBuffer));

#undef BAUD
#define BAUD MRBEE_BAUD
#include <util/setbaud.h>

#define USE_2X 0
#define UBRR_VALUE 8

#if defined( MRBEE_AT90_UART )
	// FIXME - probably need more stuff here
	UBRR = (uint8_t)UBRRL_VALUE;

#elif defined( MRBEE_ATMEGA_USART_SIMPLE )
	MRBEE_UART_UBRR = UBRR_VALUE;
	MRBEE_UART_SCR_A = (USE_2X)?_BV(U2X):0;
	MRBEE_UART_SCR_B = 0;
	MRBEE_UART_SCR_C = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
	
#elif defined( MRBEE_ATMEGA_USART0_SIMPLE )
	MRBEE_UART_UBRR = UBRR_VALUE;
	MRBEE_UART_SCR_A = (USE_2X)?_BV(U2X0):0;
	MRBEE_UART_SCR_B = 0;
	MRBEE_UART_SCR_C = _BV(URSEL0) | _BV(UCSZ01) | _BV(UCSZ00);
	
#elif defined( MRBEE_ATMEGA_USART ) || defined ( MRBEE_ATMEGA_USART0 )
	MRBEE_UART_UBRR = UBRR_VALUE;
	MRBEE_UART_SCR_A = (USE_2X)?_BV(U2X0):0;
	MRBEE_UART_SCR_B = 0;
	MRBEE_UART_SCR_C = _BV(UCSZ01) | _BV(UCSZ00);

#elif defined( MRBEE_ATTINY_USART )
	// Top four bits are reserved and must always be zero - see ATtiny2313 datasheet
	// Also, H and L must be written independently, since they're non-adjacent registers
	// on the attiny parts
	MRBEE_UART_UBRRH = 0x0F & UBRRH_VALUE;
	MRBEE_UART_UBRRL = UBRRL_VALUE;
	MRBEE_UART_SCR_A = (USE_2X)?_BV(U2X):0;
	MRBEE_UART_SCR_B = 0;
	MRBEE_UART_SCR_C = _BV(UCSZ1) | _BV(UCSZ0);

#elif defined ( MRBEE_ATMEGA_USART1 )
	MRBEE_UART_UBRR = UBRR_VALUE;
	MRBEE_UART_SCR_A = (USE_2X)?_BV(U2X1):0;
	MRBEE_UART_SCR_B = 0;
	MRBEE_UART_SCR_C = _BV(UCSZ11) | _BV(UCSZ10);
#else
#error "UART for your selected part is not yet defined..."
#endif

#undef BAUD


#ifndef MRBEE_IGNORE_FLOW
	// Wait for XBee to start (assert /CTS line low)
	do 
	{
		wdt_reset();
	} while (MRBEE_PIN & _BV(MRBEE_CTS));  // Watchdog aware loop
#endif

	/* Enable USART receiver and transmitter and receive complete interrupt */
	MRBEE_UART_SCR_B = _BV(MRBEE_RXCIE) | _BV(MRBEE_RXEN) | _BV(MRBEE_TXEN);
}

uint8_t mrbeeTxActive() 
{
	return(MRBEE_UART_SCR_B & _BV(MRBEE_UART_UDRIE));
}

uint8_t mrbeeTransmit(void)
{
	uint8_t i, xbeeChecksum, mrbusPktLen;
	uint8_t mrbeeTxUnescaped[10 + MRBUS_BUFFER_SIZE];
	uint16_t crc16 = 0x0000;

	if (mrbusPktQueueEmpty(&mrbeeTxQueue))
		return(0);

	//  Return if bus already active.
	if (mrbeeTxActive())
		return(1);
		
	memset(mrbeeTxUnescaped, 0, sizeof(mrbeeTxUnescaped));

	mrbeeTxUnescaped[0] = 0x7E;     // 0 - Start 
	mrbeeTxUnescaped[1] = 0x00;     // 1 - Len MSB
	mrbeeTxUnescaped[2] = 0x00;     // 2 - Len LSB
	mrbeeTxUnescaped[3] = 0x01;     // 3 - API being called - transmit by 16 bit address
	mrbeeTxUnescaped[4] = 0x00;     // 4 - Frame identifier
	mrbeeTxUnescaped[5] = 0xFF;     // 5 - MSB of dest address - broadcast 0xFFFF
	mrbeeTxUnescaped[6] = 0xFF;     // 6 - LSB of dest address - broadcast 0xFFFF
	mrbeeTxUnescaped[7] = 0x00; 	  // 7 - Transmit options
		
	mrbusPktQueuePeek(&mrbeeTxQueue, (uint8_t*)mrbeeTxUnescaped + 8, sizeof(mrbeeTxUnescaped) - 8);

	mrbeeTxUnescaped[2] = mrbeeTxUnescaped[8+MRBUS_PKT_LEN] + 5; 
	
	mrbusPktLen = mrbeeTxUnescaped[8 + MRBUS_PKT_LEN];

	// If we have no packet length, or it's less than the header, just silently say we transmitted it
	// On the AVRs, if you don't have any packet length, it'll never clear up on the interrupt routine
	// and you'll get stuck in indefinite transmit busy
	if (mrbusPktLen < MRBUS_PKT_TYPE)
	{
		mrbusPktQueueDrop(&mrbeeTxQueue);
		return(0);
	}
		
	// First Calculate CRC16
	for (i=0; i<mrbusPktLen; i++)
	{
		if ((i != MRBUS_PKT_CRC_H) && (i != MRBUS_PKT_CRC_L))
			crc16 = mrbusCRC16Update(crc16, mrbeeTxUnescaped[8+i]);
	}
	mrbeeTxUnescaped[8+MRBUS_PKT_CRC_L] = (crc16 & 0xFF);
	mrbeeTxUnescaped[8+MRBUS_PKT_CRC_H] = ((crc16 >> 8) & 0xFF);

	// Add up checksum
	xbeeChecksum = 0;
	for(i=3; i< (8 + mrbusPktLen); i++)
		xbeeChecksum += mrbeeTxUnescaped[i];

	xbeeChecksum = 0xFF - xbeeChecksum;
	mrbeeTxUnescaped[8 + mrbusPktLen + 1] = xbeeChecksum;

	// Time to copy and escape
	mrbeeTxEnd = 0;
	memset(mrbeeTxBuffer, 0, sizeof(mrbeeTxBuffer));

	mrbeeTxBuffer[mrbeeTxEnd++] = 0xFE; // Start of frame doesn't get escaped
	for (i=1; i<(8 + mrbusPktLen + 1); i++)
	{
		switch(mrbeeTxUnescaped[i])
		{
			case 0x7E:
			case 0x7D:
			case 0x11:
			case 0x13:
				mrbeeTxBuffer[mrbeeTxEnd++] = 0x7D;
				mrbeeTxBuffer[mrbeeTxEnd++] = 0x20 ^ mrbeeTxUnescaped[i];
				break;
			
			default:
				mrbeeTxBuffer[mrbeeTxEnd++] = mrbeeTxUnescaped[i];
				break;
		}
	}

	mrbeeTxIndex = 0;
	mrbusPktQueueDrop(&mrbeeTxQueue);
	// Enable transmit interrupt
	MRBEE_UART_SCR_B |= _BV(MRBEE_UART_UDRIE);

	return(0);
}

uint8_t mrbeeIsBusIdle()
{
	return(1);
}


