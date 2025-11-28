/*!
* \file CstiNetworkBase.cpp
* \brief Consolidation of all network tasks.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#include "CstiNetworkBase.h"
#include "stiTools.h"
#include "stiError.h"
#include "stiTrace.h"
#include "stiTools.h"
#include <fstream>
#include <cstdlib>
#include <regex.h>
#include <sys/stat.h>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>		// include it here to make sure we get the gnu version of basename

#include <dirent.h>

//
// Locals
//
enum EstiInputType
{
	estiINPUT_NEED_PASSPHRASE = 1,
	estiINPUT_NEED_IDENTITY = 2,
	estiINPUT_NEED_SSID = 4
};

/*! 
 * \brief Global Constants
 */

const std::string g_hiddenSsid = "_hidden_managed_";

/*!
 * \brief Constructor
 */
CstiNetworkBase::CstiNetworkBase (CstiBluetooth *bluetooth)
:
	CstiEventQueue("CstiNetwork"),
	m_bluetooth(bluetooth)
{
	stiLOG_ENTRY_NAME (CstiNetworkBase::CstiNetworkBase);

	QueueDepthThresholdSet (50);
} // end CstiNetwork::CstiNetwork


/*!
 * \brief destructor
 */
CstiNetworkBase::~CstiNetworkBase ()
{
	CstiEventQueue::StopEventLoop ();
	m_reconnectTimer.Stop();
	m_resetTimer.Stop();
	ConnmanUninitialize ();
}

/*!
* \brief Initialize the network object
* \retval stiRESULT_ERROR if failed
*/
stiHResult CstiNetworkBase::Initialize (
	std::string dirName,
	std::string stopCommand,
	std::string startCommand)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_connmanDirName = dirName;
	m_connmanStopCommand = stopCommand;
	m_connmanStartCommand = startCommand;

	m_signalConnections.push_back(NetworkManagerInterface::addedSignal.Connect(
		[this](std::shared_ptr<NetworkManagerInterface> networkManagerInterface)
		{
			PostEvent([this, networkManagerInterface]
			{
				eventNetworkManagerInterfaceAdded(networkManagerInterface);
			});
		}));

	m_signalConnections.push_back(NetworkTechnology::addedSignal.Connect(
		[this](std::shared_ptr<NetworkTechnology> technology)
		{
			PostEvent([this, technology]
			{
				eventTechnologyAdded(technology);
			});
		}));

	m_signalConnections.push_back(NetworkTechnology::scanFinishedSignal.Connect(
		[this](bool success)
		{
			PostEvent([this, success]
			{
				eventScanFinished(success);
			});
		}));

	m_signalConnections.push_back(NetworkTechnology::powerChangedSignal.Connect(
		[this](std::shared_ptr<NetworkTechnology> technology)
		{
			powerChangedSignalHandle(technology);
		}));

	m_signalConnections.push_back(m_reconnectTimer.timeoutEvent.Connect (
		[this]() {
			PostEvent ([this]
			{
				 eventReconnectTimerTimeout();
			});
		}));

	m_signalConnections.push_back(m_resetTimer.timeoutEvent.Connect (
		[this]() {
			PostEvent([this]
			{
				eventResetTimerTimeout();
			});
		}));

	serviceSignalsConnect();
	hResult = ConnmanInitialize();
	stiTESTRESULT();
	wirelessMacAddressUpdate();

STI_BAIL:
	return (hResult);
}

void CstiNetworkBase::serviceSignalsConnect()
{

	m_signalConnections.push_back(NetworkService::addedSignal.Connect(
		[this](std::shared_ptr<NetworkService> service)
		{
			PostEvent([this, service]
			{
				eventServiceAdded(service);
			});
		}));

	m_signalConnections.push_back(NetworkService::serviceOfflineStateChangedSignal.Connect(
		[this](std::shared_ptr<NetworkService> service, bool blocked)
		{
			PostEvent([this, service, blocked]
			{
				serviceOfflineStateChangedHandle(service, blocked);
			});
		}));

	m_signalConnections.push_back(NetworkService::serviceOnlineStateChangedSignal.Connect(
		[this](std::shared_ptr<NetworkService> service)
		{
			PostEvent([this, service]
			{
				serviceOnlineStateChangedHandle(service);
			});
		}));

	m_signalConnections.push_back(NetworkService::serviceConnectedSignal.Connect(
		[this](std::shared_ptr<NetworkService> service)
		{
			PostEvent([this, service]
			{
				serviceConnectedHandle(service);
			});
		}));
				
	m_signalConnections.push_back(NetworkService::serviceConnectionErrorSignal.Connect(
		[this](std::shared_ptr<NetworkService> service, connectionError error)
		{
			PostEvent([this, service, error]
			{
				serviceConnectionErrorHandle(service, error);
			});
		}));

	m_signalConnections.push_back(NetworkService::serviceDisconnectedSignal.Connect(
		[this](std::shared_ptr<NetworkService> service, bool success)
		{
			PostEvent([this, service, success]
			{
				serviceDisconnectedHandle(service, success);
			});
		}));

	m_signalConnections.push_back(NetworkService::newWiredConnectedSignal.Connect (
		[this]()
		{
			PostEvent([this]
			{
				m_bCableConnected = true;
				wiredNetworkConnectedSignal.Emit();
			});
		}));

	m_signalConnections.push_back(NetworkService::propertySetFinishedSignal.Connect(
		[this](std::shared_ptr<NetworkService> service, bool success)
		{
			PostEvent([this, success]
			{
				if (!success)
				{
					m_error = estiErrorGeneric;
					stateSet(estiERROR);
				}
			});
		}));

	m_signalConnections.push_back(NetworkService::specificServiceGetSignal.Connect(
		[this](std::shared_ptr<NetworkService> service, std::string objectPath)
		{
			PostEvent([this, objectPath]
			{
				m_networkManager->specificServiceGet(objectPath);
			});
		}));
}

stiHResult CstiNetworkBase::ConnmanInitialize ()
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("NetworkDebug: CstiNetworkBase::%s: enter..\n", __func__);
	);
	stiHResult hResult = stiRESULT_SUCCESS;
	DbusError dbusError;

	auto dbusConnection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, dbusError.get());
	if (dbusError.valid())
	{
		stiTrace("Can't connect to SYSTEM DBUS - %s\n", dbusError.message().c_str());
	}
	else
	{
		m_dbusConnection = std::make_shared<DbusConnection> (dbusConnection);
	}

	return hResult;
}


void CstiNetworkBase::ConnmanUninitialize ()
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("CstiNetworkBase::%s: enter..\n", __func__);
	);
	ConnmanAgentUnregister ();

	if (m_networkManager)
	{
		m_networkManager = nullptr;
	}

	if (m_dbusConnection)
	{
		m_dbusConnection = nullptr;
	}
}


void CstiNetworkBase::objectRemove(
	const std::string objectPath,
	const std::string interfaceName)
{
	auto dbusObject = m_dbusConnection->find(objectPath);
	dbusObject->interfaceRemoved(interfaceName);
	m_dbusConnection->remove(objectPath);
}


void CstiNetworkBase::processService (
	GVariant *variant)
{
	auto tmpVariant = g_variant_get_child_value(variant, 0);
	auto objectPath = g_variant_get_string(tmpVariant, nullptr);
	g_variant_unref(tmpVariant);
	tmpVariant = nullptr;

	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("Looking for %s\n", objectPath);
	);

	auto dict = g_variant_get_child_value(variant, 1);

	auto service = serviceFromObjectPathGet (objectPath);

	if (service)
	{
		stiDEBUG_TOOL (g_stiNetworkDebug > 1,
			stiTrace("NetworkDebug: processService - Already added service %s\n", objectPath);
		);

		//
		// If it is not a new service, still parse the service
		// to make sure we capture anything that might have changed
		//
		service->propertiesProcess(dict);
	}
	else
	{
		stiDEBUG_TOOL (g_stiNetworkDebug > 1,
			stiTrace("NetworkDebug: processService - Adding service %s\n", objectPath);
		);

		/*
		 * if we get a servicesChanged signal too soon after creating a new service we can get here after the DbusObject
		 * was set up but before it is added to the m_services list.  check for this condition.
		 */
		auto dbusObject = m_dbusConnection->find(objectPath);
		if (!dbusObject)
		{
			auto dbusObject = m_dbusConnection->findOrCreate(this, objectPath);
			dbusObject->interfaceAdded(CONNMAN_SERVICE_INTERFACE, dict, NetworkDbusInterface::interfaceMake);
		}
		else
		{
			stiTrace ("NetworkDebug: processService: skipping creating a new dbus Object for %s\n", objectPath);
			auto interface = std::static_pointer_cast<NetworkService>(dbusObject->interfaceGet (CONNMAN_SERVICE_INTERFACE));

			if (interface)
			{
				interface->propertiesProcess (dict);
			}
		}
	}

	g_variant_unref(dict);
	dict = nullptr;
}


void CstiNetworkBase::eventServicesChanged (
	GVariant *changedServices,
	std::vector<std::string> removedServices)
{
	bool notify = false;

	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("%s: enter: ChangedServices = %p, RemovedServices = %d\n", __func__, changedServices, removedServices.size());
	);

	if (!m_bShutdown)
	{
		for (unsigned int i = 0; i < removedServices.size(); i++)
		{
			/*
			 * assume a removed service won't be added back in the same event...
			 */
			if (!strncmp(basename(removedServices[i].c_str()), "ethernet_", 9))
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("NetworkDebug: sending stiMSG_NETWORK_WIRED_DISCONNECTED\n");
				);

				wiredNetworkDisconnectedSignal.Emit();
				m_bCableConnected = false;
			}

			stiDEBUG_TOOL (g_stiNetworkDebug,
				stiTrace("NetworkDebug: Removing service %s\n", removedServices[i].c_str());
			);

			for (auto iter = m_services.begin(); iter != m_services.end(); iter++)
			{
				if ((*iter)->objectPathGet() == removedServices[i])
				{
					if (*iter == m_currentService)
					{
						m_currentService = nullptr;
					}

					if ((*iter)->m_technologyType == technologyType::wireless)
					{
						notify = true;
					}
					objectRemove(removedServices[i], CONNMAN_SERVICE_INTERFACE);
					(*iter) = nullptr;			// delete the NetworkService the iter points at
					m_services.erase(iter);		// if we've kept count correctly, the shared pointer should go out of scope

					break;
				}
			}
		}

		/*
		 * Services that we are already tracking have proxies for them,
		 * so we should get a servicepropertychange event.
		 */
		stiDEBUG_TOOL(g_stiNetworkDebug > 1,
			stiTrace("there are %d entries for changed Services\n", g_variant_n_children(changedServices));
		);

		stiDEBUG_TOOL (g_stiNetworkDebug ,
			stiTrace("NetworkDebug: eventServicesChanged: processing changed services\n");
		);

		for (unsigned int i = 0; i < g_variant_n_children(changedServices); i++)	// one child for every service
		{
			auto variant = g_variant_get_child_value(changedServices, i);

			processService (variant);

			g_variant_unref (variant);
			variant = nullptr;
		}

		stiDEBUG_TOOL (g_stiNetworkDebug ,
			stiTrace("NetworkDebug: eventServicesChanged: finished processing changed services\n");
		);

	}

	if (notify)
	{
		wirelessUpdatedSignal.Emit();
	}

	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("Exit ServicesChangedCallback\n");
	);
}

void CstiNetworkBase::eventTechnologyAdded(std::shared_ptr<NetworkTechnology> interface)
{
	m_technologies.push_back(interface);

	if (interface->m_technologyType == technologyType::wireless && m_resetState == resetState::restarting)
	{
		interface->propertySet("Powered", g_variant_new_variant(g_variant_new_boolean(true)));
	}

	if (interface->m_technologyType == technologyType::bluetooth && m_bluetoothEnabled && m_resetState == resetState::restarting)
	{
		interface->propertySet("Powered", g_variant_new_variant(g_variant_new_boolean(true)));
	}
}

