////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiUpdateWriter
//
//  File Name:	CstiUpdateWriter.h
//
//  Owner:		Eugene R. Christensen
//
//	Abstract:
//		See CstiUpdateWriter.cpp
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

//
// Includes
//
#include "CstiEventQueue.h"
#include "CstiSignal.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//
class CstiUpdateWriter : public CstiEventQueue
{
public:

	CstiUpdateWriter ();

	CstiUpdateWriter (const CstiUpdateWriter &other) = delete;
	CstiUpdateWriter (CstiUpdateWriter &&other) = delete;
	CstiUpdateWriter &operator= (const CstiUpdateWriter &other) = delete;
	CstiUpdateWriter &operator= (CstiUpdateWriter &&other) = delete;

	~CstiUpdateWriter () override;
	
	stiHResult Initialize (
		const char *pszFilename);
	
	stiHResult Startup ();
	
	CstiSignal<char *, int> bufferWrittenSignal;
	CstiSignal<char *> errorSignal;
	
	//
	// Methods called by the Update class
	//
	stiHResult bufferWrite (
		char *buffer,
		int bufferLength);

	stiHResult imageInitialize (
		bool append = false);

private:

	void eventBufferWrite (
		char * buffer,
	int bufferLength);
	
	void eventImageInitialize (
		bool append);
	
	char *m_pszFilename {nullptr};
	FILE *m_fd {nullptr};
	
};

// end file CstiUpdateWriter.h
