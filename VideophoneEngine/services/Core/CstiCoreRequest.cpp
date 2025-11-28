///
/// \file CstiCoreRequest.cpp
/// \brief Definition of the class containing APIs for sending requests to core.
///
/// This class has the wrapper functions for creating core requests.  The public function names are
///  generally the same name as the core request itself (except the functions often start with a
///  lower-case letter).
///
/// \note This file previously contained sample XML for each of the core requests.
///  The "VP Services API" documentation on the Engineering website contains the
///  official XML API for all core requests and core responses.  That documentation
///  should take precedent over any XML samples currently or previously contained herein.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2004-2019 by Sorenson Communications, Inc. All rights reserved.
///


//
// Includes
//

#ifdef DEBUG
#include <iostream>
#endif

#include "ixml.h"
#include "CstiCoreRequest.h"
#include "stiCoreRequestDefines.h"
#include "stiTrace.h"

#ifdef stiFAVORITES
#include "SstiFavoriteList.h"
#endif

const char g_szCRQUserTypeSorenson[] = "Sorenson";
const char g_szCRQUserTypeUser[] = "User";

const int n32_BIT_INTEGER_LENGTH = 11;  // 32 bit integers are 10 digits + 1 for a NULL terminator.
const int nBOOL_NAME_LENGTH = 6;  // false or true?  false is 5 chars + 1 for a NULL terminator

std::string CstiCoreRequest::m_uniqueID;

int CstiCoreRequest::m_nAPIMajorVersion = 0;
int CstiCoreRequest::m_nAPIMinorVersion = 0;

/**
 * \brief Stores the unique identifer for use in certain requests.
 * \param pszUniqueID The unique identifier to store
 */
void CstiCoreRequest::SetUniqueID (
	const std::string &uniqueID)
{
	m_uniqueID = uniqueID;
}


/**
 * \brief Checks if authentication is required for this request
 * \return estiTRUE if auth. is required, else estiFALSE
 */
EstiBool CstiCoreRequest::authenticationRequired()
{
	return m_bAuthenticationRequired;
}

 bool CstiCoreRequest::onlineOnlyGet ()
 {
	 return m_onlineOnly;
 }

 void CstiCoreRequest::onlineOnlySet (bool value)
 {
	 m_onlineOnly = value;
 }


/**
 * \brief Sets the priority of the request unless the priority is already higher (lower number).
 *
 * \param nPriority The priority to be set
 */
void CstiCoreRequest::PrioritySet (
	int nPriority)
{
	if (nPriority < m_nPriority)
	{
		m_nPriority = nPriority;
	}
}


/**
 * \brief Gets the priority of the request.
 *
 */
int CstiCoreRequest::PriorityGet () const
{
	return m_nPriority;
}


/**
 * \brief Converts a CstiCallList::EType enum to its corresponding String name
 * \param eCallListType The call list type to stringify
 * \return The stringified value
 */
const char* stringifyCallListType(
	CstiCallList::EType eCallListType)
{
	const char *strType;

	switch (eCallListType)
	{
		case CstiCallList::eDIALED:
			strType = VAL_Dialed;
			break;

		case CstiCallList::eANSWERED:
			strType = VAL_Answered;
			break;

		case CstiCallList::eMISSED:
			strType = VAL_Missed;
			break;

		case CstiCallList::eCONTACT:
			strType = VAL_Contact;
			break;

		case CstiCallList::eBLOCKED:
			strType = VAL_Blocked;
			break;

		case CstiCallList::eTYPE_UNKNOWN:
		default:
			return (nullptr);

	} // end switch
	return strType;
}


/**
 * \brief Converts a CstiCallList::ESortField enum to its corresponding String name
 * \param eSortField The sort field to stringify
 * \return The stringified value
 */
const char* stringifySortField(
	CstiCallList::ESortField eSortField)
{
	// Find the sort field and sort direction
	const char * strSortField;
	switch (eSortField)
	{
		case CstiCallList::eNAME:
			strSortField = VAL_Name;
			break;

		case CstiCallList::eTIME:
			strSortField = VAL_Time;
			break;

		case CstiCallList::eDIAL_TYPE:
			strSortField = VAL_DialType;
			break;

		case CstiCallList::eDIAL_STRING:
			strSortField = VAL_DialString;
			break;

		default:
			return (nullptr);
	} // end switch
	return strSortField;
}


/**
 * \brief Converts a CstiCallList::ESortDir enum to its corresponding String name
 * \param eSortDir The sort direction to stringify
 * \return The stringified value
 */
const char* stringifySortDir(
	CstiList::SortDirection eSortDir)
{
	const char * strSortDir;
	switch (eSortDir)
	{
		case CstiList::SortDirection::DESCENDING:
			strSortDir = VAL_DESC;
			break;

		case CstiList::SortDirection::ASCENDING:
			strSortDir = VAL_ASC;
			break;

		default:
			return (nullptr);
	} // end switch
	return strSortDir;
}


/**
 * \brief Converts a CstiContactListItem::EPhoneNumberType enum to its corresponding String name
 * \param eType The phone number type to stringify
 * \return The stringified value
 */
const char* stringifyPhoneNumberType(
	CstiContactListItem::EPhoneNumberType eType)
{
	const char * strNumberType;
	switch (eType)
	{
		default:
			return (nullptr);

		case CstiContactListItem::ePHONE_HOME:
			strNumberType = VAL_Home;
			break;

		case CstiContactListItem::ePHONE_WORK:
			strNumberType = VAL_Work;
			break;

		case CstiContactListItem::ePHONE_CELL:
			strNumberType = VAL_Cell;
			break;
	} // end switch
	return strNumberType;
}


/**
 * \brief Specialized function to see if the specified CallList exists within m_pMainReq; if so, return that element, else return NULL
 * \param listType The call list type to check
 */
