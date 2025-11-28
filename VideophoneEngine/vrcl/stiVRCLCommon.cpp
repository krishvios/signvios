////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:  CstiVRCLCommon.cpp
//
//  Abstract:
// 		This file contains helper functions used by both the VRCLClient and
// 		the VRCLServer.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "stiVRCLCommon.h"

#include "stiTrace.h"
#include "stiTools.h"

//
// Constants
//
const char g_szAPIVersion[] = "4.5";


const char g_szCallHangupMessage[] = "<CallHangUp />";
const char g_szCallAnswerMessage[] = "<CallAnswer />";
const char g_szCallRejectMessage[] = "<CallReject />";
const char g_szIsAliveMessage[] = "<IsAlive />";
const char g_szIsCallTransferableMessage[] = "<IsCallTransferable />";
const char g_szStatusCheckMessage[] = "<StatusCheck />";
const char g_szRebootMessage[] = "<Reboot />";
const char g_szApplicationVersionGetMessage[] = "<ApplicationVersionGet />";
const char g_szApplicationCapabilitiesGetMessage[] = "<ApplicationCapabilitiesGet />";
const char g_szDeviceTypeGetMessage[] = "<DeviceTypeGet />";
const char g_szCallBandwidthDetectionBegin[] = "<CallBandwidthDetectionBegin />";
const char g_szInCallStatsGet[] = "<InCallStatsGet />";

const char g_szEnableSignMailRecording[]  = "EnableSignMailRecording";
const char g_szSignMailRecordingEnabled[] = "SignMailRecordingEnabled";  // Confirm that SignMailRecording has been enabled
const char g_szSignMailAvailable[] = "SignMailAvailable";  // Inform the client that SignMail is available at the remote device
const char g_szSignMailRecordingStarted[] = "SignMailRecordingStarted";  // Inform the client that Signmail recording has started
const char g_szSignMailGreetingStarted[] = "SignMailGreetingStarted";
const char g_szSkipToSignMailRecord[] = "SkipToSignMailRecord";
const char g_szDtmfSend[] = "DtmfSend";
const char g_szTextBackspaceSend[] = "TextBackspaceSend";
const char g_szTextMessageSend[] = "TextMessageSend";
const char g_szTextMessageClear[] = "TextMessageClear";
const char g_szTextMessageCleared[] = "TextMessageCleared";
const char g_szPhoneNumberDialMessage[] = "PhoneNumberDial";
const char g_szExternalConferenceDialMessage[] = "ExternalConferenceDial";
const char g_szExternalCameraUseMessage[] = "ExternalCameraUse";
const char g_szCallId[] = "CallId";
const char g_szTerpId[] = "TerpId";
const char g_szHearingNumber[] = "HearingNumber";
const char g_szHearingName[] = "HearingName";
const char g_szDestNumber[] = "DestNumber";
const char g_szAlwaysAllow[] = "AlwaysAllow";
const char g_szRelayLanguage[] = "RelayLanguage";
const char g_szLoggedInPhoneNumberGet[] = "LoggedInPhoneNumberGet";
const char g_szTerpNetModeSet[] = "TerpNetModeSet";
const char g_szUpdateCheck[] = "UpdateCheck";
const char g_szUpdateCheckResult[] = "UpdateCheckResult";
const char g_szCallIdSet[] = "CallIdSet";
const char g_szCallIdSetResult[] = "CallIdSetResult";
const char g_szDiagnosticsGet[] = "DiagnosticsGet";
const char g_szCount[] = "Count";
const char g_szTextInSignMail[] = "TextInSignMail";
const char g_szSignMailText[] = "SignMailText";
const char g_szStartSeconds[] = "StartSeconds";
const char g_szEndSeconds[] = "EndSeconds";
const char g_hearingVideoCapability[] = "HearingVideoCapability";
const char g_hearingNumber[] = "HearingNumber";
const char g_hearingVideoCapable[] = "HearingVideoCapable";
const char g_hearingVideoConnected[] = "HearingVideoConnected";
const char g_hearingVideoConnectionFailed[] = "HearingVideoConnectionFailed";
const char g_hearingVideoDisconnected[] = "HearingVideoDisconnected";
const char g_hearingVideoDisconnectedByHearing[] = "Hearing";
const char g_hearingVideoDisconnectedByDeaf[] = "Deaf";
const char g_hearingVideoConnecting[] = "HearingVideoConnecting";
const char g_hearingVideoConnectingConfURI[] = "ConferenceURI";
const char g_hearingVideoConnectingInitiator[] = "Initiator";
const char g_hearingVideoConnectingProfileId[] = "HearingVideoProfileID";

