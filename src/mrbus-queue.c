#ifdef __AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "mrbus-constants.h"
#include "mrbus-queue.h"
#include "mrbus-macros.h"

void mrbusPktQueueInitialize(MRBusPktQueue* q, MRBusPacket* pktBufferArray, uint8_t pktBufferArraySz)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		q->pktBufferArray = pktBufferArray;
		q->pktBufferArraySz = pktBufferArraySz;
		q->headIdx = q->tailIdx = 0;
		q->full = 0;
		memset(q->pktBufferArray, 0, pktBufferArraySz * sizeof(MRBusPacket));
	}
}

uint8_t mrbusPktQueueDepth(MRBusPktQueue* q)
{
	uint8_t result = 0;
	if(q->full)
		return(q->pktBufferArraySz);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		result = (uint8_t)(q->headIdx - q->tailIdx) % q->pktBufferArraySz;
	}
	return(result);
}

uint8_t mrbeePktQueuePush(MRBusPktQueue* q, uint8_t* data, uint8_t dataLen, uint8_t rssi)
{
	uint8_t* pktPtr;
	// If full, bail with a false
	if (q->full)
		return(0);

	dataLen = min(MRBUS_BUFFER_SIZE, dataLen);
	pktPtr = (uint8_t*)q->pktBufferArray[q->headIdx].pkt;
	memcpy(pktPtr, data, dataLen);
	memset(pktPtr+dataLen, 0, MRBUS_BUFFER_SIZE - dataLen);
	q->pktBufferArray[q->headIdx].rssi = rssi;

	if( ++q->headIdx >= q->pktBufferArraySz )
		q->headIdx = 0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (q->headIdx == q->tailIdx)
			q->full = 1;
	}
	return(1);
}

uint8_t mrbusPktQueuePush(MRBusPktQueue* q, uint8_t* data, uint8_t dataLen)
{
	return mrbeePktQueuePush(q, data, dataLen, 0);
}


uint8_t mrbeePktQueuePopInternal(MRBusPktQueue* q, uint8_t* data, uint8_t dataLen, uint8_t snoop, uint8_t* rssiPtr)
{
	memset(data, 0, dataLen);
	if (NULL != rssiPtr)
		*rssiPtr = 0;

	if (0 == mrbusPktQueueDepth(q))
		return(0);

	memcpy(data, (uint8_t*)&(q->pktBufferArray[q->tailIdx].pkt), min(dataLen, q->pktBufferArray[q->tailIdx].pkt[MRBUS_PKT_LEN]));
	if (NULL != rssiPtr)
		*rssiPtr = q->pktBufferArray[q->tailIdx].rssi;

	// Snoop indicates that we shouldn't actually pop the packet off - just copy it out
	if (snoop)
		return(1);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if( ++q->tailIdx >= q->pktBufferArraySz )
			q->tailIdx = 0;
		q->full = 0;
	}

	return(1);
}

uint8_t mrbusPktQueuePopInternal(MRBusPktQueue* q, uint8_t* data, uint8_t dataLen, uint8_t snoop)
{
	return mrbeePktQueuePopInternal(q, data, dataLen, snoop, NULL);
}

uint8_t mrbusPktQueueDrop(MRBusPktQueue* q)
{
	if (0 == mrbusPktQueueDepth(q))
		return(0);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if( ++q->tailIdx >= q->pktBufferArraySz )
			q->tailIdx = 0;
		q->full = 0;
	}
	return(1);
}

