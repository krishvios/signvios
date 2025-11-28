// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019-2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiEventQueue.h"
#include "IstiUserInput.h"
#include <map>
#include <string>
#include <functional>
#include "GStreamerEvent.h"
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Object.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>

class CstiMonitorTask;


class SocketListenerBase : public CstiEventQueue
{
public:

	SocketListenerBase (CstiMonitorTask *monitorTask);
	~SocketListenerBase () override;

	SocketListenerBase (const SocketListenerBase &other) = delete;
	SocketListenerBase (SocketListenerBase &&other) = delete;
	SocketListenerBase &operator = (const SocketListenerBase &other) = delete;
	SocketListenerBase &operator = (SocketListenerBase &&other) = delete;

	int openDomainSocket (const std::string &path);
	stiHResult initialize ();
	stiHResult startup ();

private:

	void eventReadCommandSocket ();
	void eventReadWebServerSocket ();

	void commandProcess (const Poco::JSON::Object &root, int responseFd);

	void dashboardDataSend (const Poco::JSON::Object &root);
	void dashboardDataCollectionStop ();
	void probeDataSend ();

private:

	void testModeSet (const Poco::JSON::Object &root, int responseFd);
	void systemInfo (const Poco::JSON::Object &root, int responseFd);
	void usbInfo (const Poco::JSON::Object &root, int responseFd);

	void audioIn (const Poco::JSON::Object &root, int responseFd);
	void audioOut (const Poco::JSON::Object &root, int responseFd);
	void headphonesConnected (const Poco::JSON::Object &root, int responseFd);
	void microphoneConnected (const Poco::JSON::Object &root, int responseFd);

	void singleShotFocus (const Poco::JSON::Object &root, int responseFd);
	void eventSingleShotFocus (const Poco::JSON::Object &root, int contrast);
	void eventSingleShotFocusTimeout (const Poco::JSON::Object &root);

	void contrastGet (const Poco::JSON::Object &root, int responseFd);
	void contrastLoopStart (const Poco::JSON::Object &root, int responseFd);
	void contrastLoopStop (const Poco::JSON::Object &root, int responseFd);

	void vcmPosition (const Poco::JSON::Object &root, int responseFd);
	void vcmSet (const Poco::JSON::Object &root, int responseFd);
	void vcmGet (const Poco::JSON::Object &root, int responseFd);

	void lightRing (const Poco::JSON::Object &root, int responseFd);
	void lightRingPattern (const Poco::JSON::Object &root, int responseFd);
	void alertLed (const Poco::JSON::Object &root, int responseFd);
	void alertLedPattern (const Poco::JSON::Object &root, int responseFd);
	void signMailLed (const Poco::JSON::Object &root, int responseFd);
	void missedCallLed (const Poco::JSON::Object &root, int responseFd);
	void statusLed (const Poco::JSON::Object &root, int responseFd);

	void slicRing (const Poco::JSON::Object &root, int responseFd);
	void slicDetect (const Poco::JSON::Object &root, int responseFd);
	void eventSlicDetect (const Poco::JSON::Object &root, bool connected, int mRen);
	void eventSlicDetectTimeout (const Poco::JSON::Object &root);

	void bluetoothScan (const Poco::JSON::Object &root, int responseFd);
	void eventBluetoothScanComplete (const Poco::JSON::Object &root);

	void wifiAccessPointsGet (const Poco::JSON::Object &root, int responseFd);

	void buttonStatus (const Poco::JSON::Object &root, int responseFd);
	void eventButtonStateChange (int buttonCode, bool pressed);

	void remoteInput (const Poco::JSON::Object &root, int responseFd);
	void eventRemoteInput (int buttonCode);
	void eventRemoteInputTimeout ();
	static std::string buttonNameGet (int buttonCode);

	void timeOfFlight (const Poco::JSON::Object &root, int responseFd);
	void eventTimeOfFlight (const Poco::JSON::Object &root, int distance);
	void eventTimeOfFlightTimeout (const Poco::JSON::Object &root);

	void eventQueueStatus (const Poco::JSON::Object &root, int responseFd);

	void debugTool (const Poco::JSON::Object &root, int responseFd);
	void debugToolGet (const Poco::JSON::Object &root, int responseFd);
	void debugToolSet (const Poco::JSON::Object &root, int responseFd);

	void pipelines (const Poco::JSON::Object &root, int responseFd);
	void pipeline (const Poco::JSON::Object &root, int responseFd);
	void pipelinesProbe (const Poco::JSON::Object &root, int responseFd);
	void eventBufferProbeCallback (const std::string &path, const std::string &type);
	void eventEventProbeCallback (const std::string &path, const std::string &type, const GStreamerEvent &event);

	void removePipelineProbes ();
	void pipelinesProperties (const Poco::JSON::Object &root, int responseFd);
	void pipelinesPropertyInfo (const Poco::JSON::Object &root, int responseFd);
	void pipelinesProbeList (const Poco::JSON::Object &root, int responseFd);