const std::string g_headers{ "Headers" };
const std::string g_headerItem{ "HeaderItem" };
const std::string g_headerItemName{ "Name" };
const std::string g_headerItemBody{ "Body" };

// const char g_szCallBridgeDial[] = "CallBridgeDial";
const char g_szCallBridgeDisconnect[] = "<CallBridgeDisconnect />";
const char g_szAllowedAudioCodecs[] = "AllowedAudioCodecs";
const char g_szLoggedInPhoneNumber[] = "LoggedInPhoneNumber";
const char g_szVcoPref[] = "VcoPref";
const char g_szVcoActive[] = "VcoActive";

const char g_szAnnounceClear[] = "AnnounceClear";
const char g_szAnnounceSet[] = "AnnounceSet";

const char g_szTrue[] = "true";
// const char g_szFalse[] = "false";

const char g_szSuccess[] = "Success";
const char g_szFail[] = "Fail";
const char g_szInCall[] = "InCall";
const char g_szAppVersionOK[] = "AppVersionOK";

const char g_szHearingStatus[] = "HearingStatus";
const char g_szRelayNewCallReady[] = "RelayNewCallReady";
const char g_szHearingStatusResponse[] = "HearingStatusResponse";
const char g_szRelayNewCallReadyResponse[] = "RelayNewCallReadyResponse";
const char g_szHearingCallSpawn[] = "HearingCallSpawn";
const char g_szGeolocation[] = "Geolocation";
const char g_szConferenceURI[] = "ConferenceURI";
const char g_szConferenceType[] = "ConferenceType";
const char g_szPublicIP[] = "PublicIP";
const char g_szSecurityToken[] = "SecurityToken";
const char g_szInUse[] = "InUse";

// Commands/Responses Sent

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//

//
// Function Declarations
//

/*! \brief Add an attribute to an element in the XML document
 * 
 *  \param pXmlElement - The element that the attribute is added to
 *  \param pszAttrName - The name of the attribute to be added to the element
 *  \param pszAttrValue - The value of the attribute
 *  \retval stiHResult
 */
