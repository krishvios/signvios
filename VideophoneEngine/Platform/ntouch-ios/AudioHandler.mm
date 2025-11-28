/*!
 * \file AudioHandler.mm
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioHandler.h"
#include <AVFoundation/AVAudioSession.h>
#include <Foundation/NSNotification.h>
#include <algorithm>
#include "stiTools.h"

namespace vpe {


static const AudioUnitElement OUTPUT_BUS = 0;
static const AudioUnitElement INPUT_BUS = 1;
static const size_t MAX_RECORD_LATENCY_MILLISECONDS = 150;
static const size_t BUFFER_SIZE = (1024 * 1024 * 2);

AudioHandler::AudioHandler (CstiSignal<bool>& recordReadyStateChangedSignal, CstiSignal<bool>& playbackReadyStateChangedSignal)
	: m_recordReadyStateChangedSignal (recordReadyStateChangedSignal),
	  m_playbackReadyStateChangedSignal (playbackReadyStateChangedSignal),
	  playbackBuffer(BUFFER_SIZE),
	  recordBuffer(BUFFER_SIZE)
{
	m_interruptedToken = (__bridge void *)[[NSNotificationCenter defaultCenter]
		addObserverForName:AVAudioSessionInterruptionNotification
		object:[AVAudioSession sharedInstance]
		queue:nil
		usingBlock:^(NSNotification * _Nonnull note)
		{
			if (m_started && [note.userInfo[AVAudioSessionInterruptionTypeKey] isEqual:@(AVAudioSessionInterruptionTypeBegan)])
			{
				Stop ();
				m_interrupted = true;
			}
			else if (m_interrupted && [note.userInfo[AVAudioSessionInterruptionTypeKey] isEqual:@(AVAudioSessionInterruptionTypeEnded)])
			{
				if ([note.userInfo[AVAudioSessionInterruptionOptionKey] containsObject:@(AVAudioSessionInterruptionOptionShouldResume)])
				{
					Start ();
				}
				m_interrupted = false;
			}
		}];

	m_resetToken = (__bridge void *)[[NSNotificationCenter defaultCenter]
		addObserverForName:AVAudioSessionMediaServicesWereResetNotification
		object:[AVAudioSession sharedInstance]
		queue:nil
		usingBlock:^(NSNotification * _Nonnull note)
		{
			if (m_audioUnit != nullptr)
			{
				Uninitialize ();
				Initialize ();

				EstiAudioCodec playbackCodec = m_playbackCodec;
				m_playbackCodec = estiAUDIO_NONE;
				EstiAudioCodec recordCodec = m_recordCodec;
				m_recordCodec = estiAUDIO_NONE;
				
				PlaybackCodecSet (playbackCodec);
				RecordCodecSet (recordCodec);

				if (m_started)
				{
					Start ();
				}
			}
		}];
}


AudioHandler::~AudioHandler ()
{
	Uninitialize ();

	[[NSNotificationCenter defaultCenter] removeObserver:(__bridge id)m_interruptedToken];
	[[NSNotificationCenter defaultCenter] removeObserver:(__bridge id)m_resetToken];
}


stiHResult AudioHandler::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	{
		OSStatus status = 0;

		AudioComponentDescription desc;
		desc.componentType = kAudioUnitType_Output;
		desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;
		AudioComponent component = AudioComponentFindNext (nullptr, &desc);
		stiTESTCOND (component != nullptr, stiRESULT_DRIVER_ERROR);

		status = AudioComponentInstanceNew (component, &m_audioUnit);
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);

		AURenderCallbackStruct callback;
		callback.inputProcRefCon = (void *)this;

		callback.inputProc = AudioPlayback;
		status = AudioUnitSetProperty (
			m_audioUnit,
			kAudioUnitProperty_SetRenderCallback,
			kAudioUnitScope_Input,
			OUTPUT_BUS,
			&callback,
			sizeof (callback));
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);

		callback.inputProc = AudioRecord;
		status = AudioUnitSetProperty (
			m_audioUnit,
			kAudioOutputUnitProperty_SetInputCallback,
			kAudioUnitScope_Output,
			INPUT_BUS,
			&callback,
			sizeof (callback));
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);

		uint32_t trueValue = 1;
		status = AudioUnitSetProperty (
			m_audioUnit,
			kAudioOutputUnitProperty_EnableIO,
			kAudioUnitScope_Input,
			INPUT_BUS,
			&trueValue,
			sizeof (trueValue));
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
		
		// Output is enabled by default.
	}

STI_BAIL:
	return hResult;
}


stiHResult AudioHandler::Uninitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = 0;

	if (m_audioUnit)
	{
		if (m_initialized)
		{
			status = AudioUnitUninitialize (m_audioUnit);
			stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
			m_initialized = false;
		}

		status = AudioComponentInstanceDispose (m_audioUnit);
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
		m_audioUnit = nullptr;
	}

STI_BAIL:
	return stiRESULT_SUCCESS;
}


std::streamsize AudioHandler::RecordBytesAvailableGet ()
{
	return m_recordStream.tellp () - m_recordStream.tellg ();
}


std::streamsize AudioHandler::PlaybackBytesAvailableGet ()
{
	return m_playbackStream.tellp () - m_playbackStream.tellg ();
}


stiHResult AudioHandler::Start ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = 0;
	NSError *error = nil;

	m_playbackMutex.lock ();
	m_playbackStream.clear ();
	m_playbackMutex.unlock ();
	m_playbackReadyStateChangedSignal.Emit (true);
	playbackBuffer.clear();

	m_recordMutex.lock ();
	m_recordStream.clear ();
	m_recordMutex.unlock ();
	recordBuffer.clear();

	if (!m_initialized)
	{
		status = AudioUnitInitialize (m_audioUnit);
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
		m_initialized = true;
	}
	
	[AVAudioSession.sharedInstance setCategory:AVAudioSessionCategoryPlayAndRecord mode:AVAudioSessionModeVideoChat options:AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionAllowBluetooth error:&error];
	stiTESTCONDMSG (error == nil, stiRESULT_DRIVER_ERROR, "error=%s", [error.description UTF8String]);
	
	[AVAudioSession.sharedInstance setActive:true error:&error];
	stiTESTCONDMSG (error == nil, stiRESULT_DRIVER_ERROR, "error=%s", [error.description UTF8String]);

	status = AudioOutputUnitStart (m_audioUnit);
	stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);

	m_started = true;
	m_interrupted = false;

STI_BAIL:
	return hResult;
}


bool AudioHandler::IsRunning ()
{
	UInt32 output = 0;
	UInt32 size = sizeof(output);
	AudioUnitGetProperty(
		m_audioUnit,
		kAudioOutputUnitProperty_IsRunning,
		kAudioUnitScope_Global,
		0,
		&output,
		&size);
	
	return output != 0;
}

stiHResult AudioHandler::Stop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = 0;
	NSError *error = nil;

	status = AudioOutputUnitStop (m_audioUnit);
	stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
	
	[AVAudioSession.sharedInstance setActive:false error:&error];
	stiTESTCONDMSG (error == nil, stiRESULT_DRIVER_ERROR, "error=%s", [error.description UTF8String]);

	m_interrupted = false;
	m_started = false;

STI_BAIL:
	return hResult;
}


stiHResult AudioHandler::PlaybackEnable (bool enable)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (enable && !m_playbackEnabled)
	{
		std::lock_guard<std::mutex> lock (m_playbackMutex);
		m_playbackStream.clear ();
		m_playbackReadyStateChangedSignal.Emit (true);
	}

	m_playbackEnabled = enable;
	return hResult;
}


stiHResult AudioHandler::RecordEnable (bool enable)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (enable && !m_recordEnabled)
	{
		std::lock_guard<std::mutex> lock (m_recordMutex);
		m_recordStream.clear ();
	}

	m_recordEnabled = enable;
	return hResult;
}


stiHResult AudioHandler::CodecsGet (std::list<EstiAudioCodec> *codecs)
{
	if (codecs != nullptr)
	{
		codecs->push_back (estiAUDIO_G711_MULAW);

		// If we tell the engine we support both g.711 formats, interpreters hear audio that sounds like water -- It seems they don't handle a-law well?
		//codecs->push_back (estiAUDIO_G711_ALAW);
	}

	return stiRESULT_SUCCESS;
}


stiHResult AudioHandler::StreamDescriptionGet (
	EstiAudioCodec audioCodec,
	AudioStreamBasicDescription *streamDescription,
	size_t *minPacketSize,
	size_t *maxPacketSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	switch (audioCodec)
	{
		case estiAUDIO_G711_MULAW:
		case estiAUDIO_G711_ALAW:
			streamDescription->mSampleRate = 8000;
			streamDescription->mFormatID = (audioCodec == estiAUDIO_G711_ALAW ? kAudioFormatALaw : kAudioFormatULaw);
			streamDescription->mFormatFlags = 0;
			streamDescription->mBitsPerChannel = 8;
			streamDescription->mChannelsPerFrame = 1;
			streamDescription->mBytesPerFrame = 1;
			streamDescription->mFramesPerPacket = 1;
			streamDescription->mBytesPerPacket = 1;
			if (minPacketSize != nullptr)
			{
				*minPacketSize = 80;
			}
			if (maxPacketSize != nullptr)
			{
				*maxPacketSize = 480;
			}
			hResult = stiRESULT_SUCCESS;
			break;
		default:
			hResult = stiRESULT_INVALID_CODEC;
			break;
	}
	
	return hResult;
}


stiHResult AudioHandler::PlaybackCodecSet (EstiAudioCodec audioCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = 0;

	if (audioCodec == m_playbackCodec)
	{
		goto STI_BAIL;
	}

	if (m_initialized)
	{
		status = AudioUnitUninitialize (m_audioUnit);
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
		m_initialized = false;
	}
	
	hResult = StreamDescriptionGet(audioCodec, &m_playbackStreamDescription, nullptr, nullptr);
	stiTESTRESULT ();
	
	status = AudioUnitSetProperty (
		m_audioUnit,
		kAudioUnitProperty_StreamFormat,
		kAudioUnitScope_Input,
		OUTPUT_BUS,
		&m_playbackStreamDescription,
		sizeof (m_playbackStreamDescription));
	stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
	
	if (m_recordCodec == estiAUDIO_NONE) {
		// The audio unit needs both record/playback to be configured to work correctly.
		status = AudioUnitSetProperty (
			m_audioUnit,
			kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Output,
			INPUT_BUS,
			&m_playbackStreamDescription,
			sizeof (m_playbackStreamDescription));
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
	}

	m_playbackCodec = audioCodec;

STI_BAIL:
	return hResult;
}

stiHResult AudioHandler::RecordCodecSet (EstiAudioCodec audioCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = 0;

	if (audioCodec == m_recordCodec)
	{
		goto STI_BAIL;
	}

	if (m_initialized)
	{
		status = AudioUnitUninitialize (m_audioUnit);
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
		m_initialized = false;
	}
	
	hResult = StreamDescriptionGet(audioCodec, &m_recordStreamDescription, &m_minRecordPacketSize, &m_maxRecordPacketSize);
	stiTESTRESULT ();

	status = AudioUnitSetProperty (
		m_audioUnit,
		kAudioUnitProperty_StreamFormat,
		kAudioUnitScope_Output,
		INPUT_BUS,
		&m_recordStreamDescription,
		sizeof (m_recordStreamDescription));
	stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
	
	if (m_playbackCodec == estiAUDIO_NONE) {
		// The audio unit needs both record/playback to be configured to work correctly.
		status = AudioUnitSetProperty (
			m_audioUnit,
			kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Input,
			OUTPUT_BUS,
			&m_recordStreamDescription,
			sizeof (m_recordStreamDescription));
		stiTESTCONDMSG (status == 0, stiRESULT_DRIVER_ERROR, "status=%d", status);
	}
	
	m_recordCodec = audioCodec;

STI_BAIL:
	return hResult;
}


stiHResult AudioHandler::PacketGet (SsmdAudioFrame *audioFrame)
{
	if (!m_recordEnabled) {
		return stiRESULT_SUCCESS;
	}

	std::lock_guard<std::mutex> lock(m_recordMutex);

	if (recordBuffer.size() < m_minRecordPacketSize) {
		audioFrame->bIsSIDFrame = estiTRUE;
		audioFrame->unFrameSizeInBytes = 0;
		return stiRESULT_SUCCESS;
	}

	// Calculate frames to read in bulk
	std::streamsize framesToRead = std::min(
		static_cast<size_t>(audioFrame->unBufferMaxSize),
		m_maxRecordPacketSize
	) / m_recordStreamDescription.mBytesPerFrame;

	size_t bytesToRead = framesToRead * m_recordStreamDescription.mBytesPerFrame;
	size_t availableBytes = recordBuffer.size();
	size_t bytesToCopy = std::min(availableBytes, bytesToRead);

	if (bytesToCopy > 0) {
		// Use bulk copy
		auto block1 = recordBuffer.array_one();
		std::memcpy(audioFrame->pun8Buffer, block1.first, std::min(block1.second, bytesToCopy));

		// Remove copied data from the buffer
		recordBuffer.erase(recordBuffer.begin(), recordBuffer.begin() + bytesToCopy);
	}

	audioFrame->un8NumFrames = 1;
	audioFrame->unFrameSizeInBytes = static_cast<uint32_t>(bytesToCopy);

	if (recordBuffer.size() < m_minRecordPacketSize) {
		m_recordReadyStateChangedSignal.Emit(false);
	}

	return stiRESULT_SUCCESS;
}


stiHResult AudioHandler::PacketPut (SsmdAudioFrame *audioFrame)
{
	std::lock_guard<std::mutex> lock (m_playbackMutex);

	char* bufferPtr = reinterpret_cast<char*>(audioFrame->pun8Buffer);
	for (size_t i = 0; i < audioFrame->unFrameSizeInBytes * audioFrame->un8NumFrames; ++i)
	{
		playbackBuffer.put(bufferPtr[i]);
	}

	if (playbackBuffer.size() >= m_minPlaybackBufferSize)
	{
		m_playbackReadyStateChangedSignal.Emit (false);
	}

	return stiRESULT_SUCCESS;
}

OSStatus AudioPlayback (
	void *userData,
	AudioUnitRenderActionFlags *actionFlags,
	const AudioTimeStamp *timeStamp,
	UInt32 busNumber,
	UInt32 numberFrames,
	AudioBufferList *data)
{
	AudioHandler *handler = static_cast<AudioHandler *>(userData);
	std::lock_guard<std::mutex> lock (handler->m_playbackMutex);

	for (int i = 0; i < data->mNumberBuffers; ++i)
	{
		// We're required to fill the buffer all the way otherwise it won't be played at all.
		AudioBuffer& buffer = data->mBuffers[i];
		if (handler->playbackBuffer.empty ())
		{
			// Buffer is empty so play silence
			memset((char *)buffer.mData, -1, buffer.mDataByteSize);
			return noErr;
		}
		
		if (handler->playbackBuffer.size() >= buffer.mDataByteSize)
		{
			char* dataPtr = static_cast<char*>(buffer.mData);
			for (UInt32 j = 0; j < buffer.mDataByteSize; ++j)
			{
				dataPtr[j] = handler->playbackBuffer.get();
			}
		}
		else
		{
			// If we can't fill the audio buffer we don't have any choice but to stutter and wait for new audio to come in
			*actionFlags |= kAudioUnitRenderAction_OutputIsSilence;
			memset((char *)buffer.mData, -1, buffer.mDataByteSize);
		}
	}
	
	// `numberFrames` remains constant if the input device does not change. Bluetooth, for example, needs larger
	// playback buffers so `numberFrames` is larger.
	handler->m_minPlaybackBufferSize = numberFrames;
	if (handler->PlaybackBytesAvailableGet () < handler->m_minPlaybackBufferSize)
	{
		handler->m_playbackReadyStateChangedSignal.Emit (true);
	}

	return 0;
}


OSStatus AudioRecord (
	void *userData,
	AudioUnitRenderActionFlags *actionFlags,
	const AudioTimeStamp *timeStamp,
	UInt32 busNumber,
	UInt32 numberFrames,
	AudioBufferList *data)
{
	AudioHandler *handler = static_cast<AudioHandler *>(userData);
	AudioBufferList bufferList;
	
	__block UIApplicationState appState; // Capture the state in a block

	dispatch_sync(dispatch_get_main_queue(), ^{ // Use sync to get value back
		appState = [[UIApplication sharedApplication] applicationState];
	});

	if (appState == UIApplicationStateBackground) {
		// App is in background, do nothing
		return noErr;
	}
	
	
	bufferList.mNumberBuffers = 1;
	bufferList.mBuffers[0].mNumberChannels = 1;
	bufferList.mBuffers[0].mData = nullptr; // AudioUnitRender will allocate this for us
	bufferList.mBuffers[0].mDataByteSize = numberFrames * handler->m_recordStreamDescription.mBytesPerFrame;

	OSStatus status = AudioUnitRender (
		handler->m_audioUnit,
		actionFlags,
		timeStamp,
		busNumber,
		numberFrames,
		&bufferList);

	if (status == 0)
	{
		std::lock_guard<std::mutex> lock(handler->m_recordMutex);
		AudioBuffer &buffer = bufferList.mBuffers[0];
		//FBC-1095 - Automation testing, disabling the audio
		if (NSProcessInfo.processInfo.environment[@"AUTOMATION_ENABLED"] != nil &&
			NSProcessInfo.processInfo.environment[@"AUTOMATION_AUDIO_DISABLED"] != nil &&
			[NSProcessInfo.processInfo.environment[@"AUTOMATION_AUDIO_DISABLED"] caseInsensitiveCompare:@"TRUE"] == NSOrderedSame &&
			NSProcessInfo.processInfo.environment[@"NOC_INSTRUCTIONS_IMAGE"] != nil)
		{
			for (UInt32 i = 0; i < buffer.mDataByteSize; ++i)
			{
				handler->recordBuffer.put(-1);
			}
		}
		else if (handler->m_recordEnabled)
		{
			for (UInt32 i = 0; i < buffer.mDataByteSize; ++i)
			{
				handler->recordBuffer.put(((char *)buffer.mData)[i]);
			}
		}
		else
		{
			// Silence for G.711... Probably won't work for other formats.
			//Sending 0, user should hear SILENCE if the other persons mute themselves.
			for (UInt32 i = 0; i < buffer.mDataByteSize; ++i)
			{
				handler->recordBuffer.put(0);
			}
			
			// Log buffer contents for debugging
			// TODO: Uncomment this code if you DO need to debug the buffer.
			/*std::vector<char> bufferData(buffer.mDataByteSize);
			std::copy_n((char *)buffer.mData, buffer.mDataByteSize, bufferData.begin());
			stiTrace("Buffer contents when recording is disabled: %s", bufferData.data());*/
		}

		if (handler->recordBuffer.size() >= handler->m_minRecordPacketSize && handler->m_recordEnabled)
		{
			handler->m_recordReadyStateChangedSignal.Emit (true);
		}

		return 0;
	}
	else
	{
		stiTrace ("AudioHandler.cpp: Failed to render audio (status=%d)", status);
		return status;
	}
}


} // namespace vpe