	void serialNumber (const Poco::JSON::Object &root, int responseFd);
	void serialNumberGet (const Poco::JSON::Object &root, int responseFd);
	void serialNumberSet (const Poco::JSON::Object &root, int responseFd);
	void hdcc (const Poco::JSON::Object &root, int responseFd);
	void hdccGet (const Poco::JSON::Object &root, int responseFd);
	void hdccSet (const Poco::JSON::Object &root, int responseFd);
	void hdccOverrideDelete (const Poco::JSON::Object &root, int responseFd);

	void fanDutyCycle (const Poco::JSON::Object &root, int responseFd);
	void fanDutyCycleGet (const Poco::JSON::Object &root, int responseFd);
	void fanDutyCycleSet (const Poco::JSON::Object &root, int responseFd);

private:

	// Thread related commands...

	void vcmPattern (const Poco::JSON::Object &root, int responseFd);
	bool vcmPatternStop ();
	void vcmPatternTask (int minFocus, int maxFocus);
	std::thread m_vcmPatternThread;
	bool m_runVcmPattern {false};
	std::mutex m_vcmPatternMutex;
	std::condition_variable m_vcmPatternCondVar;
	
	void iconLedPattern (const Poco::JSON::Object &root, int responseFd);
	bool iconLedPatternStop ();
	void iconLedPatternTask ();
	std::thread m_iconLedPatternThread;
	bool m_runIconLedPattern {false};
	std::mutex m_iconLedPatternMutex;
	std::condition_variable m_iconLedPatternCondVar;

	void statusLedPattern (const Poco::JSON::Object &root, int responseFd);
	bool statusLedPatternStop ();
	void statusLedPatternTask ();
	std::thread m_statusLedPatternThread;
	bool m_runStatusLedPattern {false};
	std::mutex m_statusLedPatternMutex;
	std::condition_variable m_statusLedPatternCondVar;

	void contrastCallback (int contrast);
	int m_contrast {0};
	bool m_contrastLoopRunning {false};

private:

	int m_commandSocketFd {-1};
	int m_webServerSocketFd {-1};
	int m_webServerSendFd {-1};
	bool m_testMode {false};
	std::map<std::string, std::function<void(const Poco::JSON::Object, int)>> m_commandMap;

	bool m_headphonesConnected {false};
	bool m_microphoneConnected {false};

	std::vector<float> m_leftMicMagnitudes;
	std::vector<float> m_rightMicMagnitudes;
	std::chrono::steady_clock::time_point m_lastMicUpdate;

	CstiSignalConnection m_focusCompleteSignalConnection;
	CstiSignalConnection m_focusTimeoutConnection;
	vpe::EventTimer m_focusTimeoutTimer;
	int m_focusFd {-1};

	CstiSignalConnection m_slicDetectSignalConnection;
	CstiSignalConnection m_slicDetectTimeoutConnection;
	vpe::EventTimer m_slicDetectTimeoutTimer;
	int m_slicDetectFd {-1};

	CstiSignalConnection m_bluetoothScanConnection;
	vpe::EventTimer m_bluetoothScanTimer;
	int m_bluetoothScanFd {-1};

	struct RemoteButton
	{
		std::string name;
		std::chrono::steady_clock::time_point time;
	};

	std::deque<RemoteButton> m_remoteButtons;

	CstiSignalConnection m_remoteInputTimeoutConnection;
	vpe::EventTimer m_remoteInputTimeoutTimer;
	Poco::JSON::Object m_remoteInputCommand;
	int m_remoteInputFd {-1};
	static const std::map <int, std::string> m_remoteButtonMap;

	CstiSignalConnection m_tofTimeoutConnection;
	vpe::EventTimer m_tofTimeoutTimer;
	int m_tofFd {-1};

	CstiSignalConnection::Collection m_signalConnections;

	struct button
	{
		std::string name;
		bool pressed;
	};

	std::map<int, button> m_buttonMap {
		{IstiUserInput::RemoteButtons::eSMRK_BSP_ENTER,	{"enter", false}},
		{IstiUserInput::RemoteButtons::eSMRK_BSP_UP,	{"up", false}},
		{IstiUserInput::RemoteButtons::eSMRK_BSP_DOWN,	{"down", false}},
		{IstiUserInput::RemoteButtons::eSMRK_BSP_LEFT,	{"left", false}},
		{IstiUserInput::RemoteButtons::eSMRK_BSP_RIGHT,	{"right", false}},
		{IstiUserInput::RemoteButtons::eSMRK_BSP_POWER, {"power", false}}
	};

	struct probeData
	{
		probeData ()
		:
		probeAddedTime (std::chrono::steady_clock::now ()),
		lastCheckedTime (probeAddedTime)
		{
		}

		int totalCount {0};
		int lastCheckedCount {0};
		std::chrono::steady_clock::time_point probeAddedTime;
		std::chrono::steady_clock::time_point lastCheckedTime;
	};
	std::map<std::pair<std::string,std::string>,probeData> m_bufferProbes;
	vpe::EventTimer m_probeDataTimer;

public:

	CstiSignal<bool> testModeSetSignal;
};
