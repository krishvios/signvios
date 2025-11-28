#pragma once

#include <gst/gst.h>
#include "stiDefs.h"
#include "stiSVX.h"
#include "GStreamerAppSink.h"
#include "GStreamerBin.h"
#include "GStreamerEncodingVideoProfile.h"
#include "IstiVideoInput.h"
#include "RecordBinBase.h"
#include "CaptureBinBase2.h"
#include <vector>

class EncodeBinBase : public GStreamerBin
{
public:
	EncodeBinBase();
	~EncodeBinBase() override = default;

	EncodeBinBase (const EncodeBinBase &other) = delete;
	EncodeBinBase (EncodeBinBase &&other) = delete;
	EncodeBinBase &operator= (const EncodeBinBase &other) = delete;
	EncodeBinBase &operator= (EncodeBinBase &&other) = delete;

	void create (const CaptureBinBase2 &captureBin);

	void codecSet (
		EstiVideoCodec codec,
		EstiVideoProfile profile,
		unsigned int constraints,
		unsigned int level,
		EstiPacketizationScheme packetizationMode);

	void newBufferCallbackSet(
		std::function<void(GStreamerAppSink appSink)> func);

	void sizeSet(unsigned width, unsigned height);
	virtual void bitrateSet(unsigned kbps);
	void framerateSet(unsigned fps);
	void encodingStart();
	void encodingStop();
	void recordBinInitialize(
		bool recordFastStartFile);
	void fileRecordSet(
		bool fileRecord)
	{
		m_fileRecord = fileRecord;
	}

	void encoderFilterUpdate();
	
	virtual EstiVideoFrameFormat bufferFormatGet () = 0;

protected:

	const char* mediaTypeGet();

	// Data members
	EstiVideoCodec m_codec {estiVIDEO_H265};
	EstiVideoProfile m_profile {estiH265_PROFILE_MAIN};
	unsigned int m_constraints {0};
	unsigned int m_level {estiH265_LEVEL_4};
	EstiPacketizationScheme m_packetizationMode {estiH265_NON_INTERLEAVED};

	unsigned m_width = 1280;
	unsigned m_height = 720;
	unsigned m_bitrateKbps = 1024;
	unsigned m_framerate = 30;
	
	const CaptureBinBase2 *m_pCaptureBin {nullptr};

private:

	virtual GStreamerElement videoConvertElementGet () = 0;

	static std::unique_ptr<RecordBinBase> recordBinCreate ();

	// Methods
	void elementRankingSet();
	virtual GStreamerCaps encodeLegInputCapsGet() = 0;
	virtual GStreamerCaps inputCapsGet() = 0;
	virtual GStreamerCaps outputCapsGet() = 0;
	virtual GStreamerElement encoderGet () = 0;

	void removeEncoderElement ();
	stiHResult addEncoderElement ();

	std::unique_ptr<RecordBinBase> m_recordBin;
	bool m_fileRecord {false};

	// Gstreamer elements
	GStreamerElement m_encoderPreScaleFilter;
	GStreamerElement m_encoderScale;
	GStreamerElement m_encoderPostScaleFilter;
	GStreamerElement m_encoderElement;
	GStreamerElement m_bitstreamFilter;
	GStreamerElement m_outputSelector;
	GStreamerAppSink m_appSink;
	GStreamerPad m_outputPadAppSink;
	GStreamerPad m_outputPadRecordPipeline;

	bool m_encoding {false};
};
