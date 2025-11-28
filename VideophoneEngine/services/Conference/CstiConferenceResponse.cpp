/*!
 * \file CstiConferenceResponse.cpp
 * \brief Implementation of the CstiConferenceResponse class.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//

#include "CstiConferenceResponse.h"

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

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceResponse::CstiConferenceResponse
//
// Description: 
//
// Abstract:
//
// Returns:
//
CstiConferenceResponse::CstiConferenceResponse (
	CstiVPServiceRequest *request,
	EResponse eResponse,
	EstiResult eResponseResult,
	EResponseError eError,
	const char *pszError)
:
	CstiVPServiceResponse(request, eResponseResult, pszError),
	m_eResponse (eResponse),
	m_eError (eError),
	m_bAddAllowed (false),
	m_nActiveParticipants (0),
	m_nAllowedParticipants (0)
{
	stiLOG_ENTRY_NAME (CstiConferenceResponse::CstiConferenceResponse);
} // end CstiConferenceResponse::CstiConferenceResponse


/*! \brief CstiConferenceResponse::AddAllowedGet
 * Does the conference room allow another participant?
 * This could be set to false due to the number in the room
 * or based on the amount of resources the MCU actually has remaining.
 *
 * \return - true if another participant is allowed, otherwise false.
 */
bool CstiConferenceResponse::AddAllowedGet () const
{
	return m_bAddAllowed;
}


/*! \brief CstiConferenceResponse::AddAllowedSet
 * Sets whether or not another participant can be added to the room.
 *
 * \param bSet = (true or false)
 * \return - none
 */
void CstiConferenceResponse::AddAllowedSet (
	bool bSet)
{
	m_bAddAllowed = bSet;
}


/*! \brief CstiConferenceResponse::ActiveParticipantsGet
 * Returns the number of participants already in the conference room.
 *
 * \returns - the number of participants in the conference room.
 */
int CstiConferenceResponse::ActiveParticipantsGet () const
{
	return m_nActiveParticipants;
}


/*! \brief CstiConferenceResponse::ActiveParticipantsSet
 * Update the number of participants in the room
 *
 * \param - nCount = the number of participants now in the room.
 * \return - none
 */
void CstiConferenceResponse::ActiveParticipantsSet (
	int nCount)
{
	m_nActiveParticipants = nCount;
}


/*! \brief CstiConferenceResponse::AllowedParticipantsGet
 * Returns the number of allowed participants in the conference room.
 *
 * \returns - the number of participants allowed in this conference room.
 */
int CstiConferenceResponse::AllowedParticipantsGet () const
{
	return m_nAllowedParticipants;
}


/*! \brief CstiConferenceResponse::AllowedParticipantsSet
 * Updates the number of allowed participants in the conference room.
 *
 * \param - nCount = the number allowed in the conference room.
 * \return none
 */
void CstiConferenceResponse::AllowedParticipantsSet (
	int nCount)
{
	m_nAllowedParticipants = nCount;
}


/*! \brief CstiConferenceResponse::ParticipantListGet
 * Obtains a list of participants in the conference room.
 *
 * \return - a pointer to the list of current participants.
 */
const std::list<SstiGvcParticipant> * CstiConferenceResponse::ParticipantListGet () const
{
	return &m_ParticipantList;
}


/*! \brief CstiConferenceResponse::ParticipantListAddItem
 * Adds a participant to the list in the conference room.
 *
 * \param - pstParticipant is a pointer to the participan information being added.
 * \return - an stiHResult signifying success or failure.
 */
stiHResult CstiConferenceResponse::ParticipantListAddItem (SstiGvcParticipant *pstParticipant)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (pstParticipant, stiRESULT_ERROR);

	if (pstParticipant)
	{
		m_ParticipantList.push_back (*pstParticipant);
	}

STI_BAIL:

	return hResult;
}

// end file CstiConferenceResponse.cpp
