////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//					
//  Module Name:	CstiCircularBuffer
//
//  File Name:	CstiCircularBuffer.cpp
//
//  Owner:		Scot L. Brooksby
//
//	Abstract:
//		Implements a circular buffer
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "CstiCircularBuffer.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "stiOS.h"
#include <cstring>

//
// Globals
// 

//#define SLB_LOG

#ifdef SLB_LOG
HANDLE hInFile;
HANDLE hOutFile;
#endif


///////////////////////////////////////////////////////////////////////////////
//; CstiCircularBuffer::Open
//
// Description:		Opens (creates) the circular buffer
//
// Abstract:
//		
// Returns:
//	
//	estiOK (Success) or estiERROR (Failure)
//
EstiResult CstiCircularBuffer::Open (
	PstiCallbackFunction pfnDataAvailableCallback,  // call back function that will be called when data is written into the buffer
	uint32_t un32BufferSize)						// desired size of circular buffer
{
	EstiResult eResult = estiOK;
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	m_pfnDataAvailable = pfnDataAvailableCallback;
	m_un32stiBufferSize = un32BufferSize;
	
	// Create the buffer
	m_pun8Buffer = new uint8_t [m_un32stiBufferSize];

	// Init the buffer memory
	#ifdef stiDEBUGGING_TOOLS
	memset (m_pun8Buffer, 0, m_un32stiBufferSize);
	#endif
	
	// Setup pointers into circular buffer
	m_pun8ReadMarker = &m_pun8Buffer[0];
	m_pun8WriteMarker = &m_pun8Buffer[0];
	m_un32Level = m_pun8WriteMarker - m_pun8ReadMarker;

#ifdef SLB_LOG
hInFile=CreateFile(TEXT("InFile.log"),GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW, FILE_ATTRIBUTE_NORMAL,0);
hOutFile=CreateFile(TEXT("OutFile.log"),GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW, FILE_ATTRIBUTE_NORMAL,0);
#endif

	return eResult;

} // end CstiCircularBuffer::Open


///////////////////////////////////////////////////////////////////////////////
//; CstiCircularBuffer::Close
//
// Description:		Cleans up the circular buffer 
//
// Abstract:
//		
// Returns:
//	
//	estiOK (Success) or estiERROR (Failure)
//
EstiResult CstiCircularBuffer::Close ()
{
	EstiResult eResult = estiOK;

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (m_pun8Buffer)
	{
		delete [] m_pun8Buffer;
		m_pun8Buffer = nullptr;
	}
	
#ifdef SLB_LOG
CloseHandle (hInFile);
CloseHandle (hOutFile);
#endif

	return eResult;

} // end CstiCircularBuffer::Close 


