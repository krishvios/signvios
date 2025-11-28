/*!
 *
 *\file IstiCallHistoryManager.h
 *\brief Declaration of the CallHistoryManager interface
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef ISTICALLHISTORYMANAGER_H
#define ISTICALLHISTORYMANAGER_H


#include "CstiCallList.h"
#include "CstiCallHistoryItem.h"
#include "CstiItemId.h"


/*!
 * \ingroup VideophoneEngineLayer
 * \brief Manages all of the Call History lists
 *
 */
class IstiCallHistoryManager
{
public:

	virtual CstiCallHistoryItemSharedPtr ItemCreate() = 0;

	/*!
	 * \brief Adds an item to the call history list.
	 *
	 * A core request shall be issued to add the number to core.  The request id for
	 * this request is returned to the caller.  Once the request has been completed the call history list
	 * manager shall call the callback with a message and the request id indicating the request is
	 * complete.  The caller of this method may use this to monitor the completion of
	 * the asynchronous process.
	 *
	 * \param eCallListType The call list to add the item to
	 * \param spCallListItem The item being added
	 * \param bBlockedCallerID Caller ID should be blocked status.
	 * \param punRequestId A request id for this operation.
	 *
	 * \return stiHResult
	 */
	virtual stiHResult ItemAdd (
		CstiCallList::EType eCallListType,
		CstiCallHistoryItemSharedPtr spCallListItem,
		bool bBlockedCallerID,
		unsigned int *punRequestId) = 0;


	/*!
	 * \brief Deletes an item from the call history list.
	 *
	 * A core request shall be issued to delete the item from core.  The request id
	 * for this request is returned to the caller.  Once the request has been completed the
	 * call history list manager shall call the callback with a message and the request id indicating
	 * the request is complete.  The caller of this method may use this to monitor the completion
	 * of the asynchronous process.
	 *
	 * \param eCallListType The call list to add the item to
	 * \param ItemId Id of the item being deleted
	 * \param punRequestId   A request id for this operation.
	 *
	 * \return stiHResult
	 */
	virtual stiHResult ItemDelete (
		CstiCallList::EType eCallListType,
		const CstiItemId &ItemId,
		unsigned int *punRequestId) = 0;


	/*!
	* Finds a call list entry by index.
	*
	* This method is used to iterate through the list.  An object should lock the list before
	* iterating through the list to protect the list from changing during the iteration process.
	*
	* \param eCallListType The Call List to search
	* \param nIndex The index used to identify the entry
	* \param spCallListItem The found item
	*/
	virtual stiHResult ItemGetByIndex (
		CstiCallList::EType eCallListType,
		int nIndex,
		CstiCallHistoryItemSharedPtr *pspCallListItem) const = 0;

	/*!
	 * \brief Clears a call history list
	 *
	 * \param eCallListType The Call List to clear
	 *
	 * \return stiHResult
	 */
	virtual stiHResult ListClear (
		CstiCallList::EType eCallListType) = 0;

	/*!
	 * \brief Clears all lists
	 *
	 * \return stiHResult
	 */
	virtual stiHResult ListClearAll () = 0;

	/*!
	 * \brief Clears the number of new missed calls
	 *
	 * \return stiHResult
	 */
	virtual stiHResult MissedCallsClear () = 0;

	/*!
	 * \brief Gets the current size of the list
	 *
	 * \param eCallListType The Call List to query
	 * \return The size of the list
	 */
	virtual unsigned int ListCountGet (
		CstiCallList::EType eCallListType) = 0;

	/*!
	 * \brief Gets the number of unviewed items in the list
	 *
	 * \param eCallListType The Call List to query
	 * \return The number of unviewed items in the list
	 */
	virtual unsigned int UnviewedItemCountGet (
		CstiCallList::EType eCallListType) = 0;

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
	 * The call history list manager is informed of the maxium length through this method. Setting a
	 * maximum length shorter than the current list length shall succeed.  Entries beyond the
	 * maximum shall be maintained.  New entries shall be disallowed.
	 *
	 * \param eCallListType The Call List to set
	 * \param nMaxLength The maximum size of the list
	 *
	 * \return stiHResult
	 */
	virtual stiHResult MaxCountSet (
		CstiCallList::EType eCallListType,
		int nMaxLength) = 0;

	virtual void Refresh() = 0;

	virtual void CachePurge () = 0;

	virtual stiHResult ListUpdateAll () = 0;
};

#endif // ISTICALLHISTORYMANAGER_H