stiHResult AddAttribute(
	IXML_Element *pXmlElement, 
	const char *pszAttrName, 
	const char *pszAttrValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult;
	char dummyVal[] = " ";
	const char *pszAttrVal;

	if (nullptr != pszAttrName)
	{
		if (nullptr == pszAttrValue)
		{
			pszAttrVal = dummyVal;
		}
		else
		{
			pszAttrVal = (char*)pszAttrValue;
		}

		nResult = ixmlElement_setAttribute(pXmlElement, pszAttrName, pszAttrVal);

		stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


/*! \brief Add an element to the XML document along with optionally two attributes
 *
 *  \param pXmlDoc - The XML document to be added to
 *  \param pXmlNode - The node on which to append the element
 *  \param ppXmlElement - This will be the element that is added
 *  \param pszElementName - The name of the element to be added
 *  \param pszAttr1Name - An optional attribute to be added to the element
 *  \param pszAttr1Value - The value of the first attribute
 *  \param pszAttr2Name - An optional 2nd attribute to be added to the element
 *  \param pszAttr2Value - The value of the second attribute
 *  \retval stiHResult
 */
stiHResult AddXmlElement(
	IXML_Document *pXmlDoc, 
	IXML_Node *pXmlNode, 
	IXML_Element **ppXmlElement, 
	const char *pszElementName,
	const char *pszAttr1Name, 
	const char *pszAttr1Value,
	const char *pszAttr2Name,
	const char *pszAttr2Value)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;

	IXML_Element *pXmlElement = nullptr;
	pXmlElement = ixmlDocument_createElement(pXmlDoc, pszElementName);
	stiTESTCOND (pXmlElement, stiRESULT_ERROR);

	if (pszAttr1Name)
	{
		AddAttribute(pXmlElement, pszAttr1Name, pszAttr1Value);
	}

	if (pszAttr2Name)
	{
		AddAttribute(pXmlElement, pszAttr2Name, pszAttr2Value);
	}

	nResult = ixmlNode_appendChild(pXmlNode, (IXML_Node*)pXmlElement);
	stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	
	*ppXmlElement = pXmlElement;
	pXmlElement = nullptr;

STI_BAIL:
	if (hResult != stiRESULT_SUCCESS &&
		pXmlElement)
	{
		ixmlElement_free(pXmlElement);
		pXmlElement = nullptr;
	}

	return hResult;
}


/*! \brief Add an element node along with a text value.
 * 
 *  \param pXmlDoc - The XML document that we are adding to
 *  \param pRootNode - The root node under which to add this element
 *  \param pszElementName - The name of the element node
 *  \param pszValue - The text node value
 * 
 *  The XML being created will look something like:
 * 	<pszRootNode>
 * 		<pszElementName>pszValue</pszElementName>
 * 	</pszRootNode>
 * 
 *  \retval stiHResult
 */
stiHResult AddXmlElementAndValue (
	IXML_Document *pXmlDoc, 
	IXML_Node *pRootNode, 
	const char *pszElementName,
	const char *pszValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;
	
	IXML_Element *pXmlElement = nullptr;
	IXML_Node *pXmlTextNode = nullptr;
	pXmlElement = ixmlDocument_createElement (pXmlDoc, pszElementName);
	stiTESTCOND (pXmlElement, stiRESULT_ERROR);
	
	pXmlTextNode = ixmlDocument_createTextNode (pXmlDoc, pszValue);
	
	// Add the text node to the newly created element
	nResult = ixmlNode_appendChild ((IXML_Node*)pXmlElement, pXmlTextNode);
	stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	
	// Add the element node to the root node
	nResult = ixmlNode_appendChild (pRootNode, (IXML_Node*)pXmlElement);
	stiTESTCOND (nResult == 0, stiRESULT_ERROR);
	
STI_BAIL:
	if (hResult != stiRESULT_SUCCESS)
	{
		if (pXmlElement)
		{
			ixmlElement_free(pXmlElement);
			pXmlElement = nullptr;
		}
		
		if (pXmlTextNode)
		{
			ixmlNode_free (pXmlTextNode);
			pXmlTextNode = nullptr;
		}
	}
	
	return hResult;
}

/*! \brief Free up the XML document
 * 
 *  \param pXmlDoc - the XML document being freed
 *  \retval none
 */
void CleanupXmlDocument(
	IXML_Document *pXmlDoc)
{
	if (pXmlDoc)
	{
		ixmlDocument_free(pXmlDoc);
	}
}


/*! \brief Load a formatted XML string into an XML document
 * 
 *  \param pXmlString - The formatted XML string
 *  \param ppXmlDoc - Upon success, this will be the created XML document
 *  \retval stiHResult
 */
stiHResult CreateXmlDocumentFromString(
	const char *pXmlString,
	IXML_Document **ppXmlDoc)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nReturn = 0;
	IXML_Document *pDocument = nullptr;
	*ppXmlDoc = nullptr;

	// Create an XML document from the data that was supplied
	nReturn = ixmlParseBufferEx (pXmlString, &pDocument);
	stiTESTCOND (IXML_SUCCESS == nReturn && pDocument != nullptr, stiRESULT_ERROR);
	
	*ppXmlDoc = pDocument;
	pDocument = nullptr;

STI_BAIL:
	if (hResult != stiRESULT_SUCCESS &&
		pDocument)
	{
		ixmlDocument_free(pDocument);
		pDocument = nullptr;
	}

	return hResult;
}


