/*!
 * \file IstiMessageManager.h
 * \brief The Message List Manager Interface Class
 *
 * Class declaration for the Message Manager Interface.  Provides storage
 * and access for persitent (both local and remote) and temporary data.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2011 – 2012 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef ISTIMESSAGEMANAGER_H
#define ISTIMESSAGEMANAGER_H


#include "CstiItemId.h"
#include "CstiMessageInfo.h"
#include "ISignal.h"

/*!
 * \ingroup VideophoneEngineLayer 
 * \brief Message Manager Interface Class
 *
 */
class IstiMessageManager
{
public:
	
	IstiMessageManager (const IstiMessageManager & other) = delete;
	IstiMessageManager (IstiMessageManager && other) = delete;
	IstiMessageManager& operator= (const IstiMessageManager & other) = delete;
	IstiMessageManager& operator= (IstiMessageManager && other) = delete;

	/*!
	 * \brief Locks the Message Manager so that its lists cannot be manipulated 
	 *		  by other processes. 
	 *  
	 */
	virtual stiHResult Lock () const = 0;

	/*!
	 * \brief Unlocks the Message Manager so that its lists can be manipulated 
	 *		  by other processes. 
	 *  
	 */
	virtual void Unlock () const = 0;

	/*!
	 * \brief Gets the message category by a CstiItemId
	 *  
	 * \param categoryId is a referance to a CstiItemId that inidicates the category to retieve.
	 * \param punIndex is a pointer to an unsigned int that will receive the index of the category.
	 * \param pnCategoryType is a pointer to an integer that will receive the Categories type.
	 * \param pszCatShortName is a buffer allocated by the UI that will recieve the short name of the category.
	 * \param nShortBufSize is the size of a pszCatShortName buffer.
	 * \param pszCatLongName is a buffer allocated by the UI that will recieve the long name of the category.
	 * \param nLongBufSize is the size of a pszCatLongName buffer.
	 * 
	 * \return stiRESULT_SUCCESS if successful, 
	 * 		   stiRESULT_INVALID_PARAMETER if categoryId is invalid,
	 * 		   stiRESULT_BUFFER_TOO_SMALL if pszCatShortName or pszCatLongName buffers are to small.
	 * 		   stiRESULT_ERROR if the category is not found.
	 */
	virtual stiHResult CategoryByIdGet (
		const CstiItemId &categoryId, 
		unsigned int *punIndex, 
		int *pnCategoryType, 
		char *pszCatShortName, unsigned int nShortBufSize, 
		char *pszCatLongName, unsigned int nLongBufSize) const = 0;

	/*!
	 * \brief Returns the category based on the index passed in. 
	 *  
	 * \param  unIndex is an unsigned int that specifies the index of the desired category.
	 * \param  pCategoryId is a pointer to a CstiItemId which receives a pointer to the categories Id.
	 *  
	 * \return stiRESULT_SUCCESS if successful,
	 *		   stiRESULT_INVALID_PARAMETER if unIndex is larger than the number of categories in the category list.
	 */
	virtual stiHResult CategoryByIndexGet (
		unsigned int unIndex, 
		CstiItemId **ppCategoryId) const = 0;

	/*!
	 * \brief Gets number of categories in the category list 
	 *  
	 * \param punCategoryCount is a pointer to an integer used to return the
	 *              number of categories the Message Manger currently contains.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	virtual stiHResult CategoryCountGet (
		unsigned int *punCategoryCount) const = 0;
	
	/*!
	 * \brief Get the ID of a categories parent. 
	 *  
	 * \param categoryId is a referance to a CstiItemId that inidicates the category to use to get the parent.
	 * \param pParentId is a pointer to a CstiItemId that will be set to the Id of the parent, the UI is responsible for
	 *              cleaning up the memory allocated for the pParentId.
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 *         stiRESULT_INVALID_PARAMETER if the category matching categoryId could not be found. 
	 * 		   pParentId will be NULL if pCategoryId does not have a parent.  
	 */
	virtual stiHResult CategoryParentGet (
		const CstiItemId &categoryId, 
		CstiItemId **ppParentId) const = 0;

