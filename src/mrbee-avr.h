/*************************************************************************
Title:    MRBus Atmel AVR Header
Authors:  Nathan Holmes <maverick@drgw.net>, Colorado, USA
          Michael Petersen <railfan@drgw.net>, Colorado, USA
          Michael Prader, South Tyrol, Italy
          Mark Finn <mark@mfinn.net>, Green Bay, WI, USA
File:     mrbus-avr.h
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2014 Nathan Holmes, Michael Petersen, and Michael Prader

    Original MRBus code developed by Nathan Holmes for PIC architecture.
    This file is based on AVR port by Michael Prader.  Updates and
    compatibility fixes by Michael Petersen.
    
    UART code derived from AVR UART library by Peter Fleury, and as
    modified by Tim Sharpe.

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

#ifndef MRBEE_AVR_H
#define MRBEE_AVR_H

// AVR type-specific stuff
// Define the UART port and registers used for XBee communication
// Follows the format of the AVR UART library by Fleury/Sharpe


#if defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega48P__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168P__) || \
    defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) 
#define MRBEE_ATMEGA_USART
#define MRBEE_UART_RX_INTERRUPT    USART_RX_vect
#define MRBEE_UART_TX_INTERRUPT    USART_UDRE_vect
#define MRBEE_PORT                 PORTD
#define MRBEE_PIN                  PIND
#define MRBEE_DDR                  DDRD


#ifndef MRBEE_CTS
#define MRBEE_CTS                  3       /* PD3 */
#endif
#ifndef MRBEE_RTS
#define MRBEE_RTS                  2       /* PD2 */
#endif 
#ifndef MRBEE_TX
#define MRBEE_TX                   1       /* PD1 */
#endif
#ifndef MRBEE_RX
#define MRBEE_RX                   0       /* PD0 */
#endif

#define MRBEE_UART_UBRR            UBRR0
#define MRBEE_UART_SCR_A           UCSR0A
#define MRBEE_UART_SCR_B           UCSR0B
#define MRBEE_UART_SCR_C           UCSR0C
#define MRBEE_UART_DATA            UDR0
#define MRBEE_UART_UDRIE           UDRIE0
#define MRBEE_RXEN                 RXEN0
#define MRBEE_TXEN                 TXEN0
#define MRBEE_RXCIE                RXCIE0
#define MRBEE_TXCIE                TXCIE0
#define MRBEE_TXC                  TXC0
#define MRBEE_RX_ERR_MASK          (_BV(FE0) | _BV(DOR0))

#elif defined(__AVR_ATmega32U4__)

#define MRBEE_ATMEGA_USART1
#define MRBEE_UART_RX_INTERRUPT    USART1_RX_vect
#define MRBEE_UART_TX_INTERRUPT    USART1_UDRE_vect
#define MRBEE_PORT                 PORTD
#define MRBEE_PIN                  PIND
#define MRBEE_DDR                  DDRD

#ifndef MRBEE_CTS
#define MRBEE_CTS                  3       /* PD3 */
#endif
#ifndef MRBEE_RTS
#define MRBEE_RTS                  2       /* PD2 */
#endif
#ifndef MRBEE_TX
#define MRBEE_TX                   1       /* PD1 */
#endif
#ifndef MRBEE_RX
#define MRBEE_RX                   0       /* PD0 */
#endif


#define MRBEE_UART_UBRR           UBRR1
#define MRBEE_UART_SCR_A          UCSR1A
#define MRBEE_UART_SCR_B          UCSR1B
#define MRBEE_UART_SCR_C          UCSR1C
#define MRBEE_UART_DATA           UDR1
#define MRBEE_UART_UDRIE          UDRIE1
#define MRBEE_RXEN                RXEN1
#define MRBEE_TXEN                TXEN1
#define MRBEE_RXCIE               RXCIE1
#define MRBEE_TXCIE               TXCIE1
#define MRBEE_TXC                 TXC1
#define MRBEE_RX_ERR_MASK         (_BV(FE1) | _BV(DOR1))


#elif defined(__AVR_ATmega164P__) || defined(__AVR_ATmega324P__) || \
    defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)

#define MRBEE_ATMEGA_USART0
#define MRBEE_UART_RX_INTERRUPT    USART0_RX_vect
#define MRBEE_UART_TX_INTERRUPT    USART0_UDRE_vect
#define MRBEE_PORT                 PORTD
#define MRBEE_PIN                  PIND
#define MRBEE_DDR                  DDRD

#ifndef MRBEE_CTS
#define MRBEE_CTS                  5       /* PD5 */
#endif
#ifndef MRBEE_RTS
#define MRBEE_RTS                  6       /* PD6 */
#endif
#ifndef MRBEE_TX
#define MRBEE_TX                   1       /* PD1 */
#endif
#ifndef MRBEE_RX
#define MRBEE_RX                   0       /* PD0 */
#endif


#define MRBEE_UART_UBRR           UBRR0
#define MRBEE_UART_SCR_A          UCSR0A
#define MRBEE_UART_SCR_B          UCSR0B
#define MRBEE_UART_SCR_C          UCSR0C
#define MRBEE_UART_DATA           UDR0
#define MRBEE_UART_UDRIE          UDRIE0
#define MRBEE_RXEN                RXEN0
#define MRBEE_TXEN                TXEN0
#define MRBEE_RXCIE               RXCIE0
#define MRBEE_TXCIE               TXCIE0
#define MRBEE_TXC                 TXC0
#define MRBEE_RX_ERR_MASK         (_BV(FE0) | _BV(DOR0))

#elif defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__)

#define MRBEE_ATTINY_USART
#define MRBEE_UART_RX_INTERRUPT   USART_RX_vect
#define MRBEE_UART_TX_INTERRUPT   USART_UDRE_vect
#define MRBEE_PORT                PORTD
#define MRBEE_PIN                 PIND
#define MRBEE_DDR                 DDRD


#ifndef MRBEE_TX
#define MRBEE_TX                  1       /* PD1 */
#endif
#ifndef MRBEE_RX
#define MRBEE_RX                  0       /* PD0 */
#endif

#define MRBEE_UART_UBRRH          UBRRH
#define MRBEE_UART_UBRRL          UBRRL
#define MRBEE_UART_SCR_A          UCSRA
#define MRBEE_UART_SCR_B          UCSRB
#define MRBEE_UART_SCR_C          UCSRC
#define MRBEE_UART_DATA           UDR
#define MRBEE_UART_UDRIE          UDRIE
#define MRBEE_RXEN                RXEN
#define MRBEE_TXEN                TXEN
#define MRBEE_RXCIE               RXCIE
#define MRBEE_TXCIE               TXCIE
#define MRBEE_TXC                 TXC
#define MRBEE_RX_ERR_MASK         (_BV(FE) | _BV(DOR))
#else
#error "No UART definition for MCU available"
#error "Please feel free to add one and send us the patch"
#endif

#endif // MRBEE_AVR_H


