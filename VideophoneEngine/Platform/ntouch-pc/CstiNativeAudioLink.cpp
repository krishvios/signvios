#include "CstiNativeAudioLink.h"
namespace ntouchPC
{
	CstiNativeAudioLink::AudioStateFunction CstiNativeAudioLink::StartAudioRecordCallback = NULL;
	CstiNativeAudioLink::AudioStateFunction CstiNativeAudioLink::StopAudioRecordCallback = NULL;
	CstiNativeAudioLink::AudioStateFunction CstiNativeAudioLink::StartAudioPlaybackCallback = NULL;
	CstiNativeAudioLink::AudioStateFunction CstiNativeAudioLink::StopAudioPlaybackCallback = NULL;
	CstiNativeAudioLink::AudioPacketSendFunction CstiNativeAudioLink::ReceiveAudioPacketCallback = NULL;

	void CstiNativeAudioLink::StartAudioRecording()
	{
		if (StartAudioRecordCallback)
		{
			return StartAudioRecordCallback();
		}
	}
	void CstiNativeAudioLink::StopAudioRecording()
	{
		if (StopAudioRecordCallback)
		{
			return StopAudioRecordCallback();
		}
	}
	void CstiNativeAudioLink::StartAudioPlayback()
	{
		if (StartAudioPlaybackCallback)
		{
			return StartAudioPlaybackCallback();
		}
	}
	void CstiNativeAudioLink::StopAudioPlayback()
	{
		if (StopAudioPlaybackCallback)
		{
			return StopAudioPlaybackCallback();
		}
	}
	void CstiNativeAudioLink::AudioPlayback(std::shared_ptr<vpe::Audio::Packet> data)
	{
		if (ReceiveAudioPacketCallback)
		{
			return ReceiveAudioPacketCallback(&data);
		}
	}

	void CstiNativeAudioLink::SetStartAudioRecordFunction(AudioStateFunction callBack)
	{
		StartAudioRecordCallback = callBack;
	}
	void CstiNativeAudioLink::SetStopAudioRecordFunction(AudioStateFunction callBack)
	{
		StopAudioRecordCallback = callBack;
	}
	void CstiNativeAudioLink::SetStartAudioPlaybackFunction(AudioStateFunction callBack)
	{
		StartAudioPlaybackCallback = callBack;
	}
	void CstiNativeAudioLink::SetStopAudioPlaybackFunction(AudioStateFunction callBack)
	{
		StopAudioPlaybackCallback = callBack;
	}
	void CstiNativeAudioLink::SetReceiveAudioPacketFunction(AudioPacketSendFunction callBack)
	{
		ReceiveAudioPacketCallback = callBack;
	}
}