// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2025 Sorenson Communications, Inc. -- All rights reserved
#pragma once

// Includes
//
#include "stiSVX.h"
#include "stiError.h"

#include "stiOSLinuxWd.h"
#include "CstiMonitorTask.h"
#include "CstiSignal.h"
#include "GStreamerPipeline.h"
#include "GStreamerSample.h"
#include <IstiAudibleRinger.h>
#include <IstiAudioOutput.h>
#include <IstiBluetooth.h>
#include <AudioControl.h>
#include "IAudioTest.h"
#include "IstiAudioInput.h"
#include "stiTools.h"
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <BluetoothAudio.h>
#include "TestAudioInputPipeline.h"
#include "AudioFileSourcePipeline.h"
#include "AudioPipelineOverrides.h"

#include <nonstd/observer_ptr.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <atomic>

#define DEFAULT_RINGER_VOLUME 3

#define Y_OR_N(x)				((x) ? "yes" : "no")

//
// Forward declarations
//
template<typename Platform>
class AudioFileSourcePipelinePlatform : public AudioFileSourcePipeline, public AudioPipelineOverrides<Platform>{};

template<typename Platform>
class TestAudioInputPipelinePlatform : public TestAudioInputPipeline, public AudioPipelineOverrides<Platform>{};


template <
	typename AudioRecordPipelineType,
	typename AudioPlaybackPipelineType,
	typename AudioFileSourcePipelineType,
	typename TestAudioInputPipelineType>
class CstiAudioHandlerBase : public CstiEventQueue
{
public:

	CstiAudioHandlerBase ()
	:
		CstiEventQueue ("stiAUDIO_HANDLER")
	{
	}

	~CstiAudioHandlerBase() override
	{
		StopEventLoop ();
	}

	CstiAudioHandlerBase (const CstiAudioHandlerBase &other) = delete;
	CstiAudioHandlerBase (CstiAudioHandlerBase &&other) = delete;
	CstiAudioHandlerBase &operator= (const CstiAudioHandlerBase &other) = delete;
	CstiAudioHandlerBase &operator= (CstiAudioHandlerBase &&other) = delete;

	/*!
	 * \brief Initialize
	 * Usually called once for object
	 *
	 * \return stiHResult
	 */

	stiHResult Initialize (
		EstiAudioCodec eAudioCodec,
		CstiMonitorTask *pMonitor,
		const nonstd::observer_ptr<vpe::BluetoothAudio> &bluetooth)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiTESTCOND (pMonitor != nullptr, stiRESULT_ERROR);

		m_pMonitor = pMonitor;
		m_bluetooth = bluetooth;

		hdmiAudioSupportedUpdate();

		// TODO: need to hold on to connection?
		if (m_bluetooth)
		{
			m_signalConnections.push_back(m_bluetooth->deviceUpdatedSignal.Connect (
				[this](const vpe::BluetoothAudioDevice &device)
				{
					BluetoothConnectedSet(device);
				}
			));
			m_bluetooth->currentDeviceBroadcast();
		}

		// Monitor the connection of headphones
		m_signalConnections.push_back(m_pMonitor->headphoneConnectedSignal.Connect (
			[this](bool connected)
			{
				PostEvent ([this, connected]{ eventHeadphoneConnected(connected); });
			}
		));

		// Monitor the connection of a microphone
		m_signalConnections.push_back(m_pMonitor->microphoneConnectedSignal.Connect([this](bool connected)
			{
				PostEvent ([this, connected] { eventMicrophoneConnectedSet (connected); });
			}));

		readyStateChangedSignal.Emit (false);

		stiOSStaticDataFolderGet (&m_DestDir);

		m_ringerType = AudioInputType::MediumPitchRinger;
		m_ePitch = IstiAudibleRinger::estiTONE_MEDIUM;

		m_playbackCodec = eAudioCodec;
		m_recordCodec = eAudioCodec;

	STI_BAIL:

