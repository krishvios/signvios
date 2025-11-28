// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <jni.h>

#include "AstiVideophoneEngine.h"

#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "stiTools.h"
#include "propertydef.h"
#include "stiTrace.h"
#include <functional>
#include "ServiceOutageMessage.h"
#include <memory>
#include "Address.h"

#include <sys/system_properties.h>

#include <sstream>
#include "IstiContactManager.h"
#include "IstiBlockListManager.h"
#include "IstiMessageViewer.h"

#undef TAG
#define TAG "AstiVideophoneEngine"

static const char PRODUCT_NAME[] = "Sorenson Videophone AndroidMVRS";
static const char PHONE_TYPE_ID[] = "NTOUCH-ANDROID";

static AstiVideophoneEngine *androidVideophoneEngine = NULL;

char g_dns1[INET_ADDRSTRLEN];
char g_dns2[INET_ADDRSTRLEN];
char g_macAddress[nMAX_UNIQUE_ID_LENGTH];
char g_ip[INET_ADDRSTRLEN];

extern bool g_bUseGoogleDNS;
extern bool g_h263Only = false;
extern EstiBool g_bIsATT3G;
int g_displayWidth = 0;
int g_displayHeight = 0;

extern stiHResult myPstiAppGenericCallback(int32_t n32Message, size_t MessageParam, size_t CallbackParam);

int androidSDKVersion = 0;

bool AstiVideophoneEngine::initialize(const char *version, const char *appWritablePath) {
	aLOGD("AstiVideophoneEngine::initialize");

	char* propertiesPath = new char[512];
	strcpy(propertiesPath, appWritablePath);
	strcat(propertiesPath, "/DefaultProperties.xml");

    //Initialize the WD timer so that when we start making Property manager calls
    //it is ready to send the data to core.
    stiOSWdInitialize();
	WillowPM::PropertyManager* pPM = WillowPM::PropertyManager::getInstance();
	pPM->SetFilename(propertiesPath);
	pPM->SetBackup1Filename("");
	pPM->SetDefaultFilename("");
	pPM->SetUpdateFilename("");

	pPM->init(version);

	std::map<std::string, std::string>::iterator optionsIt;
	for (optionsIt = options.begin(); optionsIt != options.end(); ++optionsIt) {
		if (optionsIt->first.compare("ssl") == 0) {
			int optionValue;
			std::stringstream strStream(optionsIt->second);
			strStream >> optionValue;
			int sslFailover = (1==optionValue)?eSSL_ON_WITH_FAILOVER:eSSL_OFF;

			pPM->propertySet(CoreServiceSSLFailOver, sslFailover, WillowPM::PropertyManager::Persistent);
			pPM->propertySet(StateNotifySSLFailOver, sslFailover, WillowPM::PropertyManager::Persistent);
			pPM->propertySet(MessageServiceSSLFailOver, sslFailover, WillowPM::PropertyManager::Persistent);
		}
		else {
			pPM->propertySet(optionsIt->first.c_str(), optionsIt->second.c_str(), WillowPM::PropertyManager::Persistent);
		}
	}

    //	Production
    //	pPM->propertySet(cmCORE_SERVICE_URL1, "http://mvrscore.sorenson.com/VPServices/CoreRequest.aspx", WillowPM::PropertyManager::Persistent);
    //	pPM->propertySet(cmSTATE_NOTIFY_SERVICE_URL1, "http://mvrsstatenotify.sorenson.com/VPServices/StateNotifyRequest.aspx", WillowPM::PropertyManager::Persistent);
    //	pPM->propertySet(cmMESSAGE_SERVICE_URL1, "http://mvrsmessage.sorenson.com/VPServices/MessageRequest.aspx", WillowPM::PropertyManager::Persistent);
    //	pPM->propertySet(cmVRS_SERVER, "call.svrs.tv", WillowPM::PropertyManager::Persistent);

	pPM->propertySet(RING_GROUP_ENABLED, 0, WillowPM::PropertyManager::Persistent);

	pPM->saveToPersistentStorage();

    m_stiVideophoneEngine = ::videophoneEngineCreate(ProductType::Android, version, true, true, appWritablePath, appWritablePath);

    if (m_stiVideophoneEngine == NULL) {
    	return false;
    }

    m_stiVideophoneEngine->AppNotifyCallBackSet(myPstiAppGenericCallback, (size_t) this);

    setupVideophoneEngineSignalCallbacks();

	m_stiVideophoneEngine->MaxCallsSet(2);
	m_stiVideophoneEngine->Startup();

	int nVrcl = 0;
	pPM->propertyGet("uiVRCLPortOverride", &nVrcl, WillowPM::PropertyManager::Persistent);
	m_stiVideophoneEngine->VRCLPortSet(nVrcl);

	// user isn't allowed to use app without accepting so set this to true
	m_stiVideophoneEngine->DefaultProviderAgreementSet(true);
	m_stiVideophoneEngine->NetworkStartup();

	aLOGD("AstiVideophoneEngine::initialize complete.");

	//    return (res == stiRESULT_SUCCESS);
	return true;
}

