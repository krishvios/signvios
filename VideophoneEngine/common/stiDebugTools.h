/*!
 * \file stiDebugTools.h
 * \brief See stiDebugTools.cpp
 *
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2000-2011 by Sorenson Communications, Inc. All rights reserved.
 */
#pragma once

//
// Includes
//
#include "stiDefs.h"
#include "nonstd/observer_ptr.h"
#include <utility>
#include <vector>
#include <map>
#include <atomic>
#include <string>

#if defined(stiDEBUG_OUTPUT) || APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_MEDIASERVER
#include <cstring>
#endif //stiDEBUG_OUTPUT


//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
void stiBackTrace();
void stiBackTraceToFile(int fd);

//
// Globals
//
class IDebugTool
{
public:

	IDebugTool(const IDebugTool &other) = delete;
	IDebugTool(IDebugTool &&other) = delete;
	IDebugTool &operator=(const IDebugTool &other) = delete;
	IDebugTool &operator=(IDebugTool &&other) = delete;

	virtual std::string nameGet () const = 0;

	virtual void valueSet (int value) = 0;

	virtual int valueGet () const = 0;

	virtual void persistSet (bool persist) = 0;

	virtual bool persistGet () const = 0;

	virtual int defaultValueGet () const = 0;

	virtual void valueReset () = 0;

protected:

	IDebugTool () = default;
	virtual ~IDebugTool () = default;
};


class DebugTool : public IDebugTool
{
public:

	DebugTool (const std::string &name, int initialValue);
	~DebugTool () override = default;

	DebugTool(const DebugTool &other) = delete;
	DebugTool(DebugTool &&other) = delete;
	DebugTool &operator=(const DebugTool &other) = delete;
	DebugTool &operator=(DebugTool &&other) = delete;

	operator int () const
	{
		return m_value;
	}

	operator int8_t () const
	{
		return static_cast<int8_t>(m_value);
	}

	operator uint8_t () const
	{
		return static_cast<uint8_t>(m_value);
	}

	operator bool () const
	{
		return m_value != 0;
	}

	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend int operator& (const DebugTool &lhs, const T &rhs) { return lhs.m_value & rhs; }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend int operator& (const T &lhs, const DebugTool &rhs) { return lhs & rhs.m_value; }

	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator< (const DebugTool &lhs, const T &rhs) { return lhs.m_value < rhs; }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator> (const DebugTool &lhs, const T &rhs) { return rhs < lhs.m_value; }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator<= (const DebugTool &lhs, const T &rhs) { return !(lhs.m_value > rhs); }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator>= (const DebugTool &lhs, const T &rhs) { return !(lhs.m_value < rhs); }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator== (const DebugTool &lhs, const T &rhs) { return lhs.m_value == rhs; }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator!= (const DebugTool &lhs, const T &rhs) { return !(lhs.m_value == rhs); }

	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator< (const T &lhs, const DebugTool &rhs) { return lhs < rhs.m_value; }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator> (const T &lhs, const DebugTool &rhs) { return rhs.m_value < lhs; }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator<= (const T &lhs, const DebugTool &rhs) { return !(lhs > rhs.m_value); }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator>= (const T &lhs, const DebugTool &rhs) { return !(lhs < rhs.m_value); }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator== (const T &lhs, const DebugTool &rhs) { return lhs == rhs.m_value; }
	template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
	friend bool operator!= (const T &lhs, const DebugTool &rhs) { return !(lhs == rhs.m_value); }

	DebugTool &operator= (int value);

	std::string nameGet () const override;

	void valueSet (int value) override;

	int valueGet () const override;

	void persistSet (bool persist) override;

	bool persistGet () const override;

	int defaultValueGet () const override;

	void valueReset () override;

	void load (int value);

private:

	std::string m_name;
	std::atomic<int> m_value {0};
	int m_defaultValue {0};
	std::atomic<bool> m_persist {false};
};


// The following are the debug tools. The initial state is defined
// in the cpp. The states are 0 - Off 1 or greater - On.
// These can be overridden at runtime by changing the file named in g_szTraceFileName.
// To add additional debug tools, add the declaration here and twice in the cpp

