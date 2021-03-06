/*************************************************************************
Title:    Wireless MRBus Common Header File
Authors:  Nathan Holmes <maverick@drgw.net>, Colorado, USA
          Michael Petersen <railfan@drgw.net>, Colorado USA
          Michael Prader, South Tyrol, Italy
File:     mrbus.h
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2014 Nathan Holmes, Michael Petersen, and Michael Prader

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

#ifndef MRBEE_H
#define MRBEE_H


#ifdef __AVR__
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include "mrbus-constants.h"
#include "mrbus-queue.h"
#include "mrbus-macros.h"
#include "mrbee-avr.h"
#endif

// Global variable externs, so everybody can see the public mrbus variabes
extern MRBusPktQueue mrbeeRxQueue;
extern MRBusPktQueue mrbeeTxQueue;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __AVR__
uint16_t mrbusCRC16Update(uint16_t crc, uint8_t a);
#endif

void mrbeeInit(void);
void mrbeeSetPriority(uint8_t priority);
uint8_t mrbeeTxActive();
uint8_t mrbeeTransmit(void);
uint8_t mrbeeIsBusIdle();
uint8_t mrbeeGetRssi(void);

#ifdef __cplusplus
}
#endif

#endif