void CstiNetworkBase::powerChangedSignalHandle(std::shared_ptr<NetworkTechnology> technology)
{
	if (technology->m_technologyType == technologyType::bluetooth)
	{
		m_bluetoothPowered = technology->m_powered;
	}
	else if (technology->m_technologyType == technologyType::wireless)
	{
		stiDEBUG_TOOL (g_stiNetworkDebug ,
			stiTrace("NetworkDebug: powerChangedSignalHandle: wifi state changing from %d to %d\n", m_wifiPowered, technology->m_powered);
		);
		m_wifiPowered = technology->m_powered;
		
		if (m_wifiPowered && m_pendingWifiScan)
		{
			stiDEBUG_TOOL (g_stiNetworkDebug ,
				stiTrace("NetworkDebug: powerChangedSignalHandle: we just enabled wifi and there's a pending scan request, so trigger one..\n");
			);
			usleep(250000);		// we need a 250ms delay to ensure connman is ready to scan
			WirelessScan();
			m_pendingWifiScan = false;
		}
	}

	if (technology->m_technologyType == technologyType::bluetooth && m_bluetoothPowered && m_resetState == resetState::restarting)
	{
		m_bluetooth->PostEvent (std::bind (&CstiBluetooth::eventConnectPairedDevices, m_bluetooth));
	}

	if (m_resetState == resetState::restarting)
	{
		if (m_bluetoothEnabled)
		{
			if (m_bluetoothPowered && m_wifiPowered && !m_waitForConnect)
			{
				m_resetState = resetState::unknown;
				connmanResetSignal.Emit();
			}
		}
		else if (m_wifiPowered && !m_waitForConnect)
		{
			m_resetState = resetState::unknown;
			connmanResetSignal.Emit();
		}
	}
}

/*!
 * \brief get technologies detected on this machine, parse and store
 */
void CstiNetworkBase::eventTechnologiesRetrieved(GVariant *technologies)
{
	GVariantIter *variantIter = nullptr;

	GVariant *variant = nullptr;

	variantIter = g_variant_iter_new(technologies);

	while ((variant = g_variant_iter_next_value(variantIter)))
	{
		GVariant *nameVariant = nullptr;
		GVariant *dict = nullptr;
		const gchar *objectPath;

		nameVariant = g_variant_get_child_value(variant, 0);
		objectPath = g_variant_get_string(nameVariant, nullptr);

		dict = g_variant_get_child_value(variant, 1);

		if (nameVariant && dict)
		{
			auto dbusObject = m_dbusConnection->findOrCreate(this, objectPath);
			dbusObject->interfaceAdded(CONNMAN_TECHNOLOGY_INTERFACE, dict, NetworkDbusInterface::interfaceMake);
		}

		if (dict)
		{
			g_variant_unref(dict);
			dict = nullptr;
		}

		if (nameVariant)
		{
			g_variant_unref(nameVariant);
			nameVariant = nullptr;
		}
		g_variant_unref(variant);
		variant = nullptr;
	}

	if (variantIter)
	{
		g_variant_iter_free(variantIter);
		variantIter = nullptr;
	}
}

/*
 * the call to scan finished.  if sucessful, get all of the services
 */
void CstiNetworkBase::eventScanFinished(bool success)
{
	if (success)
	{
		m_networkManager->servicesGet();
	}
	else
	{
		wirelessScanCompleteSignal.Emit(success);
	}
}

void CstiNetworkBase::eventGetServicesFinish(
	GVariant *services)
{
	if (services)
	{
		eventServicesRetrieved(services);
	}

	wirelessScanCompleteSignal.Emit(services != nullptr);
}


void CstiNetworkBase::specificServiceFoundHandle(const std::string &objectPath, GVariant *dict)
{
	auto iter = std::find_if(m_services.begin(), m_services.end(),
					[&objectPath](std::shared_ptr<NetworkService> &service)
					{return service->objectPathGet() == objectPath;});

	if (iter != m_services.end())
	{
		(*iter)->propertiesProcess(dict);
	}
	g_variant_unref(dict);
}

/*
 * all of the services were retrieved.  parse through them.  if we retrieved them because
 * we were scanning, send a message to the user interface that we're done scanning
 */
void CstiNetworkBase::eventServicesRetrieved (
	GVariant *services)
{
	GVariantIter *variantIter = nullptr;
	GVariant *variant = nullptr;

	variantIter = g_variant_iter_new(services);

	stiDEBUG_TOOL (g_stiNetworkDebug ,
		stiTrace("NetworkDebug: eventServicesRetrieved: processing received services\n");
	);

	while ((variant = g_variant_iter_next_value(variantIter)))
	{
		processService(variant);

		g_variant_unref(variant);
		variant = nullptr;
	}

	stiDEBUG_TOOL (g_stiNetworkDebug ,
		stiTrace("NetworkDebug: eventServicesRetrieved: finished processing received services\n");
	);

	if (variantIter)
	{
		g_variant_iter_free(variantIter);
		variantIter = nullptr;
	}

	if (m_rememberedServices.empty())		// we don't have any remembered services.  are we just starting up?
	{
		rememberedServicesUpdate();
	}
		
}

void CstiNetworkBase::eventReconnectTimerTimeout()
{
	std::shared_ptr<NetworkService> service = serviceFromObjectPathGet (m_lostService);
	m_reconnectTimer.Stop();

	if (service.get())
	{
		if (service->m_serviceState < estiServiceReady && service->m_progressState != STATE_ONLINE)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("NetworkDebug: CstiNetworkBase::%s: Attempting reconnect to %s. ServiceState=%d ProgressState=%d\n", __func__, service->m_serviceName.c_str(), static_cast<int>(service->m_serviceState), static_cast<int>(service->m_progressState));
			);
			m_reconnecting = false;
			service->serviceConnect();
		}
	}
}
	
void CstiNetworkBase::eventServiceAdded (std::shared_ptr<NetworkService> service)
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("CstiNetworkBase::%s: adding service %s\n", __func__, service->objectPathGet().c_str());
	);

	m_services.push_back(service);

	if (service->m_technologyType == technologyType::wireless)
	{
		wirelessUpdatedSignal.Emit();
	}
	else if (service->m_technologyType == technologyType::wired)
	{
		if (m_currentService && m_currentService != service)
		{
			service->propertySet("AutoConnect", g_variant_new_variant(g_variant_new_boolean(FALSE)));
			service->serviceDisconnect();
			service = nullptr;
		}
	}

	//
	// If we are connecting to a new hidden service, connman creates a new service
	// for it.
	//
	if (m_newService && (service && service->m_serviceName == m_newService->m_serviceName))
	{
		if (m_targetService == m_newService)
		{
			m_targetService = service;
		}
		m_newService = service;
	}

	if (!m_currentService && (service && service->objectPathGet() == m_lostService))
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: CstiNetworkBase::%s: lost service %s now found. serviceState=%d\n", __func__, service->objectPathGet().c_str(), static_cast<int>(service->m_serviceState));
		);
		switch(service->m_serviceState)
		{
			case estiServiceFailure:
				m_reconnecting = true;
				PostEvent(std::bind(&NetworkService::serviceConnect, service));
				break;
			case estiServiceDisconnect:
			case estiServiceIdle:
			case estiServiceAssociation:
			case estiServiceConfiguration:
				m_reconnecting = true;
				m_reconnectTimer.Start ();
				break;
			case estiServiceReady:
			case estiServiceOnline:
				m_networkManager->specificServiceGet(m_lostService);
				break;
		}
	}
}

/*!
* \brief queue up the message to start a wireless scan
*/
stiHResult CstiNetworkBase::WirelessScan ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Enter WirelessScan()\n");
	);

	PostEvent (std::bind(&CstiNetworkBase::eventWirelessScan, this));

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Exit WirelessScan()\n");
	);
	return hResult;
}

/*
 * actually scan for wireless access points
 */
void CstiNetworkBase::eventWirelessScan()
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Enter EventWirelessScan\n");
	);

	auto iter = std::find_if(m_technologies.begin(), m_technologies.end(),
					[](std::shared_ptr<NetworkTechnology> &technology)
					{return technology->m_technologyType == technologyType::wireless;});

	if (iter == m_technologies.end())			// didn't find wireless?
	{
		wirelessScanCompleteSignal.Emit(false);
		stiTrace("Didn't find wireless technology to scan\n");
	}
	else if (!(*iter)->m_powered && WirelessScanningEnabledGet())
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("Wifi scan requested but interface is not yet enabled. Wait for powered signal..\n");
		);
		m_pendingWifiScan = true;
	}
	else
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("Starting wireless scan\n");
		);
		(*iter)->scanStart();
	}
}

/*
 * enable the wireless radio on this device.
 * the same as doing rfkill block/unblock wlan
 */
stiHResult CstiNetworkBase::WirelessEnable(bool state)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto iter = std::find_if(m_technologies.begin(), m_technologies.end(),
					[](std::shared_ptr<NetworkTechnology> &technology)
					{return technology->m_technologyName == "WiFi";});

	if (iter != m_technologies.end())
	{
		if ((*iter)->m_powered != state)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("Changing wifi powered state from %d to %d\n", (*iter)->m_powered, state );
			);
			(*iter)->propertySet("Powered", g_variant_new_variant(g_variant_new_boolean(state)));
		}
	}
	else
	{
		stiTrace("Couldn't find the Wireless Technology\n");
		stiTHROW(stiRESULT_ERROR);
	}

STI_BAIL:
	return hResult;
}


/*
 * return the signal strength of the named access point
 */
int CstiNetworkBase::WirelessSignalStrengthGet(const char *ssid)
{
	int nReturnValue = 0;

	auto iter = std::find_if(m_services.begin(), m_services.end(),
					[&ssid](std::shared_ptr<NetworkService> &service)
					{return service->m_serviceName == ssid;});

	if (iter != m_services.end())
	{
		nReturnValue = (*iter)->m_signalStrength;
	}
	return (nReturnValue);
}


void CstiNetworkBase::serviceDisconnectedHandle(std::shared_ptr<NetworkService>service, bool success)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		vpe::trace ("CstiNetworkBase::", __func__, ": status = ", success, "\n");
	);

	service->m_disconnecting = false;

	if (success)
	{
		//
		// This delay seems to help reconecting	after a disconnect
		//
//		sleep (1);
		newServiceConnect (true);
	}
}


stiHResult CstiNetworkBase::newServiceConnect (bool disconnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (m_newService != nullptr, stiRESULT_ERROR);

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("CstiNetworkBase::%s:: Connecting service %s disconnected = %s\n", __func__, m_newService->m_serviceName.c_str(), torf(disconnected));
	);

	//
	// Clear the error property
	//
	m_newService->propertyClear("Error");

	//
	// Make sure we are disconnected from the service we are going to connect to.
	//
	if (!disconnected)
	{
		// After disconnection, this method (newServiceConnect) will be called.
		m_newService->m_disconnecting = true;
		m_newService->serviceDisconnect();
	}
	else
	{
		m_newService->serviceConnect();
	}

STI_BAIL:
	return hResult;
}


void CstiNetworkBase::serviceOfflineStateChangedHandle(std::shared_ptr<NetworkService>service, bool blocked)
{
	if (service == m_currentService)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: CstiNetworkBase::%s: Current service %s is offline. resetState=%d\n", __func__, service->m_serviceName.c_str(), static_cast<int>(m_resetState));
		);
		if (service->m_technologyType == technologyType::wireless)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("NetworkDebug: %s, sending wireless disconnected signal, disconnecting = %s\n", __func__, torf(service->m_disconnecting));
			);

			wirelessNetworkDisconnectedSignal.Emit();

		}

		// if this was an unexpected drop, flag the service for periodic reconnect attempts
		if (!service->m_disconnecting && m_resetState != resetState::shuttingDown)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("NetworkDebug: %s, service %s is 'lost'. We will attempt to reconnect it on intervals..\n", __func__, service->m_serviceName.c_str());
			);

			m_lostService = m_currentService->objectPathGet();
		}

		/*
		* the wiredNetworkDisconnectedSignal should only be sent when the cable is physically removed.
		* so don't do that here.
		*/

		if (blocked)
		{
			wirelessPassphraseChangedSignal.Emit();
		}
		m_currentService = nullptr;
	}
}

