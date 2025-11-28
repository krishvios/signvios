#pragma once
#include "stiDefs.h"
#include <memory>
#include "AudioStructures.h"

namespace ntouchPC
{
	class CstiNativeAudioLink
	{
	public:
		typedef void(stiSTDCALL *AudioStateFunction)(void);
		typedef void(stiSTDCALL *AudioPacketSendFunction)(std::shared_ptr<vpe::Audio::Packet> *);

		static void SetReceiveAudioPacketFunction(AudioPacketSendFunction callBack);
		static void SetStartAudioRecordFunction(AudioStateFunction callBack);
		static void SetStopAudioRecordFunction(AudioStateFunction callBack);
		static void SetStartAudioPlaybackFunction(AudioStateFunction callBack);
		static void SetStopAudioPlaybackFunction(AudioStateFunction callBack);

		static void AudioPlayback(std::shared_ptr<vpe::Audio::Packet>);
		static void StartAudioRecording();
		static void StopAudioRecording();
		static void StartAudioPlayback();
		static void StopAudioPlayback();

	private:
		static AudioPacketSendFunction ReceiveAudioPacketCallback;
		static AudioStateFunction StartAudioRecordCallback;
		static AudioStateFunction StopAudioRecordCallback;
		static AudioStateFunction StartAudioPlaybackCallback;
		static AudioStateFunction StopAudioPlaybackCallback;
	};
}