IXML_Element* CstiCoreRequest::obtainExistingCallList(
	const char *listType)
{
	IXML_NodeList *nl = ixmlElement_getElementsByTagName((IXML_Element*)m_pMainReq,
														  (char*)TAG_CallList);
	for (unsigned long i = 0; i < ixmlNodeList_length(nl); i++)
	{
		auto tmpElem = (IXML_Element*)ixmlNodeList_item(nl, i);
		IXML_Attr *tmpAttr = ixmlElement_getAttributeNode(tmpElem, (char*)ATT_Type);
		const char *nodeVal = ixmlNode_getNodeValue((IXML_Node*)tmpAttr);
		if (!strcmp(listType, nodeVal))
		{
			ixmlNodeList_free(nl);
			return tmpElem;    // This is the CallList of type listType
		}
	}

	ixmlNodeList_free(nl);
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
//
// Following are the public functions provided in the API to do CoreRequests
//
////////////////////////////////////////////////////////////////////////////////


/**
 * \brief Gets the EULA from core
 * \param pszPublicId The ID of the EULA
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::agreementGet (
	const char *pszPublicId)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_AgreementGet);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_AgreementPublicId,
		pszPublicId);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Gets the EULA accpetance status from core
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::agreementStatusGet ()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	PrioritySet (estiCRP_AGREEMENT_STATUS_GET);

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_AgreementStatusGet);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Sets the EULA acceptance in core
 * \param pszAgreementPublicId The ID of the EULA
 * \param eStatus The agreement status (accepted or rejected)
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::agreementStatusSet (
	const char *pszAgreementPublicId,
	EAgreementStatus eStatus)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_AgreementStatusSet);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_AgreementPublicId,
		pszAgreementPublicId);
	stiTESTRESULT ();

	{
		const char *pszStatus = VAL_Rejected;
		if (eAGREEMENT_ACCEPTED == eStatus)
		{
			pszStatus = VAL_Accepted;
		}

		hResult = AddSubElementToRequest(
			TAG_Status,
			pszStatus);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates request to get the size (count) of specified CallList
 * \param eCallListType The Call list type
 * \param bUnique Count only the unique list entries
 * \param ttAfterTime Count only the entries after this time
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::callListCountGet (
	CstiCallList::EType eCallListType,
	bool bUnique,
	time_t ttAfterTime)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(eCallListType);
	stiTESTCOND (nullptr != strType, stiRESULT_ERROR);

	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(CRQ_CallListCountGet))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, so add it
	{
		// create the CoreRequest to get the call list count
		hResult = AddRequest(CRQ_CallListCountGet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT);
	}
	if (nullptr == callListElement)  // This call list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();

		if (bUnique)
		{
			auto retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_Unique, (char*)VAL_True);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		}

		hResult = addTimeAttributeToElement (callListElement, ttAfterTime);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to get the size (count) of specified ContactList
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::contactListCountGet ()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(CstiCallList::eCONTACT);
	stiTESTCOND (nullptr != strType, stiRESULT_ERROR);

	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(CRQ_CallListCountGet))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, so add it
	{
		// create the CoreRequest to get the call list count
		hResult = AddRequest(CRQ_CallListCountGet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT);
	}

	if (nullptr == callListElement)  // This call list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to get the specified call list
 * \param eCallListType Call list type
 * \param pszUserType User type
 * \param baseIndex Base index to start get
 * \param count Number of items to get
 * \param eSortField 
 * \param eSortDir Sort order: Ascending or descending
 * \param nThreshold 
 * \param bUnique Get only unique items or get all items in list
 * \param bFlagAddressBook True if address book
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::callListGet (
	CstiCallList::EType eCallListType,
	const char* pszUserType,
	unsigned int baseIndex,
	unsigned int count,
	CstiCallList::ESortField eSortField,
	CstiList::SortDirection eSortDir,
	int nThreshold,
	bool bUnique,
	bool bFlagAddressBook)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(eCallListType);
	stiTESTCOND (strType != nullptr, stiRESULT_ERROR);

	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(CRQ_CallListGet))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, create it
	{
		// create the CoreRequest to get the call list count
		hResult = AddRequest(CRQ_CallListGet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_GET_RESULT);
	}

	if (nullptr == callListElement)  // This call list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(
			TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();

		// Find the sort field and sort direction
		const char * strSortField = stringifySortField(eSortField);

		const char * strSortDir = stringifySortDir(eSortDir);

		// convert the baseIndex and count to strings
		char strIndex[n32_BIT_INTEGER_LENGTH];
		sprintf(strIndex, "%u", baseIndex);
		char strCount[n32_BIT_INTEGER_LENGTH];
		sprintf(strCount, "%u", count);

		// Add remaining attributes: base, count, sortField, sortDir
		if (nullptr != pszUserType)
		{
			auto retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_UserType, (char *)pszUserType);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		} // end if

		auto retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_BaseIndex, strIndex);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_Count, strCount);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement,
										   (char*)ATT_SortField,
										   (char*)strSortField);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement,
										   (char*)ATT_SortDir,
										   (char*)strSortDir);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		if (-1 != nThreshold)
		{
			char szThreshold[n32_BIT_INTEGER_LENGTH];
			sprintf(szThreshold, "%d", nThreshold);
			retVal = ixmlElement_setAttribute(callListElement,
											   (char*)ATT_Threshold,
											   (char*)szThreshold);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		}

		if (bUnique)
		{
			retVal = ixmlElement_setAttribute(callListElement,
											   (char*)ATT_Unique,
											   (char*)VAL_True);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		}

		if (bFlagAddressBook)
		{
			retVal = ixmlElement_setAttribute(
				callListElement,
				(char*)ATT_FlagAddressBook,
				(char*)VAL_True);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		}
	}

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to get the specified call list
 * \param pszUserType User type
 * \param baseIndex Base index into the list
 * \param count Number of contacts to get
 * \param nThreshold Threshold
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::contactListGet (
	const char* pszUserType,
	unsigned int baseIndex,
	unsigned int count,
	int nThreshold)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(CstiCallList::eCONTACT);
	stiTESTCOND (strType != nullptr, stiRESULT_ERROR);

	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(CRQ_CallListGet))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, create it
	{
		// create the CoreRequest to get the call list count
		hResult = AddRequest(CRQ_CallListGet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_GET_RESULT);
	}

	if (nullptr == callListElement)  // This call list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(
			TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();

		// Find the sort field and sort direction
		const char * strSortField = stringifySortField(CstiCallList::eNAME);

		const char * strSortDir = stringifySortDir(CstiList::SortDirection::ASCENDING);

		// convert the baseIndex and count to strings
		char strIndex[n32_BIT_INTEGER_LENGTH];
		sprintf(strIndex, "%u", baseIndex);
		char strCount[n32_BIT_INTEGER_LENGTH];
		sprintf(strCount, "%u", count);

		// Add remaining attributes: base, count, sortField, sortDir
		if (nullptr != pszUserType)
		{
			auto retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_UserType, (char *)pszUserType);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		} // end if

		auto retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_BaseIndex, strIndex);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_Count, strCount);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement,
										   (char*)ATT_SortField,
										   (char*)strSortField);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement,
										   (char*)ATT_SortDir,
										   (char*)strSortDir);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		if (-1 != nThreshold)
		{
			char szThreshold[n32_BIT_INTEGER_LENGTH];
			sprintf(szThreshold, "%d", nThreshold);
			retVal = ixmlElement_setAttribute(callListElement,
											   (char*)ATT_Threshold,
											   (char*)szThreshold);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		}
	}

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates a request to Add the specified CallListItem to the list
 * \param newCallListItem The new item to add to the list
 * \param eCallListType Specifies the call list to which to add the item
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::callListItemAdd(
	const CstiCallListItem &newCallListItem,
	CstiCallList::EType eCallListType)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = constructCallListAddOrEditRequest(CRQ_CallListItemAdd,
												 newCallListItem,
												 eCallListType);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates a request to Add the specified ContactListItem to the list
 * \param newContactListItem The new item to add to the list
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::contactListItemAdd(const CstiContactListItem &newContactListItem)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = constructContactListAddOrRemoveRequest(CRQ_CallListItemAdd,
													newContactListItem);

	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to edit the specified CallListItem in the list
 * \param editCallListItem The CallListItem to edit
 * \param eCallListType Specifies the call list to which to add the item
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::callListItemEdit(
	const CstiCallListItem &editCallListItem,
	CstiCallList::EType eCallListType)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = constructCallListAddOrEditRequest(CRQ_CallListItemEdit,
												 editCallListItem,
												 eCallListType);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to edit the specified ContactListItem in the list
 * \param editContactListItem The ContactListItem to edit
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::contactListItemEdit(
	const CstiContactListItem &editContactListItem)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = constructContactListAddOrRemoveRequest(CRQ_CallListItemEdit,
													editContactListItem);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to get the specified CallListItem from the list
 * \param getCallListItem The CallListItem to get
 * \param eCallListType Specifies the call list from which to get the item
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::callListItemGet(
	const CstiCallListItem &getCallListItem,
	CstiCallList::EType eCallListType)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = constructCallListRemoveOrGetRequest(CRQ_CallListItemGet,
												 getCallListItem,
												 eCallListType);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to get the specified ContactListItem from the list
 * \param getContactListItem The ContactListItem to get
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::contactListItemGet(const CstiContactListItem &getContactListItem)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = constructContactListAddOrRemoveRequest(CRQ_CallListItemGet,
													getContactListItem);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Removes the specified CallListItem from the list
 * \param callListItem The item to remove from the list
 * \param eCallListType The call list from which to remove the item
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::callListItemRemove(
	const CstiCallListItem &callListItem,
	CstiCallList::EType eCallListType)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = constructCallListRemoveOrGetRequest(CRQ_CallListItemRemove,
												 callListItem,
												 eCallListType);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Removes the specified ContactListItem from the list
 * \param contactListItem The item to remove from the list
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::contactListItemRemove(const CstiContactListItem &contactListItem)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = constructContactListAddOrRemoveRequest(CRQ_CallListItemRemove,
													contactListItem);

	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Common code to build the call list add and edit core requests
 * \param requestType The request type
 * \param newCallListItem The pointer to the new call list item
 * \param eCallListType The call list type
 */
stiHResult CstiCoreRequest::constructCallListAddOrEditRequest(
	const char *requestType,
	const CstiCallListItem &newCallListItem,
	CstiCallList::EType eCallListType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(eCallListType);
	stiTESTCOND (strType != nullptr, stiRESULT_ERROR);

	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(requestType))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, create it
	{
		// create the CoreRequest to add the call list item
		hResult = AddRequest(requestType);
		stiTESTRESULT ();

		if (strcmp (requestType, CRQ_CallListItemEdit) == 0)
		{
			ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT);
		}
		else if (strcmp (requestType, CRQ_CallListItemAdd) == 0)
		{
			ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT);
		}
	}

	if (nullptr == callListElement)  // If this call list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(
			TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();
	}

	// Add the callListItem to the IXML callListElement
	hResult = addCallListItemToElement(newCallListItem, callListElement);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

/**
* \brief Common code to build the call list add and remove core requests
* \param requestType The request type
* \param newCallListItem The pointer to the new call list item
* \param eCallListType The call list type
*/
stiHResult CstiCoreRequest::constructCallListRemoveOrGetRequest(
	const char *requestType,
	const CstiCallListItem &newCallListItem,
	CstiCallList::EType eCallListType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(eCallListType);
	stiTESTCOND (strType != nullptr, stiRESULT_ERROR);

	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(requestType))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, create it
	{
		// create the CoreRequest to add the call list item
		hResult = AddRequest(requestType);
		stiTESTRESULT ();

		if (strcmp(requestType, CRQ_CallListItemRemove) == 0)
		{
			ExpectedResultAdd(CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT);
		}
		else if (strcmp(requestType, CRQ_CallListItemGet) == 0)
		{
			ExpectedResultAdd(CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT);
		}
	}

	if (nullptr == callListElement)  // If this call list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(
			TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();
	}

	// Add the callListItem to the IXML callListElement
	hResult = addCallListItemIdToElement(newCallListItem, callListElement);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


/**
 * \brief Common code to build the contact list add and remove core requests
 * \param requestType The request type
 * \param newContactListItem The new contact list item
 */
stiHResult CstiCoreRequest::constructContactListAddOrRemoveRequest(
	const char *requestType,
	const CstiContactListItem &newContactListItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(CstiCallList::eCONTACT);
	stiTESTCOND (strType != nullptr, stiRESULT_ERROR);

	//
	// Work-around for Core issue: create a separate request for each
	//  contact list item in the list.  Skip existing tag check below.
	//
#if 0
	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(requestType))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, create it
#endif
	{
		// create the CoreRequest to add the contact list item
		hResult = AddRequest(requestType);
		stiTESTRESULT ();

		if (strcmp (requestType, CRQ_CallListItemRemove) == 0)
		{
			ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT);
		}
		else if (strcmp (requestType, CRQ_CallListItemGet) == 0)
		{
			ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_ITEM_GET_RESULT);
		}
		else if (strcmp (requestType, CRQ_CallListItemEdit) == 0)
		{
			ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT);
		}
		else if (strcmp (requestType, CRQ_CallListItemAdd) == 0)
		{
			ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT);
		}
	}

	if (nullptr == callListElement)  // If this contact list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(
			TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();
	}

	// Add the callListItem to the IXML callListElement
	hResult = addContactListItemToElement (newContactListItem, callListElement);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


/**
 * \brief Creates an IXML_Element representing this CallListItem object AND ADDS IT TO THE PASSED IN callListElement!
 * \param callListItem The call list item to add
 * \param callListElement The pointer to the call list element
*/
stiHResult CstiCoreRequest::addCallListItemToElement(
	const CstiCallListItem &callListItem,
	IXML_Element *callListElement)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListItemElement;
	const char *szValue = nullptr;

	hResult = AddSubElementToThisElement(
		callListElement,
		TAG_CallListItem,
		nullptr,
		&callListItemElement);
	stiTESTRESULT ();

	szValue = callListItem.ItemIdGet ();

	if (szValue != nullptr)
	{
		hResult = AddSubElementToThisElement(
			callListItemElement,
			(const char*)TAG_ItemId,
			szValue);
		stiTESTRESULT ();
	} // end if

	char szDialType[n32_BIT_INTEGER_LENGTH];

	sprintf (szDialType, "%d", (int)callListItem.DialMethodGet ());

	hResult = AddSubElementToThisElement(
		callListItemElement,
		(const char*)TAG_DialType,
		(const char*)szDialType);
	stiTESTRESULT ();

	szValue = callListItem.DialStringGet();

	if (szValue && *szValue)
	{
		hResult = AddSubElementToThisElement(
			callListItemElement,
			(const char*)TAG_DialString,
			szValue);
		stiTESTRESULT ();
	}

	szValue = callListItem.NameGet();

	hResult = AddSubElementToThisElement(
		callListItemElement,
		(const char*)TAG_Name,
		szValue);
	stiTESTRESULT ();

