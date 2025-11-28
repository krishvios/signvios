////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiExtendedEvent
//
//  File Name:	CstiExtendedEvent.cpp
//
//  Owner:		Eugene R. Christensen
//
//	Abstract:
//      The extended event class is an event that can be used to pass messages
// 		that need to send more than a simple message enumeration.  It contains
//		two additional member variables, both 32-bit integers.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "CstiExtendedEvent.h"

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
//; CstiExtendedEvent::CstiExtendedEvent
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
CstiExtendedEvent::CstiExtendedEvent (
	int32_t n32Event,	// This is the event to be passed
	stiEVENTPARAM Param0,	// The first value to store with the event
	stiEVENTPARAM Param1)	// The second value to store with the event
:
	CstiEvent (n32Event, Param0),
	m_Param1 (Param1)
{
	stiLOG_ENTRY_NAME (CstiExtendedEvent::CstiExtendedEvent);
	
	m_nMaxParams = 2;

} // end CstiExtendedEvent::CstiExtendedEvent


// end file CstiExtendedEvent.cpp
