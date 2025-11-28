/*!
 *
 *\file IstiBlockListManager.h
 *\brief Declaration of the BlockListManager interface
 *
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef ISTIBLOCKLISTMANAGER_H
#define ISTIBLOCKLISTMANAGER_H


#include <string>
#include "ISignal.h"
#include "stiSVX.h"


/*!
 * \ingroup VideophoneEngineLayer 
 * \brief Manages the blocked call list. 
 *  
 */
class IstiBlockListManager
{
public:

	/*!
	 * \brief  Determines if the Block List is enabled for use. 
	 * 
	 * \return bool 
	 */
	virtual bool EnabledGet () = 0;


	/*!
	 * \brief  Sets whether the Block List is enabled for use. 
	 */
	virtual void EnabledSet (bool bEnabled) = 0;


	/*!
	 * \brief Adds an item to the block list.
	 *  
	 * The manager will search its list for the number.  If the number is not found then the
	 * manager will add the number to the list and send a core request to add the number to core.
	 *
	 * If the number is found then an error is returned.
	 *
	 * If the list already contains the maximum number of entries, an error is returned.
	 *
	 * A core request shall be issued to add the blocked to number to core.  The request id for
	 * this request is returned to the caller.  Once the request has been completed the block list
	 * manager shall call the callback with a message and the request id indicating the request is
	 * complete.  The caller of this method may use this to monitor the completion of
	 * the asynchronous process.
	 *  
	 * \param pszBlockedNumber The number to add to the list.           
	 * \param pszDescription   A description to associate to the number.
	 * \param punRequestId   A request id for this operation.         
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult ItemAdd (
		const char *pszBlockedNumber,
		const char *pszDescription,
		unsigned int *punRequestId) = 0;


	/*!
	 * \brief Deletes an item from the block list. 
	 * 
	 * The manager will search its list for the number.  If the number is found then the manager
	 * will remove the number from the list and send a core request to remove the number from
	 * core.
	 *
	 * A core request shall be issued to delete the blocked number from core.  The request id
	 * for this request is returned to the caller.  Once the request has been completed the
	 * block list manager shall call the callback with a message and the request id indicating
	 * the request is complete.  The caller of this method may use this to monitor the completion
	 * of the asynchronous process.
	 *
	 * \param pszBlockedNumber The number to remove from the block list. 
	 * \param punRequestId   A request id for this operation.          
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult ItemDelete (
		const char *pszBlockedNumber,
		unsigned int *punRequestId) = 0;

	/*!
	 * \brief Changes the block list entry to the new values specified. 
	 *
	 * The manager will search its list for the old number.  If the old number is found then the
	 * manager will replace the old number with the new number and send a core request to core to
	 * update the number.
	 *
	 * A core request shall be issued to edit the blocked number in core.  The request id for this
	 * request is returned to the caller.  Once the request has been completed the block list
	 * manager shall call the callback with a message and the request id indicating the request
	 * is complete.  The caller of this method may use this to monitor the completion of the
	 * asynchronous process.
	 * 
	 * \param pszItemId The item ID of the block list item you wish to edit.     
	 * \param pszNewBlockedNumber The new number.  This may be NULL if the number is not to be changed.                      
	 * \param pszNewDescription   The new description for the number.  This may be NULL if the description is not be changed.
	 * \param punRequestId      A request id for this operation.                                                           
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult ItemEdit (
		const char *pszItemId,
		const char *pszNewBlockedNumber,
		const char *pszNewDescription,
		unsigned int *punRequestId) = 0;

	/*!
	 * \brief Finds a blocked list entry by phone number. 
	 *
	 * The manager will search its list for the number.  If the number is found then estiTRUE
	 * is returned.  Otherwise, estiFALSE is returned.
	 *
	 * \param pszBlockedNumber The phone number used to identify the entry 
	 * \param pItemId        The item ID associated with the entry       
	 * \param pDescription   The description associated to the entry.    
	 * 
	 * \return bool
	 */
	virtual bool ItemGetByNumber (
		const char *pszBlockedNumber,
		std::string *pItemId,
		std::string *pDescription) const = 0;

	/*!
	 * \brief Finds a blocked list entry by item id. 
	 *
	 * The manager will search its list for the id.  If the number is found then estiTRUE
	 * is returned.  Otherwise, estiFALSE is returned.
	 *
	 * \param pszItemId       The item ID to be returned.              
	 * \param pBlockedNumber The phone number associated to the entry.
	 * \param pDescription   The description associated to the entry. 
	 * 
	 * \return bool
	 */
	virtual bool ItemGetById (
		const char *pszItemId,
		std::string *pBlockedNumber,
		std::string *pDescription) const = 0;

	/*!
	 * \brief Finds a blocked list entry by index. 
	 *
	 * This method is used to iterate through the list.  An object should lock the list before
	 * iterating through the list to protect the list from changing during the iteration process.
	 * 
	 * \param nIndex                       The index used to identify the entry                            
	 * \param pItemId        The item ID associated with the entry                           
	 * \param pBlockedNumber The phone number associated for the entry at the provided index 
	 * \param pDescription   The description associated to the blocked number.               
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult ItemGetByIndex (
		int nIndex,
		std::string *pItemId,
		std::string *pBlockedNumber,
		std::string *pDescription) const = 0;

	/*!
	 * \brief Locks the list so that no other thread can make changes. 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult Lock () const = 0;

	/*!
	 * \brief Unlocks the list allowing other threads to make changes. 
	 */
	virtual void Unlock () const = 0;

	/*!
	 * \brief Sets the maximum number of entries allowed in the list. 
	 * 
	 * The block list manager is informed of the maxium length through this method. Setting a
	 * maximum length shorter than the current list length shall succeed.  Entries beyond the
	 * maximum shall be maintained.  New entries shall be disallowed.
	 *
	 * \param nMaxLength 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult MaximumLengthSet (
		int nMaxLength) = 0;

	/*!
	 * \brief Gets the maximum number of entries allowed in the list.
	 *
	 * \return unsigned int
	 */
	virtual unsigned int MaximumLengthGet () const = 0;

	/*!
	 * \brief Reloads the list from Core.
	 *
	 */
	virtual void Refresh () = 0;
	
	/*!
	 * \brief Removes ALL list items
	 */
	virtual void PurgeItems() = 0;


	enum Response
	{
		Unknown,		//! Undefined response
		AddError,		//! Adding a new blocked number failed
		EditError,		//! Editing a blocked number failed
		DeleteError,	//! Unblocking a number failed
		GetListError,	//! Retrieving the block list failed
		Denied,			//! The number being blocked is on the white list.
	};

	virtual ISignal<Response, stiHResult, unsigned int, const std::string &>& errorSignalGet() = 0;
};

#endif // ISTIBLOCKLISTMANAGER_H