/*! \brief Retrieve the value of an attribute from an XML element
 * 
 *  \param pXmlElement - The element that contains the attribute being looked up
 *  \param pszAttrName - The attribute being looked up
 *  \param ppszAttrValue - The returned value of the attribute
 * 
 *  The XML being looked at will look something like:
 * 	<Element Attribute=value></Element>
 * 
 *  \retval estiERROR otherwise
 */
stiHResult GetAttrValueString(
	IXML_Element *pXmlElement,
	const char *pszAttrName,
	const char **ppszAttrValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Attr *pAttr = nullptr;

	stiTESTCOND (pXmlElement && pszAttrName, stiRESULT_ERROR);

	pAttr = ixmlElement_getAttributeNode(pXmlElement, pszAttrName);
	stiTESTCOND (pAttr, stiRESULT_ERROR);

	*ppszAttrValue = ixmlNode_getNodeValue((IXML_Node *)pAttr);
	stiTESTCOND (*ppszAttrValue, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}


/*! \brief Retrieve the first child's node name
 * 
 *  \param pXmlNode - The node whose child we are interested in
 *  \param ppszNodeType - The name of the first child's node
 *  \param ppChildNode - The XML Node of the first child
 * 
 *  \NOTE:  This method allocates memory to return the type in.  This must be freed by the calling method.
 * 
 *  \retval stiHResult
 */
stiHResult GetChildNodeType(
	IXML_Node *pXmlNode,
	char **ppszNodeType,
	IXML_Node **ppChildNode)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Node *pChildNode = nullptr;
	const char *pszNodeType = nullptr;
	unsigned int unNodeTypeLength = 0;

	pChildNode = ixmlNode_getFirstChild(pXmlNode);
	stiTESTCOND (pChildNode, stiRESULT_ERROR);

	pszNodeType = ixmlNode_getNodeName(pChildNode);
	stiTESTCOND (pszNodeType, stiRESULT_ERROR);

	unNodeTypeLength = strlen(pszNodeType) + 1;
	*ppszNodeType = new char [unNodeTypeLength];
	stiTESTCOND (*ppszNodeType, stiRESULT_ERROR);

	strcpy(*ppszNodeType, pszNodeType);
	*ppChildNode = pChildNode;
	pChildNode = nullptr;

STI_BAIL:

	return hResult;
}


/*! \brief Retrieve the first child's node name and value
 * 
 *  \param pXmlNode - The node whose child we are interested in
 *  \param ppszNodeName - The name of the first child's node
 *  \param ppszNodeValue - The value of the first child's node
 *  \param ppChildNode - The XML Node of the first child
 * 
 *  \NOTE:  This method allocates memory to return the name and values in.  They must be freed by the calling method.
 * 
 *  The XML being looked at will look something like:
 * 	<pXmlNode>
 * 		<ppszNodeName>ppszNodeValue</ppszNodeName>
 * 	</ParentNode>
 * 
 *  \retval stiHResult
 */
stiHResult GetChildNodeNameAndValue(
	IXML_Node *pXmlNode,
	std::string &nodeName,
	std::string &nodeValue,
	IXML_Node **ppChildNode)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Node *pChildNode = nullptr;
	const char *pszNodeName = nullptr;
	
	pChildNode = ixmlNode_getFirstChild(pXmlNode);
	stiTESTCOND (pChildNode, stiRESULT_ERROR);
	
	pszNodeName = ixmlNode_getNodeName(pChildNode);
	stiTESTCOND (pszNodeName, stiRESULT_ERROR);
	
	nodeName = pszNodeName;
	
	hResult = GetTextNodeValue(pChildNode, nodeValue);
	stiTESTRESULT ();
	
	*ppChildNode = pChildNode;
	pChildNode = nullptr;
	
