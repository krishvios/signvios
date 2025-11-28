/*!
 * \file CstiMessageManager.cpp
 * \brief The MessageManager class
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

//
// Includes
//

#include <cstring>
#include <iostream>

//#include "const.h"
#include "stiTools.h"
#include "stiError.h"
#include "stiDefs.h"
#include "stiTrace.h"
#include "CstiMessageManager.h"
#include "CstiMessageResponse.h"
#include "cmPropertyNameDef.h"
#include "PropertyManager.h"
#include "CstiCoreService.h"
#include "CstiMessageService.h"
#include "CstiEventQueue.h"

using namespace WillowPM;
//
// Constants
//

//
// Defines
//

// DBG_ERR_MSG is a tool to add file/line infromation to an stiTrace
// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiBlockListDebug, stiTrace ("BlockList: " fmt "\n", ##__VA_ARGS__); )
#define DBG_ERR_MSG(fmt,...) stiDEBUG_TOOL (g_stiBlockListDebug, stiTrace (HERE " ERROR: " fmt "\n", ##__VA_ARGS__); )

#define MAX_VIDEO_MESSAGES 300
#define MAX_MESSAGE_NAME_LENGTH 256
#define EXPRESSION_ARRAY_SIZE 5
#define SIGNMAIL_CATEGORY_INFO_INDEX 0
#define SORENSON_CATEGORY_INFO_INDEX 1
#define NEWS_CATEGORY_INFO_INDEX 4
#define TECH_SUPPORT_CATEGORY_INFO_INDEX 2
#define THIRD_PARTY_CATEGORY_INFO_INDEX 3

struct SstiCategoryArrayInfo
{
	const char *pszShortName;
	const char *pszLongName;
	EstiBool bAlwaysVisible;
	EstiMessageType nCategoryType;
	const char *pszToken;  // Used by VMA to indicate the program (sub category) name.
	unsigned int unParentId;    // Used to indicate the category's parent category.  '0' means no parent.
	unsigned int unCategoryId;
};

static const SstiCategoryArrayInfo g_aCategoryList[] =
{   //Short Name   					Long Name        			Always Visible    Category Type	  			Token     Parent ID  Category ID
	{"SignMail",    				"SignMail",           		   estiTRUE,   estiMT_SIGN_MAIL,			  nullptr,	     0, 		   1},
	{"Sorenson",    				"Sorenson",           		   estiTRUE,   estiMT_MESSAGE_FROM_SORENSON,  nullptr,	     0, 		   2},
	{"Customer Support",			"Customer Support",   		   estiFALSE,  estiMT_FROM_TECH_SUPPORT,      nullptr,	     0, 		   3},
	{"General Video",   			"General Video",      		   estiFALSE,  estiMT_MESSAGE_FROM_OTHER,     nullptr,	     0, 		   4},
	{"News",   						"News",          		   	   estiFALSE,  estiMT_NEWS,   			      nullptr,	     0, 		   5},
	{"DKN",         				"Deaf Kids Network",  		   estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "DKN",	 0, 		   6},
	{"SN",          				"SIGNetwork",         		   estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "SN",	     0, 		   7},
	{"Community",   				"Community",          		   estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "COM",	 0, 		   8},
	{"School News",   				"School News",        		   estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "SCNW",    0, 		   9},
	{"FCC News",   					"FCC News",        	  		   estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "FCC",     0, 		   10},
	{"News",   						"News",          		   	   estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "NEWS",	 0, 		   11},
	{"General Video",   			"General Video",      		   estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "GNVD",	 0, 		   12},
	{"Gallaudet",		            "Gallaudet",      		       estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "GUTV",	 0, 		   13},
	{"FSDB",                        "FSDB",                        estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "FSDB",    0,            14},
	{"NTID",                        "NTID",                        estiFALSE,  estiMT_MESSAGE_FROM_SORENSON,  "NTID",    0,            15},
	{"Direct SignMail",				"Direct SignMail",             estiFALSE,  estiMT_DIRECT_SIGNMAIL,		  nullptr,		 0, 		   16},
	{"Third Party SignMail",		"Third Party SignMail",        estiFALSE,  estiMT_THIRD_PARTY_VIDEO_MAIL, nullptr,		 0, 		   17},
	{"P2P SignMail",				"P2P SignMail",                estiFALSE,  estiMT_P2P_SIGNMAIL,		  	  nullptr,		 0, 		   18},
};
static const int g_nNumCategoryListItems = sizeof(g_aCategoryList) / sizeof(g_aCategoryList[0]);

using namespace std;
using namespace WillowPM;

CstiMessageManager::CstiMessageManager (
	EstiBool bEnabled,
	CstiMessageService *pMsgService,
	PstiObjectCallback pfnVPCallback,
	size_t CallbackParam)
	: 
	m_bEnabled (bEnabled),
	m_pMessageService (pMsgService),
	m_pfnVPCallback (pfnVPCallback),
	m_CallbackParam (CallbackParam),
	m_CallbackFromId (0),
	m_bInitialized (estiFALSE),
	m_bAuthenticated (estiFALSE),
	m_unNextId (0),
	m_unCategoryLastUpdate(0),
	m_unSignMailLastUpdate(0),
	m_wdExpirationTimer(nullptr),
	m_eventQueue(CstiEventQueue::sharedEventQueueGet()),
	m_saveTimer(SaveTimerMillis, &CstiEventQueue::sharedEventQueueGet())
{
	m_signalConnections.push_back (m_pMessageService->coreServiceGet()->clientAuthenticateSignal.Connect ([this](const ServiceResponseSP<ClientAuthenticateResult>& result) {
		m_eventQueue.PostEvent(std::bind(&CstiMessageManager::clientAuthenticateHandle, this, result));
	}));

	m_signalConnections.push_back (m_saveTimer.timeoutSignal.Connect ([this]() {
		m_serializer.serialize (this);
	}));

	m_serializationData.push_back (std::make_shared<vpe::Serialization::ArrayProperty<std::list<SstiCategoryInfo*>, SstiCategoryInfo*>> ("categories", m_lCategoryListItems));
}

CstiMessageManager::~CstiMessageManager ()
{
	// Cleanup the expirationTime timer
	if (m_wdExpirationTimer)
	{
		stiOSWdDelete (m_wdExpirationTimer);
		m_wdExpirationTimer = nullptr;
	}

	// Cleanup the category list and message lists.
	CleanupCategory();
}

/*!
 * \brief Utility function to add categories in the correct position in a category list.
 *
 * \param lCategoryList is a list containing SstiCategoryInfo objects.
 */
stiHResult CstiMessageManager::AddCategory(
	SstiCategoryInfo *pNewCategory,
	SstiCategoryInfo *pParentCategory)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::list<SstiCategoryInfo *> *pCategoryList = nullptr;
	std::list<SstiCategoryInfo *>::iterator itCategory;

	pCategoryList = pParentCategory ? &(pParentCategory->lCategoryInfoList) : &m_lCategoryListItems;

	if (pCategoryList->empty())
	{
		pCategoryList->push_front(pNewCategory);
	}
	else
	{
		itCategory = pCategoryList->begin();

		// If we don't have a parent then we are at the root.
		if (!pParentCategory)
		{
			// Find the SignMail channel (it should always be the first channel).
			SstiCategoryInfo *pSignMailCategory = (*itCategory);

			stiTESTCOND (!strcmp(pSignMailCategory->shortName.c_str(),
								 g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].pszShortName), stiRESULT_ERROR);

			// Start the sort from the next category.
			itCategory++;
		}

		// Add new categories alphabetically.
		while (itCategory != pCategoryList->end())
		{
			SstiCategoryInfo *pListCategory = (*itCategory);
			if ((0 < stiStrICmp(pListCategory->shortName.c_str(), pNewCategory->shortName.c_str())) ||
				(0 == stiStrICmp(pListCategory->shortName.c_str(), pNewCategory->shortName.c_str()) &&
				0 < strcmp(pListCategory->shortName.c_str(), pNewCategory->shortName.c_str())))
			{
				break;
			}
			itCategory++;
		}
		pCategoryList->insert(itCategory, pNewCategory);
	}

STI_BAIL:
	return hResult;
}

stiHResult CstiMessageManager::AddMessageItem (
		const CstiMessageListItemSharedPtr &pMessageItem,
		SstiCategoryInfo *pCategoryInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageInfo *pMessageInfo = nullptr;
	CstiItemId itemId;
	CstiMessageInfo *pListMessage = nullptr;
	std::list<CstiMessageInfo *>::iterator it;
	EstiBool bHaveMessage = estiFALSE;
	EstiBool bCategoryChanged = estiFALSE;
	CstiItemId signMailId;

	Lock ();

	// Initialize an Item ID for the message.
	itemId.ItemIdSet(pMessageItem->ItemIdGet());
	signMailId.ItemIdSet(g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId);

	// Intialize for the currentTime for the expirationTime check.
	time_t currentTime;
	time(&currentTime);

	// Search the list for the message item.
	if (!pCategoryInfo->lMessageInfoList.empty())
	{
		for (it = pCategoryInfo->lMessageInfoList.begin();
			 it != pCategoryInfo->lMessageInfoList.end();
			 it++)
		{
			pListMessage = (*it);
			if (itemId == pListMessage->ItemIdGet())
			{
				// If the message has not expired, or it is a SignMail Message, 
				// mark the message as clean and don't add it.
				time_t msgExpTime = pListMessage->ExpirationDateGet();
				if (*(pCategoryInfo->pCategoryId) == signMailId ||
					msgExpTime == 0 ||
					difftime(msgExpTime, currentTime) > 0)
				{
					pListMessage->DirtySet(estiFALSE);
				}

				// Check the viewed status:
				if (pListMessage->ViewedGet() != pMessageItem->ViewedGet())
				{
					pListMessage->ViewedSet(pMessageItem->ViewedGet());
					bCategoryChanged = estiTRUE;
				}
				// Check the pause point
				if (pListMessage->PausePointGet() != pMessageItem->PausePointGet())
				{
					pListMessage->PausePointSet(pMessageItem->PausePointGet());
					bCategoryChanged = estiTRUE;
				}

				// Check it's viewed status.
				if (!pListMessage->ViewedGet())
				{
					pCategoryInfo->unUnviewedMessageCount++;
				}

				// Update the featured video status
				if (pListMessage->FeaturedVideoGet() != pMessageItem->FeaturedVideoGet())
				{
					pListMessage->FeaturedVideoSet(pMessageItem->FeaturedVideoGet());

					// Update the featured video pos if this is a featured video
					if (pListMessage->FeaturedVideoGet())
					{
						pListMessage->FeaturedVideoPosSet(pMessageItem->FeaturedVideoPosGet());
					}
					bCategoryChanged = estiTRUE;
				}
				bHaveMessage = estiTRUE;
				break;
			}
		}
	}

	// All expiration times should happen at midnight.
	time_t msgExpTime = pMessageItem->ExpirationDateGet();
	if (msgExpTime != 0)
	{
		struct tm localExpTime{};
		localtime_r(&msgExpTime, &localExpTime);
		if (localExpTime.tm_hour < 23 || 
			localExpTime.tm_min < 59 ||
			localExpTime.tm_sec < 59)
		{
			localExpTime.tm_hour = 23;
			localExpTime.tm_min = 59;
			localExpTime.tm_sec = 59;
			msgExpTime = mktime(&localExpTime);
		}
	}

	// If we don't have the message then add it.
	if (bHaveMessage == estiFALSE &&
		(signMailId == *(pCategoryInfo->pCategoryId) ||
		 msgExpTime == 0 ||
		 difftime(msgExpTime, currentTime) > 0))
	{
		// Didn't find it so create the message item.
		pMessageInfo = new CstiMessageInfo();
		pMessageInfo->MessageTypeIdSet (pMessageItem->MessageTypeIdGet ());
		pMessageInfo->NameSet(pMessageItem->NameGet());
		pMessageInfo->PausePointSet(pMessageItem->PausePointGet());
		pMessageInfo->MessageLengthSet(pMessageItem->DurationGet());
		pMessageInfo->DateTimeSet(pMessageItem->CallTimeGet());
		pMessageInfo->DialStringSet(pMessageItem->DialStringGet());
		pMessageInfo->ViewedSet(pMessageItem->ViewedGet());
		pMessageInfo->ItemIdSet(itemId);
		pMessageInfo->InterpreterIdSet(pMessageItem->InterpreterIdGet());
		pMessageInfo->PreviewImageURLSet(pMessageItem->PreviewImageURLGet());
		pMessageInfo->FeaturedVideoSet(pMessageItem->FeaturedVideoGet());
		pMessageInfo->FeaturedVideoPosSet(pMessageItem->FeaturedVideoPosGet());
		pMessageInfo->AirDateSet(pMessageItem->AirDateGet());
		pMessageInfo->ExpirationDateSet(msgExpTime);
		pMessageInfo->DescriptionSet(pMessageItem->DescriptionGet());

		if (!pMessageInfo->ViewedGet())
		{
			pCategoryInfo->unUnviewedMessageCount++;
		}

	   if (pCategoryInfo->lMessageInfoList.empty())
		{
			pCategoryInfo->lMessageInfoList.push_front(pMessageInfo);
		}
		else
		{
			for (it = pCategoryInfo->lMessageInfoList.begin();
				 it != pCategoryInfo->lMessageInfoList.end();
				 it++)
			{
				pListMessage = (*it);
				if (pMessageInfo->DateTimeGet() > pListMessage->DateTimeGet())
				{
					break;
				}
				pListMessage = nullptr;
			}

			if (pListMessage)
			{
				pCategoryInfo->lMessageInfoList.insert(it, pMessageInfo);
			}
			else
			{
				pCategoryInfo->lMessageInfoList.push_back(pMessageInfo);
			}
		}
		bCategoryChanged = estiTRUE;
	}

	if (estiTRUE == bCategoryChanged)
	{
		// Mark the Category as changed so that the UI is notified of the changes.
		pCategoryInfo->bCategoryChanged = estiTRUE;

		// Check for a parent.  If we have a parent we need to update it's changed status.
		if (pCategoryInfo->pParentId)
		{
		   SstiCategoryInfo *pCategoryParent = nullptr;
		   FindCategory(*pCategoryInfo->pParentId, nullptr, &pCategoryParent, nullptr);
		   if (pCategoryParent)
		   {
			   pCategoryParent->bCategoryChanged = estiTRUE;
		   }
		   else
		   {
			   stiASSERT (estiFALSE);
		   }
		}
	}

//STI_BAIL:
	Unlock ();

	return hResult;
}

