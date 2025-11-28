#include "BaseVideoInput.h"

ISignal<>& BaseVideoInput::usbRcuConnectionStatusChangedSignalGet ()
{
	return usbRcuConnectionStatusChangedSignal;
}

ISignal<int>& BaseVideoInput::focusCompleteSignalGet ()
{
	return focusCompleteSignal;
}

ISignal<bool>& BaseVideoInput::videoPrivacySetSignalGet ()
{
	return videoPrivacySetSignal;
}

ISignal<>& BaseVideoInput::videoRecordSizeChangedSignalGet ()
{
	return videoRecordSizeChangedSignal;
}

ISignal<>& BaseVideoInput::videoInputSizePosChangedSignalGet ()
{
	return videoInputSizePosChangedSignal;
}

ISignal<const std::vector<uint8_t> &>& BaseVideoInput::frameCaptureAvailableSignalGet ()
{
	return frameCaptureAvailableSignal;
}

ISignal<>& BaseVideoInput::packetAvailableSignalGet ()
{
	return packetAvailableSignal;
}
ISignal<>& BaseVideoInput::holdserverVideoCompleteSignalGet ()
{
	return holdserverVideoCompleteSignal;
}

ISignal<>& BaseVideoInput::captureCapabilitiesChangedSignalGet ()
{
	return captureCapabilitiesChangedSignal;
}

ISignal<>& BaseVideoInput::cameraMoveCompleteSignalGet ()
{
	return cameraMoveCompleteSignal;
}

ISignal<uint64_t, uint64_t>& BaseVideoInput::videoRecordPositionSignalGet ()
{
	return videoRecordPositionSignal;
}

ISignal<const std::string, const std::string>& BaseVideoInput::videoRecordFinishedSignalGet ()
{
	return videoRecordFinishedSignal;
}

ISignal<>& BaseVideoInput::videoRecordEncodeCameraErrorSignalGet ()
{
	return videoRecordEncodeCameraErrorSignal;
}

ISignal<>& BaseVideoInput::videoRecordErrorSignalGet ()
{
	return videoRecordErrorSignal;
}

