#include "IntelVideoDecoder.h"
#include "mfxadapter.h"

#define MSDK_CHECK_STATUS(status, message) {if ((status) < MFX_ERR_NONE) {stiTrace("%s - %d\n",message, status); return status;}}
#define MSDK_ALIGN32(X)                 (((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define MSDK_ALIGN16(value)             (((value + 15) >> 4) << 4)

IntelVideoDecoder::IntelVideoDecoder ()
{
	mfxStatus status = MFX_ERR_NONE;
	mfxInitParam initParameters{ 0 };
	initParameters.Version.Major = 1;
	initParameters.Version.Minor = 0;
	initParameters.GPUCopy = true;

	initParameters.Implementation = MFX_IMPL_HARDWARE_ANY;

	status = m_mfxSession.InitEx (initParameters);
	if (status != MFX_ERR_NONE)
	{
		throw std::runtime_error ("Intel Session Initialized Failed");
	}

	mfxVersion version;
	status = m_mfxSession.QueryVersion (&version); // get real API version of the loaded library
	if (status != MFX_ERR_NONE)
	{
		throw std::runtime_error ("Intel Session Query Failed");
	}
	
	mfxIMPL implementation;
	status = MFXQueryIMPL(m_mfxSession, &implementation); // get the real implementation
	if (status != MFX_ERR_NONE)
	{
		throw std::runtime_error ("Intel Implementation Query Failed");
	}

	if (implementation == MFX_IMPL_SOFTWARE)
	{
		throw std::runtime_error ("Intel Implementation Software");
	}

	m_pmfxDEC = std::make_unique<MFXVideoDECODE>(m_mfxSession);

	for (int i = 0; i < outputFrameCount; i++)
	{
		m_workFrames.push(std::make_shared<IntelWorkerFrame> ());
	}
}


IntelVideoDecoder::~IntelVideoDecoder ()
{
	m_pmfxDEC->Close();
}

