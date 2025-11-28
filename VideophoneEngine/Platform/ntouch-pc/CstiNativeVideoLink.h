#pragma once
#include "stiDefs.h"
#include "stiSVX.h"
#include "IVideoDisplayFrame.h"

namespace ntouchPC
{
	class CstiNativeVideoLink
	{
		public:
			typedef void(stiSTDCALL *VideoPrivacyFunction)(bool);
			typedef void(stiSTDCALL *DisplayFrameFunction)(std::shared_ptr<IVideoDisplayFrame>);
			typedef void(stiSTDCALL *CaptureSizeFunction)(int, int);
			typedef void(stiSTDCALL *CalculateCaptureSizeFunction)(unsigned int unMaxFS, unsigned int unMaxMBps, int nCurrentDataRate, EstiVideoCodec eCodec, unsigned int *punVideoSizeCols, unsigned int *punVideoSizeRows, unsigned int *punFrameRate);
			typedef void(stiSTDCALL *EncoderConfigurationFunction)(int nCurrentBitRate, unsigned int punWidth, unsigned int punHeight, unsigned int punFrameRate, int eCodec, int eProfile, int nLevel);
			typedef void(stiSTDCALL *RequestKeyFrameFunction)(void);
			typedef void(stiSTDCALL *PlaybackStatsFunction)(int width, int height, uint64_t decodedFrames, uint64_t discaredFrames, const char *decoderName);

			static void SetRemoteVideoPrivacyCallback(VideoPrivacyFunction callBack);
			static void SetLocalVideoPrivacyCallback (VideoPrivacyFunction callBack);
			static void SetDisplayFrameCallback (DisplayFrameFunction callBack);
			static void SetCaptureSizeCallback(CaptureSizeFunction callBack);
			static void SetCalculateCaptureSizeCallBack(CalculateCaptureSizeFunction callback);
			static void SetEncoderConfigurationCallBack(EncoderConfigurationFunction callback);
			static void SetRequestKeyFrameInputCallBack(RequestKeyFrameFunction callback);
			static void SetPlaybackStatsCallback (PlaybackStatsFunction callback);

			static void RemoteVideoPrivacy(bool);
			static void LocalVideoPrivacy (bool);
			static void CaptureSize(int, int);
			static void RequestKeyFrameInput();
			static stiHResult CalculateCaptureSize(unsigned int unMaxFS, unsigned int unMaxMBps, int nCurrentDataRate, EstiVideoCodec eCodec, unsigned int *punVideoSizeCols, unsigned int *punVideoSizeRows, unsigned int *punFrameRate);
			static stiHResult EncoderConfiguration(int nCurrentBitRate, unsigned int punWidth, unsigned int punHeight, unsigned int punFrameRate, int eCodec, int eProfile, int nLevel);
			static void DisplayFrame (std::shared_ptr<IVideoDisplayFrame> frame);
			static void PlaybackStats (int width, int height, uint64_t decodedFrameCount, uint64_t discardedDisplayFrameCount, std::string decoderName);

		private:
			static VideoPrivacyFunction RemoteVideoPrivacyCallback;
			static VideoPrivacyFunction LocalVideoPrivacyCallback;
			static DisplayFrameFunction DisplayFrameCallback;
			static CaptureSizeFunction CaptureSizeCallback;
			static CalculateCaptureSizeFunction CalculateCaptureSizeCallback;
			static EncoderConfigurationFunction EncoderConfigurationCallback;
			static RequestKeyFrameFunction RequestKeyFrameInputCallback;
			static PlaybackStatsFunction PlaybackStatsCallback;
	};
}
