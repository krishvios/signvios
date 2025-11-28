///
/// \file CstiUpdate.cpp
/// \brief Definition of the Update class for ntouch
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///

//
// Includes
//
#include <cctype>  // standard type definitions.
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <regex>
#include <map>
#include <string>
#include <vector>
#include <fcntl.h>
#include <glib.h>

#include "stiOS.h"          // OS Abstraction
#include "CstiUpdate.h"
#include "stiDebugTools.h"
#include "stiTools.h"
#include "stiTaskInfo.h"
#include "IstiPlatform.h"
#include "IPlatformVP.h"
#include "IstiNetwork.h"


//
// Constants
//

//
// Typedefs
//

//
// Globals
//

//
// Function Definitions
//

//
// Function Declarations
//

///
/// \brief Constructor
///
CstiUpdate::CstiUpdate ()
	:
	CstiEventQueue("CstiUpdate")
{
	// Reset m_downloadResult
	m_downloadResult.status = result::DownloadStatus::kError;
	m_downloadResult.updates.clear ();
} // end CstiUpdate::CstiUpdate


///
/// \brief Destructor
///
CstiUpdate::~CstiUpdate ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::~CstiUpdate\n");
	);
	
	if (m_aktualizr)
	{
		m_aktualizr->Abort ();
		m_aktualizr->Shutdown ();
		m_aktualizr.reset ();
	}
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::pause: m_nPauseCount = ", m_nPauseCount, "\n");
		vpe::trace ("CstiUpdate::pause: m_runDownloadThread = ", m_runDownloadThread, "\n");
		vpe::trace ("CstiUpdate::pause: m_runInstallThread = ", m_runInstallThread, "\n");
	);
	
	CstiEventQueue::StopEventLoop ();

} // end CstiUpdate::~CstiUpdate

///
/// \brief Initialization method for the class
///
stiHResult CstiUpdate::Initialize ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::Initialize\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	stateSet (eIDLE);
	
	m_signalConnections.push_back (IstiNetwork::InstanceGet ()->wiredNetworkDisconnectedSignalGet ().Connect (
		[this]() {
			PostEvent (std::bind(&CstiUpdate::eventNetworkDisconnected, this));
	}));

	m_signalConnections.push_back (IstiNetwork::InstanceGet ()->wirelessNetworkDisconnectedSignalGet ().Connect (
		[this]() {
			PostEvent (std::bind(&CstiUpdate::eventNetworkDisconnected, this));
	}));
	
	return (hResult);

} // end CstiUpdate::Initialize

stiHResult CstiUpdate::Startup ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::Startup\n");
	);
	
	StartEventLoop ();
	PostEvent([this]{ eventOTAServerCheck (); });
	PostEvent([this]{ eventSendDeviceData (); });
	
	return stiRESULT_SUCCESS;
}

void aktualizrLoggerLevelSet ()
{
	LoggerConfig logger;
		
	switch (g_stiUpdateDebug.valueGet ())
	{
		case 0:
		{
			logger.loglevel = 3;
			break;
		} // end case 0
		
		case 1:
		{
			vpe::trace ("aktualizrLoggerLevelSet: Changing update loglevel to 1\n");
			logger.loglevel = 1;
			break;
		} // end case 1
		
		case 2:
		{
			vpe::trace ("aktualizrLoggerLevelSet: Changing update loglevel to 0\n");
			logger.loglevel = 0;
			break;
		} // end case 2
		
		default:
		{
			vpe::trace ("aktualizrLoggerLevelSet: g_stiUpdateDebug is unkown, changing update loglevel to 3\n");
			logger.loglevel = 3;
			break;
		} // end case default
		
	} // end switch
			
	logger_set_threshold(logger);
}

void CstiUpdate::updateDownload ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::updateDownload\n");
	);
	
	PostEvent ([this] {eventUpdateDownload ();}); 
}

void CstiUpdate::updateInstall ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::updateInstall\n");
	);
	
	PostEvent ([this] {eventUpdateInstall ();}); 
}

void CstiUpdate::update (const std::string &url)
{
	stiASSERTMSG (false, "This update method is not supported by this platform");
}

