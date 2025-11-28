////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiUniversalEvent
//
//  File Name:	CstiUniversalEvent.h
//
//  Owner:		Eugene R. Christensen
//
//	Abstract:
//		See CstiUniversalEvent.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIUNIVERSALEVENT_H
#define CSTIUNIVERSALEVENT_H

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
// Class Declaration
//

class CstiUniversalEvent : public CstiExtendedEvent
{
public:
	CstiUniversalEvent (
		int32_t n32Event, 
		stiEVENTPARAM Param0,
		stiEVENTPARAM Param1,
		stiEVENTPARAM Param2);
		
	inline stiEVENTPARAM ParamGet (
		int nParam);

protected:
	
	stiEVENTPARAM m_Param2;
};


////////////////////////////////////////////////////////////////////////////////
//; CstiUniversalEvent::ParamGet
//
// Description: Gets the event's 32-bit integer
//
// Abstract:
//
// Returns:
//	The event's 32-bit integer.
//
stiEVENTPARAM CstiUniversalEvent::ParamGet (
	int nParam)
{
	stiLOG_ENTRY_NAME (CstiUniversalEvent::ParamGet);

	stiEVENTPARAM Param;
	
	if (nParam < 2)
	{
		Param = CstiExtendedEvent::ParamGet (nParam);
	}
	else
	{
		stiASSERT (nParam < m_nMaxParams);
		
		Param = m_Param2;
	}
	
	return (Param);
} // end CstiUniversalEvent::ParamGet


#endif // CSTIUNIVERSALEVENT_H
// end file CstiUniversalEvent.h
