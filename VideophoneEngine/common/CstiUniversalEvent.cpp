////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiUniversalEvent
//
//  File Name:	CstiUniversalEvent.cpp
//
//  Owner:		Eugene R. Christensen
//
//	Abstract:
//		The universal event class is an event that can be used to pass any kind
// 		of message.  It contains two additional member variables, a 32-bit
//		integer and a void pointer.  The void pointer can be used to pass the
//		address of any kind of data from one task to another.  When using this
//		class, make sure that anything the void pointer points to, is persistent
//		so it will still exist when the receiving task processes the event.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "CstiUniversalEvent.h"

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
// Locals
//

//
// Class Definitions
//

////////////////////////////////////////////////////////////////////////////////
//; CstiUniversalEvent::CstiUniversalEvent
//
// Description: Constructor
//
// Abstract:
//	This constructor has initial values passed into it to set the member
//  variables.
//
// Returns:
//	none
//
CstiUniversalEvent::CstiUniversalEvent (
	int32_t n32Event,	// This is the event to be passed
	stiEVENTPARAM Param0,	// The value to store with the event
	stiEVENTPARAM Param1,		// Pointer to an object to be stored
	stiEVENTPARAM Param2)		// Pointer to an associated call object
:
	CstiExtendedEvent (n32Event, Param0, Param1),
	m_Param2 (Param2)
{
	stiLOG_ENTRY_NAME (CstiUniversalEvent::CstiUniversalEvent);
	
	m_nMaxParams = 3;
} // end CstiUniversalEvent::CstiUniversalEvent


// end file CstiUniversalEvent.cpp
