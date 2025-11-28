#pragma once
#include "IstiVideoOutput.h"
#include "CstiSignal.h"

class BaseVideoOutput : virtual public IstiVideoOutput
{
public:
	BaseVideoOutput () = default;
	~BaseVideoOutput () override = default;

	BaseVideoOutput (const BaseVideoOutput &) = delete;
	BaseVideoOutput (BaseVideoOutput &&) = delete;
	BaseVideoOutput &operator= (const BaseVideoOutput &) = delete;
	BaseVideoOutput &operator= (BaseVideoOutput &&) = delete;

	ISignal<IstiVideoOutput::EstiRemoteVideoMode>& remoteVideoModeChangedSignalGet () override;
	ISignal<uint32_t, uint32_t>& decodeSizeChangedSignalGet () override;
	ISignal<>& videoRemoteSizePosChangedSignalGet () override;
	ISignal<>& keyframeNeededSignalGet () override;
	ISignal<bool>& cecSupportedSignalGet () override;
	ISignal<>& hdmiOutStatusChangedSignalGet () override;
	ISignal<>& videoScreensaverStartFailedSignalGet () override;
	ISignal<>& preferredDisplaySizeChangedSignalGet () override;
	ISignal<bool>& frameBufferReadySignalGet () override;
	ISignal<>& videoOutputPlaybackStoppedSignalGet () override;

protected:

	CstiSignal<EstiRemoteVideoMode> remoteVideoModeChangedSignal;
	CstiSignal<uint32_t, uint32_t> decodeSizeChangedSignal;
	CstiSignal<> videoRemoteSizePosChangedSignal;
	CstiSignal<> keyframeNeededSignal;
	CstiSignal<bool> cecSupportedSignal;
	CstiSignal<> hdmiOutStatusChangedSignal;
	CstiSignal<> videoScreensaverStartFailedSignal;
	CstiSignal<> preferredDisplaySizeChangedSignal;
	CstiSignal<bool> frameBufferReadySignal;
	CstiSignal<> videoOutputPlaybackStoppedSignal;
};

