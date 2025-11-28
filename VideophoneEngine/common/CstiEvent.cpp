/*!
 * \file CstiEvent.cpp
 * \brief A basic event class.
 *
 * This is a base class that implements a very general message or event
 * being passed.  It is intended to be derived from to implement more
 * specific messages that include additional parameters etc.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

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
// Locals
//

//
// Class Definitions
//

/*! \brief Constructor
 *
 * This is a constructor that takes the event and call object as parameters.
 * It sets the event type to the value passed in.
 *
 * \return None
 */
CstiEvent::CstiEvent (
	int32_t n32Event)	//!< The event being handled, default=stiINVALID_EVENT.
	:
	m_n32Event (n32Event),
	m_nMaxParams (0),
	m_Param0  (0)
{
	stiLOG_ENTRY_NAME (CstiEvent::CstiEvent);
} // end CstiEvent::CstiEvent


CstiEvent::CstiEvent (
	int32_t n32Event,	//!< The event being handled, default=stiINVALID_EVENT.
	stiEVENTPARAM Param0)		//!< Pointer to the call object being stored, default == NULL.
	:
	m_n32Event (n32Event),
	m_nMaxParams (1),
	m_Param0  (Param0)
{
	stiLOG_ENTRY_NAME (CstiEvent::CstiEvent);
} // end CstiEvent::CstiEvent


/*! \brief Destructor
 *
 * This is the default destructor.
 *
 * \return None
 */
CstiEvent::~CstiEvent ()
{
	stiLOG_ENTRY_NAME (CstiEvent::~CstiEvent);
} // end CstiEvent::~CstiEvent


/*! \brief Return the Event.
 *
 * This method returns the event type.
 * \return The event being handled.  
 */
int32_t CstiEvent::EventGet ()
{
	stiLOG_ENTRY_NAME (CstiEvent::EventGet);

	return (m_n32Event);
} // end CstiEvent::EventGet


// end file CstiEvent.cpp