stiHResult CstiMessageManager::AuthenticatedSet (
	EstiBool bAuthenticated)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	m_bAuthenticated = bAuthenticated;
	
	if (!m_bAuthenticated)
	{
		CleanupCategory ();
		m_lCategoryListItems.clear ();
		m_saveTimer.start ();
	}
	else
	{
		// Notify the UI of any categories that loaded from cache
		for (auto category : m_lCategoryListItems)
		{
			auto itemId = new CstiItemId(*category->pCategoryId);
			m_pfnVPCallback (estiMSG_MESSAGE_CATEGORY_CHANGED,
				(size_t)itemId,
				m_CallbackParam, m_CallbackFromId);
		}

		Refresh();
	}

	Unlock ();

	return hResult;
}

/*!
 * \brief Gets the message category by a list index
 *  
 * \param nIndex is the index number of the desired category. 
 *  	  categoryId is a referance that will receive the CstiItemId of the category.
 *  	  pnCategoryType is a pointer to an integer that will receive the Categories type.
 *  	  pszCatShortName is a buffer allocated by the UI that will recieve the short name of the category.
 *  	  nShortBufSize is the size of a pszCatShortName buffer.
 *  	  pszCatLongName is a buffer allocated by the UI that will recieve the long name of the category.
 *  	  nLongBufSize is the size of a pszCatLongName buffer.
 * 
 * \return stiRESULT_SUCCESS if successful, 
 * 		   stiRESULT_INVALID_PARAMETER if nIndex is larger than the number of items in the category list,
 *  	   stiRESULT_BUFFER_TOO_SMALL if pszCatShortName or pszCatLongName buffers are to small.
 *  	   stiRESULT_ERROR if the category matching the categoryId cannot be found.
 */
stiHResult CstiMessageManager::CategoryByIdGet (
	const CstiItemId &categoryId, 
	unsigned int *punIndex, 
	int *pnCategoryType, 
	char *pszCatShortName, unsigned int unShortBufSize, 
	char *pszCatLongName, unsigned int unLongBufSize) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	SstiCategoryInfo *pCategory = nullptr;
	unsigned int unCount = 0;
  
	Lock();
	
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);
	
	FindCategory(categoryId, nullptr, &pCategory, &unCount);
	stiTESTCOND (pCategory, stiRESULT_ERROR);

	if (punIndex)
	{
		*punIndex = unCount;
	}
	
	if (pnCategoryType)
	{
		*pnCategoryType = pCategory->nCategoryType;
	}
	
	if (pszCatShortName)
	{
		stiTESTCOND (unShortBufSize > strlen(pCategory->shortName.c_str()), stiRESULT_BUFFER_TOO_SMALL);
		strcpy(pszCatShortName, pCategory->shortName.c_str());
	}
	
	if (pszCatLongName)
	{
		stiTESTCOND (unLongBufSize > pCategory->longName.length(), stiRESULT_BUFFER_TOO_SMALL);
		strcpy(pszCatLongName, pCategory->longName.c_str());
	}


STI_BAIL:
	Unlock();
	
	return hResult;
}


/*!
 * \brief Returns the category based on the index passed in. 
 *  
 * \param  unIndex is an unsigned int that specifies the index of the desired category.
 * 		   pCategoryId is a pointer to a CstiItemId which receives a pointer to the categories Id.	
 *  
 * \return stiRESULT_SUCCESS if successful,
 *		   stiRESULT_INVALID_PARAMETER if unIndex is larger than the number of categories in the category list.
 */
stiHResult CstiMessageManager::CategoryByIndexGet (
	unsigned int unIndex, 
	CstiItemId **ppCategoryId) const 
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::list<SstiCategoryInfo *>::const_iterator it;
	SstiCategoryInfo *pCategory = nullptr;
	unsigned int unCount = 0;
  
	Lock();
	
	stiTESTCOND (unIndex < m_lCategoryListItems.size(), stiRESULT_INVALID_PARAMETER);
	
	// Search for the category in the list.
	it = m_lCategoryListItems.begin();
	while (unCount != unIndex)
	{
		it++;
		unCount++;
	}
	pCategory = (*it);

	*ppCategoryId = new CstiItemId(*pCategory->pCategoryId);

STI_BAIL:

	Unlock();
	
	return hResult;
}		


/*!
 * \brief Gets number of categories in the category list 
 *  
 * \param pnCategoryCount is a pointer to an integer used to return the 
 *  	  number of categories the Message Manger currently contains.
 *  
 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
 */
stiHResult CstiMessageManager::CategoryCountGet (
	unsigned int *punCategoryCount) const 
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock();
	
	*punCategoryCount = m_lCategoryListItems.size();

	Unlock();
	
	return hResult;
}

/*!
 * \brief Creates a new category and calls AddCategory() to add it to the correct category list. 
 *        The category created is based on the category ID or token from the table.
 *  
 * \param unTableId is the category ID from the static category table 
 *        pszToken is a token from the static category table.
 * 
 */
stiHResult CstiMessageManager::CategoryCreate(
	const unsigned int unTableId,
	const char *pszToken)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nCategoryIndex = -1;
	SstiCategoryInfo *pCategory = nullptr;
	int nParentIndex = -1;
	SstiCategoryInfo *pParentCategory = nullptr;

	Lock();

	auto findResult = std::find_if(m_lCategoryListItems.begin(), m_lCategoryListItems.end(), [unTableId](SstiCategoryInfo* category) {
		uint32_t categoryId = 0;
		category->pCategoryId->ItemIdGet(&categoryId);
		return categoryId == unTableId;
	});

	stiTESTCOND_NOLOG(findResult == m_lCategoryListItems.end(), stiRESULT_ERROR);

	// Search the category list for the category.
	for (int i = 0; i < g_nNumCategoryListItems; i++)
	{
		if ((unTableId != 0 && unTableId == g_aCategoryList[i].unCategoryId) ||
			(pszToken && g_aCategoryList[i].pszToken && !strcmp(g_aCategoryList[i].pszToken, pszToken)))
		{
			nCategoryIndex = i;
			break;
		}
	}

	// Make sure that we found a category. If not the category is unknown and
	// should return an error.
	stiTESTCOND (nCategoryIndex >= 0 &&
				 nCategoryIndex < g_nNumCategoryListItems, stiRESULT_ERROR);

	pCategory = new SstiCategoryInfo;
	pCategory->pCategoryId = new CstiItemId();
	pCategory->pCategoryId->ItemIdSet(g_aCategoryList[nCategoryIndex].unCategoryId);

	pCategory->nCategoryType = g_aCategoryList[nCategoryIndex].nCategoryType;
	pCategory->bAlwaysVisible = g_aCategoryList[nCategoryIndex].bAlwaysVisible;
	
	pCategory->shortName = g_aCategoryList[nCategoryIndex].pszShortName;
	pCategory->longName = g_aCategoryList[nCategoryIndex].pszLongName;

	pCategory->token.clear ();
	pCategory->pParentId = nullptr;

	if (g_aCategoryList[nCategoryIndex].pszToken)
	{
		pCategory->token = g_aCategoryList[nCategoryIndex].pszToken;
	}

	if (g_aCategoryList[nCategoryIndex].unParentId)
	{
		pCategory->pParentId = new CstiItemId();
		pCategory->pParentId->ItemIdSet(g_aCategoryList[nCategoryIndex].unParentId);

		FindCategory(*pCategory->pParentId, nullptr, &pParentCategory, nullptr);
		if (!pParentCategory)
		{
			// Search the category list for the Parent Id.
			for (int i = 0; i < g_nNumCategoryListItems; i++)
			{
				if (g_aCategoryList[nCategoryIndex].unParentId == g_aCategoryList[i].unCategoryId)
				{
					nParentIndex = i;
					break;
				}
			}
			// Make sure that we found a parent category.
			stiTESTCOND (nParentIndex >= 0 &&
						 nParentIndex < g_nNumCategoryListItems, stiRESULT_ERROR);

			hResult = CategoryCreate(g_aCategoryList[nParentIndex].unCategoryId, nullptr);
			stiTESTRESULT();

			// Now find the newly added catetory.
			FindCategory(*pCategory->pParentId, nullptr, &pParentCategory, nullptr);
		}
	}

	pCategory->unUnviewedMessageCount = 0;
	pCategory->unSignMailCount = 0;
	pCategory->unMaxSignMailCount = 0;

	// Mark the channel as changed so that when we get the list the UI will
	// be notified that it exists.
	pCategory->bCategoryChanged = estiTRUE;

	// Add the category to the correct parent list.
	hResult = AddCategory(pCategory,
						  pParentCategory);

