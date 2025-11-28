// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include <string>
#include <cinttypes>
#include <PropertyManager.h>

class PropertyManager : WillowPM::PropertyManager {
public:
    ~PropertyManager() override = default;

    static WillowPM::PropertyManager *getInstance() {
        return WillowPM::PropertyManager::getInstance();
    }

    enum EStorageLocation {
        Persistent = WillowPM::PropertyListItem::eLOC_PERSISTENT,
        Temporary = WillowPM::PropertyListItem::eLOC_TEMPORARY
    };

    static int setPropertyString(const std::string &propertyName, const std::string &value,
                                 PropertyManager::EStorageLocation eLocation = EStorageLocation::Temporary) {
        return getInstance()->setPropertyString(propertyName, value,
                                                static_cast<WillowPM::PropertyManager::EStorageLocation>(eLocation));
    }

    static int setPropertyInt(const std::string &propertyName, int64_t value,
                              PropertyManager::EStorageLocation eLocation = EStorageLocation::Persistent) {
        return getInstance()->setPropertyInt(propertyName, value,
                                             static_cast<WillowPM::PropertyManager::EStorageLocation>(eLocation));
    }

    static std::string
    getPropertyString(const std::string &propertyName,
                      PropertyManager::EStorageLocation eLocation = EStorageLocation::Temporary) {
        std::string propertyString;
        getInstance()->getPropertyString(propertyName, &propertyString,
                                         static_cast<WillowPM::PropertyManager::EStorageLocation>(eLocation));
        return propertyString;
    }

    static int
    getPropertyInt(const std::string &propertyName,
                   PropertyManager::EStorageLocation eLocation = EStorageLocation::Temporary) {
        int propertyInt;
        getInstance()->getPropertyInt(propertyName, &propertyInt,
                                      static_cast<WillowPM::PropertyManager::EStorageLocation>(eLocation));
        return propertyInt;
    }

    static int removeProperty(const std::string &propertyName, bool fromAll = true) {
        return getInstance()->removeProperty(propertyName, fromAll);
    }

    static void PropertySend(const std::string &pszName, int nType) {
        getInstance()->PropertySend(pszName, nType);
    }

    static void SendProperties() {
        getInstance()->SendProperties();
    }

private:
    PropertyManager();
};
