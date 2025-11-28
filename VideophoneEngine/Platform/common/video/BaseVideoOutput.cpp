#include "BaseVideoOutput.h"

ISignal<IstiVideoOutput::EstiRemoteVideoMode>& BaseVideoOutput::remoteVideoModeChangedSignalGet ()
{
	return remoteVideoModeChangedSignal;
}

ISignal<uint32_t, uint32_t>& BaseVideoOutput::decodeSizeChangedSignalGet ()
{
	return decodeSizeChangedSignal;
}

ISignal<>& BaseVideoOutput::videoRemoteSizePosChangedSignalGet ()
{
	return videoRemoteSizePosChangedSignal;
}

ISignal<>& BaseVideoOutput::keyframeNeededSignalGet ()
{
	return keyframeNeededSignal;
}

ISignal<bool>& BaseVideoOutput::cecSupportedSignalGet ()
{
	return cecSupportedSignal;
}

ISignal<>& BaseVideoOutput::hdmiOutStatusChangedSignalGet ()
{
	return hdmiOutStatusChangedSignal;
}

ISignal<>& BaseVideoOutput::videoScreensaverStartFailedSignalGet ()
{
	return videoScreensaverStartFailedSignal;
}

ISignal<>& BaseVideoOutput::preferredDisplaySizeChangedSignalGet ()
{
	return preferredDisplaySizeChangedSignal;
}

ISignal<bool>& BaseVideoOutput::frameBufferReadySignalGet ()
{
	return frameBufferReadySignal;
}

ISignal<>& BaseVideoOutput::videoOutputPlaybackStoppedSignalGet ()
{
	return videoOutputPlaybackStoppedSignal;
}

