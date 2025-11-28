/*!
 * \file CstiMessageManager.h
 * \brief The Message List Manager Class
 *
 * Class declaration for the Message Manager.
 *  This class is responsible for sorting messages into categories and
 *  maintaining the list of categories.
 *  This is done by processing estMSG_MESSAGE_RESPONSEs sent from the
 *  core services module.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2011 – 2012 Sorenson Communications, Inc. -- All rights reserved
 */


#ifndef MESSAGEMANAGER_H
#define MESSAGEMANAGER_H


#include "CstiItemId.h"
#include "CstiMessageInfo.h"
#include "IstiMessageManager.h"
#include "XMLManager.h"
#include "CstiMessageService.h"
#include "CstiMessageResponse.h"
#include "CstiCoreResponse.h"
#include "ServiceResponse.h"
#include "CstiStateNotifyResponse.h"
#include "CstiSignal.h"
#include "JsonFileSerializer.h"
#include "ISerializable.h"
#include <list>
#include <algorithm>

//
// Constants
//
const std::string codecKeyHEVC = "hevc";
const std::string codecKeyH264 = "h264";
//
// Typedefs
//

//
// Forward Declarations
//
struct ClientAuthenticateResult;
class CstiEventQueue;

//
// Globals
//

/*!
 * \ingroup VideophoneEngineLayer 
 * \brief Message Manager Class
 *
 */
class CstiMessageManager : public IstiMessageManager, public vpe::Serialization::ISerializable
{
public:
	
	/*!
	 * \brief Message Manager constructor
	 * 
	 */
	CstiMessageManager (
		EstiBool bEnabled,
		CstiMessageService *pMsgService,
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam);
	~CstiMessageManager () override;

	CstiMessageManager (const CstiMessageManager &other) = delete;
	CstiMessageManager (CstiMessageManager &&other) = delete;
	CstiMessageManager &operator= (const CstiMessageManager &other) = delete;
	CstiMessageManager &operator= (CstiMessageManager &&other) = delete;

	/*!
	 * \brief Locks the Message Manager so that its lists cannot be manipulated 
	 *		  by other processes. 
	 *  
	 */
	inline stiHResult Lock () const override
	{
		m_eventQueue.Lock ();
		return stiRESULT_SUCCESS;
	}

	/*!
	 * \brief Unlocks the Message Manager so that its lists can be manipulated 
	 *		  by other processes. 
	 *  
	 */
	inline void Unlock () const override
	{
		m_eventQueue.Unlock ();
	}

	/*!
	 * \brief Gets the message category by a list index
	 *  
	 * \param categoryId is a referance to CstiItemId that specifies the category to retrieve.
	 * 		  punIndex is a ponter to an unsigned int that will receive the index of the category. 
	 *  	  pnCategoryType is a pointer to an integer that will receive the Categories type.
	 *  	  pszCatShortName is a buffer allocated by the UI that will recieve the short name of the category.
	 *  	  nShortBufSize is the size of a pszCatShortName buffer.
	 *  	  pszCatLongName is a buffer allocated by the UI that will recieve the long name of the category.
	 *  	  nLongBufSize is the size of a pszCatLongName buffer.
	 * 
	 * \return stiRESULT_SUCCESS if successful, 
	 * 		   stiRESULT_INVALID_PARAMETER if categoryId is invalid,
	 * 		   stiRESULT_BUFFER_TOO_SMALL if pszCatShortName or pszCatLongName buffers are to small.
	 * 		   stiRESULT_ERROR if the category is not found.
	 */
	stiHResult CategoryByIdGet (
		const CstiItemId &categoryId, 
		unsigned int *punIndex, 
		int *pnCategoryType, 
		char *pszCatShortName, unsigned int nShortBufSize, 
		char *pszCatLongName, unsigned int nLongBufSize) const override; 

	/*!
	 * \brief Returns the category based on the index passed in. 
	 *  
	 * \param  unIndex is an unsigned int that specifies the index of the desired category.
	 * 		   pCategoryId is a pointer to a CstiItemId which receives a pointer to the categories Id.	
	 *  
	 * \return stiRESULT_SUCCESS if successful,
	 *		   stiRESULT_INVALID_PARAMETER if unIndex is larger than the number of categories in the category list.
	 */
	stiHResult CategoryByIndexGet (
		unsigned int unIndex, 
		CstiItemId **ppCategoryId) const override;
		