void AstiVideophoneEngine::setupVideophoneEngineSignalCallbacks() {
	m_stiVideophoneEngine->ServiceOutageMessageConnect([this](std::shared_ptr<vpe::ServiceOutageMessage> message) {
		if (m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnMessage(message);
		}
	});

	m_signalConnections.push_back(m_stiVideophoneEngine->ContactManagerGet()->contactErrorSignalGet().Connect([this](IstiContactManager::Response response, stiHResult result) {
		if (m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnContactErrorMessage(response, result);
		}
	}));

	m_signalConnections.push_back(m_stiVideophoneEngine->ContactManagerGet()->favoriteErrorSignalGet().Connect([this](IstiContactManager::Response response, stiHResult result) {
		if (m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnFavoritesErrorMessage(response, result);
		}
	}));

	m_signalConnections.push_back(m_stiVideophoneEngine->BlockListManagerGet()->errorSignalGet().Connect([this](IstiBlockListManager::Response response, stiHResult result, unsigned int coreRequestId, const std::string & blocklistItemId) {
		if (m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnBlockListErrorMessage(response, result);
		}
	}));

	m_signalConnections.push_back(m_stiVideophoneEngine->contactReceivedSignalGet().Connect([this](const SstiSharedContact &contact) {
		if(m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnSharedContactReceivedMessage(contact);
		}
	}));

	m_signalConnections.push_back(m_stiVideophoneEngine->clientAuthenticateSignalGet().Connect([this](std::shared_ptr<vpe::ServiceResponse<ClientAuthenticateResult>> message) {
		if(m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnClientAuthenticateMessage(message);
		}
	}));

	m_signalConnections.push_back(m_stiVideophoneEngine->userAccountInfoReceivedSignalGet().Connect([this](std::shared_ptr<vpe::ServiceResponse<CstiUserAccountInfo>> message) {
		if(m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnUserAccountInfoMessage(message);
		}
	}));

	m_signalConnections.push_back(m_stiVideophoneEngine->emergencyAddressReceivedSignalGet().Connect([this](std::shared_ptr<vpe::ServiceResponse<vpe::Address>> message) {
		if(m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnEmergencyAddressReceivedMessage(message);
		}
	}));

	m_signalConnections.push_back(m_stiVideophoneEngine->emergencyAddressStatusReceivedSignalGet().Connect([this](std::shared_ptr<vpe::ServiceResponse<EstiEmergencyAddressProvisionStatus>> message) {
		if(m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnEmergencyAddressProvisionStatusMessage(message);
		}
	}));

	m_signalConnections.push_back(m_stiVideophoneEngine->deviceLocationTypeChangedSignalGet().Connect([this](const DeviceLocationType &locationType) {
	    if(m_videophoneEngineCallback) {
	        m_videophoneEngineCallback->OnDeviceLocationTypeChanged(locationType);
	    }
	}));

	m_signalConnections.push_back(IstiMessageViewer::InstanceGet()->stateSetSignalGet().Connect([this](const IstiMessageViewer::EState state) {
		if(m_videophoneEngineCallback) {
			m_videophoneEngineCallback->OnFilePlayStateChanged(state);
		}
	}));
}

void AstiVideophoneEngine::registerDHVCallback(IstiCall *call) {
	m_signalConnections.push_back(call->dhviStateChangeSignalGet().Connect([this](IstiCall::DhviState state) {
    		if(m_videophoneEngineCallback) {
    			m_videophoneEngineCallback->OnDHVChangedMessage(state);
    		}
    	}));

}

void AstiVideophoneEngine::destroy() {
    if (m_stiVideophoneEngine != NULL) {
        DestroyVideophoneEngine(m_stiVideophoneEngine);
        m_stiVideophoneEngine = NULL;
    }
}

void AstiVideophoneEngine::setMACAddress(const char *macAddress) {
	memcpy (g_macAddress, macAddress, nMAX_UNIQUE_ID_LENGTH);
}