STI_BAIL:

	Unlock();

	if (hResult != stiRESULT_SUCCESS &&
		pCategory)
	{
		delete pCategory;
	}

	return hResult;
}


CstiMessageManager::SstiCategoryInfo *CstiMessageManager::CategoryByTokenGet(
	const char *pszTokenName)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;

	Lock();
	FindCategory(CstiItemId (), pszTokenName, &pCategory, nullptr);

	// If we didn't find it in the list then we need to create it.
	if (!pCategory)
	{
		hResult = CategoryCreate(0, pszTokenName);
		if (hResult != stiRESULT_SUCCESS)
		{
			stiTrace("We failed to add a category for %s\n", pszTokenName);
		}
	}

	Unlock();

	// If we failed to add a category then just return back the NULL pointer.
	return (pCategory || hResult != stiRESULT_SUCCESS) ? pCategory : CategoryByTokenGet(pszTokenName);
}

CstiMessageManager::SstiCategoryInfo *CstiMessageManager::CategoryByIdGet(
	const unsigned int nTableId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
	CstiItemId itemId;
	itemId.ItemIdSet(nTableId);

	Lock();
	FindCategory(itemId, nullptr, &pCategory, nullptr);

	// If we didn't find it in the list then we need to create it.
	if (!pCategory)
	{
		hResult = CategoryCreate(nTableId, nullptr);
		if (hResult != stiRESULT_SUCCESS)
		{
			stiTrace("We failed to add a category for category ID: %d\n", nTableId);
		}
	}

	Unlock();
	// If we failed to add a category then just return back the NULL pointer.
	return (pCategory || hResult != stiRESULT_SUCCESS) ? pCategory : CategoryByIdGet(nTableId);
}

/*!
 * \brief Get the ID of a categories parent. 
 *  
 * \param categoryId is a referance to a CstiItemId that inidicates the category to use to get the parent.
 *        pParentId is a pointer to a CstiItemId that will be set to the Id of the parent, the UI is responsible for
 *        cleaning up the memory allocated for the pParentId.
 *  
 * \return stiRESULT_SUCCESS if successful 
 *         stiRESULT_INVALID_PARAMETER if the category matching categoryId could not be found or categoryId is invalid. 
 * 		   pParentId will be NULL if categoryId does not have a parent.  
 */
stiHResult CstiMessageManager::CategoryParentGet (
	const CstiItemId &categoryId, 
	CstiItemId **ppParentId) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
	*ppParentId = nullptr;

	Lock();
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);

	FindCategory(categoryId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_INVALID_PARAMETER);

	if (pCategory->pParentId)
	{
		*ppParentId = new CstiItemId(*pCategory->pParentId);
	}

STI_BAIL:
	Unlock();
	
	return hResult;
}
	
void CstiMessageManager::CleanupCategory(
	SstiCategoryInfo *pCategory)
{
	std::list<SstiCategoryInfo *> lCategoryList = pCategory ? pCategory->lCategoryInfoList : m_lCategoryListItems;
	auto it = lCategoryList.begin();
	while ( it != lCategoryList.end())
	{
		SstiCategoryInfo *pCategoryInfo = (*it);

//		if (!pCategoryInfo->lCategoryInfoList.empty())
		{
			CleanupCategory(pCategoryInfo);
		}
		it = lCategoryList.erase(it);
	}

	if (pCategory)
	{
		if (!pCategory->lMessageInfoList.empty())
		{
			auto itMessageInfo = pCategory->lMessageInfoList.begin();
			while (itMessageInfo != pCategory->lMessageInfoList.end())
			{
				delete (*itMessageInfo);
				itMessageInfo = pCategory->lMessageInfoList.erase(itMessageInfo);
			}
		}

		delete pCategory;
	}
}

EstiBool CstiMessageManager::CoreResponseHandle (
	CstiCoreResponse *pCoreResponse)
{
	EstiBool bHandled = estiFALSE;

	switch (pCoreResponse->ResponseGet ())
	{
		case CstiCoreResponse::eCLIENT_LOGOUT_RESULT:
			if (pCoreResponse->ResponseResultGet () == estiOK)
			{
				AuthenticatedSet (estiFALSE);
			}
			break;
		default:
			break;
	}

	return bHandled;
}

/*!
 * \brief Sets the enabled status of the MessageManager. If the MessageManager is being enabled it will call 
 * 		  the Refresh() function. 
 *  
 * \param bEnabled is set to estiTRUE to enable the MessageManager and estiFALSE to disable the MessageManger.
 */
void CstiMessageManager::EnabledSet (EstiBool bEnabled)
{
	Lock();

	if (bEnabled != m_bEnabled)
	{
		m_bEnabled = bEnabled;

		if (m_bEnabled == estiTRUE)
		{
			Refresh();
		}
	}

	Unlock();
}

int CstiMessageManager::ExpirationTimerCB (
	size_t param)
{
	auto pThis = (CstiMessageManager *)param;
	pThis->Lock();

	// Remove expired messages from the tree.
	pThis->RemoveExpiredMessages(&(pThis->m_lCategoryListItems));
	pThis->RemoveEmptyCategories(&(pThis->m_lCategoryListItems));
	pThis->NotifyChangedCategories();
	pThis->ExpirationTimerStart();

	pThis->Unlock();
	
	return (0);
}

stiHResult CstiMessageManager::ExpirationTimerStart()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	if (m_wdExpirationTimer)
	{
		stiOSWdCancel (m_wdExpirationTimer);
	}

	time_t currentTime;
	time_t expireTime;
	time(&currentTime);

	// Add 24 hours to the current time to advance 1 day. (24 * 60 * 60)
	expireTime = currentTime +  86400;

	// Set the hour and minutes to expire just after midnight.
	struct tm expTime{};
	localtime_r(&expireTime, &expTime);
	expTime.tm_hour = 0;
	expTime.tm_min = 10;
	expTime.tm_sec = 0;
	expireTime = mktime(&expTime);
	

	auto  unTimeDiff = static_cast<int32_t>(difftime(expireTime, currentTime));

	// Convert wait time to milliseconds.
	stiOSWdStart (m_wdExpirationTimer, unTimeDiff * 1000, (stiFUNC_PTR) &ExpirationTimerCB, (size_t)this);

	Unlock();

	return hResult;
}

/*!
 * \brief Finds the category matching the category ID or name.
 *  
 * \param Only one of the parameters needs to be passed into the function.
 * 		  pCategoryId is a CstiItemId identifying the category 
 *  	  pszToken is the Token passed from the VMA used to look up the name of the category.
 *  	  
 * \return A pointer to a SstiCategoryInfo. 
 */
void CstiMessageManager::FindCategory(
	const CstiItemId &pCategoryId,
	const char *pszToken,
	SstiCategoryInfo **ppFoundCategory,
	unsigned int *punIndex) const
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	std::list<SstiCategoryInfo *>::const_iterator itCategory;
	std::list<SstiCategoryInfo *>::const_iterator itSubCategory;
	SstiCategoryInfo *pCategory = nullptr;
	unsigned int parentIndex = 0;
	unsigned int subIndex = 0;
	
	Lock();
	
	stiTESTCOND (pCategoryId.IsValid () || pszToken, stiRESULT_INVALID_PARAMETER);
	
	// Loop through each category and see if it matches.
	for (itCategory = m_lCategoryListItems.begin();
		 itCategory != m_lCategoryListItems.end();
		 itCategory++)
	{
		if ((*(*itCategory)->pCategoryId == pCategoryId) ||
			(pszToken && !strcmp((*itCategory)->token.c_str(), pszToken)))
		{
			pCategory = *itCategory;
			if (punIndex)
			{
				*punIndex = parentIndex;
			}
			break;
		}
		if (!(*itCategory)->lCategoryInfoList.empty())
		{
			subIndex = 0;
			for (itSubCategory = (*itCategory)->lCategoryInfoList.begin();
				 itSubCategory != (*itCategory)->lCategoryInfoList.end();
				 itSubCategory++)
			{
				if ((pCategoryId.IsValid () && *(*itSubCategory)->pCategoryId == pCategoryId) ||
					(pszToken && !strcmp((*itSubCategory)->token.c_str(), pszToken)))
				{
					pCategory = *itSubCategory;
					if (punIndex)
					{
						*punIndex = subIndex;
					}
					break;
				}
				subIndex++;
			}

			if (pCategory)
			{
				break;
			}
		}
		parentIndex++;
	}
	
STI_BAIL:
	Unlock();

	*ppFoundCategory = pCategory;
}

void CstiMessageManager::Initialize ()
{
	Lock();

#if APPLICATION != APP_NTOUCH_VP2 && APPLICATION != APP_NTOUCH_VP3 && APPLICATION != APP_NTOUCH_VP4
	m_serializer.deserialize(this);
#endif

	// Find always visible channels and add them to the channel list.
	for (auto &category: g_aCategoryList)
	{
		if (category.bAlwaysVisible == estiTRUE)
		{
			CategoryCreate(category.unCategoryId, nullptr);
		}
	}

	// Set the NextId to use for an unknown (one not in the table) category.
	m_unNextId = g_nNumCategoryListItems + 1;

	m_wdExpirationTimer = stiOSWdCreate ();

	m_bInitialized = estiTRUE;

	Unlock();
}

void CstiMessageManager::lastCategoryUpdateSet()
{
	// Get the current time and pass it into LastCategoryUpdateSet()
	LastCategoryUpdateSet(time(nullptr));
}

/*!
 * \brief Sets the time of the last category update.  Messages received after
 *        this time will be counted as "new".
 *
 * \param unLastTime The time of the last update
 */
void CstiMessageManager::LastCategoryUpdateSet(time_t unLastTime)
{
	if (m_unCategoryLastUpdate == 0 ||
		0.0 < difftime(unLastTime, m_unCategoryLastUpdate))
	{
		m_unCategoryLastUpdate = unLastTime; 
		lastMessageUpdateSignal.Emit(m_unCategoryLastUpdate);
	}
}

void CstiMessageManager::lastSignMailUpdateSet()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Get the time of the last SignMail and pass it into LastSignMailUpdateSet()
	time_t updateTime = 0;
	CstiMessageInfo signMail;
	hResult = MessageByIndexGet(g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId, 0, &signMail);
	if (stiIS_ERROR(hResult))
	{
		updateTime = time(nullptr);
	}
	else
	{
		updateTime = signMail.DateTimeGet();
	}
	LastSignMailUpdateSet(updateTime);
}

/*!
 * \brief Sets the time of the last SignMail update.  Messages received after
 *        this time will be counted as "new".
 *
 * \param unLastTime The time of the last update
 */
void CstiMessageManager::LastSignMailUpdateSet(time_t unLastTime) 
{ 
	if (unLastTime == INT_MAX) {
		m_unSignMailLastUpdate = 0;
	}
	else {
		m_unSignMailLastUpdate = unLastTime;
	}
	lastSignMailUpdateSignal.Emit(m_unSignMailLastUpdate);
	
}