#ifndef stiFORCE_CORE_TIMESTAMP
	hResult = addTimeToSubElement (callListItemElement, callListItem.CallTimeGet());
	stiTESTRESULT ();
#endif

	if (callListItem.RingToneGet() != 0)
	{
		char szRingTone[n32_BIT_INTEGER_LENGTH];
		sprintf(szRingTone, "%d", (int)callListItem.RingToneGet());
		hResult = AddSubElementToThisElement(
			callListItemElement,
			(const char*)TAG_RingTone,
			szRingTone);
		stiTESTRESULT ();
	} // end if

    // Add the CallPublicId if it exists
	szValue = callListItem.CallPublicIdGet();
	if (szValue && *szValue)
	{
		hResult = AddSubElementToThisElement(
			callListItemElement,
			(const char*)TAG_CallPublicId,
			szValue);
		stiTESTRESULT ();
	}

	// Add the IntendedDialString if it exists
	szValue = callListItem.IntendedDialStringGet();
	if (szValue && *szValue)
	{
		hResult = AddSubElementToThisElement(
			callListItemElement,
			(const char*)TAG_IntendedDialString,
			szValue);
		stiTESTRESULT ();
	}

	// Add the BlockedCallerID if it exists
	{
		EstiBool bBlockCallerID = callListItem.BlockedCallerIDGet();
		if (bBlockCallerID)
		{
			hResult = AddSubElementToThisElement(
				callListItemElement,
				(const char*)TAG_BlockCallerId,
				bBlockCallerID?"True":"False");
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return hResult;
}

/**
* \brief Creates an IXML_Element representing this CallListItem object AND ADDS IT TO THE PASSED IN callListElement!
* \param callListItem The call list item to add
* \param callListElement The pointer to the call list element
*/
stiHResult CstiCoreRequest::addCallListItemIdToElement(
	const CstiCallListItem &callListItem,
	IXML_Element *callListElement)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListItemElement = nullptr;

	hResult = AddSubElementToThisElement(
		callListElement,
		TAG_CallListItem,
		nullptr,
		&callListItemElement);
	stiTESTRESULT ();

	if (callListItem.ItemIdGet() != nullptr)
	{
		hResult = AddSubElementToThisElement(
			callListItemElement,
			(const char*)TAG_ItemId,
			callListItem.ItemIdGet());
		stiTESTRESULT ();
	} // end if

STI_BAIL:

	return hResult;
}


/**
 * \brief Creates an IXML_Element representing this ContactListItem object AND ADDS IT TO THE PASSED IN contactListElement!
 * \param contactListItem The contact list item
 * \param contactListElement The contact list element
*/
stiHResult CstiCoreRequest::addContactListItemToElement(
	const CstiContactListItem &contactListItem,
	IXML_Element *contactListElement)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *contactListItemElement;
	CstiItemId Id;
	CstiItemId PublicId;
	const char *szValue = nullptr;

	hResult = AddSubElementToThisElement(
		contactListElement,
		TAG_CallListItem,
		nullptr,
		&contactListItemElement);
	stiTESTRESULT ();

	contactListItem.ItemIdGet (&Id);

	if (Id.IsValid())
	{
		std::string IdString;
		Id.ItemIdGet(&IdString);
		hResult = AddSubElementToThisElement(
			contactListItemElement,
			(const char*)TAG_ItemId,
			IdString.c_str());
		stiTESTRESULT ();
	} // end if

	contactListItem.ItemPublicIdGet(&PublicId);
	
	if (PublicId.IsValid())
	{
		std::string PublicIdString;
		PublicIdString = contactListItem.ItemPublicIdGet();
		hResult = AddSubElementToThisElement(
			contactListItemElement,
			(const char*)TAG_PublicId,
			PublicIdString.c_str());
		stiTESTRESULT ();
	} // end if

	hResult = AddSubElementToThisElement(
		contactListItemElement,
		(const char*)TAG_Name,
		contactListItem.NameGet());
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		contactListItemElement,
		(const char*)VAL_Company,
		contactListItem.CompanyNameGet());
	stiTESTRESULT ();

#ifdef stiCONTACT_PHOTOS
	szValue = contactListItem.ImageIdGet ();

	if (szValue != nullptr && 0 < strlen(szValue))
	{
		hResult = AddSubElementToThisElement(
			contactListItemElement,
			(const char*)TAG_ImageId,
			szValue);
		stiTESTRESULT ();
	} // end if
#endif

	if (contactListItem.RingToneGet() != 0)
	{
		char szRingTone[n32_BIT_INTEGER_LENGTH];
		sprintf(szRingTone, "%d", (int)contactListItem.RingToneGet());
		hResult = AddSubElementToThisElement(
			contactListItemElement,
			(const char*)TAG_RingTone,
			szRingTone);
		stiTESTRESULT ();
	} // end if

    // NOTE: always send ring color, so we know when switching back to default color
//    if (contactListItem.RingColorGet() != -1)
    if (m_nAPIMajorVersion >= 8)
    {
        char szRingColor[n32_BIT_INTEGER_LENGTH];
        sprintf(szRingColor, "%d", (int)contactListItem.RingColorGet());
        hResult = AddSubElementToThisElement(
            contactListItemElement,
            (const char*)TAG_RingColor,
            szRingColor);
		stiTESTRESULT ();
    } // end if

    szValue = contactListItem.RelayLanguageGet();
	if (szValue != nullptr && *szValue != 0)
	{
		hResult = AddSubElementToThisElement(
			contactListItemElement,
			(const char*)TAG_RelayLanguage,
			szValue);
		stiTESTRESULT ();
	} // end if

	// Only add the ContactPhoneNumberList tag if there are phone numbers
	if (0 < contactListItem.PhoneNumberCountGet())
	{
		IXML_Element *phoneNumberListElement;
		hResult = AddSubElementToThisElement(
			contactListItemElement,
			TAG_ContactPhoneNumberList,
			nullptr,
			&phoneNumberListElement);
		stiTESTRESULT ();

		for (int i = 0; i < contactListItem.PhoneNumberCountGet(); i++)
		{
			IXML_Element *phoneNumberListItemElement;
			hResult = AddSubElementToThisElement(
				phoneNumberListElement,
				TAG_ContactPhoneNumber,
				nullptr,
				&phoneNumberListItemElement);
			stiTESTRESULT ();

			char szId[n32_BIT_INTEGER_LENGTH];
			sprintf(szId, "%d", i);
			hResult = AddSubElementToThisElement(
				phoneNumberListItemElement,
				SNRQ_Id,
				szId);
			stiTESTRESULT ();

			contactListItem.PhoneNumberPublicIdGet(i, &Id);
			std::string IdString;
			Id.ItemIdGet(&IdString);
			hResult = AddSubElementToThisElement(
				phoneNumberListItemElement,
				TAG_PublicId,
				IdString.c_str());
			stiTESTRESULT ();

			CstiContactListItem::EPhoneNumberType eType = contactListItem.PhoneNumberTypeGet(i);
			szValue = stringifyPhoneNumberType(eType);
			hResult = AddSubElementToThisElement(
				phoneNumberListItemElement,
				TAG_PhoneNumberType,
				szValue);
			stiTESTRESULT ();

			char szDialType[n32_BIT_INTEGER_LENGTH];

			if (estiDM_UNKNOWN == contactListItem.DialMethodGet(i))
			{
				// We need to send "something" here, but we're not sure if
				// it's safe to actually send "Unknown" yet.  That might cause
				// a problem for users who switch to ntouch and then switch
				// back to VP-200.  So we'll use "by dial string" instead for now.
				sprintf(szDialType, "%d", estiDM_BY_DIAL_STRING);
			}
			else
			{
				sprintf(szDialType, "%d", (int)contactListItem.DialMethodGet(i));
			}

			hResult = AddSubElementToThisElement(
				phoneNumberListItemElement,
				(const char*)TAG_DialType,
				(const char*)szDialType);
			stiTESTRESULT ();

			hResult = AddSubElementToThisElement(
				phoneNumberListItemElement,
				(const char*)TAG_DialString,
				contactListItem.DialStringGet(i));
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return hResult;
}


/**
* \brief Creates request to set the specified call list
 * \param callList The call list to set
 * \return An error code (non-zero integer) or SUCCESS (0)
*/
stiHResult CstiCoreRequest::callListSet(
	const CstiCallList *callList)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(callList->TypeGet());
	stiTESTCOND (strType != nullptr, stiRESULT_ERROR);

	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(CRQ_CallListSet))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, create it
	{
		// create the CoreRequest to add the call list item
		hResult = AddRequest(CRQ_CallListSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_SET_RESULT);
	}

	if (nullptr == callListElement)  // If this call list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(
			TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();
	}

	{
		// Add remaining attributes: base, count, sortField, sortDir
		//
		unsigned int baseIndex = callList->BaseIndexGet();
		unsigned int count = callList->CountGet();
		CstiCallList::ESortField eSortField = callList->SortFieldGet();
		CstiList::SortDirection eSortDir = callList->SortDirectionGet();
		bool bUnique = (callList->UniqueGet() == estiTRUE);
		int nThreshold = callList->ThresholdGet();

		// convert the baseIndex, count, sortField and sortDir to strings
		char strIndex[n32_BIT_INTEGER_LENGTH];
		sprintf(strIndex, "%u", baseIndex);
		char strCount[n32_BIT_INTEGER_LENGTH];
		sprintf(strCount, "%u", count);
		const char * strSortField = stringifySortField(eSortField);
		const char * strSortDir = stringifySortDir(eSortDir);

		auto retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_BaseIndex, strIndex);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_Count, strCount);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement,
										   (char*)ATT_SortField,
										   (char*)strSortField);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement,
										   (char*)ATT_SortDir,
										   (char*)strSortDir);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		if (bUnique)
		{
			retVal = ixmlElement_setAttribute(callListElement,
											   (char*)ATT_Unique,
											   (char*)VAL_True);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		}

		if (-1 != nThreshold)
		{
			char szThreshold[n32_BIT_INTEGER_LENGTH];
			sprintf(szThreshold, "%d", nThreshold);
			retVal = ixmlElement_setAttribute(callListElement,
											   (char*)ATT_Threshold,
											   (char*)szThreshold);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		}

		// Finally, add each CallListItem in this CallList to the IXML representation
		for (unsigned int i = 0; i < count; i++)
		{
			auto cli = callList->ItemGet (i);
			hResult = addCallListItemToElement(*cli, callListElement);
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return hResult;
}


