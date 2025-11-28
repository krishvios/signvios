// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef ASTIVIDEOPHONEENGINE_H
#define ASTIVIDEOPHONEENGINE_H

#include "IstiVideophoneEngine.h"
#include "VideophoneEngineCallback.h"

#include <string>
#include <map>

class AstiVideophoneEngine {
public:
    static void setMACAddress(const char *macAddress);
    static bool setNetworkInformation(bool isEngineInitialized);
    static void setDNS(bool isEngineInitialized);
    static void setIPAddress(bool isEngineInitialized, const char *ipAddress);
    static void setGatewayIP(bool isEngineInitialized, const char *ipAddress);
    static void setAndroidVersion(int sdkVersion);
    static void seth263Only(bool h263Only);
	static void setPreferredDisplaySize(int preferredWidth, int preferredHeight);

    bool initialize(const char* version, const char* appWritablePath);
    void destroy();
    void addCoreOption(const char *key, const char *value) { options[key] = value; }
    void setIsBad3GNetwork(bool isBad);
    void setPublicIpAddressAuto();
    void setupVideophoneEngineSignalCallbacks();
    
    IstiVideophoneEngine *getVideophoneEngine() { return m_stiVideophoneEngine; }
	void setVideophoneEngineCallback(VideophoneEngineCallback *callback);
	void registerDHVCallback(IstiCall *call);

private:
    IstiVideophoneEngine *m_stiVideophoneEngine;
    std::map<std::string, std::string> options;
    int sdkVersion;
	VideophoneEngineCallback *m_videophoneEngineCallback = nullptr;
	CstiSignalConnection::Collection m_signalConnections;
};

#endif
