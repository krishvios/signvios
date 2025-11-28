/*!
* \file IMediaServerEngine.h
* \brief An Interface for the Media Server Engine
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once
#include "stiSVX.h"
#include "stiConfDefs.h"
#include "IstiCall.h"

//Constants
static const int T35_COUNTRY_CODE = 181;  // United States of America
static const int T35_MFG_CODE = 21334;    // Sorenson Communications' assigned code
static const int MaxCalls = 50; //Maximum number of concurent calls.

//
// Forward Declarations
//
class IMediaServerEngine;

/*
	Create an instance of the Media Server Engine.
*/
IMediaServerEngine * CreateMediaServerEngine(PstiObjectCallback appCallback, size_t callbackParam);

void DestroyMediaServerEngine(IMediaServerEngine * engine);



class IMediaServerEngine
{
public:
	/*!
	// \brief Initializer for MediaServerEngine
	//
	// \param productName the name of the product
	// \param version the version of the product
	// \param conferenceParams the configuration of the conference manager
	// \param conferencePorts the ports used by the conference manager
	//
	// \return stiRESULT_SUCCESS
	*/
	virtual stiHResult initialize(	ProductType productType,
									const char *version,
									const SstiConferenceParams *conferenceParams,
									const SstiConferencePorts *conferencePorts) = 0;

	/*!
	// \brief Removes a call object, should be called when the UI is finished with the call object
	//
	// \param call: the call object to remove
	//
	// \return stiRESULT_SUCCESS
	*/
	virtual stiHResult callObjectRemove(IstiCall * call) = 0;

	/*!
	// \brief Returns the number of call objects in the conference manager.
	//
	// \return Number of Call Objects
	*/
	virtual int callObjectsCountGet() = 0;

	/*!
	// \brief Shutsdown the conference manager waiting for it to finish shutting down.
	//
	// \return stiRESULT_SUCCESS
	*/
	virtual stiHResult shutdown() = 0;

	/*!
	// \brief Reinvite a CAll
	//
	// \return stiRESULT_SUCCESS
	*/
	virtual stiHResult reinvite (IstiCall * call) = 0;
};


