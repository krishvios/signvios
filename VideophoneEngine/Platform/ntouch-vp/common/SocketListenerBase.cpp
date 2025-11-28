// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019-2020 Sorenson Communications, Inc. -- All rights reserved

#include "SocketListenerBase.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fstream>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Array.h>
#include "JsonTestResponse.h"
#include <boost/optional.hpp>

#include "stiOS.h"
#include "stiTools.h"
#include "IstiPlatform.h"
#include "IPlatformVP.h"
#include "IAudioTest.h"
#include "IstiLightRing.h"
#include "ILightRingVP.h"
#include "IstiStatusLED.h"
#include "ISLICRinger.h"
#include "IstiVideoInput.h"
#include "IVideoInputVP2.h"
#include "IstiUserInput.h"
#include "IstiBluetooth.h"
#include "IBluetoothVP.h"
#include "IstiNetwork.h"
#include "IFanControl.h"
#include "CstiMonitorTask.h"
#include "stiDebugTools.h"
#include "GStreamerPipeline.h"

#define SOCKET_PATH "/tmp/.ntouchIpcSocket"
#define WEBSERVER_SOCKET_PATH "/tmp/.ntouchWebServerSocket"
#define MAX_COMMAND_SIZE 512
#define FOCUS_TIMEOUT 5000
#define SLIC_DETECT_TIMEOUT 4000
#define BLUETOOTH_SCAN_TIME_DEFAULT 5000
#define REMOTE_INPUT_TIMEOUT 30000
#define REMOTE_INPUT_EXPIRATION 500
#define TOF_TIMEOUT 1000
#define PROBE_DATA_PERIOD 1000
#define MIC_DATA_EXPIRATION 500

const std::map <int, std::string> SocketListenerBase::m_remoteButtonMap
{
	{IstiUserInput::eSMRK_POWER_TOGGLE, "powerToggle"},
	{IstiUserInput::eSMRK_SHOW_CONTACTS, "showContacts"},
	{IstiUserInput::eSMRK_SHOW_MISSED, "showMissed"},
	{IstiUserInput::eSMRK_SHOW_MAIL, "showMail"},
	{IstiUserInput::eSMRK_FAR_FLASH, "farFlash"},
	{IstiUserInput::eSMRK_MENU, "menu"},
	{IstiUserInput::eSMRK_HOME, "home"},
	{IstiUserInput::eSMRK_BACK_CANCEL, "back/cancel"},
	{IstiUserInput::eSMRK_UP, "up"},
	{IstiUserInput::eSMRK_LEFT, "left"},
	{IstiUserInput::eSMRK_ENTER, "enter"},
	{IstiUserInput::eSMRK_RIGHT, "right"},
	{IstiUserInput::eSMRK_DOWN, "down"},
	{IstiUserInput::eSMRK_DISPLAY, "display"},
	{IstiUserInput::eSMRK_STATUS, "status"},
	{IstiUserInput::eSMRK_KEYBOARD, "keyboard"},
	{IstiUserInput::eSMRK_ZERO, "zero"},
	{IstiUserInput::eSMRK_ONE, "one"},
	{IstiUserInput::eSMRK_TWO, "two"},
	{IstiUserInput::eSMRK_THREE, "three"},
	{IstiUserInput::eSMRK_FOUR, "four"},
	{IstiUserInput::eSMRK_FIVE, "five"},
	{IstiUserInput::eSMRK_SIX, "six"},
	{IstiUserInput::eSMRK_SEVEN, "seven"},
	{IstiUserInput::eSMRK_EIGHT, "eight"},
	{IstiUserInput::eSMRK_NINE, "nine"},
	{IstiUserInput::eSMRK_STAR, "star"},
	{IstiUserInput::eSMRK_POUND, "pound"},
	{IstiUserInput::eSMRK_DOT, "dot"},
	{IstiUserInput::eSMRK_TILT_UP, "tiltUp"},
	{IstiUserInput::eSMRK_PAN_LEFT, "panLeft"},
	{IstiUserInput::eSMRK_PAN_RIGHT, "panRight"},
	{IstiUserInput::eSMRK_TILT_DOWN, "tiltDown"},
	{IstiUserInput::eSMRK_NEAR, "near"},
	{IstiUserInput::eSMRK_FAR, "far"},
	{IstiUserInput::eSMRK_SPARE_A, "spareA"},
	{IstiUserInput::eSMRK_SPARE_B, "spareB"},
	{IstiUserInput::eSMRK_SPARE_C, "spareC"},
	{IstiUserInput::eSMRK_SPARE_D, "spareD"},
	{IstiUserInput::eSMRK_ZOOM_OUT, "zoomOut"},
	{IstiUserInput::eSMRK_ZOOM_IN, "zoomIn"},
	{IstiUserInput::eSMRK_SPEAKER, "speaker"},
	{IstiUserInput::eSMRK_CLEAR, "clear"},
	{IstiUserInput::eSMRK_VIEW_SELF, "viewSelf"},
	{IstiUserInput::eSMRK_VIEW_MODE, "viewMode"},
	{IstiUserInput::eSMRK_VIEW_POS, "viewPosition"},
	{IstiUserInput::eSMRK_AUDIO_PRIV, "audioPrivacy"},
	{IstiUserInput::eSMRK_DND, "doNotDisturb"},
	{IstiUserInput::eSMRK_HELP, "help"},
	{IstiUserInput::eSMRK_VIDEO_PRIV, "videoPrivacy"},
	{IstiUserInput::eSMRK_SVRS, "svrs"},
	{IstiUserInput::eSMRK_EXIT_UI, "exitUI"},
	{IstiUserInput::eSMRK_FOCUS, "focus"},
	{IstiUserInput::eSMRK_CALL, "call"},
	{IstiUserInput::eSMRK_HANGUP, "hangup"},
	{IstiUserInput::eSMRK_VIDEO_CENTER, "videoCenter"},
	{IstiUserInput::eSMRK_SATURATION, "saturation"},
	{IstiUserInput::eSMRK_BRIGHTNESS, "brightness"},
	{IstiUserInput::eSMRK_SETTINGS, "settings"},
	{IstiUserInput::eSMRK_CALL_HISTORY, "callHistory"},
	{IstiUserInput::eSMRK_FAVORITES, "favorites"},
	{IstiUserInput::eSMRK_CANCEL, "cancel"},
	{IstiUserInput::eSMRK_CUSTOMER_SERVICE_INFO, "customerServiceInfo"},
	{IstiUserInput::eSMRK_CALL_CUSTOMER_SERVICE, "callCustomerService"},
	{IstiUserInput::eSMRK_MISSED_CALLS, "missedCalls"},
	{IstiUserInput::eSMRK_SEARCH, "search"},
	{IstiUserInput::eSMRK_CAMERA_CONTROLS, "cameraControls"}
};