void CstiUpdate::aktualizrSignalHandler (
	const std::shared_ptr<event::BaseEvent> &event)
{
	if (event->isTypeOf<event::DownloadProgressReport>())
	{
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::aktualizrSignalHandler: got progress report\n");
		);
		
		const auto download_progress = dynamic_cast<event::DownloadProgressReport *>(event.get());
		
		m_newProgress = download_progress->progress;
		
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::aktualizrSignalHandler: got progress report. progress = ", m_newProgress,"\n");
		);
		
		if (m_newProgress > m_previousProgress)
		{
			m_previousProgress = m_newProgress;
			
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrSignalHandler: Download progress: ", m_newProgress, "\n");
			);
			
			downloadingSignal.Emit (m_newProgress);
		}
	}
	else if (event->variant == "UpdateCheckComplete")
	{
		const auto targets_event = dynamic_cast<event::UpdateCheckComplete *>(event.get());
		
		if (targets_event->result.status == result::UpdateStatus::kUpdatesAvailable)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrSignalHandler: got ", event->variant, " event with status: Updates available\n");
			);
		}
		else
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrSignalHandler: got ", event->variant, " event with status: No updates available\n");
			);
		}
	}
	else if (event->variant == "AllDownloadsComplete")
	{
		const auto downloads_complete = dynamic_cast<event::AllDownloadsComplete *>(event.get());
		
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::aktualizrSignalHandler: got ", event->variant, " event with status: ", downloads_complete->result.status, "\n");
		);
	}
	else if (event->variant == "AllInstallsComplete")
	{
		const auto installs_complete = dynamic_cast<event::AllInstallsComplete *>(event.get());
		
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::aktualizrSignalHandler: got ", event->variant, " event with result code: ", installs_complete->result.dev_report.result_code, "\n");
		);
	}
	else if (event->variant == "PutManifestComplete")
	{
		const auto put_complete = dynamic_cast<event::PutManifestComplete *>(event.get());
		
		if (put_complete->success)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrSignalHandler: PutManifestComplete: success\n");
			);
		}
		else
		{
			stiASSERTMSG (false, "CstiUpdate::aktualizrSignalHandler: PutManifestComplete: failure\n");
		}
	}
	else if (event->variant == "SendDeviceDataComplete")
	{
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::aktualizrSignalHandler: SendDeviceDataComplete\n");
		);
	}
	else
	{
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::aktualizrSignalHandler: got ", event->variant, " event (unknown)\n");
		);
	}
}

