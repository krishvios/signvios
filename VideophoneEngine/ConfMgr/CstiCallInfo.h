/*!
 * \file CstiCallInfo.h
 * \brief Contains either local or remote information about a given call.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once


//
// Includes
//
#include "ICallInfo.h"
#include "stiSVX.h"
#include "stiError.h"


//
//  Class Declaration
//
class CstiCallInfo : public ICallInfo
{
public:
	CstiCallInfo () = default;

	CstiCallInfo (const CstiCallInfo &other) = delete;
	CstiCallInfo (CstiCallInfo &&other) = delete;
	CstiCallInfo &operator= (const CstiCallInfo &other) = delete;
	CstiCallInfo &operator= (CstiCallInfo &&other) = delete;

	~CstiCallInfo () override = default;


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
	void UserIDGet (
		std::string *userID) const override;

	void GroupUserIDGet (
		std::string *groupUserID) const override;

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
	stiHResult UserIDSet (
		const std::string &userId);

	stiHResult GroupUserIDSet (
		const std::string &groupUserId);

	/*! \brief UserPhoneNumbersGet 
	*  Gets the current Users phone numbers.
	*  phone number are:
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
	const SstiUserPhoneNumbers *UserPhoneNumbersGet () const override;

	/*! \brief UserPhoneNumbersSet 
	*  Sets the current users phone numbers.
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
	void UserPhoneNumbersSet (
		const SstiUserPhoneNumbers *psUserPhoneNumbers);
	
private:
	
	std::string m_userID;
	std::string m_groupUserID;
	SstiUserPhoneNumbers m_sUserPhoneNumbers{};
};