void CstiMessageManager::MarkMessagesDirty(
	std::list<SstiCategoryInfo *> *plCategoryList)
{
	// Loop through each category and mark each message as dirty.
	std::list<SstiCategoryInfo *>::iterator itCategory;
	for (itCategory = plCategoryList->begin();
		 itCategory != plCategoryList->end();
		 itCategory++)
	{
		// Clear the new message count
		(*itCategory)->unUnviewedMessageCount = 0;

		// If a category has sub categories recurse into the sub categories list.
		if (!(*itCategory)->lCategoryInfoList.empty())
		{
			MarkMessagesDirty(&(*itCategory)->lCategoryInfoList);
		}
		else if (!(*itCategory)->lMessageInfoList.empty())
		{
			std::list<CstiMessageInfo *>::iterator itMessage;
			for (itMessage = (*itCategory)->lMessageInfoList.begin();
				 itMessage != (*itCategory)->lMessageInfoList.end();
				 itMessage++)
			{
				(*itMessage)->DirtySet(estiTRUE);
			}
		}
	}
}

/*!
 * \brief Gets the CstiMessageInfo based on the itemId
 *  
 * \param pMessageId is a CstiItemId object that specifies CstiMessageInfo we are looking for.
 *  
 * \return A pointer to the CstiMessageInfo if successfull, NULL otherwise. 
 */
void CstiMessageManager::MessageByIdGet(
	const CstiItemId &pMessageId,
	std::list<SstiCategoryInfo *> lCategoryList,
	CstiMessageInfo **ppMessageInfo,
	SstiCategoryInfo **ppCategoryInfo) const
{
	CstiMessageInfo *pMsgInfo = nullptr;
	SstiCategoryInfo *pCatInfo = nullptr;

	std::list<SstiCategoryInfo *>::iterator itCategoryInfo;
	for (itCategoryInfo = lCategoryList.begin();
		 itCategoryInfo != lCategoryList.end();
		 itCategoryInfo++)
	{
		if (!(*itCategoryInfo)->lMessageInfoList.empty())
		{
			std::list<CstiMessageInfo *>::iterator itMessageInfo;
			for (itMessageInfo = (*itCategoryInfo)->lMessageInfoList.begin();
				 itMessageInfo != (*itCategoryInfo)->lMessageInfoList.end();
				 itMessageInfo++)
			{
				if (pMessageId == (*itMessageInfo)->ItemIdGet())
				{
					pMsgInfo = (*itMessageInfo);
					pCatInfo = (*itCategoryInfo);
					break;
				}
			}
		}

		if (!pMsgInfo && 
			!(*itCategoryInfo)->lCategoryInfoList.empty())
		{
			MessageByIdGet(pMessageId,
						   (*itCategoryInfo)->lCategoryInfoList, 
						   &pMsgInfo,
						   &pCatInfo);
		}

		if (pMsgInfo)
		{
			*ppMessageInfo = pMsgInfo;

			if (ppCategoryInfo)
			{
				*ppCategoryInfo = pCatInfo;
			}
			break;
		}
	}
}

/*!
 * \brief Populates a CstiMessageItem based on the categoryId and index. 
 *  
 * \param categoryId is a CstiItemId object that specifies a category.
 * 		  pnIndex is a pointer to an integer used to return the 
 *  	  pMessageItem is a CstiMessageItem that is passed in and the Message Manager
 * 		  will populate its data members.
 *  
 * \return stiRESULT_SUCCESS if successful 
 * 		   stiRESULT_INVALID_PARAMETER if categoryId is invalid, not found in the list or 
 * 		   							   unIndex is larger than the number of messages in the list,
 * 		   stiRESULT_ERROR otherwise. 
 */
stiHResult CstiMessageManager::MessageByIndexGet (
	const CstiItemId &categoryId,
	unsigned int unIndex,
	CstiMessageInfo *pMessageItem) const 
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;

	std::list<CstiMessageInfo *>::iterator itMessage;
	unsigned int unMessageIndex = 0; 
	unsigned int unCount = 0; stiUNUSED_ARG (unCount);
	Lock();
	
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);
	
	// Find the category.
	FindCategory(categoryId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_INVALID_PARAMETER);

	unCount = pCategory->lMessageInfoList.size();

	// Make sure the requested index is valid.
	stiTESTCOND (unIndex < pCategory->lMessageInfoList.size(), stiRESULT_INVALID_PARAMETER);

	// Find the message for the given index.
	itMessage = pCategory->lMessageInfoList.begin(); 
	while (unMessageIndex < unIndex)
	{
		unMessageIndex++;
		itMessage++;
	}
	CstiMessageInfo *pMessage;
	pMessage = (*itMessage);

	// Fill in the CstiMessageItem.
	pMessageItem->ItemIdSet(pMessage->ItemIdGet());
	pMessageItem->DateTimeSet(pMessage->DateTimeGet());
	pMessageItem->DialStringSet(pMessage->DialStringGet());
	pMessageItem->MessageTypeIdSet (pMessage->MessageTypeIdGet ());
	pMessageItem->NameSet(pMessage->NameGet());
	pMessageItem->PausePointSet(pMessage->PausePointGet());
	pMessageItem->MessageLengthSet(pMessage->MessageLengthGet());
	pMessageItem->DialStringSet(pMessage->DialStringGet());
	pMessageItem->ViewedSet(pMessage->ViewedGet());
	pMessageItem->InterpreterIdSet(pMessage->InterpreterIdGet());

	pMessageItem->FeaturedVideoSet(pMessage->FeaturedVideoGet());
	pMessageItem->FeaturedVideoPosSet(pMessage->FeaturedVideoPosGet());
	pMessageItem->AirDateSet(pMessage->AirDateGet());
	pMessageItem->ExpirationDateSet(pMessage->ExpirationDateGet());
	pMessageItem->DescriptionSet(pMessage->DescriptionGet());
	pMessageItem->PreviewImageURLSet(pMessage->PreviewImageURLGet());

STI_BAIL:
	Unlock();
	
	return hResult;
}

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
stiHResult CstiMessageManager::MessageCategoryGet (
	const CstiItemId &messageId, 
	CstiItemId **ppCategoryId) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageInfo *pMessageInfo = nullptr;
	SstiCategoryInfo *pCategoryInfo = nullptr;

	Lock();

	stiTESTCOND (messageId.IsValid() && ppCategoryId, stiRESULT_INVALID_PARAMETER);

	// Find the message.
	MessageByIdGet(messageId,
				   m_lCategoryListItems,
				   &pMessageInfo,
				   &pCategoryInfo);

	stiTESTCOND (pMessageInfo, stiRESULT_INVALID_PARAMETER);

	// Allocate an itemId for the category and return it.
	*ppCategoryId = new CstiItemId(*pCategoryInfo->pCategoryId);

STI_BAIL:
	Unlock();

	return hResult;
}

/*!
 * \brief Returns the number of messages that a category contains. 
 *  
 * \param categoryId is a referance to a CstiItemId object that specifies a category.
 * 		  pnMessageCount is a pointer to an integer used to return the 
 *  	  number of messages the category specified by pCategoryId.
 *  
 * \return stiRESULT_SUCCESS if successful 
 * 		   stiRESULT_INVALID_PARAMETER if pCategoryid is NULL or not found in the list,
 * 		   stiRESULT_ERROR otherwise. 
 */
stiHResult CstiMessageManager::MessageCountGet (
	const CstiItemId &categoryId,
	unsigned int *punMessageCount) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
	
	Lock();
	
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);
	
	// Find the matching Category.
	FindCategory(categoryId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_INVALID_PARAMETER);
	
	*punMessageCount = pCategory->lMessageInfoList.size();
	
STI_BAIL:
	Unlock();

	return hResult;
}

/*!
 * \brief Gets the data of a CstiMessageInfo based on the categoryId and itemId 
 * 		  contained in the CstiMessageInfo. 
 *  
 * \param pMessageItem is a CstiMessageInfo that is passed in with a valid itemId (CstiItemId)
 *  	  for the desired message.
 *  
 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
 */
stiHResult CstiMessageManager::MessageInfoGet (
	CstiMessageInfo *pMessageItem) const 
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageInfo *pMsgInfo = nullptr;

	Lock();
	MessageByIdGet(pMessageItem->ItemIdGet(),
				   m_lCategoryListItems,
				   &pMsgInfo,
				   nullptr);
	stiTESTCOND (pMsgInfo, stiRESULT_ERROR);

	pMessageItem->DateTimeSet(pMsgInfo->DateTimeGet());
	pMessageItem->DialStringSet(pMsgInfo->DialStringGet());
	pMessageItem->NameSet(pMsgInfo->NameGet());
	pMessageItem->PausePointSet(pMsgInfo->PausePointGet());
	pMessageItem->MessageLengthSet(pMsgInfo->MessageLengthGet());
	pMessageItem->ViewedSet(pMsgInfo->ViewedGet());
	pMessageItem->InterpreterIdSet(pMsgInfo->InterpreterIdGet());
	pMessageItem->FeaturedVideoSet(pMsgInfo->FeaturedVideoGet());
	pMessageItem->FeaturedVideoPosSet(pMsgInfo->FeaturedVideoPosGet());
	pMessageItem->AirDateSet(pMsgInfo->AirDateGet());
	pMessageItem->ExpirationDateSet(pMsgInfo->ExpirationDateGet());
	pMessageItem->DescriptionSet(pMsgInfo->DescriptionGet());
	pMessageItem->PreviewImageURLSet(pMsgInfo->PreviewImageURLGet());
	pMessageItem->MessageTypeIdSet(pMsgInfo->MessageTypeIdGet());

STI_BAIL:

	Unlock();

	return hResult;
}

/*!
 * \brief Sets the data contained in the CstiMessageItem, based on the categoryId and itemId 
 * 		  of the CstiMessageItem, on the message object maintained by the Message Manager. 
 *  
 * \param pMessageItem is a CstiMessageItem that is passed in with a valid itemId (CstiItemId)
 *  	  and all other data members set to the desired state.
 *  
 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
 */
stiHResult CstiMessageManager::MessageInfoSet (
	CstiMessageInfo *pMessageItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageInfo *pMsgInfo = nullptr;
	SstiCategoryInfo *pCategoryInfo = nullptr;

	Lock();
	MessageByIdGet(pMessageItem->ItemIdGet(),
				   m_lCategoryListItems,
				   &pMsgInfo,
				   &pCategoryInfo);
	stiTESTCOND (pMsgInfo, stiRESULT_ERROR);

	// Check for data that should be changed.  If it has changed then update it
	// locally and if necessary notify core.
	if (pMsgInfo->PausePointGet() != pMessageItem->PausePointGet())
	{
		pMsgInfo->PausePointSet(pMessageItem->PausePointGet());

		auto pMessageRequest = new CstiMessageRequest;

		if (pMessageRequest)
		{
			pMessageRequest->MessagePausePointSave(pMsgInfo->ItemIdGet(),
												   pMessageItem->ViewIdGet(),
												   pMsgInfo->PausePointGet());

			pMessageRequest->contextItemSet(pMsgInfo->ItemIdGet());
			MessageRequestSend(pMessageRequest, nullptr);
		}

	}

	if (pMsgInfo->ViewedGet() != pMessageItem->ViewedGet())
	{
		pMsgInfo->ViewedSet(pMessageItem->ViewedGet());
		pCategoryInfo->unUnviewedMessageCount--;

		auto pMessageRequest = new CstiMessageRequest;

		if (pMessageRequest)
		{
			pMessageRequest->MessageViewed (pMsgInfo->ItemIdGet(), 
											pMsgInfo->NameGet(),
											pMsgInfo->DialStringGet());

			pMessageRequest->contextItemSet(pMsgInfo->ItemIdGet());
			MessageRequestSend(pMessageRequest, nullptr);
		}
	}

STI_BAIL:

	Unlock();

	return hResult;
}

