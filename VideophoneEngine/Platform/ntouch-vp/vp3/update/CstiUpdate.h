///
/// \file CstiUpdate.h
/// \brief Declaration of the Update class for ntouch
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
///
#pragma once

//
// Includes
//
#include "CstiEventQueue.h"
#include "IUpdate.h"


//
// aktualizr includes
//

#include "bootstrap/bootstrap.h"
#include "config/config.h"
#include "logging/logging.h"
#include "primary/aktualizr.h"
#include "utilities/utils.h"
#include <curl/curl.h>

//
// Constants
//

//
// Typedefs
//
typedef std::map<std::string, std::string> http_headers;

//
// Globals
//

//
// Class Declaration
//
class CstiUpdate : public IUpdate, public CstiEventQueue
{
public:
	
	CstiUpdate ();
	
	CstiUpdate (const CstiUpdate &other) = delete;
	CstiUpdate (CstiUpdate &&other) = delete;
	CstiUpdate &operator= (const CstiUpdate &other) = delete;
	CstiUpdate &operator= (CstiUpdate &&other) = delete;

	~CstiUpdate () override;
	
	stiHResult Startup ();
	
	stiHResult Initialize ();

	//
	// Methods called by the Application
	//
	void updateDownload () override;
	void updateInstall () override;
	void update (const std::string &url);
	
	void pause () override;

	void resume () override;

private:

	/*!
	* \enum EState
	*
	* \brief Update progress states
	*
	*/
	enum EState
	{
		eUNINITIALIZED,
		eIDLE,
		ePAUSED,
		eDOWNLOADING,
		ePROGRAMMING,
		eUPDATE_COMPLETE,
		eERROR
	}; // end EState
	
	//
	// Public Access Methods
	//
	EState stateGet ();
	
	//
	// Utility Methods
	//
	void stateSet (
		EState eState);
	
	stiHResult aktualizrInitialize ();
	
	void aktualizrSignalHandler (
		const std::shared_ptr<event::BaseEvent> &event);
	
	void eventOTAServerCheck ();
	void eventSendDeviceData ();
	void eventUpdateDownload ();
	void eventUpdateInstall ();
	void eventNetworkDisconnected ();
	
	CstiSignalConnection::Collection m_signalConnections;
	
	std::mutex m_aktualizrMutex;
	
	void downloadThreadTask ();
	bool m_runDownloadThread {false};
	
	void installThreadTask ();
	bool m_runInstallThread {false};
	
	static constexpr const char *sorensonMacPrefix = "000872";
	boost::signals2::connection m_aktualizrConnection;
	std::unique_ptr<Aktualizr> m_aktualizr = nullptr;
	std::vector<boost::filesystem::path> m_configDirs {"/usr/lib/sota/conf.d", "/etc/sota/conf.d/", "/var/ntouchvp3/sota/conf.d/"};
	result::Download m_downloadResult;
	
	std::map<std::string, unsigned int> m_progress;
	
	bool m_bAllDownloadsComplete {false};
	
	EState m_eState {eUNINITIALIZED};
	
	unsigned int m_previousProgress {0};
	unsigned int m_newProgress {0};
		
	int m_nPauseCount {0};

};

// end file CstiUpdate.h