STI_BAIL:

	return hResult;
}


/*! \brief Retrieve the next child's node name and value
 * 
 *  \param pXmlNode - The node whose child we are interested in
 *  \param ppszNodeName - The name of the first child's node
 *  \param ppszNodeValue - The value of the first child's node
 *  \param ppChildNode - The XML Node of the first child
 * 
 *  \NOTE:  This method allocates memory to return the name and values in.  They must be freed by the calling method.
 * 
 *  The XML being looked at will look something like:
 * 	<ParentNode>
 * 		<pXmlNode>someValue</pXmlNode>
 * 		<ppszNodeName>ppszNodeValue</ppszNodeName>
 * 	</ParentNode>
 * 
 *  \retval stiHResult
 */
stiHResult GetNextChildNodeNameAndValue(
	IXML_Node *pXmlNode,
	std::string &nodeName,
	std::string &nodeValue,
	IXML_Node **ppChildNode)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	IXML_Node *pChildNode = nullptr;
	const char *pszNodeName = nullptr;
	
	pChildNode = ixmlNode_getNextSibling (pXmlNode);
	stiTESTCOND (pChildNode, stiRESULT_ERROR);
	
	pszNodeName = ixmlNode_getNodeName(pChildNode);
	stiTESTCOND (pszNodeName, stiRESULT_ERROR);
	
	nodeName = pszNodeName;

	hResult = GetTextNodeValue(pChildNode, nodeValue);
	stiTESTRESULT ();
	
	*ppChildNode = pChildNode;
	pChildNode = nullptr;
	
STI_BAIL:

	return hResult;
}


/*! \brief Retrieve the text node value.
 * 
 *  \param pXmlNode - The node whose child text node we are interested in
 *  \param ppszNodeValue - The value of the text node
 * 
 *  \NOTE:  This method allocates memory to return the name and values in.  They must be freed by the calling method.
 * 
 *  The XML being looked at will look something like:
 * 	<pXmlNode>textValue</pXmlNode>
 * 
 *  \retval stiHResult
 */
stiHResult GetTextNodeValue (
	IXML_Node *pXmlNode,
	std::string &nodeValue)
{
	IXML_Node *pValueNode = nullptr;
	const char *pszNodeValue = nullptr;
	const char *pszNodeName = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;

	// Get the child node (the node that contains the value we are after)
	pValueNode = ixmlNode_getFirstChild(pXmlNode);
	pszNodeName = ixmlNode_getNodeName (pValueNode);

	// If the node name is null, then the value was empty, which is a valid value
	if (!pszNodeName)
	{
		nodeValue.clear ();
	}
	else
	{
		// If the xml at pXmlNode looks like <name>value</name>, then pszNodeName will contain "#text" in it at this point.
		// If the xml looks more like
		//   <name>
		//      <subname>value</subname>
		//   </name>
		// pszNodeName will not contain "#text" but instead will contain subname.

		stiTESTCOND_NOLOG (pszNodeName && 0 == strcmp ("#text", pszNodeName), stiRESULT_ERROR);

		// Now get the value (this is a pointer to the actual string ... don't modify it.  Instead, make a copy to return).
		pszNodeValue = ixmlNode_getNodeValue (pValueNode);
		stiTESTCOND (pszNodeValue, stiRESULT_ERROR);

		nodeValue = pszNodeValue;
	}

STI_BAIL:

	return hResult;
}


