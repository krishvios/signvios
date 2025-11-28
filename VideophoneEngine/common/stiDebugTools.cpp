/*!
 * \file stiDebugTools.cpp
 * \brief This Module contains debug functions that are used to help in testing
 * and debugging the project.
 *
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2000-2011 by Sorenson Communications, Inc. All rights reserved.
 */

//
// Includes
//
#include <cstdio>
#include <cstdlib>
#if !defined(WIN32) && APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
	#include <execinfo.h>
#endif //WIN32
#include "stiDebugTools.h"
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include "stiTools.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//
std::map<std::string, nonstd::observer_ptr<DebugTool>> DebugTools::m_debugTools;

// The following are the debug tools. The initial state is defined.
// The states are (0 - Off) (1 or greater - On).
// To add additional debug tools, add the declaration here and in the h

DebugTool::DebugTool (
	const std::string &name,
	int value)
:
	m_name(name),
	m_value (value),
	m_defaultValue (value)
{
	DebugTools::registerTool (nonstd::make_observer<DebugTool>(this));
}

DebugTool &DebugTool::operator= (int value)
{
	valueSet (value);
	return *this;
}

std::string DebugTool::nameGet () const
{
	return m_name;
}

void DebugTool::valueSet (int value)
{
	m_value = value;

	if (persistGet ())
	{
		DebugTools::instanceGet ()->fileSave ();
	}
}

int DebugTool::valueGet () const
{
	return m_value;
}

void DebugTool::persistSet (bool persist)
{
	bool previousValue = m_persist;
	m_persist = persist;

	if (persist != previousValue)
	{
		DebugTools::instanceGet ()->fileSave ();
	}
}

bool DebugTool::persistGet () const
{
	return m_persist;
}

int DebugTool::defaultValueGet () const
{
	return m_defaultValue;
}

void DebugTool::valueReset ()
{
	m_value = defaultValueGet ();

	if (persistGet ())
	{
		DebugTools::instanceGet()->fileSave ();
	}
}

void DebugTool::load (int value)
{
	m_value = value;
	m_persist = true;
}