///////////////////////////////////////////////////////////////////////////////
//; CstiCircularBuffer::Peek
//
// Description:		Reads data from the circular buffer without removing it
//
// Abstract:
//		
// Returns:
//	
//	estiOK (Success) or estiERROR (Failure)
//
EstiResult CstiCircularBuffer::Peek (void * lpReturnBuffer, 
		uint32_t un32NumberOfBytesToRead, 
		uint32_t & un32NumberOfBytesRead)
{
	EstiResult eResult = estiERROR;

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	stiDEBUG_TOOL (g_stiCircularBufferOutput,
		stiTrace("CstiCircularBuffer::Peek - "
		"Requesting: %d bytes\n", un32NumberOfBytesToRead);
	) // end stiDEBUG_TOOL

	if (m_un32Level == 0)
	{
		un32NumberOfBytesRead = 0;
		return eResult = estiOK;
	} // end if

	auto  pun8Buffer = (uint8_t *) lpReturnBuffer;
	uint32_t un32UntilEnd;
	uint32_t un32Level = m_un32Level;
	uint8_t * pun8ReadMarker = m_pun8ReadMarker;

	un32UntilEnd = &m_pun8Buffer[m_un32stiBufferSize] - pun8ReadMarker;

	// Is a wrap necessary in order to read the requested number of bytes?
	if (un32UntilEnd < un32NumberOfBytesToRead)
	{
		// Yes, a wrap is necessary
		uint32_t un32Left;

		// Are there fewer bytes available then were requested?
		if (un32Level < un32NumberOfBytesToRead)
		{
			// Yes, but is a wrap really necessary?
			if (un32UntilEnd < un32Level)
			{
				stiDEBUG_TOOL (g_stiCircularBufferOutput,
					stiTrace("CstiCircularBuffer::Peek - "
					"Wrapping less than requested... \n");
				) // end stiDEBUG_TOOL

				// Yes, so copy until the end then all that are available from the beginning
				memcpy (pun8Buffer, pun8ReadMarker, un32UntilEnd);
				un32Left = un32Level - un32UntilEnd;
				memcpy (pun8Buffer + un32UntilEnd, m_pun8Buffer, un32Left);
				un32NumberOfBytesRead = un32Level;
			}
			else
			{
				stiDEBUG_TOOL (g_stiCircularBufferOutput,
					stiTrace("CstiCircularBuffer::Peek - "
					"No Wrap needed (less than requested)... \n");
				) // end stiDEBUG_TOOL

				//No, no wrap needed after all
				memcpy (pun8Buffer, pun8ReadMarker, un32Level);
				un32NumberOfBytesRead = un32Level;
			}
			eResult = estiOK;
		}
		else
		{
			stiDEBUG_TOOL (g_stiCircularBufferOutput,
				stiTrace("CstiCircularBuffer::Peek - "
				"Wrapping all that were requested... \n");
			) // end stiDEBUG_TOOL

			// No, copy until the end of the Buffer and then continue from the beginning
			memcpy (pun8Buffer, pun8ReadMarker, un32UntilEnd);
			un32Left = un32NumberOfBytesToRead - un32UntilEnd;
			memcpy (pun8Buffer + un32UntilEnd, m_pun8Buffer, un32Left);
			un32NumberOfBytesRead = un32NumberOfBytesToRead;
			eResult = estiOK;
		}
	}

	else
	{
		// No, there is no wrap necessary.
		// Are there fewer bytes available then were requested?
		if (un32Level < un32NumberOfBytesToRead)
		{
			stiDEBUG_TOOL (g_stiCircularBufferOutput,
				stiTrace("CstiCircularBuffer::Peek - "
				"Returning less than requested... \n");
			) // end stiDEBUG_TOOL

			// Yes, so copy all that are left in the Buffer
			memcpy (pun8Buffer, pun8ReadMarker, un32Level);
			un32NumberOfBytesRead = un32Level;
			eResult = estiOK;
		}
		else
		{
			stiDEBUG_TOOL (g_stiCircularBufferOutput,
				stiTrace("CstiCircularBuffer::Peek - "
				"Returning all requested bytes... \n");
			) // end stiDEBUG_TOOL

			// No, so copy the number of bytes requested.
			memcpy (pun8Buffer, pun8ReadMarker, un32NumberOfBytesToRead);
			un32NumberOfBytesRead = un32NumberOfBytesToRead;
			eResult = estiOK;
		}
	}

#ifdef SLB_LOG
uint32_t un32Num;
WriteFile(hOutFile, pun8Buffer, un32NumberOfBytesRead, &un32Num, NULL);
FlushFileBuffers (hOutFile);
#endif

	return eResult;

} // end CstiCircularBuffer::Peek



///////////////////////////////////////////////////////////////////////////////
//; CstiCircularBuffer::Read
//
// Description:	Reads data from the circular buffer and then removes it
//
// Abstract:
//		
// Returns:
//	
//	estiOK (Success) or estiERROR (Failure)
//
EstiResult CstiCircularBuffer::Read (void * pun8Buffer, 
		uint32_t un32NumberOfBytesToRead, 
		uint32_t & un32NumberOfBytesRead)
{
	EstiResult eResult = estiOK;

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Did the Peek operation work?
	if (estiOK == Peek (pun8Buffer, un32NumberOfBytesToRead, un32NumberOfBytesRead))
	{
		// Yes, it did.  So, set the new pointer position
		uint32_t un32UntilEnd;
		uint32_t un32Level;

		un32UntilEnd = &m_pun8Buffer[m_un32stiBufferSize] - m_pun8ReadMarker;
		// Was a wrap necessary?
		if (un32UntilEnd < un32NumberOfBytesRead)
		{
			// Yes
			m_pun8ReadMarker = (un32NumberOfBytesRead - un32UntilEnd) + m_pun8Buffer;
		} // end if
		else
		{
			// No, just do a straight copy.
			m_pun8ReadMarker += un32NumberOfBytesRead;
		} // end else

		m_un32Level -= un32NumberOfBytesRead;
		un32Level = m_un32Level;

#ifdef SLB_LOG
uint32_t un32Num;
WriteFile(hOutFile, pun8Buffer, un32NumberOfBytesRead, &un32Num, NULL);
FlushFileBuffers (hOutFile);
#endif

		stiDEBUG_TOOL (g_stiCircularBufferOutput,
			stiTrace("CstiCircularBuffer::Read - "
			"New level = %d\n", un32Level);
		) // end stiDEBUG_TOOL

		eResult =  estiOK;
	} // end if
	else
	{
		// Peek didn't work
		eResult = estiERROR;
	} // end else

	return eResult;
} // end CstiCircularBuffer::Read 