		return hResult;
	}


	/*!
	 * \brief put an audio packet onto the audio pipeline
	 */
	stiHResult PacketPut (
		SsmdAudioFrame *pAudioFrame)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		Lock ();

		stiTESTCONDMSG (pAudioFrame->esmdAudioMode == m_playbackCodec, stiRESULT_INVALID_CODEC,
				"%d, %d", pAudioFrame->esmdAudioMode, m_playbackCodec);

		if (m_playbackPipeline.isPlaying ())
		{
			const int size {static_cast<int>(pAudioFrame->un8NumFrames * pAudioFrame->unFrameSizeInBytes)};

			GStreamerBuffer buffer (size);
			stiTESTCOND (buffer.get () != nullptr, stiRESULT_ERROR);

			hResult = buffer.dataCopy (pAudioFrame->pun8Buffer, size);
			stiTESTRESULT ();

			m_playbackPipeline.pushBuffer (buffer);
		}

	STI_BAIL:

		Unlock ();

		return (hResult);
	}


	stiHResult Startup ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		if(!CstiEventQueue::StartEventLoop ())
		{
			hResult = stiRESULT_ERROR;
		}

		return hResult;
	}


	stiHResult OutputEnable (
		bool bEnable)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::OutputEnable bEnable = %s\n", torf(bEnable));
		);

		PostEvent ([this, bEnable]
		{
			EventOutputEnable (bEnable);
		});

		return (hResult);
	}


	
	/*!
	 * \brief initiate the event that starts the audio pipeline
	 */
	void recordStart ()
	{
		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::RecordStart\n");
		);

		PostEvent ([this]{eventRecordStart ();});
	}


	/*!
	 * \brief initiate an event to stop the pipeline
	 */
	void recordStop ()
	{
		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::recordStop\n");
		);

		PostEvent ([this]{ eventRecordStop ();});
	}

	void recordCodecSet (EstiAudioCodec audioCodec)
	{
		PostEvent ([this, audioCodec] () { eventRecordCodecSet(audioCodec); });
	}

	void recordSettingsSet (
		const IstiAudioInput::SstiAudioRecordSettings &recordSettings)
	{
		PostEvent ([this, recordSettings] () { eventRecordSettingsSet(recordSettings); });
	}


	/*!
	 * \brief initiate the event that starts the audio pipeline
	 */
	stiHResult DecodeStart ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::DecodeStart\n");
		);

		PostEvent ([this]{eventPlaybackStart ();});

		return (hResult);
	}

	/*!
	 * \brief initiate an event to stop the pipeline
	 */
	stiHResult DecodeStop ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;


		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::DecodeStop\n");
		);

		PostEvent ([this]{eventPlaybackStop ();});

		return (hResult);
	}


	/*!
	 * \brief initiate the event to store decoder settings.
	 *
	 * \param SstiAudioDecodeSettings pAudioDecodeSettings
	 * Includes:
	 * 	channels	  number of channels in the stream.
	 *  sampleRate	  sample rate of the stream
	 *
	 *
	 * \return stiHResult
	 */
	stiHResult DecodeSettingsSet(
		SstiAudioDecodeSettings *pAudioDecodeSettings)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("Enter CstiAudioHandlerBase::DecodeSettingsSet\n");
		);

		PostEvent ([this, pAudioDecodeSettings]
		{
			EventDecodeSettingsSet (*pAudioDecodeSettings);
		});

		return hResult;
	}


	/*!
	 * \brief queue an event to start the ringer
	 */
	stiHResult RingerStart ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::RingerStart: enter\n");
		);

		PostEvent ([this]{EventRingerStart ();});

		return (hResult);
	}

	/*!
	 * \brief queue an event to stop the ringer
	 */
	stiHResult RingerStop ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::RingerStop\n");
		);

		PostEvent ([this]{EventRingerStop ();});

		return (hResult);
	}

	
	stiHResult CodecSet (
		EstiAudioCodec eAudioCodec)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::CodecSet\n");
		);

		PostEvent ([this, eAudioCodec]
		{
			EventCodecSet (eAudioCodec);
		});

		return (hResult);
	}
	

	/*!
	 * \brief set the ringer output volume
	 */
	stiHResult RingerVolumeSet (
		int nVolume)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		Lock ();

		if (nVolume < 0 || nVolume > 10)
		{
			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				stiTrace("RingerVolumeSet bad volume %d\n", nVolume);
			);
			stiTHROW (stiRESULT_INVALID_PARAMETER);
		}

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			vpe::trace ("RingerVolumeSet - current = ", m_nRingerVolume, ", new = ", nVolume, "\n");
		);

		if (nVolume != m_nRingerVolume)
		{
			m_nRingerVolume = nVolume;

			if (m_ringing)
			{
				m_ringerPipeline.volumeSet (m_nRingerVolume);
			}
		}

	STI_BAIL:

		Unlock ();

		return (hResult);
	}

	
	/*!
	 * \brief return the ringer output volume
	 */
	int RingerVolumeGet ()
	{
		int ringerVolume;

		Lock ();

		ringerVolume = m_nRingerVolume;

		Unlock ();
		
		return ringerVolume;
	}

	
	/*!
	 * \brief free up the current ringer file and read a new one
	 */
	stiHResult RingerPitchSet (
		IstiAudibleRinger::EstiTone ePitch)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::RingerPitchSet pitch = %d\n", ePitch);
		);

		PostEvent ([this, ePitch]
		{
			EventPitchSet (ePitch);
		});

		return (hResult);
	}


	/*!
	 * \brief return the enum describing the current pitch
	 */
	IstiAudibleRinger::EstiTone RingerPitchGet ()
	{
		IstiAudibleRinger::EstiTone eTone;

		Lock ();

		eTone = m_ePitch;

		Unlock ();

		return (eTone);
	}


	/*!
	 * \brief initiate the event to change the codec
	 *
	 * \param EstiAudioCodec eAudioCodec
	 * One of the below
	 *	estiAUDIO_NONE
	 *	estiAUDIO_G711_ALAW	  G.711 ALAW audio
	 *	estiAUDIO_G711_MULAW  G.711 MULAW audio
	 *	estiAUDIO_G722		  G.722 compressed audio
	 *	esmdAUDIO_G723_5	  G.723.5 compressed audio
	 *	esmdAUDIO_G723_6	  G.723.6 compressed audio
	 *  estiAUDIO_RAW		  Raw Audio (8000 hz, signed 16 bits, mono)
	 *  estiAUDIO_AAC,		  AAC audio used for playback and recording.
	 *
	 *  \see EstiAudioCodec
	 *
	 * \return stiHResult
	 */
	stiHResult BluetoothConnectedSet (
		const vpe::BluetoothAudioDevice &device)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::BluetoothConnectedSet: bluetoothAddress = %s\n", device.m_address.c_str());
		);

		PostEvent ([this, device]
		{
			EventBluetoothConnectedSet (device);
		});

		return (hResult);
	}


	/*
	 * force the headset audio to be used
	 */
	stiHResult headsetAudioForce(
		bool headsetAudioForce)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		PostEvent ([this, headsetAudioForce]
		{
			eventHeadsetAudioForce (headsetAudioForce);
		});

		return(hResult);
	}

	bool HeadphoneConnectedGet ()
	{
		bool headphoneConnected;

		Lock ();

		headphoneConnected = m_headphoneConnected;

		Unlock ();
		
		return headphoneConnected;
	};
	

	/*!
	 * \brief Gets the device file descriptor
	 *
	 * \return int
	 */
	int GetDeviceFD ()
	{
		return 0;
	}


	void testInputStart (
		AudioInputType input,
		AudioPipeline::OutputDevice outputDevice,
		int numberOfBands)
	{
		PostEvent ([this, input, outputDevice, numberOfBands]
			{
				m_testInputSettings.inputType = input;
				m_testInputSettings.outputDevice = outputDevice;
				m_testInputSettings.numberOfBands = numberOfBands;
				eventTestInputStart ();
			});
	}


	void testInputStop ()
	{
		PostEvent ([this]{eventTestInputStop ();});
	}


	void testOutputStart (AudioInputType file, int volume, AudioPipeline::OutputDevice outputDevice)
	{
		PostEvent ([this, file, volume, outputDevice]
		{
			eventTestOutputStart (file, volume, outputDevice);
		});
	}

	void testOutputStop ()
	{
		PostEvent ([this]{eventTestOutputStop ();});
	}

	CstiSignal<EstiAudioCodec, const GStreamerSample &> m_newRecordSampleSignal;
	CstiSignal<int> m_audioLevelSignal;
	CstiSignal<bool> readyStateChangedSignal;
	CstiSignal<std::vector<float>&, std::vector<float>&> micMagnitudeSignal;

