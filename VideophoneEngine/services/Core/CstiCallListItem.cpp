///
/// \file CstiCoreRequest.cpp
/// \brief Definition of the class that implements a single call list item.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2004-2012 by Sorenson Communications, Inc. All rights reserved.
///


//
// Includes
//
#include "stiDefs.h"
#include "CstiCallListItem.h"

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
// Function Declarations
//


/**
 * \brief Gets the CallListItem name value
 * \return The Name value
 */
const char * CstiCallListItem::NameGet () const
{
	return m_Name;
}

/**
 * \brief Sets the CallListItem name value
 * \param pszName Name value
 */
void CstiCallListItem::NameSet (const char * pszName)
{
	m_Name = pszName;
}

/**
 * \brief Gets the CallListItem DialString value
 * \return The dial string value
 */
const char * CstiCallListItem::DialStringGet () const
{
	return m_DialString;
}

/**
 * \brief Sets the CallListItem DialString value
 * \param pszDialString The dial string value
 */
void CstiCallListItem::DialStringSet (const char *pszDialString)
{
	m_DialString = pszDialString;
}

/**
 * \brief Gets the CallListItem ItemId value
 * \return The item id value
 */
const char * CstiCallListItem::ItemIdGet () const
{
	return m_ItemId;
}

/**
 * \brief Sets the CallListItem ItemId value
 * \param pszDialString The item id value
 */
void CstiCallListItem::ItemIdSet (const char *pszItemId)
{
	m_ItemId = pszItemId;
}

/**
 * \brief Gets the CallListItem DialMethod value
 * \return The dial method
 */
EstiDialMethod CstiCallListItem::DialMethodGet () const
{
	return m_eDialMethod;
}

/**
 * \brief Sets the CallListItem DialMethod value
 * \param eDialMethod The dial method value
 */
void CstiCallListItem::DialMethodSet (EstiDialMethod eDialMethod)
{
	m_eDialMethod = eDialMethod;
}

/**
 * \brief Gets the CallListItem call time value
 * \return The ttCallTime
 */
time_t CstiCallListItem::CallTimeGet () const
{
	return m_ttCallTime;
}

/**
 * \brief Sets the CallListItem call time value
 * \param ttCallTime The call time value
 */
void CstiCallListItem::CallTimeSet (time_t ttCallTime)
{
	m_ttCallTime = ttCallTime;
}

/**
 * \brief Gets the CallListItem RingTone value
 * \return The RingTone
 */
int CstiCallListItem::RingToneGet () const
{
    return m_nRingTone;
}

/**
 * \brief Sets the CallListItem RingTone value
 * \param nRingTone The ring tone value
 */
void CstiCallListItem::RingToneSet (int nRingTone)
{
    m_nRingTone = nRingTone;
}

/**
 * \brief Gets the InSpeedDial setting
 * \return estiTRUE If InSpeedDial, otherwise estiFALSE
 */
EstiBool CstiCallListItem::InSpeedDialGet () const
{
	return m_bInSpeedDial;
}

/**
 * \brief Sets the InSpeedDial setting
 * \param bInSpeedDial The boolean InSpeedDial value
 */
void CstiCallListItem::InSpeedDialSet (EstiBool bInSpeedDial)
{
	m_bInSpeedDial = bInSpeedDial;
}
	

/**
 * \brief Gets the CallPublicId 
 * \return The CallPublicId
 */
const char * CstiCallListItem::CallPublicIdGet () const
{
	return m_CallPublicId;
}

/**
 * \brief Sets the CallPublicId 
 * \param pszCallPublicId The CallPublicId value
 */
void CstiCallListItem::CallPublicIdSet (const char *pszCallPublicId)
{
	m_CallPublicId = pszCallPublicId;
}


/**
 * \brief Gets the IntendedDialString 
 * \return The IntendedDialString
 */
const char * CstiCallListItem::IntendedDialStringGet () const
{
	return m_IntendedDialString;
}

/**
 * \brief Sets the IntendedDialString 
 */
void CstiCallListItem::IntendedDialStringSet (const char *pszIntendedDialString)
{
	m_IntendedDialString = pszIntendedDialString;
}


/*! 
 * \brief Sets the duration
 */
void CstiCallListItem::DurationSet (
	int nDuration)		///< Duration in seconds
{
	m_nDuration = nDuration;
}


/*! 
 * \brief Gets the duration
 */
int CstiCallListItem::DurationGet () const
{
	return m_nDuration;
}

/**
 * \brief Gets the BlockedCallerID setting
 * \return estiTRUE If BlockedCallerID, otherwise estiFALSE
 */
EstiBool CstiCallListItem::BlockedCallerIDGet () const
{
	return m_bBlockedCallerID;
}

/**
 * \brief Sets the BlockedCallerID setting
 * \param bBlockedCallerID The boolean BlockedCallerID value
 */
void CstiCallListItem::BlockedCallerIDSet (EstiBool bBlockedCallerID)
{
	m_bBlockedCallerID = bBlockedCallerID;
}

/**
 * \brief Gets the CallListItem VRS Call ID value
 * \return The VRS Call Id value
 */
std::string CstiCallListItem::vrsCallIdGet () const
{
	return m_vrsCallId;
}

/**
 * \brief Sets the CallListItem VRS Call ID value
 * \param vrsCallId VRS Call ID value
 */
void CstiCallListItem::vrsCallIdSet (const std::string &vrsCallId)
{
	m_vrsCallId = vrsCallId;
}

// end file CstiCallListItem.cpp