#ifdef stiDEBUGGING_TOOLS
#define DEBUG_TOOL(t, v) DebugTool t (#t, (v));
DEBUG_TOOL(g_stiAllDebug, 0);
DEBUG_TOOL(g_stiAPDebug, 0);
DEBUG_TOOL(g_stiAPPacketQueueDebug, 0);
DEBUG_TOOL(g_stiAPReadDebug, 0);
DEBUG_TOOL(g_stiARDebug, 0);
DEBUG_TOOL(g_stiAudibleRingerDebug, 0);
DEBUG_TOOL(g_stiAudioDecodeDebug, 0);
DEBUG_TOOL(g_stiAudioEncodeDebug, 0);
DEBUG_TOOL(g_stiAudioJitterDebug, 0);
DEBUG_TOOL(g_stiBlockListDebug, 0);
DEBUG_TOOL(g_stiCallHandlerDebug, 0);
DEBUG_TOOL(g_stiCallListDebug, 0);
DEBUG_TOOL(g_stiCallStats, 0);
DEBUG_TOOL(g_stiCaptureFileDebug, 0);
DEBUG_TOOL(g_stiCaptureRCUDebug, 0);
DEBUG_TOOL(g_stiCaptureTaskDebug, 0);
DEBUG_TOOL(g_stiCapturePTZDebug, 0);
DEBUG_TOOL(g_stiCaptureWidth, 0);
DEBUG_TOOL(g_stiCaptureHeight, 0);
DEBUG_TOOL(g_stiCCIDebug, 0);
DEBUG_TOOL(g_stiCDTDebug, 0);
DEBUG_TOOL(g_stiCircularBufferOutput, 0);
DEBUG_TOOL(g_stiCMCallBackTracking, 0);
DEBUG_TOOL(g_stiCMCallObjectDebug, 0);
DEBUG_TOOL(g_stiCMCapabilities, 0);
DEBUG_TOOL(g_stiCMChannelDebug, 0);
DEBUG_TOOL(g_stiCMDebug, 0);
DEBUG_TOOL(g_stiCMEventTracking, 0);
DEBUG_TOOL(g_stiCMStateTracking, 0);
DEBUG_TOOL(g_stiCSDebug, 0);
DEBUG_TOOL(g_stiDHCPDebug, 0);
DEBUG_TOOL(g_stiNetworkDebug, 0);
DEBUG_TOOL(g_stiDialDebug, 0);
DEBUG_TOOL(g_stiDisplayDebug, 0);
DEBUG_TOOL(g_stiDisplayTaskDebug, 0);
DEBUG_TOOL(g_stiDisplayTimingDebug, 0);
DEBUG_TOOL(g_stiDNSDebug, 0);
DEBUG_TOOL(g_stiFeaturesDebug, 0);
DEBUG_TOOL(g_stiFECCDebug, 0);
DEBUG_TOOL(g_stiFilePlayDebug, 0);
DEBUG_TOOL(g_stiFrameTimingDebug, 0);
DEBUG_TOOL(g_stiGVC_Debug, 0);
DEBUG_TOOL(g_stiCnfDebug, 0);
DEBUG_TOOL(g_stiHeapTrace, 0);
DEBUG_TOOL(g_stiHoldServerDebug, 0);
DEBUG_TOOL(g_stiHTTPTaskDebug, 0);
DEBUG_TOOL(g_stiLightRingDebug, 0);
DEBUG_TOOL(g_stiListDebug, 0);
DEBUG_TOOL(g_stiLoadTestLogging, 0);
DEBUG_TOOL(g_stiLogging, 1);
DEBUG_TOOL(g_stiMessageListDebug, 0);
DEBUG_TOOL(g_stiMessageQueueDebug, 0);
DEBUG_TOOL(g_stiMessageCleanupDebug, 0);
DEBUG_TOOL(g_stiMP4CircBufDebug, 0);
DEBUG_TOOL(g_stiMP4FileDebug, 0);
DEBUG_TOOL(g_stiMSDebug, 0);
DEBUG_TOOL(g_stiNATTraversal, 0);
DEBUG_TOOL(g_stiNSDebug, 0);
DEBUG_TOOL(g_stiPMDebug, 0);
DEBUG_TOOL(g_stiPSDebug, 0);
DEBUG_TOOL(g_stiQTFileDebug, 0);
DEBUG_TOOL(g_stiRateControlDebug, 0);
DEBUG_TOOL(g_stiRcuDm365Debug, 0);
DEBUG_TOOL(g_stiRcuMonitorDebug, 0);
DEBUG_TOOL(g_stiRemoteControlDebug, 0);
DEBUG_TOOL(g_stiResizer422To420Debug, 0);
DEBUG_TOOL(g_stiResizerCopyDebug, 0);
DEBUG_TOOL(g_stiResizerDebug, 0);
DEBUG_TOOL(g_stiResizerScaleDebug, 0);
DEBUG_TOOL(g_stiRTCPDebug, 0);
DEBUG_TOOL(g_stiSInfoDebug, 0);
DEBUG_TOOL(g_stiSipCallBackTracking, 0);
DEBUG_TOOL(g_stiSipMgrDebug, 0);
DEBUG_TOOL(g_stiSipRegDebug, 0);
DEBUG_TOOL(g_stiSipResDebug, 0);
DEBUG_TOOL(g_stiSSLDebug, 0);
DEBUG_TOOL(g_stiStunDebug, 0);
DEBUG_TOOL(g_stiSYNCDebug, 0);
DEBUG_TOOL(g_stiSystemDataDebug, 0);
DEBUG_TOOL(g_stiTaskDebug, 0);
DEBUG_TOOL(g_stiTraceDebug, 1);
DEBUG_TOOL(g_stiTrueCallWaitingDebug, 0);
DEBUG_TOOL(g_stiTPDebug, 0);
DEBUG_TOOL(g_stiTPPacketQueueDebug, 0);
DEBUG_TOOL(g_stiTPReadDebug, 0);
DEBUG_TOOL(g_stiTRDebug, 0);
DEBUG_TOOL(g_stiUdpTraceDebug, 0);
DEBUG_TOOL(g_stiUiDebug, 0);
DEBUG_TOOL(g_stiUiEventDebug, 0);
DEBUG_TOOL(g_stiUIGDebug, 0);
DEBUG_TOOL(g_stiUpdateDebug, 0);
DEBUG_TOOL(g_stiUPnPDebug, 0);
DEBUG_TOOL(g_stiVideoDecodeTaskDebug, 0);
DEBUG_TOOL(g_stiVideoEncodeTaskDebug, 0);
DEBUG_TOOL(g_stiVideoPlaybackCaptureStream2File, 0);
DEBUG_TOOL(g_stiVideoPlaybackDebug, 0);
DEBUG_TOOL(g_stiVideoPlaybackKeyframeDebug, 0);
DEBUG_TOOL(g_stiVideoPlaybackPacketDebug, 0);
DEBUG_TOOL(g_stiVideoPlaybackPacketQueueDebug, 0);
DEBUG_TOOL(g_stiVideoPlaybackReadDebug, 0);
DEBUG_TOOL(g_stiVideoPlaybackFlowControlDebug, 0);
DEBUG_TOOL(g_stiVideoRecordCaptureStream2File, 0);
DEBUG_TOOL(g_stiVideoRecordDebug, 0);
DEBUG_TOOL(g_stiVideoRecordKeyframeDebug, 0);
DEBUG_TOOL(g_stiVideoRecordPacketDebug, 0);
DEBUG_TOOL(g_stiVideoRecordFlowControlDebug, 0);
DEBUG_TOOL(g_stiVideoTracking, 0);
DEBUG_TOOL(g_stiVRCLServerDebug, 0);
DEBUG_TOOL(g_stiXMLCircularBufferOutput, 0);
DEBUG_TOOL(g_stiXMLOutput, 0);
DEBUG_TOOL(g_stiIceMgrDebug, 0);
DEBUG_TOOL(g_stiPreferTURN, 0);
DEBUG_TOOL(g_stiRingGroupDebug, 0);
DEBUG_TOOL(g_stiRemoteLoggerServiceDebug, 0);
DEBUG_TOOL(g_stiVideoInputDebug, 0);
DEBUG_TOOL(g_stiVideoDisableTOFAutoFocus, 0);
DEBUG_TOOL(g_stiVideoDisableIPUAutoFocus, 0);
DEBUG_TOOL(g_stiVideoOutputDebug, 0);
DEBUG_TOOL(g_stiVideoSizeDebug, 0);
DEBUG_TOOL(g_stiVideoUseTestSource, 0);
DEBUG_TOOL(g_stiVideoExcludeEncode, 0);
DEBUG_TOOL(g_stiBluetoothDebug, 0);
DEBUG_TOOL(g_stiHeliosDfuDebug, 0);
DEBUG_TOOL(g_stiMonitorDebug, 0);
DEBUG_TOOL(g_stiAudioHandlerDebug, 0);
DEBUG_TOOL(g_stiSlicRingerDebug, 0);
DEBUG_TOOL(g_stiTaskTimeoutValueDebug, 0);
DEBUG_TOOL(g_stiDiscreteCameraMovements, 0);
DEBUG_TOOL(g_stiUserInputDebug, 0);
DEBUG_TOOL(g_stiISPDebug, 0);
DEBUG_TOOL(g_stiFileDownloadDebug, 0);
DEBUG_TOOL(g_stiFileUploadDebug, 0);
DEBUG_TOOL(g_stiCallRefDebug, 0);
DEBUG_TOOL(g_stiPlaybackReadDebug, 0);
DEBUG_TOOL(g_stiMediaRecordDebug, 0);
DEBUG_TOOL(g_stiMediaPlaybackDebug, 0);
DEBUG_TOOL(g_stiDisableIce, 0); 			// 1 to disable, 0 to enable
DEBUG_TOOL(g_stiDisableInboundAudio, 0);
DEBUG_TOOL(g_stiDisableOutboundAudio, 0);
DEBUG_TOOL(g_stiDisableInboundVideo, 0);
DEBUG_TOOL(g_stiDisableOutboundVideo, 0);
DEBUG_TOOL(g_stiDisableInboundText, 0);
DEBUG_TOOL(g_stiDisableOutboundText, 0);
DEBUG_TOOL(g_stiCECControlDebug, 0);
DEBUG_TOOL(g_stiNackTimerInterval, 0);
DEBUG_TOOL(g_stiNackAvgNumFramesToDelay, 0);
DEBUG_TOOL(g_stiDumpPipelineGraphs, 0);
DEBUG_TOOL(g_stiDhvStateChange, 0);
DEBUG_TOOL(g_stiRtpSessionDebug, 0);
DEBUG_TOOL(g_endpointMonitorDebug, 0);
DEBUG_TOOL(g_cpuSpeedDebug, 0);
DEBUG_TOOL(g_enable1080Encode, 0);

