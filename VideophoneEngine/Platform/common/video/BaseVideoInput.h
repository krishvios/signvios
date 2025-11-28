#pragma once
#include "IstiVideoInput.h"
#include "CstiSignal.h"
#include <vector>

class BaseVideoInput : virtual public IstiVideoInput
{
public:
	BaseVideoInput () = default;
	~BaseVideoInput () override = default;

	BaseVideoInput (const BaseVideoInput &) = delete;
	BaseVideoInput (BaseVideoInput &&) = delete;
	BaseVideoInput &operator= (const BaseVideoInput &) = delete;
	BaseVideoInput &operator= (BaseVideoInput &&) = delete;

	ISignal<>& usbRcuConnectionStatusChangedSignalGet () override;
	ISignal<int>& focusCompleteSignalGet () override;
	ISignal<bool>& videoPrivacySetSignalGet () override;
	ISignal<>& videoRecordSizeChangedSignalGet () override;
	ISignal<>& videoInputSizePosChangedSignalGet () override;
	ISignal<const std::vector<uint8_t> &>& frameCaptureAvailableSignalGet () override;
	ISignal<>& packetAvailableSignalGet () override;
	ISignal<>& holdserverVideoCompleteSignalGet () override;
	ISignal<>& captureCapabilitiesChangedSignalGet () override;
	ISignal<>& cameraMoveCompleteSignalGet () override;
	ISignal<uint64_t, uint64_t>& videoRecordPositionSignalGet () override;
	ISignal<const std::string, const std::string>& videoRecordFinishedSignalGet () override;
	ISignal<>& videoRecordEncodeCameraErrorSignalGet () override;
	ISignal<>& videoRecordErrorSignalGet () override;

protected:

	CstiSignal<> usbRcuConnectionStatusChangedSignal;
	CstiSignal<int> focusCompleteSignal;
	CstiSignal<bool> videoPrivacySetSignal;
	CstiSignal<> videoRecordSizeChangedSignal;
	CstiSignal<> videoInputSizePosChangedSignal;
	CstiSignal<const std::vector<uint8_t> &> frameCaptureAvailableSignal;
	CstiSignal<> packetAvailableSignal;
	CstiSignal<> holdserverVideoCompleteSignal;
	CstiSignal<> captureCapabilitiesChangedSignal;
	CstiSignal<> cameraMoveCompleteSignal;
	CstiSignal<uint64_t, uint64_t> videoRecordPositionSignal;
	CstiSignal<const std::string, const std::string> videoRecordFinishedSignal;
	CstiSignal<> videoRecordEncodeCameraErrorSignal;
	CstiSignal<> videoRecordErrorSignal;
};
