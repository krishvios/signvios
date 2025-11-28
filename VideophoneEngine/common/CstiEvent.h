/*!
 * \file CstiEvent.h
 * \brief See CstiEvent.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CSTIEVENT_H
#define CSTIEVENT_H

//
// Includes
//
#include "stiDefs.h"
#include <cstdio>
#include "stiAssert.h"
#include "stiTrace.h"

//
// Constants
//
const int32_t stiINVALID_EVENT = 0xFFFFFFFF;

//
// Typedefs
//
typedef intptr_t stiEVENTPARAM;

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//
class CstiEvent
{
public:
	CstiEvent (
		int32_t n32Event);
		
	CstiEvent (
		int32_t n32Event,
		stiEVENTPARAM Param);
		
	virtual ~CstiEvent ();

	int32_t EventGet ();
	inline void EventSet (int32_t n32Event);
	
	inline stiEVENTPARAM ParamGet ();

protected:
	int32_t m_n32Event;
	
	int m_nMaxParams;
	stiEVENTPARAM m_Param0;
};


/*! \brief Sets the Event.
 *
 * Takes the parameter passed in and sets the event type.
 *
 * \return None
 */
void CstiEvent::EventSet (
	IN int32_t n32Event) //!< The event to be sent.
{
	stiLOG_ENTRY_NAME (CstiEvent::EventSet);

	m_n32Event = n32Event;
} // end CstiEvent::EventSet ()


stiEVENTPARAM CstiEvent::ParamGet ()
{
	stiLOG_ENTRY_NAME (CstiEvent::ParamGet);

	stiASSERT (m_nMaxParams > 0);
	
	return (m_Param0);
} // end CstiEvent::ParamGet


#endif // CSTIEVENT_H
// end file CstiEvent.h