/**
* \brief Creates request to set the specified call list
 * \param contactList  The contact list
 * \return An error code (non-zero integer) or SUCCESS (0)
*/
stiHResult CstiCoreRequest::contactListSet(
	const CstiContactList *contactList)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *callListElement = nullptr;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// get a string version of the eCallListType
	auto strType = stringifyCallListType(CstiCallList::eCONTACT);
	stiTESTCOND (nullptr != strType, stiRESULT_ERROR);

	// check to see if this request already exists... if so, use existing tag
	if (ObtainExistingRequest(CRQ_CallListSet))
	{
		// check if requested CallList already exists in this request
		callListElement = obtainExistingCallList(strType);
	}
	else  // request doesn't yet exist, create it
	{
		// create the CoreRequest to add the call list item
		hResult = AddRequest(CRQ_CallListSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eCALL_LIST_SET_RESULT);
	}

	if (nullptr == callListElement)  // If this call list doesn't exist, create it
	{
		hResult = AddSubElementToRequest(
			TAG_CallList,
			nullptr,
			&callListElement,
			ATT_Type,
			strType);
		stiTESTRESULT ();
	}

	{
		// Add remaining attributes: base, count, sortField, sortDir
		//
		unsigned int baseIndex = contactList->BaseIndexGet();
		unsigned int count = contactList->CountGet();
		int nThreshold = contactList->ThresholdGet();

		// convert the baseIndex, count, sortField and sortDir to strings
		char strIndex[n32_BIT_INTEGER_LENGTH];
		sprintf(strIndex, "%u", baseIndex);
		char strCount[n32_BIT_INTEGER_LENGTH];
		sprintf(strCount, "%u", count);

		auto retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_BaseIndex, strIndex);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		retVal = ixmlElement_setAttribute(callListElement, (char*)ATT_Count, strCount);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);

		if (-1 != nThreshold)
		{
			char szThreshold[n32_BIT_INTEGER_LENGTH];
			sprintf(szThreshold, "%d", nThreshold);
			retVal = ixmlElement_setAttribute(callListElement,
											   (char*)ATT_Threshold,
											   (char*)szThreshold);
			stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
		}

		// Finally, add each CallListItem in this CallList to the IXML representation
		for (unsigned int i = 0; i < count; i++)
		{
			auto cli = contactList->ItemGet(i);
			hResult = addContactListItemToElement(*cli, callListElement);
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return hResult;
}


