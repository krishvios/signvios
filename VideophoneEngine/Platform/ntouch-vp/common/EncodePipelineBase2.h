// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2021 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "EncodePipelineBase.h"
#include "CaptureBinBase2.h"
#include "EncodeBinBase.h"
#include "SelfViewBinBase.h"


class EncodePipelineBase2 : public EncodePipelineBase
{
public:

	EncodePipelineBase2 () = default;
	~EncodePipelineBase2 () override = default;

	EncodePipelineBase2 (const EncodePipelineBase2 &other) = delete;
	EncodePipelineBase2 (EncodePipelineBase2 &&other) = delete;
	EncodePipelineBase2 &operator= (const EncodePipelineBase2 &other) = delete;
	EncodePipelineBase2 &operator= (EncodePipelineBase2 &&other) = delete;

	void create (void *qquickItem);

	void encodeSizeSet (unsigned width, unsigned height) override;
	void codecSet (
		EstiVideoCodec codec,
		EstiVideoProfile profile,
		unsigned int constraints,
		unsigned int level,
		EstiPacketizationScheme packetizationMode) override;
	void bitrateSet (unsigned kbps) override;
	void framerateSet (unsigned fps) override;
	void encodingStart () override;
	void encodingStop () override;
	
	void ptzRectangleSet (
		const CstiCameraControl::SstiPtzSettings &ptzRectangle) override;

	virtual stiHResult exposureWindowSet (const CstiCameraControl::SstiPtzSettings &exposureWindow);

	void recordBinInitialize (
		bool recordFastStartFile) override;

	stiHResult brightnessRangeGet (
		PropertyRange *range) const override;

	stiHResult brightnessGet (
		int *brightness) const override;

	stiHResult brightnessSet (
		int brightness) override;

	stiHResult saturationRangeGet (
		PropertyRange *range) const override;

	stiHResult saturationGet (
		int *focusPosition) const override;

	stiHResult saturationSet (
		int focusPosition) override;

	stiHResult focusRangeGet (
		PropertyRange *range) const override;
		
	stiHResult focusPositionGet (
		int *focusPosition) const override;
	
	stiHResult focusPositionSet (
		int focusPosition) override;

	stiHResult singleFocus (
		int initialFocusPosition,
		int focusRegionStartX,
		int focusRegionStartY,
		int focusRegionWidth,
		int focusRegionHeight,
		std::function<void(bool success, int contrast)> callback) override;

	stiHResult contrastLoopStart (
		std::function<void(int contrast)> callback) override;
	stiHResult contrastLoopStop () override;
	
	stiHResult ambientLightGet (
		IVideoInputVP2::AmbientLight &ambientLight,
		float &rawAmbientLight) override;

protected:

	std::unique_ptr<EncodeBinBase> m_encodeBin;
	std::unique_ptr<SelfViewBinBase> m_selfViewBin;

	GStreamerElement displaySinkGet () override;

	virtual const CaptureBinBase2 &captureBinBase2Get () const = 0;
	virtual CaptureBinBase2 &captureBinBase2Get () = 0;

	const CaptureBinBase &captureBinBaseGet () const override
	{
		return captureBinBase2Get ();
	}

	CaptureBinBase &captureBinBaseGet () override
	{
		return captureBinBase2Get ();
	}

private:

	bool m_recordFile {false};

	virtual void captureBinCreate () = 0;
	virtual std::unique_ptr<EncodeBinBase> encodeBinCreate (const CaptureBinBase2 &captureBin) = 0;
	virtual std::unique_ptr<SelfViewBinBase> selfViewBinCreate (void *qquickItem) = 0;
};
