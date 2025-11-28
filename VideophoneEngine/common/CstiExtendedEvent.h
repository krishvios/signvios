////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiExtendedEvent
//
//  File Name:	CstiExtendedEvent.h
//
//  Owner:		Eugene R. Christensen
//
//	Abstract:
//		See CstiExtendedEvent.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIEXTENDEDEVENT_H
#define CSTIEXTENDEDEVENT_H

//
// Includes
//
#include "CstiEvent.h"

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

class CstiExtendedEvent : public CstiEvent
{
public:
	CstiExtendedEvent (
		int32_t n32Event, 
		stiEVENTPARAM Param0,
		stiEVENTPARAM Param1);
		
	inline stiEVENTPARAM ParamGet (
		int nParam);

protected:
	
	stiEVENTPARAM m_Param1;

};


////////////////////////////////////////////////////////////////////////////////
//; CstiExtendedEvent::ParamGet
//
// Description: Gets one of the event's parameters.
//
// Abstract:
//
// Returns:
//	The specified events parameter
//
stiEVENTPARAM CstiExtendedEvent::ParamGet (
	int nParam)
{
	stiLOG_ENTRY_NAME (CstiExtendedEvent::ParamGet);

	stiEVENTPARAM Param;
	
	if (nParam == 0)
	{
		Param = CstiEvent::ParamGet ();
	}
	else
	{
		stiASSERT (nParam < m_nMaxParams);
		
		Param = m_Param1;
	}
	
	return (Param);
} // end CstiExtendedEvent::ParamGet


#endif // CSTIEXTENDEDEVENT_H
// end file CstiExtendedEvent.h