/*! \brief Retrieve the wide/unicode text node value.
 *
 *  \param pXmlNode - The node whose child text node we are interested in
 *  \param ppwszNodeValue - The value of the text node
 *
 *  \NOTE:  This method allocates memory to return the name and values in.  They must be freed by the calling method.
 *
 *  TODO:  This does not yet account for xml encoding, nor ampersand-delimited characters like &gt; and so on.
 *
 *  The XML being looked at will look something like:
 * 	<pXmlNode>textValue</pXmlNode>
 *
 *  \retval stiHResult
 */
stiHResult GetTextNodeValueWide (
	IXML_Node *pXmlNode,
	uint16_t **ppwszNodeValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string nodeValue {};
	const uint8_t* nodeUtf8 {nullptr};
	int unicodeLength {0};

	hResult = GetTextNodeValue (pXmlNode, nodeValue);
	stiTESTRESULT ();

	nodeUtf8 = reinterpret_cast<const uint8_t*> (nodeValue.data ());
	stiTESTCOND (nodeUtf8, stiRESULT_ERROR);

	unicodeLength = UTF8LengthGet (nodeUtf8, nodeValue.size ());

	*ppwszNodeValue = new uint16_t[unicodeLength + 1];
	stiTESTCOND (*ppwszNodeValue, stiRESULT_ERROR);

	unicodeLength = UTF8ToUnicodeConvert (*ppwszNodeValue, nodeUtf8, nodeValue.size ());
	stiTESTCOND (unicodeLength >= 0, stiRESULT_ERROR);

	// Null terminate the result.
	(*ppwszNodeValue)[unicodeLength] = 0;

STI_BAIL:

	return hResult;
}


/*! \brief Create a formatted XML string from an XML node
 * 
 *  \param pXmlNode - The node whose tree we will get a string representation of
 *  \param ppszXmlString - The string representation of the xml node
 * 
 *  \NOTE:  This method allocates memory to return the string value in.  This must be freed by the calling method.
 * 
 *  \retval stiHResult
 */
stiHResult GetXmlString(
	IXML_Node *pXmlNode,
	char **ppszXmlString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	char *pXmlTmp = nullptr;
	char *pszReturnString = nullptr;
	size_t xmlStrLen = 0;

	pXmlTmp = ixmlNodetoString(pXmlNode);
	stiTESTCOND (pXmlTmp, stiRESULT_ERROR);

	// allocate buffer and copy completed xml string to it
	xmlStrLen = strlen(pXmlTmp) + 1;
	pszReturnString = new char[xmlStrLen];
	stiTESTCOND (pszReturnString, stiRESULT_ERROR);

	strcpy(pszReturnString, pXmlTmp);
	*ppszXmlString = pszReturnString;
	pszReturnString = nullptr;

STI_BAIL:
	if (hResult != stiRESULT_SUCCESS &&
		pszReturnString)
	{
		delete []pszReturnString;
		pszReturnString = nullptr;
	}

	if (pXmlTmp)
	{
		// free the temporary xml stream
		ixmlFreeDOMString(pXmlTmp);
		pXmlTmp = nullptr;
	}

	return hResult;
}


/*! \brief Create an XML document
 * 
 *  \param pXmlDoc - This will be the pointer to the document created.
 *  \param ppRootNode - The pointer to the root node within the newly created document.
 *  \param pszNodeType - The name of the root node to be added.
 * 
 *  \retval stiHResult
 */
