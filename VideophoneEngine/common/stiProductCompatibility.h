////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	stiProductCompatibility
//
//  File Name:	stiProductCompatibility.h
//
//  Owner:		Eugene R. Christensen
//
//	Abstract:
//		This file contains those enumerations, data structures and constants
// 		that enable the compatibility between Sorenson products.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STIPRODUCTCOMPATIBILITY_H
#define STIPRODUCTCOMPATIBILITY_H

//
// Includes
//
#include "stiDefs.h"

//
// Constants
//
const char szstiSRA[] = "SRA";
const char szstiSRV1[] = "SRV1";	// NOTE:  Due to implementation, this must
									// always be four bytes in length.
const char szstiSRV3[] = "SRV3";	// NOTE:  Due to implementation, this must
									// always be four bytes in length.
const char szstiSPTZ[] = "SPTZ";
const char szstiSRV1_ACTIVE[] = "SRV1_ACTIVE";

const char szstiFLASH_LIGHTRING[] = "FLASH_LIGHTRING";

const char szstiVIDEO_COMPLETE[] = "VIDEO_COMPLETE";

enum EstiSorensonMessage
{
	// **IMPORTANT!!** DO NOT change the values of these enumerations!! If more
	// are needed, they must be added at the end of the list! If one value is no
	// longer used, just put on a comment on the line indicating that it is
	// unused.
	estiSM_UNKNOWN = 0,

	// Messages
	estiSM_IS_HOLDABLE_MESSAGE = 1,

	// Indications
	estiSM_VIDEO_COMPLETE_INDICATION = 2,

	// Commands
	estiSM_FLASH_LIGHT_RING_COMMAND = 3,
	estiSM_HOLD_COMMAND = 4,
	estiSM_RESUME_COMMAND = 5,

	estiSM_IS_PTZABLE_MESSAGE = 6, // No longer using remote camera control feature.
	estiSM_IS_TRANSFERABLE_MESSAGE = 7,
	estiSM_CALL_TRANSFER_COMMAND = 8,
	estiSM_DELAYED_SINFO = 9,
	estiSM_CLEAR_DTMF = 10,
	estiSM_CLEAR_TEXTSHARE = 11, // indicates to receiver to clear text from both sides
	estiSM_CLEAR_REMOTETEXTSHARE = 12, // indicates to receiver to clear text from remote side
	estiSM_CLEAR_LOCALTEXTSHARE = 13, // indicates to receiver to clear text they sent

	// ^^^ Add new items above this line!
	estiSM_ZZZZ_NO_MORE
};


const char szPRODUCT_FAMILY_NAME[] = "Sorenson Videophone";


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
// Class Declaration
//


#endif // STIPRODUCTCOMPATIBILITY_H
// end file stiProductCompatibility.h