EstiBool CstiMessageManager::MessageResponseHandle (
		CstiMessageResponse *pMsgResponse)
{
	EstiBool bHandled = estiFALSE;

	switch (pMsgResponse->ResponseGet ())
	{
		case CstiMessageResponse::eMESSAGE_LIST_GET_RESULT:
			if (RemoveRequestId (pMsgResponse->RequestIDGet ()))
			{
				if (pMsgResponse->ErrorGet() == CstiMessageResponse::eNO_ERROR)
				{
					Lock();
					DBG_MSG ("MessageResponse eMESSAGE_LIST_GET_RESULT (id %d) = %s", pMsgResponse->RequestIDGet (), pMsgResponse->ResponseResultGet() == estiOK ? "OK" : "Err");

					ParseMessageList(pMsgResponse);

					NotifyChangedCategories();

					Unlock();
				}
				pMsgResponse->destroy ();
				bHandled = estiTRUE;
			}
			break;

#if 0 // Currently we are not looking for this message but we may in the future.
		case CstiMessageResponse::eNEW_MESSAGE_COUNT_GET_RESULT:
			if (RemoveRequestId (pMsgResponse->RequestIDGet ()))
			{
				DBG_MSG ("MessageResponse eNEW_MESSAGE_COUNT_GET_RESULT (id %d) = %s", pMsgResponse->RequestIDGet (), pMsgResponse->ResponseResultGet() == estiOK ? "OK" : "Err");


				pMsgResponse->destroy ();
				bHandled = estiTRUE;

			}
			break;
#endif

		case CstiMessageResponse::eMESSAGE_DELETE_RESULT:
			if (RemoveRequestId (pMsgResponse->RequestIDGet ()))
			{
				if (pMsgResponse->ErrorGet() == CstiMessageResponse::eNO_ERROR)
				{
					SstiCategoryInfo *pCategoryInfo = nullptr;
					CstiItemId *pCategoryId = nullptr;

					Lock();
					CstiItemId pItemId = pMsgResponse->contextItemGet<CstiItemId> ();

					// If we don't have an itemId we are probably deleting all messages in a category
					// and we only want to process the clean up on the last message delete, which
					// will contain the itemId of the category.
					// TODO: this used to be a pointer check, does this work?
					if (pItemId.IsValid ())
					{
						// Look for a Category.
						FindCategory(pItemId, 
									 nullptr,
									 &pCategoryInfo,
									 nullptr);

						// If we found a matching category we are clearing the messages from a category.
						// otherwise we are just removing one message so find it.
						if (pCategoryInfo)
						{
							// Reset the unviewed message count to 0, just in case the category is one that is always visible.
							pCategoryInfo->unUnviewedMessageCount = 0;

							// If this is the SignMail Category then clear the signmail count because it will now
							// be empty.
							uint32_t un32IdValue = 0;
							pCategoryInfo->pCategoryId->ItemIdGet(&un32IdValue);
							if (un32IdValue == g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId)
							{
								pCategoryInfo->unSignMailCount = 0;
							}

							if (!pCategoryInfo->lMessageInfoList.empty())
							{
								auto itMessageInfo = pCategoryInfo->lMessageInfoList.begin();
								while (itMessageInfo != pCategoryInfo->lMessageInfoList.end())
								{
									delete (*itMessageInfo);
									itMessageInfo = pCategoryInfo->lMessageInfoList.erase(itMessageInfo);
								}
							}
						}
						else
						{
							CstiMessageInfo *pMessageInfo = nullptr;

							// Find the message and its parent category.
							MessageByIdGet(pItemId,
										   m_lCategoryListItems,
										   &pMessageInfo,
										   &pCategoryInfo);

							if (pMessageInfo)
							{
								// If this message is a signmail message then we need to decrement the
								// signmail count.
								if ((pMessageInfo->MessageTypeIdGet () == estiMT_SIGN_MAIL || 
										pMessageInfo->MessageTypeIdGet () == estiMT_DIRECT_SIGNMAIL ||
										pMessageInfo->MessageTypeIdGet () == estiMT_P2P_SIGNMAIL ||
										pMessageInfo->MessageTypeIdGet () == estiMT_THIRD_PARTY_VIDEO_MAIL)
									&& pCategoryInfo->unSignMailCount > 0)
								{
									pCategoryInfo->unSignMailCount--;
								}

								pCategoryInfo->lMessageInfoList.remove(pMessageInfo);
								delete pMessageInfo;
								pMessageInfo = nullptr;

								// Reset the new Message count, we may have deleted an unviewed message.
								pCategoryInfo->unUnviewedMessageCount = 0;
								std::list<CstiMessageInfo *>::iterator itMsgInfo;
								for (itMsgInfo = pCategoryInfo->lMessageInfoList.begin();
									 itMsgInfo != pCategoryInfo->lMessageInfoList.end();
									 itMsgInfo++)
								{
									if ((*itMsgInfo)->ViewedGet() == estiFALSE)
									{
										pCategoryInfo->unUnviewedMessageCount++;
									}
								}
							}
						}

						// Check to see we need to remove the category.
						// pCategoryInfo can be null when quickly deleting many SignMail.
						if (nullptr != pCategoryInfo &&
							(pCategoryInfo->bAlwaysVisible == estiTRUE ||
							!pCategoryInfo->lMessageInfoList.empty()))
						{
							// Save the categories ID so the UI can be notified that it changed.
							pCategoryId = new CstiItemId(*pCategoryInfo->pCategoryId);

							m_pfnVPCallback (estiMSG_MESSAGE_CATEGORY_CHANGED, 
											 (size_t)pCategoryId, 
											  m_CallbackParam, m_CallbackFromId);
						}
						else
						{
							// RemoveEmptyCategories will update the UI if the category is removed.
							RemoveEmptyCategories(&m_lCategoryListItems);
						}
					}

					Unlock();
				}
				else
				{
					Lock();
					auto itemId = pMsgResponse->contextItemGet<CstiItemId> ();

					if (itemId.IsValid ())
					{
						// Save the categories ID so the UI can be notified that it changed.
						auto pDeleteId = new CstiItemId(itemId);

						m_pfnVPCallback (estiMSG_MESSAGE_DELETE_FAIL,
										 (size_t)pDeleteId,
										  m_CallbackParam, m_CallbackFromId);
					}

					Unlock();
				}    

				pMsgResponse->destroy ();
				bHandled = estiTRUE;
			}
			break;

		case CstiMessageResponse::eMESSAGE_VIEWED_RESULT:
		case CstiMessageResponse::eMESSAGE_LIST_ITEM_EDIT_RESULT:
			if (RemoveRequestId (pMsgResponse->RequestIDGet ()))
			{
				if (pMsgResponse->ErrorGet() == CstiMessageResponse::eNO_ERROR)
				{
					Lock();
					auto itemId = pMsgResponse->contextItemGet<CstiItemId> ();

					auto pCategoryId = new CstiItemId(itemId);

					m_pfnVPCallback (estiMSG_MESSAGE_ITEM_CHANGED, 
									 (size_t)pCategoryId, 
									  m_CallbackParam, m_CallbackFromId);
					Unlock();
				}
			}
			break;
		case CstiMessageResponse::eMESSAGE_DOWNLOAD_URL_LIST_GET_RESULT:
			if (pMsgResponse->ResponseResultGet () == estiOK)
			{
				auto propertyMgr = PropertyManager::getInstance ();
				//Used for codec sorting to assign weights to the codecs
				auto codecWeights = [](std::string codec)
				{
					if (codec == codecKeyHEVC)
					{
						return 2;
					}
					if (codec == codecKeyH264)
					{
						return 1;
					}
					return 0;
				};

				//Sorting function to sort by HEVC and then by download speed.
				auto itemComparer = [codecWeights](CstiMessageResponse::CstiMessageDownloadURLItem msg1, CstiMessageResponse::CstiMessageDownloadURLItem msg2)
				{
					if (msg1.m_Codec == msg2.m_Codec)
					{
						return msg1.m_nMaxBitRate > msg2.m_nMaxBitRate;
					}
					return codecWeights (msg1.m_Codec) > codecWeights (msg2.m_Codec);
				};

				auto messageList = *pMsgResponse->MessageDownloadUrlListGet ();

				//Filter out unsupported codecs
				
				auto hevcPropertyEnabled = false;
				propertyMgr->propertyGet (HEVC_SIGNMAIL_PLAYBACK_ENABLED, &hevcPropertyEnabled);
				auto hevcEnabled = m_hevcSignMailPlaybackSupported && hevcPropertyEnabled;
				auto removedItems = std::remove_if (messageList.begin (), messageList.end (), [hevcEnabled](CstiMessageResponse::CstiMessageDownloadURLItem message)
				{
					return (!message.m_Codec.empty()) && 						//If codec is not defined assume h.264
							(message.m_Codec != codecKeyH264) && 				//We don't remove if it's h.264
							!(hevcEnabled && message.m_Codec == codecKeyHEVC); 	//We might support h.265 (everything else is unsupported)
				});

				messageList.erase (removedItems, messageList.end());

				//Sort by codec and then by DownloadSpeed
				std::sort (messageList.begin (), messageList.end (), itemComparer);
				//Switching to use HTTPS instead of HTTP
				for (auto& it : messageList)
				{
					protocolSchemeChange (it.m_DownloadURL, "https");
				}
				pMsgResponse->MessageDownloadUrlListSet (messageList);
			}
			break;
		default:
			break;
	}

	m_saveTimer.start ();

	return bHandled;
}

void CstiMessageManager::updateCategoryUpdateTime(
	const SstiCategoryInfo *category)
{
	// Use the current time as the update time.
	uint32_t itemId = 0;
	category->pCategoryId->ItemIdGet(&itemId);

	// SignMail Category
	if (itemId == g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId)
	{
		lastSignMailUpdateSet();
	}
	// All other categories.
	else 
	{
		lastCategoryUpdateSet();
	}
}

/*!
 * \brief Deletes the message item, specified by the msgItemId, from the message server.
 *  
 * \param msgItemId is a referance to the unique ID of the message item that should be deleted from the message server
 * 
 */
stiHResult CstiMessageManager::MessageItemDelete (
	const CstiItemId &msgItemId)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	CstiMessageInfo *pMessageInfo = nullptr;
	SstiCategoryInfo *pCategoryInfo = nullptr;

	Lock();
	stiTESTCOND (msgItemId.IsValid(), stiRESULT_INVALID_PARAMETER);

	// Find the message.
	MessageByIdGet(msgItemId,
				   m_lCategoryListItems,
				   &pMessageInfo,
				   &pCategoryInfo);

	if (pMessageInfo)
	{
		// Update the lastUpdateTime.
		updateCategoryUpdateTime(pCategoryInfo);

		auto pMessageRequest = new CstiMessageRequest();
		pMessageRequest->MessageDelete(pMessageInfo->ItemIdGet(), 
									   pMessageInfo->NameGet(),
									   pMessageInfo->DialStringGet());
		pMessageRequest->contextItemSet(pMessageInfo->ItemIdGet());
		hResult = MessageRequestSend(pMessageRequest, nullptr);

		stiTESTRESULT();
	}

STI_BAIL:
	Unlock();

	return hResult;
}


stiHResult CstiMessageManager::MessageRequestSend (
	CstiMessageRequest *poMsgRequest,
	unsigned int *punRequestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unInternalMsgRequestId = 0;

	if (m_pMessageService)
	{
		poMsgRequest->RemoveUponCommunicationError () = estiTRUE;
		EstiResult eResult = m_pMessageService->RequestSend (poMsgRequest,
															 &unInternalMsgRequestId);
		hResult = stiRESULT_CONVERT (eResult);
		if (stiRESULT_SUCCESS == hResult)
		{
			m_PendingRequestIds.push_back (unInternalMsgRequestId);
		}
	}

	DBG_MSG ("Core request %d sent.", unInternalMsgRequestId);

	if (punRequestId)
	{
		*punRequestId = unInternalMsgRequestId;
	}

	return hResult;
}

