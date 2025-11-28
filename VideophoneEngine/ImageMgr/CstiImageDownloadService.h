/*!
 * \file CstiImageDownloadService.h
 * \brief See CstiImageDownloadService.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef CSTIIMAGEDOWNLOADSERVICE_H
#define	CSTIIMAGEDOWNLOADSERVICE_H

#include "CstiEvent.h"
#include "CstiOsTaskMQ.h"

#include "stiEventMap.h"

class CstiHTTPService;

class CstiImageDownloadService: public CstiOsTaskMQ
{
public:
	/// nonstandard events this class handles
	enum EEventType
	{
		estiDOWNLOAD_IMAGE = estiEVENT_NEXT,
	};

	enum EMessageType
	{
		estiMSG_IMAGE_DOWNLOADED = estiMSG_NEXT,
		estiMSG_IMAGE_DOWNLOAD_ERROR
	};

	CstiImageDownloadService (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam);

	~CstiImageDownloadService () override = default;

	stiHResult Startup () override;
	virtual stiHResult Initialize ();

	/*!
	 * \brief Sets up a proxy server for contact photo download requests.
	 *
	 * \param host hostname or address of the proxy server or empty for none
	 * \param port port of the proxy server (ignored when host is empty)
	 *
	 * \return stiHResult
	 */
	stiHResult ProxyServerSet(const std::string& host, uint16_t port);

	stiHResult Schedule(
		const std::string& local_save_path,
		const std::string& remote_url,
		const std::string& alternate_remote_url,
		unsigned int* request_id);

private:
	EstiBool m_bInitialized{estiFALSE};

	unsigned int m_NextRequestId{1};
	stiMUTEX_ID m_NextRequestIdMutex{nullptr};

	std::string m_ProxyServerAddress;
	uint16_t m_ProxyServerPort{0};

	CstiHTTPService *m_pDownloadService{nullptr};

	stiDECLARE_EVENT_MAP(CstiImageDownloadService);
	stiDECLARE_EVENT_DO_NOW(CstiImageDownloadService);

	using CstiOsTaskMQ::ShutdownHandle;
	stiHResult stiEVENTCALL ShutdownHandle(void *poEvent);
	stiHResult stiEVENTCALL DownloadImage(void *poEvent);
	unsigned int NextRequestId();
	stiHResult EstablishImageServerConnection(const char* pServerAddress);

	void Uninitialize();
	int Task () override;
};

#endif
