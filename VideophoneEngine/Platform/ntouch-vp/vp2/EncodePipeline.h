#pragma once

#include "EncodePipelineBase.h"
#include "CaptureBin.h"
#include "IstiVideoInput.h"
#include "IstiVideoOutput.h"

#define stiMAX_CAPTURE_WIDTH 1920
#define stiMAX_CAPTURE_HEIGHT 1072 //because its the value under 1080 thats a multiple of 16


class EncodePipeline : public EncodePipelineBase
{
public:

	EncodePipeline () = default;
	~EncodePipeline ();

	void create (
		const IstiVideoOutput::SstiImageSettings &displaySettings,
		bool privacy,
		const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings);

	void rcuConnectedSet (bool rcuConnected);

	void cameraControlFdSet (int cameraControlFd);

	void encodeSizeSet (unsigned width, unsigned height) override;
	void codecSet (
		EstiVideoCodec codec,
		EstiVideoProfile profile,
		unsigned int constraints,
		unsigned int level,
		EstiPacketizationScheme packetizationMode) override;
	void bitrateSet (unsigned kbps) override;
	void framerateSet (unsigned fps) override;

	stiHResult brightnessSet (int brightness) override;
	stiHResult saturationSet (int saturation) override;

	void recordBinInitialize (bool recordFastStartFile) override {}

	void encodingStart () override;
	void encodingStop () override;

	void displaySettingsSet (
		const IstiVideoOutput::SstiImageSettings &displaySettings);

	gboolean keyframeEventSend () override;

	stiHResult H264EncodeSettingsSet (
		const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings);

	stiHResult EncodeConvertResize (
		uint32_t nWidth,
		uint32_t nHeight);

	GstStateChangeReturn stateSet (GstState state) override;

protected:

	GStreamerElement displaySinkGet () override;

	const CaptureBinBase &captureBinBaseGet () const override
	{
		return m_captureBin;
	}

	CaptureBinBase &captureBinBaseGet () override
	{
		return m_captureBin;
	}

private:

	CaptureBin m_captureBin;

	stiHResult EncodeSinkBinCreate (const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings);
	stiHResult EncodeSinkBinAddAndLink ();
	stiHResult DisplaySinkBinCreate (
		const IstiVideoOutput::SstiImageSettings &displaySettings,
		bool privacy);
	stiHResult DisplaySinkBinAddAndLink ();
	stiHResult EncodeSinkBinRemove ();
	stiHResult DisplaySinkBinRemove ();
	stiHResult H264EncodeBinCreate (const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings);
	stiHResult H264EncodeBinAddAndLink (
		GstElement *pGstElementPipeline,
		GstPad *pGstLinkPad);
	stiHResult H264EncodeBinRemove (
		GstElement *pGstElementPipeline);
	stiHResult KeyFrameForce (
		GstElement *pGstElementPipeline);

	GstElement *m_pGstElementEncodeSinkBin {nullptr};
	GstElement *m_pGstElementEncodeConvert {nullptr};
	GstElement *m_pGstElementEncodeOutputSelector {nullptr};
	GstPad *m_pGstPadEncodeOuputSelectorFakeSrc {nullptr};
	GstElement *m_pGstElementEncodeFakeSink {nullptr};
	GstElement *m_pGstElementEncodeFunnel {nullptr};
	GstPad *m_pGstPadEncodeOuputSelectorH264Src {nullptr};
	GstPad *m_pGstPadEncodeFunnelH264Sink {nullptr};
	GstElement *m_pGstElementEncodeH264Bin {nullptr};
	GstElement *m_pGstElementEncodeH264 {nullptr};
	GstElement *m_pGstElementEncodePostH264Filter {nullptr};
	GStreamerAppSink m_gstElementEncodeAppSink;
	GStreamerElement m_displaySink;
};