	/*!
	 * \brief Updates the time of the last category update.
	 */
	virtual void lastCategoryUpdateSet() = 0;

	/*!
	 * \brief Updates the time of the signMail update. 
	 */
	virtual void lastSignMailUpdateSet() = 0;

	/*!
	 * \brief Populates a CstiMessageItem based on the categoryId and index. 
	 *  
	 * \param categoryId is a referance to a CstiItemId object that specifies a category.
	 * \param nIndex is a pointer to an integer used to return the
	 * \param pMessageItem is a CstiMessageItem that is passed in and the Message Manager
	 * 		  will populate its data members.
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 * 		   stiRESULT_INVALID_PARAMETER if categoryId is invalid, not found in the list or 
	 * 		   							   unIndex is larger than the number of messages in the list,
	 * 		   stiRESULT_ERROR otherwise. 
	 */
	virtual stiHResult MessageByIndexGet (
		const CstiItemId &categoryId,
		unsigned int nIndex,
		CstiMessageInfo *pMessageItem) const = 0;
	
	/*!
	 * \brief Get the category ID of a message. 
	 *  
	 * \param messageId is a referance to a CstiItemId that inidicates the message to use to get the category.
	 * \param pCategoryId is a pointer to a CstiItemId that will be set to the Id of the Category, the UI is responsible for
	 *        cleaning up the memory allocated for the pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 *         stiRESULT_INVALID_PARAMETER if the message matching message ID could not be found. 
	 */
	virtual stiHResult MessageCategoryGet (
		const CstiItemId &messageId, 
		CstiItemId **ppCategoryId) const = 0;
	
	/*!
	 * \brief Returns the number of messages that a category contains. 
	 *  
	 * \param categoryId is a referance to a CstiItemId object that specifies a category.
	 * \param punMessageCount is a pointer to an integer used to return the
	 *              number of messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	virtual stiHResult MessageCountGet (
		const CstiItemId &categoryId,
		unsigned int *punMessageCount) const = 0;
	
	/*!
	 * \brief Gets the data of a CstiMessageItem based on the categoryId and itemId 
	 * 		  contained in the CstiMessageItem. 
	 *  
	 * \param pMessageItem is a CstiMessageItem that is passed in with a valid itemId (CstiItemId)
	 *  	        for the desired message.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	virtual stiHResult MessageInfoGet (
		CstiMessageInfo *pMessageItem) const = 0;
	
	/*!
	 * \brief Sets the data contained in the CstiMessageItem, based on the categoryId and itemId 
	 * 		  of the CstiMessageItem, on the message object maintained by the Message Manager. 
	 *  
	 * \param pMessageItem is a CstiMessageItem that is passed in with a valid itemId (CstiItemId)
	 *  	        and all other data members set to the desired state.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	virtual stiHResult MessageInfoSet (
		CstiMessageInfo *pMessageItem) = 0;

	/*!
	 * \brief Deletes the message item, specified by the msgItemId, from the message server.
	 *  
	 * \param msgItemId is a referance to the unique ID of the message item that should be deleted from the message server
	 * 
	 * \return stiRESULT_SUCCESS if the delete request was successfully sent.
	 */
	virtual stiHResult MessageItemDelete (
		const CstiItemId &msgItemId) = 0;

	/*!
	 * \brief Deletes all messages in the category specified by the categoryId.
	 *  
	 * \param pCategoryId is the unique ID of the category that should have its messages deleted from the message server
	 * 
	 * \return stiRESULT_SUCCESS if the delete request was successfully sent.
	 */
	virtual stiHResult MessagesInCategoryDelete (
		const CstiItemId &categoryId) = 0;

	/*!
	 * \brief Returns the number of new messages that a category contains. 
	 *  
	 * \param categoryId is a referance to a CstiItemId object that specifies a category.
	 * \param pnNewMessageCount is a pointer to an unsigned integer used to return the
	 *  	        number of new messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	virtual stiHResult NewMessageCountGet (
		const CstiItemId &categoryId,
		unsigned int *pnNewMessageCount) const = 0;
	
	/*!
	 * \brief Causes the message manager to re-request the message list.
	 */
	virtual void Refresh() = 0;

