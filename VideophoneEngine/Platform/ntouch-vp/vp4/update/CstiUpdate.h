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

//#include <iostream>
#include <vector>


//
// aktualizr includes
//
#include "aktualizr-lite/api.h"
//#include "libaktualizr/aktualizr.h"
//#include "libaktualizr/config.h"
//#include "libaktualizr/events.h"

//
// Constants
//

//
// Typedefs
//

//
// Globals
//

//class INvStorage;
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
	void update (const std::string &url) override;

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

	EState m_eState {eUNINITIALIZED};
	
	stiHResult aktualizrInitialize ();
	stiHResult aktualizrLoggerLevelSet ();
	
	//void aktualizrSignalHandler (
	//	const std::shared_ptr<event::BaseEvent> &event);
	
	void eventUpdateCheck ();
	//void eventAkliteRun ();
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
	std::unique_ptr<AkliteClient> m_akliteClient = nullptr;
	std::unique_ptr<InstallContext> m_installContext = nullptr;
	//boost::log::filter m_originalCoreFilter;

	//std::vector<boost::filesystem::path> m_configDirs {"/usr/lib/sota/conf.d", "/var/ntouchvp4/sota/conf.d/"};

	std::map<std::string, unsigned int> m_progress;
	
	unsigned int m_previousProgress {0};
	unsigned int m_newProgress {0};
		
	int m_nPauseCount {0};
};

// end file CstiUpdate.h
