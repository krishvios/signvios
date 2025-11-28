#include "CstiAudio.h"

#include <algorithm>

bool debugAudioQueue = false;

extern bool getAndroidAudioFrame(SsmdAudioFrame * pAudioFrame);
extern void putAndroidAudioFrame(SsmdAudioFrame * pAudioFrame);

#define stiAudioTrace(...) if (debugAudioQueue) stiTrace(__VA_ARGS__)


#define CopySamples(dst, src, samples) memcpy(dst, src, samples/2)
#define MoveSamples(dst, src, samples) memmove(dst, src, samples/2)

extern void notifyAndroidOfAudio(CstiAudio *audio);

/*!
 * \brief default constructor
 */
CstiAudio::CstiAudio () : 
	m_audioOpened(false), 
	m_playbackEnabled(false), 
	m_recordEnabled(false),
	m_recordPrivacy(false),
	m_fdPSignal(NULL),
	m_audioSessionMutex(NULL),
	m_playbackMutex(NULL),
    m_recordMutex(NULL),
    m_bAudioOutEnabled(false)
{
}

static const unsigned char silence[] = { // one packet of silence in uLaw
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };

/*!
 * \brief Sets boolean to determine if we play recevied audio
 *
 * \return stiHResult
 */
stiHResult CstiAudio::AudioOut (EstiSwitch eSwitch)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	m_bAudioOutEnabled = eSwitch;
	
	return hResult;
}


/*!
 * \brief Initialize
 * Usually called once for object
 *
 * \return stiHResult
 */
stiHResult CstiAudio::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiResult eResult = estiOK;

    readyStateChangedSignalGet().Emit(true);

	if (NULL == m_fdPSignal)
	{
		eResult = stiOSSignalCreate (&m_fdPSignal);
	}
	stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);


	//stiOSSignalSet (m_fdSignal);
	stiOSSignalClear(m_fdPSignal);

	m_audioSessionMutex = stiOSMutexCreate();
	stiTESTCOND(m_audioSessionMutex != NULL, stiRESULT_ERROR);

	m_playbackMutex = stiOSMutexCreate();
	stiTESTCOND(m_playbackMutex != NULL, stiRESULT_ERROR);

	m_recordMutex = stiOSMutexCreate();
	stiTESTCOND(m_recordMutex != NULL, stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}

stiHResult CstiAudio::Uninitialize() {
	if (NULL != m_fdPSignal) {
		stiOSSignalClose(&m_fdPSignal);
		m_fdPSignal=NULL;
	}

	stiOSMutexDestroy(m_audioSessionMutex);
	stiOSMutexDestroy(m_playbackMutex);
	stiOSMutexDestroy(m_recordMutex);


	return stiRESULT_SUCCESS;
}


/*!
 * \brief Start audio playback
 * 
 * \return stiHResult 
 */
stiHResult CstiAudio::AudioPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

// lock both for start/stop operations
	CstiOSMutexLock threadSafe(m_audioSessionMutex);
	CstiOSMutexLock threadSafe2(m_playbackMutex);

	stiTrace ( "AUDIO OUT: AudioPlaybackStart\n" );

	if (m_playbackEnabled) goto STI_BAIL;
    

	m_playbackQueue.skip(m_playbackQueue.size());

	readyStateChangedSignalGet().Emit (true);
	
	m_playbackEnabled=true;

STI_BAIL:
	return hResult;
}

/*!
 * \brief Stops audio playback
 * 
 * \return stiHResult 
 */
stiHResult CstiAudio::AudioPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;	
	CstiOSMutexLock threadSafe(m_audioSessionMutex);
	
	stiTrace ( "AUDIO OUT: AudioPlaybackStop\n" );
	if (!m_playbackEnabled) goto STI_BAIL;
	
#if 0 // NOTE bug in VPE is calling stop/start then stop again at
	// TODO: Stop me.
#endif
	m_playbackEnabled=false;
	
//	if (!m_recordEnabled)
//		CloseAudio();

STI_BAIL:
	return hResult;
}

void CstiAudio::AudioFrameAvailable(const char *audioSendBuffer, int length) {
    CstiOSMutexLock threadSafe ( m_recordMutex );
    
    if (!m_recordEnabled) return;
    if (m_recordPrivacy) return;
    
    m_recordQueue.write(audioSendBuffer, length);
    if (m_recordQueue.size() > AUDIO_LATENCY_PACKETS) {
        m_recordQueue.skip( m_recordQueue.size() - AUDIO_LATENCY_PACKETS );
    }
    if (m_recordQueue.size() >= 240) {
        packetReadyStateSignalGet().Emit (true);
    }
}
        

/*!
 * \brief Sets codec for audio playback
 * 
 * \param EstiAudioCodec eVideoCodec 
 * One of the below 
 *	estiAUDIO_NONE
 *	estiAUDIO_G711_ALAW	  G.711 ALAW audio
 *	estiAUDIO_G711_MULAW  G.711 MULAW audio
 *	esmdAUDIO_G723_5	  G.723.5 compressed audio
 *	esmdAUDIO_G723_6	  G.723.6 compressed audio
 *  estiAUDIO_RAW		  Raw Audio (8000 hz, signed 16 bits, mono)
 *  
 *  \see EstiAudioCodec
 * 
 * \return stiHResult 
 */
stiHResult CstiAudio::AudioPlaybackCodecSet (
		EstiAudioCodec eVideoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (estiAUDIO_G711_MULAW != eVideoCodec)
		return stiRESULT_INVALID_CODEC;

	return hResult;
}