///////////////////////////////////////////////////////////////////////////////
//; CstiCircularBuffer::Write
//
// Description:	writes data into the circular buffer
//
// Abstract:
//		
// Returns:
//	
//	estiOK (Success) or estiERROR (Failure)
//
EstiResult CstiCircularBuffer::Write (void * lpInBuffer, 
		uint32_t un32NumberOfBytesToWrite, 
		uint32_t & un32NumberOfBytesWritten)
{
	EstiResult eResult = estiOK;
	auto  pun8Buffer = (uint8_t *) lpInBuffer;
	uint32_t un32UntilEnd;
	uint32_t un32FreeBuffer;

	un32NumberOfBytesWritten = 0;

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	stiDEBUG_TOOL (g_stiCircularBufferOutput,
		stiTrace("CstiCircularBuffer::Write - "
		"un32NumberOfBytesToWrite: %d \n", un32NumberOfBytesToWrite);
	) // end stiDEBUG_TOOL

	uint32_t un32Level = m_un32Level;

	un32UntilEnd = &m_pun8Buffer[m_un32stiBufferSize] - m_pun8WriteMarker;
	un32FreeBuffer = m_un32stiBufferSize - un32Level;

	// Is a wrap necessary?
	if (un32UntilEnd < un32NumberOfBytesToWrite)
	{
		// Yes, a wrap was necessary.
		uint32_t un32Left;

		// Is the available space lower then the space needed?
		if (un32FreeBuffer < un32NumberOfBytesToWrite)
		{
			// Yes, but is a wrap really necessary?
			if (un32UntilEnd < un32FreeBuffer)
			{
				stiDEBUG_TOOL (g_stiCircularBufferOutput,
					stiTrace("CstiCircularBuffer::Write - "
					"Wrap with overrun\n");
				) // end stiDEBUG_TOOL

				// Yes, so apply wrap and fill the rest of the available space 
				// and throw out the excess
				memcpy (m_pun8WriteMarker, pun8Buffer, un32UntilEnd);
				un32Left = un32FreeBuffer - un32UntilEnd;
				memcpy (m_pun8Buffer, pun8Buffer + un32UntilEnd, un32Left);
				un32NumberOfBytesWritten = un32FreeBuffer;
				m_pun8WriteMarker = &m_pun8Buffer[un32Left];
			}
			else
			{
				stiDEBUG_TOOL (g_stiCircularBufferOutput,
					stiTrace("CstiCircularBuffer::Write - "
					"Overrun with no wrap\n");
				) // end stiDEBUG_TOOL

				// No, so just fill the left over space
				memcpy (m_pun8WriteMarker, pun8Buffer, un32FreeBuffer);
				un32NumberOfBytesWritten = un32FreeBuffer;
				m_pun8WriteMarker += un32NumberOfBytesWritten;
			} // end else

		} // end if

		else
		{
			stiDEBUG_TOOL (g_stiCircularBufferOutput,
				stiTrace("CstiCircularBuffer::Write - "
				"Wrapping...\n");
			) // end stiDEBUG_TOOL

			// No, so apply wrap and continue from the beginning
			memcpy (m_pun8WriteMarker, pun8Buffer, un32UntilEnd);
			un32Left = un32NumberOfBytesToWrite - un32UntilEnd;
			memcpy (m_pun8Buffer, pun8Buffer + un32UntilEnd, un32Left);
			un32NumberOfBytesWritten = un32NumberOfBytesToWrite;
			m_pun8WriteMarker = &m_pun8Buffer[un32Left];
		
		} // end else

		// Update the markers
		m_un32Level += un32NumberOfBytesWritten;
		un32Level = m_un32Level;

#ifdef SLB_LOG
uint32_t un32Num;
WriteFile(hInFile, lpInBuffer, un32NumberOfBytesWritten, &un32Num, NULL);
if (un32NumberOfBytesWritten != un32NumberOfBytesToWrite)
	WriteFile (hInFile, "OVERRUN", 8, &un32Num, NULL);
FlushFileBuffers (hInFile);
#endif
		eResult = estiOK;
	} // end if

	else
	{
		// No, wrap is unnecessary
		// Is the free space less then Bytes to write?
		if (un32FreeBuffer < un32NumberOfBytesToWrite)
		{
			stiDEBUG_TOOL (g_stiCircularBufferOutput,
				stiTrace("CstiCircularBuffer::Write - "
				"Overrun on normal write\n");
			) // end stiDEBUG_TOOL

			// Yes, only fill available free space and throw out the rest
			memcpy (m_pun8WriteMarker, pun8Buffer, un32FreeBuffer);
			un32NumberOfBytesWritten = un32FreeBuffer;
		} // end if
		else
		{
			stiDEBUG_TOOL (g_stiCircularBufferOutput,
				stiTrace("CstiCircularBuffer::Write - "
				"All Bytes written\n");
			) // end stiDEBUG_TOOL

			// No, write all of the bytes
			memcpy (m_pun8WriteMarker, pun8Buffer, un32NumberOfBytesToWrite);
			un32NumberOfBytesWritten = un32NumberOfBytesToWrite;
		} // end else

		// Update the markers
		m_pun8WriteMarker += un32NumberOfBytesWritten;

		m_un32Level += un32NumberOfBytesWritten;
		un32Level = m_un32Level;

#ifdef SLB_LOG
uint32_t un32Num;
WriteFile(hInFile, lpInBuffer, un32NumberOfBytesWritten, &un32Num, NULL);
if (un32NumberOfBytesWritten != un32NumberOfBytesToWrite)
	WriteFile (hInFile, "OVERRUN", 8, &un32Num, NULL);
FlushFileBuffers (hInFile);
#endif
		eResult = estiOK;
	} // end else

	if (estiOK == eResult)
	{
		// Indicate that data is available
		if ( nullptr != m_pfnDataAvailable)
		{
			m_pfnDataAvailable ();
		} // end if

	} // end if

	stiDEBUG_TOOL (g_stiCircularBufferOutput,
		stiTrace("CstiCircularBuffer::Write - "
		"New level = %d\n", un32Level);
	) // end stiDEBUG_TOOL

	return eResult;
} // end CstiCircularBuffer::Write



