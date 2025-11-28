////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiCircularBuffer
//
//  File Name:	CstiCircularBuffer.h
//
//  Owner:		Scot L. Brooksby
//
//	Abstract:
//      See CstiCircularBuffer.cpp

//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTICIRCULARBUFFER_H
#define CSTICIRCULARBUFFER_H


//
// Includes
//
#include "stiDefs.h"
#include <mutex>

//
// Constants
//

#define stiBUFFER_SIZE		100000
#define stiBUFFER_EMPTY		4000
#define stiBUFFER_FULL		80000
#define stiBUFFER_RESUME	8000
	
//
// Typedefs
//
typedef void (*PstiCallbackFunction) ();


//
// Class Declaration
//

class CstiCircularBuffer
{
public:
	uint32_t GetNumberOfBytesInBuffer ();
	uint32_t	m_un32Level = 0;
	
	EstiResult Open(PstiCallbackFunction pfnDataAvailableCallback);

	EstiResult Open (PstiCallbackFunction pfnDataAvailableCallback, 
		uint32_t un32BufferSize = stiBUFFER_SIZE);

	EstiResult Close ();

	EstiResult Read (IN void * lpReturnBuffer, 
		IN uint32_t un32NumberOfBytesToRead, 
		OUT uint32_t & un32NumberOfBytesRead); 

	EstiResult Peek (IN void * lpBuffer, 
		IN uint32_t un32NumberOfBytesToRead, 
		OUT uint32_t & un32NumberOfBytesRead); 

	EstiResult Write (void * lpInBuffer, 
		uint32_t un32NumberOfBytesToWrite, 
		uint32_t & un32NumberOfBytesWritten);

	stiHResult Seek (uint32_t un32NumberOfBytesToSeek);

	EstiResult Purge ();
	
	void WaitForWriteEvent (uint32_t un32Timeout);

private:

	PstiCallbackFunction m_pfnCallbackDataAvailable = nullptr;
	PstiCallbackFunction m_pfnDataAvailable = nullptr;
	std::recursive_mutex m_mutex;
	uint32_t	m_un32stiEmptyMark = 0;
	uint32_t	m_un32stiFullMark = 0;
	uint32_t	m_un32stiResumeMark = 0;

protected:
	uint8_t * m_pun8Buffer = nullptr;	// Allocated Dynamically
	uint8_t * m_pun8ReadMarker = nullptr;
	uint8_t * m_pun8WriteMarker = nullptr;
	uint32_t m_un32stiBufferSize = 0;


};

#endif // CSTICIRCULARBUFFER_H