/*!
 * \brief Deletes all messages in the category specified by the category ID.
 *  
 * \param categoryId is a referance to the unique ID of the category that should have its messages deleted from the message server
 * 
 */
stiHResult CstiMessageManager::MessagesInCategoryDelete(
	const CstiItemId &categoryId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory;

	Lock();
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);

	// Find the Category.
	FindCategory(categoryId,
				 nullptr,
				 &pCategory,
				 nullptr);

	if (pCategory &&
		!pCategory->lMessageInfoList.empty())
	{
		// Create a message request.
		std::list<CstiMessageInfo *>::const_iterator itMsgInfo;

		auto pMessageRequest = new CstiMessageRequest();

		updateCategoryUpdateTime(pCategory);

		itMsgInfo = pCategory->lMessageInfoList.begin(); 
		while (itMsgInfo != pCategory->lMessageInfoList.end())
		{
			// Add messages to the Delete message reqeust.
			pMessageRequest->MessageDelete((*itMsgInfo)->ItemIdGet(), 
										   (*itMsgInfo)->NameGet(),
										   (*itMsgInfo)->DialStringGet());

			itMsgInfo++;

			// If we are deleting the last message in the category then set the categoryId on the reqeust.
			if (itMsgInfo == pCategory->lMessageInfoList.end())
			{
				pMessageRequest->contextItemSet((*pCategory->pCategoryId));
			}
		}

		hResult = MessageRequestSend(pMessageRequest, nullptr);
		stiTESTRESULT();
	}

STI_BAIL:
	Unlock();

	return hResult;
}

/*!
 * \brief Deletes all messages in the category specified by the category ID.
 *
 * \param categoryId is a referance to the unique ID of the category that should have its messages deleted from the message server
 *
 */
stiHResult CstiMessageManager::MessagesInCategoryDelete(
	const CstiItemId &categoryId,
	int nCount)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory;

	Lock();
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);

	// Find the Category.
	FindCategory(categoryId,
				 nullptr,
				 &pCategory,
				 nullptr);

	if (pCategory &&
		!pCategory->lMessageInfoList.empty())
	{
		// Update the category update time.
		updateCategoryUpdateTime(pCategory);

		// Create a message request.
		std::list<CstiMessageInfo *>::const_iterator itMsgInfo;
		int nNumDeleted = 0;

		auto pMessageRequest = new CstiMessageRequest();

		itMsgInfo = pCategory->lMessageInfoList.begin();
		while (nNumDeleted < nCount && itMsgInfo != pCategory->lMessageInfoList.end())
		{
			// Add messages to the Delete message reqeust.
			pMessageRequest->MessageDelete((*itMsgInfo)->ItemIdGet(),
										   (*itMsgInfo)->NameGet(),
										   (*itMsgInfo)->DialStringGet());

			itMsgInfo++;
			nNumDeleted++;

			// If we are deleting the last message in the category then set the categoryId on the reqeust.
			if (nNumDeleted == nCount || itMsgInfo == pCategory->lMessageInfoList.end())
			{
				pMessageRequest->contextItemSet(*pCategory->pCategoryId);
			}
		}

		hResult = MessageRequestSend(pMessageRequest, nullptr);
		stiTESTRESULT();
	}
STI_BAIL:
	Unlock();

	return hResult;
}


/*!
 * \brief Returns the number of new messages that a category contains. 
 *  
 * \param categoryId is a referance to a CstiItemId object that specifies a category.
 * 		  punNewMessageCount is a pointer to an unsigned integer used to return the 
 *  	  number of new messages the category specified by pCategoryId.
 *  
 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
 */
stiHResult CstiMessageManager::NewMessageCountGet (
	const CstiItemId &categoryId,
	unsigned int *punNewMessageCount) const 
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
	unsigned int unNewMsgCount = 0;

	Lock();
	
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);
	
	// Find the category.
	FindCategory(categoryId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_INVALID_PARAMETER);

	if (!pCategory->lCategoryInfoList.empty())
	{
		// Search all sub categories for new messages.
		std::list<SstiCategoryInfo *>::iterator itCategory;
		for (itCategory = pCategory->lCategoryInfoList.begin(); 
			 itCategory != pCategory->lCategoryInfoList.end(); 
			 itCategory++)
		{
			unsigned int unSubNewMsgCount = 0;
			NewMessageCountGet(*(*itCategory)->pCategoryId, &unSubNewMsgCount);
			unNewMsgCount += unSubNewMsgCount;
		}
	}
	else if (!pCategory->lMessageInfoList.empty())
	{
		uint32_t un32Id = 0;

		// Check the category ID to see if it is the SignMail category.  If it is
		// the SignMail category then use SignMail last check time. Otherwise use 
		// the cateogry last check time.
		time_t unTimeCheck = m_unCategoryLastUpdate;
		pCategory->pCategoryId->ItemIdGet(&un32Id);
		if (un32Id == g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId)
		{
			unTimeCheck = m_unSignMailLastUpdate;
		}

		// Search the message list for any new messages.
		std::list<CstiMessageInfo *>::iterator itMessage;
		for (itMessage = pCategory->lMessageInfoList.begin(); 
			 itMessage != pCategory->lMessageInfoList.end();
			 itMessage++)
		{
			CstiMessageInfo *pMsgInfo = (*itMessage);

			if (0.0 > difftime(unTimeCheck, pMsgInfo->DateTimeGet()))
			{
				unNewMsgCount++;
			}
		}
	}
	*punNewMessageCount = unNewMsgCount;

STI_BAIL:
	Unlock();
	return hResult;
}

stiHResult CstiMessageManager::NotifyChangedCategories()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Search the category list looking for changes.
	std::list<SstiCategoryInfo *>::iterator itCategory;
	for (itCategory = m_lCategoryListItems.begin();
		 itCategory != m_lCategoryListItems.end(); 
		 itCategory++)
	{
		if ((*itCategory)->bCategoryChanged)
		{
			// If the category has changed then we need to notify the UI.
			if (m_pfnVPCallback)
			{
				// If the category has sub categories then find the changed categories.
				if (!(*itCategory)->lCategoryInfoList.empty())
				{
					std::list<SstiCategoryInfo *>::iterator itSubCategory;

					// Check to see how many sub categories have changed.  If more than one
					// then notify the UI that the root has changed otherwise just notify the
					// UI of the changed subcategory.
					unsigned int unChangedCount = 0;
					SstiCategoryInfo *pFirstChangedCategory = nullptr;
					EstiBool bUiNotified = estiFALSE;
					for (itSubCategory = (*itCategory)->lCategoryInfoList.begin(); 
						 itSubCategory != (*itCategory)->lCategoryInfoList.end(); 
						 itSubCategory++)
					{
						if ((*itSubCategory)->bCategoryChanged)
						{
							unChangedCount++;

							// Save the first change category we find.
							if (!pFirstChangedCategory)
							{
								pFirstChangedCategory = (*itSubCategory);
							}

							if (unChangedCount > 1) 
							{
								if (bUiNotified == estiFALSE)
								{
									// Notify the UI that the category has changed.
									auto pItemId = new CstiItemId(*(*itCategory)->pCategoryId);
									m_pfnVPCallback (estiMSG_MESSAGE_CATEGORY_CHANGED, 
													 (size_t)pItemId, 
													  m_CallbackParam, m_CallbackFromId);

									bUiNotified = estiTRUE;

									// Clear the categories changed status.
									(*itSubCategory)->bCategoryChanged = estiFALSE;
									pFirstChangedCategory->bCategoryChanged = estiFALSE;
								}
								else
								{
									// The UI has been notified so clear the subcategory's changed status.
									(*itSubCategory)->bCategoryChanged = estiFALSE;
								}
							}
						}
					}

					if (unChangedCount == 1)
					{
						// Notify the UI that the category has changed.
						auto pItemId = new CstiItemId(*pFirstChangedCategory->pCategoryId);
						m_pfnVPCallback (estiMSG_MESSAGE_CATEGORY_CHANGED, 
										 (size_t)pItemId, 
										  m_CallbackParam, m_CallbackFromId);

						pFirstChangedCategory->bCategoryChanged = estiFALSE;
					}

					// If we have a message list at the root make sure we update them as well.
					if (!(*itCategory)->lMessageInfoList.empty())
					{
						auto pItemId = new CstiItemId(*(*itCategory)->pCategoryId);
						m_pfnVPCallback (estiMSG_MESSAGE_CATEGORY_CHANGED,
										 (size_t)pItemId,
										  m_CallbackParam, m_CallbackFromId);
					}
				}
				else
				{
					auto pItemId = new CstiItemId(*(*itCategory)->pCategoryId);
					m_pfnVPCallback (estiMSG_MESSAGE_CATEGORY_CHANGED, 
									 (size_t)pItemId, 
									  m_CallbackParam, m_CallbackFromId);
				}
			}

			// Clear the changed status.
			(*itCategory)->bCategoryChanged = estiFALSE;
		}
	}

	return hResult;
}

stiHResult CstiMessageManager::ParseMessageList(
	CstiMessageResponse *pMessageResponse)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
	CstiMessageListItemSharedPtr pMessageItem;
	EstiMessageType msgType;

	Lock();
	CstiMessageList *pMessageList = pMessageResponse->MessageListGet();
	unsigned int unMessageCount = pMessageList->CountGet();

	// Update the sign mail counts.
	pCategory = CategoryByIdGet(g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId);
	stiTESTCOND (pCategory, stiRESULT_ERROR);
	pCategory->unSignMailCount = pMessageList->SignMailMsgCountGet();
	pCategory->unMaxSignMailCount = pMessageList->SignMailMsgLimitGet();

	// Mark all Messages as "dirty".
	MarkMessagesDirty(&m_lCategoryListItems);

	for (unsigned int i = 0; i < unMessageCount; i++)
	{
		pCategory = nullptr;
		pMessageItem = pMessageList->ItemGet(i);
		stiTESTCOND(pMessageItem, stiRESULT_ERROR);

		msgType =  pMessageItem->MessageTypeIdGet();

		switch (msgType)
		{
			case estiMT_SIGN_MAIL:
			case estiMT_P2P_SIGNMAIL:
			case estiMT_THIRD_PARTY_VIDEO_MAIL:
			case estiMT_FROM_TECH_SUPPORT:
			case estiMT_MESSAGE_FROM_OTHER:
			case estiMT_NEWS:
			case estiMT_DIRECT_SIGNMAIL:
			{
				// Get the SignMail category.
				pCategory = CategoryByIdGet(g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId);
				stiTESTCOND (pCategory, stiRESULT_ERROR);
				AddMessageItem(pMessageItem,
							   pCategory);
			}
			break;

			case estiMT_SORENSON_PRODUCT_TIPS:
			case estiMT_MESSAGE_FROM_SORENSON:
			{
				auto tokens = splitString (pMessageItem->NameGet(), '|');

				if (tokens.size () == 3)
				{
					Trim (&tokens[0]);
					Trim (&tokens[1]);

					pCategory = SubCategoryByNameGet(tokens[1].c_str (),	// subcategory
													 tokens[0].c_str ());	// category token

					if (pCategory)
					{
						//If we succeeded to get a category then set message name.
						Trim (&tokens[2]);

						pMessageItem->NameSet(tokens[2].c_str ());
					}
				}

				if (!pCategory)
				{
					pCategory = CategoryByIdGet(g_aCategoryList[SORENSON_CATEGORY_INFO_INDEX].unCategoryId);
				}

				if (pCategory)
				{
					AddMessageItem(pMessageItem, pCategory);
				}
			}
			break;

			case estiMT_NONE:
			case estiMT_INTERACTIVE_CARE:
				stiASSERTMSG(false, "Unimplemented messageTypeID: %d.", msgType);
			break;
		}
	}

	// Remove any "dirty" messages and empty categories.
	RemoveDirtyMessages(&m_lCategoryListItems);
	RemoveEmptyCategories(&m_lCategoryListItems);

	// Start the expiration timer.
	ExpirationTimerStart();

	// Check for new SignMail/Messages
	{
		unsigned int newMsgCount = 0;
		CstiItemId signMailId(g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId);
		NewMessageCountGet(signMailId, &newMsgCount);
		if (newMsgCount > 0)
		{
			// Send NewSignMail signal
			newSignMailSignal.Emit(newMsgCount);
		}

		newMsgCount = 0;
		CstiItemId videoMsgId(g_aCategoryList[SORENSON_CATEGORY_INFO_INDEX].unCategoryId);
		NewMessageCountGet(videoMsgId, &newMsgCount);
		if (newMsgCount > 0)
		{
			// Send New Video signal
			newVideoMsgSignal.Emit(newMsgCount);
		}
	}

