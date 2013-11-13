#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "mrbus.h"

static volatile uint8_t mrbusActivity;
// While mrbuxRxBuffer is volatile, it's only accessed within the ISR
static uint8_t mrbusRxBuffer[MRBUS_BUFFER_SIZE];
static volatile uint8_t mrbusRxIndex=0;
static volatile uint8_t mrbusTxBuffer[MRBUS_BUFFER_SIZE];
static volatile uint8_t mrbusTxIndex=0;
static uint8_t mrbusLoneliness;
static uint8_t mrbusPriority;

MRBusPktQueue mrbusRxQueue;
MRBusPktQueue mrbusTxQueue;

uint8_t mrbusArbBitSend(uint8_t bitval)
{
	uint8_t slice;
	uint8_t tmp = 0;

	cli();
	if (bitval)
		MRBUS_PORT &= ~_BV(MRBUS_TXE);
	else
		MRBUS_PORT |= _BV(MRBUS_TXE);

	for (slice = 0; slice < 10; slice++)
	{
		if (slice > 2)
		{
			if (MRBUS_PIN & _BV(MRBUS_RX)) tmp = 1;
			if (tmp ^ bitval)
			{
				MRBUS_PORT &= ~_BV(MRBUS_TXE);
				MRBUS_DDR &= ~_BV(MRBUS_TX);
				sei();
				return(1);
			}

		}
		_delay_us(20);
	}
	return(0);
}


ISR(MRBUS_UART_RX_INTERRUPT)
{

	//Receive Routine
	mrbusActivity = MRBUS_ACTIVITY_RX;

	if (MRBUS_UART_SCR_A & MRBUS_RX_ERR_MASK)
	{
		// Handle framing errors - these are likely arbitration bytes
		mrbusRxIndex = MRBUS_UART_DATA;
		mrbusRxIndex = 0; // Reset receive buffer
	}
	else
	{
		mrbusRxBuffer[mrbusRxIndex++] = MRBUS_UART_DATA;

		if (mrbusRxIndex > 5 && mrbusRxIndex == mrbusRxBuffer[MRBUS_PKT_LEN])
		{
			mrbusRxIndex = 0;
			mrbusPktQueuePush(&mrbusRxQueue, mrbusRxBuffer, min(mrbusRxBuffer[MRBUS_PKT_LEN], MRBUS_BUFFER_SIZE));
			mrbusActivity = MRBUS_ACTIVITY_IDLE;
		}
		else if (mrbusRxIndex > MRBUS_BUFFER_SIZE)
		// On the off chance we just keep receiving stuff, decrement the buffer to prevent overflow
			mrbusRxIndex--;

	}
}

ISR(MRBUS_UART_DONE_INTERRUPT)
{
	// Transmit is complete: terminate
	MRBUS_PORT &= ~_BV(MRBUS_TXE);
	// Disable the various transmit interrupts and the transmitter itself
	// Re-enable receive interrupt (might be killed if no loopback define is on...)
	MRBUS_UART_SCR_B = (MRBUS_UART_SCR_B & ~(_BV(MRBUS_TXCIE) | _BV(MRBUS_TXEN) | _BV(MRBUS_UART_UDRIE))) | _BV(MRBUS_RXCIE);
	mrbusLoneliness = 6;
}


ISR(MRBUS_UART_TX_INTERRUPT)
{
	// Transmit Routine - load up the USART with the next byte in the packet
	MRBUS_UART_DATA = mrbusTxBuffer[mrbusTxIndex++];

	// If we've just loaded the last byte of the packet into the buffer, enable the "transmit complete"
	// interrupt.  If we enable this before the last byte, there's the possibility of latency between interrupts 
	// causing "transmission complete" before we're actually done sending.

	// At the same time, we can kill the USART DATA READY interrupt.  We're done with it until the next transmission
	if (mrbusTxIndex >= MRBUS_BUFFER_SIZE || mrbusTxBuffer[MRBUS_PKT_LEN] == mrbusTxIndex)
	{
		MRBUS_UART_SCR_A |= _BV(MRBUS_TXC);
		MRBUS_UART_SCR_B &= ~_BV(MRBUS_UART_UDRIE);
		MRBUS_UART_SCR_B |= _BV(MRBUS_TXCIE);
	}
}

void mrbusSetPriority(uint8_t priority)
{
	if (priority < 12)
		mrbusPriority = priority;
}

