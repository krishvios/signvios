// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "BleDevice.h"

static const std::string AUTHENTICATE_KEY = ">T*u(eoT?EqQ!_y9";
static const std::vector<uint8_t> AUTHORIZED = {'A','U','T','H','O','R','I','Z','E','D'};

#define PULSE_MAX_COMMAND_LENGTH 128

static const std::string PULSE_CUSTOM_PATTERN_START_COMMAND = "PATTERN_START";
static const std::string PULSE_CUSTOM_PATTERN_FRAME_COMMAND = "FRAME";

static const std::string PULSE_PRESET_PATTERN_START_COMMAND = "PRESET_PATTERN_START";

static const std::string PULSE_PATTERN_STOP_COMMAND = "PATTERN_STOP";

static const std::string PULSE_MISSED_ON_COMMAND = "MISSED_ON";
static const std::string PULSE_MISSED_OFF_COMMAND = "MISSED_OFF";

static const std::string PULSE_SIGNMAIL_ON_COMMAND = "SIGNMAIL_ON";
static const std::string PULSE_SIGNMAIL_OFF_COMMAND = "SIGNMAIL_OFF";

static const std::string PULSE_INFO_GET_COMMAND = "INFO_GET";
static const std::string PULSE_INFO_RESPONSE_COMMAND = "INFO_RESPONSE";

static const std::string PULSE_ALL_OFF = "ALL_OFF";

class PulseDevice : public BleDevice
{
public:
	PulseDevice (std::unique_ptr<BleIODevice> ioDevice, std::function<void(BleDevice*)> callback, CstiEventQueue *eventQueue);
	~PulseDevice ();
	
	virtual bool isPulseDevice () override {return true;};
	
	virtual void didAddService (const std::string &serviceUuid) override;
	virtual void didAddCharacteristic (const std::string &serviceUuid, const std::string &characUuid) override;
	virtual void didUpdateStartNotify (const std::string &serviceUuid, const std::string &characUuid, bool enabled) override;
	virtual void didChangeCharacteristicValue (const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value) override;
	
	void reset ();
	
	void presetRingPatternStart (
		int pattern,
		uint8_t red,
		uint8_t green,
		uint8_t blue,
		uint8_t brightness);
	
	void customRingPatternStart (
		const std::vector<std::string> &frames);
	
	void ringStop ();
	
	void missedSet (bool value);
	
	void signmailSet (bool value);
	
	void allOff ();
	
	void commandSend (const std::string &command, const std::string &arguments);
	
	inline bool isAuthenticated () {return m_authenticated;};
	
	std::string firmwareVersionGet () const {return m_version;};
	std::string serialNumberGet () const {return m_serialNumber;};
	void restartInBootloader (const std::string &name);
	
private:
	bool deviceReady ();
	
	void infoGet ();
	
	void verifyFirmware ();
	
	// Callback Events
	void readCharacValueChangedEvent (const std::vector<uint8_t> &value);
	void authCharacValueChangedEvent (const std::vector<uint8_t> &value);
	void dfuCharacValueChangedEvent (const std::vector<uint8_t> &value);
	
private:
	bool m_receivedPulseService = false;
	bool m_receivedPulseReadChar = false;
	bool m_receivedPulseWriteChar = false;
	bool m_receivedPulseAuthChar = false;
	bool m_receivedDfuService = false;
	bool m_receivedDfuButtonlessChar = false;
	bool m_notifyingPulseReadChar = false;
	bool m_notifyingPulseAuthChar = false;
	bool m_notifyingDfuButtonlessChar = false;
	
	bool m_authenticated = false;
	std::string m_version;
	std::string m_serialNumber;
	
	int m_writeSequenceNumber = 0;
	
	std::function<void(BleDevice*)> m_firmwareVerifyCallback = nullptr;
};


