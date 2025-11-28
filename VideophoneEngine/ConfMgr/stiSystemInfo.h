////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Module Name:	stiSystemInfo
//
//	File Name:	stiSystemInfo.h
//
//	Owner:		Eugene Christensen
//
//	Abstract:
//		See stiSystemInfo.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STISYSTEMINFO_H
#define STISYSTEMINFO_H

//
// Includes
//
#include "stiDefs.h"
#include "stiSVX.h"
#include "stiError.h"
#include "stiProductCompatibility.h"

//
// Constants
//

const char * const stiINFO_HEADER_CSTR = "SInfo:";

// 
// Typedefs
//

//
// Forward Declarations
//
class CstiCall;
class CstiSipCallLeg;

//
// Globals
//

//
// Function Prototypes
//
stiHResult SystemInfoDeserialize (
    CstiCallSP call,
	const char *pszString);

stiHResult SystemInfoDeserialize (
    CstiSipCallLegSP callLeg,
	const char *pszString);

stiHResult SystemInfoSerialize (
	std::string *transmitString,				///< This new string will contain the serialized data
    CstiSipCallSP sipCall, 		///< The call containing directory-resolved information
	bool bDefaultProviderAgreementSigned, 	///< Has this agreement been signed?
	EstiInterfaceMode eLocalInterfaceMode, 	///< The local interface mode.
	const char *pszAutoDetectedPublicIp, 	///< The auto detected public IP address
	EstiVcoType ePreferredVCOType,			///< The type of VCO to use: 0 = none, 1 = 1-line, 2 = 2-line
	const char *pszVCOCallbackNumber,		///< The VCO callback number
	bool bVerifyAddress,					///< Does the address need to be verified (e.g. for 911 calls)?
	const char *pszProductName,
	const char *pszProductVersion,
	int nSorensonSIPVersion,
	bool bBlockCallerID,
	EstiAutoSpeedMode eAutoSpeedSetting);



#endif // STISYSTEMINFO_H
// End file stiSystemInfo.h
