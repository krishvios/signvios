////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiURLResolve
//
//  File Name:	CstiURLResolve.h
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:
//		
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

//
// Includes
//
#include "CstiEventQueue.h"
#include "stiSVX.h"
#include "CstiHTTPService.h"
#include <string>
#include <vector>

//
// Constants
//

//
// Typedefs
//

struct SHTTPPostServiceData
{
	SHTTPPost postInfo;
	CstiHTTPService * poHTTPService{nullptr};
	std::vector<std::string> ResolvedServerIPList;
	unsigned short nPort{0};
	std::string File;
};

//
// Forward Declarations
//
class CstiHTTPTask;

//
// Globals
//

//
// Class Declaration
//
class CstiURLResolve : public CstiEventQueue
{
public:

	CstiURLResolve () = delete;
	~CstiURLResolve () override = default;

	CstiURLResolve (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam,
		CstiHTTPTask *pHTTPTask);
	
	CstiURLResolve (const CstiURLResolve &other) = delete;
	CstiURLResolve (CstiURLResolve &&other) = delete;
	CstiURLResolve &operator= (const CstiURLResolve &other) = delete;
	CstiURLResolve &operator= (CstiURLResolve &&other) = delete;
	
	void httpUrlResolve (
		SHTTPPostServiceData *poServiceData);

private:

	CstiHTTPTask *m_pHTTPTask;
};