	/*!
	 * \brief Returns the number of SignMail messages that a category contains. 
	 *  
	 * \param punSignMailMessageCount is a pointer to an integer used to return the
	 *              number of messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	virtual stiHResult SignMailCountGet (
		unsigned int *punSignMailMessageCount) const = 0;

	/*!
	 * \brief Returns the max number of SignMail messages that a category can contains. 
	 *  
	 * \param punSignMailMaxMessageCount is a pointer to an integer used to return the
	 *              number of messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	virtual stiHResult SignMailMaxCountGet (
		unsigned int *punSignMailMaxMessageCount) const = 0;

	/*!
	 * \brief Returns the sub category of a category, based on the index passed in. 
	 *  
	 * \param  categoryId is a refrance to a CstiItemId object that specifies the category to retrieve the sub categories from.
	 * \param  unIndex is an unsigned int that specifies the index of the desired sub category.
	 * \param  pSubCategoryId is a pointer to a CstiItemId which receives a pointer to the sub categories Id, the UI will
	 *              be responsible for cleaning up the memory allocated for this pointer.
	 *
	 * \return stiRESULT_SUCCESS if successful,
	 *         stiRESULT_INVALID_PARAMETER if unIndex is larger than the number of sub categories in the category list
	 *         or the category cannot be found.
	 */
	virtual stiHResult SubCategoryByIndexGet (
		const CstiItemId &categoryId,
		unsigned int unIndex, 
		CstiItemId **ppSubCategoryId) const = 0;

	/*!
	 * \brief Gets number of sub categories contained in a category's category list 
	 *  
	 * \param categoryId is a referance to a CstiItemId object that specifies the category to retrieve the sub categories from.
	 * \param punSubCategoryCount is a pointer to an integer used to return the
	 *  	        number of sub categories a category currently contains.
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 *         stiRESULT_INVALID_PARAMETER if the category cannot be found. 
	 */
	virtual stiHResult SubCategoryCountGet (
		const CstiItemId &categoryId,
		unsigned int *punSubCategoryCount) const = 0;

	/*!
	 * \brief Returns the number of unviewed messages that a category contains. 
	 *  
	 * \param categoryId is a referance to a CstiItemId object that specifies a category.
	 * \param pnUnviewedMessageCount is a pointer to an integer used to return the
	 *  	        number of new messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful
	 * 		   stiRESULT_INVALID_PARAMETER if nIndex is larger than the number of items in the category list,
	 *  	   stiRESULT_ERROR if the category matching the categoryId cannot be found.
	 */
	virtual stiHResult UnviewedMessageCountGet (
		const CstiItemId &categoryId,
		unsigned int *punUnviewedMessageCount) const = 0;

	/*!
	* \brief Sets whether the platform supports HEVC SignMail Playback
	*
	* \param hevcSignMailPlaybackSupported is a bool that determines the state of hevcSignMailPlayback.
	*
	* \return stiRESULT_SUCCESS if successful in setting the hevcSignMailPlaybackSupported bool
	*/
	virtual stiHResult HEVCSignMailPlaybackSupportedSet (bool hevcSignMailPlaybackSupported) = 0;

	/*!
	* \brief Notification lastViewedSignMail Property in the property table needs to be updated with the time.
	*/
	virtual ISignal<time_t>& lastSignMailUpdateSignalGet () = 0;

	/*!
	* \brief Notification lastViewdMessage Property in the property table needs to be updated with the time.
	*/
	virtual ISignal<time_t>& lastMessageUpdateSignalGet () = 0;

	/*!
	* \brief Notification that there are new SignMails and the number of new signMails.
	*/
	virtual ISignal<int>& newSignMailSignalGet () = 0;

	/*!
	* \brief Notification that there are new videos and the number of new videos.
	*/
	virtual ISignal<int>& newVideoMsgSignalGet () = 0;

 protected:
	IstiMessageManager () = default;
	virtual ~IstiMessageManager () = default;

}; // end MessageManager class definition

#endif // ISTIMESSAGEMANAGER_H