void CstiUpdate::eventOTAServerCheck ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::eventOTAServerCheck\n");
	);
	
	// Here we will see if the OTA server in the current credentials zip file
	// matches the OTA server being used from storage in the sql.db
	// We use the root.json in the zip file and the root_image in the storage
	// If they are different then we will remove sql.db so we will
	// have to re-provision to the correct server
	
	std::shared_ptr<INvStorage> storage;
	std::string imageRootFromDatabase;
	std::string imageRootFromZip;
	std::string storagePath;
	bool removeStorage = false;
	
	try
	{
		Config config (m_configDirs);
		
		aktualizrLoggerLevelSet ();
		
		if (m_aktualizr)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventOTAServerCheck: m_aktualizr already created\n");
			);
		}
		else
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventOTAServerCheck: m_aktualizr needs to be created\n");
			);
			
			m_aktualizr = std_::make_unique<Aktualizr>(config);
			stiTESTCOND (m_aktualizr != nullptr, stiRESULT_ERROR);
			
			m_aktualizrConnection = m_aktualizr->SetSignalHandler ([this](const std::shared_ptr<event::BaseEvent>& event){
			aktualizrSignalHandler (event);});
		}
		
		if (m_aktualizr->IsRegistered ())
		{
			// If we are registered then see if we match
			
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventOTAServerCheck: Device is registered.\n");
			);
			
			try
			{
				m_aktualizr->Initialize ();
			}
			catch (std::exception &ex)
			{
				stateSet (eERROR);
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventOTAServerCheck: std::exeption %s.", ex.what ());
			}
			catch (...)
			{
				stateSet (eERROR);
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventOTAServerCheck: Unknown error.");
			}

			storage = INvStorage::newStorage (config.storage);
			storage->importData(config.import);
			
			storage->loadLatestRoot(&imageRootFromDatabase, Uptane::RepositoryType::Image());
			
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("imageRootFromDatabase = ", imageRootFromDatabase, "\n\n\n");
			);
						
			if (!config.provision.provision_path.empty())
			{
				std::ifstream as(config.provision.provision_path.c_str(), std::ios::in | std::ios::binary);
				
				if (as.fail())
				{
					stiDEBUG_TOOL (g_stiUpdateDebug,
						vpe::trace ("CstiUpdate::eventOTAServerCheck: Unable to open provided provision archive ", config.provision.provision_path, ": ", std::strerror(errno), "\n");
					);
					stiTHROWMSG (stiRESULT_ERROR, "Unable to open credentials file.");
				}

				imageRootFromZip = Utils::readFileFromArchive(as, "root.json");
				
				if (imageRootFromZip.empty())
				{
					stiTHROWMSG (stiRESULT_ERROR, "Unable to get root.json form credentials file.");
				}
				else
				{
					stiDEBUG_TOOL (g_stiUpdateDebug,
						vpe::trace ("imageRootFromZip = ", imageRootFromZip, "\n\n\n");
					);
				}
				
				if (imageRootFromZip == imageRootFromDatabase)
				{
					stiDEBUG_TOOL (g_stiUpdateDebug,
						vpe::trace ("imageRootFromZip and imageRootFromDatabase are equal\n\n\n");
					);
				}
				else
				{
					stiDEBUG_TOOL (g_stiUpdateDebug,
						vpe::trace ("imageRootFromZip and imageRootFromDatabase are not equal\n\n\n");
					);
					
					removeStorage = true;
				}
			}
			else
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventOTAServerCheck: Config provision path is empty\n");
			}
		}
		else 
		{
			// If we are not registered dont do anything. We will provision next.
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventOTAServerCheck: Device is not registered. No need to compare root.\n");
			);
			
			m_aktualizr->Abort ();
			m_aktualizr->Shutdown ();
			m_aktualizr.reset ();
		}
		
		
		if (removeStorage)
		{
			// We need to remove the storage so that we re-provision 
			// with the new credentials-server-environment.zip file
			m_aktualizr->Abort ();
			m_aktualizr->Shutdown ();
			m_aktualizr.reset ();
			
			// This should remove /var/sota/sql.db
			storage->cleanUp ();
		}
		
	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventOTAServerCheck: std::exeption %s.", ex.what ());
	}
	catch (...)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventOTAServerCheck: Unknown error.");
	}
	
STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		stateSet (eERROR);
		stiASSERTMSG (false, "CstiUpdate::eventSendDeviceData: Error\n");
	}
	
	return;
}

void CstiUpdate::eventSendDeviceData ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::eventSendDeviceData\n");
	);
	
	try
	{
		// The purpose of the following code is to let the update server know 
		// we have successfully installed after a reboot. otherwise this does not
		// happen until an update is called.
		Config config (m_configDirs);
		
		aktualizrLoggerLevelSet ();
		
		if (m_aktualizr)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventSendDeviceData: m_aktualizr already created\n");
			);
		}
		else
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventSendDeviceData: m_aktualizr needs to be created\n");
			);
			
			m_aktualizr = std_::make_unique<Aktualizr>(config);
			stiTESTCOND (m_aktualizr != nullptr, stiRESULT_ERROR);
			
			m_aktualizrConnection = m_aktualizr->SetSignalHandler ([this](const std::shared_ptr<event::BaseEvent>& event){
			aktualizrSignalHandler (event);});
		}
		
		if (m_aktualizr->IsRegistered ())
		{
			// If we are registered then send device data
			// If this takes too long for initialization we could 
			// move it to a function called after startup
			
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventSendDeviceData: Device is registered. Sending device data.\n");
			);
			
			try
			{
				m_aktualizr->Initialize ();
			}
			catch (std::exception &ex)
			{
				stateSet (eERROR);
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventSendDeviceData: std::exeption %s.", ex.what ());
			}
			catch (...)
			{
				stateSet (eERROR);
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventSendDeviceData: Unknown error.");
			}

			// Asynchronous
			m_aktualizr->SendDeviceData();
		}
		else 
		{
			// If we are not registered dont do anything.
			// Let the register happen when update is called.
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventSendDeviceData: Device is not registered. We will register on first call to update.\n");
			);
			
			m_aktualizr->Abort ();
			m_aktualizr->Shutdown ();
			m_aktualizr.reset ();
		}
		
	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventSendDeviceData: std::exeption %s.", ex.what ());
	}
	catch (...)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventSendDeviceData: Unknown error.");
	}
	
STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		stateSet (eERROR);
		stiASSERTMSG (false, "CstiUpdate::eventSendDeviceData: Error\n");
	}
	
	return;
	
}

stiHResult CstiUpdate::aktualizrInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::aktualizrInitialize\n");
	);
	
	try
	{
		Config config (m_configDirs);

		aktualizrLoggerLevelSet ();

		if (m_aktualizr)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: m_aktualizr already created\n");
			);
		}
		else
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: m_aktualizr needs to be created\n");
			);
		
			m_aktualizr = std_::make_unique<Aktualizr>(config);
			stiTESTCOND (m_aktualizr != nullptr, stiRESULT_ERROR);

			m_aktualizrConnection = m_aktualizr->SetSignalHandler ([this](const std::shared_ptr<event::BaseEvent>& event){
			aktualizrSignalHandler (event);});
		}

		// We don't want to initialize right now because we may not be registered yet
		// If we are not registered then we need to set the device_id and serial
		// before we initialize because then it will register with the correct device_id
		if (!m_aktualizr->IsRegistered ())
		{
			m_aktualizr->Abort ();
			m_aktualizr->Shutdown ();
			m_aktualizr.reset ();
			
			std::string deviceId;
			
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: Device not registered yet.\nThis is probably the first boot or the device has not had a connection to the update server yet.\n");
			);
			
			if (estiOK != stiOSGetUniqueID (&deviceId))
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: Cant get UniqueID\n");
			}
			
			// Remove all ":"
			boost::erase_all (deviceId, ":");

			if (sorensonMacPrefix != deviceId.substr(0, 6))
			{
				vpe::trace ("CstiUpdate::eventUpdateDownload: UniqueID: ", deviceId, "\n");
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: UniqueID not valid\n");
			}

			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: We will now try to register this device with name: ", deviceId, " and serial number: ", deviceId, "\n");
			);
			
			config.provision.device_id = deviceId;
			config.provision.primary_ecu_serial = deviceId;
			config.postUpdateValues ();

			m_aktualizr = std_::make_unique<Aktualizr>(config);
			stiTESTCOND (m_aktualizr != nullptr, stiRESULT_ERROR);

			m_aktualizrConnection = m_aktualizr->SetSignalHandler ([this](const std::shared_ptr<event::BaseEvent>& event){
			aktualizrSignalHandler (event);});

			aktualizrLoggerLevelSet ();
		}

		try
		{
			m_aktualizr->Initialize ();
		}
		catch (std::exception &ex)
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: std::exeption %s. Probably not provisioned yet!", ex.what ());
		}
		catch (...)
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: Unknown error. Probably not provisioned yet!");
		}
		
		// We should be registered by now
		if (!m_aktualizr->IsRegistered ())
		{
			// TODO: If we are not registerd something bad has happened.
			// We should remove /var/sota and somehow let
			// the app know to try again.
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: IsRegistered failed. Try cleaning /var/sota/\n");
		}
	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: std::exeption %s.", ex.what ());
	}

STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		m_aktualizr->Abort ();
		m_aktualizr->Shutdown ();
		m_aktualizr.reset ();

		stiASSERTMSG (false, "CstiUpdate::aktualizrInitialize: Error\n");
	}
	
	return hResult;
}

void CstiUpdate::eventUpdateDownload ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::eventUpdateDownload\n");
	);
	
	std::lock_guard<std::mutex> lock (m_aktualizrMutex);
	
	auto m_networkConnected = (IstiNetwork::InstanceGet ()->ServiceStateGet() == estiServiceOnline) ||
				   (IstiNetwork::InstanceGet ()->ServiceStateGet() == estiServiceReady);

	stiTESTCONDMSG (m_networkConnected, stiRESULT_OFFLINE_ACTION_NOT_ALLOWED, "CstiUpdate::eventUpdateDownload: network not connected");

	try
	{
		// Make sure we are initialized
		hResult = aktualizrInitialize ();
		stiTESTRESULT();
	
		stiTESTCONDMSG (!m_runDownloadThread, stiRESULT_TASK_ALREADY_STARTED, "CstiUpdate::eventUpdateDownload: download task is already running");
		
		// The download task will be run in a seperate thread
		// to allow for pause, resume and abort to be called on
		// the event queue
		m_runDownloadThread = true;
		std::thread ([this]{downloadThreadTask ();}).detach ();

	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventUpdateDownload: std::exeption %s.", ex.what ());
	}

STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		stateSet (eERROR);
		stiASSERTMSG (false, "CstiUpdate::eventUpdateDownload: Error\n");
	}
	
	return;
}

void CstiUpdate::downloadThreadTask ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::mutex> lock (m_aktualizrMutex);
	
	try
	{
		Config config (m_configDirs);
		std::shared_ptr<INvStorage> storage;
		boost::optional<Uptane::Target> currentVersion;
		boost::optional<Uptane::Target> pendingVersion;
		result::UpdateCheck updateCheckResult;
		
		try
		{
			// Check for pending intallation already downloaded and installed
			// just needs to reboot
			storage = INvStorage::newStorage (config.storage);
			storage->importData(config.import);
			storage->loadPrimaryInstalledVersions (&currentVersion, &pendingVersion);
		}
		catch (std::exception &ex)
		{
			stateSet (eERROR);
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: loadPrimaryInstalledVersions std::exeption %s. Probably not provisioned yet!", ex.what ());
		}
		catch (...)
		{
			stateSet (eERROR);
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: loadPrimaryInstalledVersions Unknown error. Probably not provisioned yet!");
		}

		if (pendingVersion && pendingVersion->IsValid())
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::downloadThreadTask: pending version detected. Reboot needed.\n");
				vpe::trace ("CstiUpdate::downloadThreadTask: pending version image hash = ", pendingVersion->sha256Hash (), "\n");
				vpe::trace ("CstiUpdate::downloadThreadTask: pending version image filename = ", pendingVersion->filename (), "\n");
				
				if (currentVersion && currentVersion->IsValid() )
				{
					vpe::trace ("CstiUpdate::downloadThreadTask: current version image hash = ", currentVersion->sha256Hash (), "\n");
					vpe::trace ("CstiUpdate::downloadThreadTask: current version image filename = ", currentVersion->filename (), "\n");
				}
				
			); // stiDEBUG_TOOL
			
			m_aktualizr->SendManifest().get();

			stateSet (eUPDATE_COMPLETE);
		}
		else if (m_downloadResult.status == result::DownloadStatus::kSuccess
			&& m_downloadResult.updates.size () > 0)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::downloadThreadTask: Already downloaded.\n");
			);

			downloadSuccessfulSignal.Emit ();
		}
		else
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::downloadThreadTask: No local pending version. Checking if an update is available.\n");
			);

			m_aktualizr->SendDeviceData().get();
			
			updateCheckResult = m_aktualizr->CheckUpdates().get();
						
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::downloadThreadTask: size = ", updateCheckResult.updates.size (), "\n");
			);
			
			if (updateCheckResult.status == result::UpdateStatus::kUpdatesAvailable 
				&& updateCheckResult.updates.size () > 0)
			{
				stiDEBUG_TOOL (g_stiUpdateDebug,
					vpe::trace ("CstiUpdate::downloadThreadTask: Updates available, size = ", updateCheckResult.updates.size (), "\n");
				);
				
				updateNeededSignal.Emit (true);
				
				stateSet (eDOWNLOADING);
				
				m_downloadResult = m_aktualizr->Download (updateCheckResult.updates).get();
				
				if (m_downloadResult.status != result::DownloadStatus::kSuccess
					|| m_downloadResult.updates.empty())
				{
					if (m_downloadResult.status != result::DownloadStatus::kNothingToDownload)
					{
						// If the download failed, inform the backend immediately.
						m_aktualizr->SendManifest().get();
					}
					
					stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: Download failed");
				}
				else
				{
					stiDEBUG_TOOL (g_stiUpdateDebug,
						vpe::trace ("CstiUpdate::downloadThreadTask: Download successfull\n");
					);
					
					downloadingSignal.Emit (100);
					downloadSuccessfulSignal.Emit ();
				}
			}
			else if (updateCheckResult.status == result::UpdateStatus::kError) 
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: CheckUpdates failed");
			}
			else
			{
				stiDEBUG_TOOL (g_stiUpdateDebug,
					vpe::trace ("CstiUpdate::downloadThreadTask: No updates available\n");
				);
				
				updateNeededSignal.Emit (false);
			}
		}
	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: std::exeption %s.", ex.what ());
	}

