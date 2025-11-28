////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	stiXMLCircularBuffer
//
//  File Name:	CstiXMLCircularBuffer.h
//
//  Owner:		Justin Ball
//
//	Abstract:
//      This class encapsulates a circular buffer with XML capabilities

//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIXMLCIRCULARBUFFER_H
#define CSTIXMLCIRCULARBUFFER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//
// Includes
//
#include "stiDefs.h"
#include "CstiXML.h"
#include "CstiCircularBuffer.h"

//
// Constants
//

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

class CstiXMLCircularBuffer : 
	public CstiCircularBuffer, 
	public CstiXML  
{
public:

	CstiXMLCircularBuffer () = default;
	~CstiXMLCircularBuffer () override = default;

	stiHResult GetNextXMLTag (
		char * szTag,
		uint32_t un32TagBufferSize,
		uint32_t & un32TagSize,
		EstiBool & bTagFound,
		char ** pchData,
		uint32_t & un32DataSize,
		EstiBool & bInvalidTag);


private:

};

#endif // CSTIXMLCIRCULARBUFFER_H