#ifdef stiDEBUGGING_TOOLS
extern DebugTool g_stiAllDebug;
extern DebugTool g_stiAPDebug;
extern DebugTool g_stiAPPacketQueueDebug;
extern DebugTool g_stiAPReadDebug;
extern DebugTool g_stiARDebug;
extern DebugTool g_stiAudibleRingerDebug;
extern DebugTool g_stiAudioDecodeDebug;
extern DebugTool g_stiAudioEncodeDebug;
extern DebugTool g_stiAudioJitterDebug;
extern DebugTool g_stiBlockListDebug;
extern DebugTool g_stiCallHandlerDebug;
extern DebugTool g_stiCallListDebug;
extern DebugTool g_stiCallStats;
extern DebugTool g_stiCaptureFileDebug;
extern DebugTool g_stiCaptureRCUDebug;
extern DebugTool g_stiCaptureTaskDebug;
extern DebugTool g_stiCapturePTZDebug;
extern DebugTool g_stiCaptureWidth;
extern DebugTool g_stiCaptureHeight;
extern DebugTool g_stiCCIDebug;
extern DebugTool g_stiCDTDebug;
extern DebugTool g_stiCircularBufferOutput;
extern DebugTool g_stiCMCallBackTracking;
extern DebugTool g_stiCMCallObjectDebug;
extern DebugTool g_stiCMCapabilities;
extern DebugTool g_stiCMChannelDebug;
extern DebugTool g_stiCMDebug;
extern DebugTool g_stiCMEventTracking;
extern DebugTool g_stiCMStateTracking;
extern DebugTool g_stiCSDebug;
extern DebugTool g_stiDHCPDebug;
extern DebugTool g_stiNetworkDebug;
extern DebugTool g_stiDialDebug;
extern DebugTool g_stiDisplayDebug;
extern DebugTool g_stiDisplayTaskDebug;
extern DebugTool g_stiDisplayTimingDebug;
extern DebugTool g_stiDNSDebug;
extern DebugTool g_stiFeaturesDebug;
extern DebugTool g_stiFECCDebug;
extern DebugTool g_stiFilePlayDebug;
extern DebugTool g_stiFrameTimingDebug;
extern DebugTool g_stiGVC_Debug;
extern DebugTool g_stiCnfDebug;
extern DebugTool g_stiHeapTrace;
extern DebugTool g_stiHoldServerDebug;
extern DebugTool g_stiHTTPTaskDebug;
extern DebugTool g_stiLightRingDebug;
extern DebugTool g_stiListDebug;
extern DebugTool g_stiLogging;
extern DebugTool g_stiLoadTestLogging;
extern DebugTool g_stiMessageListDebug;
extern DebugTool g_stiMessageQueueDebug;
extern DebugTool g_stiMessageCleanupDebug;
extern DebugTool g_stiMP4CircBufDebug;
extern DebugTool g_stiMP4FileDebug;
extern DebugTool g_stiMSDebug;
extern DebugTool g_stiNATTraversal;
extern DebugTool g_stiNSDebug;
extern DebugTool g_stiPMDebug;
extern DebugTool g_stiPSDebug;
extern DebugTool g_stiQTFileDebug;
extern DebugTool g_stiRateControlDebug;
extern DebugTool g_stiRcuDm365Debug;
extern DebugTool g_stiRcuMonitorDebug;
extern DebugTool g_stiRemoteControlDebug;
extern DebugTool g_stiResizer422To420Debug;
extern DebugTool g_stiResizerCopyDebug;
extern DebugTool g_stiResizerDebug;
extern DebugTool g_stiResizerScaleDebug;
extern DebugTool g_stiRTCPDebug;
extern DebugTool g_stiSInfoDebug;
extern DebugTool g_stiSipCallBackTracking;
extern DebugTool g_stiSipMgrDebug;
extern DebugTool g_stiSipRegDebug;
extern DebugTool g_stiSipResDebug;
extern DebugTool g_stiSSLDebug;
extern DebugTool g_stiStunDebug;
extern DebugTool g_stiSYNCDebug;
extern DebugTool g_stiTaskDebug;
extern DebugTool g_stiTPDebug;
extern DebugTool g_stiTPPacketQueueDebug;
extern DebugTool g_stiTPReadDebug;
extern DebugTool g_stiTRDebug;
extern DebugTool g_stiTraceDebug;
extern DebugTool g_stiTrueCallWaitingDebug;
extern DebugTool g_stiUdpTraceDebug;
extern DebugTool g_stiUiDebug;
extern DebugTool g_stiUiEventDebug;
extern DebugTool g_stiUIGDebug;
extern DebugTool g_stiUpdateDebug;
extern DebugTool g_stiUPnPDebug;
extern DebugTool g_stiVideoDecodeTaskDebug;
extern DebugTool g_stiVideoEncodeTaskDebug;
extern DebugTool g_stiVideoPlaybackCaptureStream2File;
extern DebugTool g_stiVideoPlaybackDebug;
extern DebugTool g_stiVideoPlaybackKeyframeDebug;
extern DebugTool g_stiVideoPlaybackPacketDebug;
extern DebugTool g_stiVideoPlaybackPacketQueueDebug;
extern DebugTool g_stiVideoPlaybackReadDebug;
extern DebugTool g_stiVideoPlaybackFlowControlDebug;
extern DebugTool g_stiVideoRecordCaptureStream2File;
extern DebugTool g_stiVideoRecordDebug;
extern DebugTool g_stiVideoRecordKeyframeDebug;
extern DebugTool g_stiVideoRecordPacketDebug;
extern DebugTool g_stiVideoRecordFlowControlDebug;
extern DebugTool g_stiVideoTracking;
extern DebugTool g_stiVRCLServerDebug;
extern DebugTool g_stiXMLCircularBufferOutput;
extern DebugTool g_stiXMLOutput;
extern DebugTool g_stiIceMgrDebug;
extern DebugTool g_stiPreferTURN;
extern DebugTool g_stiRingGroupDebug;
extern DebugTool g_stiRemoteLoggerServiceDebug;
extern DebugTool g_stiVideoInputDebug;
extern DebugTool g_stiVideoDisableTOFAutoFocus;
extern DebugTool g_stiVideoDisableIPUAutoFocus;
extern DebugTool g_stiVideoOutputDebug;
extern DebugTool g_stiVideoSizeDebug;
extern DebugTool g_stiVideoUseTestSource;
extern DebugTool g_stiVideoExcludeEncode;
extern DebugTool g_stiBluetoothDebug;
extern DebugTool g_stiHeliosDfuDebug;
extern DebugTool g_stiMonitorDebug;
extern DebugTool g_stiAudioHandlerDebug;
extern DebugTool g_stiSlicRingerDebug;
extern DebugTool g_stiTaskTimeoutValueDebug;
extern DebugTool g_stiDiscreteCameraMovements;
extern DebugTool g_stiUserInputDebug;
extern DebugTool g_stiISPDebug;
extern DebugTool g_stiFileDownloadDebug;
extern DebugTool g_stiFileUploadDebug;
extern DebugTool g_stiCallRefDebug;
extern DebugTool g_stiPlaybackReadDebug;
extern DebugTool g_stiMediaRecordDebug;
extern DebugTool g_stiMediaPlaybackDebug;
extern DebugTool g_stiDisableIce;
extern DebugTool g_stiDisableInboundAudio;
extern DebugTool g_stiDisableOutboundAudio;
extern DebugTool g_stiDisableInboundVideo;
extern DebugTool g_stiDisableOutboundVideo;
extern DebugTool g_stiDisableInboundText;
extern DebugTool g_stiDisableOutboundText;
extern DebugTool g_stiCECControlDebug;
extern DebugTool g_stiNackTimerInterval;
extern DebugTool g_stiNackAvgNumFramesToDelay;
extern DebugTool g_stiDumpPipelineGraphs;
extern DebugTool g_stiDhvStateChange;
extern DebugTool g_stiRtpSessionDebug;
extern DebugTool g_endpointMonitorDebug;
extern DebugTool g_cpuSpeedDebug;
extern DebugTool g_enable1080Encode;