STI_BAIL:
	
	m_runDownloadThread = false;
	
	if (stiIS_ERROR(hResult))
	{
		// If the download failed, inform the backend immediately.
		m_aktualizr->SendManifest().get();

		stateSet (eERROR);
		stiASSERTMSG (false, "CstiUpdate::downloadThreadTask: Error\n");
	}
	
	return;
}

void CstiUpdate::eventUpdateInstall ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::eventUpdateInstall\n");
	);
	
	std::lock_guard<std::mutex> lock (m_aktualizrMutex);
	
	try
	{
		// Make sure we are initialized
		hResult = aktualizrInitialize ();
		stiTESTRESULT();
		
		// Make sure we have something downloaded
		if (m_downloadResult.status == result::DownloadStatus::kSuccess 
			&& m_downloadResult.updates.size () > 0)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventUpdateInstall: download result size = ", m_downloadResult.updates.size (), "\n");
			);
			
			stiTESTCONDMSG (!m_runDownloadThread, stiRESULT_ERROR, "CstiUpdate::eventUpdateInstall: download task is still running");
			stiTESTCONDMSG (!m_runInstallThread, stiRESULT_TASK_ALREADY_STARTED, "CstiUpdate::eventUpdateInstall: install task is already running");
			
			// The install task will be run in a seperate thread
			// to allow for pause, resume and abort to be called on
			// the event queue
			m_runInstallThread = true;
			std::thread ([this]{installThreadTask ();}).detach ();
		}
		else
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: Nothing downloaded");
		}
	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::eventUpdateInstall: std::exeption %s.", ex.what ());
	}
	
STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		stateSet (eERROR);
		stiASSERTMSG (false, "CstiUpdate::eventUpdateInstall: Error\n");
	}
	
	return;
}


void CstiUpdate::installThreadTask ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::mutex> lock (m_aktualizrMutex);
	
	try
	{
		result::Install installResult;
		stateSet (ePROGRAMMING);
			
		//Synchronous, Install call will take thread control until it is finished
		installResult = m_aktualizr->Install (m_downloadResult.updates).get();
			
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::installThreadTask: intall result size = ", installResult.ecu_reports.size(), "\n");
		);
		
		if (installResult.ecu_reports.size() > 0)
		{
			if (installResult.ecu_reports[0].install_res.result_code.num_code != data::ResultCode::Numeric::kNeedCompletion
				&& installResult.ecu_reports[0].install_res.result_code.num_code != data::ResultCode::Numeric::kAlreadyProcessed)
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::installThreadTask: Install failed");
			}
			else
			{
				stiDEBUG_TOOL (g_stiUpdateDebug,
					vpe::trace ("CstiUpdate::installThreadTask: Install successfull\n");
				);
				
				// Reset m_downloadResult
				m_downloadResult.status = result::DownloadStatus::kError;
				m_downloadResult.updates.clear ();
			
				programmingSignal.Emit (100);
				
				stateSet (eUPDATE_COMPLETE);
			}
		}
		else
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::installThreadTask: Install report size is 0\n");
			);
		}
	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::installThreadTask: std::exeption %s.", ex.what ());
	}
	
STI_BAIL:
	
	m_runInstallThread = false;
	
	if (stiIS_ERROR(hResult))
	{
		stateSet (eERROR);
		stiASSERTMSG (false, "CstiUpdate::installThreadTask: Error\n");
	}
	
	return;
}

void CstiUpdate::eventNetworkDisconnected ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::eventNetworkDisconnected\n");
	);
	
	// We are only concerned about network disconnect if we are in the 
	// download thread.  The other functions dont need a connection.
	if (m_aktualizr && m_runDownloadThread)
	{
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::eventNetworkDisconnected: Currently downloading and network was disconnected\n");
		);
		
		stateSet (eERROR);
		
		// We could call m_aktualizr->Abort (); or pause ();
		// but it works better for aktualizr to just keep
		// trying to download. If abort is called then the OTA server
		// looses the queued update and one must re-queue an update.
		// Leaving the thread running will let it still
		// respond to pause and resume.
	}
}

/*!
 * \brief Pauses the download of a firmware update if one is in progress
 *
 */