SocketListenerBase::SocketListenerBase (CstiMonitorTask *monitorTask)
:
	CstiEventQueue ("SocketListener"),
	m_focusTimeoutTimer (FOCUS_TIMEOUT, this),
	m_slicDetectTimeoutTimer (SLIC_DETECT_TIMEOUT, this),
	m_bluetoothScanTimer (BLUETOOTH_SCAN_TIME_DEFAULT, this),
	m_remoteInputTimeoutTimer (REMOTE_INPUT_TIMEOUT, this),
	m_tofTimeoutTimer (TOF_TIMEOUT, this),
	m_probeDataTimer (PROBE_DATA_PERIOD, this)
{
	m_commandMap.emplace ("/system/info", [this](const Poco::JSON::Object &root, int fd){systemInfo (root, fd);});
	m_commandMap.emplace ("/usb/info", [this](const Poco::JSON::Object &root, int fd){usbInfo (root, fd);});
	m_commandMap.emplace ("/testMode", [this](const Poco::JSON::Object &root, int fd){testModeSet (root, fd);});
	m_commandMap.emplace ("/audio/in", [this](const Poco::JSON::Object &root, int fd){audioIn (root, fd);});
	m_commandMap.emplace ("/audio/out", [this](const Poco::JSON::Object &root, int fd){audioOut (root, fd);});
	m_commandMap.emplace ("/headphones/detect", [this](const Poco::JSON::Object &root, int fd){headphonesConnected (root, fd);});
	m_commandMap.emplace ("/microphone/detect", [this](const Poco::JSON::Object &root, int fd){microphoneConnected (root, fd);});
	m_commandMap.emplace ("/focus", [this](const Poco::JSON::Object &root, int fd){singleShotFocus (root, fd);});
	m_commandMap.emplace ("/contrast", [this](const Poco::JSON::Object &root, int fd){contrastGet (root, fd);});
	m_commandMap.emplace ("/contrast/start", [this](const Poco::JSON::Object &root, int fd){contrastLoopStart (root, fd);});
	m_commandMap.emplace ("/contrast/stop", [this](const Poco::JSON::Object &root, int fd){contrastLoopStop (root, fd);});
	m_commandMap.emplace ("/vcm/position", [this](const Poco::JSON::Object &root, int fd){vcmPosition (root, fd);});
	m_commandMap.emplace ("/vcm/pattern", [this](const Poco::JSON::Object &root, int fd){vcmPattern (root, fd);});
	m_commandMap.emplace ("/lightring", [this](const Poco::JSON::Object &root, int fd){lightRing (root, fd);});
	m_commandMap.emplace ("/lightring/pattern", [this](const Poco::JSON::Object &root, int fd){lightRingPattern (root, fd);});
	m_commandMap.emplace ("/alertLed", [this](const Poco::JSON::Object &root, int fd){alertLed (root, fd);});
	m_commandMap.emplace ("/alertLed/pattern", [this](const Poco::JSON::Object &root, int fd){alertLedPattern (root, fd);});
	m_commandMap.emplace ("/iconLed/signMail", [this](const Poco::JSON::Object &root, int fd){signMailLed (root, fd);});
	m_commandMap.emplace ("/iconLed/missedCall", [this](const Poco::JSON::Object &root, int fd){missedCallLed (root, fd);});
	m_commandMap.emplace ("/iconLed/pattern", [this](const Poco::JSON::Object &root, int fd){iconLedPattern (root, fd);});
	m_commandMap.emplace ("/statusLed", [this](const Poco::JSON::Object &root, int fd){statusLed (root, fd);});
	m_commandMap.emplace ("/statusLed/pattern", [this](const Poco::JSON::Object &root, int fd){statusLedPattern (root, fd);});
	m_commandMap.emplace ("/slic", [this](const Poco::JSON::Object &root, int fd){slicRing (root, fd);});
	m_commandMap.emplace ("/slic/detect", [this](const Poco::JSON::Object &root, int fd){slicDetect (root, fd);});
	m_commandMap.emplace ("/bluetooth/scan", [this](const Poco::JSON::Object &root, int fd){bluetoothScan (root, fd);});
	m_commandMap.emplace ("/wifi/accessPoints", [this](const Poco::JSON::Object &root, int fd){wifiAccessPointsGet (root, fd);});
	m_commandMap.emplace ("/buttonStatus", [this](const Poco::JSON::Object &root, int fd){buttonStatus (root, fd);});
	m_commandMap.emplace ("/remoteInput", [this](const Poco::JSON::Object &root, int fd){remoteInput (root, fd);});
	m_commandMap.emplace ("/timeOfFlight", [this](const Poco::JSON::Object &root, int fd){timeOfFlight (root, fd);});
	m_commandMap.emplace ("/eventQueueStatus", [this](const Poco::JSON::Object &root, int fd){eventQueueStatus (root, fd);});
	m_commandMap.emplace ("/debugTool", [this](const Poco::JSON::Object &root, int fd){debugTool (root, fd);});
	m_commandMap.emplace ("/pipelines", [this](const Poco::JSON::Object &root, int fd){pipelines (root, fd);});
	m_commandMap.emplace ("/pipelines/probe", [this](const Poco::JSON::Object &root, int fd){pipelinesProbe (root, fd);});
	m_commandMap.emplace ("/pipelines/probe-list", [this](const Poco::JSON::Object &root, int fd){pipelinesProbeList (root, fd);});
	m_commandMap.emplace ("/pipeline/properties", [this](const Poco::JSON::Object &root, int fd){pipelinesProperties (root, fd);});
	m_commandMap.emplace ("/pipeline/property-info", [this](const Poco::JSON::Object &root, int fd){pipelinesPropertyInfo (root, fd);});
	m_commandMap.emplace ("/pipeline", [this](const Poco::JSON::Object &root, int fd){pipeline (root, fd);});
	m_commandMap.emplace ("/serialNumber", [this](const Poco::JSON::Object &root, int fd){serialNumber (root, fd);});
	m_commandMap.emplace ("/hdcc", [this](const Poco::JSON::Object &root, int fd){hdcc (root, fd);});
	m_commandMap.emplace ("/fan/dutyCycle", [this](const Poco::JSON::Object &root, int fd){fanDutyCycle (root, fd);});

	if (monitorTask)
	{
		m_headphonesConnected = monitorTask->headphoneConnectedGet ();
		m_microphoneConnected = monitorTask->microphoneConnectedGet ();

		m_signalConnections.push_back (monitorTask->headphoneConnectedSignal.Connect (
			[this](bool connected)
			{
				PostEvent (
					[this, connected]
					{
						m_headphonesConnected = connected;
					});
			}
		));

		m_signalConnections.push_back (monitorTask->microphoneConnectedSignal.Connect (
			[this](bool connected)
			{
				PostEvent (
					[this, connected]
					{
						m_microphoneConnected = connected;
					});
			}
		));
	}

	m_signalConnections.push_back (IstiUserInput::InstanceGet ()->buttonStateSignalGet ().Connect (
			[this](int buttonCode, bool pressed)
				{
					PostEvent ([this, buttonCode, pressed]
						{
							eventButtonStateChange (buttonCode, pressed);
						});
				}));

	m_signalConnections.push_back (IstiUserInput::InstanceGet ()->inputSignalGet ().Connect (
			[this](int buttonCode)
				{
					PostEvent ([this, buttonCode]
						{
							eventRemoteInput (buttonCode);
						});
				}));

	m_signalConnections.push_back (dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->micMagnitudeSignal.Connect (
			[this](std::vector<float> &left, std::vector<float> &right)
				{
					PostEvent ([this, left, right]
						{
							m_leftMicMagnitudes = left;
							m_rightMicMagnitudes = right;

							m_lastMicUpdate = std::chrono::steady_clock::now ();
						});
				}));

	m_lastMicUpdate = std::chrono::steady_clock::now ();

	m_signalConnections.push_back (m_probeDataTimer.timeoutSignal.Connect (
			[this]{
				probeDataSend ();
				m_probeDataTimer.restart ();
			}));
}


SocketListenerBase::~SocketListenerBase ()
{
	CstiEventQueue::StopEventLoop ();

	if (m_commandSocketFd > -1)
	{
		FileDescriptorDetach (m_commandSocketFd);

		close (m_commandSocketFd);
		m_commandSocketFd = -1;
	}

	if (m_webServerSendFd > -1)
	{
		close (m_webServerSendFd);
		m_webServerSendFd = -1;
	}
	
	if (m_webServerSocketFd > -1)
	{
		FileDescriptorDetach (m_webServerSocketFd);

		close (m_webServerSocketFd);
		m_webServerSocketFd = -1;
	}

	// Shut down threads that have been spun up
	iconLedPatternStop ();
}


int SocketListenerBase::openDomainSocket (const std::string &path)
{
	auto socketFd = socket (AF_LOCAL, SOCK_STREAM, 0);
	stiASSERT (socketFd != -1);

	if (socketFd != -1)
	{
		struct sockaddr_un addr {};
		addr.sun_family = AF_UNIX;
		strcpy (addr.sun_path, path.c_str());

		unlink (path.c_str()); // In case socket file already exists

		if (0 == bind (socketFd, reinterpret_cast<sockaddr*>(&addr), sizeof (addr)))
		{
			chmod (path.c_str(), 0666);
			
			if (0 == listen (socketFd, 0))
			{
				return socketFd;
			}
		}

		close (socketFd);
		socketFd = -1;
	}

	return socketFd;
}


stiHResult SocketListenerBase::initialize ()
{
	m_commandSocketFd = openDomainSocket (SOCKET_PATH);
	stiASSERT (m_commandSocketFd != -1);

	if (m_commandSocketFd != -1)
	{
		if (!FileDescriptorAttach (m_commandSocketFd, [this]{eventReadCommandSocket ();}))
		{
			stiASSERTMSG(estiFALSE, "%s: Can't attach socketFd %d\n", __func__, m_commandSocketFd);
		}
	}

	m_webServerSocketFd = openDomainSocket (WEBSERVER_SOCKET_PATH);
	stiASSERT (m_webServerSocketFd != -1);

	if (m_webServerSocketFd != -1)
	{
		if (!FileDescriptorAttach (m_webServerSocketFd, [this]{eventReadWebServerSocket ();}))
		{
			stiASSERTMSG(estiFALSE, "%s: Can't attach socketFd %d\n", __func__, m_webServerSocketFd);
		}
	}

	return stiRESULT_SUCCESS;	
}


stiHResult SocketListenerBase::startup ()
{
	CstiEventQueue::StartEventLoop ();

	return stiRESULT_SUCCESS;
}


void commandResponse (const Poco::JSON::Object &root, int responseFd)
{
	std::stringstream response;
	root.stringify(response);

	stiOSWrite (responseFd, response.str().c_str(), response.str().length());

	close (responseFd);
}


void SocketListenerBase::eventReadCommandSocket ()
{
	auto fd = accept (m_commandSocketFd, nullptr, nullptr);
	stiASSERT (fd != -1);

	if (fd != -1)
	{
		char command[MAX_COMMAND_SIZE] {};
		auto bytesRead = stiOSRead (fd, command, sizeof (command) - 1);

		if (bytesRead > 0)
		{
			try
			{
				Poco::JSON::Parser parser;
				auto root = parser.parse(command).extract<Poco::JSON::Object::Ptr>();

				commandProcess (*root, fd);
			}
			catch(Poco::Exception& ex)
			{
				JsonTestResponse response {std::string {command}};
				response.successfulSet (false);
				response.errorMessageSet (ex.message());

				commandResponse (response, fd);
			}

		}
	}	
}