/**
 * \brief Authenticates a user attempting to log in
 * \param szName The user's name
 * \param szPhoneNumber The user's phone number
 * \param szPIN The user's pin
 * \return Error code if fails to authenticate, otherwise SUCCESS (0)
*/
int CstiCoreRequest::clientAuthenticate(
	const char *szName,
	const char *szPhoneNumber,
	const char *szPIN,
	const char *szUniqueID,
	const char *pszLoginAs)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *tmpElement = nullptr;

	hResult = AddRequest(CRQ_ClientAuthenticate);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT);

	PrioritySet (estiCRP_AUTHENTICATE);

	hResult = AddSubElementToRequest(
		TAG_PhoneIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	if (szUniqueID)
	{
		hResult = AddSubElementToThisElement(tmpElement, TAG_UniqueId, szUniqueID);
		stiTESTRESULT ();
	}
	else
	{
		hResult = AddSubElementToThisElement(tmpElement, TAG_UniqueId, m_uniqueID.c_str ());
		stiTESTRESULT ();
	}

	hResult = AddSubElementToThisElement(tmpElement, TAG_RingGroupCapable, VAL_True);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_UserIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(tmpElement, TAG_Name, szName);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(tmpElement, TAG_PhoneNumber, szPhoneNumber);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(tmpElement, TAG_Pin, szPIN);
	stiTESTRESULT ();

	if (pszLoginAs && *pszLoginAs)
	{
		hResult = AddSubElementToThisElement(tmpElement, TAG_LogInAs, pszLoginAs);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Authenticates a user attempting to log in
 * \param szPhoneNumber The user's phone number
 * \param szPIN The user's pin
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::userLoginCheck(
										const char *szPhoneNumber,
										const char *szPIN,
										const char *szUniqueID)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *tmpElement = nullptr;

	hResult = AddRequest(CRQ_UserLoginCheck);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT);
	
	PrioritySet (estiCRP_USER_LOGIN_CHECK);
	
	hResult = AddSubElementToRequest(
									 TAG_PhoneIdentity,
									 nullptr,
									 &tmpElement);
	stiTESTRESULT ();

	if (szUniqueID)
	{
		hResult = AddSubElementToThisElement(tmpElement, TAG_UniqueId, szUniqueID);
		stiTESTRESULT ();
	}
	else
	{
		hResult = AddSubElementToThisElement(tmpElement, TAG_UniqueId, m_uniqueID.c_str ());
		stiTESTRESULT ();
	}
	
	hResult = AddSubElementToRequest(
									 TAG_UserIdentity,
									 nullptr,
									 &tmpElement);
	stiTESTRESULT ();
	
	hResult = AddSubElementToThisElement(tmpElement, TAG_PhoneNumber, szPhoneNumber);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(tmpElement, TAG_Pin, szPIN);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Delete a profile or contact photo from core
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::imageDelete()
{
	auto hResult = AddRequest(CRQ_ImageDelete);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eIMAGE_DELETE_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Requests the image download URL from the server
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::imageDownloadURLGet(const char *pszId, EIdType eIdType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = AddRequest(CRQ_ImageDownloadURLGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT);

	if (ePHONE_NUMBER == eIdType)
	{
		hResult = AddSubElementToRequest(
				TAG_PhoneNumber,
				pszId);
		stiTESTRESULT ();
	}
	else // (ePUBLIC_ID == eIdType)
	{
		hResult = AddSubElementToRequest(
				TAG_ImageId,
				pszId);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Requests the URL from which to download specified image from core
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::imageUploadURLGet()
{
	auto hResult = AddRequest(CRQ_ImageUploadURLGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eIMAGE_UPLOAD_URL_GET_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Requests the default user settings from the server
 * \note: Only called after setting up and authenticating a new user
 * \return Error code if fails, otherwise SUCCESS (0)
*/
int CstiCoreRequest::getUserDefaults()
{
	auto hResult = AddRequest(CRQ_GetUserDefaults);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eUSER_DEFAULTS_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Logs the user off of the videophone
 * \return Error code on failure, otherwise SUCCESS (0)
*/
int CstiCoreRequest::clientLogout()
{
	IXML_Element *tmpElement = nullptr;

	auto hResult = AddRequest(CRQ_ClientLogout);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eCLIENT_LOGOUT_RESULT);

	hResult = AddSubElementToRequest(
		TAG_PhoneIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(tmpElement, TAG_UniqueId, m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates a request to Resolve the dial string
 * \param pszDialString The string to resolve
 * \param eDialMethod The dial method to use
 * \param pszFromDialString The "from" dial string
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::directoryResolve(
	const char *pszDialString,
	EstiDialMethod eDialMethod,
	const char *pszFromDialString,
	EstiBool bBLockCallerId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *pNewReqElement = nullptr;

	hResult = AddRequest(CRQ_DirectoryResolve, &pNewReqElement);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eDIRECTORY_RESOLVE_RESULT);

	PrioritySet (estiCRP_DIRECTORY_RESOLVE);

	hResult = AddSubElementToRequest(
		TAG_DialString,
		pszDialString);
	stiTESTRESULT ();

	char szDialType[8];
	sprintf(szDialType, "%d", (int)eDialMethod);
	hResult = AddSubElementToRequest(
		TAG_DialType,
		szDialType);
	stiTESTRESULT ();

	if (pszFromDialString && *pszFromDialString)
	{
		// Pass hearing caller's phone number to core for the from dial string
		hResult = AddSubElementToRequest(
			TAG_FromDialString,
			pszFromDialString);
		stiTESTRESULT ();
	}

	if (bBLockCallerId)
	{
		hResult = AddSubElementToRequest(
			TAG_BlockCallerId,
			bBLockCallerId?"True":"False");
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates request to deprovision the address
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::emergencyAddressDeprovision()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to deprovision the submitted 911 address
	auto hResult = AddRequest(CRQ_EmergencyAddressDeprovision);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eEMERGENCY_ADDRESS_DEPROVISION_RESULT);

	hResult = AddSubElementToRequest(TAG_UniqueId, m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates request to get the submitted 911 address
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::emergencyAddressGet()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get the submitted 911 address
	auto hResult = AddRequest(CRQ_EmergencyAddressGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eEMERGENCY_ADDRESS_GET_RESULT);

	hResult = AddSubElementToRequest(TAG_UniqueId, m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates request to send the address for provisioning
 * \param pszAddress1 Address line one
 * \param pszAddress2 Address line two
 * \param pszCity City
 * \param pszState State
 * \param pszZip Zip
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::emergencyAddressProvision(
	const char *pszAddress1,
	const char *pszAddress2,
	const char *pszCity,
	const char *pszState,
	const char *pszZip)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get the submitted 911 address
	hResult = AddRequest(CRQ_EmergencyAddressProvision);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_RESULT);

	hResult = AddSubElementToRequest(TAG_UniqueId, m_uniqueID.c_str ());
	stiTESTRESULT ();

	if (pszAddress1)
	{
		hResult = AddSubElementToRequest(
			TAG_Street,
			pszAddress1);
		stiTESTRESULT ();
	}

	if (pszAddress2)
	{
		hResult = AddSubElementToRequest(
			TAG_ApartmentNumber,
			pszAddress2);
		stiTESTRESULT ();
	}

	if (pszCity)
	{
		hResult = AddSubElementToRequest(
			TAG_City,
			pszCity);
		stiTESTRESULT ();
	}

	if (pszState)
	{
		hResult = AddSubElementToRequest(
			TAG_State,
			pszState);
		stiTESTRESULT ();
	}

	if (pszZip)
	{
		hResult = AddSubElementToRequest(
			TAG_Zip,
			pszZip);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Request to get info associated with user Groups
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userInterfaceGroupGet()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get the user interface group
	auto hResult = AddRequest(CRQ_UserInterfaceGroupGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eUSER_INTERFACE_GROUP_GET_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to get the address provision status
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::emergencyAddressProvisionStatusGet()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get the submitted 911 address STATUS
	auto hResult = AddRequest(CRQ_EmergencyAddressProvisionStatusGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT);

	hResult = AddSubElementToRequest(TAG_UniqueId, m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates request to accept the address as updated by tech support
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::emergencyAddressUpdateAccept()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to accept the 911 address as updated by tech support
	auto hResult = AddRequest(CRQ_EmergencyAddressUpdateAccept);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_ACCEPT_RESULT);

	hResult = AddSubElementToRequest(TAG_UniqueId, m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates request to reject the address as updated by tech support
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::emergencyAddressUpdateReject()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to reject the 911 address as updated by tech support
	auto hResult = AddRequest(CRQ_EmergencyAddressUpdateReject);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eEMERGENCY_ADDRESS_UPDATE_REJECT_RESULT);

	hResult = AddSubElementToRequest(TAG_UniqueId, m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates a request to add a missed call
 * \param calleeDialString The dial string of the person being called
 * \param ttcallTime The time of the missed call
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::missedCallAdd(
	const char *calleeDialString,
	time_t ttCallTime,
	EstiBool bBLockCallerId)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	stiHResult hResult = stiRESULT_SUCCESS;

	// check to see if this request already exists... if so, use existing request
	if (!ObtainExistingRequest(CRQ_MissedCallAdd))
	{
		// request didn't exist, so create it
		hResult = AddRequest(CRQ_MissedCallAdd);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eMISSED_CALL_ADD_RESULT);
	}

	IXML_Element *missedCallElem;
	hResult = AddSubElementToRequest(
		TAG_MissedCall,
		nullptr,
		&missedCallElem);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		missedCallElem,
		TAG_CalleeDialString,
		calleeDialString);
	stiTESTRESULT ();

#ifndef stiFORCE_CORE_TIMESTAMP
	hResult = addTimeToSubElement (missedCallElem, ttCallTime);
	stiTESTRESULT ();
#endif

    if (bBLockCallerId)
    {
        hResult = AddSubElementToThisElement(
            missedCallElem,
            TAG_BlockCallerId,
            bBLockCallerId?"True":"False");
		stiTESTRESULT ();
    }

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates a request to add a missed call
 * \param calleeDialString The dial string of the person being called
 * \param eDialMethod The dial type of the call
 * \param szDialString The dial string of the caller
 * \param szName The name of the caller
 * \param ttCallTime The time of the missed call
 * \param pszCallPublicId The SIP call ID (for ring group calls)
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::missedCallAdd(
	const char *calleeDialString,
	EstiDialMethod eDialMethod,
	const char *szDialString,
	const char *szName,
	time_t ttCallTime,
	const char *pszCallPublicId,
	EstiBool bBLockCallerId)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *missedCallElem = nullptr;

	// check to see if this request already exists... if so, use existing request
	if (!ObtainExistingRequest(CRQ_MissedCallAdd))
	{
		// request didn't exist, so create it
		hResult = AddRequest(CRQ_MissedCallAdd);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eMISSED_CALL_ADD_RESULT);
	}

	hResult = AddSubElementToRequest(
		TAG_MissedCall,
		nullptr,
		&missedCallElem);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		missedCallElem,
		TAG_CalleeDialString,
		calleeDialString);
	stiTESTRESULT ();

	char szDialType[8];
	sprintf(szDialType, "%d", (int)eDialMethod);

	hResult = AddSubElementToThisElement(
		missedCallElem,
		TAG_DialType,
		szDialType);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		missedCallElem,
		TAG_DialString,
		szDialString);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		missedCallElem,
		TAG_Name,
		szName);
	stiTESTRESULT ();

#ifndef stiFORCE_CORE_TIMESTAMP
	hResult = addTimeToSubElement (missedCallElem, ttCallTime);
	stiTESTRESULT ();
#endif

	if (*pszCallPublicId)
	{
		hResult = AddSubElementToThisElement(
			missedCallElem,
			TAG_CallPublicId,
			pszCallPublicId);
		stiTESTRESULT ();
	}

    if (bBLockCallerId)
    {
    	hResult = AddSubElementToThisElement(
            missedCallElem,
            TAG_BlockCallerId,
            bBLockCallerId?"True":"False");
    	stiTESTRESULT ();
    }

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates an account on the server for this phone
 * \param szType  The type of the videophone
 * \return Error code if fails to create, otherwise SUCCESS (0)
 */
int CstiCoreRequest::phoneAccountCreate(
	const std::string &phoneType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	IXML_Element *tmpElement = nullptr;
	hResult = AddRequest(CRQ_PhoneAccountCreate);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::ePHONE_ACCOUNT_CREATE_RESULT);

	PrioritySet (estiCRP_PHONE_ACCOUNT_CREATE);

	hResult = AddSubElementToRequest(
		TAG_PhoneIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_UniqueId,
		m_uniqueID.c_str ());
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_PhoneType,
		phoneType.c_str());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Registers the IP Address (or DialString) with the server
 * \param dialString The dial string (IP Address) of the phone
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::phoneOnline(
	const char *dialString)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	IXML_Element *tmpElement = nullptr;
	hResult = AddRequest(CRQ_PhoneOnline);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::ePHONE_ONLINE_RESULT);

	PrioritySet (estiCRP_PHONE_ONLINE);

	hResult = AddSubElementToRequest(
		TAG_PhoneIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_UniqueId,
		m_uniqueID.c_str ());
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_DialString,
		dialString);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to get a phone setting of specified name
 * \param phoneSettingName The phone setting to get
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::phoneSettingsGet(
	const char *phoneSettingName)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// check to see if this request already exists... if so, use existing request
	if (!ObtainExistingRequest(CRQ_PhoneSettingsGet))
	{
		// request didn't exist, so create it
		hResult = AddRequest(CRQ_PhoneSettingsGet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::ePHONE_SETTINGS_GET_RESULT);

		IXML_Element *tmpElement = nullptr;

		hResult = AddSubElementToRequest(
			TAG_PhoneIdentity,
			nullptr,
			&tmpElement);
		stiTESTRESULT ();

		hResult = AddSubElementToThisElement(
			tmpElement,
			TAG_UniqueId,
			m_uniqueID.c_str ());
		stiTESTRESULT ();
	}

	if (nullptr == phoneSettingName)
	{
		auto retVal = ixmlElement_setAttribute(
			(IXML_Element*)m_pMainReq,
			(char*)ATT_RequestType,
			(char*)VAL_All);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
	}
	else
	{
		m_PhoneSettingList.push_back (phoneSettingName);
			
		hResult = AddSubElementToRequest(
			TAG_PhoneSettingName,
			phoneSettingName);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to set the specified phone setting
 * \param phoneSettingName The phone setting to set
 * \param phoneSettingValue The value to set
 * \param eSettingType The phone setting type
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::phoneSettingsSet(
	const char *PhoneSettingName,
	const char *PhoneSettingValue,
	ESettingType ePhoneSettingType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_bAuthenticationRequired = estiTRUE;

	char szType[16];
	if (ePhoneSettingType == eSETTING_TYPE_INT)
	{
		strcpy(szType, "int");
	}
	else if (ePhoneSettingType == eSETTING_TYPE_BOOL)
	{
		strcpy(szType, "bool");
	}
	else // (ePhoneSettingType == eSETTING_TYPE_STRING)
	{
		strcpy(szType, "string");
	}
	IXML_Element *tmpElement = nullptr;

	// check to see if this request already exists... if so, use existing request
	if (!ObtainExistingRequest(CRQ_PhoneSettingsSet))
	{
		// request didn't exist, so create it
		hResult = AddRequest(CRQ_PhoneSettingsSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::ePHONE_SETTINGS_SET_RESULT);
	}

	hResult = AddSubElementToRequest(
		TAG_PhoneSettingItem,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_PhoneSettingName,
		PhoneSettingName);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_PhoneSettingValue,
		PhoneSettingValue);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_PhoneSettingType,
		szType);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to set the specified phone setting (type string)
 * \param phoneSettingName The phone setting to set
 * \param phoneSettingStringValue The value to set
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::phoneSettingsSet(
	const char *phoneSettingName,
	const char *phoneSettingStringValue)
{
	return phoneSettingsSet(phoneSettingName, phoneSettingStringValue, eSETTING_TYPE_STRING);
}


/**
 * \brief Creates request to set the specified phone setting (type int)
 * \param phoneSettingName The phone setting to set
 * \param phoneSettingIntValue The phone setting value
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::phoneSettingsSet(
	const char *phoneSettingName,
	int phoneSettingIntValue)
{
	char szIntValue[n32_BIT_INTEGER_LENGTH];
	sprintf(szIntValue, "%d", phoneSettingIntValue);
	return phoneSettingsSet(phoneSettingName, szIntValue, eSETTING_TYPE_INT);
}


/**
 * \brief Creates request to set the specified phone setting (type bool)
 * \param phoneSettingName The phone setting to set
 * \param phoneSettingBoolValue The boolean value to set
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::phoneSettingsSet(
	const char *phoneSettingName,
	EstiBool phoneSettingBoolValue)
{
	char szBoolValue[nBOOL_NAME_LENGTH];
	strcpy(szBoolValue, (phoneSettingBoolValue ? "true" : "false"));
	return phoneSettingsSet(phoneSettingName, szBoolValue, eSETTING_TYPE_BOOL);
}


/**
* \brief Determines if a primary user account is created for this VP
* \return Error code if fails, else SUCCESS (0)
*/
int CstiCoreRequest::primaryUserExists()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *tmpElement = nullptr;

	hResult = AddRequest("PrimaryUserExists");
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::ePRIMARY_USER_EXISTS_RESULT);

	hResult = AddSubElementToRequest(
		TAG_PhoneIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_UniqueId,
		m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Calls the server to obtain the public IP addr for this phone
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::publicIPGet()
{
	// create the CoreRequest to get Public IP
	auto hResult = AddRequest(CRQ_PublicIPGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::ePUBLIC_IP_GET_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Gets the SIP registration info
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::registrationInfoGet ()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_RegistrationInfoGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eREGISTRATION_INFO_GET_RESULT);

	PrioritySet (estiCRP_REGISTRATION_INFO_GET);

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Requests that a ring group be created with this account's phone number
 * \param pszAlias The descriptive name of the phone from which ring group is created
 * \param pszPin The ring group password
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupCreate (
	const char *pszAlias,
	const char *pszPin)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to Create the ring group
	hResult = AddRequest(CRQ_RingGroupCreate);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eRING_GROUP_CREATE_RESULT);

	hResult = AddSubElementToRequest(
		TAG_Alias,
		pszAlias);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_Pin,
		pszPin);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Updates the ring group password
 * \param pszPhoneNumber The ring group phone number
 * \param pszPin The ring group password
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupCredentialsUpdate (
	const char *pszPhoneNumber,
	const char *pszPin)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to update ring group credentials
	hResult = AddRequest(CRQ_RingGroupCredentialsUpdate);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eRING_GROUP_CREDENTIALS_UPDATE_RESULT);

	hResult = AddSubElementToRequest(
		TAG_PhoneNumber,
		pszPhoneNumber);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_Pin,
		pszPin);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Validates the ring group password
 * \param pszPhoneNumber The ring group phone number
 * \param pszPin The ring group password
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupCredentialsValidate (
	const char *pszPhoneNumber,
	const char *pszPin)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to validate ring group credentials
	hResult  = AddRequest(CRQ_RingGroupCredentialsValidate);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eRING_GROUP_CREDENTIALS_VALIDATE_RESULT);

	hResult = AddSubElementToRequest(
		TAG_PhoneNumber,
		pszPhoneNumber);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_Pin,
		pszPin);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Gets the descriptions of each ring group member
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupInfoGet ()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_RingGroupInfoGet);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Updates the description of any ring group member
 * \param pszPhoneNumber The private phone number of the endpoint to set
 * \param pszDescription The Description (location) of the endpoint to set
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupInfoSet (
	const char *pszPhoneNumber,
	const char *pszDescription)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *pRingGroupMemberList = nullptr;
	IXML_Element *pRingGroupMember = nullptr;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	hResult = AddRequest(CRQ_RingGroupInfoSet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eRING_GROUP_INFO_SET_RESULT);

	hResult = AddSubElementToRequest(
		TAG_RingGroupMemberList,
		nullptr,
		&pRingGroupMemberList);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		pRingGroupMemberList,
		TAG_RingGroupMember,
		nullptr,
		&pRingGroupMember);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		pRingGroupMember,
		TAG_PhoneNumber,
		pszPhoneNumber);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		pRingGroupMember,
		TAG_Description,
		pszDescription);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Accepts the invite to join a ring group
 * \param pszPhoneNumber The ring group phone number
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupInviteAccept (
	const char *pszPhoneNumber)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_RingGroupInviteAccept);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_GroupPhoneNumber,
		pszPhoneNumber);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Gets the information for the ring group that invited endpoint to join group
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupInviteInfoGet ()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_RingGroupInviteInfoGet);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Declines the invite to join a ring group
 * \param pszPhoneNumber The ring group phone number
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupInviteReject (
	const char *pszPhoneNumber)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_RingGroupInviteReject);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_GroupPhoneNumber,
		pszPhoneNumber);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Sets the ring group password
 * \param pszPin The ring group password
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupPasswordSet (
	const char *pszPin)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to set password
	auto hResult = AddRequest(CRQ_RingGroupPasswordSet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eRING_GROUP_PASSWORD_SET_RESULT);

	hResult = AddSubElementToRequest(
		TAG_Pin,
		pszPin);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Invites an endpoint to join a ring group
 * \param pszPhoneNumber The ring group phone number
 * \param pszDescription The descriptive name for the invited phone
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupUserInvite (
	const char *pszPhoneNumber,
	const char *pszDescription)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_RingGroupUserInvite);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_PhoneNumber,
		pszPhoneNumber);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_Description,
		pszDescription);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Removes an endpoint from a ring group
 * \param pszPhoneNumber The ring group phone number
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::ringGroupUserRemove (
	const char *pszPhoneNumber)
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_RingGroupUserRemove);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_PhoneNumber,
		pszPhoneNumber);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Requests the screensaver list from core
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::screenSaverListGet()
{
	// create the CoreRequest to get ring group info
	auto hResult = AddRequest(CRQ_ScreensaverListGet);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Indicates to service whether a fail-over has occurred, and how the service was contacted.
 * \param svcPriority the service priority
 * \param svcUrl the service url
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::serviceContact(
	const char *svcPriority,
	const char *svcUrl)
{
	auto hResult = AddRequest(CRQ_ServiceContact);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eSERVICE_CONTACT_RESULT);

	hResult = AddSubElementToRequest(
		TAG_ServicePriority,
		svcPriority);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_ServiceUrl,
		svcUrl);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Requests contact methods and failover priorities of all available services.
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::serviceContactsGet()
{
	IXML_Element *tmpElement = nullptr;

	auto hResult = AddRequest(CRQ_ServiceContactsGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eSERVICE_CONTACTS_GET_RESULT);

	PrioritySet (estiCRP_SERVICE_CONTACTS_GET);

	hResult = AddSubElementToRequest(
		TAG_PhoneIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_UniqueId,
		m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates request to Register the client with the sign mail service
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::signMailRegister()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = AddRequest(CRQ_SignMailRegister);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eSIGNMAIL_REGISTER_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Retrieves the current date/time from the server
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::timeGet()
{
	auto hResult = AddRequest(CRQ_TimeGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eTIME_GET_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Validates the credentials of the trainer/installer
 * \note: Must be administrator
 * \param pszName The trainer's user id
 * \param pszPassword The trainer's password
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::trainerValidate(
	const char *pszName,
	const char *pszPassword)
{
	auto hResult = AddRequest("TrainerValidate");
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eTRAINER_VALIDATE_RESULT);

	hResult = AddSubElementToRequest(
		"Username",
		pszName);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		"Password",
		pszPassword);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_UniqueId,
		m_uniqueID.c_str ());
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Checks if there is an application update on the server
 * \param appVersion The firmware version currently on the videophone
 * \param appLoaderVersion The apploader version currently on the videophone
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::updateVersionCheck(
	const char *appVersion,
	const char *appLoaderVersion)
{
	auto hResult = AddRequest(CRQ_UpdateVersionCheck);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eUPDATE_VERSION_CHECK_RESULT);

	PrioritySet (estiCRP_FIRMWARE_UPDATE_CHECK);

	hResult = AddSubElementToRequest(
		TAG_UniqueId,
		m_uniqueID.c_str ());
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_AppVersion,
		appVersion);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_AppLoaderVersion,
		appLoaderVersion);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Gets the current version of Core
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::versionGet ()
{
	m_bAuthenticationRequired = estiFALSE;
	
	auto hResult = AddRequest(CRQ_VersionGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eVERSION_GET_RESULT);

STI_BAIL:

	return requestResultGet (hResult);
}