/*!
 * \brief Audio playback packet put 
 * 
 * \param SsmdAudioFrame* pAudioFrame 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudio::AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame)
{
    stiHResult hResult = stiRESULT_SUCCESS;
	CstiOSMutexLock threadSafe(m_playbackMutex);

#if 0
		// NOTE re-enable later when SIP calls don't stop the audio playback when they're not supposed to.
	if (m_playbackEnabled) {
#endif	
    if(m_bAudioOutEnabled){
        putAndroidAudioFrame(pAudioFrame);
    }
#if 0
	} else {
		stiTrace ( "AUDIO OUT: incoming audio packets but playback disabled.\n" );
	}
#endif
	
	return hResult;
}




/*!
 * \brief Gets the device file descriptor
 * 
 * \return int 
 */
int CstiAudio::GetDeviceFD ()
{
    return stiOSSignalDescriptor (m_fdPSignal);
}


#pragma mark Input Methods


stiHResult CstiAudio::AudioRecordStart () {

	stiHResult hResult = stiRESULT_SUCCESS;

	// lock both for start/stop operations
	CstiOSMutexLock threadSafe(m_audioSessionMutex);

	stiTrace ( "AUDIO IN: AudioRecordStart\n" );
	if (m_recordEnabled) goto STI_BAIL;
		
    notifyAndroidOfAudio(this);
	m_recordEnabled=true;

STI_BAIL:
	return hResult;
}

stiHResult CstiAudio::AudioRecordStop () {
	stiHResult hResult = stiRESULT_SUCCESS;	
	CstiOSMutexLock threadSafe(m_audioSessionMutex);
	
	stiTrace ( "AUDIO IN: AudioRecordStop\n" );
	if (!m_recordEnabled) goto STI_BAIL;
	
	notifyAndroidOfAudio(NULL);
	m_recordEnabled=false;
    
//	if (!m_playbackEnabled)
//		CloseAudio();

STI_BAIL:
	return hResult;
}
stiHResult CstiAudio::AudioRecordCodecSet (
	EstiAudioCodec eAudioCodec) {
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiAudio::AudioRecordPacketGet (
		SsmdAudioFrame * pAudioFrame) {
    	stiHResult hResult = stiRESULT_SUCCESS;
		
		CstiOSMutexLock threadSafe ( m_recordMutex );
				
		if (m_recordEnabled && m_recordQueue.size() >= 240)  {			
			m_recordQueue.read((char*)pAudioFrame->pun8Buffer, 240);
			pAudioFrame->unFrameSizeInBytes=240;
			if (debugAudioQueue) {
				uint8_t *buffer = pAudioFrame->pun8Buffer;
				stiTrace( "AUDIO IN GET: %02x%02x%02x%02x%02x%02x%02x%02x...\n", 
					buffer[0],
					buffer[1],
					buffer[2],
					buffer[3],
					buffer[4],
					buffer[5],
					buffer[6],
					buffer[7]	);
			}		
		} else {
			pAudioFrame->bIsSIDFrame=estiTRUE; // silence
			stiAudioTrace( "AUDIO IN GET: recordEnabled %d bytes available: %d\n", m_recordEnabled ? 1 : 0, m_recordQueue.size());
		}		
		if (m_recordQueue.size() < 240) {
            packetReadyStateSignalGet().Emit (false);
        }
		
STI_BAIL:
		
		return hResult;
}

stiHResult CstiAudio::AudioRecordSettingsSet (
	const SstiAudioRecordSettings * pAudioRecordSettings) {
	return stiRESULT_SUCCESS;
}
stiHResult CstiAudio::EchoModeSet (
	EsmdEchoMode eEchoMode) {
	return stiRESULT_SUCCESS;
}
stiHResult CstiAudio::PrivacySet (
	EstiBool bEnable) {
	bool enable = bEnable == estiTRUE ? true : false;
	aLOGD("Audio PrivacySet:%d", enable);
	if (m_recordPrivacy != enable) {
		audioPrivacySignalGet().Emit(enable);
	}
	m_recordPrivacy = enable;
	return stiRESULT_SUCCESS;
}
stiHResult CstiAudio::PrivacyGet (
	EstiBool *pbEnabled) const  {
	*pbEnabled = m_recordPrivacy ? estiTRUE : estiFALSE;
	return stiRESULT_SUCCESS;
}

int CstiAudio::GetDeviceFD () const {
    return 0;
}

void CstiAudio::DtmfReceived (EstiDTMFDigit eDtmfDigit)
{
    dtmfReceivedSignalGet().Emit(eDtmfDigit);
}

///\brief Reports to the IstiAudioInput object which codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.  If the preferred codec is to be returned, it should be placed in the
///\brief front of the list.
stiHResult CstiAudio::CodecsGet (
	std::list<EstiAudioCodec> *pCodecs,
	EstiAudioCodec ePreferredCodec)
{
	// Add the supported audio codecs to the list
	//	pCodecs->push_back (estiAUDIO_G722);

	pCodecs->push_front (estiAUDIO_G711_MULAW);

	return stiRESULT_SUCCESS;
}

///\brief Reports to IstiAudioOutput object which codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.  If the preferred codec is to be returned, it should be placed in the
///\brief front of the list.
stiHResult CstiAudio::CodecsGet (
	std::list<EstiAudioCodec> *pCodecs)
{
	pCodecs->push_back (estiAUDIO_G711_MULAW);
	return stiRESULT_SUCCESS;
};

	