	/*!
	 * \brief Gets number of categories in the category list 
	 *  
	 * \param pnCategoryCount is a pointer to an integer used to return the 
	 *  	  number of categories the Message Manger currently contains.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	stiHResult CategoryCountGet (
		unsigned int *punCategoryCount) const override;
	
	/*!
	 * \brief Get the ID of a categories parent. 
	 *  
	 * \param categoryId is a referance to a CstiItemId that inidicates the category to use to get the parent.
	 *        pParentId is a pointer to a CstiItemId that will be set to the Id of the parent, the UI is responsible for
	 *        cleaning up the memory allocated for the pParentId.
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 *         stiRESULT_INVALID_PARAMETER if the category matching categoryId could not be found. 
	 * 		   pParentId will be NULL if pCategoryId does not have a parent.  
	 */
	stiHResult CategoryParentGet (
		const CstiItemId &categoryId, 
		CstiItemId **pParentId) const override;
	
	EstiBool CoreResponseHandle (
		CstiCoreResponse *pCoreResponse);

	/*!
	 * \brief Returns enabled state of the MessageManager. 
	 *  
	 * \return estiTRUE if the MessageManger is enabled.
	 * 		   estiFALSE if the MessageManger is disabled.
	 */
	EstiBool EnabledGet () 
	{ 
		return m_bEnabled; 
	}

	/*!
	 * \brief Sets the enabled status of the MessageManager. If the MessageManager is being enabled it will call 
	 * 		  the Refresh() function. 
	 *  
	 * \param bEnabled is set to estiTRUE to enable the MessageManager and estiFALSE to disable the MessageManger.
	 */
	void EnabledSet (EstiBool bEnabled);


	void Initialize ();

	/*!
	 * \brief Sets the time of the last category update.  Messages received after
	 *        this time will be counted as "new".
	 *
	 * \param unLastTime The time of the last update
	 */
	void LastCategoryUpdateSet(
		time_t unLastTime);

	/*!
	 * \brief Sets the time of the last SignMail update.  Messages received after
	 *        this time will be counted as "new".
	 *
	 * \param unLastTime The time of the last update
	 */
	void LastSignMailUpdateSet(
		time_t unLastTime); 

	/*!
	 * \brief Updates the time of the last category update.
	 */
	void lastCategoryUpdateSet() override;

	/*!
	 * \brief Updates the time of the signMail update. 
	 */
	void lastSignMailUpdateSet() override;

	/*!
	 * \brief Populates a CstiMessageItem based on the categoryId and index. 
	 *  
	 * \param categoryId is a referance to the CstiItemId object that specifies a category.
	 * 		  pnIndex is a pointer to an integer used to return the 
	 *  	  pMessageItem is a CstiMessageItem that is passed in and the Message Manager
	 * 		  will populate its data members.
	 *  
	 * \return stiRESULT_SUCCESS if successful 
	 * 		   stiRESULT_INVALID_PARAMETER if categoryId is invalid, not found in the list or 
	 * 		   							   unIndex is larger than the number of messages in the list,
	 * 		   stiRESULT_ERROR otherwise. 
	 */
	stiHResult MessageByIndexGet (
		const CstiItemId &categoryId,
		unsigned int unIndex,
		CstiMessageInfo *pMessageItem) const override;
	
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
	stiHResult MessageCategoryGet (
		const CstiItemId &messageId, 
		CstiItemId **pCategoryId) const override;
	
