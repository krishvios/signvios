////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiUpdateWriter
//
//  File Name:  CstiUpdateWriter.cpp
//
//  Owner:      Eugene R. Christensen
//
//  Abstract:
//      The UpdateWriter task implements the functions needed to write the new
//		image to flash.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//


#include <cctype>  // standard type definitions.

#include "stiOS.h" 
#include "CstiUpdateWriter.h"
#include "stiDebugTools.h"
#include "stiTools.h"
#include "stiTaskInfo.h"



//
// Constants
//

//
// Typedefs
//

//
// Globals
//

//
// Function Declarations
//

////////////////////////////////////////////////////////////////////////////////
//; CstiUpdateWriter::CstiUpdateWriter
//
// Description: Default constructor
//
// Abstract:
//
// Returns:
//  none
//
CstiUpdateWriter::CstiUpdateWriter ()
	:
	CstiEventQueue("CstiUpdateWriter")
{
} // end CstiUpdateWriter::CstiUpdateWriter


////////////////////////////////////////////////////////////////////////////////
//; CstiUpdateWriter::~CstiUpdateWriter
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//  none
//
CstiUpdateWriter::~CstiUpdateWriter ()
{
	if (m_pszFilename)
	{
		delete [] m_pszFilename;
		m_pszFilename = nullptr;
	}

	if (m_fd)
	{
		fclose (m_fd);
		m_fd = nullptr;
	}
	
} // end CstiUpdateWriter::~CstiUpdateWriter


////////////////////////////////////////////////////////////////////////////////
//; CstiUpdateWriter::Initialize
//
// Description: Initialization method for the class
//
// Abstract:
//
// Returns:
//
stiHResult CstiUpdateWriter::Initialize (
	const char *pszFilename)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// If a filename was passed it, allocate a member variable to store it.
	if (pszFilename && *pszFilename)
	{
		m_pszFilename = new char[strlen(pszFilename) + 1];

		stiTESTCOND (nullptr != m_pszFilename, stiRESULT_MEMORY_ALLOC_ERROR);

		strcpy (m_pszFilename, pszFilename);
	}
	
STI_BAIL:

	if (stiIS_ERROR (hResult) && m_pszFilename)
	{
		delete [] m_pszFilename;
		m_pszFilename = nullptr;
	}

	return (hResult);

} // end CstiUpdateWriter::Initialize


stiHResult CstiUpdateWriter::Startup ()
{
	StartEventLoop();
	return stiRESULT_SUCCESS;
}


stiHResult CstiUpdateWriter::bufferWrite (
	char * buffer,
	int bufferLength)
{
	PostEvent ([this, buffer, bufferLength]{ eventBufferWrite (buffer, bufferLength); });

	return stiRESULT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiUpdateWriter::eventBufferWrite
//
// Description: Handles writing to the flash image
//
// Abstract:
//
// Returns:
//  estiOK (Success), estiERROR (Failure)
//
void CstiUpdateWriter::eventBufferWrite (
	char * buffer,
	int bufferLength)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	if (nullptr != m_fd && nullptr != buffer && 0 < bufferLength)
	{
		//
		// Write header to flash (uses mtd)
		//
		int nLengthWritten = fwrite (buffer, 1, bufferLength, m_fd);

		if (nLengthWritten != bufferLength)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("<CstiUpdateWriter::eventBufferWrite> fwrite failed to write buffer\n");
			); // stiDEBUG_TOOL

			errorSignal.Emit (buffer);
			stiTESTCOND (estiFALSE, stiRESULT_ERROR);
		}

		// If we got to this point then we were successful in writing this buffer
		// Write the number of bytes written to flash into the buffer being returned.
		auto pn32Len = (int32_t*)buffer;
		*pn32Len = bufferLength;
		
		bufferWrittenSignal.Emit (buffer, bufferLength);
	}
	else
	{
		// Something is wrong.
		errorSignal.Emit (buffer);
		stiTESTCOND (estiFALSE, stiRESULT_ERROR);
	}

STI_BAIL:

	return;
} // end CstiUpdateWriter::eventBufferWrite

stiHResult CstiUpdateWriter::imageInitialize (
	bool append)
{
	PostEvent ([this, append]{ eventImageInitialize (append); });

	return stiRESULT_SUCCESS;
}

void CstiUpdateWriter::eventImageInitialize (
	bool append)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// If we have the filename (we should), open the file for "write" access.
	if (m_pszFilename && *m_pszFilename)
	{
		//
		// If we have the file open already, close it and open it again.
		//
		if (m_fd)
		{
			fclose (m_fd);
			m_fd = nullptr;
		}

		if (append)
		{
			m_fd = fopen (m_pszFilename, "ab");
		}
		else
		{
			m_fd = fopen (m_pszFilename, "wb");
		}

		if (nullptr == m_fd)
		{
			stiTHROWMSG (stiRESULT_ERROR, "Can't open file: %s\n", m_pszFilename);
		}
	}

STI_BAIL:

	stiUNUSED_ARG (hResult);
	
	return;
}

// end file CstiUpdateWriter.cpp