protected:

	class CstiMonitorTask *m_pMonitor {nullptr};

	virtual bool doesEdidSupportHdmiAudio() { return true; }

	bool m_ringing {false};
	EstiAudioCodec m_playbackCodec {estiAUDIO_NONE};
	EstiAudioCodec m_recordCodec {estiAUDIO_NONE};

	AudioRecordPipelineType m_recordPipeline;
	AudioPlaybackPipelineType m_playbackPipeline;

	void eventHeadphoneConnected (bool headphoneConnected)
	{
		if (headphoneConnected != m_headphoneConnected)
		{
			m_headphoneConnected = headphoneConnected;

			EventOutputSinkUpdate ();
		}
	}

	void eventHeadsetAudioForce (bool headsetAudioForce)
	{
		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			vpe::trace (__func__, ": enter: current = ", m_headsetAudioForce, " setting to ", headsetAudioForce, "\n");
		);

		if (headsetAudioForce != m_headsetAudioForce)
		{
			m_headsetAudioForce = headsetAudioForce;

			EventOutputSinkUpdate ();
		}
	}

	void EventOutputSinkUpdate ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			vpe::trace(
					"CstiAudioHandlerBase::EventOutputSinkUpdate: bluetooth: ", m_bluetoothDevice.m_connected,
					", headphone: ", m_headphoneConnected,
					", forced: ", m_headsetAudioForce, "\n");
			);

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::EventOutputSinkUpdate - start\n");
		);

		if (!m_bluetoothDevice.m_connected && m_playbackPipeline.isPlaying ())
		{
			stiDEBUG_TOOL (g_stiAudioHandlerDebug,
				stiTrace("CstiAudioHandlerBase::EventOutputSinkUpdate - pipe is active, taking down...\n");
			);

			hResult = playbackPipelineStop ();
			stiTESTRESULT ();

			stiDEBUG_TOOL (g_stiAudioHandlerDebug,
				stiTrace("CstiAudioHandlerBase::EventOutputSinkUpdate - pipe was active, restoring....\n");
			);

			// playbackPipelineCreate() will destroy the current pipeline before creating a new one.
			hResult = playbackPipelineCreate();
			stiTESTRESULT ();

			hResult = playbackPipelineStart ();
			stiTESTRESULT ();
		}

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::EventOutputSinkUpdate - complete.\n");
		);

	STI_BAIL:

		stiUNUSED_ARG (hResult);
	}


	bool m_microphoneConnected {false};

	/*
	 * since all we were doing in the event handler for this, just collapse it down so that we don't
	 * need yet another event handler here.
	 */
	void eventMicrophoneConnectedSet (
		bool micConnected)
	{
		if (micConnected != m_microphoneConnected)
		{
			m_microphoneConnected = micConnected;
			
			if (m_recordPipeline.isPlaying ())
			{
				eventRecordStop ();
				eventRecordStart ();
			}

			if (m_testAudioInPipeline.isPlaying ())
			{
				eventTestInputStart ();
			}
		}
	}


	std::atomic<bool> m_headphoneConnected {false};

	vpe::BluetoothAudioDevice m_bluetoothDevice;
	nonstd::observer_ptr<vpe::BluetoothAudio> m_bluetooth;

	stiHResult recordPipelineCreate ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		m_recordPipeline.create (
				m_recordCodec,
				m_recordSettings,
				headsetUse (),
				m_bluetoothDevice, m_bluetooth);

		if (m_recordPipeline.get ())
		{
			m_recordPipelineSignalConnections.push_back(m_recordPipeline.m_newSampleSignal.Connect(
				[this](EstiAudioCodec codec, const GStreamerSample &sample)
				{
					m_newRecordSampleSignal.Emit (codec, sample);
				}
			));

			m_recordPipelineSignalConnections.push_back(m_recordPipeline.m_audioLevelSignal.Connect (
				[this](int level)
				{
					m_audioLevelSignal.Emit (level);
				}
			));
		}

		return hResult;
	}


	stiHResult playbackPipelineCreate ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		m_playbackPipeline.create (
				m_playbackCodec, m_decoderSettings,
				m_bluetoothDevice, m_bluetooth,
				headsetUse () ? AudioPipeline::OutputDevice::Headset : AudioPipeline::OutputDevice::Default,
				!m_hdmiAudioSupported);

		if (m_playbackPipeline.get ())
		{
			m_signalConnections.push_back(m_playbackPipeline.readyStateChangedSignal.Connect(
				[this](bool ready)
				{
					readyStateChangedSignal.Emit (ready);
				}
			));
#if 0
			m_signalConnections.push_back(m_playbackPipeline.pipelineErrorSignal.Connect (
				[this]
				{
					if (m_playbackPipeline.isPlaying ())
					{
						playbackPipelineStop();
						playbackPipelineStart ();
					}
				}));
#endif
			// If we have a record pipeline and the AEC configuration does
			// not match that of the playback pipeline then stop it,
			// destroy it, create and start it.
			if (m_recordPipeline.get () && m_playbackPipeline.aecEnabledGet () != m_recordPipeline.aecEnabledGet ())
			{
				eventRecordStop ();
				eventRecordStart ();
			}
		}

		return hResult;
	}

	SstiAudioDecodeSettings m_decoderSettings {};
	IstiAudioInput::SstiAudioRecordSettings m_recordSettings {};