stiHResult InitXmlDocument(
	IXML_Document **ppXmlDoc,
	IXML_Node **ppRootNode,
	const char *pszNodeType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;

	IXML_Document *pxDoc = nullptr;
	IXML_Node *pRootNode = nullptr;

	nResult = ixmlDocument_createDocumentEx(&pxDoc);
	stiTESTCOND (nResult == 0, stiRESULT_ERROR);

	// add the element as the root element     
	nResult = ixmlDocument_createElementEx(pxDoc, pszNodeType, (IXML_Element**)(void *)&pRootNode);
	stiTESTCOND (nResult == 0, stiRESULT_ERROR);

	nResult = ixmlNode_appendChild((IXML_Node*)pxDoc, pRootNode);
	stiTESTCOND (nResult == 0, stiRESULT_ERROR);

	*ppXmlDoc = pxDoc;
	pxDoc = nullptr;

	*ppRootNode = pRootNode;
	pRootNode = nullptr;

STI_BAIL:
	if (hResult != stiRESULT_SUCCESS)
	{
		if (pxDoc)
		{
			ixmlDocument_free(pxDoc);
			pxDoc = nullptr;
		}

		if (pRootNode)
		{
			ixmlElement_free((IXML_Element *)pRootNode);
			pRootNode = nullptr;
		}
	}

	return hResult;
}


/*! \brief Create an XML document
 * 
 *  \param pXmlNode - The node whose children we want to count.
 * 
 *  \retval int - the number of child nodes
 */
int GetNodeChildCount (
	IXML_Node *pXmlNode)
{
	unsigned long unCount = 0;

	IXML_NodeList *pList = ixmlNode_getChildNodes (pXmlNode);
	
	unCount = ixmlNodeList_length (pList);
	
	ixmlNodeList_free (pList);
	
	return (int)unCount;
}

/*! \brief Process a node in PhoneNumberDial XML
 *
 *  \param node - The node to process
 *  \param callDialInfo. The struct to put parsed data into

 *  \retval stiHResult
 */
stiHResult phoneNumberDialNodeProcess(
	IXML_Node* node,
	SstiCallDialInfo &callDialInfo)
{
	auto hResult = stiRESULT_SUCCESS;
	unsigned short nodeType{ eINVALID_NODE };
	std::string nodeName{};
	std::string nodeValue{};
	const char* cstr{ nullptr };

	// make sure this node is an element
	nodeType = ixmlNode_getNodeType(node);
	stiTESTCOND(nodeType == eELEMENT_NODE, stiRESULT_ERROR);

	// get the tag name
	cstr = ixmlNode_getNodeName(node);
	stiTESTCOND(cstr, stiRESULT_ERROR);
	nodeName = cstr;

	// get text value
	nodeValue = nodeTextValueGet(node);

	if (!nodeValue.empty())
	{
		terpStructLoad(callDialInfo, nodeName, nodeValue);
	}
	else if (nodeName == g_headers)
	{
		terpStructHeadersLoad(callDialInfo, node);
	}

STI_BAIL:

	return hResult;
}


/*! \brief Get text value of XML element or empty if no text value.
 *
 *  \param node - The node to get text value from

 *  \retval std::string - Text value of XML element. Will be empty if no text value.
 */
std::string nodeTextValueGet(
	IXML_Node* node) 
{
	std::string textValue{};
	IXML_NodeList* nodeListHead{ nullptr };
	IXML_NodeList* nodeListPtr{ nullptr };

	// get text value from first text node found in children
	nodeListHead = ixmlNode_getChildNodes(node);
	nodeListPtr = nodeListHead;
	while (nodeListPtr && nodeListPtr->nodeItem)
	{
		IXML_Node* item = nodeListPtr->nodeItem;
		if (ixmlNode_getNodeType(item) == eTEXT_NODE)
		{
			const char* cstr = ixmlNode_getNodeValue(item);
			if (cstr)
			{
				textValue = cstr;
			}
			break;
		}
		nodeListPtr = nodeListPtr->next;
	}
	nodeListPtr = nullptr;
	ixmlNodeList_free(nodeListHead);

	return textValue;
}

/*! \brief Helper function to assign call dial info from XML node name and value
 *
 *  \param callDialInfo - The call dial info struct to fill
 *  \param nodeName - Name of node
 *  \param nodeValue - Text value of node
 */
