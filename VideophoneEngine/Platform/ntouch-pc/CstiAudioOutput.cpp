#include "CstiAudioOutput.h"
#include "stiTrace.h"

using namespace vpe::Audio;

CstiAudioOutput::CstiAudioOutput () :
	CstiEventQueue("CstiAudioOutput")
{
	m_fdPSignal = NULL;
}

stiHResult CstiAudioOutput::Uninitialize() 
{
	StopEventLoop ();
	if (NULL != m_fdPSignal) 
	{
		stiOSSignalClose(&m_fdPSignal);
		m_fdPSignal=NULL;
	}
	return stiRESULT_SUCCESS;
}

stiHResult CstiAudioOutput::Initialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (NULL == m_fdPSignal)
	{
		stiOSSignalCreate (&m_fdPSignal);			//TODO: get this EstiResult, and get the data from it...
	}
	stiOSSignalClear(m_fdPSignal);
	StartEventLoop ();
	return hResult;
}

stiHResult CstiAudioOutput::AudioPlaybackStart ()
{
	PostEvent (std::bind (&CstiAudioOutput::playbackStartEvent, this));
	return stiRESULT_SUCCESS;
}

stiHResult CstiAudioOutput::AudioPlaybackStop ()
{
	PostEvent (std::bind (&CstiAudioOutput::playbackStopEvent, this));
	return stiRESULT_SUCCESS;
}

stiHResult CstiAudioOutput::AudioPlaybackCodecSet (EstiAudioCodec audioCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiTESTCOND (audioCodec == estiAUDIO_G711_MULAW || audioCodec == estiAUDIO_G711_ALAW, stiRESULT_INVALID_CODEC);
	PostEvent (std::bind (&CstiAudioOutput::codecSetEvent, this, audioCodec));

STI_BAIL:
	return hResult;
}

///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel negotiations.
stiHResult CstiAudioOutput::CodecsGet(std::list<EstiAudioCodec> *pCodecs)
{
	pCodecs->push_back(estiAUDIO_G711_MULAW);
	pCodecs->push_back (estiAUDIO_G711_ALAW);	
	return stiRESULT_SUCCESS;
}

stiHResult CstiAudioOutput::AudioPlaybackPacketPut (SsmdAudioFrame * pAudioFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND_NOLOG (pAudioFrame, stiRESULT_INVALID_PARAMETER);

	PostEvent (std::bind (&CstiAudioOutput::packetPutEvent, this, pAudioFrame));

STI_BAIL:
	return hResult;
}

int CstiAudioOutput::GetDeviceFD ()
{
	return stiOSSignalDescriptor (m_fdPSignal);
}

void CstiAudioOutput::DtmfReceived (EstiDTMFDigit eDtmfDigit)
{
	dtmfReceivedSignal.Emit (eDtmfDigit);
	return;
}

void CstiAudioOutput::outputSettingsSet (int channels, int sampleRate, int sampleFormat, int alignment)
{
	PostEvent (std::bind (&CstiAudioOutput::outputSettingsSetEvent, this, channels, sampleRate, sampleFormat, alignment));
}

void CstiAudioOutput::playbackStartEvent ()
{
	ntouchPC::CstiNativeAudioLink::StartAudioPlayback ();
	readyStateChangedSignal.Emit (true);
}

void CstiAudioOutput::playbackStopEvent ()
{
	ntouchPC::CstiNativeAudioLink::StopAudioPlayback ();
}

void CstiAudioOutput::packetPutEvent (SsmdAudioFrame * pAudioFrame)
{
	Packet sourceBuffer;
	sourceBuffer.data = pAudioFrame->pun8Buffer;
	sourceBuffer.dataLength = pAudioFrame->unFrameSizeInBytes;
	sourceBuffer.maxLength = pAudioFrame->unBufferMaxSize;

	while (sourceBuffer.dataLength > 0)
	{
		BufferStatus flags = BufferStatus::None;
		auto destinationBuffer = std::make_shared<Packet> ();
		int bytesDecoded = m_audioDecoder.decode (sourceBuffer, destinationBuffer.get(), &flags);
		
		if (bytesDecoded > 0)
		{
			sourceBuffer.data += bytesDecoded;
			sourceBuffer.dataLength -= bytesDecoded;
			if ((flags & BufferStatus::FrameComplete) == BufferStatus::FrameComplete)
			{
				ntouchPC::CstiNativeAudioLink::AudioPlayback (destinationBuffer);
			}
		}
		else
		{
			break;
		}
	}

	//We don't want the source buffer data deleted so setting it to null.
	sourceBuffer.data = nullptr;
}

void CstiAudioOutput::codecSetEvent (EstiAudioCodec audioCodec)
{
	m_audioDecoder.init (audioCodec, g711ChannelCount, g711SampleRate);
}

void CstiAudioOutput::outputSettingsSetEvent (int channels, int sampleRate, int sampleFormat, int alignment)
{
	m_audioDecoder.outputSettingsSet (channels, sampleRate, sampleFormat, alignment);
}
