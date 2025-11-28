// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include <string>


#define PULSE_SERVICE_UUID							"6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define PULSE_WRITE_CHARACTERISTIC_UUID				"6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define PULSE_READ_CHARACTERISTIC_UUID				"6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define PULSE_AUTHENTICATE_CHARACTERISTIC_UUID		"6e400004-b5a3-f393-e0a9-e50e24dcca9e"
#define PULSE_DFU_CONTROL_POINT_CHARACTERISTIC_UUID	"8ec90001-f315-4f60-9fb8-838830daea50"
#define PULSE_DFU_PACKET_CHARACTERISTIC_UUID		"8ec90002-f315-4f60-9fb8-838830daea50"
#define PULSE_DFU_BUTTONLESS_CHARACTERISTIC_UUID	"8ec90003-f315-4f60-9fb8-838830daea50"
#define PULSE_GATT_CHARACTERISTIC_UUID				"00002a05-0000-1000-8000-00805f9b34fb"

#define HELIOS_DFU_TARGET_NAME	"DfuPulse"

extern const std::string PropertyPaired;
extern const std::string PropertyTrusted;
extern const std::string PropertyConnected;
extern const std::string PropertyBlocked;
extern const std::string PropertyName;
extern const std::string PropertyAlias;
extern const std::string PropertyUUIDs;
extern const std::string PropertyServicesResolved;
extern const std::string PropertyAddress;
extern const std::string PropertyIcon;
extern const std::string PropertyClass;
extern const std::string PropertyNotifying;
extern const std::string PropertyValue;
extern const std::string PropertyRSSI;

namespace vpe {

enum class transportType {
	typeUnknown = 0,
	typeSco,
	typeA2dp,
};

struct BluetoothAudioDevice {
	std::string m_address;
	bool m_connected {false};
	transportType m_transportType {transportType::typeUnknown};
	int m_sampleRate {0};
};

};
