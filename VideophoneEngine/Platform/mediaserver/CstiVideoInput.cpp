#include "CstiVideoInput.h"
#include "CstiExtendedEvent.h"
#include "stiTrace.h"

extern "C" 
{
#include "libavformat/avformat.h"
}

#define stiMAX_MESSAGES_IN_QUEUE 12
#define stiMAX_MSG_SIZE 512
#define stiTASK_NAME "CstiVideoInput"
#define stiTASK_PRIORITY 151
#define stiSTACK_SIZE 4096
#define stiDEFAULT_FRAME_DURATION 33

stiMUTEX_ID CstiVideoInput::m_LockMutex = NULL; // Used to secure access to VideoInputList
std::unordered_map<uint32_t, std::shared_ptr<CstiVideoInput>> CstiVideoInput::m_videoInputs;
//
// Forward Declarations
//

stiEVENT_MAP_BEGIN(CstiVideoInput)
stiEVENT_DEFINE(estiEVENT_SHUTDOWN, CstiVideoInput::ShutdownHandle)
stiEVENT_MAP_END(CstiVideoInput)
stiEVENT_DO_NOW(CstiVideoInput)

CstiVideoInput::CstiVideoInput () : CstiOsTaskMQ(NULL, 0, (size_t)this, stiMAX_MESSAGES_IN_QUEUE, stiMAX_MSG_SIZE, stiTASK_NAME, stiTASK_PRIORITY, stiSTACK_SIZE),
m_bPrivacy(estiFALSE),
m_bIsRecording(estiFALSE),
m_nCallIndex(0)
{
}

CstiVideoInput::CstiVideoInput(uint32_t nCallIndex) : CstiOsTaskMQ(NULL, 0, (size_t)this, stiMAX_MESSAGES_IN_QUEUE, stiMAX_MSG_SIZE, stiTASK_NAME, stiTASK_PRIORITY, stiSTACK_SIZE),
m_bPrivacy(estiFALSE),
m_bIsRecording(estiFALSE),
m_nCallIndex(nCallIndex)
{
}

CstiVideoInput::~CstiVideoInput()
{
	std::swap(std::queue<VideoPacket*>(), m_qPackets); //Empty the queue
}

stiHResult CstiVideoInput::Initialize()
{
	static int filenumber = 0;
	stiHResult hResult = stiRESULT_SUCCESS;
	m_EncoderMutex = stiOSMutexCreate(); // NOTE check valid?
	m_QueueMutex = stiOSMutexCreate(); // NOTE check valid?
	return hResult;
}


/*! \brief Starts the shutdown process of the display task
 *
 * This method is responsible for doing all clean up necessary to gracefully
 * terminate the display task. Note that once this method exits,
 * there will be no more message received from the message queue since the
 * message queue will be deleted and the display task will go away.
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
stiHResult CstiVideoInput::ShutdownHandle(CstiEvent* poEvent)  // The event to be handled
{
	stiHResult hResult = stiRESULT_SUCCESS;
	// Call the base class method
	hResult = CstiOsTaskMQ::ShutdownHandle();
	
	stiOSMutexDestroy(m_QueueMutex);
	stiOSMutexDestroy(m_EncoderMutex);

	return (hResult);
}

bool CstiVideoInput::NextFrame(VideoPacket * packet)
{
	//We must be recording to start sending frames.
	if (m_bIsRecording)
	{
		//Thread Safe Queue
		CstiOSMutexLock threadSafe(m_QueueMutex);
		//Don't let the queue get too full.
		if (m_qPackets.size() < 5)
		{
			m_qPackets.push(packet);
			packetAvailableSignal.Emit ();
			return true;
		}
		return false;
	}
	return true;
}

stiHResult CstiVideoInput::VideoRecordStart()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_bIsRecording = estiTRUE;
	return hResult;
}


stiHResult CstiVideoInput::VideoRecordStop()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_bIsRecording = estiFALSE;
	return hResult;
}

EstiVideoCodec CstiVideoInput::VideoRecordCodecGet()
{
	switch (m_stVideoRecordSettings.codec)
	{
	case estiVIDEO_H263:
		return estiVIDEO_H263;
	case estiVIDEO_H264:
		return estiVIDEO_H264;
	default:
		return estiVIDEO_NONE;
	}
}

stiHResult CstiVideoInput::VideoRecordSettingsSet(const SstiVideoRecordSettings *pVideoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_stVideoRecordSettings = *pVideoRecordSettings;
	CstiVideoCallback::SetRecordSettings(m_nCallIndex, &m_stVideoRecordSettings);
	return hResult;
}

stiHResult CstiVideoInput::VideoRecordFrameGet(SstiRecordVideoFrame * pVideoFrame)
{
	//Thread Safe
	CstiOSMutexLock threadSafe(m_EncoderMutex);

	stiHResult hResult = stiRESULT_SUCCESS;

	//Make sure the VideoFrame has been allocated
	stiTESTCOND(pVideoFrame, stiRESULT_ERROR);

	// Clear out the pVideoFrame structure
	pVideoFrame->keyFrame = estiFALSE;
	pVideoFrame->frameSize = 0;

	//We can not process an empty queue
	stiTESTCOND(!m_qPackets.empty(), stiRESULT_PACKET_QUEUE_ERROR);

	//-----Place the Packet in the Queue into our Video Frame--------------
	VideoPacket* data;
	{
		//Thread Safe Queue
		CstiOSMutexLock threadSafe(m_QueueMutex);
		data = m_qPackets.front();
		m_qPackets.pop();
	}

	int MaxBufferSize = pVideoFrame->unBufferMaxSize;
	int FrameLength = data->Length;

	stiTESTCOND(FrameLength < MaxBufferSize, stiRESULT_MEMORY_ALLOC_ERROR);

	memcpy(pVideoFrame->buffer, data->Data, data->Length);
	pVideoFrame->frameSize = data->Length;
	pVideoFrame->keyFrame = data->IsKeyFrame ? estiTRUE : estiFALSE;
	pVideoFrame->eFormat = data->EndianFormat;
	//---------------------------------------------------------------------

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		// Since there was an error, reset packet information.
		pVideoFrame->frameSize = 0;
		pVideoFrame->keyFrame = estiFALSE;
	}

	return hResult;
}

stiHResult CstiVideoInput::PrivacySet(EstiBool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;
		videoPrivacySetSignal.Emit (bEnable);
	}

	return hResult;
}


stiHResult CstiVideoInput::PrivacyGet(EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pbEnabled = m_bPrivacy;

	//STI_BAIL:

	return hResult;
}


stiHResult CstiVideoInput::KeyFrameRequest()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

void CstiVideoInput::Lock()
{
	stiOSMutexLock(m_LockMutex);
}

void CstiVideoInput::Unlock()
{
	stiOSMutexUnlock(m_LockMutex);
}

/*! \brief Task function, executes until the task is shutdown.
 *
 * This task will loop continuously looking for messages in the message
 * queue.  When it receives a message, it will call the appropriate message
 * handling routine.
 *
 *  \retval Always returns 0
 */