	/*!
	 * \brief Returns the number of messages that a category contains. 
	 *  
	 * \param categoryId is a referance to a CstiItemId object that specifies a category.
	 * 		  pnMessageCount is a pointer to an integer used to return the 
	 *  	  number of messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	stiHResult MessageCountGet (
		const CstiItemId &categoryId,
		unsigned int *punMessageCount) const override;
	
	/*!
	 * \brief Gets the data of a CstiMessageItem based on the categoryId and itemId 
	 * 		  contained in the CstiMessageItem. 
	 *  
	 * \param pMessageItem is a CstiMessageItem that is passed in with a valid itemId (CstiItemId)
	 *  	  for the desired message.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	stiHResult MessageInfoGet (
		CstiMessageInfo *pMessageItem) const override;
	
	/*!
	 * \brief Sets the data contained in the CstiMessageItem, based on the categoryId and itemId 
	 * 		  of the CstiMessageItem, on the message object maintained by the Message Manager. 
	 *  
	 * \param pMessageItem is a CstiMessageItem that is passed in with a valid itemId (CstiItemId)
	 *  	  and all other data members set to the desired state.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	stiHResult MessageInfoSet (
		CstiMessageInfo *pMessageItem) override;

	/*!
	 * \brief Deletes the message item, specified by the msgItemId, from the message server.
	 *  
	 * \param msgItemId is a referance to the unique ID of the message item that should be deleted from the message server
	 * 
	 * \return stiRESULT_SUCCESS if the delete request was successfully sent.
	 */
	stiHResult MessageItemDelete(
		const CstiItemId &msgItemId) override;

	EstiBool MessageResponseHandle (
		CstiMessageResponse *pMessageResponse);

	/*!
	 * \brief Deletes all messages in the category specified by the category ID.
	 *  
	 * \param categoryId is a referance to the unique ID of the category that should have its messages deleted from the message server
	 * 
	 * \return stiRESULT_SUCCESS if the delete request was successfully sent.
	 */
	stiHResult MessagesInCategoryDelete (
		const CstiItemId &categoryId) override;

	stiHResult MessagesInCategoryDelete (
		const CstiItemId &categoryId,
		int nCount);

	/*!
	 * \brief Returns the number of new messages that a category contains. 
	 *  
	 * \param categoryId is a referance to a CstiItemId object that specifies a category.
	 * 		  pnNewMessageCount is a pointer to an unsigned integer used to return the 
	 *  	  number of new messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	stiHResult NewMessageCountGet (
		const CstiItemId &categoryId,
		unsigned int *pnNewMessageCount) const override;

	/*!
	 * \brief Causes the message manager to re-request the message list.
	 */
	void Refresh() override;

	/*!
	 * \brief Returns the number of SignMail messages that a category contains. 
	 *  
	 * \param punSignMailMessageCount is a pointer to an integer used to return the
	 *              number of messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	stiHResult SignMailCountGet (
		unsigned int *punSignMailMessageCount) const override;

	/*!
	 * \brief Returns the max number of SignMail messages that a category can contains. 
	 *  
	 * \param punSignMailMaxMessageCount is a pointer to an integer used to return the
	 *              number of messages the category specified by pCategoryId.
	 *  
	 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
	 */
	stiHResult SignMailMaxCountGet (
		unsigned int *punSignMailMaxMessageCount) const override;

	EstiBool StateNotifyEventHandle(
		CstiStateNotifyResponse *poResponse);

	/*!
	 * \brief Returns the sub category of a category, based on the index passed in. 
	 *  
	 * \param  categoryId is a referance to a CstiItemId object that specifies the category to retrieve the sub categories from. 
	 * 		   unIndex is an unsigned int that specifies the index of the desired sub category.
	 *         pSubCategoryId is a pointer to a CstiItemId which receives a pointer to the sub categories Id, the UI will
	 *         be responsible for cleaning up the memory allocated for this pointer.	
	 *  
	 * \return stiRESULT_SUCCESS if successful,
	 *         stiRESULT_INVALID_PARAMETER if unIndex is larger than the number of sub categories in the category list
	 *         or the category cannot be found.
	 */
	stiHResult SubCategoryByIndexGet (
		const CstiItemId &categoryId,
		unsigned int unIndex, 
		CstiItemId **ppSubCategoryId) const override;

