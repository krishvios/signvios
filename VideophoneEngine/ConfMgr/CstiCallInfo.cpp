/*!
 * \file CstiCallInfo.cpp
 * \brief Contains either local or remote information about a given call.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
 */


//
// Includes
//
#include "stiError.h"
#include "stiTools.h"
#include "CstiCallInfo.h"


//
// Functions
//


/*! \brief UserIDGet
*  Gets the backend user ID.  
*  
* \param  char *pszBuffer 
* The buffer to copy the string into.  
* \param  int nBufferLength 
* The maximum length of the string to be copied including the null character.  
*  
* \note 
* The method returns the length of the string copied not including the null character. 
*  
* \return Returns the string length of the ID (buffer length)
*/
void CstiCallInfo::UserIDGet (
	std::string *userID) const
{
	if (userID)
	{
		*userID = m_userID;
	}
	else
	{
		stiASSERT(false);
	}
}


void CstiCallInfo::GroupUserIDGet (
	std::string *groupUserID) const
{
	if (groupUserID)
	{
		*groupUserID = m_groupUserID;
	}
	else
	{
		stiASSERT(false);
	}
}


/*! \brief UserIDset
*  Sets the backend user ID.  
*  
* \param  
* The maximum length of the string to be copied including the null character.  
*  
* \note 
* The method returns the length of the string copied not including the null character. 
*  
* \return Returns the string length of the ID (buffer length)
*/
stiHResult CstiCallInfo::UserIDSet (
	const std::string &userId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_userID = userId;

	return hResult;
}


stiHResult CstiCallInfo::GroupUserIDSet (
	const std::string &groupUserId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_groupUserID = groupUserId;

	return hResult;
}


/*! \brief UserPhoneNumbersGet 
*  Gets the current Users phone numbers.
*  Phone numbers are:
*     \li Preferred phone number         
*     \li Sorenson phone number          
*     \li Toll-free phone number         
*     \li local phone number                 
*     \li the hearing callers number
*  
* \note Will never return null, but phone numbers may contain empty "\0" strings.  
*  
* \return Returns a pointer to SstiUserPhoneNumbers 
* 
*/
const SstiUserPhoneNumbers *CstiCallInfo::UserPhoneNumbersGet () const
{
	return &m_sUserPhoneNumbers;
}


/*! \brief UserPhoneNumbersSet 
*  Sets the current users phone numbers.
*  This function will store a duplicate of the passed data, 
*  so the original value passed in can be safely deleted
*  
*  Phone numbers are:
*     \li Preferred phone number         
*     \li Sorenson phone number          
*     \li Toll-free phone number         
*     \li local phone number                 
*     \li the hearing callers number
*  
* \note Will never return null, but phone numbers may contain empty "\0" strings.  
*  
* \return Returns a pointer to SstiUserPhoneNumbers 
*/
void CstiCallInfo::UserPhoneNumbersSet (
	const SstiUserPhoneNumbers *psUserPhoneNumbers)
{
	m_sUserPhoneNumbers = *psUserPhoneNumbers;
}