int CstiVideoInput::Task ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32Buffer[(stiMAX_MSG_SIZE / sizeof (uint32_t)) + 1];

	//
	// Continue executing until shutdown
	//
	for (;;)
	{
		// read a message from the message queue
		hResult = MsgRecv(
			(char *)un32Buffer,
			stiMAX_MSG_SIZE,
			stiWAIT_FOREVER);

		// Was a message received?
		if (stiIS_ERROR(hResult))
		{
			Callback(estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
		}
		else
		{
			// Yes! Process the message.
			CstiEvent *poEvent = (CstiEvent*)un32Buffer;

			// Lookup and handle the event
			Lock();
			hResult = EventDoNow(poEvent);
			Unlock();

			if (stiIS_ERROR(hResult))
			{
				Callback(estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
			}

			if (estiEVENT_SHUTDOWN == poEvent->EventGet())
			{
				break;
			}
		} // end if
	} // end for

	return (0);

} // end CstiVPService::Task


/*!
* \brief Get video codec
*
* \param pCodecs - a std::list where the codecs will be stored.
*
* \return stiHResult
*/
stiHResult CstiVideoInput::VideoCodecsGet(std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
	pCodecs->push_back(estiVIDEO_H264);
	pCodecs->push_back(estiVIDEO_H263);

	return stiRESULT_SUCCESS;
}

/*!
* \brief Get Codec Capability
*
* \param eCodec
* \param pstCaps
*
* \return stiHResult
*/
stiHResult CstiVideoInput::CodecCapabilitiesGet(EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	switch (eCodec)
	{
	case estiVIDEO_H263:
		stProfileAndConstraint.eProfile = estiH263_ZERO;
		pstCaps->Profiles.push_back(stProfileAndConstraint);
		break;

	case estiVIDEO_H264:
	{
		SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;

		stProfileAndConstraint.eProfile = estiH264_BASELINE;
		stProfileAndConstraint.un8Constraints = 0xa0;
		pstH264Caps->Profiles.push_back(stProfileAndConstraint);

		pstH264Caps->eLevel = estiH264_LEVEL_4_1;

		break;
	}
	default:
		hResult = stiRESULT_ERROR;
	}

	return hResult;
}

void CstiVideoInput::instanceRelease ()
{
	m_videoInputs.erase (m_nCallIndex);
}

IstiVideoInput *IstiVideoInput::instanceCreate (uint32_t callIndex)
{
	auto inputInstance = std::make_shared<CstiVideoInput> (callIndex);
	inputInstance->Initialize ();
	inputInstance->Startup ();
	
	CstiVideoInput::m_videoInputs[callIndex] = inputInstance;
	CstiVideoCallback::AssociateInput (callIndex, inputInstance);
	return inputInstance.get();
}