	/*!
	 * \brief Gets number of sub categories contained in a category's category list 
	 *  
	 * \param categoryId is a referance to a CstiItemId object that specifies the category to retrieve the sub categories from.
	 *        punSubCategoryCount is a pointer to an integer used to return the
	 *  	  number of sub categories a category currently contains.
	 *
	 * \return stiRESULT_SUCCESS if successful 
	 *         stiRESULT_INVALID_PARAMETER if the category cannot be found. 
	 */
	stiHResult SubCategoryCountGet (
		const CstiItemId &categoryId,
		unsigned int *punSubCategoryCount) const override;

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
	stiHResult UnviewedMessageCountGet (
		const CstiItemId &categoryId,
		unsigned int *punUnviewedMessageCount) const override;

	/*!
	* \brief Sets whether the platform supports HEVC SignMail Playback
	*
	* \param hevcSignMailPlaybackSupported is a bool that determines the state of hevcSignMailPlayback.
	*
	* \return stiRESULT_SUCCESS if successful in setting the hevcSignMailPlaybackSupported bool
	*/

	//Signals
	stiHResult HEVCSignMailPlaybackSupportedSet (bool hevcSignMailPlaybackSupported) override;

	ISignal<time_t>& lastSignMailUpdateSignalGet () override;

	ISignal<time_t>& lastMessageUpdateSignalGet () override;

	ISignal<int>& newSignMailSignalGet () override;

	ISignal<int>& newVideoMsgSignalGet () override;

	CstiSignal<time_t> lastSignMailUpdateSignal;

	CstiSignal<time_t> lastMessageUpdateSignal;

	CstiSignal<int> newSignMailSignal;

	CstiSignal<int> newVideoMsgSignal;

private:
	static constexpr int SaveTimerMillis = 250;

	EstiBool m_bEnabled;
	CstiMessageService *m_pMessageService;
	PstiObjectCallback m_pfnVPCallback;
	size_t m_CallbackParam;
	size_t m_CallbackFromId;
	std::list<unsigned int> m_PendingRequestIds;
	EstiBool m_bInitialized;
	EstiBool m_bAuthenticated;
	unsigned int m_unNextId;

	time_t m_unCategoryLastUpdate;
	time_t m_unSignMailLastUpdate;
	stiWDOG_ID m_wdExpirationTimer;
	bool m_hevcSignMailPlaybackSupported = false;
	CstiSignalConnection::Collection m_signalConnections;
	CstiEventQueue& m_eventQueue;
	vpe::EventTimer m_saveTimer;

	vpe::Serialization::JsonFileSerializer m_serializer{ "Messages.json" };

	struct SstiCategoryInfo : public vpe::Serialization::ISerializable
	{
		SstiCategoryInfo ();

		~SstiCategoryInfo() override
		{
			delete pCategoryId;
			delete pParentId;
		}

		SstiCategoryInfo (const SstiCategoryInfo &other) = delete;
		SstiCategoryInfo (SstiCategoryInfo &&other) = delete;
		SstiCategoryInfo &operator= (const SstiCategoryInfo &other) = delete;
		SstiCategoryInfo &operator= (SstiCategoryInfo &&other) = delete;

		CstiItemId *pCategoryId{nullptr};
		EstiMessageType nCategoryType {estiMT_NONE};
		std::string longName;
		std::string shortName;
		std::string token;
		CstiItemId *pParentId{nullptr};
		std::list<SstiCategoryInfo *> lCategoryInfoList;
		std::list<CstiMessageInfo *> lMessageInfoList;
		unsigned int unUnviewedMessageCount{0};
		unsigned int unSignMailCount{0};
		unsigned int unMaxSignMailCount{0};
		EstiBool bAlwaysVisible {estiFALSE};
		EstiBool bCategoryChanged {estiFALSE};

		vpe::Serialization::MetaData m_serializationData;

		std::string typeNameGet () override;
		vpe::Serialization::MetaData& propertiesGet () override;
		ISerializable* instanceCreate (const std::string& typeName) override;
	};
	std::list<SstiCategoryInfo *> m_lCategoryListItems;

	/*!
	 * \brief Utility function to add categories in the correct position in a category list.
	 *
	 * \param lCategoryList is a list containing SstiCategoryInfo objects.
	 */
	stiHResult AddCategory(
		SstiCategoryInfo *pNewCategory,
		SstiCategoryInfo *pParentCategory);