void mrbusInit(void)
{
	MRBUS_DDR |= _BV(MRBUS_TXE);
	MRBUS_PORT &= ~_BV(MRBUS_TXE);
	MRBUS_DDR &= ~(_BV(MRBUS_RX) | _BV(MRBUS_TX));

	mrbusTxIndex = 0;
	mrbusActivity = MRBUS_ACTIVITY_IDLE;
	mrbusLoneliness = 6;
	mrbusPriority = 6;

#undef BAUD
#define BAUD MRBUS_BAUD
#include <util/setbaud.h>

#if defined( MRBUS_AT90_UART )
	// FIXME - probably need more stuff here
	UBRR = (uint8_t)UBRRL_VALUE;

#elif defined( MRBUS_ATMEGA_USART_SIMPLE )
	MRBUS_UART_UBRR = UBRR_VALUE;
	MRBUS_UART_SCR_A = (USE_2X)?_BV(U2X):0;
	MRBUS_UART_SCR_B = 0;
	MRBUS_UART_SCR_C = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
	
#elif defined( MRBUS_ATMEGA_USART0_SIMPLE )
    MRBUS_UART_UBRR = UBRR_VALUE;
	MRBUS_UART_SCR_A = (USE_2X)?_BV(U2X0):0;
	MRBUS_UART_SCR_B = 0;
	MRBUS_UART_SCR_C = _BV(URSEL0) | _BV(UCSZ01) | _BV(UCSZ00);
	
#elif defined( MRBUS_ATMEGA_USART ) || defined ( MRBUS_ATMEGA_USART0 )
	MRBUS_UART_UBRR = UBRR_VALUE;
	MRBUS_UART_SCR_A = (USE_2X)?_BV(U2X0):0;
	MRBUS_UART_SCR_B = 0;
	MRBUS_UART_SCR_C = _BV(UCSZ01) | _BV(UCSZ00);

#elif defined( MRBUS_ATTINY_USART )
	// Top four bits are reserved and must always be zero - see ATtiny2313 datasheet
	// Also, H and L must be written independently, since they're non-adjacent registers
	// on the attiny parts
	MRBUS_UART_UBRRH = 0x0F & UBRRH_VALUE;
	MRBUS_UART_UBRRL = UBRRL_VALUE;
	MRBUS_UART_SCR_A = (USE_2X)?_BV(U2X):0;
	MRBUS_UART_SCR_B = 0;
	MRBUS_UART_SCR_C = _BV(UCSZ1) | _BV(UCSZ0);

#elif defined ( MRBUS_ATMEGA_USART1 )
	MRBUS_UART_UBRR = UBRR_VALUE;
	MRBUS_UART_SCR_A = (USE_2X)?_BV(U2X1):0;
	MRBUS_UART_SCR_B = 0;
	MRBUS_UART_SCR_C = _BV(UCSZ11) | _BV(UCSZ10);
#else
#error "UART for your selected part is not yet defined..."
#endif

#undef BAUD

	/* Enable USART receiver and transmitter and receive complete interrupt */
	MRBUS_UART_SCR_B = _BV(MRBUS_RXCIE) | _BV(MRBUS_RXEN) | _BV(MRBUS_TXEN);

}

uint8_t mrbusTxActive() 
{
	return(MRBUS_UART_SCR_B & (_BV(MRBUS_UART_UDRIE) | _BV(MRBUS_TXCIE)));
}