/**
* \brief Sends a list of URIs to the server
* \param pszPublicPriority1Uri The first priority public URI in the list  (may be a NULL pointer)
* \param pszPrivatePriority1Uri The first priority private URI in the list (may be a NULL pointer)
* \param pszPublicPriority2Uri The second priority public URI in the list  (may be a NULL pointer)
* \param pszPrivatePriority2Uri The second priority private URI in the list (may be a NULL pointer)
* \return Error code if fails, otherwise SUCCESS (0)
*
* <CoreRequest Id="1" ClientToken="21EC2020-3AEA-1069-A2DD-08002B30309D">
* 	<URIListSet>
* 		<PhoneIdentity>
* 			<UniqueId>00:22:64:AF:2C:57</UniqueId>
* 		</PhoneIdentity>
* 		<URI Priority="1">sip:\1@10.20.213.12</URI>
* 		<URI Priority="2" Network="Sorenson">sip:\1@10.20.213.13</URI>
* 		<URI Priority="2" Network="iTRS">sip:\1@10.20.213.14</URI>
*	</URIListSet>
*</CoreRequest>
*
*/
int CstiCoreRequest::uriListSet (
	const char *pszPublicPriority1Uri,
	const char *pszPrivatePriority1Uri,
	const char *pszPublicPriority2Uri,
	const char *pszPrivatePriority2Uri,
	const char *pszPublicPriority3Uri,
	const char *pszPrivatePriority3Uri)
{
	IXML_Element *tmpElement = nullptr;
	int nPriority = 1;
	char szPriority[2];

	auto hResult = AddRequest(CRQ_URIListSet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eURI_LIST_SET_RESULT);

	PrioritySet (estiCRP_PHONE_ONLINE);

	hResult = AddSubElementToRequest(
		TAG_PhoneIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_UniqueId,
		m_uniqueID.c_str ());
	stiTESTRESULT ();

	if (pszPrivatePriority1Uri && *pszPrivatePriority1Uri)
	{
		sprintf (szPriority, "%d", nPriority);
	
		hResult = AddSubElementToRequest(
			TAG_URI,
			pszPrivatePriority1Uri,
			nullptr,
			ATT_Priority,
			szPriority,
			ATT_Network,
			VAL_Sorenson);
		stiTESTRESULT ();

		++nPriority;
	}
	
	if (pszPublicPriority1Uri && *pszPublicPriority1Uri)
	{
		sprintf (szPriority, "%d", nPriority);
		
		hResult = AddSubElementToRequest(
			TAG_URI,
			pszPublicPriority1Uri,
			nullptr,
			ATT_Priority,
			szPriority,
			ATT_Network,
			VAL_iTRS);
		stiTESTRESULT ();

		++nPriority;
	}

	if (pszPrivatePriority2Uri && *pszPrivatePriority2Uri)
	{
		sprintf (szPriority, "%d", nPriority);
		
		hResult = AddSubElementToRequest(
			TAG_URI,
			pszPrivatePriority2Uri,
			nullptr,
			ATT_Priority,
			szPriority,
			ATT_Network,
			VAL_Sorenson);
		stiTESTRESULT ();

		++nPriority;
	}
		
	if (pszPublicPriority2Uri && *pszPublicPriority2Uri)
	{
		sprintf (szPriority, "%d", nPriority);
		
		hResult = AddSubElementToRequest(
			TAG_URI,
			pszPublicPriority2Uri,
			nullptr,
			ATT_Priority,
			szPriority,
			ATT_Network,
			VAL_iTRS);
		stiTESTRESULT ();

		++nPriority;
	}

	if (pszPrivatePriority3Uri && *pszPrivatePriority3Uri)
	{
		sprintf (szPriority, "%d", nPriority);
		
		hResult = AddSubElementToRequest(
			TAG_URI,
			pszPrivatePriority3Uri,
			nullptr,
			ATT_Priority,
			szPriority,
			ATT_Network,
			VAL_Sorenson);
		stiTESTRESULT ();

		++nPriority;
	}
		
	if (pszPublicPriority3Uri && *pszPublicPriority3Uri)
	{
		sprintf (szPriority, "%d", nPriority);
		
		hResult = AddSubElementToRequest(
			TAG_URI,
			pszPublicPriority3Uri,
			nullptr,
			ATT_Priority,
			szPriority,
			ATT_Network,
			VAL_iTRS);
		stiTESTRESULT ();

		++nPriority;
	}

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief DEPRECATED: Associates a user account with this phone
 * \note: Must be administrator
 * \note: This request is obsolete as of Core 3.0; will now just simply return OK status
 * \param szName The user's name
 * \param szPhoneNumber The user's phone number
 * \param szPin The user's pin
 * \param ePlan The CallList plan
 * \param bIsPrimaryUser estiTRUE if this is the primary user on this phone
 * \param bMatchingPrimaryUserRequired estiTRUE if re-associating the previous account on the phone
 * \param bMultipleEndpointEnabled estiTRUE if multiple endpoint is enabled
 * \return Error code if fails, otherwise SUCCESS (0)
 */
int CstiCoreRequest::userAccountAssociate(
	const char *szName,
	const char *szPhoneNumber,
	const char *szPin,
	EstiBool bIsPrimaryUser,
	EstiBool bMatchingPrimaryUserRequired,
	EstiBool bMultipleEndpointEnabled)
{
	IXML_Element *pPhoneIdElement = nullptr;
	IXML_Element *pUserIdElement = nullptr;

	auto hResult = AddRequest(CRQ_UserAccountAssociate);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eUSER_ACCOUNT_ASSOCIATE_RESULT);

	hResult = AddSubElementToRequest(
		TAG_PhoneIdentity,
		nullptr,
		&pPhoneIdElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		pPhoneIdElement,
		TAG_UniqueId,
		m_uniqueID.c_str ());
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_UserIdentity,
		nullptr,
		&pUserIdElement);

	hResult = AddSubElementToThisElement(
		pUserIdElement,
		TAG_Name,
		szName);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		pUserIdElement,
		TAG_PhoneNumber,
		szPhoneNumber);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		pUserIdElement,
		TAG_Pin,
		szPin);
	stiTESTRESULT ();

	if (bIsPrimaryUser)
	{
		// add the IsPrimaryUser tag to the UserIdentity tag
		hResult = AddSubElementToThisElement(
			pUserIdElement,
			"IsPrimaryUser",
			VAL_True);
		stiTESTRESULT ();
	}

	// Attempting to reinstall the same user account to the phone...
	//  require existing account to match
	if (bMatchingPrimaryUserRequired)
	{
		hResult = AddSubElementToRequest(
			"PrimaryUserRequired",
			VAL_True);
		stiTESTRESULT ();
	}

	if (bMultipleEndpointEnabled)
	{
		hResult = AddSubElementToRequest(
			"MultipleEndpointEnabled",
			VAL_True);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Sends request to get info associated with logged in user
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userAccountInfoGet()
{
	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	auto hResult = AddRequest(CRQ_UserAccountInfoGet);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT);

	PrioritySet (estiCRP_USER_ACCOUNT_INFO_GET);

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Updates the user's pin in user account info
 * \param pin The user's pin
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userPinSet (const std::string& pin)
{
	stiHResult hResult{ stiRESULT_SUCCESS };

	m_bAuthenticationRequired = estiTRUE;

	if (!ObtainExistingRequest (CRQ_UserAccountInfoSet))
	{
		hResult = AddRequest (CRQ_UserAccountInfoSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT);
	}

	hResult = AddSubElementToRequest (TAG_Pin, pin.c_str ());
	stiTESTRESULT ();

STI_BAIL:
	return requestResultGet (hResult);
}

/**
 * \brief Updates the user's profile image ID in user account info
 * \param imageId The user's profile image ID
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::profileImageIdSet (const std::string& imageId)
{
	stiHResult hResult{ stiRESULT_SUCCESS };

	m_bAuthenticationRequired = estiTRUE;

	if (!ObtainExistingRequest (CRQ_UserAccountInfoSet))
	{
		hResult = AddRequest (CRQ_UserAccountInfoSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT);
	}

	hResult = AddSubElementToRequest (TAG_ImageId, imageId.c_str ());
	stiTESTRESULT ();

STI_BAIL:
	return requestResultGet (hResult);
}

/**
 * \brief Updates the user's email addresses and notification preference in user account info
 * \param enableSignMailNotifications Preference for receiving email notifications for SignMail
 * \param emailPrimary Primary email address AKA EmailMain
 * \param emailSecondary Secondary email address AKA EmailPager
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::signMailEmailPreferencesSet (bool enableSignMailNotifications, const std::string& emailPrimary, const std::string& emailSecondary)
{
	stiHResult hResult{ stiRESULT_SUCCESS };

	m_bAuthenticationRequired = estiTRUE;

	if (!ObtainExistingRequest (CRQ_UserAccountInfoSet))
	{
		hResult = AddRequest (CRQ_UserAccountInfoSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT);
	}

	// This code is here to resolve an issue with core not sending an email
	// to the "Secondary" m_EmailPager if there is no "Primary" m_EmailMain
	if (emailPrimary.empty () && !emailSecondary.empty ())
	{
		hResult = AddSubElementToRequest (TAG_EmailMain, emailSecondary.c_str ());
		stiTESTRESULT ();

		// Send a blank pager address (in case we are switching an already bad setting)
		hResult = AddSubElementToRequest (TAG_EmailPager, "");
		stiTESTRESULT ();
	}
	else
	{
		hResult = AddSubElementToRequest (TAG_EmailMain, emailPrimary.c_str ());
		stiTESTRESULT ();

		hResult = AddSubElementToRequest (TAG_EmailPager, emailSecondary.c_str ());
		stiTESTRESULT ();
	}

	hResult = AddSubElementToRequest (TAG_SignMailEnabled, enableSignMailNotifications ? "True" : "False");
	stiTESTRESULT ();

STI_BAIL:
	return requestResultGet (hResult);
}

/**
 * \brief Updates the user's FCC registration info in user account info
 * \param birthDate User's birth date in the format MM/dd/yyyy
 * \param lastFourSSN Last 4 digits of SSN or an empty string
 * \param hasIdentification True if the user has supplied identification (last 4 of SSN)
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userRegistrationInfoSet (const std::string& birthDate, const std::string& lastFourSSN, bool hasIdentification)
{
	stiHResult hResult{ stiRESULT_SUCCESS };

	m_bAuthenticationRequired = estiTRUE;

	IXML_Element* userRegistrationElement = nullptr;

	if (!ObtainExistingRequest (CRQ_UserAccountInfoSet))
	{
		hResult = AddRequest (CRQ_UserAccountInfoSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eUSER_ACCOUNT_INFO_SET_RESULT);
	}

	hResult = AddSubElementToRequest (TAG_UserRegistrationData, nullptr, &userRegistrationElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement (userRegistrationElement, TAG_HasIdentification, hasIdentification ? "True" : "False");
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement (userRegistrationElement, TAG_IdentificationData, lastFourSSN.c_str ());
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement (userRegistrationElement, TAG_DOB, birthDate.c_str ());
	stiTESTRESULT ();

STI_BAIL:
	return requestResultGet (hResult);
}

/**
 * \brief Creates request to get a user setting of specified name
 * \param userSettingName Name of setting to get
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userSettingsGet(
	const char *userSettingName)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// check to see if this request already exists... if so, use existing request
	if (!ObtainExistingRequest(CRQ_UserSettingsGet))
	{
		// request didn't exist, so create it
		hResult = AddRequest(CRQ_UserSettingsGet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eUSER_SETTINGS_GET_RESULT);
	}
	if (nullptr == userSettingName)
	{
		auto retVal = ixmlElement_setAttribute(
			(IXML_Element*)m_pMainReq,
			(char*)ATT_RequestType,
			(char*)VAL_All);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
	}
	else
	{
		m_UserSettingList.push_back (userSettingName);

		hResult = AddSubElementToRequest(
			TAG_UserSettingName,
			userSettingName);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Creates request to set the specified user setting
 * \param userSettingName Name of setting to set
 * \param userSettingValue Value of setting to set
 * \param eUserSettingType Type of setting
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userSettingsSet(
	const char *userSettingName,
	const char *userSettingValue,
	ESettingType eUserSettingType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	char szType[16];
	if (eUserSettingType == eSETTING_TYPE_INT)
	{
		strcpy(szType, "int");
	}
	else if (eUserSettingType == eSETTING_TYPE_BOOL)
	{
		strcpy(szType, "bool");
	}
	else // (eUserSettingType == eSETTING_TYPE_STRING)
	{
		strcpy(szType, "string");
	}

	IXML_Element *tmpElement = nullptr;
	// check to see if this request already exists... if so, use existing request
	if (!ObtainExistingRequest(CRQ_UserSettingsSet))
	{
		// request didn't exist, so create it
		hResult = AddRequest(CRQ_UserSettingsSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eUSER_SETTINGS_SET_RESULT);
	}

	hResult = AddSubElementToRequest(
		TAG_UserSettingItem,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_UserSettingName,
		userSettingName);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_UserSettingValue,
		userSettingValue);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(
		tmpElement,
		TAG_UserSettingType,
		szType);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}


/**
 * \brief Creates request to set the specified user setting (type string)
 * \param userSettingName Name of setting to set
 * \param userSettingValue Value of setting to set
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userSettingsSet(
	const char *userSettingName,
	const char *userSettingStringValue)
{
	return userSettingsSet(userSettingName, userSettingStringValue, eSETTING_TYPE_STRING);
}


/**
 * \brief Creates request to set the specified user setting (type int)
 * \param userSettingName Name of setting to set
 * \param userSettingIntValue Integer value to set
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userSettingsSet(
	const char *userSettingName,
	int userSettingIntValue)
{
	char szIntValue[n32_BIT_INTEGER_LENGTH];
	sprintf(szIntValue, "%d", userSettingIntValue);
	return userSettingsSet(userSettingName, szIntValue, eSETTING_TYPE_INT);
}


/**
 * \brief Creates request to set the specified user setting (type bool)
 * \param userSettingName Name of setting to set
 * \param userSettingBoolValue Boolean value to set
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::userSettingsSet(
	const char *userSettingName,
	EstiBool userSettingBoolValue)
{
	char szBoolValue[nBOOL_NAME_LENGTH];
	strcpy(szBoolValue, (userSettingBoolValue ? "true" : "false"));
	return userSettingsSet(userSettingName, szBoolValue, eSETTING_TYPE_BOOL);
}


const char *CstiCoreRequest::RequestTypeGet()
{
	return CRQ_COREREQUEST;
}


/**
 * \brief This allows both the endpoint initiating the transfer and 
 * the endpoint performing the transfer to log that a transfer occurred.
 * \param szTransferredFromDialString is required
 * \param szTransferredToDialString is required
 * \param szOriginalCallPublicId is optional
 * \param szTransferredCallPublicId is optional
 * \param bInitiatedTransfer false if performing transfer
 * \return Error code if fails, else SUCCESS (0)
 */
int CstiCoreRequest::logCallTransfer(
	const char *szTransferredFromDialString,
	const char *szTransferredToDialString,
	const char *szOriginalCallPublicId,
	const char *szTransferredCallPublicId,
	bool bInitiatedTransfer)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Element *tmpElement = nullptr;
	
	hResult = AddRequest(CRQ_LogCallTransfer);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eLOG_CALL_TRANSFER_RESULT);
	
	hResult = AddSubElementToRequest(
		TAG_TRANSFERRED_FROM_DIAL_STRING,
		szTransferredFromDialString,
		&tmpElement);
	stiTESTRESULT ();

	hResult = AddSubElementToRequest(
		TAG_TRANSFERRED_TO_DIAL_STRING,
		szTransferredToDialString,
		&tmpElement);
	stiTESTRESULT ();

	if (szOriginalCallPublicId)
	{
		hResult = AddSubElementToRequest(
			TAG_ORIGINAL_CALL_PUBLIC_ID,
			szOriginalCallPublicId,
			&tmpElement);
		stiTESTRESULT ();
	}

	if (szTransferredCallPublicId)
	{
		hResult = AddSubElementToRequest(
			TAG_TRANSFERRED_CALL_PUBLIC_ID,
			szTransferredCallPublicId,
			&tmpElement);
		stiTESTRESULT ();
	}

	if (bInitiatedTransfer)
	{
		hResult = AddSubElementToRequest(
			TAG_Action,
			VAL_InitiatedTransfer,
			&tmpElement);
		stiTESTRESULT ();
	}
	else
	{
		hResult = AddSubElementToRequest(
			TAG_Action,
			VAL_PerformingTransfer,
			&tmpElement);
		stiTESTRESULT ();
	}
	
