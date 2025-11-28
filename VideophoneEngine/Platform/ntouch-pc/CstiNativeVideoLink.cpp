#include "CstiNativeVideoLink.h"
#include "IPlatformVideoOutput.h"

namespace ntouchPC
{

	CstiNativeVideoLink::VideoPrivacyFunction CstiNativeVideoLink::RemoteVideoPrivacyCallback = nullptr;
	CstiNativeVideoLink::VideoPrivacyFunction CstiNativeVideoLink::LocalVideoPrivacyCallback = nullptr;
	CstiNativeVideoLink::DisplayFrameFunction CstiNativeVideoLink::DisplayFrameCallback = nullptr;
	CstiNativeVideoLink::CaptureSizeFunction CstiNativeVideoLink::CaptureSizeCallback = nullptr;
	CstiNativeVideoLink::CalculateCaptureSizeFunction CstiNativeVideoLink::CalculateCaptureSizeCallback = nullptr;
	CstiNativeVideoLink::EncoderConfigurationFunction CstiNativeVideoLink::EncoderConfigurationCallback = nullptr;
	CstiNativeVideoLink::RequestKeyFrameFunction CstiNativeVideoLink::RequestKeyFrameInputCallback = nullptr;
	CstiNativeVideoLink::PlaybackStatsFunction CstiNativeVideoLink::PlaybackStatsCallback = nullptr;

	void CstiNativeVideoLink::SetRemoteVideoPrivacyCallback(VideoPrivacyFunction callBack)
	{
		RemoteVideoPrivacyCallback = callBack;
	}

	void CstiNativeVideoLink::SetLocalVideoPrivacyCallback (VideoPrivacyFunction callBack)
	{
		LocalVideoPrivacyCallback = callBack;
	}

	void CstiNativeVideoLink::SetDisplayFrameCallback (DisplayFrameFunction callBack)
	{
		DisplayFrameCallback = callBack;
	}
	void CstiNativeVideoLink::SetCaptureSizeCallback(CaptureSizeFunction callBack)
	{
		CaptureSizeCallback = callBack;
	}

	void CstiNativeVideoLink::SetCalculateCaptureSizeCallBack(CalculateCaptureSizeFunction callback)
	{
		CalculateCaptureSizeCallback = callback;
	}

	void CstiNativeVideoLink::SetEncoderConfigurationCallBack(EncoderConfigurationFunction callback)
	{
		EncoderConfigurationCallback = callback;
	}

	void CstiNativeVideoLink::SetRequestKeyFrameInputCallBack(RequestKeyFrameFunction callback)
	{
		RequestKeyFrameInputCallback = callback;
	}

	void CstiNativeVideoLink::SetPlaybackStatsCallback (PlaybackStatsFunction callback)
	{
		PlaybackStatsCallback = callback;
	}

	void CstiNativeVideoLink::RemoteVideoPrivacy(bool videoPrivacy)
	{
		if (RemoteVideoPrivacyCallback)
		{
			RemoteVideoPrivacyCallback(videoPrivacy);
		}
	}

	void CstiNativeVideoLink::LocalVideoPrivacy (bool videoPrivacy)
	{
		if (LocalVideoPrivacyCallback)
		{
			LocalVideoPrivacyCallback (videoPrivacy);
		}
	}

	void CstiNativeVideoLink::DisplayFrame (std::shared_ptr<IVideoDisplayFrame> frame)
	{
		if (DisplayFrameCallback)
		{
			DisplayFrameCallback (frame);
		}
		else
		{
			auto videoOutput = (IPlatformVideoOutput*)IstiVideoOutput::InstanceGet();
			videoOutput->returnFrame(frame);
		}
	}

	void CstiNativeVideoLink::CaptureSize(int Width, int Height)
	{
		if (CaptureSizeCallback)
		{
			CaptureSizeCallback(Width, Height);
		}
	}

	void CstiNativeVideoLink::RequestKeyFrameInput()
	{
		if (RequestKeyFrameInputCallback)
		{
			RequestKeyFrameInputCallback();
		}
	}

	stiHResult CstiNativeVideoLink::CalculateCaptureSize(unsigned int unMaxFS, unsigned int unMaxMBps, int nCurrentDataRate, EstiVideoCodec eCodec, unsigned int *punVideoSizeCols, unsigned int *punVideoSizeRows, unsigned int *punFrameRate)
	{
		if (CalculateCaptureSizeCallback)
		{
			CalculateCaptureSizeCallback(unMaxFS, unMaxMBps, nCurrentDataRate, eCodec, punVideoSizeCols, punVideoSizeRows, punFrameRate);
			return stiRESULT_SUCCESS;
		}
		else
		{
			return stiRESULT_ERROR;
		}
	}

	stiHResult CstiNativeVideoLink::EncoderConfiguration(int nCurrentBitRate, unsigned int punWidth, unsigned int punHeight, unsigned int punFrameRate, int eCodec, int eProfile, int nLevel)
	{
		if (EncoderConfigurationCallback)
		{
			EncoderConfigurationCallback(nCurrentBitRate, punWidth, punHeight, punFrameRate, eCodec, eProfile, nLevel);
			return stiRESULT_SUCCESS;
		}
		else
		{
			return stiRESULT_ERROR;
		}
	}

	void CstiNativeVideoLink::PlaybackStats (int width, int height, uint64_t decodedFrameCount, uint64_t discardedDisplayFrameCount, std::string decoderName)
	{
		if (PlaybackStatsCallback)
		{
			PlaybackStatsCallback (width, height, decodedFrameCount, discardedDisplayFrameCount, decoderName.c_str());
		}
	}
}