#ifdef stiINTEROP_EVENT
	extern DebugTool g_stiEnableVideoProfiles;
	extern DebugTool g_stiEnableVideoCodecs;
	extern DebugTool g_stiEnableAudioCodecs;
#endif
#endif // stiDEBUGGING_TOOLS

#ifdef stiDEBUGGING_TOOLS
	#define stiDEBUG_TOOL(ToolName, Stuff) if ((ToolName) || g_stiAllDebug) {Stuff}
#else
	#define stiDEBUG_TOOL(ToolName, Stuff)
#endif // stiDEBUGGING_TOOLS



/*!
 * \brief Helps us turn on/off debug traces.
 */
class DebugTools
{
public:
	static DebugTools *instanceGet ();

	bool debugToolEditingEnabled ();

	void fileLoad();
	void fileSave();

	nonstd::observer_ptr<IDebugTool> debugToolGet (
		const std::string &name);

	std::vector<nonstd::observer_ptr<IDebugTool>> debugToolListGet ();

	static void registerTool (nonstd::observer_ptr<DebugTool> debugTool);

#if defined(stiDEBUG_OUTPUT) || APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_MEDIASERVER
	typedef void (__stdcall *DebugOutputType)(std::string);
	DebugOutputType mpDebugOutput;
//	static DebugOutputType gpErrorOutput;

	void DebugOutput(std::string isText);
	static void ErrorOutput(std::string isText);
#endif //stiDEBUG_OUTPUT

private:

	DebugTools(); //!< In practice, call GetInstance() instead.

	std::string m_TraceFileName;
	bool m_traceFileExists{false}; //!< Used to control availability of QA tools screen.
	static std::map<std::string, nonstd::observer_ptr<DebugTool>> m_debugTools;
};