bool AstiVideophoneEngine::setNetworkInformation(bool isEngineInitialized) {
    std::string szBuf;
    if (stiRESULT_SUCCESS == stiGetLocalIp(&szBuf, estiTYPE_IPV4)) {
		if (isEngineInitialized) {
			WillowPM::PropertyManager *pPm = WillowPM::PropertyManager::getInstance ();
            pPm->propertySet(IPv4IpAddress, szBuf.c_str(), WillowPM::PropertyManager::Persistent);
            stiGetNetSubnetMaskAddress(&szBuf, estiTYPE_IPV4);
            pPm->propertySet(IPv4SubnetMask, szBuf.c_str(), WillowPM::PropertyManager::Persistent);

			pPm->PropertySend(IPv4IpAddress, estiPTypePhone);
			pPm->PropertySend(IPv4SubnetMask, estiPTypePhone);
		}
		return true;
	}
	return false;
}

bool convertToEngineDNSFormat(char *dns, char* out) {
	char *p = dns, *p1;
	for (int c = 0; c < 4; c ++) {
		out[c] = strtol(p, &p1, 10);
		if (NULL == p1 || p == p1 || (*p1 != '.' && c < 3)) {
			return false;
		}
		p = p1+1;
	}
	return true;
}

void AstiVideophoneEngine::setDNS(bool isEngineInitialized) {
	char dns_in1[PROP_VALUE_MAX];
	char dns_in2[PROP_VALUE_MAX];
	__system_property_get("net.dns1", dns_in1);
	__system_property_get("net.dns2", dns_in2);
	aLOGD("Setting dns values: net.dns1=%s  net.dns2=%s", dns_in1, dns_in2);

	//Always use google dns.
	g_bUseGoogleDNS = true;

	//Set to IPV6 Address lenth
	char dns_out1[INET6_ADDRSTRLEN];
	char dns_out2[INET6_ADDRSTRLEN];

	convertToEngineDNSFormat(dns_in1, dns_out1);
	convertToEngineDNSFormat(dns_in2, dns_out2);

	// This used to get checked when the method was first called but now we need
	// to copy over the DNS for the google check.
	if(isEngineInitialized){
		// Once the engine is initialized, copy the new values over to it.  We need to
		// wait to see if the engine is initialized so we can grab the property manager
		// and store some values inside it.
		WillowPM::PropertyManager *pPm = WillowPM::PropertyManager::getInstance ();

        pPm->propertySet(IPv4Dns1, dns_in1, WillowPM::PropertyManager::Persistent);
        pPm->propertySet(IPv4Dns2, dns_in2, WillowPM::PropertyManager::Persistent);

        memcpy (g_dns1, dns_out1, INET_ADDRSTRLEN);
        memcpy (g_dns2, dns_out2, INET_ADDRSTRLEN);

        pPm->PropertySend(IPv4Dns1, estiPTypePhone);
        pPm->PropertySend(IPv4Dns2, estiPTypePhone);
	}
}

void AstiVideophoneEngine::seth263Only(bool h263Only) {
	g_h263Only = h263Only;
}

void AstiVideophoneEngine::setPreferredDisplaySize(int preferredWidth, int preferredHeight) {
    g_displayWidth = preferredWidth;
    g_displayHeight = preferredHeight;
}

void AstiVideophoneEngine::setIPAddress(bool isEngineInitialized, const char *ipAddress) {
	if (isEngineInitialized) {
        strcpy(g_ip, ipAddress);
	}
}

void AstiVideophoneEngine::setGatewayIP(bool isEngineInitialized, const char *ipAddress) {
    if (isEngineInitialized && ipAddress) {
        WillowPM::PropertyManager *pPm = WillowPM::PropertyManager::getInstance();
        pPm->propertySet(IPv4GatewayIp, ipAddress, WillowPM::PropertyManager::Persistent);
        pPm->PropertySend(IPv4GatewayIp, estiPTypePhone);
    }
}


void AstiVideophoneEngine::setAndroidVersion(int sdkVersion) {
    androidSDKVersion = sdkVersion;
}

void AstiVideophoneEngine::setIsBad3GNetwork(bool isBad) {
	g_bIsATT3G = (isBad) ? estiTRUE : estiFALSE;
}

void AstiVideophoneEngine::setPublicIpAddressAuto() {
	WillowPM::PropertyManager *pPm = WillowPM::PropertyManager::getInstance ();
	// send public ip address as hostname
	std::string tmpEntryIPAddress;
	pPm->propertyGet(cmPUBLIC_IP_ADDRESS_AUTO, &tmpEntryIPAddress, WillowPM::PropertyManager::Persistent);
    pPm->propertySet(NetHostname, tmpEntryIPAddress.c_str (), WillowPM::PropertyManager::Persistent);
    pPm->PropertySend(NetHostname, estiPTypePhone);
}

void AstiVideophoneEngine::setVideophoneEngineCallback(VideophoneEngineCallback *callback) {
	m_videophoneEngineCallback = callback;
}