void SocketListenerBase::eventReadWebServerSocket ()
{
	if (m_webServerSendFd > -1)
	{
		// Close the current fd. The web server must have destroyed the previous connection.
		// It will only attempt one connection at a time.
		close (m_webServerSendFd);
		m_webServerSendFd = -1;
	}

	m_webServerSendFd = accept (m_webServerSocketFd, nullptr, nullptr);
	stiASSERT (m_webServerSendFd != -1);
}


void SocketListenerBase::commandProcess (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (false);

	auto command = root.getValue<std::string>("command");
	if (!command.empty())
	{
		auto itr = m_commandMap.find (command);
		if (itr != m_commandMap.end ())
		{
			itr->second (root, responseFd);
			return;
		}

		response.errorMessageSet ("command not recognized");
	}
	else
	{
		response.errorMessageSet ("command not provided or not of string type");
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::dashboardDataSend (const Poco::JSON::Object &root)
{
	if (m_webServerSendFd > -1)
	{
		std::stringstream sendStream;
		root.stringify (sendStream);
		int32_t length = sendStream.str().size();

		if (-1 == stiOSWrite (m_webServerSendFd, &length, sizeof(length))
			|| -1 == stiOSWrite (m_webServerSendFd, sendStream.str().c_str(), sendStream.str().length()))
		{
			// Failed to write to socket.
			// Close the socket because this is likely due to the client disconnecting.
			close (m_webServerSendFd);
			m_webServerSendFd = -1;

			dashboardDataCollectionStop ();
		}
	}
}


void SocketListenerBase::dashboardDataCollectionStop ()
{
	removePipelineProbes ();
}


void SocketListenerBase::probeDataSend ()
{
	auto now = std::chrono::steady_clock::now ();

	Poco::JSON::Array probeArray;

	for (auto &probe : m_bufferProbes)
	{
		Poco::JSON::Object probeData;
		probeData.set ("path", probe.first.first);
		probeData.set ("type", probe.first.second);

		Poco::JSON::Array probeDataArray;
		Poco::JSON::Object numBuffers;
		numBuffers.set ("name", "Number of Buffers");
		numBuffers.set ("value", probe.second.totalCount);
		probeDataArray.add (numBuffers);

		Poco::JSON::Object fps;
		fps.set ("name", "fps");
		fps.set ("value",
				 static_cast<double>(probe.second.totalCount - probe.second.lastCheckedCount) * USEC_PER_SEC /
				 std::chrono::duration_cast<std::chrono::microseconds>(now - probe.second.lastCheckedTime).count ());
		probeDataArray.add (fps);

		Poco::JSON::Object fpsAverage;
		fpsAverage.set ("name", "Average fps");
		fpsAverage.set ("value",
						static_cast<double>(probe.second.totalCount) * USEC_PER_SEC /
						std::chrono::duration_cast<std::chrono::microseconds>(now - probe.second.probeAddedTime).count ());
		probeDataArray.add (fpsAverage);

		probeData.set ("data", probeDataArray);

		probeArray.add (probeData);

		probe.second.lastCheckedCount = probe.second.totalCount;
		probe.second.lastCheckedTime = now;
	}

	if (probeArray.size ())
	{
		Poco::JSON::Object probeArrayObject;
		probeArrayObject.set ("probes", probeArray);

		dashboardDataSend (probeArrayObject);
	}
}


void SocketListenerBase::testModeSet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	
	Poco::Dynamic::Var on = root.get("on");
	if (on.isBoolean())
	{
		m_testMode = on.extract<bool>();

		if (m_testMode)
		{
			IstiBluetooth::InstanceGet ()->Enable (true);
		}

		testModeSetSignal.Emit (m_testMode);

		response.successfulSet (true);
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::systemInfo (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (true);

	auto snFileRead = [](const std::string &fileName)->std::string
						{
							std::ifstream ifs {fileName, std::ifstream::in};

							if (ifs.is_open ())
							{
								std::string serialNumber {std::istreambuf_iterator<char>(ifs),
														  std::istreambuf_iterator<char>()};

								Trim (&serialNumber);

								return serialNumber;
							}
							
							return "Unknown";
						};

	response.set ("boardSerialNumber", dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->serialNumberGet ());	
	response.set("chassisSerialNumber", snFileRead ("/sys/class/dmi/id/chassis_serial"));
	response.set("productSerialNumber", snFileRead ("/sys/class/dmi/id/product_serial"));
	response.set("ntouchVersionNumber", smiSW_VERSION_NUMBER);

	commandResponse (response, responseFd);
}


void SocketListenerBase::usbInfo (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	
	auto *file = popen ("lsusb | cut -d ' ' -f 7-", "r");

	if (file)
	{
		response.successfulSet (true);
		
		char deviceName[128];
		Poco::JSON::Array deviceArray;

		while (fgets (deviceName, sizeof (deviceName), file))
		{
			std::string device {deviceName};
			Trim (&device);
			deviceArray.add (device);
		}

		pclose (file);

		response.set ("devices", deviceArray);
	}
	else
	{
		response.successfulSet (false);
		response.errorMessageSet ("lsusb command could not be run");
	}
	
	commandResponse (response, responseFd);
}


AudioInputType getAudioInput (const std::string &name)
{
	AudioInputType testInput = AudioInputType::Unknown;

	if (name == "microphones")
	{
		testInput = AudioInputType::Microphone;
	}
	else if (name == "lowRinger")
	{
		testInput = AudioInputType::LowPitchRinger;
	}
	else if (name == "mediumRinger")
	{
		testInput = AudioInputType::MediumPitchRinger;
	}
	else if (name == "highRinger")
	{
		testInput = AudioInputType::HighPitchRinger;
	}
	else if (name == "stereoTest")
	{
		testInput = AudioInputType::StereoTest;
	}
	else if (name == "toneTest")
	{
		testInput = AudioInputType::ToneTest;
	}

	return testInput;
}


void SocketListenerBase::audioIn (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto method = root.getValue<std::string>("method");
	if (method == "PUT")
	{
		Poco::Dynamic::Var on = root.get("on");
		if (on.isBoolean())
		{
			bool mode = on.extract<bool>();
			if (mode)
			{
				Poco::Dynamic::Var file = root.get("file");

				AudioInputType testInput = getAudioInput(file.extract<std::string>());

				if (testInput == AudioInputType::Unknown)
				{

				}
				else
				{
					AudioPipeline::OutputDevice outputDevice {AudioPipeline::OutputDevice::None};
					Poco::Dynamic::Var outputDev = root.get("outputDevice");
					if (outputDev.isString ())
					{
						auto device = outputDev.extract<std::string>();
						if (device == "speaker")
						{
							outputDevice = AudioPipeline::OutputDevice::Speaker;
						}
						else if (device == "headset")
						{
							outputDevice = AudioPipeline::OutputDevice::Headset;
						}
						else if (device == "hdmi")
						{
							outputDevice = AudioPipeline::OutputDevice::Default;
						}
					}

					int bands = 2;

					if (root.get("bands").isInteger())
					{
						bands = root.getValue<int>("bands");
					}

					IAudioTest::InstanceGet ()->testAudioInputStart (testInput, outputDevice, bands);
				}
			}
			else
			{
				IAudioTest::InstanceGet ()->testAudioInputStop ();
			}

			response.successfulSet (true);
		}
		else
		{
			response.invalidParametersSet ();
		}
	}
	else if (method == "GET")
	{
		if (std::chrono::steady_clock::now () - m_lastMicUpdate < std::chrono::milliseconds (MIC_DATA_EXPIRATION))
		{	
			Poco::JSON::Array micArray;
			for (auto &magnitude : m_leftMicMagnitudes)
			{
				micArray.add(magnitude);
			}
			response.set("leftMic", micArray);

			micArray.clear();
			for (auto &magnitude : m_rightMicMagnitudes)
			{
				micArray.add(magnitude);
			}
			response.set("rightMic", micArray);

			response.successfulSet (true);
		}
		else
		{
			response.successfulSet (false);
			response.errorMessageSet ("mic data is not available");
		}
		
	}
	else
	{
		response.successfulSet (false);
		response.errorMessageSet ("http method is not specified as either GET or PUT");
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::audioOut (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var on = root.get("on");
	Poco::Dynamic::Var volume = root.get("volume");
	Poco::Dynamic::Var file = root.get("file");
	Poco::Dynamic::Var outputDev = root.get("outputDevice");

	if ((on.isBoolean () && !on.extract<bool>()) ||
		(volume.isInteger () && root.getValue<int>("volume") == 0))
	{
		IAudioTest::InstanceGet ()->testAudioOutputStop ();
		response.successfulSet (true);
	}
	else if (file.isString () && volume.isInteger () && outputDev.isString ())
	{
		bool invalidParameters {false};

		std::string fileStr {file.extract<std::string>()};
		int vol {root.getValue<int>("volume")};
		std::string device {outputDev.extract<std::string>()};

		AudioInputType testInput = getAudioInput(fileStr);

		if (testInput == AudioInputType::Unknown)
		{
			invalidParameters = true;
			response.errorMessageSet ("Invalid parameters. File is not valid.");
		}

		if (vol < 0 || vol > 10)
		{
			invalidParameters = true;
			response.errorMessageSet ("Invalid parameters. Volume must be 0-10");
		}

		AudioPipeline::OutputDevice testDevice;
		if (device == "speaker")
		{
			testDevice = AudioPipeline::OutputDevice::Speaker;
		}
		else if (device == "headset")
		{
			testDevice = AudioPipeline::OutputDevice::Headset;
		}
		else if (device == "hdmi")
		{
			testDevice = AudioPipeline::OutputDevice::Default;
		}
		else
		{
			invalidParameters = true;
			response.errorMessageSet ("Invalid parameters. Device is not valid");
		}
		

		if (!invalidParameters)
		{
			IAudioTest::InstanceGet ()->testAudioOutputStart (testInput, volume, testDevice);
			response.successfulSet (true);
		}
	}

	if (response.successfulIsEmpty ())
	{
		response.successfulSet (false);
		if (response.errorMessageIsEmpty ())
		{
			response.invalidParametersSet ();
		}
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::headphonesConnected (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (true);
	response.set ("connected", m_headphonesConnected);

	commandResponse (response, responseFd);
}


void SocketListenerBase::microphoneConnected (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (true);
	response.set ("connected", m_microphoneConnected);

	commandResponse (response, responseFd);
}


void SocketListenerBase::singleShotFocus (const Poco::JSON::Object &root, int responseFd)
{
	if (!m_focusTimeoutTimer.isActive () && !m_contrastLoopRunning)
	{
		m_focusTimeoutConnection.Disconnect ();

		m_focusFd = responseFd;

		m_focusCompleteSignalConnection = IstiVideoInput::InstanceGet ()->focusCompleteSignalGet ().Connect (
			[this, root](int contrast){
				PostEvent ([this, root, contrast]{
					eventSingleShotFocus (root, contrast);
				});
			});

		m_focusTimeoutConnection = m_focusTimeoutTimer.timeoutSignal.Connect (
			[this, root]{
				eventSingleShotFocusTimeout (root);
			});
	
		dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet ())->SingleFocus ();

		m_focusTimeoutTimer.restart ();
	}
	else
	{
		JsonTestResponse response {root};
		response.successfulSet (false);
		if (m_contrastLoopRunning)
		{
			response.errorMessageSet ("cannot focus while contrast loop is running");
		}
		else
		{
			response.errorMessageSet ("single shot focus is currently running");
		}
		

		commandResponse (response, responseFd);
	}
}


void SocketListenerBase::eventSingleShotFocus (const Poco::JSON::Object &root, int contrast)
{
	m_focusTimeoutTimer.stop ();
	m_focusCompleteSignalConnection.Disconnect ();

	if (m_focusFd != -1)
	{
		JsonTestResponse response {root};

		if (contrast == -1)
		{
			// This represents TOF focus only
			response.successfulSet (true);
		}
		else if (contrast > 0)
		{
			// If contrast is greater than 0, assume it's good
			response.successfulSet (true);
			response.set("contrast", contrast);
		}
		else
		{
			// Either focus failed or Intel errantly returned 0
			response.successfulSet (false);
			response.errorMessageSet ("focus failed");
		}
		
		commandResponse (response, m_focusFd);

		m_focusFd = -1;
	}
}


void SocketListenerBase::eventSingleShotFocusTimeout (const Poco::JSON::Object &root)
{
	// If a timeout is received, don't listen for the focus complete signal anymore.
	m_focusCompleteSignalConnection.Disconnect ();
	
	if (m_focusFd != -1)
	{
		JsonTestResponse response {root};
		response.successfulSet (false);
		response.errorMessageSet ("timed out with no response");

		commandResponse (response, m_focusFd);

		m_focusFd = -1;
	}
}


void SocketListenerBase::contrastGet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	if (m_contrastLoopRunning && m_contrast > 0)
	{
		response.successfulSet (true);
		response.set ("contrast", m_contrast);
	}
	else
	{
		response.successfulSet (false);
		response.errorMessageSet ("contrast loop is not running");
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::contrastLoopStart (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	
	auto hResult = dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet ())->contrastLoopStart ([this](int contrast){contrastCallback (contrast);});

	if (!stiIS_ERROR (hResult))
	{
		m_contrastLoopRunning = true;
		response.successfulSet (true);
	}
	else
	{
		response.successfulSet (false);
		
		if (m_contrastLoopRunning)
		{
			response.errorMessageSet ("contrast loop is already running");
		}
		else
		{
			response.errorMessageSet ("cannot start contrast loop while focus is in process");
		}
		
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::contrastLoopStop (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	
	auto hResult = dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet ())->contrastLoopStop ();

	if (!stiIS_ERROR (hResult))
	{
		response.successfulSet (true);
	}
	else
	{
		response.successfulSet (false);
		response.errorMessageSet ("contrast loop is not running");
	}

	m_contrast = 0;
	m_contrastLoopRunning = false;
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::contrastCallback (int contrast)
{
	m_contrast = contrast;
}

void SocketListenerBase::vcmPosition (const Poco::JSON::Object &root, int responseFd)
{
	auto method = root.getValue<std::string>("method");
	if (method == "PUT")
	{
		vcmSet(root, responseFd);
	}
	else if (method == "GET")
	{
		vcmGet(root, responseFd);
	}
	else
	{
		JsonTestResponse response {root};
		response.invalidParametersSet ();

		commandResponse (response, responseFd);
	}
}

void SocketListenerBase::vcmSet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var pos = root.get("position");
	if (pos.isInteger ())
	{
		auto position = root.getValue<int>("position");

		PropertyRange focusRange {};

		auto videoInput = dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet ());
		videoInput->FocusRangeGet (&focusRange);

		if (position < focusRange.min || position > focusRange.max)
		{
			response.successfulSet (false);
			std::string errorMessage {"position must be "};
			errorMessage += std::to_string (focusRange.min) + '-' + std::to_string (focusRange.max);
			response.errorMessageSet (errorMessage);
		}
		else
		{
			videoInput->FocusPositionSet (position);
			response.successfulSet (true);
		}
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::vcmGet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto position = dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet ())->FocusPositionGet ();
	response.successfulSet (true);
	response.set ("position", position);

	commandResponse (response, responseFd);
}


void SocketListenerBase::lightRing (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var intensity = root.get("intensity");
	if (intensity.isInteger ())
	{
		Poco::Dynamic::Var red = root.get("red");
		Poco::Dynamic::Var green = root.get("green");
		Poco::Dynamic::Var blue = root.get("blue");

		auto value = root.getValue<int>("intensity");
		if (value == 0)
		{
			static_cast<ILightRingVP*>(IstiLightRing::InstanceGet ())->lightRingTurnOff ();
			response.successfulSet (true);
		}
		else if (red.isInteger () &&
				 green.isInteger () &&
				 blue.isInteger ())
		{
			auto redVal = root.getValue<int>("red");
			auto greenVal = root.getValue<int>("green");
			auto blueVal =  root.getValue<int>("blue");

			if (redVal	< 0 || redVal > 255 ||
				greenVal < 0 || greenVal > 255 ||
				blueVal < 0 || blueVal > 255 ||
				value < 0 || value > 255)
			{
				response.successfulSet (false);
				response.errorMessageSet ("All values, rgb and intensity, must be 0-255");
			}
			else
			{
				static_cast<ILightRingVP*>(IstiLightRing::InstanceGet ())->lightRingTurnOn (red, green, blue, intensity);
				response.successfulSet (true);
			}
		}
	}

	if (response.successfulIsEmpty ())
	{
		response.invalidParametersSet ();
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::lightRingPattern (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var on = root.get("on");
	if (on.isBoolean())
	{
		if (on.extract<bool>())
		{
			static_cast<ILightRingVP*>(IstiLightRing::InstanceGet ())->lightRingTestPattern (true);
		}
		else
		{
			static_cast<ILightRingVP*>(IstiLightRing::InstanceGet ())->lightRingTestPattern (false);
		}

		response.successfulSet (true);
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::alertLed (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var intensity = root.get("intensity");
	if (intensity.isInteger ())
	{
		auto value = root.getValue<int>("intensity");

		if (value == 0)
		{
			static_cast<ILightRingVP*>(IstiLightRing::InstanceGet ())->alertLedsTurnOff ();
			response.successfulSet (true);
		}
		else if (value < 0 || value > 1000)
		{
			response.successfulSet (false);
			response.errorMessageSet ("intensity must be 0-1000");
		}
		else
		{
			static_cast<ILightRingVP*>(IstiLightRing::InstanceGet ())->alertLedsTurnOn (intensity);
			response.successfulSet (true);
		}
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::alertLedPattern (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var intensity = root.get("intensity");
	if (intensity.isInteger ())
	{
		auto value = root.getValue<int>("intensity");

		if (value < 0 || value > 1000)
		{
			response.successfulSet (false);
			response.errorMessageSet ("intensity must be 0-1000");
		}
		else
		{
			static_cast<ILightRingVP*>(IstiLightRing::InstanceGet ())->alertLedsTestPattern (intensity);
			response.successfulSet (true);
		}
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::signMailLed (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var on = root.get("on");
	if (on.isBoolean())
	{
		if (on.extract<bool>())
		{
			IstiStatusLED::InstanceGet ()->Start (IstiStatusLED::estiLED_SIGNMAIL, 0);
		}
		else
		{
			IstiStatusLED::InstanceGet ()->Stop (IstiStatusLED::estiLED_SIGNMAIL);
		}

		response.successfulSet (true);
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::missedCallLed (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var on = root.get("on");
	if (on.isBoolean())
	{
		if (on.extract<bool>())
		{
			IstiStatusLED::InstanceGet ()->Start (IstiStatusLED::estiLED_MISSED_CALL, 0);
		}
		else
		{
			IstiStatusLED::InstanceGet ()->Stop (IstiStatusLED::estiLED_MISSED_CALL);
		}

		response.successfulSet (true);
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::statusLed (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	bool on {true};

	Poco::Dynamic::Var onVal = root.get("on");
	if (onVal.isBoolean())
	{
		on = onVal.extract<bool>();
	}

	if (on)
	{
		Poco::Dynamic::Var red = root.get("red");
		Poco::Dynamic::Var green  = root.get("green");
		Poco::Dynamic::Var blue = root.get("blue");

		if (red.isInteger() && green.isInteger() && blue.isInteger())
		{
			auto redVal = root.getValue<int>("red");
			auto greenVal = root.getValue<int>("green");
			auto blueVal =  root.getValue<int>("blue");

			if (redVal < 0	|| redVal > 255 ||
				greenVal < 0 || greenVal > 255 ||
				blueVal < 0 || blueVal > 255)
			{
				response.successfulSet (false);
				response.errorMessageSet ("RGB values must be 0-255");
			}
			else
			{
				auto leds = IstiStatusLED::InstanceGet ();
				leds->ColorSet (IstiStatusLED::estiLED_STATUS, red, green, blue);
				leds->Start (IstiStatusLED::estiLED_STATUS, 0);
				response.successfulSet (true);
			}
		}
		else
		{
			response.invalidParametersSet ();
		}
	}
	else
	{
		IstiStatusLED::InstanceGet ()->Stop (IstiStatusLED::estiLED_STATUS);
		response.successfulSet (true);
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::slicRing (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var on = root.get("on");
	if (on.isBoolean())
	{
		if (on.extract<bool>())
		{
			ISLICRinger::InstanceGet ()->start ();
		}
		else
		{
			ISLICRinger::InstanceGet ()->stop ();
		}

		response.successfulSet (true);
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::slicDetect (const Poco::JSON::Object &root, int responseFd)
{
	if (!m_slicDetectTimeoutTimer.isActive ())
	{
		m_slicDetectTimeoutConnection.Disconnect ();

		m_slicDetectFd = responseFd;
		
		m_slicDetectSignalConnection = ISLICRinger::InstanceGet()->deviceConnectedSignal.Connect (
			[this, root](bool connected, int mRen){
				PostEvent ([this, root, connected, mRen]{
					eventSlicDetect (root, connected, mRen);
				});
			});

		m_slicDetectTimeoutConnection = m_slicDetectTimeoutTimer.timeoutSignal.Connect (
			[this, root]{
				eventSlicDetectTimeout (root);
			});

		ISLICRinger::InstanceGet ()->deviceDetect ();

		m_slicDetectTimeoutTimer.restart ();
	}
	else
	{
		JsonTestResponse response {root};
		response.successfulSet (false);
		response.errorMessageSet ("slicDetect is currently running");

		commandResponse (response, responseFd);
	}
}


void SocketListenerBase::eventSlicDetect (const Poco::JSON::Object &root, bool connected, int mRen)
{
	m_slicDetectTimeoutTimer.stop ();
	m_slicDetectSignalConnection.Disconnect ();

	if (m_slicDetectFd != -1)
	{
		JsonTestResponse response {root};
		response.successfulSet (true);
		response.set ("connected", connected);
		response.set ("mRen", mRen);

		commandResponse (response, m_slicDetectFd);

		m_slicDetectFd = -1;
	}
}


void SocketListenerBase::eventSlicDetectTimeout (const Poco::JSON::Object &root)
{
	// If a timeout is received, don't listen for the slic detect signal anymore.
	m_slicDetectSignalConnection.Disconnect ();
	
	if (m_slicDetectFd != -1)
	{
		JsonTestResponse response {root};
		response.successfulSet (false);
		response.errorMessageSet ("timed out with no response");

		commandResponse (response, m_slicDetectFd);

		m_slicDetectFd = -1;
	}
}


void SocketListenerBase::bluetoothScan (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	
	if (m_bluetoothScanFd == -1)
	{
		Poco::Dynamic::Var seconds = root.get("seconds");
		if (seconds.isInteger())
		{
			auto secondsVal = root.getValue<int>("seconds");
			if (secondsVal == 0) // Get devices without scanning
			{
				std::list<BluetoothDevice> devices;
				dynamic_cast<IBluetoothVP*>(IstiBluetooth::InstanceGet ())->allDevicesListGet (&devices);

				if (devices.empty ())
				{
					response.successfulSet (false);
					response.errorMessageSet ("No devices are available without scanning.");
				}
				else
				{
					response.successfulSet (true);
					Poco::JSON::Array devicesArray;
					for (auto &device : devices)
					{
						devicesArray.add(device.name);
					}
					response.set("devices", devicesArray);
				}
			}
			else if (secondsVal < 0 || secondsVal > 15)
			{
				response.successfulSet (false);
				response.errorMessageSet ("scan seconds range is 0-15");
			}
			else
			{
				m_bluetoothScanTimer.timeoutSet (seconds * MSEC_PER_SEC);
			}
		}
		else
		{
			m_bluetoothScanTimer.timeoutSet (BLUETOOTH_SCAN_TIME_DEFAULT);
		}
	}
	else
	{
		response.successfulSet (false);
		response.errorMessageSet ("bluetooth scan is currently running");
	}


	if (!response.successfulIsEmpty ())
	{
		commandResponse (response, responseFd);
	}
	else
	{
		m_bluetoothScanFd = responseFd;

		m_bluetoothScanConnection = m_bluetoothScanTimer.timeoutSignal.Connect (
			[this, root]
			{
				PostEvent (
					[this, root]
					{
						eventBluetoothScanComplete (root);
					}
				);
			}
		);

		IstiBluetooth::InstanceGet ()->Scan ();

		m_bluetoothScanTimer.restart ();
	}
	
}

void SocketListenerBase::eventBluetoothScanComplete (const Poco::JSON::Object &root)
{
	if (m_bluetoothScanFd != -1)
	{
		if (m_bluetoothScanTimer.timeoutGet () < 15000)
		{
			IstiBluetooth::InstanceGet ()->ScanCancel ();
		}
		
		JsonTestResponse response {root};
		
		std::list<BluetoothDevice> devices;
		dynamic_cast<IBluetoothVP*>(IstiBluetooth::InstanceGet ())->allDevicesListGet (&devices);

		if (devices.empty ())
		{
			response.successfulSet (false);
			response.errorMessageSet ("No devices were discovered.");
		}
		else
		{
			response.successfulSet (true);
			Poco::JSON::Array devicesArray;
			for (auto &device : devices)
			{
				devicesArray.add(device.name);
			}
			response.set("devices", devicesArray);
		}
		
		commandResponse (response, m_bluetoothScanFd);

		m_bluetoothScanFd = -1;
	}

	m_bluetoothScanConnection.Disconnect ();
}


void SocketListenerBase::wifiAccessPointsGet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	WAPListInfo WAPInfo;
	IstiNetwork::InstanceGet ()->WAPListGet (&WAPInfo);

	if (WAPInfo.empty ())
	{
		response.successfulSet (false);
		response.errorMessageSet ("No acces points are available.");
	}
	else
	{
		response.successfulSet (true);

		Poco::JSON::Array accessPointArray;
		for (auto &wap : WAPInfo)
		{
			Poco::JSON::Object accessPoint;

			std::string ssid = wap.SSID;

			accessPoint.set ("ssid", ssid);
			accessPoint.set ("level", wap.SignalStrength);
			
			accessPointArray.add(accessPoint);
		}
		response.set("accessPoints", accessPointArray);
	}

	commandResponse (response, responseFd);
}

void SocketListenerBase::buttonStatus (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (true);

	Poco::JSON::Array buttonNameArray;
	for (auto &button : m_buttonMap)
	{
		if (button.second.pressed)
		{
			buttonNameArray.add(button.second.name);
		}
	}
	response.set("pressed", buttonNameArray);

	commandResponse (response, responseFd);
}


void SocketListenerBase::eventButtonStateChange (int buttonCode, bool pressed)
{
	auto itr = m_buttonMap.find (buttonCode);
	if (itr != m_buttonMap.end ())
	{
		itr->second.pressed = pressed;
	}
}


void SocketListenerBase::remoteInput (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
		
	m_remoteInputTimeoutConnection.Disconnect ();

	auto now = std::chrono::steady_clock::now ();

	Poco::JSON::Array buttonNameArray;

	for (auto &button : m_remoteButtons)
	{
		if (now - button.time <= std::chrono::milliseconds (REMOTE_INPUT_EXPIRATION))
		{
			buttonNameArray.add(button.name);
		}
	}

	m_remoteButtons.clear ();

	if (buttonNameArray.size ())
	{
		response.set("buttons", buttonNameArray);
		response.successfulSet (true);
		commandResponse (response, responseFd);
	}
	else
	{
		m_remoteInputCommand = root;
		m_remoteInputFd = responseFd;

		m_remoteInputTimeoutConnection = m_remoteInputTimeoutTimer.timeoutSignal.Connect (
			[this]
			{
				eventRemoteInputTimeout ();
			}
		);

		m_remoteInputTimeoutTimer.restart ();
	}
}


void SocketListenerBase::eventRemoteInput (int buttonCode)
{
	m_remoteInputTimeoutTimer.stop ();

	if (m_remoteInputFd != -1)
	{
		JsonTestResponse response (m_remoteInputCommand);
		response.successfulSet (true);

		Poco::JSON::Array buttonNameArray;
		buttonNameArray.add(buttonNameGet (buttonCode));
		response.set("buttons", buttonNameArray);
		
		commandResponse (response, m_remoteInputFd);

		m_remoteInputFd = -1;
	}
	else
	{
		auto now = std::chrono::steady_clock::now ();

		// Remove button presses that are too old
		for (auto it = m_remoteButtons.begin (); it != m_remoteButtons.end (); )
		{
			if (now - (*it).time > std::chrono::milliseconds (REMOTE_INPUT_EXPIRATION))
			{
				it = m_remoteButtons.erase(it);
			}
			else
			{
				++it;
			}
		}

		m_remoteButtons.push_back ({buttonNameGet (buttonCode), now});
	}
}


void SocketListenerBase::eventRemoteInputTimeout ()
{
	if (m_remoteInputFd != -1)
	{
		JsonTestResponse response (m_remoteInputCommand);
		response.successfulSet (false);
		response.errorMessageSet ("timed out with no remote input");

		commandResponse (response, m_remoteInputFd);

		m_remoteInputFd = -1;
	}
}


std::string SocketListenerBase::buttonNameGet (int buttonCode)
{
	auto itr = m_remoteButtonMap.find (buttonCode);
	if (itr != m_remoteButtonMap.end ())
	{
		return itr->second;
	}

	return std::string {"unknown"};
}


void SocketListenerBase::timeOfFlight (const Poco::JSON::Object &root, int responseFd)
{
	if (!m_tofTimeoutTimer.isActive ())
	{
		m_tofTimeoutConnection.Disconnect ();

		m_tofFd = responseFd;
	
		auto platform = dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ());

		m_tofTimeoutConnection = m_tofTimeoutTimer.timeoutSignal.Connect (
			[this, root]
			{
				eventTimeOfFlightTimeout (root);
			}
		);

		auto onSuccess {
			[this, root](int distance)
			{
				PostEvent (
					[this, root, distance]
					{
						eventTimeOfFlight (root, distance);
					}
				);
			}
		};
		
		platform->tofDistanceGet (onSuccess);

		m_tofTimeoutTimer.restart ();
	}
	else
	{
		JsonTestResponse response {root};
		response.successfulSet (false);
		response.errorMessageSet ("time of flight is already active");

		commandResponse (response, responseFd);
	}
}


void SocketListenerBase::eventTimeOfFlight (const Poco::JSON::Object &root, int distance)
{
	m_tofTimeoutTimer.stop ();

	if (m_tofFd != -1)
	{
		JsonTestResponse response {root};
		response.successfulSet (true);
		response.set("distance", distance);

		commandResponse (response, m_tofFd);

		m_tofFd = -1;
	}
}


void SocketListenerBase::eventTimeOfFlightTimeout (const Poco::JSON::Object &root)
{
	if (m_tofFd != -1)
	{
		JsonTestResponse response {root};
		response.successfulSet (false);
		response.errorMessageSet ("timed out with no response");

		commandResponse (response, m_tofFd);

		m_tofFd = -1;
	}
}


void SocketListenerBase::vcmPattern (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var on = root.get("on");
	if (on.isBoolean())
	{
		if (on.extract<bool>())
		{
			if (!m_runVcmPattern)
			{
				PropertyRange focusRange {};
				dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet ())->FocusRangeGet (&focusRange);

				if (focusRange.min != focusRange.max)
				{
					m_runVcmPattern = true;
					m_vcmPatternThread = std::thread ([this, focusRange]{vcmPatternTask (focusRange.min, focusRange.max);});
					response.successfulSet (true);
				}
				else
				{
					response.successfulSet (false);
					response.errorMessageSet ("vcm focus range is not available");
				}
			}
			else
			{
				response.successfulSet (false);
				response.errorMessageSet ("vcmPattern is already running");
			}
		}
		else
		{
			if (vcmPatternStop ())
			{
				response.successfulSet (true);
			}
			else
			{
				response.successfulSet (false);
				response.errorMessageSet ("vcmPattern is not running");
			}
		}
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


bool SocketListenerBase::vcmPatternStop ()
{
	if (m_runVcmPattern)
	{
		{
			// Scoping is necessary to properly shut down thread
			std::lock_guard<std::mutex> lock (m_vcmPatternMutex);
			m_runVcmPattern = false;
			m_vcmPatternCondVar.notify_one ();
		}

		if (m_vcmPatternThread.joinable ())
		{
			m_vcmPatternThread.join ();
		}

		return true;
	}

	return false;
}


void SocketListenerBase::vcmPatternTask (int minFocus, int maxFocus)
{
	const int NUMBER_OF_STEPS {5};

	uint16_t setting = minFocus;
	const int stepSize = (maxFocus - minFocus) / NUMBER_OF_STEPS;

	while (m_runVcmPattern)
	{
		std::unique_lock<std::mutex> lock (m_vcmPatternMutex);

		dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet ())->FocusPositionSet (setting);

		setting += stepSize;
		if (setting > maxFocus)
		{
			setting = minFocus;
		}

		auto waitUntil = std::chrono::system_clock::now() + std::chrono::milliseconds (1000);
		while (std::chrono::system_clock::now() < waitUntil && m_runVcmPattern)
		{
			m_vcmPatternCondVar.wait_until (lock, waitUntil);
		}

		if (!m_runVcmPattern)
		{
			dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet ())->FocusPositionSet (minFocus);
			break;
		}
	}
}


void SocketListenerBase::iconLedPattern (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var on = root.get("on");
	if (on.isBoolean())
	{
		if (on.extract<bool>())
		{
			if (!m_runIconLedPattern)
			{
				m_runIconLedPattern = true;
				m_iconLedPatternThread = std::thread ([this]{iconLedPatternTask ();});
				response.successfulSet (true);
			}
			else
			{
				response.successfulSet (false);
				response.errorMessageSet ("iconLedPattern is already running");
			}
		}
		else
		{
			if (iconLedPatternStop ())
			{
				response.successfulSet (true);
			}
			else
			{
				response.successfulSet (false);
				response.errorMessageSet ("iconLedPattern is not running");
			}
		}
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


bool SocketListenerBase::iconLedPatternStop ()
{
	if (m_runIconLedPattern)
	{
		{
			// Scoping is necessary to properly shut down thread
			std::lock_guard<std::mutex> lock (m_iconLedPatternMutex);
			m_runIconLedPattern = false;
			m_iconLedPatternCondVar.notify_one ();
		}

		if (m_iconLedPatternThread.joinable ())
		{
			m_iconLedPatternThread.join ();
		}

		return true;
	}

	return false;
}

void SocketListenerBase::iconLedPatternTask ()
{
	auto i = 0U;

	while (m_runIconLedPattern)
	{
		std::unique_lock<std::mutex> lock (m_iconLedPatternMutex);

		if (i++ % 2)
		{
			IstiStatusLED::InstanceGet ()->Start (IstiStatusLED::estiLED_MISSED_CALL, 0);
			IstiStatusLED::InstanceGet ()->Stop (IstiStatusLED::estiLED_SIGNMAIL);
		}
		else
		{
			IstiStatusLED::InstanceGet ()->Stop (IstiStatusLED::estiLED_MISSED_CALL);
			IstiStatusLED::InstanceGet ()->Start (IstiStatusLED::estiLED_SIGNMAIL, 0);
		}

		auto waitUntil = std::chrono::system_clock::now() + std::chrono::milliseconds (1000);
		while (std::chrono::system_clock::now() < waitUntil && m_runIconLedPattern)
		{
			m_iconLedPatternCondVar.wait_until (lock, waitUntil);
		}

		if (!m_runIconLedPattern)
		{
			IstiStatusLED::InstanceGet ()->Stop (IstiStatusLED::estiLED_MISSED_CALL);
			IstiStatusLED::InstanceGet ()->Stop (IstiStatusLED::estiLED_SIGNMAIL);
			break;
		}
	}
}


void SocketListenerBase::statusLedPattern (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var on = root.get("on");
	if (on.isBoolean())
	{
		if (on.extract<bool>())
		{
			if (!m_runStatusLedPattern)
			{
				m_runStatusLedPattern = true;
				m_statusLedPatternThread = std::thread ([this]{statusLedPatternTask ();});
				response.successfulSet (true);
			}
			else
			{
				response.successfulSet (false);
				response.errorMessageSet ("statusLedPattern is already running");
			}
		}
		else
		{
			if (statusLedPatternStop ())
			{
				response.successfulSet (true);
			}
			else
			{
				response.successfulSet (false);
				response.errorMessageSet ("statusLedPattern is not running");
			}
		}
	}
	else
	{
		response.invalidParametersSet ();
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::eventQueueStatus (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto stats = eventQueueStatsGet ();

	Poco::JSON::Array accumulatorsArray;

	for (auto &stat: stats)
	{
		Poco::JSON::Object accumulator;

		accumulator.set("name", stat.name);

		Poco::JSON::Object timeObject;
		for (auto &entry: stat.timeAccumulators)
		{
			timeObject.set(entry.first, entry.second.count());
		}
		accumulator.set("time", timeObject);

		Poco::JSON::Object countObject;
		for (auto &entry: stat.counters)
		{
			countObject.set(entry.first, entry.second);
		}
		accumulator.set("counts", countObject);

		accumulatorsArray.add(accumulator);
	}
	response.set("accumulators", accumulatorsArray);

	response.successfulSet (true);

	commandResponse (response, responseFd);
}


bool SocketListenerBase::statusLedPatternStop ()
{
	if (m_runStatusLedPattern)
	{
		{
			// Scoping is necessary to properly shut down thread
			std::lock_guard<std::mutex> lock (m_statusLedPatternMutex);
			m_runStatusLedPattern = false;
			m_statusLedPatternCondVar.notify_one ();
		}

		if (m_statusLedPatternThread.joinable ())
		{
			m_statusLedPatternThread.join ();
		}

		return true;
	}

	return false;
}

void SocketListenerBase::statusLedPatternTask ()
{
	auto i = 0U;

	while (m_runStatusLedPattern)
	{
		std::unique_lock<std::mutex> lock (m_statusLedPatternMutex);

		auto leds = IstiStatusLED::InstanceGet ();

		switch (i)
		{
			case 0:
			{
				leds->ColorSet (IstiStatusLED::estiLED_STATUS, 255, 0, 0);
				break;
			}
			case 1:
			{
				leds->ColorSet (IstiStatusLED::estiLED_STATUS, 0, 255, 0);
				break;
			}
			case 2:
			{
				leds->ColorSet (IstiStatusLED::estiLED_STATUS, 0, 0, 255);
				break;
			}
		}

		leds->Start (IstiStatusLED::estiLED_STATUS, 0);

		i++;
		if (i == 3)
		{
			i = 0;
		}

		auto waitUntil = std::chrono::system_clock::now() + std::chrono::milliseconds (1000);
		while (std::chrono::system_clock::now() < waitUntil && m_runStatusLedPattern)
		{
			m_statusLedPatternCondVar.wait_until (lock, waitUntil);
		}

		if (!m_runStatusLedPattern)
		{
			IstiStatusLED::InstanceGet ()->Stop (IstiStatusLED::estiLED_STATUS);
			break;
		}
	}
}


void SocketListenerBase::debugTool (const Poco::JSON::Object &root, int responseFd)
{
	auto method = root.getValue<std::string>("method");
	if (method == "PUT")
	{
		debugToolSet(root, responseFd);
	}
	else if (method == "GET")
	{
		debugToolGet(root, responseFd);
	}
	else
	{
		JsonTestResponse response {root};
		response.invalidParametersSet ();

		commandResponse (response, responseFd);
	}
}


void SocketListenerBase::debugToolGet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (true);

	Poco::JSON::Array toolsArray;

	auto list = DebugTools::instanceGet ()->debugToolListGet ();

	for (auto &tool : list)
	{
		Poco::JSON::Object entry;

		entry.set("name", tool->nameGet ());
		entry.set("value", tool->valueGet ());
		entry.set("defaultValue", tool->defaultValueGet ());
		entry.set("store", tool->persistGet ());

		toolsArray.add(entry);
	}

	response.set("tools", toolsArray);
	commandResponse (response, responseFd);
}


void SocketListenerBase::debugToolSet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	Poco::Dynamic::Var value = root.get("value");
	Poco::Dynamic::Var name = root.get("name");
	if (value.isInteger() && name.isString())
	{
		DebugTools::instanceGet ()->debugToolGet (name.extract<std::string>())->valueSet (root.getValue<int>("value"));
		response.successfulSet (true);
	}

	Poco::Dynamic::Var store = root.get("store");
	if (store.isBoolean() && name.isString())
	{
		DebugTools::instanceGet ()->debugToolGet (name.extract<std::string>())->persistSet (store.extract<bool>());
		response.successfulSet (true);
	}

	Poco::Dynamic::Var reset = root.get("reset");
	if (!reset.isEmpty() && name.isString())
	{
		DebugTools::instanceGet ()->debugToolGet (name.extract<std::string>())->valueReset();
		response.successfulSet (true);
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::pipelines (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto method = root.getValue<std::string>("method");
	if (method == "GET")
	{
		auto pipelines = GStreamerBin::binsGet ();

		Poco::JSON::Array pipelineArray;
		for (auto &entry: pipelines)
		{
			pipelineArray.add(entry);
		}
		response.set("pipelines", pipelineArray);
	}
	else
	{
		response.invalidParametersSet ();
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::pipelinesProbeList (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto method = root.getValue<std::string>("method");
	if (method == "GET")
	{
		Poco::JSON::Array probeList;

		Poco::DynamicStruct probe;

		probe["title"] = "Buffer Rate";
		probe["name"] = "bufferRate";

		probeList.add(probe);

		probe["title"] = "Drop Buffers";
		probe["name"] = "dropBuffers";

		probeList.add(probe);

		probe["title"] = "Events";
		probe["name"] = "events";

		probeList.add(probe);

		response.set("probeList", probeList);
	}
	else
	{
		response.invalidParametersSet ();
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::pipelinesProbe (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto method = root.getValue<std::string>("method");
	if (method == "POST")
	{
		if (root.get("path").isString()
			&& root.get("type").isString()
			&& root.get("enabled").isBoolean())
		{
			auto path = root.get("path").extract<std::string>();
			auto type = root.get("type").extract<std::string>();
			auto enabled = root.get("enabled").extract<bool>();

			auto pad = GStreamerBin::findPad(path);

			if (pad.get())
			{
				if (enabled)
				{
					if (type == "bufferRate")
					{
						auto itr = m_bufferProbes.find ({path,type});
						if (itr == m_bufferProbes.end ())
						{
							m_bufferProbes.emplace (std::make_pair (path, type), probeData());

							if (m_bufferProbes.size () == 1)
							{
								m_probeDataTimer.restart ();
							}
						}

						pad.addBufferProbe(
							type,
							[this, path, type]
							{
								PostEvent ([this, path, type] {
									eventBufferProbeCallback (path, type);
								});
								return true;
							}
						);
					}
					else if (type == "dropBuffers")
					{
						auto itr = m_bufferProbes.find ({path,type});
						if (itr == m_bufferProbes.end ())
						{
							m_bufferProbes.emplace (std::make_pair (path, type), probeData());

							if (m_bufferProbes.size () == 1)
							{
								m_probeDataTimer.restart ();
							}
						}

						pad.addBufferProbe(
							type,
							[this, path, type]
							{
								PostEvent ([this, path, type] {
									eventBufferProbeCallback (path, type);
								});
								return false;
							}
						);
					}
					else if (type == "events")
					{
						pad.addEventProbe(
							type,
							[this, path, type](const GStreamerEvent &event)
							{
								PostEvent ([this, path, type, event] {
									eventEventProbeCallback (path, type, event);
								});
								return true;
							}
						);
					}
				}
				else
				{
					pad.removeProbe(type);

					auto itr = m_bufferProbes.find ({path,type});
					if (itr != m_bufferProbes.end ())
					{
						m_bufferProbes.erase (itr);

						if (m_bufferProbes.empty ())
						{
							m_probeDataTimer.stop ();
						}
					}
				}
			}
		}
		else
		{
			response.invalidParametersSet ();
		}
	}
	else
	{
		response.invalidParametersSet ();
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::eventBufferProbeCallback (const std::string &path, const std::string &type)
{
	auto itr = m_bufferProbes.find ({path,type});
	if (itr != m_bufferProbes.end ())
	{
		itr->second.totalCount++;
	}
}


void SocketListenerBase::eventEventProbeCallback (
	const std::string &path,
	const std::string &type,
	const GStreamerEvent &event)
{
	try
	{
		Poco::JSON::Array probeArray;

		Poco::JSON::Object probeData;
		probeData.set ("path", path);
		probeData.set ("type", type);

		auto jsonObject = event.asJson();

		Poco::JSON::Array probeDataArray;

		Poco::JSON::Object data;
		data.set ("name", "event");
		data.set ("value", jsonObject);
		probeDataArray.add (data);

		probeData.set ("data", probeDataArray);

		probeArray.add (probeData);

		Poco::JSON::Object probeArrayObject;
		probeArrayObject.set ("probes", probeArray);

		dashboardDataSend(probeArrayObject);
	}
	catch (std::exception &ex)
	{
		stiASSERTMSG (false, "%s", ex.what ());
	}
}


void SocketListenerBase::removePipelineProbes ()
{
	m_probeDataTimer.stop ();
	
	for (auto itr = m_bufferProbes.begin (); itr != m_bufferProbes.end (); )
	{
		auto pad = GStreamerBin::findPad (itr->first.first); // "path"

		if (pad.get())
		{
			pad.removeProbe (itr->first.second); // "type"
		}

		itr = m_bufferProbes.erase (itr);
	}
}


void SocketListenerBase::pipelinesProperties (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto method = root.getValue<std::string>("method");
	if (method == "GET")
	{
		if (root.get("path").isString())
		{
			auto path = root.get("path").extract<std::string>();

			auto element = GStreamerBin::findElement(path);

			if (element.get())
			{
				Poco::JSON::Array properties;
				Poco::DynamicStruct enums;

				// Retrieve the properties
				element.foreachProperty(
					[&properties, &enums] (const GParamSpec &paramSpec, const GValue &propertyValue)
					{
						auto property = getPropertyAsJson (paramSpec, propertyValue);

						auto defaultGValue = g_param_spec_get_default_value(const_cast<GParamSpec *>(&paramSpec));

						if (defaultGValue) {
							std::string defaultValue;
							std::tie(std::ignore, defaultValue) = getTypeAndValueStrings (*defaultGValue);

							property["default"] = defaultValue;
						}

						if (G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(&propertyValue)) == G_TYPE_ENUM)
						{
							auto enumName = g_type_name(G_VALUE_TYPE(&propertyValue));

							Poco::DynamicStruct enumValues;

							auto enumObject = G_ENUM_CLASS( g_type_class_ref(paramSpec.value_type));

							for (guint i = 0; i < enumObject->n_values; i++)
							{
								enumValues[enumObject->values[i].value_nick] = enumObject->values[i].value;
							}

							enums[enumName] = enumValues;

							property["enumType"] = enumName;

							g_type_class_unref(enumObject);
						}

						properties.add (property);

						return true;
					},
					false);

				response.set("properties", properties);
				response.set("enums", enums);
			}
		}
	}
	else if (method == "PUT")
	{
		if (root.get("path").isString()
			&& root.get("properties").isArray())
		{
			auto path = root.get("path").extract<std::string>();

			auto element = GStreamerBin::findElement(path);

			if (element.get())
			{
				Poco::Dynamic::Array properties = *root.getArray("properties");

				std::vector<std::pair<std::string, std::string>> props;

				for (const auto &property: properties)
				{
					props.emplace_back(property["name"].toString(), property["value"].toString());
				}

				element.propertiesSet(props);
			}
		}
	}
	else
	{
		response.invalidParametersSet ();
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::pipelinesPropertyInfo (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto method = root.getValue<std::string>("method");
	if (method == "GET")
	{
		if (root.get("path").isString()
			&& root.get("property").isString())
		{
			auto path = root.get("path").extract<std::string>();
			auto propertyName = root.get("property").extract<std::string>();

			auto element = GStreamerBin::findElement(path);

			if (element.get())
			{
				auto propertyInfo = element.propertyInfoGet(propertyName);

				response.set("propertyInfo", propertyInfo);
			}
		}
	}
	else
	{
		response.invalidParametersSet ();
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::pipeline (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	auto method = root.getValue<std::string>("method");

	if (method == "GET")
	{
		Poco::Dynamic::Var name = root.get("name");

		if (name.isString())
		{
			auto bin = GStreamerBin::find (name.extract<std::string>());

			if (bin.get ())
			{
				auto interactiveMode = root.get("interactiveMode");

				if (interactiveMode.isBoolean () && interactiveMode.extract<bool>())
				{
					try
					{
						response.set("elements", bin.asJson ());
					}
					catch(std::exception& ex)
					{
						response.successfulSet (false);
						response.errorMessageSet (ex.what());
					}
				}
				else
				{
					auto graph = bin.getGraph ();

					response.set("content", graph);
				}
			}
		}
	}
	else
	{
		response.invalidParametersSet ();
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::serialNumber (const Poco::JSON::Object &root, int responseFd)
{
	auto method = root.getValue<std::string>("method");
	if (method == "GET")
	{
		serialNumberGet (root, responseFd);
	}
	else if (method == "PUT")
	{
		serialNumberSet (root, responseFd);
	}
	else
	{
		JsonTestResponse response {root};
		response.invalidParametersSet ();

		commandResponse (response, responseFd);
	}
}


void SocketListenerBase::serialNumberGet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (true);
	response.set ("serialNumber", dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->serialNumberGet ());	
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::serialNumberSet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	
	auto serialNumber = root.get ("serialNumber");
	if (serialNumber.isString ())
	{
		auto result = dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->serialNumberSet (serialNumber.extract<std::string>());
		if (result == stiRESULT_SUCCESS)
		{
			response.successfulSet (true);
		}
		else
		{
			response.errorMessageSet ("call to serialNumberSet() failed");
		}
	}

	if (response.successfulIsEmpty ())
	{
		response.successfulSet (false);
		if (response.errorMessageIsEmpty ())
		{
			response.invalidParametersSet ();
		}
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::hdcc (const Poco::JSON::Object &root, int responseFd)
{
	auto method = root.getValue<std::string>("method");
	if (method == "GET")
	{
		hdccGet (root, responseFd);
	}
	else if (method == "PUT")
	{
		hdccSet (root, responseFd);
	}
	else if (method == "DELETE")
	{
		hdccOverrideDelete (root, responseFd);
	}
	else
	{
		JsonTestResponse response {root};
		response.invalidParametersSet ();

		commandResponse (response, responseFd);
	}
}


void SocketListenerBase::hdccGet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (true);

	int hdcc {0};
	boost::optional<int> hdccOverride;
	std::tie (hdcc, hdccOverride) = dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->hdccGet ();

	response.set ("hdcc", hdcc);

	if (hdccOverride)
	{
		response.set ("hdccOverride", *hdccOverride);
	}
	
	commandResponse (response, responseFd);
}


void SocketListenerBase::hdccSet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	boost::optional<int> optHdcc;
	boost::optional<int> optHdccOverride;
	
	auto hdcc = root.get ("hdcc");
	if (hdcc.isInteger ())
	{
		optHdcc = root.getValue<int>("hdcc");
	}

	auto hdccOverride = root.get ("hdccOverride");
	if (hdccOverride.isInteger ())
	{
		optHdccOverride = root.getValue<int>("hdccOverride");
	}

	if (optHdcc || optHdccOverride)
	{	
		auto hdccResult {stiRESULT_SUCCESS};
		auto hdccOverrideResult {stiRESULT_SUCCESS};

		std::tie (hdccResult, hdccOverrideResult) = dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->hdccSet (optHdcc, optHdccOverride);

		if (hdccResult != stiRESULT_SUCCESS &&
			hdccOverrideResult != stiRESULT_SUCCESS)
		{
			response.successfulSet (false);
			response.errorMessageSet ("failed to set both hdcc and hdccOverride");
		}
		else if (hdccResult != stiRESULT_SUCCESS)
		{
			response.successfulSet (false);
			response.errorMessageSet ("failed to set hdcc");
		}
		else if (hdccOverrideResult != stiRESULT_SUCCESS)
		{
			response.successfulSet (false);
			response.errorMessageSet ("failed to set hdccOverride");
		}
		else
		{
			response.successfulSet (true);
		}	
	}

	if (response.successfulIsEmpty ())
	{
		response.successfulSet (false);
		if (response.errorMessageIsEmpty ())
		{
			response.invalidParametersSet ();
		}
	}

	commandResponse (response, responseFd);
}


void SocketListenerBase::hdccOverrideDelete (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};
	response.successfulSet (true);

	dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->hdccOverrideDelete ();

	commandResponse (response, responseFd);
}


void SocketListenerBase::fanDutyCycle (const Poco::JSON::Object &root, int responseFd)
{
	auto method = root.getValue<std::string>("method");
	if (method == "GET")
	{
		fanDutyCycleGet (root, responseFd);
	}
	else if (method == "PUT")
	{
		fanDutyCycleSet (root, responseFd);
	}
	else
	{
		JsonTestResponse response {root};
		response.invalidParametersSet ();

		commandResponse (response, responseFd);
	}
}


void SocketListenerBase::fanDutyCycleGet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	response.successfulSet (true);

	auto dutyCycle = IFanControl::InstanceGet ()->dutyCycleGet ();

	response.set ("dutyCycle", dutyCycle);

	commandResponse (response, responseFd);
}


void SocketListenerBase::fanDutyCycleSet (const Poco::JSON::Object &root, int responseFd)
{
	JsonTestResponse response {root};

	if (m_testMode)
	{
		if (root.get("dutyCycle").isInteger ())
		{
			auto dutyCycle = root.getValue<int>("dutyCycle");
			if (dutyCycle < 0 || dutyCycle > 100)
			{
				response.invalidParametersSet ();
			}
			else
			{
				response.successfulSet (true);
				IFanControl::InstanceGet ()->dutyCycleSet (dutyCycle);
			}
		}
		else
		{
			response.invalidParametersSet ();
		}
	}
	else
	{
		response.successfulSet (false);
		response.errorMessageSet ("must be in test mode to set fan duty cycle");
	}

	commandResponse (response, responseFd);
}