#ifdef stiINTEROP_EVENT
DEBUG_TOOL(g_stiEnableVideoProfiles, 7);	// 0x01 = baseline, 0x02 = main, 0x04 = high, 7 = all three
DEBUG_TOOL(g_stiEnableVideoCodecs, 7);		// 0x01 = H.263, 0x02 = H.264, 0x04 = H.265, 7 = all
DEBUG_TOOL(g_stiEnableAudioCodecs, 7);		// 0x01 = PCMA, 0x02 = PCMU, 0x04 = G722, 7 = all
#endif

#endif // stiDEBUGGING_TOOLS

// Initialize static variables:

/*!
 * \brief Constructs the class and loads the file.
 * NOTE: To get an object of this class, you should use GetInstance instead.
 */
DebugTools::DebugTools()
{
	std::string DynamicDataFolder;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	std::stringstream TraceFileName;

	TraceFileName << DynamicDataFolder << "trace.conf";

	m_TraceFileName = TraceFileName.str ();
}


bool DebugTools::debugToolEditingEnabled ()
{
	return m_traceFileExists;
}


/*!
 * \brief Saves the current settings to flash.
 */
void DebugTools::fileSave()
{
	FILE *pFile = fopen(m_TraceFileName.c_str (), "w+");

	if (pFile)
	{
		for (auto &tool: m_debugTools)
		{
			if (tool.second->persistGet ())
			{
				fprintf(pFile, "%s = %d\r\n", tool.second->nameGet ().c_str (), static_cast<int>(*tool.second));
			}
		}

		fclose(pFile);
	}
}


