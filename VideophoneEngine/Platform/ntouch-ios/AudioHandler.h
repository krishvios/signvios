/*!
 * \file AudioHandler.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "stiSVX.h"
#include "stiError.h"
#include "AudioControl.h"
#include "CstiSignal.h"
#include <sstream>
#include <AudioToolbox/AudioToolbox.h>
#include "CircularBuffer.h"

namespace vpe {


class AudioHandler
{
public:
	explicit AudioHandler (CstiSignal<bool>& recordReadyStateChangedSignal, CstiSignal<bool>& playbackReadyStateChangedSignal);
	virtual ~AudioHandler ();

	stiHResult Initialize ();
	stiHResult Uninitialize ();

	stiHResult Start ();
	bool IsRunning ();
	stiHResult Stop ();

	stiHResult PlaybackEnable (bool enable);
	bool IsPlaybackEnabled () { return m_playbackEnabled; }
	stiHResult RecordEnable (bool enable);
	bool IsRecordEnabled () { return m_recordEnabled; }

	stiHResult CodecsGet (std::list<EstiAudioCodec> *codecs);
	stiHResult PlaybackCodecSet (EstiAudioCodec audioCodec);
	stiHResult RecordCodecSet (EstiAudioCodec audioCodec);

	stiHResult PacketGet (SsmdAudioFrame *audioFrame);
	stiHResult PacketPut (SsmdAudioFrame *audioFrame);

private:
	std::streamsize RecordBytesAvailableGet ();
	std::streamsize PlaybackBytesAvailableGet ();
	stiHResult StreamDescriptionGet (EstiAudioCodec audioCodec, AudioStreamBasicDescription *streamDescription, size_t *minPacketSize, size_t *maxPacketSize);

	bool m_initialized = false;
	AudioUnit m_audioUnit = nullptr;
	
	bool m_recordEnabled = false;
	CstiSignal<bool>& m_recordReadyStateChangedSignal;
	EstiAudioCodec m_recordCodec = estiAUDIO_NONE;
	AudioStreamBasicDescription m_recordStreamDescription;
	size_t m_minRecordPacketSize = 0;
	size_t m_maxRecordPacketSize = SIZE_T_MAX;
	
	bool m_playbackEnabled = false;
	CstiSignal<bool>& m_playbackReadyStateChangedSignal;
	EstiAudioCodec m_playbackCodec = estiAUDIO_NONE;
	AudioStreamBasicDescription m_playbackStreamDescription;
	size_t m_minPlaybackBufferSize = 0;

	bool m_started = false;
	bool m_interrupted = false;
	void *m_interruptedToken;
	void *m_resetToken;


	std::stringstream m_recordStream;
	std::stringstream m_playbackStream;
	std::mutex m_recordMutex;
	std::mutex m_playbackMutex;
	CircularBuffer<char>  playbackBuffer;
	CircularBuffer<char>  recordBuffer;
	
	friend OSStatus AudioPlayback (
		void *userData,
		AudioUnitRenderActionFlags *actionFlags,
		const AudioTimeStamp *timeStamp,
		UInt32 busNumber,
		UInt32 numberFrames,
		AudioBufferList *data);
	friend OSStatus AudioRecord (
		void *userData,
		AudioUnitRenderActionFlags *actionFlags,
		const AudioTimeStamp *timeStamp,
		UInt32 busNumber,
		UInt32 numberFrames,
		AudioBufferList *data);
};

OSStatus AudioPlayback (
	void *userData,
	AudioUnitRenderActionFlags *actionFlags,
	const AudioTimeStamp *timeStamp,
	UInt32 busNumber,
	UInt32 numberFrames,
	AudioBufferList *data);
OSStatus AudioRecord (
	void *userData,
	AudioUnitRenderActionFlags *actionFlags,
	const AudioTimeStamp *timeStamp,
	UInt32 busNumber,
	UInt32 numberFrames,
	AudioBufferList *data);
}

