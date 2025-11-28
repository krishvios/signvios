/*!
*  \file CstiGStreamerVideoGraphBase2.h
*  \brief The GStremer Interface
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2021 Sorenson Communications, Inc. -- All rights reserved
*
*/
#pragma once

#include "CstiGStreamerVideoGraphBase.h"
#include "EncodePipelineBase2.h"
#include "RecordBinBase.h"

class CstiGStreamerVideoGraphBase2 : public CstiGStreamerVideoGraphBase
{
public:

	CstiGStreamerVideoGraphBase2 (
		bool rcuConnected);

	~CstiGStreamerVideoGraphBase2 () override;

	CstiGStreamerVideoGraphBase2 (const CstiGStreamerVideoGraphBase2 &other) = delete;
	CstiGStreamerVideoGraphBase2 (CstiGStreamerVideoGraphBase2 &&other) = delete;
	CstiGStreamerVideoGraphBase2 &operator = (const CstiGStreamerVideoGraphBase2 &other) = delete;
	CstiGStreamerVideoGraphBase2 &operator = (CstiGStreamerVideoGraphBase2 &&other) = delete;

	stiHResult PipelineReset () override;

	stiHResult PipelineDestroy ();
	
	stiHResult EncodeSettingsSet (
		const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings,
		bool *pbEncodeSizeChanged) override;
	
	IstiVideoInput::SstiVideoRecordSettings EncodeSettingsGet () const override;
	
	stiHResult DisplaySettingsSet (
		IstiVideoOutput::SstiImageSettings *pImageSettings) override;
		
	int32_t MaxPacketSizeGet () override;
	
	stiHResult RcuConnectedSet (
		bool bRcuConnected);

	stiHResult SelfViewWidgetSet (
		void * QQuickItem);
	
	stiHResult exposureWindowSet (
		const CstiCameraControl::SstiPtzSettings &exposureWindow);
	
	stiHResult brightnessRangeGet (
		PropertyRange *range) const override;

	stiHResult brightnessGet (
		int *brightness) const override;

	stiHResult brightnessSet (
		int brightness) override;

	stiHResult saturationRangeGet (
		PropertyRange *range) const override;

	stiHResult saturationGet (
		int *saturation) const override;

	stiHResult saturationSet (
		int saturation) override;

	stiHResult focusRangeGet (
		PropertyRange *range) const;
		
	stiHResult focusPositionGet (
		int *focusPosition) const;
	
	stiHResult focusPositionSet (
		int focusPosition);

	stiHResult singleFocus (
		int initialFocusPosition,
		int focusRegionStartX,
		int focusRegionStartY,
		int focusRegionWidth,
		int focusRegionHeight,
		std::function<void(bool success, int contrast)> callback);

	stiHResult contrastLoopStart (
		std::function<void(int contrast)> callback);
	stiHResult contrastLoopStop ();
	
	stiHResult ambientLightGet (
		IVideoInputVP2::AmbientLight &ambientLight,
		float &rawAmbientLight);

	stiHResult EncodeStop () override;
	stiHResult EncodeStart () override;

	stiHResult KeyFrameRequest ();

	stiHResult PlaySet (
		bool  play);
	
	stiHResult videoRecordToFile();

	stiHResult EncodePipelineStatePauseSet ();
	
	stiHResult EncodePipelineStatePlaySet ();
	
	bool EncodePipelineCreated ()
	{
		return m_EncodeCapturePipelineCreated;
	};
	
	CstiSignal<> encodeCameraError;
	
protected:

	virtual IstiVideoInput::SstiVideoRecordSettings defaultVideoRecordSettingsGet () = 0;

	stiHResult KeyFrameForce ();

	// Encode pipeline functions
	stiHResult EncodePipelineCreate ();
	stiHResult EncodePipelineDestroy ();
	stiHResult EncodePipelineStart ();
	
	const EncodePipelineBase &encodeCapturePipelineBaseGet () const override
	{
		return encodeCapturePipelineBase2Get();
	}

	EncodePipelineBase &encodeCapturePipelineBaseGet () override
	{
		return encodeCapturePipelineBase2Get();
	}

	void *m_qquickItem {nullptr};
	bool m_bEncodePipelinePlaying {false};

	bool m_EncodeCapturePipelineCreated {false};

private:

	void encodeCapturePipelineCreate (void *qquickItem);

	virtual const EncodePipelineBase2 &encodeCapturePipelineBase2Get() const = 0;
	virtual EncodePipelineBase2 &encodeCapturePipelineBase2Get() = 0;
	
	void ptzRectangleSet (const CstiCameraControl::SstiPtzSettings &ptzRectangle) override;

private:

	GStreamerSample m_currentGstSample;
	SstiImageCaptureSize *m_imageSize {nullptr};
	bool m_doCrop {false};
	bool m_frameCapture {false};

	GstAppSinkCallbacks m_appSinkCallbacks {};
	
	gulong m_unEncodeNewBufferSignalHandler{};

	gulong m_unFrameCaptureNewBufferSignalHandler = 0;
	
	void frameCountIncrement ();

	IVideoInputVP2::AmbientLight m_lastAmbientLight {IVideoInputVP2::AmbientLight::HIGH};
	float m_lastRawAmbientLight {0};

};
