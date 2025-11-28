/*!
 * \file CstiVPServiceRequest.cpp
 * \brief Base class for all enterpise services request classes
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2006 – 2012 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiVPServiceRequest.h"
#include "stiCoreRequestDefines.h"     // for the TAG_... defines, etc.
#include <ctime>
#include "ixml.h"


static const char REQ_ID[] = "Id";
static const char REQ_ClientToken[] = "ClientToken";


CstiVPServiceRequest::~CstiVPServiceRequest()
{
	if (m_pxDoc)
	{
		ixmlDocument_free(m_pxDoc);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//; CstiCoreRequest::Initialize
//  Constructs the initial shell of the needed XML document (creates
//   the main <CoreRequest> tag)
//
stiHResult CstiVPServiceRequest::Initialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pxDoc == nullptr)
	{
		int nResult;

		// create a new ixml document
		nResult = ixmlDocument_createDocumentEx(&m_pxDoc);
		stiTESTCOND (nResult == 0, stiRESULT_ERROR);

		// add the CoreRequest element as the root element
		nResult = ixmlDocument_createElementEx(m_pxDoc, (char*)RequestTypeGet(), (IXML_Element**)(void *)&m_pRootNode);
		stiTESTCOND (nResult == 0, stiRESULT_ERROR);

		nResult = ixmlNode_appendChild((IXML_Node*)m_pxDoc, m_pRootNode);
		stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (m_pxDoc)
		{
			ixmlDocument_free(m_pxDoc);
		}
		m_pxDoc = nullptr;
	}

	return hResult;
}


const char* CstiVPServiceRequest::XMLPrefixGet ()
{
	return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
}


/*!
 * \brief Adds the id and ClientToken attributes to the MessageRequest tag.
 *
 *   If the id and ClientToken attributes already exist, they are overwritten
 *   with the new data.
 */
int CstiVPServiceRequest::PrepareRequestForSubmission(
	const std::string &clientToken,		//!< The client token to use in the request
	const std::string &uniqueID)		//!< The unique id to use in the request
{
	int retVal = 0;

	// add the Id attribute to the MessageRequest element
	char idStr[12];
	sprintf(idStr, "%u", m_unId);
	bool bCreateIdAttr = true;
	bool bCreateTokenAttr = true;

	// Determine if id and ClientToken attrs exist, and if so update them
	IXML_NamedNodeMap *nodeMapPtr = ixmlNode_getAttributes((IXML_Node*)m_pRootNode);
	if (nodeMapPtr)
	{
		IXML_Node *idNode = ixmlNamedNodeMap_getNamedItem(nodeMapPtr, (char*)REQ_ID);
		if (idNode)
		{
			retVal |= ixmlNode_setNodeValue(idNode, idStr);
			bCreateIdAttr = false;
		}

		IXML_Node *tokenNode = ixmlNamedNodeMap_getNamedItem(nodeMapPtr, (char*)REQ_ClientToken);
		if (tokenNode)
		{
			retVal |= ixmlNode_setNodeValue(tokenNode, (char*)clientToken.c_str ());
			bCreateTokenAttr = false;
		}
		ixmlNamedNodeMap_free(nodeMapPtr);
	}

	if (bCreateIdAttr)
	{
		retVal |= ixmlElement_setAttribute((IXML_Element*)m_pRootNode, (char*)REQ_ID, idStr);
	}

	if (bCreateTokenAttr)
	{
		retVal |= ixmlElement_setAttribute((IXML_Element*)m_pRootNode, (char*)REQ_ClientToken, (char*)clientToken.c_str ());

		m_clientToken = clientToken;
	}

	// Is the client token empty?
	if (clientToken.empty () && UseUniqueIDAsAlternate() && !uniqueID.empty ())
	{
		retVal |= ixmlElement_setAttribute((IXML_Element*)m_pRootNode, (char*)TAG_UniqueId, uniqueID.c_str ());
	} // end if

	if (retVal)
	{
		if (m_pxDoc)
		{
			ixmlDocument_free(m_pxDoc);
		}
		m_pxDoc = nullptr;
		retVal = REQUEST_ERROR;
	}
	return retVal;

}


