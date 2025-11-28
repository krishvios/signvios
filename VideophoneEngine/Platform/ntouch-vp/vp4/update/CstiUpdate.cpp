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
//#include <cctype>  // standard type definitions.
//#include <sys/stat.h>
//#include <iostream>
//#include <sstream>
//#include <regex>
//#include <map>
//#include <string>
//#include <fcntl.h>
//#include <glib.h>
//#include <string>
//#include <boost/algorithm/string/erase.hpp>
#include <glib.h>
#include "stiTaskInfo.h"
#include "IstiNetwork.h"


//
// Aktualizr includes
//


#include <boost/filesystem.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>

#include "aktualizr-lite/api.h"
#include "aktualizr-lite/aklite_client_ext.h"
#include "aktualizr-lite/cli/cli.h"
//#include "logging/logging.h"

#include "stiOS.h"          // OS Abstraction
#include "CstiUpdate.h"
#include "stiDebugTools.h"
#include "stiTools.h"
#include "stiTaskInfo.h"

namespace logging = boost::log;
namespace trivial = boost::log::trivial;

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
// Defines
//
#define LOG_INFO BOOST_LOG_TRIVIAL(info)
#define LOG_WARNING BOOST_LOG_TRIVIAL(warning)
#define LOG_ERROR BOOST_LOG_TRIVIAL(error)

///
/// \brief Constructor
///
CstiUpdate::CstiUpdate ()
	:
	CstiEventQueue("CstiUpdate")
{
} // end CstiUpdate::CstiUpdate


///
/// \brief Destructor
///
CstiUpdate::~CstiUpdate ()
{
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::~CstiUpdate\n");
	);

	if (m_akliteClient)
	{
		//boost::log::core::get()->set_filter(m_originalCoreFilter);
		m_akliteClient.reset ();
	}

	if (m_installContext)
	{
		m_installContext.reset ();
	}
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::~CstiUpdate: m_nPauseCount = ", m_nPauseCount, "\n");
		vpe::trace ("CstiUpdate::~CstiUpdate: m_runDownloadThread = ", m_runDownloadThread, "\n");
		vpe::trace ("CstiUpdate::~CstiUpdate: m_runInstallThread = ", m_runInstallThread, "\n");
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
//	PostEvent([this]{ eventOTAServerCheck (); });
//	PostEvent([this]{ eventSendDeviceData (); });
	
	return stiRESULT_SUCCESS;
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

stiHResult CstiUpdate::aktualizrInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::aktualizrInitialize\n");
	);
	
	aktualizrLoggerLevelSet ();

	try
	{
		//std::string local_repo_path = "";
		//bool is_local_update{false};
		//LocalUpdateSource local_update_source;

		std::vector<boost::filesystem::path> cfg_dirs;

		auto env{boost::this_process::environment()};

		if (env.end() != env.find("AKLITE_CONFIG_DIR"))
		{
			cfg_dirs.emplace_back(env.get("AKLITE_CONFIG_DIR"));
		}
		else
		{
			cfg_dirs = AkliteClient::CONFIG_DIRS;
		}

		/*
		if (!local_repo_path.empty())
		{
			local_update_source = {
			local_repo_path + "/tuf",
			local_repo_path + "/ostree_repo",
			local_repo_path + "/apps"
			};
			is_local_update = true;
			LOG_INFO << "Offline mode. Updates path=" << local_repo_path;
		}
		else
		{
			LOG_INFO << "Online mode";
		}
		*/

		if (m_akliteClient)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: m_akliteClient already created\n");
			);
		}
		else
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: m_akliteClient needs to be created\n");
			);

			m_akliteClient = std::make_unique<AkliteClient>(cfg_dirs);

		}

		//auto interval = m_akliteClient->GetConfig().get("uptane.polling_sec", 600);

/*
		auto reboot_cmd = get_reboot_cmd(*m_akliteClient);

		if (access(reboot_cmd.c_str(), X_OK) != 0)
		{
			LOG_ERROR << "Reboot command: " << reboot_cmd << " is not executable";
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: Reboot command: %s is not executable\n", reboot_cmd);
			);

			stateSet (eERROR);
			stiTHROW(stiRESULT_ERROR);
		}
*/
		//LOG_INFO << "aklite interval is " << interval << " second interval";

		auto current = m_akliteClient->GetCurrent();

		LOG_INFO << "Active Target: " << current.Name() << ", sha256: " << current.Sha256Hash();

		// We need to delete this object because aktualizr
		// locks some files so only one prcess uses it at
		// any given time

		//client.reset();