STI_BAIL:

	return requestResultGet (hResult);
}

/**
 * \brief Imports contacts from another account
 * \param szPhoneNumber The user's phone number
 * \param szPIN The user's pin
 * \return An error code (non-zero integer) or SUCCESS (0)
 */
int CstiCoreRequest::contactsImport(
	const char *szPhoneNumber,
	const char *szPIN)
{
	IXML_Element *tmpElement = nullptr;

	auto hResult = AddRequest(CRQ_ContactsImport);
	stiTESTRESULT ();

	ExpectedResultAdd (CstiCoreResponse::eCONTACTS_IMPORT_RESULT);
	
	hResult = AddSubElementToRequest(
		TAG_UserIdentity,
		nullptr,
		&tmpElement);
	stiTESTRESULT ();
	
	hResult = AddSubElementToThisElement(tmpElement, TAG_PhoneNumber, szPhoneNumber);
	stiTESTRESULT ();

	hResult = AddSubElementToThisElement(tmpElement, TAG_Pin, szPIN);
	stiTESTRESULT ();
	
STI_BAIL:

	return requestResultGet (hResult);
}

stiHResult CstiCoreRequest::vrsAgentHistoryAdd (const CstiCallListItem &callHistoryItem, EstiDirection callDirection)
{
	auto hResult = stiRESULT_SUCCESS;
	const char *crq = "";
	IXML_Element *historyElement = nullptr;

	stiTESTCOND_NOLOG (!callHistoryItem.vrsCallIdGet ().empty (), stiRESULT_INVALID_PARAMETER);
	stiTESTCOND_NOLOG (!callHistoryItem.m_agentHistory.empty(), stiRESULT_INVALID_PARAMETER);

	if (callDirection == estiINCOMING)
	{
		crq = CRQ_VrsAgentHistoryAnsweredAdd;
		ExpectedResultAdd (CstiCoreResponse::eVRS_AGENT_HISTORY_ANSWERED_ADD_RESULT);
	}
	else if (callDirection == estiOUTGOING)
	{
		crq = CRQ_VrsAgentHistoryDialedAdd;
		ExpectedResultAdd (CstiCoreResponse::eVRS_AGENT_HISTORY_DIALED_ADD_RESULT);
	}

	hResult = AddRequest (crq, &historyElement, ATT_VrsCallId, callHistoryItem.vrsCallIdGet ().c_str ());
	stiTESTRESULT ();

	if (!std::string (callHistoryItem.ItemIdGet ()).empty())
	{
		ixmlElement_setAttribute (historyElement, ATT_CallListItemId, callHistoryItem.ItemIdGet ());
	}

	hResult = AddSubElementToRequest (TAG_Agent, nullptr, nullptr, TAG_AgentId, callHistoryItem.m_agentHistory.front().m_agentId.c_str ());
	stiTESTRESULT ();

STI_BAIL:
	return hResult;
}