void CstiNetworkBase::serviceOnlineStateChangedHandle(
	std::shared_ptr<NetworkService> service)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: CstiNetworkBase::%s: Current service %s is online. resetState=%d\n", __func__, service->m_serviceName.c_str(), static_cast<int>(m_resetState));
	);
	if (m_resetState == resetState::restarting && m_waitForConnect)
	{
		m_waitForConnect = false;
		m_resetState = resetState::unknown;
		connmanResetSignal.Emit();
	}

	if (service == m_newService)
	{
		stiDEBUG_TOOL (g_stiNetworkDebug,
			stiTrace("NetworkDebug: %s: We have enough to be online, sending SETTINGS_CHANGED, and IDLE\n", service->m_serviceName.c_str ());
		);

		service->propertySet("AutoConnect", g_variant_new_variant(g_variant_new_boolean(TRUE)));
		m_currentService = service;
		m_oldService = nullptr;
		m_newService = nullptr;

		if (!m_targetService || m_targetService == m_currentService)
		{
			m_error = estiErrorNone;
			stateSet(estiIDLE);

			// disable wifi if the new connection is wired
			syncWifiPowerState();
		}
		else
		{
			m_error = estiErrorGeneric;
			stateSet(estiERROR);
		}

		networkSettingsChangedSignal.Emit();
		m_targetService = nullptr;
	}
	else if (!m_newService && !m_currentService)
	{
		stiDEBUG_TOOL (g_stiNetworkDebug,
			stiTrace("NetworkDebug: %s: No current service and state is unknown, transition to Idle\n", service->m_serviceName.c_str ());
		);
		m_currentService = service;
		stateSet(estiIDLE);

		// we just verified that the pre-existing service is online. If it's wired, we're safe to disable wifi.
		syncWifiPowerState();
		
		networkSettingsChangedSignal.Emit();
	}

	m_lostService.clear();
}

EstiServiceState CstiNetworkBase::ServiceStateGet()
{
	if (m_currentService)
	{
		return(m_currentService->m_serviceState);
	}
	return(estiServiceIdle);
}


void CstiNetworkBase::networkInfoSet (
	NetworkType type,
	const std::string &data)
{
};


NetworkType CstiNetworkBase::networkTypeGet () const
{
	auto networkType = NetworkType::None;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_currentService && (m_currentService->m_progressState & STATE_ONLINE) == STATE_ONLINE)
	{
		switch (m_currentService->m_technologyType)
		{
			case technologyType::unknown:
				networkType = NetworkType::None;
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("Unknown\n");
				);
				break;
			case technologyType::wired:
				networkType = NetworkType::Wired;
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("Wired\n");
				);
				break;
			case technologyType::wireless:
				networkType = NetworkType::WiFi;
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("Wireless\n");
				);
				break;
			case technologyType::bluetooth:
				networkType = NetworkType::None;
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("Bluetooth\n");
				);
				break;
		}
	}

	return networkType;
};


std::string CstiNetworkBase::networkDataGet () const
{
	return "";
};


void CstiNetworkBase::networkManagerCreate()
{
	auto dbusObject = m_dbusConnection->findOrCreate(this, CONNMAN_MANAGER_PATH);
	dbusObject->interfaceAdded(CONNMAN_MANAGER_INTERFACE, nullptr, NetworkDbusInterface::interfaceMake);
}

/*!
* \brief Start up the Network task
* \retval None
*/
stiHResult CstiNetworkBase::Startup ()
{
	CstiEventQueue::StartEventLoop();
	PostEvent([this]{ networkManagerCreate (); });
	return (stiRESULT_SUCCESS);
}


IstiNetwork::EstiError CstiNetworkBase::ErrorGet () const
{
	return m_error;
}


/*!
* \brief Returns the current network state
* \retval the current network state
*/
IstiNetwork::EstiState CstiNetworkBase::StateGet () const
{
	return m_stState.eState;
}


const char *CstiNetworkBase::stateNameGet(EstiState state)
{
	const char *stateStr = "unknown";
	switch (state)
	{
		case estiIDLE:
			stateStr = "estiIDLE";
			break;
		case estiNEGOTIATING:
			stateStr = "estiNEGOTIATING";
			break;
		case estiERROR:
			stateStr = "estiERROR";
			break;
		case estiUNKNOWN:
			break;
	}
	return stateStr;
}
/*!
* \brief Sets the current network state
* \retval the current network state
*/
void CstiNetworkBase::stateSet (EstiState eState)
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("NetworkDebug: Enter stateSet, current state = %s\n", stateNameGet(eState));
	);
	bool bPerformChange = false;

	switch (eState)
	{
		case estiIDLE:
			stiDEBUG_TOOL(g_stiNetworkDebug > 1,
				stiTrace("NetworkDebug: changing to estiIDLE, count = %d\n", m_stState.nNegotiateCnt);
			);
			// If we are currently negotiating, reduce the negotiating count by one.  If we are
			// back to 0, then change the state.
//			if ((estiNEGOTIATING == m_stState.eState && --m_stState.nNegotiateCnt == 0)
//			 || estiUNKNOWN == m_stState.eState)
			if (estiIDLE != m_stState.eState)
			{
				bPerformChange = true;
			}
			break;

		case estiNEGOTIATING:
			stiDEBUG_TOOL(g_stiNetworkDebug > 1,
				stiTrace("NetworkDebug: changing to estiNEGOTIATING, count = %d\n", m_stState.nNegotiateCnt);
			);
			// If we aren't already in the negotiating state, perform the change.
			if (estiNEGOTIATING != m_stState.eState)
			{
				bPerformChange = true;
			}
			// increment the counter in any case
			m_stState.nNegotiateCnt++;
			break;

		case estiERROR:
			stiDEBUG_TOOL(g_stiNetworkDebug > 1,
				stiTrace("NetworkDebug: changing to estiERROR, count = %d\n", m_stState.nNegotiateCnt);
			);
			// If we aren't already in the error state, return the count to 0 and
			// allow the change.
			if (estiERROR != m_stState.eState)
			{
				m_stState.nNegotiateCnt = 0;
				bPerformChange = true;
			}
			break;

		case estiUNKNOWN:

			stiASSERT (false);

			break;
	}

	if (bPerformChange)
	{
		stiDEBUG_TOOL (g_stiNetworkDebug,
			stiTrace ("NetworkDebug: Changing state from %s to %s\n",
			stateNameGet(m_stState.eState),
			stateNameGet(eState));
		);
		m_stState.eState = eState;

		// Notify the application of the state change
		networkStateChangedSignal.Emit(eState);
	}
}

/*
 * return whether or not the ethernet cable is connected
 */
stiHResult CstiNetworkBase::EthernetCableStatusGet (bool *pbConnected)
{
	*pbConnected = m_bCableConnected;
	return (stiRESULT_SUCCESS);
}


/*!
* \brief Return which network is used.
* \retval int
*/
int CstiNetworkBase::WirelessInUse()
{
	int nRet = USE_HARDWIRED_NETWORK_DEVICE;

	if (m_currentService && technologyType::wireless == m_currentService->m_technologyType)
	{
		nRet = USE_WIRELESS_NETWORK_DEVICE;
	}

	return nRet;
}


/*!
* \brief Return a list of available WAPs
* \retval stiHResult
*/
stiHResult CstiNetworkBase::WAPListGet (WAPListInfo* list) const
{
	SstiSSIDListEntry entry{};

	for (auto iter = m_services.begin(); iter != m_services.end(); iter++)
	{
		if ((*iter)->m_technologyType != technologyType::wireless)
		{
			continue;
		}

        /* Skip hidden SSIDs */
		if ((*iter)->isHidden ())
		{
			continue;
		}

		strcpy(entry.SSID, (*iter)->m_serviceName.c_str());
		entry.SignalStrength = (*iter)->m_signalStrength;
		entry.bImmutable = (*iter)->m_immutable;
		entry.Security = (*iter)->m_securityType;
		list->push_back(entry);
	}
	return (stiRESULT_SUCCESS);
}

/*!
* \brief Determine if the wireles driver is available
* \retval stiHResult
*/
bool CstiNetworkBase::WirelessAvailable () const
{

	bool returnValue = false;

	auto iter = std::find_if(m_technologies.begin(), m_technologies.end(),
				[](const std::shared_ptr<NetworkTechnology> &technology)
					{return technology->m_technologyType == technologyType::wireless;});

	if (iter != m_technologies.end())
	{
		returnValue = (*iter)->m_powered;
	}
	return (returnValue);
}


/*!
* \brief Set the in-call status
* \retval none
*/
void CstiNetworkBase::InCallStatusSet (bool bSet)
{
	// ACTION - NETWORK - Need to implement.
}

static const uint8_t un8IP_OCTET_MAX = 255;		// The largest number valid in any
													// segment of an IP address.
static const int nIP_OCTET_LENGTH = 3;				// Longest length of an IP octet

stiHResult IPAddressToIntegerConvert (
	const char *pszAddress,	///< The string containing the IP address to convert.
	uint32_t *pun32Value); ///< The value of the IP address as an integer

static bool IPOctetValidate (
	const char *pszOctet);

/*! \brief Check if IP Octet value is valid
*
* The Octet value must be between 0 and 255.
* \retval true If the octet value is between 0 and 255.
* \retval false Otherwise.
*/
static bool IPOctetValidate (
	const char *pszOctet)
{
	bool bValid = false;
	size_t Length = strlen (pszOctet);
	size_t i;

	if (Length >= 1 && Length <= (size_t)nIP_OCTET_LENGTH)
	{
		// Look at each character in the segment
		for (i = 0; i < Length; i++)
		{
			// Is the current character not a digit
			if (!isdigit (pszOctet[i]))
			{
				// yes! - not a valid ip address.
				break;
			} // end if
		} // end for

		// Is the number in this segment less than 256?
		if (i == Length && atoi (pszOctet) <= un8IP_OCTET_MAX)
		{
			bValid = true;
		} // end if
	}

	return bValid;
}


///!\brief Converts an integer into an IP address string
///
///\returns The IP address in integer form, zero if the IP address is invalid
///
stiHResult IPAddressToIntegerConvert (
	const char *pszAddress,	///< The string containing the IP address to convert.
	uint32_t *pun32Value) ///< The value of the IP address as an integer
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32IPAddress = 0;

	// Did we get a valid pointer?
	stiTESTCOND (pszAddress, stiRESULT_ERROR);

	{
		int nBits = 24;
		const char *pszDotPosition;
		const char *pszPrevPosition = pszAddress;

		// Is there a period in the number?
		while (nBits > 0 && nullptr != (pszDotPosition = strchr (pszPrevPosition, '.')))
		{
			// Is this segment less than 3 digits and greater than 0 digits?
			size_t Length = pszDotPosition - pszPrevPosition;

			stiTESTCOND((size_t)nIP_OCTET_LENGTH >= Length && 0 < Length, stiRESULT_ERROR);

			char szSegment[nIP_OCTET_LENGTH + 1];
			strncpy (szSegment, pszPrevPosition, Length);
			szSegment[Length] = '\0';

			bool bResult = IPOctetValidate (szSegment);

			stiTESTCOND(bResult, stiRESULT_ERROR);

			uint32_t un32Value = atoi (szSegment);
			un32IPAddress |= (un32Value << nBits);
			nBits -= 8;

			pszPrevPosition = pszDotPosition + 1;
		} // end while

		stiTESTCOND (nBits == 0, stiRESULT_ERROR);

		//
		// Do we need to shift on the last octet
		// and is the last octet valid?
		//
		bool bResult = IPOctetValidate (pszPrevPosition);
		stiTESTCOND (bResult, stiRESULT_ERROR);

		// Yes.
		uint32_t un32Value = atoi (pszPrevPosition);
		un32IPAddress |= (un32Value << nBits);
	}  // end if

STI_BAIL:

	if (!stiIS_ERROR (hResult))
	{
		*pun32Value = un32IPAddress;
	}

	return hResult;
}

