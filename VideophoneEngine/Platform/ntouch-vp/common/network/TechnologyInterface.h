
#pragma once

#include "NetworkDbusInterface.h"
#include "CstiNetworkConnmanInterface.h"
#include "CstiSignal.h"

#include "stiDefs.h"

#include "DbusError.h"
#include "DbusCall.h"

#include <string>
#include <memory>

enum class technologyType
{
	unknown = -1,
	wired = 0,
	wireless,
	bluetooth,
};

class NetworkTechnology : public NetworkDbusInterface
{
public:

	/*
	 * the properties of the available technologies
	 * (we aren't currently interested in tethering)
	 */
	NetworkTechnology () = delete;
	NetworkTechnology (DbusObject *object)
		 : NetworkDbusInterface(object, CONNMAN_TECHNOLOGY_INTERFACE)
	{
	}

	~NetworkTechnology () override;

	void added(GVariant *properties) override;

public:			// signals
	static CstiSignal<std::shared_ptr<NetworkTechnology>> addedSignal;
	static CstiSignal<std::shared_ptr<NetworkTechnology>> powerChangedSignal;
	static CstiSignal<bool> scanFinishedSignal;

	void scanStart();

	void propertySet(
		const std::string &property,
		GVariant *value);
	
	std::string m_technologyName;
	technologyType m_technologyType = technologyType::unknown;
	bool m_powered = false;
	bool m_connected = false;

private:
/*
 * there are more properties than these, but they have to do with tethering
 * and we're not interested in them
 */
	static constexpr const char *propertyPowered = "Powered";			//readwrite
	static constexpr const char *propertyConnected = "Connected";		//readonly
	static constexpr const char *propertyName = "Name";					//readonly
	static constexpr const char *propertyType = "Type";					//readonly

	static constexpr const char *typeWifi = "wifi";
	static constexpr const char *typeEthernet = "ethernet";
	static constexpr const char *typeBluetooth = "bluetooth";

	void eventProxyNew (
		GObject *proxy,
		GAsyncResult *res,
		GVariant *properties);

	void eventTechnologyChanged (
		CstiNetworkTechnology *proxy,
		const std::string &name,
		GVariant *value);

	static void technologyChangedCallback(
		CstiNetworkTechnology *proxy,
		const gchar *name,
		GVariant *value,
		gpointer userData);

	void propertySetFinish(
		GObject *proxy,
		GAsyncResult *result,
		gpointer userData);

	void scanFinish(
		GObject *proxy,
		GAsyncResult *res);

	void propertiesProcess(GVariant *properties);

	CstiNetworkTechnology *m_technologyProxy{nullptr};

	unsigned long m_technologyChangedSignalHandlerId = 0;
};
