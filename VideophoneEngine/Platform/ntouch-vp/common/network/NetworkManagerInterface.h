
#pragma once

#include "CstiNetworkConnmanInterface.h"
#include "NetworkDbusInterface.h"
#include "stiTools.h"
#include "stiDefs.h"
#include "CstiSignal.h"

#include "DbusError.h"
#include "DbusCall.h"

#include <string>
#include <memory>


class NetworkManagerInterface: public NetworkDbusInterface
{

public:
	static constexpr const char *propertyState = "State";
	static constexpr const char *propertyOfflineMode = "OfflineMode";
	static constexpr const char *propertySessionMode = "SessionMode";

	NetworkManagerInterface( DbusObject *dbusObject) :
		NetworkDbusInterface(dbusObject, CONNMAN_MANAGER_INTERFACE)
	{
	}

	~NetworkManagerInterface() override;

	void added(GVariant *properties) override;

	void agentRegister(std::string path);

	void servicesGet();

	void specificServiceGet(const std::string &objectPath);

public:		// signals
	static CstiSignal<std::shared_ptr<NetworkManagerInterface>> addedSignal;
	CstiSignal<> agentRegisteredSignal;
	CstiSignal<> agentRegisterationFailedSignal;
	CstiSignal<> agentUnregisteredSignal;
	CstiSignal<GVariant *> technologiesRetrievedSignal;
	CstiSignal<GVariant *> servicesRetrievedSignal;
	CstiSignal<std::string, GVariant *> technologyAddedSignal;
	CstiSignal<std::string> technologyRemovedSignal;
	CstiSignal<GVariant *, std::vector<std::string>> servicesChangedSignal;
	CstiSignal<GVariant *> servicesScannedSignal;
	CstiSignal<const std::string &, GVariant *> specificServiceFoundSignal;

public:		// member variables
	std::string m_managerState;
	bool m_offlineMode{false};
	bool m_sessionMode{false};

private:
// event handlers
	void eventProxyNew (GObject *proxy, GAsyncResult *res);
	void eventAgentRegistered(GObject *proxy, GAsyncResult *res);
	void getServicesFinish(GObject *proxy, GAsyncResult *res);
	void eventGetServices(GObject *proxy, GAsyncResult *res);
	void eventGetTechnologies(GObject *proxy, GAsyncResult *res);
	void eventGetProperties(GObject *proxy, GAsyncResult *res);
	void eventPropertyChanged (CstiNetworkManager *proxy, const std::string &name, GVariant *value);
	void specificServiceGetFinish(GObject *proxy, GAsyncResult *res, const std::string &objectPath);

//	callbacks
	static void propertyChangedCallback(
		CstiNetworkManager *proxy,
		const gchar *name,
		GVariant *value,
		gpointer userData);

	static void technologyAddedCallback(
		CstiNetworkManager *proxy,
		gchar *objectPath,
		GVariant *properties,
		gpointer userData);

	static void technologyRemovedCallback(
		CstiNetworkManager *proxy,
		gchar *objectPath,
		gpointer userData);

	static void servicesChangedCallback(
		CstiNetworkManagerProxy *proxy,
		GVariant *chagnged,
		gchar **removed,
		gpointer userData);

	CstiNetworkManager *m_networkManagerProxy = nullptr;

	unsigned long m_propertyChangedSignalHandlerId = 0;
	unsigned long m_technologyAddedSignalHandlerId = 0;
	unsigned long m_technologyRemovedSignalHandlerId = 0;
	unsigned long m_servicesChangedSignalHandlerId = 0;
};
