////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	File Name:	CstiVRCLCommon.h

//
//	Abstract:
// 		Contains common defines used by VRCL.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIVRCLCOMMON_H
#define CSTIVRCLCOMMON_H

//
// Includes
//
#include "ixml.h"
#include "stiDefs.h"
#include "stiConfDefs.h"
#include "stiSVX.h"
#include "OptString.h"
#include <vector>
#include "utf8EncodeDecode.h"

//
// Constants
//
extern const char g_szAPIVersion[];

extern const char g_szCallHangupMessage[];
extern const char g_szCallAnswerMessage[];
extern const char g_szCallRejectMessage[];
extern const char g_szIsAliveMessage[];
extern const char g_szIsCallTransferableMessage[];
extern const char g_szStatusCheckMessage[];
extern const char g_szRebootMessage[];
extern const char g_szApplicationVersionGetMessage[];
extern const char g_szDeviceTypeGetMessage[];
extern const char g_szCallBandwidthDetectionBegin[];
extern const char g_szInCallStatsGet[];

// VRCL constant strings
extern const char g_szEnableSignMailRecording[] ;
extern const char g_szSignMailRecordingEnabled[];  // Confirm that SignMailRecording has been enabled
extern const char g_szSignMailAvailable[];  // Inform the client that SignMail is available at the remote device
extern const char g_szSignMailRecordingStarted[];  // Inform the client that Signmail recording has started
extern const char g_szSignMailGreetingStarted[];
extern const char g_szSkipToSignMailRecord[];
extern const char g_szDtmfSend[];
extern const char g_szTextBackspaceSend[];
extern const char g_szTextMessageSend[];
extern const char g_szTextMessageClear[];
extern const char g_szTextMessageCleared[];
extern const char g_szExternalConferenceDialMessage[];
extern const char g_szExternalCameraUseMessage[];
extern const char g_szPhoneNumberDialMessage[];
extern const char g_szCallId[];
extern const char g_szTerpId[];
extern const char g_szHearingNumber[];
extern const char g_szHearingName[];
extern const char g_szDestNumber[];
extern const char g_szAlwaysAllow[];
extern const char g_szLoggedInPhoneNumberGet[];
extern const char g_szTerpNetModeSet[];
extern const char g_szUpdateCheck[];
extern const char g_szUpdateCheckResult[];
extern const char g_szCallIdSet[];
extern const char g_szCallIdSetResult[];
extern const char g_szDiagnosticsGet[];
extern const char g_szCount[];
extern const char g_szTextInSignMail[];
extern const char g_szSignMailText[];
extern const char g_szStartSeconds[];
extern const char g_szEndSeconds[];
extern const char g_hearingVideoCapability[];
extern const char g_hearingNumber[];
extern const char g_hearingVideoCapable[];
extern const char g_hearingVideoConnected[];
extern const char g_hearingVideoConnectionFailed[];
extern const char g_hearingVideoDisconnected[];
extern const char g_hearingVideoDisconnectedByHearing[];
extern const char g_hearingVideoDisconnectedByDeaf[];
extern const char g_hearingVideoConnecting[];
extern const char g_hearingVideoConnectingConfURI[];
extern const char g_hearingVideoConnectingInitiator[];
extern const char g_hearingVideoConnectingProfileId[];

// extern const char g_szCallBridgeDial[];
extern const char g_szCallBridgeDisconnect[];
extern const char g_szAllowedAudioCodecs[];
extern const char g_szLoggedInPhoneNumber[];
extern const char g_szVcoPref[];
extern const char g_szVcoActive[];

extern const char g_szAnnounceClear[];
extern const char g_szAnnounceSet[];

extern const char g_szTrue[];
// extern const char g_szFalse[];

extern const char g_szSuccess[];
extern const char g_szFail[];
extern const char g_szInCall[];
extern const char g_szAppVersionOK[];

extern const char g_szHearingStatus[];
extern const char g_szRelayNewCallReady[];
extern const char g_szHearingStatusResponse[];
extern const char g_szRelayNewCallReadyResponse[];
extern const char g_szHearingCallSpawn[];
extern const char g_szGeolocation[];
extern const char g_szConferenceURI[];
extern const char g_szConferenceType[];
extern const char g_szPublicIP[];
extern const char g_szSecurityToken[];
extern const char g_szApplicationCapabilitiesGetMessage[];
extern const char g_szInUse[];

//
// Typedefs
//
struct SstiCallDialInfo
{
	EstiDialMethod eDialMethod {estiDM_BY_DS_PHONE_NUMBER};
	std::string CallId;
	std::string TerpId;
	std::string HearingNumber;
	OptString hearingName;  // OptString so we can differentiate between empty and unset
	std::string DestNumber;
	std::string RelayLanguage;
	bool bAlwaysAllow{false};
	std::vector<SstiSipHeader> additionalHeaders{};
};

struct SstiTextInSignMailInfo
{
	std::string signMailText;
	std::string startSeconds;
	std::string endSeconds;
};

//
// Forward Declarations
//

stiHResult AddAttribute(
	IXML_Element *pXmlElement, 
	const char *pszAttrName, 
	const char *pszAttrValue);

stiHResult AddXmlElement(
	IXML_Document *pXmlDoc, 
	IXML_Node *pRootNode, 
	IXML_Element **ppXmlElement, 
	const char *pszElementName,
	const char *pszAttr1Name, 
	const char *pszAttr1Value,
	const char *pszAttr2Name,
	const char *pszAttr2Value);

stiHResult AddXmlElementAndValue (
	IXML_Document *pXmlDoc, 
	IXML_Node *pRootNode, 
	const char *pszElementName,
	const char *pszValue);

void CleanupXmlDocument(
	IXML_Document *pXmlDoc);

stiHResult CreateXmlDocumentFromString(
	const char *pszXmlString,
	IXML_Document **ppXmlDoc);

stiHResult GetAttrValueString(
	IXML_Element *pXmlElement,
	const char *pszAttrName,
	const char **ppszAttrValue);

stiHResult GetChildNodeType(
	IXML_Node *pXmlNode,
	char **ppszNodeType,
	IXML_Node **ppChildNode);

stiHResult GetChildNodeNameAndValue(
	IXML_Node *pXmlNode,
	std::string &nodeName,
	std::string &nodeValue,
	IXML_Node **ppChildNode);

stiHResult GetNextChildNodeNameAndValue(
	IXML_Node *pXmlNode,
	std::string &nodeName,
	std::string &nodeValue,
	IXML_Node **ppChildNode);

stiHResult GetXmlString(
	IXML_Node *pXmlNode,
	char **ppszXmlString);

stiHResult InitXmlDocument(
	IXML_Document **ppXmlDoc,
	IXML_Node **ppRootNode,
	const char *pszNodeType);

stiHResult GetTextNodeValue (
	IXML_Node *pXmlNode,
	std::string &nodeValue);

stiHResult GetTextNodeValueWide (
	IXML_Node *pXmlNode,
	uint16_t **ppwszNodeValue);

int GetNodeChildCount (
	IXML_Node *pXmlNode);

stiHResult phoneNumberDialNodeProcess (
	IXML_Node* node,
	SstiCallDialInfo& callDialInfo
);

std::string nodeTextValueGet (
	IXML_Node* node
);

void terpStructLoad (
	SstiCallDialInfo& callDialInfo,
	const std::string& nodeName,
	const std::string& nodeValue
);

stiHResult terpStructHeadersLoad (
	SstiCallDialInfo& callDialInfo,
	IXML_Node* node
);


//
// Globals
//



#endif // CSTIVRCLCOMMON_H
// end file CstiVRCLCommon.h