/*!
* \brief Validates the IP address.
*/
stiHResult IPValidate (
	const char *pszIPAddress,
	const char *pszSubnetMask)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32IPAddress;
	uint32_t un32SubnetMask;
	hResult = IPAddressToIntegerConvert (pszIPAddress, &un32IPAddress);
	stiTESTRESULT ();
	//
	// Make sure the most significant octet is between 1 and 223, inclusive.
	//
	stiTESTCOND ((un32IPAddress & 0xFF000000) > 0 && (un32IPAddress & 0xFF000000) <= 0xDF000000,
				 stiRESULT_ERROR);
	//
	// Make sure the host address is not zero or all ones.
	//
	hResult = IPAddressToIntegerConvert (pszSubnetMask, &un32SubnetMask);
	stiTESTRESULT ();

	stiTESTCOND ((un32IPAddress & ~un32SubnetMask) != 0, stiRESULT_ERROR);
	stiTESTCOND ((un32IPAddress & ~un32SubnetMask) != ~un32SubnetMask, stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}

/*!
* \brief Validates the IP address.
*/
stiHResult DNSIPValidate (
	const char *pszIPAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32IPAddress;

	hResult = IPAddressToIntegerConvert (pszIPAddress, &un32IPAddress);
	stiTESTRESULT ();
	//
	// Make sure the most significant octet is between 1 and 223, inclusive.
	//
	stiTESTCOND ((un32IPAddress & 0xFF000000) > 0 && (un32IPAddress & 0xFF000000) <= 0xDF000000,
				 stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}

static stiHResult DNSValidate (
	const char *pszDNS1,
	const char *pszDNS2,
	IstiNetwork::EstiError *peError)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (pszDNS1 && *pszDNS1)
	{
		hResult = DNSIPValidate(pszDNS1);
		if (stiIS_ERROR (hResult))
		{
			*peError = IstiNetwork::estiErrorDNS1;
			stiTHROW (stiRESULT_ERROR);
		}
	}

	if (pszDNS2 && *pszDNS2)
	{
		hResult = DNSIPValidate(pszDNS2);
		if (stiIS_ERROR (hResult))
		{
			*peError = IstiNetwork::estiErrorDNS2;
			stiTHROW (stiRESULT_ERROR);
		}
	}

STI_BAIL:
	return hResult;
}

/*!
* \brief Validates the subnet mask.
*/
static stiHResult SubnetMaskValidate (
	const char *pszSubnetMask)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32SubnetMask;
	//
	// Make sure the one's complement of the subnet mask plus one is a power of 2.
	//
	hResult = IPAddressToIntegerConvert (pszSubnetMask, &un32SubnetMask);
	stiTESTRESULT ();

	stiTESTCOND (un32SubnetMask != 0, stiRESULT_ERROR);
	stiTESTCOND ((((~un32SubnetMask) + 1) & (~un32SubnetMask)) == 0, stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}


/* \brief Utility method build a path to an SSID's config file
 *
 */
stiHResult CstiNetworkBase::eapPathGet (
	const char *ssid,
	std::string *eapPath)
{

	*eapPath = m_connmanDirName + ssid + ".config";

	return stiRESULT_SUCCESS;
}


/*
 * there are some values that need to be prompted for and
 * saved if we are connecting to an EAP.
 */
stiHResult CstiNetworkBase::enterpriseValuesSet ()
{
	const char *eap;
	const char *phase2;
	FILE *file = nullptr;
	std::string eapPath;

	stiHResult hResult = stiRESULT_SUCCESS;

	switch (m_tmpSettings.eEapType)
	{
		case estiEAP_PEAP:
			eap = "peap";
			break;
		case estiEAP_TLS:
			eap = "tls";
			break;
		case estiEAP_TTLS:
			eap = "ttls";
			break;
		default:
			// this shouldn't happen;
			stiTHROW(stiRESULT_ERROR);
	}

	switch (m_tmpSettings.ePhase2Type)
	{
		case estiPHASE2_MSCHAPV2:
			phase2 = "MSCHAPV2";
			break;
		case estiPHASE2_GTC:
			phase2 = "GTC";
			break;
		case estiPHASE2_TLS:
			phase2 = "TLS";
			break;
		default:
			// this shouldn't happen
			stiTHROW(stiRESULT_ERROR);
	}

	hResult = eapPathGet (m_tmpSettings.szSsid, &eapPath);
	stiTESTRESULT ();

	if ((file = fopen(eapPath.c_str(), "w+")) == nullptr)
	{
		stiTrace("enterpriseValuesSet: Unable to open enterprise config file %s\n", eapPath.c_str());
		stiTHROW(stiRESULT_UNABLE_TO_OPEN_FILE);		
	}

	fprintf(file, "[service_%s]\nType = wifi\nName = %s\nEAP = %s\nPhase2 = %s\n",
		m_tmpSettings.szSsid, m_tmpSettings.szSsid, eap, phase2);

STI_BAIL:

	if (file)
	{
		fclose(file);
		file = nullptr;
	}

	return (hResult);
}


std::string CstiNetworkBase::localIpAddressGet(
	EstiIpAddressType addressType,
	const std::string& destinationIpAddress) const
{
	std::string ipAddress;

	stiASSERT(addressType == estiTYPE_IPV4);

	if (addressType == estiTYPE_IPV4)
	{
		SstiNetworkSettings settings;

		auto hResult = SettingsGet (&settings);

		if (!stiIS_ERROR(hResult))
		{
			ipAddress = settings.IPv4IP;
		}
	}

	return ipAddress;
}


/*! 
 * \struct SstiNetworkSettings 
 *  
 * \brief Return the settings of the current service.
 *  
 */
stiHResult CstiNetworkBase::SettingsGet (SstiNetworkSettings *pstSettings) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: Enter SettingsGet\n");
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (!m_currentService || (m_currentService->m_progressState & STATE_ONLINE) != STATE_ONLINE)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("No service or not online: return default settings\n");
		);

		pstSettings->bIPv4UseDHCP = true;		// default this to true....
		pstSettings->IPv4IP = "";
		pstSettings->IPv4SubnetMask = "";
		pstSettings->IPv4NetGatewayIP = "";
		pstSettings->IPv4DNS1 = "";
		pstSettings->IPv4DNS2 = "";
		pstSettings->eIPv6Method = estiIPv6MethodAutomatic;
		pstSettings->IPv6IP = "";
		pstSettings->nIPv6Prefix = 0;
		pstSettings->IPv6GatewayIP = "";
		pstSettings->IPv6DNS1 = "";
		pstSettings->IPv6DNS2 = "";
		pstSettings->eNetwork = NetworkType::None;
		pstSettings->eWirelessNetwork = estiWIRELESS_OPEN;
		*pstSettings->szSsid =  '\0';
	}
	else
	{
		pstSettings->bIPv4UseDHCP = m_currentService->m_ipv4.method == NetworkService::methodDhcp;
		pstSettings->IPv4IP = m_currentService->m_ipv4.address;
		pstSettings->IPv4SubnetMask = m_currentService->m_ipv4.netmask;
		pstSettings->IPv4NetGatewayIP = m_currentService->m_ipv4.gateway;

		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: %s: IPv4 settings - DHCP = %d, IPv4IP = %s, Subnet = %s, Gateway = %s\n",
					m_currentService->m_serviceName.c_str (),
					pstSettings->bIPv4UseDHCP,
					pstSettings->IPv4IP.c_str(),
					pstSettings->IPv4SubnetMask.c_str(),
					pstSettings->IPv4NetGatewayIP.c_str());
		);

		/*
		 * connman doesn't have any concept of an IPv4 or IPv6 DNS address.  there are just nameservers.
		 * If we get both IPv6 and IPv4 addresses here, filter them appropriately.
		 */
		if (!m_currentService->m_nameServers.empty())
		{
			std::string *pString = &pstSettings->IPv4DNS1;
			for (unsigned int i = 0, j = 0; i < m_currentService->m_nameServers.size(); i++)
			{
				if (IPv4AddressValidate(m_currentService->m_nameServers[i].c_str()))
				{
					*pString = m_currentService->m_nameServers[i];
					stiDEBUG_TOOL(g_stiNetworkDebug,
						stiTrace("NetworkDebug: %s: Nameserver %d = %s\n", m_currentService->m_serviceName.c_str (), j, pString->c_str());
					);
					j++;
					if (j == 1)
					{
						pString = &pstSettings->IPv4DNS2;
					}
					else
					{
						break;
					}
				}
			}
		}

		if (m_useIpv6)
		{
			pstSettings->eIPv6Method = m_currentService->m_ipv4.method == NetworkService::methodAuto ? estiIPv6MethodAutomatic : estiIPv6MethodManual;
			pstSettings->IPv6IP = m_currentService->m_ipv6.address;
//	pstSettings->IPv6Zone			// not used
			pstSettings->nIPv6Prefix = m_currentService->m_ipv6.prefixLength;
			pstSettings->IPv6GatewayIP = m_currentService->m_ipv6.gateway;
//			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("IPv6Method = %d, IPv6IP = %s, Prefix = %d, Gateway = %s\n", pstSettings->eIPv6Method,
							pstSettings->IPv6IP.c_str(), pstSettings->nIPv6Prefix, pstSettings->IPv6GatewayIP.c_str());
//			);
		}

		/*
		 * filter DNS address as above, only for IPv6.
		 */
		if (m_useIpv6 && !m_currentService->m_nameServers.empty())
		{
			std::string *pString = &pstSettings->IPv6DNS1;
			for (unsigned int i = 0, j = 0; i < m_currentService->m_nameServers.size(); i++)
			{
				if (IPv6AddressValidate(m_currentService->m_nameServers[i].c_str()))
				{
					*pString = m_currentService->m_nameServers[i];
					stiDEBUG_TOOL(g_stiNetworkDebug,
						stiTrace("NetworkDebug: %s, Nameserver %d = %s\n", m_currentService->m_serviceName.c_str (), j, pString->c_str());
					);
					j++;
					if (j == 1)
					{
						pString = &pstSettings->IPv6DNS2;
					}
					else
					{
						break;
					}
				}
			}
		}

		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: %s: TechnologyType = ", m_currentService->m_serviceName.c_str ());
		);

		pstSettings->eNetwork = networkTypeGet ();

		pstSettings->eWirelessNetwork = m_currentService->m_securityType;

		if (m_currentService->m_serviceName == "Wired")
		{
			memset(pstSettings->szSsid, '\0', sizeof(pstSettings->szSsid));
		}
		else
		{
			strcpy(pstSettings->szSsid, m_currentService->m_serviceName.c_str());
		}

//	pstSettings->szKey				// not used
//	pstSettings->szKeyIndex			// not used
//	pstSettings->szAuthentication	// not used
//	pstSettings->szEncryption		// not used
//	pstSettings->szIdentity			// not used
//	pstSettings->szPrivateKeyPasswd	// not used
	}

	return (hResult);
}

/*
 * return the pointer to the named service, or else if there are hidden
 * services and the name doesn't match anything, assume they want the
 * hidden service
 */
std::shared_ptr<NetworkService> CstiNetworkBase::serviceGet(
	std::string name,
	EstiWirelessType type)
{
	std::string securityType;
	std::shared_ptr<NetworkService>tmpService;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("serviceGet - looking for %s\n", name.c_str());
	);

	if (name == "Wired")
	{
		securityType = "none";
		type = estiWIRELESS_UNDEFINED;
	}
	else
	{
		switch (type)
		{
			case estiWIRELESS_UNDEFINED:
				stiDEBUG_TOOL(g_stiNetworkDebug,
						stiTrace("security type is undefined\n");
				);
				securityType = "undefined\n";
				break;

			case estiWIRELESS_OPEN:
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("none\n");
				);
				securityType = "none";
				break;

			case estiWIRELESS_WEP:
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("wep\n");
				);
				securityType = "wep";
				break;

			case estiWIRELESS_WPA:
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("wpa/psk\n");
				);
				securityType = "psk";
				break;

			case estiWIRELESS_EAP:
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("eap\n");
				);
				securityType = "ieee8021x";
				break;
		}
	}

	for (auto iter = m_services.begin(); iter != m_services.end(); iter++)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace ("Service Name %s, Security: %d\n", (*iter)->m_serviceName.c_str(), (*iter)->m_securityType);
		);

		if ((*iter)->m_serviceName == name)
		{
			if ((*iter)->m_technologyType == technologyType::wireless)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					vpe::trace("service: ", (*iter)->m_serviceName, " is wireless: returning found\n");
				);
				tmpService = *iter;
				break;
			}

			if (type == estiWIRELESS_UNDEFINED || type == (*iter)->m_securityType)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("returning known name %s\n", name.c_str());
				);

				tmpService = *iter;
				break;
			}
		}

		if ((*iter)->isHidden () && type == (*iter)->m_securityType)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("setting hidden with ssid - %s, Security = %s\n", (*iter)->objectPathGet().c_str(), securityType.c_str());
			);
			tmpService = *iter;
		}
	}

	return (tmpService);
}