uint8_t mrbusTransmit(void)
{
	uint8_t status;
	uint8_t address;
	uint8_t i;
	uint16_t crc16 = 0x0000;

	if (mrbusPktQueueEmpty(&mrbusTxQueue))
		return(0);

	//  Return if bus already active.
	if (mrbusTxActive())
		return(1);

	mrbusPktQueuePeek(&mrbusTxQueue, (uint8_t*)mrbusTxBuffer, sizeof(mrbusTxBuffer));

	// If we have no packet length, or it's less than the header, just silently say we transmitted it
	// On the AVRs, if you don't have any packet length, it'll never clear up on the interrupt routine
	// and you'll get stuck in indefinite transmit busy
	if (mrbusTxBuffer[MRBUS_PKT_LEN] < MRBUS_PKT_TYPE)
	{
		mrbusPktQueueDrop(&mrbusTxQueue);
		return(0);
	}
		
	address = mrbusTxBuffer[MRBUS_PKT_SRC];

	// First Calculate CRC16
	for (i=0; i<mrbusTxBuffer[MRBUS_PKT_LEN]; i++)
	{
		if ((i != MRBUS_PKT_CRC_H) && (i != MRBUS_PKT_CRC_L))
			crc16 = mrbusCRC16Update(crc16, mrbusTxBuffer[i]);
	}
	mrbusTxBuffer[MRBUS_PKT_CRC_L] = (crc16 & 0xFF);
	mrbusTxBuffer[MRBUS_PKT_CRC_H] = ((crc16 >> 8) & 0xFF);

	/* Start 2ms wait for activity */
	mrbusActivity = MRBUS_ACTIVITY_IDLE;

	/* Now go into critical timing loop */
	/* Note that status is abused to calculate bus wait */
	status = ((mrbusLoneliness + mrbusPriority) * 10) + (mrbusTxBuffer[MRBUS_PKT_SRC] & 0x0F);

	_delay_ms(2);

	// Return if activity - we may have a packet to receive
	// Application is responsible for waiting 10ms or for successful receive
	if (mrbusActivity)
	{
		if (mrbusLoneliness)
			mrbusLoneliness--;
		return(1);
	}

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		// Clear driver enable to prevent transmitting and set TX pin low
		MRBUS_PORT &= ~(_BV(MRBUS_TXE) | _BV(MRBUS_TX));  
		// Set TX as an output
		MRBUS_DDR |= _BV(MRBUS_TX);     
		//  Disable transmitter
		MRBUS_UART_SCR_B &= ~_BV(MRBUS_TXEN);

		// Be sure to reset RX index - normally this is reset by other arbitration bytes causing
		//  framing errors, but if we're talking to ourselves, we're screwed because the RX
		//  side of the uart isn't on during arbitration sending
		mrbusRxIndex = 0;
	}

	for (i = 0; i < 44; i++)
	{
		_delay_us(10);
		if (0 == (MRBUS_PIN & _BV(MRBUS_RX)))
		{
			MRBUS_DDR &= ~_BV(MRBUS_TX);
			if (mrbusLoneliness)
				mrbusLoneliness--;
			return(1);
		}
	}

	// Now, wait calculated time from above
	for (i = 0; i < status; i++)
	{
		_delay_us(10);
		if (0 == (MRBUS_PIN & _BV(MRBUS_RX)))
		{
			MRBUS_DDR &= ~_BV(MRBUS_TX);
			if (mrbusLoneliness)
				mrbusLoneliness--;
			return(1);
		}
	}

	// Arbitration Sequence - 4800 bps
	// Start Bit
	if (mrbusArbBitSend(0))
	{
		if (mrbusLoneliness)
			mrbusLoneliness--;
		return(1);
	}

	for (i = 0; i < 8; i++)
	{
		status = mrbusArbBitSend(address & 0x01);
		address = address / 2;

		if (status)
		{
			if (mrbusLoneliness)
				mrbusLoneliness--;
			return(1);
		}
	}

	// Stop Bits
	if (mrbusArbBitSend(1))
	{
		if (mrbusLoneliness)
			mrbusLoneliness--;
		return(1);
	}
	if (mrbusArbBitSend(1))
	{
		if (mrbusLoneliness)
			mrbusLoneliness--;
		return(1);
	}

	mrbusTxIndex = 0;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		/* Set TX back to input */
		MRBUS_DDR &= ~_BV(MRBUS_TX);
		/* Enable transmitter since control over bus is assumed */
		MRBUS_UART_SCR_B |= _BV(MRBUS_TXEN);
		MRBUS_UART_SCR_A |= _BV(MRBUS_TXC);
		MRBUS_PORT |= _BV(MRBUS_TXE);

#ifdef MRBUS_DISABLE_LOOPBACK
		// Disable receive interrupt while transmitting
		MRBUS_UART_SCR_B &= ~_BV(MRBUS_RXCIE);
#endif
		// Enable transmit interrupt
		MRBUS_UART_SCR_B |= _BV(MRBUS_UART_UDRIE);
	}

	mrbusPktQueueDrop(&mrbusTxQueue);

	return(0);
}

uint8_t mrbusIsBusIdle()
{
	return(MRBUS_ACTIVITY_IDLE == mrbusActivity);
}


