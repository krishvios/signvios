// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAudioHandler.h"
#include "GStreamerCaps.h"
#include "GStreamerPad.h"
#include "stiTrace.h"
#include "AudioRecordPipelineVP2.h"
#include "AudioPlaybackPipelineVP2.h"

//#define HEADPHONE_JACK_GPIO     "/sys/class/gpio/gpio178/value"
//
//#define Y_OR_N(x)				((x) ? "yes" : "no")

//#define AUDIO_JACK_PATHNAME "/dev/input/by-path/platform-tegra-snd-wm8903.0-event"
//#define BITS_PER_LONG (sizeof(long) * 8)
//#define NBITS(x) ((((x) - 1) / BITS_PER_LONG) + 1)
//#define OFF(x)  ((x) % BITS_PER_LONG)
//#define BIT(x)  (1UL << OFF(x))
//#define LONG(x) ((x) / BITS_PER_LONG)
//#define stiTEST_BIT(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)

/*!
 * \brief Initialize
 * Usually called once for object
 *
 * \return stiHResult
 */
stiHResult CstiAudioHandler::Initialize (
	EstiAudioCodec eAudioCodec,
	CstiMonitorTask *pMonitor,
	const nonstd::observer_ptr<vpe::BluetoothAudio> &bluetooth)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiAudioHandlerBase::Initialize(eAudioCodec, pMonitor, bluetooth);

	// Todo: The connections to these signals need to be reviewed to determine if all
	// of these are really needed.
	m_signalConnections.push_back (pMonitor->usbRcuConnectionStatusChangedSignal.Connect (
		[this]()
		{
			PostEvent([this]{ EventOutputSinkUpdate ();});
		}));

	m_signalConnections.push_back (pMonitor->usbRcuResetSignal.Connect (
		[this]()
		{
			PostEvent([this]{ EventOutputSinkUpdate ();});
		}));

	m_signalConnections.push_back (pMonitor->mfddrvdStatusChangedSignal.Connect (
		[this]()
		{
			PostEvent([this]{ EventOutputSinkUpdate ();});
		}));

	m_signalConnections.push_back (pMonitor->hdmiInResolutionChangedSignal.Connect (
		[this](int resolution)
		{
			PostEvent([this]{ EventOutputSinkUpdate ();});
		}));

	m_signalConnections.push_back (pMonitor->hdmiInStatusChangedSignal.Connect (
		[this](int resolution)
		{
			PostEvent([this]{ EventOutputSinkUpdate ();});
		}));

	m_signalConnections.push_back (pMonitor->hdmiOutStatusChangedSignal.Connect (
		[this]()
		{
			PostEvent([this]{ EventOutputSinkUpdate ();});
		}));

	m_signalConnections.push_back (pMonitor->hdmiHotPlugStateChangedSignal.Connect (
		[this]()
		{
			PostEvent([this]{ EventOutputSinkUpdate ();});
		}));

	return hResult;
}