/*
 * check to see if the SettingsSet will actually change anything in our current settings
 */
bool CstiNetworkBase::settingsChanged()
{
	bool result = false;

	if (!m_currentService.get())
	{
		result = true;
	}
	else
	{
		if (m_tmpSettings.bUseIPv4)
		{
			if (m_tmpSettings.bIPv4UseDHCP)
			{
				result = m_currentService->m_ipv4.method != NetworkService::methodDhcp;
			}
			else
			{
				if (m_currentService->m_ipv4.method == NetworkService::methodDhcp)
				{
					result = true;
				}
				else if ((m_tmpSettings.IPv4IP.length() && m_currentService->m_ipv4.address != m_tmpSettings.IPv4IP) ||
					(m_tmpSettings.IPv4SubnetMask.length() && m_currentService->m_ipv4.netmask != m_tmpSettings.IPv4SubnetMask) ||
					(m_tmpSettings.IPv4NetGatewayIP.length() && m_currentService->m_ipv4.gateway != m_tmpSettings.IPv4NetGatewayIP))
				{
					result = true;
				}
				else
				{
					//
					// Check to see if the number of DNS servers is different.
					//
					int nNumDnsServers = 0;
	
					if (m_tmpSettings.IPv4DNS1.length ())
					{
						nNumDnsServers++;
					}

					if (m_tmpSettings.IPv4DNS2.length ())
					{
						nNumDnsServers++;
					}

					if (std::min(m_currentService->m_nameServers.size(), (size_t)2) != (unsigned)nNumDnsServers)
					{
						result = true;
					}
					else
					{
						//
						// Check to see if any of the name servers have changed.
						//
						for (unsigned int i = 0; i < std::min(m_currentService->m_nameServers.size(), (size_t)2); i++)
						{
							if ((i == 0 && m_tmpSettings.IPv4DNS1 != m_currentService->m_nameServers[i])
						 	|| (i == 1 && m_tmpSettings.IPv4DNS2 != m_currentService->m_nameServers[i]))
							{
								result = true;
								break;
							}
						}
					}
				}
			}
		}
		else if (m_tmpSettings.bUseIPv6)
		{
			if (m_tmpSettings.eIPv6Method && m_currentService->m_ipv6.method != NetworkService::methodAuto)
			{
				result = true;
			}
			else if (!m_tmpSettings.eIPv6Method)
			{
#if 0
				// how does this get set if the method is not auto????
				if (m_currentService->sIpv6.eMethod == estiMethodDhcp)
				{
					result = true;
				}
				else
#endif
				if ((m_tmpSettings.IPv6IP.length() && m_currentService->m_ipv6.address != m_tmpSettings.IPv6IP) ||
					(m_tmpSettings.nIPv6Prefix != m_currentService->m_ipv6.prefixLength) ||
					(m_tmpSettings.IPv6GatewayIP.length() && m_currentService->m_ipv6.gateway != m_tmpSettings.IPv6GatewayIP))
				{
					result = true;
				}
			}
		}
	}

	return result;
}


/*!
* \brief fires of an event that will change the network settings
*/
stiHResult CstiNetworkBase::SettingsSet (
	const SstiNetworkSettings *newSettings, 	// < A structure containing the network settings to be applied.
	unsigned int *requestId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Enter SettingsSet\n");
	);

	if (newSettings)
	{
		std::lock_guard<std::recursive_mutex> lock(m_execMutex);

		++m_nextRequestId;
		if (m_nextRequestId == 0)
		{
			++m_nextRequestId;
		}

		m_tmpSettings = *newSettings;
		PostEvent(std::bind(&CstiNetworkBase::eventSettingsSet, this, m_nextRequestId));

		if (requestId)
		{
			*requestId = m_nextRequestId;
		}
	}
	
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Exit SettingsSet\n");
	);
	return hResult;
}

#define GET_CASE_STRING(s) case s: caseString=#s; break;
static const char *connErr2String(connectionError error)
{

	const char *caseString = nullptr;

	switch(error)
	{
		GET_CASE_STRING(connectionError::none);
		GET_CASE_STRING(connectionError::inProgress);
		GET_CASE_STRING(connectionError::alreadyConnected);
		GET_CASE_STRING(connectionError::notConnected);
		GET_CASE_STRING(connectionError::ioError);
		GET_CASE_STRING(connectionError::other);
	}
	return caseString;
}


/*!
* \brief Changes the settings for the given network service
*/
void CstiNetworkBase::eventSettingsSet (unsigned int nextRequestId)
{

	stiHResult hResult = stiRESULT_SUCCESS;

	GVariantBuilder *builder = nullptr;
	std::shared_ptr<NetworkService> tmpService;
	std::string tmpStr;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Enter EventSettingsSet, current state = %s\n", stateNameGet(m_stState.eState));
	);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	if (m_tmpSettings.eNetwork == NetworkType::Wired)
	{
		tmpStr = "Wired";
	}
	else
	{
		tmpStr = m_tmpSettings.szSsid;
	}

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("setting security = %d\n", m_tmpSettings.eWirelessNetwork);
	);

	tmpService = serviceGet(tmpStr, m_tmpSettings.eWirelessNetwork);

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("return from serviceGet: %p\n", tmpService.get());
	);

	if (!tmpService)
	{
		stiTrace("Trying to change settings on unknown service: %s\n", tmpStr.c_str());
		stiTHROW(stiRESULT_ERROR);
	}

	/*
	 * if this is an enterprise system, we will need to set the type and phase2
	 *         both need to be set!
	 */
	if (m_tmpSettings.eNetwork == NetworkType::WiFi &&
		tmpService->m_securityType == estiWIRELESS_EAP &&
		m_tmpSettings.eEapType && m_tmpSettings.ePhase2Type)
	{
		hResult = enterpriseValuesSet();
		stiTESTRESULT();
		tmpService->m_immutable = true;
	}

	/*
	 * setting the current service to the same settings????
	 */
	if (tmpService != m_currentService || settingsChanged() || m_stState.eState == estiERROR)
	{
		/*
		 * we are going to re-negotiate the settings on our current connection
		 */
		if (tmpService == m_currentService)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("NetworkDebug: re-negotiating our current service\n");
			);
			stateSet(estiNEGOTIATING);
		}

		/*
		 * if we aren't using DHCP we need to set the properties for the
		 * connection.
		 */
		if (m_tmpSettings.bUseIPv4)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("NetworkDebug: EventSettingsSet: bUseIPv4\n");
			);

			if (!m_tmpSettings.bIPv4UseDHCP)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("NetworkDebug: EventSettingsSet: -NOT- DHCP\n");
				);

				if (tmpService->m_immutable)
				{
					stiTrace("Attempting to change an immutable interface\n");
					stiTHROW(stiRESULT_INVALID_PARAMETER);
				}

				hResult = SubnetMaskValidate (m_tmpSettings.IPv4SubnetMask.c_str ());
				if (stiIS_ERROR (hResult))
				{
					m_error = estiErrorSubnetMask;
					stiTHROW (hResult);
				}

				hResult = IPValidate (m_tmpSettings.IPv4IP.c_str (), m_tmpSettings.IPv4SubnetMask.c_str ());
				if (stiIS_ERROR (hResult))
				{
					m_error = estiErrorIP;
					stiTHROW (hResult);
				}

				hResult = IPValidate(m_tmpSettings.IPv4NetGatewayIP.c_str(), m_tmpSettings.IPv4SubnetMask.c_str ());
				if (stiIS_ERROR(hResult))
				{
					m_error = estiErrorIP;
					stiTHROW(hResult);
				}

				hResult = DNSValidate (m_tmpSettings.IPv4DNS1.c_str (), m_tmpSettings.IPv4DNS2.c_str (), &m_error);
				stiTESTRESULT ();

				tmpService->disableDHCP (m_tmpSettings);
			}
			else
			{
				/*
				 * DHCP. clear the DNS settings so that they can be set by the DHCP server
				 */
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("useDHCP is set\n");
				);

				if (!tmpService->m_immutable)
				{
					tmpService->enableDHCP ();
				}

				m_tmpSettings.IPv4IP.clear ();
				m_tmpSettings.IPv4SubnetMask.clear ();
				m_tmpSettings.IPv4NetGatewayIP.clear ();
			}
		}

		/*
		 * if we aren't using DHCP we need to set the properties for the
		 * connection.
		 */
		if (m_tmpSettings.bUseIPv6)
		{
			m_useIpv6 = true;
			if (m_tmpSettings.eIPv6Method == estiIPv6MethodManual)
			{
				builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(builder, "{sv}", "Method", g_variant_new_string("manual"));
				if (!m_tmpSettings.IPv6IP.empty ())
				{
					if (!IPv6AddressValidate(m_tmpSettings.IPv6IP.c_str()))
					{
						stiTrace("Invalid IPv6 Address - %s\n", m_tmpSettings.IPv6IP.c_str());
						m_error = estiErrorIP;
						stiTHROW(stiRESULT_ERROR);
					}
					g_variant_builder_add(builder, "{sv}", "Address", g_variant_new_string(m_tmpSettings.IPv6IP.c_str ()));
				}
				if (!m_tmpSettings.IPv6GatewayIP.empty ())
				{
					g_variant_builder_add(builder, "{sv}", "Gateway", g_variant_new_string(m_tmpSettings.IPv6GatewayIP.c_str ()));
				}

				tmpService->propertySet("IPv6.Configuration", g_variant_new_variant(g_variant_builder_end(builder)));

				g_variant_builder_unref(builder);
				builder = nullptr;

				const char *nameservers[3]{};

				int i = 0;
				if (!m_tmpSettings.IPv6DNS1.empty())
				{
					nameservers[i++] = m_tmpSettings.IPv6DNS1.c_str();
				}

				if (!m_tmpSettings.IPv6DNS2.empty())
				{
					nameservers[i++] = m_tmpSettings.IPv6DNS2.c_str();
				}

				if (i)
				{
					tmpService->propertySet("Nameservers.Configuration",
							g_variant_new_variant(g_variant_new_strv(nameservers, i)));
				}
			}
			else
			{
				/*
				 * DHCP. clear the DNS settings so that they can be set by the DHCP server
				 */
				char *nullServers [1] = { nullptr };

				tmpService->propertySet( "Nameservers.Configuration",
						g_variant_new_variant(g_variant_new_strv(nullServers, 0)));

				m_tmpSettings.IPv6IP.clear();
				m_tmpSettings.nIPv6Prefix = 0;
	//			m_tmpSettings.IPv6Zone.clear();			// not used
				m_tmpSettings.IPv6GatewayIP.clear ();

				builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(builder, "{sv}", "Method", g_variant_new_string("auto"));
				g_variant_builder_add(builder, "{sv}", "Address", g_variant_new_string(""));
				g_variant_builder_add(builder, "{sv}", "Netmask", g_variant_new_string(""));
				g_variant_builder_add(builder, "{sv}", "Gateway", g_variant_new_string(""));

				tmpService->propertySet( "IPv6.Configuration",
						g_variant_new_variant(g_variant_builder_end(builder)));

				g_variant_builder_unref(builder);
				builder = nullptr;
			}
		}
		else
		{
			m_useIpv6 = false;
		}
	}

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("EventSettingsSet: sending networkSettingsSetSignal\n");
	);

	m_error = estiErrorNone;
	networkSettingsSetSignal.Emit();

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("EventSettingsSet, sending NETWORK_SETTINGS_ERROR\n");
		);

		if (m_error == estiErrorNone)
		{
			m_error = estiErrorGeneric;
		}

		stiDEBUG_TOOL(g_stiNetworkDebug,
			vpe::trace("NetworkDebug: ", __func__, ": emitting networkSettingsErrorSignal\n");
		);

		networkSettingsErrorSignal.Emit();
		stateSet(estiERROR);
	}

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: return from EventSettingsSet\n");
	);
}