protected:

	void hdmiAudioSupportedUpdate()
	{
		m_hdmiAudioSupported = doesEdidSupportHdmiAudio();

		if (m_playbackPipeline.isPlaying ())
		{
			playbackPipelineSinkReplace();
		}
	}

	bool headsetUse()
	{
		return(m_headphoneConnected || m_headsetAudioForce);
	}

	void eventRecordStart ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		hResult = recordPipelineCreate ();
		stiTESTRESULT();

		{
			auto ret = m_recordPipeline.stateSet (GST_STATE_PLAYING);
			stiTESTCOND (ret != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);
		}

	STI_BAIL:

		return;
	}

	void eventRecordStop ()
	{
		if (m_recordPipeline.isPlaying ())
		{
			auto ret = m_recordPipeline.stateSet (GST_STATE_NULL);
			stiASSERTMSG (ret != GST_STATE_CHANGE_FAILURE, "CstiAudioHandlerBase::eventRecordStop: GST_STATE_CHANGE_FAILURE: gst_element_set_state (m_pGstElementDecodePipeline, GST_STATE_NULL)\n");
		}

		m_recordPipeline.clear ();
		m_recordPipelineSignalConnections.clear ();
	}

	CstiSignalConnection::Collection m_signalConnections;
	CstiSignalConnection::Collection m_recordPipelineSignalConnections;
	CstiSignalConnection::Collection m_playbackPipelineSignalConnections;
	CstiSignalConnection::Collection m_testAudioInPipelineSignalConnections;