void terpStructLoad(SstiCallDialInfo& callDialInfo, const std::string& nodeName, const std::string& nodeValue)
{
	if (g_szCallId == nodeName)
	{
		callDialInfo.CallId = nodeValue;
	}
	else if (g_szTerpId == nodeName)
	{
		callDialInfo.TerpId = nodeValue;
	}
	else if (g_szHearingNumber == nodeName)
	{
		callDialInfo.HearingNumber = nodeValue;
	}
	else if (g_szHearingName == nodeName)
	{
		callDialInfo.hearingName = nodeValue;
	}
	else if (g_szDestNumber == nodeName)
	{
		callDialInfo.DestNumber = nodeValue;
	}
	else if (g_szAlwaysAllow == nodeName)
	{
		callDialInfo.bAlwaysAllow = (stricmp(nodeValue.c_str(), g_szTrue) == 0);
	}
	else if (g_szRelayLanguage == nodeName)
	{
		callDialInfo.RelayLanguage = nodeValue;
	}
}

/*! \brief Helper function to extract headers from XML and assign to call dial info.
 *
 *  \param callDialInfo - The call dial info struct to fill
 *  \param node - <Headers> node
 * 
 *	\retval stiHResult
 */
stiHResult terpStructHeadersLoad(SstiCallDialInfo& callDialInfo, IXML_Node* node)
{
	stiHResult hResult{ stiRESULT_SUCCESS };
	const char* nodeName{ nullptr };
	IXML_NodeList* nodeListHead{ nullptr };
	IXML_NodeList* nodeListPtr{ nullptr };
	std::vector<SstiSipHeader> headers{};

	// make sure this is <Headers> node
	nodeName = ixmlNode_getNodeName(node);
	stiTESTCONDMSG(nodeName, stiRESULT_ERROR, "XML Node missing name");
	stiTESTCONDMSG(nodeName == g_headers, stiRESULT_ERROR, "Tried loading headers from wrong XML node. Expected '%s', Actual '%s'", g_headers.c_str(), nodeName);

	// start fresh -> make sure extra headers only set if xml successfully parsed
	callDialInfo.additionalHeaders.clear(); // should this be done before header name check?

	// iterate each <HeaderItem>
	nodeListHead = ixmlNode_getChildNodes(node);
	nodeListPtr = nodeListHead;
	while (nodeListPtr && nodeListPtr->nodeItem)
	{
		IXML_Node* item = nodeListPtr->nodeItem;
		nodeName = ixmlNode_getNodeName(item);
		if (!nodeName || nodeName != g_headerItem)
		{
			nodeListPtr = nodeListPtr->next;
			continue; // not a <HeaderItem> node -> skip
		}

		// get what should be <Name> and <Body>
		IXML_Node* node1 = ixmlNode_getFirstChild(item);
		const char* node1Name = ixmlNode_getNodeName(node1);
		IXML_Node* node2 = ixmlNode_getNextSibling(node1);
		const char* node2Name = ixmlNode_getNodeName(node2);

		// do we have name and body
		if (node1Name && node2Name && node1Name == g_headerItemName && node2Name == g_headerItemBody)
		{	// yes
			SstiSipHeader header;
			header.name = nodeTextValueGet(node1);
			header.body = nodeTextValueGet(node2);
			headers.push_back(std::move(header));
		}
		// what about body and name
		else if (node1Name && node2Name && node1Name == g_headerItemBody && node2Name == g_headerItemName)
		{ 	// yes
			SstiSipHeader header;
			header.name = nodeTextValueGet(node2);
			header.body = nodeTextValueGet(node1);
			headers.push_back(std::move(header));
		}
		else
		{
			hResult = stiRESULT_ERROR;
			stiTESTRESULTMSG("HeaderItem missing Body and/or Name");
		}

		nodeListPtr = nodeListPtr->next;
	}
	nodeListPtr = nullptr;
	ixmlNodeList_free(nodeListHead);
	callDialInfo.additionalHeaders = std::move(headers);

STI_BAIL:
	return hResult;
}