stiHResult CstiCoreRequest::addFavoriteItemToElement(IXML_Element *pElement, const CstiFavorite &favorite)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	const char *szValue = favorite.IsContact() ? "Contact" : "Corporate";

	CstiItemId Id = favorite.PhoneNumberIdGet();
	if (Id.IsValid())
	{
		std::string IdString;
		Id.ItemIdGet(&IdString);
		hResult = AddSubElementToThisElement(
			pElement,
			(const char*)TAG_PublicId,
			IdString.c_str());
		stiTESTRESULT ();
	}

	hResult = AddSubElementToThisElement(
		pElement,
		(const char*)TAG_FavoriteType,
		szValue);
	stiTESTRESULT ();

	char szPosition[n32_BIT_INTEGER_LENGTH];
	sprintf(szPosition, "%d", favorite.PositionGet());
	hResult = AddSubElementToThisElement(
		pElement,
		(const char*)TAG_Position,
		szPosition);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

int CstiCoreRequest::favoriteListGet()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// check to see if this request already exists... if so, use existing tag
	if (!ObtainExistingRequest(CRQ_FavoritesListGet))
	{
		// create the CoreRequest to retrieve the list
		hResult = AddRequest(CRQ_FavoritesListGet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eFAVORITE_LIST_GET_RESULT);
	}

STI_BAIL:

	return requestResultGet (hResult);
}

int CstiCoreRequest::favoriteListSet (
	const SstiFavoriteList &list)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	// check to see if this request already exists... if so, use existing tag
	if (!ObtainExistingRequest(CRQ_FavoritesListSet))
	{
		// create the CoreRequest to retrieve the list
		hResult = AddRequest(CRQ_FavoritesListSet);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eFAVORITE_LIST_SET_RESULT);
	}

	// Finally, add each Favorite item in this Favorite list to the IXML representation
	for (const CstiFavorite &favorite : list.favorites)
	{
		IXML_Element *favoriteItemElement;

		hResult = AddSubElementToRequest(
			TAG_FavoriteItem,
			nullptr,
			&favoriteItemElement);
		stiTESTRESULT ();

		hResult = addFavoriteItemToElement(favoriteItemElement, favorite);
		stiTESTRESULT ();
	}

STI_BAIL:

	return requestResultGet (hResult);
}

int CstiCoreRequest::favoriteItemAdd (
	const CstiFavorite &favorite)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	IXML_Element *favoriteItemElement;

	// check to see if this request already exists... if so, use existing tag
	if (!ObtainExistingRequest(CRQ_FavoriteAdd))
	{
		// create the CoreRequest to add the favorite item
		hResult = AddRequest(CRQ_FavoriteAdd);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT);
	}

	hResult = AddSubElementToRequest(
		TAG_FavoriteItem,
		nullptr,
		&favoriteItemElement);
	stiTESTRESULT ();

	hResult = addFavoriteItemToElement(favoriteItemElement, favorite);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

int CstiCoreRequest::favoriteItemEdit (
	const CstiFavorite &favorite)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	IXML_Element *favoriteItemElement;

	// check to see if this request already exists... if so, use existing tag
	if (!ObtainExistingRequest(CRQ_FavoriteEdit))
	{
		// create the CoreRequest to edit the favorite item
		hResult = AddRequest(CRQ_FavoriteEdit);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eFAVORITE_ITEM_EDIT_RESULT);
	}

	hResult = AddSubElementToRequest(
		TAG_FavoriteItem,
		nullptr,
		&favoriteItemElement);
	stiTESTRESULT ();

	hResult = addFavoriteItemToElement(favoriteItemElement, favorite);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}

int CstiCoreRequest::favoriteItemRemove (
	const CstiFavorite &favorite)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Authentication required for this request
	m_bAuthenticationRequired = estiTRUE;

	IXML_Element *favoriteItemElement;

	// check to see if this request already exists... if so, use existing tag
	if (!ObtainExistingRequest(CRQ_FavoriteRemove))
	{
		// create the CoreRequest to remove the favorite item
		hResult = AddRequest(CRQ_FavoriteRemove);
		stiTESTRESULT ();

		ExpectedResultAdd (CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT);
	}

	hResult = AddSubElementToRequest(
		TAG_FavoriteItem,
		nullptr,
		&favoriteItemElement);
	stiTESTRESULT ();

	hResult = addFavoriteItemToElement(favoriteItemElement, favorite);
	stiTESTRESULT ();

STI_BAIL:

	return requestResultGet (hResult);
}