	stiHResult AddMessageItem (
		const CstiMessageListItemSharedPtr &pMessageItem,
		SstiCategoryInfo *pCategoryInfo);

	void clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult);

	stiHResult AuthenticatedSet (
		EstiBool bAuthenticated);

	SstiCategoryInfo *CategoryByIdGet(
		unsigned int unTableId);

	SstiCategoryInfo *CategoryByTokenGet(
		const char *pszToken);

	/*!
	 * \brief Creates a new category and calls AddCategory() to add it to the correct category list. 
	 *        The category created is based on the category ID or token from the table.
	 *  
	 * \param unTableId is the category ID from the static category table 
	 *        pszToken is a token from the static category table.
	 * 
	 */
	stiHResult CategoryCreate (
		unsigned int unTableId,
		const char *pszToken);

	void CleanupCategory (
		SstiCategoryInfo *pCateogryInfo = nullptr);

	stiHResult ExpirationTimerStart ();
	
	static int ExpirationTimerCB (
		size_t param);

	/*!
	 * \brief Finds the category matching the category ID or token.
	 *  
	 * \param Only one of the parameters needs to be passed into the function.
	 * 		  pCategoryId is a CstiItemId identifying the category 
	 *  	  pszToken is the Token passed from the VMA used to look up the name of the category.
	 *  	  
	 * \return A pointer to a SstiCategoryInfo. 
	 */
	void FindCategory(
		const CstiItemId &pCategoryId,
		const char *pszToken,
		SstiCategoryInfo **ppFoundCategory,
		unsigned int *punIndex) const;

	void MarkMessagesDirty(
		std::list<SstiCategoryInfo *> *plCategoryList);

	void MessageByIdGet(
		const CstiItemId &pMessageId,
		std::list<SstiCategoryInfo *> lCategoryList,
		CstiMessageInfo **ppMessageInfo,
		SstiCategoryInfo **ppCategoryInfo) const;

	stiHResult MessageRequestSend (
		CstiMessageRequest *poMsgRequest,
		unsigned int *punRequestId);

	stiHResult NotifyChangedCategories();

	stiHResult ParseMessageList(
		CstiMessageResponse *pMessageResponse);

	/*!
	 * \brief Utility function that prints the category tree and messages.
	 *
	 * \param lCategoryList is a list containing SstiCategoryInfo objects.
	 */
	void PrintMessageList(
		std::list<SstiCategoryInfo *> lCategoryList,
		int nTabCount);

	/*!
	 * \brief Utility function that removes any messages items that have been marked
	 * 		  as dirty (they no longer exist on the messages server).
	 *
	 * \param plCategoryList is a list containing SstiCategoryInfo objects.
	 */
	void RemoveDirtyMessages(
		std::list<SstiCategoryInfo *> *plCategoryList);

	/*!
	 * \brief Utility function that removes any messages items that have reached the
	 * 		  expiration date.
	 *
	 * \param pCategory is a pointer to a SstiCategoryInfo that will be checked for expired messages.
	 */
	void RemoveExpiredMessages(
		std::list<SstiCategoryInfo *> *plCategoryList);

	/*!
	 * \brief Utility function that removes any categories that no longer contain any messages 
	 * 		  or are not marked as always visible.
	 *
	 * \param plCategoryList is a list containing SstiCategoryInfo objects.
	 */
	void RemoveEmptyCategories(
		std::list<SstiCategoryInfo *> *plCategoryList);

	EstiBool RemoveRequestId (
		unsigned int unRequestId);

	void updateCategoryUpdateTime(
		const SstiCategoryInfo *category);

	SstiCategoryInfo *SubCategoryByNameGet(
		const char *pszCategoryName,
		const char *pszTokenName);

	SstiCategoryInfo *UnknownCategoryCreate(
		const char *pszCategoryName,
		SstiCategoryInfo *pParentCategory);

	std::string typeNameGet () override;
	vpe::Serialization::MetaData& propertiesGet () override;
	vpe::Serialization::ISerializable* instanceCreate (const std::string& typeName) override;

	vpe::Serialization::MetaData m_serializationData;

}; // end MessageManager class definition

#endif // ISTIMESSAGEMANAGER_H
