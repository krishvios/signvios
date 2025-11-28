/*!
* \file MediaServerEngine.cpp
* \brief Engine for Media Server that starts up Conference Manager
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#include "MediaServerEngine.h"

/*!
// \brief Constructor
//
// \paramapp callback function pointer for callback events
// \param callbackParam object returned to function pointer
*/

MediaServerEngine::MediaServerEngine(PstiObjectCallback callback, size_t callbackParam)
{
	applicationCallback = callback;
	applicationCallbackParam = callbackParam;
	conferenceManager = new CstiConferenceManager();

	m_signalConnections.push_back (conferenceManager->incomingCallSignal.Connect (
		[this](CstiCallSP call)
	{
		applicationCallback (estiMSG_INCOMING_CALL, (size_t)call.get (), applicationCallbackParam, 0);
	}));

	m_signalConnections.push_back (conferenceManager->disconnectedSignal.Connect (
		[this](CstiCallSP call)
	{
		applicationCallback (estiMSG_DISCONNECTED, (size_t)call.get (), applicationCallbackParam, 0);
	}));

	m_signalConnections.push_back (conferenceManager->remoteTextReceivedSignal.Connect (
		[this](SstiTextMsg* message)
	{
		applicationCallback (estiMSG_REMOTE_TEXT_RECEIVED, (size_t)message, applicationCallbackParam, 0);
	}));
}

/*!
// \brief Destructor
*/
MediaServerEngine::~MediaServerEngine()
{
	if (conferenceManager)
	{
		delete conferenceManager;
	}
}


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
stiHResult MediaServerEngine::initialize(ProductType productType,
	const char *version,
	const SstiConferenceParams *conferenceParams,
	const SstiConferencePorts *conferencePorts)
{
	if (conferenceManager)
	{
		stiHResult hResult = conferenceManager->Initialize(
			0,  // SCI-ALA Disable blocklistManager - the Hold Server does not need this
			productType,
			version,
			conferenceParams,
			conferencePorts);
		hResult = conferenceManager->MaxCallsSet(MaxCalls);
		hResult = conferenceManager->AutoRejectSet(estiOFF);
		conferenceManager->LocalInterfaceModeSet(estiSTANDARD_MODE);
		hResult = conferenceManager->LocalDisplayNameSet("SignMail");
		hResult = conferenceManager->Startup();

		return hResult;
	}
	return stiMAKE_ERROR(stiRESULT_ERROR);
}

/*!
// \brief Removes a call object, should be called when the UI is finished with the call object
//
// \param call: the call object to remove
//
// \return stiRESULT_SUCCESS
*/
stiHResult MediaServerEngine::callObjectRemove(IstiCall * call)
{
	if (call && conferenceManager)
	{
		return conferenceManager->CallObjectRemove(static_cast<CstiCall*>(call));
	}
	return stiMAKE_ERROR(stiRESULT_ERROR);
}

/*!
// \brief Returns the number of call objects in the conference manager.
//
// \return Number of Call Objects
*/
int MediaServerEngine::callObjectsCountGet()
{
	if (conferenceManager)
	{
		conferenceManager->CallObjectsCountGet();
	}
	return 0;
}

/*!
// \brief Shutsdown the conference manager waiting for it to finish shutting down.
//
// \return stiRESULT_SUCCESS
*/
stiHResult MediaServerEngine::shutdown()
{
	if (conferenceManager)
	{
		stiHResult hResult = conferenceManager->Shutdown();
		return hResult;
	}
	return stiMAKE_ERROR(stiRESULT_ERROR);
}

stiHResult MediaServerEngine::reinvite (IstiCall * call)
{
	call->Hold ();
	call->Resume ();
	return stiRESULT_SUCCESS;
}

/*!
// \brief Creates an IMediaServerEngine
//
// \param appCallback function pointer for callback events
// \param callbackParam object returned to function pointer
//
// \return stiRESULT_SUCCESS
*/
IMediaServerEngine * CreateMediaServerEngine(PstiObjectCallback appCallback, size_t callbackParam)
{
	return new MediaServerEngine(appCallback, callbackParam);
}

/*!
// \brief deletes the media server engine
//
*/
void DestroyMediaServerEngine(IMediaServerEngine *pEngine)
{
	delete pEngine;
}