/*!
 * \brief Gets (and possibly creates) the global instance of this class.
 * \return The global instance.
 */
DebugTools *DebugTools::instanceGet ()
{
	// Meyers singleton: a local static variable (cleaned up automatically at exit)
	static DebugTools localDebugTools;

	return &localDebugTools;
}


void DebugTools::registerTool (nonstd::observer_ptr<DebugTool> debugTool)
{
	m_debugTools.emplace(debugTool->nameGet (), debugTool);
}


nonstd::observer_ptr<IDebugTool> DebugTools::debugToolGet (
	const std::string &name)
{
	nonstd::observer_ptr<IDebugTool> tool;

	auto iter = m_debugTools.find(name);

	if (iter != m_debugTools.end ())
	{
		tool = iter->second;
	}

	return tool;
}


std::vector<nonstd::observer_ptr<IDebugTool>> DebugTools::debugToolListGet ()
{
	std::vector<nonstd::observer_ptr<IDebugTool>> list;

	for (auto &tool: m_debugTools)
	{
		list.emplace_back(tool.second);
	}

	return list;
}


/*!
 * \brief Loads the tool settings from the flash file.
 * The parser expects: [#]NAME[' ',\\t,'=']*[VALUE][\\r,\\n]
 */
void DebugTools::fileLoad()
{
	std::ifstream traceFile(m_TraceFileName);

	m_traceFileExists = traceFile.is_open ();
	//stiSetDefaultTraceOrder();
	if (m_traceFileExists)
	{
		std::string line;

		while (std::getline (traceFile, line))
		{
			auto parts = splitString (line, '=');

			// The line should only have the debug tool name
			// and the value. Everything else will be
			// ignored.
			if (parts.size () == 2)
			{
				Trim (&parts[0]);
				Trim (&parts[1]);

				auto iter = m_debugTools.find(parts[0]);

				if (iter != m_debugTools.end ())
				{
					try
					{
						iter->second->load (std::stoi(parts[1]));
					}
					catch (...)
					{
					}
				}
			}
		}

		traceFile.close ();
	}
}

//
// Locals
//

//
// Function Definitions
//

void stiBackTrace()
{
#ifndef WIN32
	stiBackTraceToFile(fileno(stdout));
#else
	stiBackTraceToFile(_fileno(stdout));
#endif
}

/*!\brief Prints a backtrace to the file descriptor specified
 *
 * Note: This function is a signal handler and therefore must only
 * use functions approved for use within signal handlers by POSIX.
 * See 'man 7 signal' for details.
 *
 */
void stiBackTraceToFile(int fd)
{
#if !defined(WIN32) && APPLICATION != APP_NTOUCH_ANDROID && APPLICATION != APP_DHV_ANDROID
	void *trace[32];
	int trace_size = 0;
	int Length; stiUNUSED_ARG(Length);
	const char szLabel[] = "[bt] Execution path:\n";

	trace_size = backtrace(trace, 32);

	Length = write (fd, szLabel, sizeof(szLabel) - 1);

	backtrace_symbols_fd(trace, trace_size, fd);

	Length = write(fd, "\n\n", 2);
#else
	// backtrace is GNU C call that creates a list of pointers to functions that are active in the current thread.
	// TODO: This needs to be implemented using the WIN32 process control and threading libraries.  
	//		 Possibly StackWalker at ( http://www.codeproject.com/KB/threads/StackWalker.aspx ) may do the trick.
	//		 StackWaler would need to have a new method defined to fill the trace and messages array with the stack trace messages or
	//       by passing the FILE * handle pOutputFile and letting StackWalker output the stack to the file handle.
#endif
}

#if defined(stiDEBUG_OUTPUT) || APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_MEDIASERVER
void DebugTools::DebugOutput(std::string isText)
{
	if (mpDebugOutput != NULL)
	{
		if (isText[isText.length()-1] == 0x0a)
		{
			isText.resize(isText.length()-1);
		}
		mpDebugOutput(isText);
	}
}

void DebugTools::ErrorOutput(std::string isText)
{
	/*if (gpErrorOutput != NULL)
	{
		gpErrorOutput(isText);
	}*/
}
#endif //stiDEBUG_OUTPUT

// end file stiDebugTools.cpp