/*!
 * \brief set up an event that will connect the named service
 */
stiHResult CstiNetworkBase::ServiceConnect(
	NetworkType networkType,
	const char *ssid,
	int securityType)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Enter ServiceConnect, ssid = %s, nType = %d\n", ssid ? ssid : "nil", securityType);
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::string tmpSsid;

	if (networkType == NetworkType::Wired)
	{
		tmpSsid = "Wired";
	}
	else if (networkType == NetworkType::WiFi && ssid)
	{
		tmpSsid = ssid;
	}
	else
	{
		stiTHROWMSG(stiRESULT_INVALID_PARAMETER, "networkType = %d, SSID = %s", networkType, ssid);
	}

	PostEvent(std::bind(&CstiNetworkBase::eventServiceConnect, this, securityType, tmpSsid));

STI_BAIL:

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Exit ServiceConnect\n");
	);

	return hResult;
}


void CstiNetworkBase::eventServiceConnect(
	int securityType,
	std::string ssid)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("Enter EventServiceConnect\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto  wirelessType = static_cast<EstiWirelessType>(securityType);

	std::string eapPath;
	struct stat sbuff{};
	std::shared_ptr<NetworkService> service;

	stiTESTCOND (!ssid.empty(), stiRESULT_ERROR);

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("calling serviceGet, ssid = %s, security = %d\n", ssid.c_str(), securityType);
	);

	service = serviceGet(ssid, wirelessType);
	if (!service)
	{
		m_error = estiErrorGeneric;
		stateSet(estiERROR);
		stiTHROWMSG(stiRESULT_ERROR, "Service = %s", ssid.c_str ());
	}

	//
	// If this is a hidden service then change the name of the service to the requested service.
	//
	if (service->isHidden ())
	{
		service->m_serviceName = ssid;
	}

	hResult = eapPathGet (ssid.c_str(), &eapPath);
	stiTESTRESULT ();

	if (service->m_securityType != estiWIRELESS_EAP || stat(eapPath.c_str(), &sbuff) >= 0)
	{
		m_targetService = service;
		hResult = doConnect(service);
		stiTESTRESULT ();
	}
	else
	{
		enterpriseNetworkSignal.Emit();
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		m_targetService = nullptr;
	}
}


stiHResult CstiNetworkBase::doConnect (
	std::shared_ptr<NetworkService> service)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (service != nullptr, stiRESULT_ERROR);

	m_newService = service;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: m_newService = %s (%p)\n"
				 "NetworkDebug: m_currentService = %s (%p)\n",
		m_newService->m_serviceName.c_str(), m_newService.get(),
		m_currentService ? m_currentService->m_serviceName.c_str() : "nil", m_currentService.get());
	);

	//
	// m_cancelled here can be true if they hit the cancel button -really- fast
	//
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: determine whether or not to disconnect new = %p, curr = %p, cancelled = %s, state = %d\n",
					m_newService.get(), m_currentService.get(), torf(m_newService->m_cancelled),
					m_currentService ? m_currentService->m_serviceState : -2);
	);

	if (!m_newService->m_cancelled)
	{
		stateSet(estiNEGOTIATING);
	}

	m_oldService = m_currentService;

	if (m_newService == m_currentService || m_newService == m_oldService)
	{
		m_oldService = nullptr;
	}

	//
	// If we have a current service then disconnect it
	//
	if (m_newService == m_currentService && !settingsChanged())
	{
		stateSet(estiIDLE);
		networkSettingsChangedSignal.Emit();
	}
	else if (m_currentService)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: Disconnecting service %s\n", m_currentService->m_serviceName.c_str());
		);

		//
		// Set the old service not to be auto-connect, don't wait for the return.  assume it works
		// then disconnect it
		//
		m_currentService->propertySet("AutoConnect", g_variant_new_variant(g_variant_new_boolean(FALSE))),
		m_newService->m_cancelled = false;
		m_currentService->m_disconnecting = true;
		m_currentService->serviceDisconnect();
		m_currentService = nullptr;
	}
	else
	{
		bool val = m_newService->m_cancelled || m_currentService == nullptr;
		m_newService->m_cancelled = false;
		hResult = newServiceConnect (val);
		stiTESTRESULT();
	}

STI_BAIL:

	return(hResult);
}


void CstiNetworkBase::clearCredentialsInvocation ()
{
	//
	// Respond to the needs credentials if we have one.
	//
	if (m_needsCredentialsInvocation)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("%s: has credentials invocation, clearing\n", __func__);
		);
		g_dbus_method_invocation_return_value(m_needsCredentialsInvocation, g_variant_new("(a{sv})", NULL));
		g_object_unref (m_needsCredentialsInvocation);
		m_needsCredentialsInvocation = nullptr;
		m_needsCredentialsService = nullptr;
	}
}


void CstiNetworkBase::usePreviousService ()
{
	if (m_oldService)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: CstiNetworkBase::%s: attempting to connect to previous service: %s\n", __func__, m_oldService->m_serviceName.c_str());
		);

		m_targetService = nullptr;
		PostEvent(std::bind(&CstiNetworkBase::doConnect, this, m_oldService));
	}
	else
	{
		stateSet(estiERROR);
		m_error = estiErrorGeneric;
		m_currentService = nullptr;
		m_oldService = nullptr;
		m_newService = nullptr;
		m_targetService = nullptr;
	}
}


void CstiNetworkBase::serviceConnectedHandle (
	std::shared_ptr<NetworkService> service)
{
	stiDEBUG_TOOL( g_stiNetworkDebug,
		vpe::trace("CstiNetworkBase::", __func__, ": cancelled = ", service->m_cancelled, "\n");
	);

	//
	// If the connection request was canceled or if we received and unexpected error
	// from connman then clean things up by:
	// 1. Release an "needs credentials" invocation
	// 2. Send a disconnect request on the  new service (yes, we are disconnected but this seems to further clean things up on the connman side)
	// 3. Turn off auto-connect for the service
	// 4. If there was a previous service then try to reconnect to it. Otherwise set an error state.
	//
	if (service->m_cancelled)
	{
		clearCredentialsInvocation ();
		usePreviousService ();
	}
	else if (service->m_technologyType == technologyType::wireless)
	{
		std::list<std::tuple<std::string, std::string>>::iterator iter;
		for (iter = m_rememberedServices.begin(); iter != m_rememberedServices.end(); iter++)
		{
			if (std::get<0>(*iter) == service->m_serviceName)
			{
				break;
			}
		}
		if (iter == m_rememberedServices.end())
		{
			m_rememberedServices.push_back(std::make_tuple(service->m_serviceName, basename(service->objectPathGet().c_str())));
		}
	}

	m_newPassphrase = false;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: exit CstiNetworkBase::serviceConnectedHandle finish\n");
	);
}


void CstiNetworkBase::serviceConnectionErrorHandle (
	std::shared_ptr<NetworkService> service,
	connectionError error)
{
	stiDEBUG_TOOL( g_stiNetworkDebug,
		stiTrace("CstiNetworkBase::%s: error = %s, cancelled = %s\n", __func__, connErr2String(error), torf(service->m_cancelled));
	);

	//
	// If the connection request was canceled or if we received and unexpected error
	// from connman then clean things up by:
	// 1. Release an "needs credentials" invocation
	// 2. Send a disconnect request on the  new service (yes, we are disconnected but this seems to further clean things up on the connman side)
	// 3. Turn off auto-connect for the service
	// 4. If there was a previous service then try to reconnect to it. Otherwise set an error state.
	//

	clearCredentialsInvocation ();

	if (service->m_cancelled)
	{
		service->m_cancelled = false;		// mark it no longer cancelled since we're going back to the earlier service
		if (m_oldService)
		{
			m_oldService->m_cancelled = true;
		}
		usePreviousService ();
	}
	else
	{
		if (error == connectionError::alreadyConnected)
		{
			if (service->m_serviceState == estiServiceFailure)
			{
				
				stiDEBUG_TOOL( g_stiNetworkDebug,
					stiTrace("NetworkDebug: CstiNetworkBase::%s: already connected error.. so disconnect and reconnect..\n", __func__);
				);
				service->synchronousDisconnect ();
				service->serviceConnect ();
			}
		}
		else if (error == connectionError::inProgress && m_reconnecting)
		{
			m_reconnectTimer.Start();
		}
		else
		{
			// the passphrase was wrong remove the serivce so it's not remembered (can't remove a hidden service)
			if ((error == connectionError::ioError && !service->isHidden())	|| (error == connectionError::inProgress && service->isHidden()))
			{
				if (m_newPassphrase)
				{
					service->serviceRemove();
				}
				else
				{
					for (auto iter : m_rememberedServices)
					{
						if (std::get<0>(iter) == service->m_serviceName)
						{
							wirelessPassphraseChangedSignal.Emit();
							break;
						}
					}
				}
				service->m_disconnecting = true;
				service->serviceDisconnect();
			}

			usePreviousService ();

			stiDEBUG_TOOL(g_stiNetworkDebug,
				vpe::trace("NetworkDebug: ", __func__, ": emitting networkSettingsErrorSignal\n");
			);

			networkSettingsErrorSignal.Emit();

			stiASSERT(false);
		}
	}

	m_newPassphrase = false;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: exit CstiNetworkBase::serviceConnectionErrorHandle finish\n");
	);
}

/*
 * cancel an in-progress wireless connection
 * this needs to happen synchronously.
 */
stiHResult CstiNetworkBase::NetworkConnectCancel()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent(std::bind(&CstiNetworkBase::eventNetworkConnectCancel, this));

	return hResult;
}

void CstiNetworkBase::eventNetworkConnectCancel ()
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: Enter eventNetworkConnectCancel\n");
	);

	if (!m_newService)
	{
		stiTrace("No in-progress connection to cancel\n");
	}
	else
	{
		m_targetService = nullptr;
		m_newService->m_disconnecting = true;
		auto result = m_newService->synchronousDisconnect();

		if (result == connectionError::notConnected
		 && !m_oldService)
		{
			stateSet (estiIDLE);
		}

		if (result != connectionError::other)
		{
			m_newService->propertyClear("Error");
		}
	}

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: return from EventNetworkConnectCancel\n");
	);
}

/*
 * set the values that the connman agent request input function needs
 */
stiHResult CstiNetworkBase::WirelessCredentialsSet(
	const char *passphrase,
	const char *userName)
{
	if (passphrase && strlen(passphrase))
	{
		m_passPhrase = passphrase;
	}

	if (userName && strlen(userName))
	{
		m_userName = userName;
	}

	if (m_needsCredentialsInvocation)
	{
		credentialsProcess (m_needsCredentialsInvocation, m_needsCredentialsInputType);
		g_object_unref (m_needsCredentialsInvocation);
		m_needsCredentialsInvocation = nullptr;
		m_needsCredentialsService = nullptr;
	}

	return (stiRESULT_SUCCESS);
}