////////////////////////////////////////////////////////////////////////////////
// Retrieves the SSL mode of this CoreRequest
//
EstiBool CstiVPServiceRequest::SSLEnabledGet() const
{
	return m_bSSLEnabled;
}


////////////////////////////////////////////////////////////////////////////////
// Sets the SSL mode of this CoreRequest
//
void CstiVPServiceRequest::SSLEnabledSet(EstiBool bSSLEnabled)
{
	m_bSSLEnabled = bSSLEnabled;
}

////////////////////////////////////////////////////////////////////////////////
// Retrieves the Request Id of this MessageRequest
//
unsigned int CstiVPServiceRequest::requestIdGet()
{
	return m_unId;
}

////////////////////////////////////////////////////////////////////////////////
// Sets the Request Id of this MessageRequest
//
int CstiVPServiceRequest::requestIdSet(
	IN unsigned int unRequestNumber)
{
	int retVal = REQUEST_SUCCESS;

	m_unId = unRequestNumber;

	return retVal;
}


/*!
 * \brief Return the client token for this request
 *
 */
std::string CstiVPServiceRequest::clientTokenGet()
{
	return m_clientToken;
}


////////////////////////////////////////////////////////////////////////////////
//
//; CstiStateNotifyRequest::getXMLString
//  Retrieves the xml document as a character string
//
//
int CstiVPServiceRequest::getXMLString(
	OUT char **xmlString)
{
	char *xmlTmp, *szReturnString;
	xmlTmp = ixmlNodetoString((IXML_Node*)m_pxDoc);

	// get the length of the prefix string and the xml stream
	size_t len = strlen(XMLPrefixGet()) + 1;
	if (xmlTmp)
	{
		len += (strlen(xmlTmp) + 1);
	}

	// allocate buffer and copy completed xml string to it
	szReturnString = new char[len];
	strcpy(szReturnString, XMLPrefixGet());
	if (xmlTmp)
	{
		strcat(szReturnString, xmlTmp);
	}

	// free the temporary xml stream
	ixmlFreeDOMString(xmlTmp);

	// point return string to newly created string
	*xmlString = szReturnString;

	return 0;
}