private:

	stiHResult playbackPipelineDestroy ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::playbackPipelineDestroy\n");
		);

		if (m_playbackPipeline.get ())
		{
			hResult = playbackPipelineStop ();
			stiTESTRESULT ();

			m_playbackPipeline.clear ();
			m_playbackPipelineSignalConnections.clear ();
		}

	STI_BAIL:

		return (hResult);
	}

	/*!
	 * \brief change the pipeline state to playing
	 */
	stiHResult playbackPipelineStart ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("enter CstiAudioHandlerBase::playbackPipelineStart\n");
		);

		stiTESTCOND (m_playbackPipeline.get (), stiRESULT_ERROR);

		{
			auto StateChangeReturn = m_playbackPipeline.stateSet (GST_STATE_PLAYING);

			if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiAudioHandlerBase::playbackPipelineStart: GST_STATE_CHANGE_FAILURE: gst_element_set_state (m_pGstElementDecodePipeline, GST_STATE_PLAYING)\n");
			}
		}

	STI_BAIL:

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("exit CstiAudioHandlerBase::playbackPipelineStart\n");
		);

		return (hResult);
	}

	/*!
	 * \brief change the pipeline state to paused
	 */
	stiHResult playbackPipelineStop ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("enter CstiAudioHandlerBase::playbackPipelineStop\n");
		);

		stiTESTCOND (m_playbackPipeline.get (), stiRESULT_ERROR);

		{
			auto StateChangeReturn = m_playbackPipeline.stateSet (GST_STATE_NULL);

			if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiAudioHandlerBase::playbackPipelineStop: GST_STATE_CHANGE_FAILURE: gst_element_set_state (m_pGstElementDecodePipeline, GST_STATE_NULL)\n");
			}
		}

	STI_BAIL:

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("exit CstiAudioHandlerBase::playbackPipelineStop\n");
		);

		return (hResult);
	}

	AudioFileSourcePipelineType m_ringerPipeline;
	TestAudioInputPipelineType m_testAudioInPipeline;
	AudioFileSourcePipelineType m_testAudioOutPipeline;

	stiHResult pipelineDestroy (GStreamerPipeline &pipeline)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::pipelineDestroy\n");
		);

		if (pipeline.get ())
		{
			GstStateChangeReturn StateChangeReturn;

			StateChangeReturn = pipeline.stateSet (GST_STATE_NULL);

			if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiAudioHandlerBase::pipelineDestroy: Could not set pipeline state to GST_STATE_NULL\n");
			}

			pipeline.clear ();
		}

	STI_BAIL:

		return hResult;
	}


	stiHResult pipelineStart (GStreamerPipeline &pipeline)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::pipelineStart\n");
		);

		stiTESTCOND (pipeline.get (), stiRESULT_ERROR);

		{
			auto StateChangeReturn = pipeline.stateSet (GST_STATE_PLAYING);

			if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiAudioHandlerBase::pipelineStart: Could not set pipeline state to GST_STATE_PLAYING)\n");
			}

			stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
				pipeline.writeGraph ("start_raw_file_source_pipeline");
			);
		}

	STI_BAIL:

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("exit CstiAudioHandlerBase::pipelineStart\n");
		);

		return hResult;
	}

	bool m_bOutputEnable {false};
	
	std::atomic<int> m_nRingerVolume {DEFAULT_RINGER_VOLUME};

	bool m_headsetAudioForce {false};

	bool m_hdmiPassthrough {false};

	bool m_hdmiAudioSupported {true};			// default this to true

	IstiAudibleRinger::EstiTone m_ePitch {IstiAudibleRinger::estiTONE_MEDIUM};

	std::string m_DestDir {};
	AudioInputType m_ringerType {AudioInputType::MediumPitchRinger};

	TestAudioInputSettings m_testInputSettings {};

	char const * StateNameGet (
		GstState state)
	{
		const char *ret = nullptr;
		switch (state)
		{
			case GST_STATE_NULL:
				ret = "GST_STATE_NULL";
				break;
			case GST_STATE_READY:
				ret = "GST_STATE_READY";
				break;
			case GST_STATE_PAUSED:
				ret = "GST_STATE_PAUSED";
				break;
			case GST_STATE_PLAYING:
				ret = "GST_STATE_PLAYING";
				break;
			case GST_STATE_VOID_PENDING:
				ret = "GST_STATE_VOID_PENDING";
				break;
		}

		return (ret);
	}

	
	char const * ReturnNameGet (
		GstStateChangeReturn type)
	{
		const char *ret = nullptr;

		switch (type)
		{
			case GST_STATE_CHANGE_FAILURE:
				ret = "GST_STATE_CHANGE_FAILURE";
				break;
			case GST_STATE_CHANGE_SUCCESS:
				ret = "GST_STATE_CHANGE_SUCCESS";
				break;
			case GST_STATE_CHANGE_ASYNC:
				ret = "GST_STATE_CHANGE_ASYNC";
				break;
			case GST_STATE_CHANGE_NO_PREROLL:
				ret = "GST_STATE_CHANGE_NO_PREROLL";
				break;
		}

		return (ret);
	}

	//
	// Event Handlers
	//

	/*!
	 * \brief handle the even to set the codec for audio playback
	 */
	void EventOutputEnable (bool outputEnable)
	{
		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::EventOutputEnable\n");
		);

		m_bOutputEnable = outputEnable;

		// TODO: We could do some things here to handle
		// AudioOutput being turned on or off durring
		// a call but currently this is not a use case
	}

	void eventRecordCodecSet (EstiAudioCodec audioCodec)
	{
		m_recordCodec = audioCodec;
	}

	void eventRecordSettingsSet (const IstiAudioInput::SstiAudioRecordSettings &settings)
	{
		m_recordSettings = settings;
	}

	/*!
	 * \brief process the event to start the audio pipeline
	 */
	void eventPlaybackStart ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::eventPlaybackStart, m_bOutputEnable = %s\n", torf(m_bOutputEnable));
		);
		stiTESTCOND (!m_playbackPipeline.isPlaying (), stiRESULT_ERROR);

		if (m_bOutputEnable)
		{
			hResult = playbackPipelineCreate ();
			stiTESTRESULT();

			hResult = playbackPipelineStart ();
			stiTESTRESULT();
		}

	STI_BAIL:

		return;
	}


	/*!
	 * \brief process the event to stop the pipeline
	 */
	void eventPlaybackStop ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::eventPlaybackStop\n");
		);

		hResult = playbackPipelineDestroy ();
		stiTESTRESULT ();

	STI_BAIL:
		stiUNUSED_ARG (hResult);
	}


	/*!
	 * \brief process the even to start the ringer
	 */
	void EventRingerStart ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		if (m_playbackPipeline.isPlaying ())
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiAudioHandlerBase::EventRingerStart: Trying to start ringer while not paused\n");
		}

		m_ringerPipeline.create (m_ringerType, m_nRingerVolume, AudioPipeline::OutputDevice::Speaker);
		stiTESTCOND (m_ringerPipeline.get (), stiRESULT_ERROR);

		hResult = pipelineStart (m_ringerPipeline);
		stiTESTRESULT();

		m_ringing = true;

		stiTESTCOND (m_bluetooth, stiRESULT_ERROR);
		hResult = m_bluetooth->RingIndicate (true);
		stiTESTRESULT();

	STI_BAIL:

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("exit CstiAudioHandlerBase::EventRingerStart\n");
		);
		stiUNUSED_ARG (hResult);
	}

	/*!
	 * \brief process the event to stop the ringer
	 */
	void EventRingerStop ()
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::EventRingerStop\n");
		);

		if (m_ringing)
		{
			hResult = pipelineDestroy (m_ringerPipeline);
			stiTESTRESULT();

			m_ringing = false;

			stiTESTCOND (m_bluetooth, stiRESULT_ERROR);
			hResult = m_bluetooth->RingIndicate (false);
			stiTESTRESULT();
		}

	STI_BAIL:

		stiUNUSED_ARG (hResult);
	}


	/*!
	 * \brief handle the event to set the codec for audio playback
	 */
	void EventCodecSet (EstiAudioCodec codec)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::EventCodecSet\n");
		);

		if (m_playbackPipeline.isPlaying ())
		{
			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				stiTrace("Setting codec when not paused\n");
			);

			stiTHROW (stiRESULT_ERROR);
		}

		m_playbackCodec = codec;

	STI_BAIL:
		stiUNUSED_ARG (hResult);
	}


	void EventPitchSet (IstiAudibleRinger::EstiTone pitch)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::EventPitchSet\n");
		);

		if (pitch != m_ePitch)
		{
			/*
			 * these file names still need to be finalized
			 */
			switch (pitch)
			{
				case IstiAudibleRinger::estiTONE_LOW:
					m_ringerType = AudioInputType::LowPitchRinger;
					break;
				case IstiAudibleRinger::estiTONE_MEDIUM:
					m_ringerType = AudioInputType::MediumPitchRinger;
					break;
				case IstiAudibleRinger::estiTONE_HIGH:
					m_ringerType = AudioInputType::HighPitchRinger;
					break;
				case IstiAudibleRinger::estiTONE_MAX:
					stiTHROW (stiRESULT_ERROR);
			}
			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				vpe::trace ("CstiAudioHandlerBase::EventPitchSet - ringer type: ", static_cast<int>(m_ringerType), "\n");
			);

			m_ePitch = pitch;
		}

	STI_BAIL:

		stiUNUSED_ARG (hResult);
	}

	stiHResult playbackPipelineSinkReplace()
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		hResult = playbackPipelineStop ();
		stiTESTRESULT ();

		// playbackPipelineCreate() will destroy the current pipeline before creating a new one.
		hResult = playbackPipelineCreate();
		stiTESTRESULT ();

		hResult = playbackPipelineStart ();
		stiTESTRESULT ();