void CstiNetworkBase::credentialsProcess (
	GDBusMethodInvocation *invocation,
	int inputType)
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("NetworkDebug: CstiNetworkBase::%s: enter\n", __func__);
	);
	GVariantBuilder *builder = nullptr;
	GVariant *variant = nullptr;

	if (m_passPhrase.length() == 0 && m_userName.length() == 0 && (inputType & estiINPUT_NEED_SSID) == 0)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("good status, but we don't have the necessary input, returning zero length dict\n");
		);

		g_dbus_method_invocation_return_value(invocation, g_variant_new("(a{sv})", NULL));
	}
	else
	{
		/*
		 * put together the dictionary reply here and we're done.
		 */
		builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

		if (inputType & estiINPUT_NEED_SSID)
		{
			//
			// If we have a new service, use its name. Otherwise, use the current service.
			//
			if (m_newService)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("Adding name of new service to return: %s\n", m_newService->m_serviceName.c_str ());
				);

				g_variant_builder_add(builder, "{sv}", "Name", g_variant_new_string(m_newService->m_serviceName.c_str ()));
			}
			else if (m_currentService)
			{
				stiDEBUG_TOOL(g_stiNetworkDebug,
					stiTrace("Adding name of current service to return: %s\n", m_currentService->m_serviceName.c_str ());
				);

				g_variant_builder_add(builder, "{sv}", "Name", g_variant_new_string(m_currentService->m_serviceName.c_str ()));
			}
			else
			{
				stiASSERT (false);
			}
		}

		if (inputType & estiINPUT_NEED_PASSPHRASE)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("Adding passphrase to return\n");
			);
			g_variant_builder_add(builder, "{sv}", "Passphrase", g_variant_new_string(m_passPhrase.c_str()));
		}

		if (inputType & estiINPUT_NEED_IDENTITY)
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("adding identity to return\n");
			);
			g_variant_builder_add(builder, "{sv}", "Identity", g_variant_new_string(m_userName.c_str()));
		}

		variant = g_variant_new("(a{sv})", builder);

		stiDEBUG_TOOL(g_stiNetworkDebug > 1,
			stiTrace("NetworkDebug: CstiNetworkBase::%s: posting return and setting m_newPassphrase=true\n", __func__);
		);
		m_newPassphrase = true;
		g_dbus_method_invocation_return_value(invocation, variant);

		//
		// pVariant shouldn't need to be freed here since the floating
		// ref should have been taken by g_dbus_method_invocation_return_value.
		//

		g_variant_builder_unref(builder);
		builder = nullptr;
	}
}


std::shared_ptr<NetworkService> CstiNetworkBase::serviceFromObjectPathGet (
	std::string objectPath)
{
	auto iter = std::find_if(m_services.begin(), m_services.end(),
				[&objectPath](std::shared_ptr<NetworkService> &service)
					{return service->objectPathGet() == objectPath;});

	if (iter != m_services.end())
	{
		return (*iter);
	}
	return nullptr;
}


/*
 * connman has requested some input through this DBus callback.
 */
gboolean CstiNetworkBase::ConnmanRequestInput(
	CstiNetworkAgent *,
	GDBusMethodInvocation *invocation,
	gchar *serviceName,
	GVariant *input,
	gpointer userData)
{
	auto self = (CstiNetworkBase *)userData;
	GVariant *variant = nullptr;
	int inputType = 0;
	std::string name;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: Enter ConnmanRequestInput for %s\n", serviceName);
	);

	variant = g_variant_lookup_value(input, "Name", nullptr);
	if (variant)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: got Name\n");
		);
		inputType |= estiINPUT_NEED_SSID;

		g_variant_unref (variant);
		variant = nullptr;
	}

	variant = g_variant_lookup_value(input, "Passphrase", nullptr);
	if (variant)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: got Passphrase\n");
		);
		inputType |= estiINPUT_NEED_PASSPHRASE;

		g_variant_unref (variant);
		variant = nullptr;
	}

	variant = g_variant_lookup_value(input, "PreviousPassphrase", nullptr);
	if (variant)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: got PreviousPassphrase\n");
		);
		inputType |= estiINPUT_NEED_PASSPHRASE;

		g_variant_unref (variant);
		variant = nullptr;
	}

	variant = g_variant_lookup_value(input, "Identity", nullptr);
	if (variant)
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: got Identity\n");
		);
		inputType |= estiINPUT_NEED_IDENTITY;

		g_variant_unref (variant);
		variant = nullptr;
	}

	g_object_ref (invocation);

	name = serviceName;
	self->PostEvent(std::bind(&CstiNetworkBase::eventNeedsCredentials, self, invocation, inputType, name));

	return (TRUE);
}


void CstiNetworkBase::eventNeedsCredentials (
	GDBusMethodInvocation *invocation,
	int inputType,
	std::string serviceName)
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("NetworkDebug: CstiNetworkBase::%s: Enter. Svc %s. inputType=%d\n", __func__, serviceName.c_str(), inputType);
	);

	if (inputType & estiINPUT_NEED_PASSPHRASE || inputType & estiINPUT_NEED_IDENTITY)
	{
		std::shared_ptr<NetworkService> service;
		service = serviceFromObjectPathGet (serviceName);

		if (service && (service == m_newService || service == m_currentService))
		{
			/*
			 * if we don't have the inputs we need, ask the user interface
			 * to get them, and wait.
			 */
//			std::string *pSSID = new std::string (service->m_serviceName.c_str ());

			stiDEBUG_TOOL(g_stiNetworkDebug > 1,
				stiTrace("NetworkDebug: CstiNetworkBase::%s: Asking UI for creds..\n", __func__);
			);
			m_needsCredentialsInvocation = (GDBusMethodInvocation *)g_object_ref(invocation);
			m_needsCredentialsService = service;
			m_needsCredentialsInputType = inputType;
			networkNeedsCredentialsSignal.Emit(service->m_serviceName);
		}
		else
		{
			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("NetworkDebug: CstiNetworkBase::%s: Credential request was for a service we no longer care about. Return null..\n", __func__);
			);
			//
			// Must be a need for credentials for a service we no longer care about.
			//
			g_dbus_method_invocation_return_value(invocation, g_variant_new("(a{sv})", NULL));
		}
	}
	else
	{
		credentialsProcess (invocation, inputType);
	}

	if (invocation)
	{
		g_object_unref(invocation);
		invocation = nullptr;
	}
}


/*!
 * \brief - requested a browser based authentication.  not supported.
 */
gboolean CstiNetworkBase::ConnmanRequestBrowser(
	CstiNetworkAgent *,
	GDBusMethodInvocation *invocation,
	gchar *serviceName,
	gchar *,
	gpointer userData)
{
	auto self = (CstiNetworkBase *)userData;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: ConnmanRequestBrowser\n");
	);

	g_object_ref (invocation);

	std::string name = serviceName;
	self->PostEvent(std::bind(&CstiNetworkBase::eventRequestBrowser, self, invocation, name));

	return (TRUE);
}


void CstiNetworkBase::eventRequestBrowser (
	GDBusMethodInvocation *invocation,
	std::string serviceName)
{
	std::shared_ptr<NetworkService> service;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: EventRequestBrowswer\n");
	);

	if (!serviceName.empty())
	{
		service = serviceFromObjectPathGet (serviceName);

		if (service)
		{
			service->m_disconnecting = true;
			service->serviceDisconnect();

			stiDEBUG_TOOL(g_stiNetworkDebug,
				stiTrace("NetworkDebug: %s: Disabling automatic connection\n", service->m_serviceName.c_str ());
			);
			service->propertySet("AutoConnect", g_variant_new_variant(g_variant_new_boolean(FALSE)));
		}
		else
		{
			stiASSERT (false);
		}
	}

	g_dbus_method_invocation_return_value(invocation, nullptr);

	g_object_unref (invocation);
	invocation = nullptr;
	networkUnsupportedSignal.Emit();
}


gboolean CstiNetworkBase::ConnmanReportError(
	CstiNetworkAgent *,
	GDBusMethodInvocation *invocation,
	gchar *serviceName,
	gchar *message,
	gpointer userData)
{
	auto self = (CstiNetworkBase *)userData;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: Connman Agent error %s\n", message);
	);

	if (strcmp (message, "invalid-key") == 0)
	{
		g_object_ref (invocation);
		std::string name = serviceName;
		self->PostEvent(std::bind(&CstiNetworkBase::eventInvalidKey, self, invocation, name));
	}
	else
	{
		g_dbus_method_invocation_return_value(invocation, nullptr);
	}

	return (TRUE);
}


void CstiNetworkBase::eventInvalidKey (
	GDBusMethodInvocation *invocation,
	std::string serviceName)
{
	std::shared_ptr<NetworkService> service;
	service = serviceFromObjectPathGet (serviceName);

	//
	// Did we find the service and was it the "new" service or the current service?
	//
	if (service && (service == m_newService || service == m_currentService))
	{
		DbusError dbusError;

		GDBusMessage *message1 = g_dbus_method_invocation_get_message (invocation);

		GDBusMessage *message2 = g_dbus_message_new_method_error (message1, "net.connman.Agent.Error.Retry", "invalid-key");

		g_dbus_connection_send_message (m_dbusConnection->connectionGet (), message2, G_DBUS_SEND_MESSAGE_FLAGS_NONE, nullptr, dbusError.get());

		if (dbusError.valid())
		{
			stiASSERT(false);
		}

		g_object_unref (message2);
		message2 = nullptr;

		g_dbus_method_invocation_return_value(invocation, nullptr);

		g_object_unref (invocation);
		invocation = nullptr;
	}
	else
	{
		stiASSERTMSG (false, "Report Error service not found or incorrect: %s, %s\n", service->m_serviceName.c_str(), m_newService->m_serviceName.c_str());
	}
}


gboolean CstiNetworkBase::ConnmanRelease(CstiNetworkAgent *, GDBusMethodInvocation *invocation, gchar *, gchar *, gpointer)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("NetworkDebug: Connman Release\n");
	);

	g_dbus_method_invocation_return_value(invocation, nullptr);

	return(TRUE);
}

/*
 * this registers the functions that gets called when the agent has a request
 */
void CstiNetworkBase::eventNetworkManagerInterfaceAdded (
	std::shared_ptr<NetworkManagerInterface> networkManagerInterface)
{

	m_networkManager = networkManagerInterface;

	m_managerConnections.push_back(m_networkManager->technologiesRetrievedSignal.Connect(
		[this](GVariant *technologies)
		{
			g_variant_ref(technologies);

			PostEvent([this, technologies]
			{
				eventTechnologiesRetrieved(technologies);

				g_variant_unref(technologies);
			});
		}));

	m_managerConnections.push_back(m_networkManager->technologyAddedSignal.Connect(
		[this](const std::string &objectPath, GVariant *properties)
		{
			g_variant_ref(properties);

			PostEvent([this, objectPath, properties]
			{
				eventTechnologySupplied(objectPath, properties);		// eventTechnologyAdded was already used!

				g_variant_unref(properties);
			});
		}));

	m_managerConnections.push_back(m_networkManager->technologyRemovedSignal.Connect(
		[this](std::string objectPath)
		{
			PostEvent([this, objectPath]
			{
				eventTechnologyRemoved(objectPath);
			});
		}));

	m_managerConnections.push_back(m_networkManager->servicesRetrievedSignal.Connect(
		[this](GVariant *services)
		{
			g_variant_ref (services);

			PostEvent([this, services]
			{
				eventServicesRetrieved(services);

				g_variant_unref(services);
			});
		}));

	m_managerConnections.push_back(m_networkManager->servicesChangedSignal.Connect(
		[this](GVariant *changed, const std::vector<std::string> &removed)
		{
			g_variant_ref(changed);

			PostEvent([this, changed, removed]
			{
				eventServicesChanged(changed, removed);

				g_variant_unref(changed);
			});
		}));

	m_managerConnections.push_back(m_networkManager->specificServiceFoundSignal.Connect(
		[this](const std::string &objectPath, GVariant *dict)
		{
			PostEvent([this, objectPath, dict]
			{
				specificServiceFoundHandle(objectPath, dict);
			});
		}));

	m_managerConnections.push_back(m_networkManager->servicesScannedSignal.Connect(
		[this](GVariant *services)
		{
			g_variant_ref(services);

			PostEvent([this, services]
			{
				eventGetServicesFinish(services);

				g_variant_unref(services);
			});
		}));

	char szAgentPath[128];

	sprintf(szAgentPath, "/net/connman/agent%d", getpid());

	m_pObjectSkeleton = csti_network_object_skeleton_new (szAgentPath);
	m_pAgentSkeleton = csti_network_agent_skeleton_new();
	csti_network_object_skeleton_set_agent(m_pObjectSkeleton, m_pAgentSkeleton);

	m_RequestInputSignalHandlerId = g_signal_connect(m_pAgentSkeleton, "handle-request-input", G_CALLBACK(ConnmanRequestInput), this);
	m_RequestBrowserSignalHandlerId = g_signal_connect(m_pAgentSkeleton, "handle-request-browser", G_CALLBACK(ConnmanRequestBrowser), this);
	m_ReportErrorSignalHandlerId = g_signal_connect(m_pAgentSkeleton, "handle-report-error", G_CALLBACK(ConnmanReportError), this);
	m_ReleaseSignalHandlerId = g_signal_connect(m_pAgentSkeleton, "handle-release", G_CALLBACK(ConnmanRelease), this);

	m_pObjectManagerServer = g_dbus_object_manager_server_new("/net/connman");
	g_dbus_object_manager_server_export(m_pObjectManagerServer, G_DBUS_OBJECT_SKELETON(m_pObjectSkeleton));
	g_dbus_object_manager_server_set_connection(m_pObjectManagerServer, m_dbusConnection->connectionGet ());

	m_networkManager->agentRegister(std::string(szAgentPath));
}