///////////////////////////////////////////////////////////////////////////////
//; CstiCircularBuffer::Purge
//
// Description:	Removes all data from the circular buffer
//
// Abstract:
//		
// Returns:
//	
//	estiOK (Success) or estiERROR (Failure)
//
EstiResult CstiCircularBuffer::Purge()
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Set the read and write markers equal to each other and reset m_un32Level
	m_un32Level = 0;
	m_pun8ReadMarker = m_pun8WriteMarker;

	return (estiOK);
} // end CstiCircularBuffer::Purge


///////////////////////////////////////////////////////////////////////////////
//; CstiCircularBuffer::Seek
//
// Description:	This advances the read pointer inside of the buffer without
//				returning any data.
//
// Abstract:	This function moves the current read position inside the buffer
//				This is usefull if you use the peek function directly to 
//				retrieve data, and then want to skip over the data you have read
//		
// Returns:
//	
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiCircularBuffer::Seek (
	uint32_t un32NumberOfBytesToSeek)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32UntilEnd;

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Do not allow a seek beyond the number of bytes in the buffer
	if (un32NumberOfBytesToSeek > m_un32Level)
	{
		un32NumberOfBytesToSeek = m_un32Level;
	} // end if

	// Do not allow a seek backwards.
	if (un32NumberOfBytesToSeek > 0)
	{
		un32UntilEnd = &m_pun8Buffer[m_un32stiBufferSize] - m_pun8ReadMarker;

		// Was a wrap necessary?
		if (un32UntilEnd < un32NumberOfBytesToSeek)
		{
			// Yes
			m_pun8ReadMarker = (un32NumberOfBytesToSeek - un32UntilEnd) + m_pun8Buffer;
		} // end if
		else
		{
			// No, just do a straight copy.
			m_pun8ReadMarker += un32NumberOfBytesToSeek;
		} // end else

		m_un32Level -= un32NumberOfBytesToSeek;
	} // end if

	return hResult;
} // end CstiCircularBuffer::Read




///////////////////////////////////////////////////////////////////////////////
//; CstiCircularBuffer::GetNumberOfBytesInBuffer
//
// Description:	Gets the number of bytes currently in the buffer.
//
// Abstract:	Gets the number of bytes currently in the buffer.
//		
// Returns:
//	
//				The number of bytes in the buffe
//
uint32_t CstiCircularBuffer::GetNumberOfBytesInBuffer()
{
	return m_un32Level;
} // end CstiCircularBuffer::GetNumberOfBytesInBuffer