STI_BAIL:
		return hResult;
	}

	void EventBluetoothConnectedSet (const vpe::BluetoothAudioDevice &device)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::%s: new connected: %d, old: %s, new: %s\n", __FUNCTION__, device.m_connected, m_bluetoothDevice.m_address.c_str(), device.m_address.c_str());
		);

		if (device.m_address == m_bluetoothDevice.m_address && !device.m_connected)
		{
			m_bluetoothDevice = {};
		}
		else
		{
			m_bluetoothDevice = device;
		}


		if (m_playbackPipeline.isPlaying ())
		{
			hResult = playbackPipelineSinkReplace();
			stiTESTRESULT ();
		}

		if (m_recordPipeline.isPlaying ())
		{
			eventRecordStop ();
			eventRecordStart ();
		}

	STI_BAIL:

		stiUNUSED_ARG(hResult);
	}

	/*!
	 * \brief handle the event to save the decoder settings for audio playback
	 */
	void EventDecodeSettingsSet (const SstiAudioDecodeSettings &settings)
	{
		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("CstiAudioHandlerBase::EventDecoderSettingsSet\n");
		);

		m_decoderSettings = settings;
	}


	void eventTestInputStart ()
	{
		stiHResult hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG(hResult);

		eventTestInputStop ();

		m_testAudioInPipeline.create (m_testInputSettings, m_microphoneConnected);
		stiTESTCOND (m_testAudioInPipeline.get (), stiRESULT_ERROR);

		m_testAudioInPipelineSignalConnections.push_back(m_testAudioInPipeline.micMagnitudeSignal.Connect (
			[this](std::vector<float> &left, std::vector<float> &right)
			{
				micMagnitudeSignal.Emit (left, right);
			}
		));

		pipelineStart (m_testAudioInPipeline);

	STI_BAIL:

		return;
	}


	void eventTestInputStop ()
	{
		pipelineDestroy (m_testAudioInPipeline);
		m_testAudioInPipelineSignalConnections.clear ();
	}

	void eventTestOutputStart (AudioInputType input, int volume, AudioPipeline::OutputDevice outputDevice)
	{
		stiHResult hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG (hResult);

		eventTestOutputStop ();

		m_testAudioOutPipeline.create(input, volume, outputDevice);
		stiTESTCOND (m_testAudioOutPipeline.get (), stiRESULT_ERROR);

		pipelineStart (m_testAudioOutPipeline);

	STI_BAIL:

		return;
	}

	void eventTestOutputStop ()
	{
		pipelineDestroy (m_testAudioOutPipeline);
	}
};