bool IntelVideoDecoder::decode (IstiVideoPlaybackFrame *frame, uint8_t *flags)
{
	auto buffer = frame->BufferGet ();
	auto length = frame->DataSizeGet ();
	mfxBitstream bitstream{ 0 };
	bitstream.Data = buffer;
	bitstream.DataLength = length;
	bitstream.MaxLength = frame->BufferSizeGet ();
	bitstream.DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;

	auto status = MFX_ERR_NONE;
	
	m_videoParameters.mfx.CodecId = m_currentCodecId;
	status = m_pmfxDEC->DecodeHeader (&bitstream, &m_videoParameters);

	if (status != MFX_ERR_MORE_DATA)
	{
		if (status != MFX_ERR_NONE)
		{
			return false;
		}
		*flags |= IstiVideoDecoder::FLAG_KEYFRAME;

		m_videoParameters.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

		status = m_pmfxDEC->Query (&m_videoParameters, &m_videoParameters);
		if (status != MFX_ERR_NONE)
		{
			return false;
		}
		if (!m_decoderInitialized)
		{
			// Query number of required surfaces for decoder
			mfxFrameAllocRequest request;
			memset (&request, 0, sizeof (request));
			status = m_pmfxDEC->QueryIOSurf (&m_videoParameters, &request);
			if (status == MFX_ERR_NONE)
			{
				m_workerFrameSuggestedNumber = request.NumFrameSuggested;
			}

			status = m_pmfxDEC->Init (&m_videoParameters);
			if (status != MFX_ERR_NONE)
			{
				return false;
			}

			m_decoderInitialized = true;
		}
	}
	else if (!m_decoderInitialized)
	{
		*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
	}
	

	std::shared_ptr<IntelWorkerFrame> workFrame;
	//We will attempt to get a worker frame up to 30 times before bailing.
	const int MaxFrameRetries = 30;

	for (auto i = 0; i < MaxFrameRetries; i++)
	{
		std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
		if (!m_decoderInitialized)
		{
			*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
			return false;
		}

		if (m_workFrames.empty ())
		{
			if (outputFrameCount >= m_workerFrameSuggestedNumber)
			{
				Sleep (10);
				continue;
			}
			else
			{
				outputFrameCount++;
				workFrame = std::make_shared<IntelWorkerFrame> ();
			}
		}
		else
		{
			workFrame = m_workFrames.front ();
			m_workFrames.pop ();
		}

		if (!workFrame->m_frame->Data.Locked)
		{
			break;
		}
		else
		{
			m_workFrames.push (workFrame);
		}
	}
	if (!workFrame)
	{
		return false;
	}

	memcpy (&(workFrame->m_frame->Info), &(m_videoParameters.mfx.FrameInfo), sizeof (mfxFrameInfo));
	workFrame->m_frame->Data.Pitch = (mfxU16)MSDK_ALIGN32 (m_videoParameters.mfx.FrameInfo.Width);

	mfxFrameSurface1 *outputFrame;
	status = m_pmfxDEC->DecodeFrameAsync (&bitstream, workFrame->m_frame, &outputFrame, &workFrame->m_syncPoint);
	if (status == MFX_WRN_VIDEO_PARAM_CHANGED)
	{
		m_pmfxDEC->GetVideoParam (&m_videoParameters);
		status = m_pmfxDEC->DecodeFrameAsync (&bitstream, workFrame->m_frame, &outputFrame, &workFrame->m_syncPoint);
	}

	if (status == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM)
	{
		m_decoderInitialized = false;
		status = m_pmfxDEC->Close ();
		if (status != MFX_ERR_NONE)
		{
			std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
			m_workFrames.push (workFrame);
			return false;
		}

		status = m_pmfxDEC->DecodeHeader (&bitstream, &m_videoParameters);
		if (status != MFX_ERR_NONE)
		{
			std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
			m_workFrames.push (workFrame);
			return false;
		}

		m_videoParameters.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

		status = m_pmfxDEC->Query (&m_videoParameters, &m_videoParameters);
		if (status != MFX_ERR_NONE)
		{
			std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
			m_workFrames.push (workFrame);
			return false;
		}

		status = m_pmfxDEC->Init (&m_videoParameters);
		if (status != MFX_ERR_NONE)
		{
			std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
			m_workFrames.push (workFrame);
			return false;
		}
		m_decoderInitialized = true;
		bitstream.DataOffset = 0;
		status = m_pmfxDEC->DecodeFrameAsync (&bitstream, workFrame->m_frame, &outputFrame, &workFrame->m_syncPoint);
	}

	if (status != MFX_ERR_NONE)
	{
		*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
		std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
		m_workFrames.push (workFrame);
		return false;
	}

	status = m_mfxSession.SyncOperation (workFrame->m_syncPoint, 1000);
	if (status != MFX_ERR_NONE)
	{
		std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
		m_workFrames.push (workFrame);
		return false;
	}

	mfxDecodeStat stats;
	status = m_pmfxDEC->GetDecodeStat (&stats);
	if (status == MFX_ERR_NONE)
	{
		if (stats.NumError - m_stats.NumError != 0)
		{
			//We encountered an error so request a keyframe
			*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
		}
		m_stats = stats;

		if (workFrame->m_frame->Data.Corrupted >= MFX_CORRUPTION_MINOR)
		{
			//We encountered corruption so request a keyframe
			*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
		}
	}

	{
		std::lock_guard<std::recursive_mutex> lock (m_outputFramesLock);
		m_outputFrames.push (workFrame);
	}

	*flags |= IstiVideoDecoder::FLAG_FRAME_COMPLETE;	

	return true;
}


std::shared_ptr<IVideoDisplayFrame> IntelVideoDecoder::frameGet ()
{
	std::shared_ptr<IVideoDisplayFrame> outputFrame;
	{
		std::lock_guard<std::recursive_mutex> lock (m_outputFramesLock);
		if (!m_outputFrames.empty ())
		{
			outputFrame = m_outputFrames.front ();
			m_outputFrames.pop ();
		}
	}
	return outputFrame;
}

void IntelVideoDecoder::frameReturn (std::shared_ptr<IVideoDisplayFrame> returnFrame)
{
	std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
	m_workFrames.push (std::dynamic_pointer_cast<IntelWorkerFrame>(returnFrame));
}

bool IntelVideoDecoder::codecSet (EstiVideoCodec codec)
{
auto status = m_pmfxDEC->Close ();
m_decoderInitialized = false;
	switch (codec)
	{
		case estiVIDEO_H264:
			m_currentCodecId = MFX_CODEC_AVC;
			break;

		case estiVIDEO_H265:
			m_currentCodecId = MFX_CODEC_HEVC;
			break;

		default:
			m_videoParameters.mfx.CodecId = 0;
			stiAssert(false, "IntelVideoDecoder::codecSet Invalid codec set (%i)\n", (int)codec);
			return false;
	}
	m_videoParameters.mfx.CodecId = m_currentCodecId;
	return true;
}


