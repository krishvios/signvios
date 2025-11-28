/*!
 * \file ICallInfo.h
 * \brief Contains either local or remote information about a given call.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once


//
// Includes
//
#include "stiSVX.h"




/*! \class ICallInfo 
 *  
 * \brief contains information about a call 
 * \li Ringer pattern 
 * \li User ID 
 * \li Users phone numbers 
 * 		- Preferred phone number           
 * 		- Sorenson phone number            
 * 		- Toll-free phone number           
 * 		- local phone number               
 * 		- the hearing callers number       
 * 
 */
class ICallInfo
{
public:
	/*! \brief UserIDGet
	*  Gets the backend user ID.  
	*  
	* \param  pszBuffer 
	* Copy the UserID into this buffer.  
	* \param  nBufferLength 
	* The maximum length of the string to be copied including the null character.  
	*  
	* \note 
	* The method returns the length of the string copied not including the null character. 
	*  
	* \return Returns the string length of the ID (buffer length)
	*/
	virtual void UserIDGet (
		std::string *userID) const = 0;

	/*! \brief GroupUserIDGet
	 *  Gets the backend group user ID.
	 *
	 * \param  pszBuffer
	 * Copy the GroupUserID into this buffer.
	 * \param  nBufferLength
	 * The maximum length of the string to be copied including the null character.
	 *
	 * \note
	 * The method returns the length of the string copied not including the null character.
	 *
	 * \return Returns the string length of the ID (buffer length)
	 */
	virtual void GroupUserIDGet (
		std::string *grouUserID) const = 0;
	
	/*! \brief UserPhoneNumbersGet 
	*  Gets the current Users phone numbers.
	*  Phone numbers are:
	*     \li Preferred phone number         
	*     \li Sorenson phone number          
	*     \li Toll-free phone number         
	*     \li local phone number                 
	*     \li the hearing callers number 
	*  
	* \return Returns a pointer to SstiUserPhoneNumbers 
	*/
	virtual const SstiUserPhoneNumbers *UserPhoneNumbersGet () const = 0;

protected :
	/*!  \brief ~ICallInfo 
	*  
	*  Pervents accidental attempts to delete this.  
	*  (and fixes a warning with that idea in mind)  
	*/
	virtual ~ICallInfo() = default;
};

