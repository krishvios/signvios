////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Module Name: stiEventMap
//
//  File Name:  stiEventMap.h
//
//  Owner:		Eugene R. Christensen
//
//  Abstract:
//		This file defines macros for implementing Event Maps.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STIEVENTMAP_H
#define STIEVENTMAP_H

//
// Includes
//
#include "stiDefs.h"
#include "CstiTask.h"
#include "stiTrace.h"

//
// Constants
//
// The following macros are for implementation of an event map that contains
// two entities: The event and the handler for the event
#define stiDECLARE_EVENT_MAP(class) \
	typedef stiHResult (stiEVENTCALL class::*stiemEVENT_HANDLER)(void* poEvent);\
	typedef struct stiEVENT_MAP_ITEM\
	{\
		int32_t m_n32Event;\
		stiemEVENT_HANDLER m_pEventHandler;\
	} stiEVENT_MAP_ITEM;\
	static int m_nNumEventEntries;\
	static const stiEVENT_MAP_ITEM m_stiEventMap[];

#define stiEVENT_MAP_BEGIN(class) \
	const class::stiEVENT_MAP_ITEM class::m_stiEventMap[] = {

#define stiEVENT_DEFINE(event, proc) {event, (stiemEVENT_HANDLER)&proc}, /* NOLINT (bugprone-macro-parentheses) */

#define stiEVENT_MAP_END(class) };\
	int class::m_nNumEventEntries  = sizeof (class::m_stiEventMap) \
									/ sizeof (class::stiEVENT_MAP_ITEM);

#define stiNUM_EVENT_ENTRIES m_nNumEventEntries


//
// Common EventDoNow method
//

#if (APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4) && defined (stiDEBUGGING_TOOLS)
//
// If stiDEBUGGING_TOOLS is defined then this stiEVENT_DO_NOW
// will help troubleshoot events that take a long time.
// It will print out some info everytime the event takes 
// longer than g_stiTaskTimeoutValueDebug
//

#define stiEVENT_DO_NOW(class) \
stiHResult class::EventDoNow ( \
	CstiEvent* poEvent) \
{ \
	stiHResult hResult = stiRESULT_SUCCESS; \
\
	m_nCurrentEvent = (poEvent)->EventGet (); \
\
	if ((poEvent)->EventGet () != estiEVENT_NOP) \
	{ \
		int i{0}; \
\
		for (i = 0; i < stiNUM_EVENT_ENTRIES; ++i) \
		{ \
			if (m_stiEventMap[i].m_n32Event == (poEvent)->EventGet ()) \
			{ \
				auto pEventHandler = (stiemEVENT_HANDLER)m_stiEventMap[i].m_pEventHandler; \
\
				if (g_stiTaskTimeoutValueDebug) \
				{ \
					timeval start{}; \
					timeval end{}; \
					timeval elapsed{}; \
\
					gettimeofday (&start, NULL); \
\
					hResult = (this->*pEventHandler)(poEvent); \
\
					gettimeofday (&end, NULL); \
\
					timersub (&end, &start, &elapsed); \
\
					if (((elapsed.tv_sec * 1000000) + elapsed.tv_usec)  > g_stiTaskTimeoutValueDebug) \
					{ \
						stiTrace ("WARNING: Event = %d in Taskname = %s Took longer than %d usecs. Total time %d usecs\n", \
								  m_stiEventMap[i].m_n32Event, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()), \
								  static_cast<int>(g_stiTaskTimeoutValueDebug), (elapsed.tv_sec * 1000000) + elapsed.tv_usec); \
					} \
\
				} \
				else \
				{ \
					hResult = (this->*pEventHandler)(poEvent); \
				} \
				break; \
			} \
		} \
\
		stiTESTCOND (i < stiNUM_EVENT_ENTRIES, stiRESULT_ERROR); \
	} \
\
STI_BAIL: \
\
	m_nCurrentEvent = -1; \
\
	return (hResult);\
}

#else //if (APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4) && defined (stiDEBUGGING_TOOLS)

#define stiEVENT_DO_NOW(class) \
stiHResult class::EventDoNow ( \
	CstiEvent* poEvent) \
{ \
	stiHResult hResult = stiRESULT_SUCCESS; \
\
	m_nCurrentEvent = poEvent->EventGet (); \
\
	if (poEvent->EventGet () != estiEVENT_NOP) \
	{ \
		int i; \
\
		for (i = 0; i < stiNUM_EVENT_ENTRIES; ++i) \
		{ \
			if (m_stiEventMap[i].m_n32Event == poEvent->EventGet ()) \
			{ \
				auto pEventHandler = (stiemEVENT_HANDLER)m_stiEventMap[i].m_pEventHandler; \
\
				hResult = (this->*pEventHandler)(poEvent); \
				break; \
			} \
		} \
\
		stiTESTCOND (i < stiNUM_EVENT_ENTRIES, stiRESULT_ERROR); \
	} \
\
STI_BAIL: \
\
	m_nCurrentEvent = -1; \
\
	return (hResult);\
}

#endif


#define stiDECLARE_EVENT_DO_NOW(class) stiHResult EventDoNow (CstiEvent *poEvent);
#define stiDECLARE_EVENT_DO_NOW_OVERRIDE(class) stiHResult EventDoNow (CstiEvent *poEvent) override;

//
// Typedefs
//

// The following structure is implemented for an event map that contains
// two entities: The event and the handler for the event

// The following structure is implemented for an event map that contains
// three entities: The event, valid states and the handler for the event

// The following structure is implemented for an event map that contains
// three entities: The event, valid states and the handler for the event

//
// Forward Declarations
//

//
// Globals
//

//
// Function Declarations
//

#endif // STIEVENTMAP_H
// end file stiEventMap.h