void IntelVideoDecoder::clear ()
{
	auto status = m_pmfxDEC->Close ();
	m_decoderInitialized = false;

	std::lock(m_workFramesLock, m_outputFramesLock);
	while (!m_outputFrames.empty ())
	{
		m_workFrames.push (m_outputFrames.front ());
		m_outputFrames.pop ();
	}
	m_workFramesLock.unlock();
	m_outputFramesLock.unlock();
}

std::string IntelVideoDecoder::decoderDescription ()
{	
	std::string codecName = "unknown";
	switch (m_currentCodecId)
	{
		case MFX_CODEC_AVC:
			return "Intel_H264";
		case MFX_CODEC_HEVC:
			return "Intel_H265";
		default:
			return "Intel_Uninitialized_" + m_currentCodecId;
	}
}

vpe::DeviceSupport IntelVideoDecoder::supportGet (EstiVideoCodec codec, uint16_t profile, uint16_t level)
{
	mfxStatus status = MFX_ERR_NONE;
	mfxInitParam initParameters{ 0 };
	initParameters.Version.Major = 1;
	initParameters.Version.Minor = 0;
	initParameters.GPUCopy = true;

	initParameters.Implementation = MFX_IMPL_HARDWARE_ANY;
	MFXVideoSession session;
	status = session.InitEx (initParameters);
	if (status != MFX_ERR_NONE)
	{
		return vpe::DeviceSupport::Unavailable;
	}

	mfxIMPL implementation;
	status = MFXQueryIMPL (session, &implementation); // get the real implementation
	if (status != MFX_ERR_NONE)
	{
		return vpe::DeviceSupport::Unavailable;
	}

	if (implementation == MFX_IMPL_SOFTWARE)
	{
		return vpe::DeviceSupport::Software;
	}


	mfxVideoParam inputTest{ 0 };

	switch (codec)
	{
		case estiVIDEO_H264:
			inputTest.mfx.CodecId = MFX_CODEC_AVC;
			break;
		case estiVIDEO_H265:
			level = level / 3; //The rtp parameters for this specify it should be multiplied by 3 but intel does not do that.
			inputTest.mfx.CodecId = MFX_CODEC_HEVC;
			break;
		default:
			return vpe::DeviceSupport::Unavailable;
	}
	inputTest.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

	inputTest.mfx.CodecProfile = profile;
	inputTest.mfx.CodecLevel = level;
	inputTest.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
	inputTest.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
	inputTest.mfx.FrameInfo.CropX = 0;
	inputTest.mfx.FrameInfo.CropY = 0;
	inputTest.mfx.FrameInfo.CropW = IntelWorkerFrame::MaxFrameWidth;
	inputTest.mfx.FrameInfo.CropH = IntelWorkerFrame::MaxFrameHeight;
	inputTest.mfx.FrameInfo.Width = MSDK_ALIGN16 (IntelWorkerFrame::MaxFrameWidth);
	inputTest.mfx.FrameInfo.Height = MSDK_ALIGN16 (IntelWorkerFrame::MaxFrameHeight);

	inputTest.vpp.In.FourCC = MFX_FOURCC_NV12;
	inputTest.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
	inputTest.vpp.In.CropX = 0;
	inputTest.vpp.In.CropY = 0;
	inputTest.vpp.In.CropW = IntelWorkerFrame::MaxFrameWidth;
	inputTest.vpp.In.CropH = IntelWorkerFrame::MaxFrameHeight;
	inputTest.vpp.In.Width = MSDK_ALIGN16 (IntelWorkerFrame::MaxFrameWidth);
	inputTest.vpp.In.Height = MSDK_ALIGN16 (IntelWorkerFrame::MaxFrameHeight);

	mfxVideoParam outputTest{ inputTest };
	status = MFXVideoDECODE_Query (session, &inputTest, &outputTest);
	if (status == MFX_ERR_NONE && outputTest.mfx.CodecId == inputTest.mfx.CodecId)
	{
		session.Close ();
		return vpe::DeviceSupport::Hardware;
	}
	else
	{
		session.Close ();
		return vpe::DeviceSupport::Software;
	}
}