STI_BAIL:
	Unlock();

	return hResult;
}

/*!
 * \brief Utility function that prints the category tree and messages.
 *
 * \param lCategoryList is a list containing SstiCategoryInfo objects.
 */
void CstiMessageManager::PrintMessageList(
	std::list<SstiCategoryInfo *> lCategoryList,
	int nTabCount)
{
	std::list<SstiCategoryInfo *>::iterator it;
	for (it = lCategoryList.begin();
		 it != lCategoryList.end();
		 it++)
	{
		SstiCategoryInfo *pCategoryInfo = (*it);
		for (int i = 0; i < nTabCount; i++)
		{
			stiTrace("\t");
		}
		stiTrace("Category Title: %s\n", pCategoryInfo->shortName.c_str());
		if (!pCategoryInfo->lMessageInfoList.empty())
		{
			std::list<CstiMessageInfo *>::iterator itMessageInfo;
			for (itMessageInfo = pCategoryInfo->lMessageInfoList.begin();
				 itMessageInfo != pCategoryInfo->lMessageInfoList.end();
				 itMessageInfo++)
			{
				CstiMessageInfo *pMessageInfo = (*itMessageInfo);
				for (int i = 0; i < nTabCount; i++)
				{
					stiTrace("\t");
				}
				stiTrace("\tMessage Title: %s\n", pMessageInfo->NameGet());
			}
		}

		if (!pCategoryInfo->lCategoryInfoList.empty())
		{
			PrintMessageList(pCategoryInfo->lCategoryInfoList, nTabCount + 1);
		}
	}
}

/*!
 * \brief Utility function that removes any messages items that have been marked
 *  	  as dirty (they no longer exist on the messages server).
 *  
 * \param lCategoryList is a list containing SstiCategoryInfo objects.
 */
void CstiMessageManager::RemoveDirtyMessages(
	std::list<SstiCategoryInfo *> *plCategoryList)
{
	// Loop through each category and find each dirty message.
	std::list<SstiCategoryInfo *>::iterator itCategory;
	for (itCategory = plCategoryList->begin();
		 itCategory != plCategoryList->end();
		 itCategory++)
	{
		// If a category has sub categories recurse into the sub categories list.
		if (!(*itCategory)->lCategoryInfoList.empty())
		{
			RemoveDirtyMessages(&(*itCategory)->lCategoryInfoList);
		}
		else if (!(*itCategory)->lMessageInfoList.empty())
		{
			std::list<CstiMessageInfo *>::iterator itMessage;
			itMessage = (*itCategory)->lMessageInfoList.begin();
			while (itMessage != (*itCategory)->lMessageInfoList.end())
			{
				// If the message is dirty then delete it.
				if ((*itMessage)->DirtyGet() == estiTRUE)
				{
					delete (*itMessage);
					itMessage = (*itCategory)->lMessageInfoList.erase(itMessage);

					// Mark the Category as changed so that the UI is notified of the changes.
					(*itCategory)->bCategoryChanged = estiTRUE;

					// Check for a parent.  If we have a parent we need to update it's changed status.
					if ((*itCategory)->pParentId)
					{
						SstiCategoryInfo *pCategoryParent = nullptr;
						FindCategory(*(*itCategory)->pParentId, nullptr, &pCategoryParent, nullptr);
						
						if (pCategoryParent)
						{
							pCategoryParent->bCategoryChanged = estiTRUE;
						}
						else
						{
							stiASSERT (estiFALSE);
						}
					}
				}
				else
				{
					itMessage++;
				}
			}
		}
	}
}

/*!
 * \brief Utility function that removes any messages items that have reached the
 * 		  expiration date.
 *
 * \param pCategory is a pointer to a SstiCategoryInfo that will be checked for expired messages.
 */
void CstiMessageManager::RemoveExpiredMessages(
	std::list<SstiCategoryInfo *> *plCategoryList)
{
	// Get the signMail categoryId
	CstiItemId signMailId;
	signMailId.ItemIdSet(g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId);

	// Loop through each category and find each dirty message.
	std::list<SstiCategoryInfo *>::iterator itCategory;
	for (itCategory = plCategoryList->begin();
		 itCategory != plCategoryList->end();
		 itCategory++)
	{
		// Don't remove messages from signMail
		if (*((*itCategory)->pCategoryId) == signMailId)
		{
			continue;
		}
		// If a category has sub categories recurse into the sub categories list.
		if (!(*itCategory)->lCategoryInfoList.empty())
		{
			RemoveExpiredMessages(&(*itCategory)->lCategoryInfoList);
		}
		else if (!(*itCategory)->lMessageInfoList.empty())
		{
			time_t currentTime;
			time(&currentTime);

			std::list<CstiMessageInfo *>::iterator itMessage;
			itMessage = (*itCategory)->lMessageInfoList.begin();
			while (itMessage != (*itCategory)->lMessageInfoList.end())
			{
				time_t msgExpDate = (*itMessage)->ExpirationDateGet();

				// If the message is expired then delete it. (currentTime > msgExpDate)
				if (msgExpDate != 0 &&
					difftime(currentTime, msgExpDate) >= 0)
				{
					delete (*itMessage);
					itMessage = (*itCategory)->lMessageInfoList.erase(itMessage);

					// Mark the Category as changed so that the UI is notified of the changes.
					(*itCategory)->bCategoryChanged = estiTRUE;

					// Check for a parent.  If we have a parent we need to update it's changed status.
					if ((*itCategory)->pParentId)
					{
						SstiCategoryInfo *pCategoryParent = nullptr;
						FindCategory(*(*itCategory)->pParentId, nullptr, &pCategoryParent, nullptr);
						
						if (pCategoryParent)
						{
							pCategoryParent->bCategoryChanged = estiTRUE;
						}
						else
						{
							stiASSERT (estiFALSE);
						}
					}
				}
				else
				{
					itMessage++;
				}
			}
		}
	}
}

/*!
 * \brief Utility function that removes any categories that no longer contain any messages 
 * 		  or are not marked as always visible.
 *
 * \param lCategoryList is a list containing SstiCategoryInfo objects.
 */
void CstiMessageManager::RemoveEmptyCategories(
	std::list<SstiCategoryInfo *> *plCategoryList)
{
	// Loop through each category and see if it is empty.
	std::list<SstiCategoryInfo *>::iterator itCategory;
	itCategory = plCategoryList->begin(); 
	while (itCategory != plCategoryList->end())
	{
		// If a category has sub categories recurse into the sub categories list.
		if (!(*itCategory)->lCategoryInfoList.empty())
		{
			RemoveEmptyCategories(&(*itCategory)->lCategoryInfoList);
		}

		if ((*itCategory)->lCategoryInfoList.empty() && 
			(*itCategory)->lMessageInfoList.empty() &&
			(*itCategory)->bAlwaysVisible == estiFALSE)
		{
			// Save the time ID so the UI can be notifed that the category has been changed.
			auto pItemId = new CstiItemId(*(*itCategory)->pCategoryId);

			// Cleaup the category and remove it from the lists.
			CleanupCategory((*itCategory));
			itCategory = plCategoryList->erase(itCategory);

			// Notify the UI that the cateogry has been changed (removed).
			m_pfnVPCallback (estiMSG_MESSAGE_CATEGORY_CHANGED, 
							 (size_t)pItemId, 
							  m_CallbackParam, m_CallbackFromId);
			continue;
		}

		itCategory++;
	}
}

/*!
 * \brief Utility function to determine if the request id of the given message
 *        response was one that we were waiting for.  If so, the request
 *        id will be removed from the list.
 *
 * \param unRequestId, the ID which will be checked.
 */
EstiBool CstiMessageManager::RemoveRequestId (
	unsigned int unRequestId)
{
	EstiBool bHasMsgRequestId = estiFALSE;

	Lock ();
	for (auto it = m_PendingRequestIds.begin (); 
		 it != m_PendingRequestIds.end ();
		 ++it)
	{
		if (*it == unRequestId)
		{
			bHasMsgRequestId = estiTRUE;
			m_PendingRequestIds.erase (it);
			break;
		}
	}
	Unlock ();

	return bHasMsgRequestId;
}

/*!
 * \brief Causes the message manager to re-request the message list.
 */
void CstiMessageManager::Refresh()
{
	Lock ();

	if (m_bEnabled && m_bAuthenticated && m_bInitialized)
	{
		auto pMessageRequest = new CstiMessageRequest ();

		if (pMessageRequest)
		{
			pMessageRequest->MessageListGet(0, MAX_VIDEO_MESSAGES, CstiMessageList::SortDirection::DESCENDING);
			MessageRequestSend(pMessageRequest, nullptr);
		}

		// Mark all root categories as changed so that all categories are sent to the UI.
		std::list<SstiCategoryInfo *>::iterator itCategory;
		for (itCategory = m_lCategoryListItems.begin(); 
			 itCategory != m_lCategoryListItems.end(); 
			 itCategory++)
		{
			(*itCategory)->bCategoryChanged = estiTRUE;
		}
	}

	Unlock ();
}

/*!
 * \brief Returns the number of SignMail messages that a category contains. 
 *  
 * \param punSignMailMessageCount is a pointer to an integer used to return the
 *              number of messages the category specified by pCategoryId.
 *  
 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
 */
stiHResult CstiMessageManager::SignMailCountGet (
	unsigned int *punSignMailMessageCount) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
	CstiItemId itemId;

	Lock();
	itemId.ItemIdSet(g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId);
	FindCategory(itemId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_INVALID_PARAMETER);

	*punSignMailMessageCount = pCategory->unSignMailCount;

STI_BAIL:
	Unlock();
	
	return hResult;
}

/*!
 * \brief Returns the max number of SignMail messages that a category can contains. 
 *  
 * \param punSignMailMaxMessageCount is a pointer to an integer used to return the
 *              number of messages the category specified by pCategoryId.
 *  
 * \return stiRESULT_SUCCESS if successful stiRESULT_ERROR otherwise. 
 */
