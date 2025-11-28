////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: stiSipTools
//
//  File Name:  stiSipTools.h
//
//  Abstract:
//	See stiSipTools.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTISIPTOOLS_H
#define CSTISIPTOOLS_H


//
// Forward Declarations
//

//
// Includes
//
#include "stiSVX.h"
#include "CstiRoutingAddress.h"
#include "stiSipConstants.h"
#include "Sip/Message.h"
#include "RvSipStackTypes.h"
#include "RvSipStack.h"
#include "rvsdp.h"
#include "CstiSdp.h"

//
// Constants
//
extern const int g_nSVRSSIPVersion;


struct viaHeader
{
	RvSipTransport transport{RVSIP_TRANSPORT_UNDEFINED};
	std::string hostAddr;
	int hostPort{UNDEFINED};
	std::string branch;
	std::string receivedAddr;
	int rPort{UNDEFINED};
};

//
// Typedefs
//
//
// Forward Declarations
//

//
// Globals
//

stiHResult sipMessageGet (
	RvSipMsgHandle hMsg,
	RV_LOG_Handle hLog,
	std::string *message);

stiHResult sipAddrGet (
	RvSipAddressHandle hAddress,
	HRPOOL hPool,
	std::string *header);

std::string AddressStringCreate (
	const char *pszLocation,
	const char *pszUser,
	unsigned int nPort,
	EstiTransport eTransport,
	const char *pszProtocol);

std::string AddressHeaderStringCreate (
	const std::string &headerType,
	const std::string &address,
	const std::string &displayName = "",
	float fOrder = -1.0F,
	float fExpiration = -1.0F);

stiHResult ContactHeaderAttach (RvSipMsgHandle hMsg, const char *pszContactString);
stiHResult ContactHeaderAttach (RvSipMsgHandle hMsg, RvSipContactHeaderHandle hOriginalContact);
stiHResult AddressPrint (HRPOOL hPool, RvSipAddressHandle hAddress);
bool AddressCompare (RvSipAddressHandle hAddress1, RvSipAddressHandle hAddress2);
void EscapeDisplayName (const char *pszEscapedName, std::string *pResult);
std::string AddPlusToPhoneNumber (const std::string &uri);

void SdpPrint (const CstiSdp &Sdp);

void MethodInAllowVerify(
	const vpe::sip::Message &message,
	vpe::sip::Message::Method method,
	bool *pbAllowHeaderFound,
	bool *pbAllowTypeFound);

void InfoPackageVerify (
	RvSipMsgHandle hMsg,
	const char *pszInfoPackage,
	bool *pbInfoPackageHeaderFound,
	bool *pbInfoPackageFound);

RvSdpMediaType RadvisionMediaTypeGet (EstiMediaType eMediaType);
EstiMediaType SorensonMediaTypeGet (RvSdpMediaType eSdpMediaType, const char *pszMediaType);

stiHResult EncryptString (
	const std::string &inputString,
	std::string *encryptedString);

stiHResult DecryptString (
	const unsigned char *pszEncryptedString,
	unsigned int encryptedLength,
	std::string *pDecryptedString);

void SipUriCreate (
	const std::string &user,
	const std::string &domain,
	std::string *pURI);

viaHeader viaHeaderGet (
	RvSipMsgHandle message);

std::string dhvUriCreate (
	const std::string &uri,
	const std::string &participantType);

void UnescapeDisplayName (const char *pszEscapedName, std::string *pResult);

//
// Class Declaration
//


#endif // CSTISIPTOOLS_H
// end file stiSipTools.h