#if 0
		std::string local_repo_path = "";

		auto client = init_client(local_repo_path.empty());

		if (client == nullptr)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: init_client failed\n");
			);
			stateSet (eERROR);
			stiTHROW(stiRESULT_ERROR);
		}

		LocalUpdateSource local_update_source;

		if (local_repo_path.empty())
		{
			LOG_INFO << "Online mode";
		}
		else
		{
			auto abs_repo_path = boost::filesystem::canonical(local_repo_path).string();
			LOG_INFO << "Offline mode. Updates path=" << abs_repo_path;
			local_update_source = {abs_repo_path + "/tuf", abs_repo_path + "/ostree_repo", abs_repo_path + "/apps"};
		}

		auto status = aklite::cli::CheckIn(*client, local_repo_path.empty() ? nullptr : &local_update_source);
		print_status(status);

		if (aklite::cli::IsSuccessCode(status))
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::aktualizrInitialize: CheckIn success\n");
			);
		}
		else
		{
			stateSet (eERROR);
			stiTHROW(stiRESULT_ERROR);
		}
#endif


#if 0
		std::string UniqueID;

		if (estiOK != stiOSGetUniqueID (&UniqueID))
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: Cant get UniqueID\n");
		}
			
		// Remove all ":"
		//boost::erase_all (UniqueID, ":");

		if (sorensonMacPrefix != UniqueID.substr(0, 6))
		{
			vpe::trace ("CstiUpdate::akliteInitialize: UniqueID: ", UniqueID, "\n");
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: UniqueID not valid\n");
		}

		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::aktualizrInitialize: Just for info deviceId: ", UniqueID, "\n");
		);
#endif


		//aktualizrLoggerLevelSet ();


#if 0
		// We should be registered by now
		if (!m_aktualizr->IsRegistered ())
		{
			// TODO: If we are not registerd something bad has happened.
			// We should remove /var/sota and somehow let
			// the app know to try again.
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: IsRegistered failed. Try cleaning /var/sota/\n");
		}
#endif

	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::aktualizrInitialize: std::exeption %s.", ex.what ());
	}

STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		stiASSERTMSG (false, "CstiUpdate::aktualizrInitialize: Error\n");
	}
	
	return hResult;
}

stiHResult CstiUpdate::aktualizrLoggerLevelSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int loglevel = 3;

	//m_originalCoreFilter = boost::log::core::get()->get_filter();

	switch (g_stiUpdateDebug.valueGet ())
	{
		case 0:
		{
			loglevel = 3;
			break;
		} // end case 0

		case 1:
		{
			vpe::trace ("CstiUpdate::aktualizrLoggerLevelSet: Changing update loglevel to 1\n");
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
			loglevel = 1;
			break;
		} // end case 1

		case 2:
		{
			vpe::trace ("CstiUpdate::aktualizrLoggerLevelSet: Changing update loglevel to 0\n");
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
			loglevel = 0;
			break;
		} // end case 2

		default:
		{
			vpe::trace ("aktualizrLoggerLevelSet: g_stiUpdateDebug is unkown, changing update loglevel to 3\n");
			loglevel = 3;
			break;
		} // end case default

	} // end switch

	if (m_akliteClient)
	{
		m_akliteClient->GetConfig().put("logger.loglevel", loglevel);
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiASSERTMSG (false, "CstiUpdate::aktualizrLoggerLevelSet: Error\n");
	}

	return hResult;

}

void CstiUpdate::eventUpdateCheck ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::eventUpdateCheck\n");
	);

//STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stateSet (eERROR);
		stiASSERTMSG (false, "CstiUpdate::eventUpdateCheck: Error\n");
	}

	return;
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

	stiDEBUG_TOOL (g_stiUpdateDebug,
		vpe::trace ("CstiUpdate::downloadThreadTask\n");
	);
	
	g_autoptr(GMainContext) main_context = NULL;

	main_context = g_main_context_new ();

	g_main_context_push_thread_default (main_context);

	try
	{
		auto current = m_akliteClient->GetCurrent();

		LOG_INFO << "Active Target: " << current.Name() << ", sha256: " << current.Sha256Hash();
		LOG_INFO << "Checking for a new Target...";

		// TODO: Fix when we add local updates
		//CheckInResult res = !is_local_update?
		//					m_akliteClient->CheckIn():
		//					m_akliteClient->CheckInLocal(&local_update_source);
		CheckInResult res = m_akliteClient->CheckIn();

		if (res.status != CheckInResult::Status::Ok && res.status != CheckInResult::Status::OkCached)
		{
			LOG_ERROR << "Unable to update latest metadata";
			stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: Unable to update latest metadata (try again later?)");
		}

		auto latest = res.GetLatest();

		if (m_akliteClient->IsRollback(latest))
		{
			LOG_INFO << "Latest Target is marked for causing a rollback and won't be installed: " << latest.Name();
		}
		else
		{
			LOG_INFO << "Found Latest Target: " << latest.Name();
		}

		if (m_akliteClient->IsRollback(latest) && current.Name() == latest.Name())
		{
			// Handle the case when Apps failed to start on boot just after an update.
			// This is only possible with `pacman.create_containers_before_reboot = 0`.
			LOG_INFO << "The currently booted Target is a failing Target, finding Target to rollback to...";

			const TufTarget rollback_target = m_akliteClient->GetRollbackTarget();

			if (rollback_target.IsUnknown())
			{
				LOG_ERROR << "Failed to find Target to rollback to after failure to start Apps at boot of a new sysroot";
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: Failed to find Target to rollback to after failure to start Apps at boot of a new sysroot (reboot?)");
			}

			latest = rollback_target;
			LOG_INFO << "Rollback Target is " << latest.Name();
		}

		if (latest.Name() != current.Name() && !m_akliteClient->IsRollback(latest))
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::downloadThreadTask: Update needed\n");
			);

			updateNeededSignal.Emit (true);

			stateSet (eDOWNLOADING);

			std::string reason = "Updating from " + current.Name() + " to " + latest.Name();

			//m_installContext = m_akliteClient->Installer(latest, reason, "", InstallMode::All,
			//							is_local_update ? &local_update_source : nullptr);

			m_installContext = m_akliteClient->Installer(latest, reason, "", InstallMode::All, nullptr);

			if (!m_installContext)
			{
				LOG_ERROR << "Found latest Target but failed to retrieve its metadata from DB, skipping update";
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: Found latest Target but failed to retrieve its metadata from DB, skipping update (try again later?)");
			}

			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::downloadThreadTask: Downloading\n");
			);

			auto dres = m_installContext->Download();

			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::downloadThreadTask: Download returned\n");
			);

			if (dres.status != DownloadResult::Status::Ok)
			{
				LOG_ERROR << "Unable to download target: " << dres;
				stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: Unable to download target (try again later?)");
			}

			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::downloadThreadTask: Download successful\n");
			);

			downloadingSignal.Emit (100);
			downloadSuccessfulSignal.Emit ();
		}
		//else if (!is_local_update || m_akliteClient->IsRollback(latest))
		else if (m_akliteClient->IsRollback(latest))
		{
			m_installContext = m_akliteClient->CheckAppsInSync();

			if (m_installContext != nullptr)
			{
				LOG_INFO << "Syncing Active Target Apps";
				auto dres = m_installContext->Download();

				downloadingSignal.Emit (100);
				downloadSuccessfulSignal.Emit ();
			}
		}
		else
		{
			LOG_INFO << "Device is up-to-date";
			updateNeededSignal.Emit (false);
		}
	}
	catch (std::exception &exc)
	{
		LOG_ERROR << "Failed to find or update Target: " << exc.what();
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::downloadThreadTask: std::exeption %s.", exc.what ());
	}

STI_BAIL:
	
	m_runDownloadThread = false;

	if (main_context)
	{
		g_main_context_pop_thread_default (main_context);
	}
	
	if (stiIS_ERROR(hResult))
	{
		// If the download failed, inform the backend immediately.
		//m_aktualizr->SendManifest().get();

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
		if (m_installContext)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::eventUpdateInstall: Starting install thread");
			);

			stiTESTCONDMSG (!m_runDownloadThread, stiRESULT_ERROR, "CstiUpdate::eventUpdateDownload: download task is still running");
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
	
	g_autoptr(GMainContext) main_context = NULL;

	main_context = g_main_context_new ();

	g_main_context_push_thread_default (main_context);

	try
	{
		auto ires = m_installContext->Install();

		if (ires.status == InstallResult::Status::Ok)
		{
			//Done, no reboot needed
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::installThreadTask: Done, no reboot needed\n");
			);

			programmingSignal.Emit (100);
			stateSet (eUPDATE_COMPLETE);
		}
		else if (ires.status == InstallResult::Status::BootFwNeedsCompletion)
		{
			//Install needs reboot to finish because of previous firmware update
			LOG_ERROR << "Cannot start installation since the previous boot fw update requires device rebooting; "
					<< "the client will start the target installation just after reboot.";

			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::installThreadTask: Cannot start installation since the previous boot firmware update requires device rebooting\n");
			);

			programmingSignal.Emit (100);

			stateSet (eUPDATE_COMPLETE);
		}
		else if (ires.status == InstallResult::Status::NeedsCompletion)
		{
			//Install complete but needs reboot
			stiDEBUG_TOOL (g_stiUpdateDebug,
				vpe::trace ("CstiUpdate::installThreadTask: Intall complete but needs reboot\n");
			);

			programmingSignal.Emit (100);

			stateSet (eUPDATE_COMPLETE);
		}
		else
		{
			LOG_ERROR << "Unable to install target: " << ires;
			stiASSERTMSG (false, "CstiUpdate::eventUpdateInstall: Error: Unable to install target\n");
		}

	}
	catch (std::exception &ex)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiUpdate::installThreadTask: std::exeption %s.", ex.what ());
	}
	
STI_BAIL:
	
	m_runInstallThread = false;

	if (main_context)
	{
		g_main_context_pop_thread_default (main_context);
	}
	
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
	if (m_akliteClient && m_runDownloadThread)
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
		m_nPauseCount++;

		// TODO: Find a way to pause and resume
#if 0
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
#endif
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
		// TODO: Find a way to pause and resume
#if 0
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
				}
				
			}
		}
#endif
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