static stiHResult SetAttribute (
	IXML_Element *pElement,
	const char *pszAttribute,
	const char *pszValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;
	char dummyVal[] = " ";
	char *attrVal;

	if (nullptr != pszAttribute)
	{
		if (nullptr == pszValue)
		{
			attrVal = dummyVal;
		}
		else
		{
			attrVal = (char*)pszValue;
		}

		nResult = ixmlElement_setAttribute(pElement, (char*)pszAttribute, attrVal);

		stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief Constructs the initial shell of the needed XML document
 *
 * Creates a child of type szRequest under the main MessageRequest tag.
 */
stiHResult CstiVPServiceRequest::AddRequest(
	IN const char *szRequest,
	OUT IXML_Element **ppNewElement,
	IN const char *attr1,
	IN const char *attr1Val,
	IN const char *attr2,
	IN const char *attr2Val)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;
	IXML_Element *pxeRequest = nullptr;

	// call initialize to ensure core request is initialized
	hResult = Initialize();  // TODO: what happens on failure?

	stiTESTRESULT ();

	// Add the request tag to the basic CoreRequest document
	pxeRequest = ixmlDocument_createElement(m_pxDoc, (char*)szRequest);

	// Add the attributes to the tag, if they exist
	hResult = SetAttribute (pxeRequest, attr1, attr1Val);

	stiTESTRESULT ();

	hResult = SetAttribute (pxeRequest, attr2, attr2Val);

	stiTESTRESULT ();

	nResult = ixmlNode_appendChild(m_pRootNode, (IXML_Node*)pxeRequest);

	stiTESTCOND (nResult == 0, stiRESULT_ERROR);

	m_pMainReq = (IXML_Node*)pxeRequest;

	if (nullptr != ppNewElement)
	{
		*ppNewElement = pxeRequest;
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief Adds a sub element to the initially created core request (child of the request tag)
 *
 */
stiHResult CstiVPServiceRequest::AddSubElementToRequest(
	IN const char *szElement,
	IN const char *content,
	OUT IXML_Element **ppNewElement,
	IN const char *attr1,
	IN const char *attr1Val,
	IN const char *attr2,
	IN const char *attr2Val)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;

	// Add the subElement tag to the main Request tag
	IXML_Element* subElem = ixmlDocument_createElement(m_pxDoc, (char*)szElement);

	// Add the attributes to the tag, if they exist
	hResult = SetAttribute (subElem, attr1, attr1Val);

	stiTESTRESULT ();

	hResult = SetAttribute (subElem, attr2, attr2Val);

	stiTESTRESULT ();

	nResult = ixmlNode_appendChild(m_pMainReq, (IXML_Node*)subElem);

	stiTESTCOND (nResult == 0, stiRESULT_ERROR);

	if (nullptr != content)
	{
		// create a text element and add it to the newly created subElement
		IXML_Node *textNode;
		nResult = ixmlDocument_createTextNodeEx(m_pxDoc, (char*)content, &textNode);

		stiTESTCOND (nResult == 0, stiRESULT_ERROR);

		nResult = ixmlNode_appendChild((IXML_Node*)subElem, textNode);

		stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	}

	if (nullptr != ppNewElement)
	{
		*ppNewElement = subElem;
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//
//; CstiCoreRequest::AddSubElementThisElement
//  Adds a sub element to the passed-in element (parentElement)
//
//
stiHResult CstiVPServiceRequest::AddSubElementToThisElement(
	const IXML_Element *parentElement,
	const char *szElement,
	const char *content,
	IXML_Element **ppNewElement,
	const char *attr1,
	const char *attr1Val,
	const char *attr2,
	const char *attr2Val)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;

	// Add the subElement tag to the main Request tag
	IXML_Element* subElem = ixmlDocument_createElement(m_pxDoc, (char*)szElement);

	// Add the attributes to the tag, if they exist
	// Add the attributes to the tag, if they exist
	hResult = SetAttribute (subElem, attr1, attr1Val);

	stiTESTRESULT ();

	hResult = SetAttribute (subElem, attr2, attr2Val);

	stiTESTRESULT ();

	nResult = ixmlNode_appendChild((IXML_Node*)parentElement, (IXML_Node*)subElem);

	stiTESTCOND (nResult == 0, stiRESULT_ERROR);

	if (nullptr != content)
	{
		// create a text element and add it to the newly created subElement
		IXML_Node *textNode;
		nResult = ixmlDocument_createTextNodeEx(m_pxDoc, content, &textNode);

		stiTESTCOND (nResult == 0, stiRESULT_ERROR);

		nResult = ixmlNode_appendChild((IXML_Node*)subElem, textNode);

		stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	}

	if (nullptr != ppNewElement)
	{
		*ppNewElement = subElem;
	}

STI_BAIL:

	return hResult;
}


// Points m_pMainReq to the request of type tag name, if it exists
// NOTE: protected function
bool CstiVPServiceRequest::ObtainExistingRequest(
	const char *tagName)
{
	bool requestExists = false;

	IXML_NodeList *nl = ixmlDocument_getElementsByTagName(m_pxDoc, (char*)tagName);
	if (ixmlNodeList_length(nl))
	{
		requestExists = true;
		m_pMainReq = ixmlNodeList_item(nl, 0);
	}
	else
	{
		m_pMainReq = nullptr;
	}

	ixmlNodeList_free(nl);

	return requestExists;
}


/*!
 * \brief Gets pointers to the main node and the IXML_Document itself
 *  (so calling functions can use the pointers to do more custom XML files)
 *
 */
int CstiVPServiceRequest::GetIXMLDocumentPointers(
	IXML_Document **pxDoc,
	IXML_Node **pMainNode )
{
	if (m_pxDoc == nullptr || m_pMainReq == nullptr)
	{
		return REQUEST_ERROR;
	}
	*pxDoc = m_pxDoc;
	*pMainNode = m_pMainReq;

	return REQUEST_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//
//; CstiStateNotifyRequest::freeXMLString
//  Frees the character string returned by getXMLString
//
//
void CstiVPServiceRequest::freeXMLString(IN const char *xmlString)
{
	if (xmlString == nullptr)
	{
		return;
	}
	delete [] xmlString;
	xmlString = nullptr;
}


EstiBool &CstiVPServiceRequest::RemoveUponCommunicationError()
{
	return m_bRemoveUponCommunicationError;
}


void GetFormattedTime (
	time_t *pTime,
	char *pszFormattedTime,
	int nLength)
{
	struct tm  pstTime{};
#ifdef stiLINUX
	gmtime_r (pTime, &pstTime); // Convert to UTC time
#elif defined WIN32
	gmtime_s (&pstTime, pTime);
#endif

	strftime (pszFormattedTime, nLength, "%m/%d/%Y %H:%M:%S", &pstTime);
}


stiHResult addTimeAttributeToElement (IXML_Element *element, time_t time)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (time != 0)
	{
		char szTime[32];

		GetFormattedTime (&time, szTime, sizeof (szTime));

		auto retVal = ixmlElement_setAttribute(element, (char*)TAG_Time, (char*)szTime);
		stiTESTCOND (retVal == IXML_SUCCESS, stiRESULT_ERROR);
	} // end if

STI_BAIL:

	return hResult;
}


stiHResult CstiVPServiceRequest::addTimeToSubElement (IXML_Element *element, time_t time)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (time != 0)
	{
		char szTime[32];

		GetFormattedTime (&time, szTime, sizeof (szTime));

		hResult = AddSubElementToThisElement(
			element,
			(const char*)TAG_Time,
			(const char*)szTime);
		stiTESTRESULT ();
	} // end if

STI_BAIL:

	return hResult;
}


bool CstiVPServiceRequest::UseUniqueIDAsAlternate ()
{
	return false;
}


void CstiVPServiceRequest::ExpectedResultAdd (
	int nResponse)
{
	SResponse Response{};

	Response.nResponse = nResponse;
	Response.bHandled = false;

	m_ExpectedResponses.push_back (Response);
}


void CstiVPServiceRequest::ExpectedResultsReset ()
{
	std::list <SResponse>::iterator i;

	for (i = m_ExpectedResponses.begin (); i != m_ExpectedResponses.end (); i++)
	{
		(*i).bHandled = false;
	} // end for
}


void CstiVPServiceRequest::ExpectedResultHandled (
	int nResponse)
{
	std::list <SResponse>::iterator i;

	for (i = m_ExpectedResponses.begin (); i != m_ExpectedResponses.end (); i++)
	{
		if (nResponse == (*i).nResponse && !(*i).bHandled)
		{
			(*i).bHandled = true;
			break;
		} // end if
	} // end for
}


void CstiVPServiceRequest::ProcessRemainingResponses (
	PstiRemainingResponseCallback pCallback,
	int nSystemicError,
	size_t CallbackParam)
{
	std::list <SResponse>::iterator i;

	for (i = m_ExpectedResponses.begin (); i != m_ExpectedResponses.end (); i++)
	{
		if (!(*i).bHandled)
		{
			pCallback (this, (*i).nResponse, nSystemicError, CallbackParam);
		} // end if
	} // end for
}


void CstiVPServiceRequest::retryOfflineSet (bool value)
{
	if (value)
	{
		RemoveUponCommunicationError () = estiTRUE;
	}
	m_retryOffline = value;
}


bool CstiVPServiceRequest::retryOfflineGet ()
{
	return m_retryOffline;
}