stiHResult CstiMessageManager::SignMailMaxCountGet (
	unsigned int *punSignMailMaxMessageCount) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
	CstiItemId itemId;

	Lock();
	itemId.ItemIdSet(g_aCategoryList[SIGNMAIL_CATEGORY_INFO_INDEX].unCategoryId);
	FindCategory(itemId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_INVALID_PARAMETER);

	*punSignMailMaxMessageCount = pCategory->unMaxSignMailCount;

STI_BAIL:
	Unlock();
	
	return hResult;
}

EstiBool CstiMessageManager::StateNotifyEventHandle(
	CstiStateNotifyResponse *pStateNotifyResponse)
{
	EstiBool bHandled = estiFALSE;

	switch (pStateNotifyResponse->ResponseGet ())
	{
	case CstiStateNotifyResponse::eNEW_MESSAGE_RECEIVED:
	case CstiStateNotifyResponse::eMESSAGE_LIST_CHANGED:
		{
			// Request message list.
			auto  pMessageRequest = new CstiMessageRequest();
			pMessageRequest->MessageListGet(0, MAX_VIDEO_MESSAGES, CstiMessageList::SortDirection::DESCENDING);
			MessageRequestSend(pMessageRequest, nullptr);

			bHandled = estiTRUE;
		}
		break;

	default:
		break;
	}


	return bHandled;
}

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
stiHResult CstiMessageManager::SubCategoryByIndexGet (
	const CstiItemId &categoryId,
	const unsigned int unIndex, 
	CstiItemId **ppSubCategoryId) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::list<SstiCategoryInfo *>::const_iterator it;
	SstiCategoryInfo *pCategory = nullptr;
	unsigned int unCount = 0;
  
	Lock();
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);
	
	FindCategory(categoryId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_INVALID_PARAMETER);

	stiTESTCOND (unIndex < pCategory->lCategoryInfoList.size(), stiRESULT_INVALID_PARAMETER);
	
	// Search for the category in the list.
	it = pCategory->lCategoryInfoList.begin();
	while (unCount != unIndex)
	{
		it++;
		unCount++;
	}
	pCategory = (*it);

	*ppSubCategoryId = new CstiItemId(*pCategory->pCategoryId);

STI_BAIL:
	Unlock();
	
	return hResult;
}

CstiMessageManager::SstiCategoryInfo *CstiMessageManager::SubCategoryByNameGet(
	const char *pszCategoryName,
	const char *pszTokenName)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	std::list<SstiCategoryInfo *>::const_iterator itSubCategory;
	SstiCategoryInfo *pCategory = nullptr;
	SstiCategoryInfo *pRootCategory = nullptr;

	Lock();
	stiTESTCOND (pszCategoryName, stiRESULT_INVALID_PARAMETER);

	pRootCategory = CategoryByTokenGet(pszTokenName);
	stiTESTCOND (pRootCategory, stiRESULT_INVALID_PARAMETER);

	for (itSubCategory = pRootCategory->lCategoryInfoList.begin();
		itSubCategory != pRootCategory->lCategoryInfoList.end();
		itSubCategory++)
	{
		// Look for a matching name.  If we find save the category as the
		// one we are looking for.
		if (!strcmp((*itSubCategory)->shortName.c_str(), pszCategoryName))
		{
			pCategory = *itSubCategory;
		}
	}

	// If we didn't find it in the list then we need to create it.
	if (!pCategory)
	{
		pCategory = UnknownCategoryCreate(pszCategoryName,
										  pRootCategory);
	}

STI_BAIL:
	Unlock();

	return pCategory;
}


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
stiHResult CstiMessageManager::SubCategoryCountGet (
	const CstiItemId &categoryId,
	unsigned int *punSubCategoryCount) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
  
	Lock();
	
	stiTESTCOND (categoryId.IsValid(), stiRESULT_INVALID_PARAMETER);
	
	// Find the matching Category.
	FindCategory(categoryId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_INVALID_PARAMETER);
	
	*punSubCategoryCount = pCategory->lCategoryInfoList.size();

STI_BAIL:
	Unlock();
	
	return hResult;
}

CstiMessageManager::SstiCategoryInfo *CstiMessageManager::UnknownCategoryCreate(
		const char *pszCategoryName,
		SstiCategoryInfo *pParentCategory)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	SstiCategoryInfo *pCategory = nullptr;
//    std::list<SstiCategoryInfo *>::iterator itCategory;

	pCategory = new SstiCategoryInfo;
	pCategory->pCategoryId = new CstiItemId(m_unNextId);

	// Increment the id count for the next use.
	m_unNextId++;

	pCategory->nCategoryType = estiMT_MESSAGE_FROM_SORENSON;
	
	pCategory->shortName = pszCategoryName;

	pCategory->longName = pszCategoryName;

	pCategory->token.clear();
	pCategory->pParentId = new CstiItemId(*pParentCategory->pCategoryId);

	pCategory->unUnviewedMessageCount = 0;
	pCategory->unSignMailCount = 0;
	pCategory->unMaxSignMailCount = 0;

	// Unknown categories will never be always visible.
	pCategory->bAlwaysVisible = estiFALSE;

	// Mark the channel as changed so that when we get the list the UI will
	// be notified that it exists.
	pCategory->bCategoryChanged = estiTRUE;

	// Add the category to the correct parent list.
	hResult = AddCategory(pCategory, 
						  pParentCategory);

	return pCategory;
}

std::string CstiMessageManager::typeNameGet ()
{
	return typeid(CstiMessageManager).name();
}

vpe::Serialization::MetaData& CstiMessageManager::propertiesGet ()
{
	return m_serializationData;
}

vpe::Serialization::ISerializable* CstiMessageManager::instanceCreate (const std::string& typeName)
{
	if (typeName == typeid(CstiMessageManager::SstiCategoryInfo).name ())
	{
		return new CstiMessageManager::SstiCategoryInfo ();
	}
	return nullptr;
}

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
stiHResult CstiMessageManager::UnviewedMessageCountGet (
	const CstiItemId &categoryId,
	unsigned int *punUnviewedMessageCount) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCategoryInfo *pCategory = nullptr;
	unsigned int unMsgCount = 0;

	Lock();

	stiTESTCOND (categoryId.IsValid() && punUnviewedMessageCount, stiRESULT_INVALID_PARAMETER);
	
	FindCategory(categoryId, nullptr, &pCategory, nullptr);
	stiTESTCOND (pCategory, stiRESULT_ERROR);

	if (!pCategory->lCategoryInfoList.empty())
	{
		std::list<SstiCategoryInfo *>::iterator itCategory;
		for (itCategory = pCategory->lCategoryInfoList.begin(); 
			 itCategory != pCategory->lCategoryInfoList.end(); 
			 itCategory++)
		{
			unMsgCount += (*itCategory)->unUnviewedMessageCount;
		}
	}
	else if (!pCategory->lMessageInfoList.empty())
	{
		unMsgCount = pCategory->unUnviewedMessageCount;
	}

	*punUnviewedMessageCount = unMsgCount;

STI_BAIL:
	Unlock();
	
	return hResult;
}

/*!\brief Signal callback that is raised when a ClientAuthenticateResult core response is received.
 *
 */
void CstiMessageManager::clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult)
{
	DBG_MSG ("CoreResponse eCLIENT_AUTHENTICATE_RESULT (id %d) = %s", clientAuthenticateResult->requestId, clientAuthenticateResult->responseSuccessful ? "OK" : "Err");
	AuthenticatedSet (clientAuthenticateResult->responseSuccessful ? estiTRUE : estiFALSE);
}

stiHResult CstiMessageManager::HEVCSignMailPlaybackSupportedSet (bool hevcSignMailPlaybackSupported)
{
	m_hevcSignMailPlaybackSupported = hevcSignMailPlaybackSupported;
	return stiRESULT_SUCCESS;
}

ISignal<time_t>& CstiMessageManager::lastSignMailUpdateSignalGet ()
{
	return lastSignMailUpdateSignal;
}

ISignal<time_t>& CstiMessageManager::lastMessageUpdateSignalGet ()
{
	return lastMessageUpdateSignal;
}

ISignal<int>& CstiMessageManager::newSignMailSignalGet ()
{
	return newSignMailSignal;
}

ISignal<int>& CstiMessageManager::newVideoMsgSignalGet ()
{
	return newVideoMsgSignal;
}

CstiMessageManager::SstiCategoryInfo::SstiCategoryInfo ()
{
	using namespace vpe::Serialization;
	m_serializationData.push_back (std::make_shared<CustomIntegerProperty<CstiItemId*>> ("id", pCategoryId,
		[](CstiItemId* itemId) {
			uint32_t id = 0;
			itemId->ItemIdGet (&id);
			return static_cast<int64_t>(id);
		},
		[this](int64_t value, CstiItemId* itemId) {
			if (pCategoryId == nullptr)
			{
				pCategoryId = new CstiItemId ();
			}
			pCategoryId->ItemIdSet (static_cast<uint32_t>(value));
		}
	));
	m_serializationData.push_back (std::make_shared<IntegerProperty<EstiMessageType>> ("type", nCategoryType));
	m_serializationData.push_back (std::make_shared<StringProperty> ("long_name", longName));
	m_serializationData.push_back (std::make_shared<StringProperty> ("short_name", shortName));
	m_serializationData.push_back (std::make_shared<StringProperty> ("token", token));
	m_serializationData.push_back (std::make_shared<CustomIntegerProperty<CstiItemId*>> ("parentId", pParentId,
		[](CstiItemId* itemId) {
			uint32_t id = 0;
			if (itemId != nullptr)
			{
				itemId->ItemIdGet (&id);
			}
			return static_cast<int64_t>(id);
		},
		[this](int64_t value, CstiItemId* itemId) {
			if (pParentId == nullptr)
			{
				pParentId = new CstiItemId ();
			}
			pParentId->ItemIdSet (static_cast<uint32_t>(value));
		}));
	m_serializationData.push_back (std::make_shared<ArrayProperty<std::list<CstiMessageInfo*>, CstiMessageInfo*>> ("messages", lMessageInfoList));
	m_serializationData.push_back (std::make_shared<ArrayProperty<std::list<CstiMessageManager::SstiCategoryInfo*>, CstiMessageManager::SstiCategoryInfo*>> ("categories", lCategoryInfoList));
	m_serializationData.push_back (std::make_shared<IntegerProperty<unsigned int>> ("unviewed_message_count", unUnviewedMessageCount));
	m_serializationData.push_back (std::make_shared<IntegerProperty<unsigned int>> ("signmail_count", unSignMailCount));
	m_serializationData.push_back (std::make_shared<IntegerProperty<unsigned int>> ("max_signmail_count", unMaxSignMailCount));
	m_serializationData.push_back (std::make_shared<IntegerProperty<EstiBool>> ("always_visible", bAlwaysVisible));
}

std::string CstiMessageManager::SstiCategoryInfo::typeNameGet ()
{
	return typeid(CstiMessageManager::SstiCategoryInfo).name();
}

vpe::Serialization::MetaData& CstiMessageManager::SstiCategoryInfo::propertiesGet ()
{
	return m_serializationData;
}

vpe::Serialization::ISerializable* CstiMessageManager::SstiCategoryInfo::instanceCreate (const std::string& typeName)
{
	if (typeName == typeid(CstiMessageInfo).name())
	{
		return new CstiMessageInfo ();
	}
	if (typeName == typeid(CstiMessageManager::SstiCategoryInfo).name())
	{
		return new CstiMessageManager::SstiCategoryInfo ();
	}
	return nullptr;
}