stiHResult CstiNetworkBase::ConnmanAgentUnregister()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pAgentSkeleton)
	{
		g_signal_handler_disconnect(m_pAgentSkeleton, m_RequestInputSignalHandlerId);
		m_RequestInputSignalHandlerId = 0;

		g_signal_handler_disconnect(m_pAgentSkeleton, m_RequestBrowserSignalHandlerId);
		m_RequestBrowserSignalHandlerId = 0;

		g_signal_handler_disconnect(m_pAgentSkeleton, m_ReportErrorSignalHandlerId);
		m_ReportErrorSignalHandlerId = 0;

		g_signal_handler_disconnect(m_pAgentSkeleton, m_ReleaseSignalHandlerId);
		m_ReleaseSignalHandlerId = 0;

		if (m_pObjectSkeleton)
		{
			csti_network_object_skeleton_set_agent (m_pObjectSkeleton, nullptr);
		}

		g_object_unref (m_pAgentSkeleton);
		m_pAgentSkeleton = nullptr;
	}

	if (m_pObjectSkeleton)
	{
		g_object_unref (m_pObjectSkeleton);
		m_pObjectSkeleton = nullptr;
	}

	if (m_pObjectManagerServer)
	{
		g_dbus_object_manager_server_set_connection (m_pObjectManagerServer, nullptr);

		g_object_unref (m_pObjectManagerServer);
		m_pObjectManagerServer = nullptr;
	}

//STI_BAIL:

	return (hResult);
}

/*
 * network manager added a new technology (not eventTechnologyAdded that happens when we get the proxy)
 */
void CstiNetworkBase::eventTechnologySupplied(
	std::string objectPath,
	GVariant *properties)
{
	auto dbusObject = m_dbusConnection->findOrCreate(this, objectPath);
	dbusObject->interfaceAdded(CONNMAN_TECHNOLOGY_INTERFACE, properties, NetworkDbusInterface::interfaceMake);
}

void CstiNetworkBase::eventTechnologyRemoved(std::string objectPath)
{
	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("CstiNetworkBase::%s: enter, objectPath = %s\n", __func__, objectPath.c_str());
	);

	auto iter = std::find_if(m_technologies.begin(), m_technologies.end(),
				[&objectPath](std::shared_ptr<NetworkTechnology> &technology)
					{return technology->objectPathGet() == objectPath;});

	if (iter != m_technologies.end())
	{
		objectRemove(objectPath, CONNMAN_TECHNOLOGY_INTERFACE);
		(*iter) = nullptr;
		m_technologies.erase(iter);
	}
}

/*
 * get the name of the wired network interface
 */
std::string CstiNetworkBase::NetworkDeviceNameGet() const
{

	std::string retVal;
	const std::string sysName = "/sys/class/net/";

	auto sysDir = opendir(sysName.c_str());

	if (sysDir != nullptr)
	{
		struct dirent *dirEntry = nullptr;
		while((dirEntry = readdir(sysDir)) != nullptr)
		{
			std::string pathName;
			struct stat statBuff {};

			if (!strcmp(dirEntry->d_name, ".") || !strcmp(dirEntry->d_name, ".."))
			{
				continue;
			}

			// if this directory exists, this is a real physical device
			pathName = sysName + dirEntry->d_name + "/device";

			if (stat(pathName.c_str(), &statBuff) > -1)
			{
				// wired device does not have this directory
				pathName = sysName + dirEntry->d_name + "/wireless";
				statBuff = {};

				if (stat(pathName.c_str(), &statBuff) < 0)
				{
					retVal = dirEntry->d_name;
					break;
				}
			}
		}
		closedir(sysDir);
	}
	return(retVal);
}

/*
 * get the mac address of the wireless device
 */
void CstiNetworkBase::wirelessMacAddressUpdate()
{

	const std::string sysName = "/sys/class/net/";

	auto sysDir = opendir(sysName.c_str());

	if (sysDir != nullptr)
	{
		struct dirent *dirEntry = nullptr;
		while((dirEntry = readdir(sysDir)) != nullptr)
		{
			if (!strcmp(dirEntry->d_name, ".") || !strcmp(dirEntry->d_name, ".."))
			{
				continue;
			}

			std::string interfaceName = dirEntry->d_name;

			// Check for common wireless interface name patterns
			if (!(interfaceName.find ("wlan") == 0 ||	// wlan0, wlan1
				  interfaceName.find ("wlp")  == 0 ||	// wlp3s0 (PCIe)
				  interfaceName.find ("wlo")  == 0 ||	// wlo1 (USB)
				  interfaceName.find ("wlx")  == 0))	// wlx... (USB with MAC)
			{     
				continue; // Skip non-wireless interface names
			}
			
			// Verify it's wireless by checking for wireless directory
			std::string wirelessPath = sysName + interfaceName + "/wireless";
			struct stat statBuff {};

			if (stat (wirelessPath.c_str (), &statBuff) > -1)
			{
				// Read MAC address
				std::string macAddressPath = sysName + interfaceName + "/address";
				int fd = open (macAddressPath.c_str (), O_RDONLY);

				if (fd > -1)
				{
					char address[32];
					memset (address, '\0', sizeof (address));
					int bytesRead = read (fd, address, sizeof (address) - 1);
					close (fd);

					if (bytesRead > 10)
					{
						char *ptr = strchr (address, '\n');
						if (ptr)
						{
							*ptr = '\0';
						}
						m_wirelessMacAddress = address;
						break; // Found it, exit the loop
					}
				}
			}
		}
		closedir (sysDir);
	}
}

std::string CstiNetworkBase::WirelessMacAddressGet()
{
	return m_wirelessMacAddress;
}

void CstiNetworkBase::rememberedServicesUpdate()
{

	m_rememberedServices.clear();
	auto libDir = opendir(m_connmanDirName.c_str());

	if (libDir != nullptr)
	{
		struct dirent *dirEntry = nullptr;

		while((dirEntry = readdir(libDir)) != nullptr)
		{
			if (strncmp(dirEntry->d_name, "wifi_", 5))
			{
				continue;
			}

			std::string fileName = m_connmanDirName + dirEntry->d_name + "/settings";
			int fd = open(fileName.c_str(), O_RDONLY);

			if (fd < 0)
			{
				continue;
			}

			int size = lseek(fd, 0l, SEEK_END);
			lseek(fd, 0l, SEEK_SET);
			char buff[size];
			int rSize = read(fd, buff, size);
			close(fd);

			if (rSize < size)
			{
				continue;
			}

			char *ptr = strstr(buff, "\nName=");

			if (!ptr)
			{
				continue;
			}

			ptr += 6;
			char *eptr = strchr(ptr, '\n');

			if (eptr)
			{
				*eptr = '\0';
			}
			m_rememberedServices.push_back(std::make_tuple(ptr, dirEntry->d_name));
		}
		closedir(libDir);
	}
}
	

std::list<std::string> CstiNetworkBase::RememberedServicesGet()
{
	std::list<std::string> retVal;

	for (auto iter : m_rememberedServices)
	{
		retVal.push_back(std::get<0>(iter));
	}
	return retVal;
}

stiHResult CstiNetworkBase::RememberedServiceRemove(std::string ssid)
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("NetworkDebug: CstiNetworkBase::%s: Enter. Removing svc %s\n", __func__, ssid.c_str());
	);
	stiHResult hResult = stiRESULT_SUCCESS;

	std::list<std::tuple<std::string, std::string>>::iterator iter;

	for (iter = m_rememberedServices.begin(); iter != m_rememberedServices.end(); iter++)
	{
		if (std::get<0>(*iter) == ssid)
		{
			if (m_currentService && m_currentService->m_serviceName == ssid)
			{
				wirelessNetworkDisconnectedSignal.Emit();
			}
			PostEvent(std::bind(&CstiNetworkBase::eventRememberedServiceRemove, this, ssid));
			break;
		}
	}

	if (iter == m_rememberedServices.end())
	{
		stiTHROW_NOLOG(stiRESULT_ERROR);
	}

STI_BAIL:
	return hResult;
}

void CstiNetworkBase::eventRememberedServiceRemove(std::string ssid)
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("NetworkDebug: CstiNetworkBase::%s: Enter. Removing svc %s\n", __func__, ssid.c_str());
	);
	std::list<std::tuple<std::string, std::string>>::iterator iter;

	for (iter = m_rememberedServices.begin(); iter != m_rememberedServices.end(); iter++)
	{
		if (std::get<0>(*iter) == ssid)
		{
			break;
		}
	}

	if (iter != m_rememberedServices.end())
	{
		if (!m_currentService || m_currentService->m_serviceName == ssid)
		{
			m_waitForConnect = false;
		}
		else
		{
			m_waitForConnect = true;
		}

		m_resetName = std::get<1>(*iter);
		m_resetState = resetState::shuttingDown;
		m_bluetoothEnabled = m_bluetooth->Powered();

		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: CstiNetworkBase::%s: Stopping Connman service and starting resetTimer. m_waitForConnect=%d, m_resetName=%s\n", __func__, m_waitForConnect, m_resetName.c_str());
		);
		int i = system(m_connmanStopCommand.c_str());
		stiUNUSED_ARG(i);
		m_resetTimer.Start ();
	}
}

void CstiNetworkBase::eventResetTimerTimeout()
{
	stiDEBUG_TOOL(g_stiNetworkDebug > 1,
		stiTrace("NetworkDebug: CstiNetworkBase::%s: enter..\n", __func__);
	);
	if (m_resetState == resetState::unknown || m_resetState == resetState::shuttingDown)
	{
		std::string  pathName = m_connmanDirName + m_resetName;
		std::string fileName = pathName + "/settings";
		m_resetName = "";

		stiDEBUG_TOOL(g_stiNetworkDebug > 1,
			stiTrace("NetworkDebug: CstiNetworkBase::%s: Deleting connman service dir: %s\n", __func__, pathName.c_str());
		);

		int i = unlink(fileName.c_str());
		fileName = pathName + "/data";
		i = unlink(fileName.c_str());
		i = rmdir(pathName.c_str());

		m_rememberedServices.clear();

		for (auto serviceIter = m_services.begin(); serviceIter != m_services.end(); )
		{
			objectRemove((*serviceIter)->objectPathGet(), CONNMAN_SERVICE_INTERFACE);
			if (*serviceIter == m_currentService)
			{
				m_currentService = nullptr;
			}
			(*serviceIter) = nullptr;
			serviceIter = m_services.erase(serviceIter);
		}

		for (auto techIter = m_technologies.begin(); techIter != m_technologies.end(); )
		{
			objectRemove((*techIter)->objectPathGet(), CONNMAN_TECHNOLOGY_INTERFACE);
			(*techIter) = nullptr;
			techIter = m_technologies.erase(techIter);
		}
		m_wifiPowered = false;
		m_bluetoothPowered = false;

		ConnmanUninitialize ();
		m_managerConnections.clear();
		m_resetState = resetState::restarting;

		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: CstiNetworkBase::%s: Starting Connman Service and (re)starting resetTimer.\n", __func__);
		);
		i = system(m_connmanStartCommand.c_str());
		stiUNUSED_ARG(i);
		m_resetTimer.Start ();
	}
	else
	{
		stiDEBUG_TOOL(g_stiNetworkDebug,
			stiTrace("NetworkDebug: CstiNetworkBase::%s: Stopping resetTimer and disconnecting paired bluetooth devices..\n", __func__);
		);
		m_resetTimer.Stop();
		m_bluetooth->pairedDevicesReset();
		ConnmanInitialize();
		PostEvent([this]{ networkManagerCreate (); });
	}
}