void CstiUpdate::pause ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::pause\n");
	);
	
	PostEvent([this]
	{
		std::lock_guard<std::mutex> lock (m_aktualizrMutex);

		m_nPauseCount++;

		if (m_aktualizr)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::pause: m_nPauseCount = ", m_nPauseCount, "\n");
				vpe::trace ("CstiUpdate::pause: m_runDownloadThread = ", m_runDownloadThread, "\n");
				vpe::trace ("CstiUpdate::pause: m_runInstallThread = ", m_runInstallThread, "\n");
			);

			result::Pause pause_result = m_aktualizr->Pause ();
			
			if (result::PauseStatus::kSuccess == pause_result.status
				|| result::PauseStatus::kAlreadyPaused == pause_result.status)
			{
				stiDEBUG_TOOL (g_stiUpdateDebug,
					vpe::trace ("CstiUpdate::pause: UPDATE IS PAUSED\n\n");
				);
				
				stateSet (ePAUSED);
			}
		}
	});

	return;
}

/*!
 * \brief Resume the download of a firmware update if one is in progress
 *
 */
void CstiUpdate::resume ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::resume\n");
	);
	
	PostEvent([this]
	{
		if (m_nPauseCount > 0)
		{
			m_nPauseCount--;
			
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::resume: m_nPauseCount = ", m_nPauseCount, "\n");
			);

			if (m_nPauseCount == 0)
			{
				if (m_aktualizr)
				{
					stiDEBUG_TOOL (g_stiUpdateDebug,
						vpe::trace ("CstiUpdate::resume: m_runDownloadThread = ", m_runDownloadThread, "\n");
						vpe::trace ("CstiUpdate::resume: m_runInstallThread = ", m_runInstallThread, "\n");
					);
					
					result::Pause resume_result = m_aktualizr->Resume ();
					
					if (result::PauseStatus::kSuccess == resume_result.status
					|| result::PauseStatus::kAlreadyRunning == resume_result.status)
					{
						stiDEBUG_TOOL (g_stiUpdateDebug,
							vpe::trace ("CstiUpdate::resume: UPDATE IS RESUMED\n\n");
						);
					}
					else
					{
						stateSet (eERROR);
						stiASSERTMSG (false, "CstiUpdate::resume: Error can't resume\n");
					}
				}
				
			}
		}
	});

	return;
}

//:-----------------------------------------------------------------------------
//: Private methods
//:-----------------------------------------------------------------------------

///
/// \brief Returns the state of the Update service
///
///	\return CstiUpdate::EState
///
CstiUpdate::EState CstiUpdate::stateGet ()
{
	stiLOG_ENTRY_NAME (CstiUpdate::stateGet);

	return (m_eState);

} // end CstiUpdate::stateGet

///
/// \brief Sets the current state
///
void CstiUpdate::stateSet (
	EState eState)
{
	stiLOG_ENTRY_NAME (CstiUpdate::stateSet);

	m_eState = eState;

	// Switch on the new state...
	switch (m_eState)
	{
		case eUNINITIALIZED:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::stateSet - Changing state to eUNINITIALIZED\n");
			);
			
			break;
		} // end case eUNITIALIZED

		case eIDLE:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::stateSet - Changing state to eIDLE\n");
			);
			
			m_previousProgress = 0;
			m_newProgress = 0;
			
			break;
		} // end case eIDLE

		case eDOWNLOADING:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::stateSet - Changing state to eDOWNLOADING\n");
			);

			downloadingSignal.Emit (0);

			break;
		} // end case eDOWNLOADING

		case ePROGRAMMING:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::stateSet - Changing state to ePROGRAMMING\n");
			);

			programmingSignal.Emit (0);

			break;
		} // end case ePROGRAMMING

		case ePAUSED:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::stateSet - Changing state to ePAUSED\n");
			);

			break;
		} // end case ePAUSED

		case eERROR:
		{
			stiASSERTMSG (false, "CstiUpdate::stateSet - Changing state to eERROR\n");
			
			errorSignal.Emit ();
			
			stateSet (eIDLE);
			
			break;
		}
			
		case eUPDATE_COMPLETE:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::stateSet - Changing state to eUPDATE_COMPLETE\n");
			);
			
			successfulSignal.Emit ();
			
			stateSet (eIDLE);
			
			break;
		}

	} // end switch

} // end CstiUpdate::stateSet
