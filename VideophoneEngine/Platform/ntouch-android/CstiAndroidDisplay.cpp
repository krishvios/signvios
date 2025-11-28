//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved

#include "stiTrace.h"
#include "CstiAndroidDisplay.h"
#include <map>

#define SLICE_TYPE_NON_IDR	1
#define SLICE_TYPE_IDR		5


#define	stiVIDEODISPLAY_MAX_MESSAGES_IN_QUEUE 35
#define	stiVIDEODISPLAY_MAX_MSG_SIZE 512
#define	stiVIDEODISPLAY_TASK_NAME "CstiDisplayTask"
#define	stiVIDEODISPLAY_TASK_PRIORITY 151
#define	stiVIDEODISPLAY_STACK_SIZE 4096

#define DEFAULT_PLAYBACK_FRAME_SIZE 1024*7 // start with 7k buffers
#define MAX_PLAYBACK_FRAMES 30
#define START_PLAYBACK_FRAMES 5

extern JNIEnv *getJNIEnvFromDirector(void *ptr);

class CstiAndroidVideoPlaybackFrame : public IstiVideoPlaybackFrame
{
public:
	
	CstiAndroidVideoPlaybackFrame (CstiAndroidDisplay *display)
	{
		++m_un32BufferCount;
        m_pDisplay = display;
		m_pBuffer = display->AllocateBuffer( DEFAULT_PLAYBACK_FRAME_SIZE, &m_n32ID );
		m_un32DataSize = 0; //DEFAULT_PLAYBACK_FRAME_SIZE;
		m_un32BufferSize = DEFAULT_PLAYBACK_FRAME_SIZE;
		m_bFrameIsComplete = false;
		m_bFrameIsKeyframe = false;
		stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
			stiTrace ( "Created new playback buffer.  Buffers: %d\n", m_un32BufferCount );
		);
        mapped_frames[m_n32ID] = this;
	}
	
	~CstiAndroidVideoPlaybackFrame ()
	{
        mapped_frames.erase(mapped_frames.find(m_n32ID));
		if (m_pBuffer)
		{
			m_pDisplay->FreeBuffer(m_pBuffer, m_n32ID);
			m_pBuffer = NULL;
		}
		--m_un32BufferCount;
		stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
			stiTrace ( "Removed a playback buffer.  Buffers: %d\n", m_un32BufferCount );
		);
	}
	
	
	virtual uint8_t *BufferGet ()	// pointer to the video packet data
	{
        return m_pBuffer;
	}
	
	
	virtual uint32_t BufferSizeGet () const	// size of this buffer in bytes
	{
		return m_un32BufferSize;
	}
	
	virtual stiHResult BufferSizeSet (uint32_t un32BufferSize)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		if (un32BufferSize > m_un32BufferSize)
		{
			// Allocate new memory.
			uint8_t *pNewBuffer = m_pDisplay->ReallocateBuffer( un32BufferSize, m_n32ID, &m_n32ID );

			stiTESTCOND_NOLOG (pNewBuffer, stiRESULT_MEMORY_ALLOC_ERROR);

			mapped_frames[m_n32ID] = this;
			m_pBuffer = pNewBuffer;
			m_un32BufferSize = un32BufferSize;
		}
		
	STI_BAIL:

		return hResult;
	}
	
	virtual uint32_t DataSizeGet () const		// Number of bytes in the buffer.
	{
		return m_un32DataSize;
	}
	
	
	virtual stiHResult DataSizeSet (
									uint32_t un32DataSize)
	{
        stiHResult hResult = stiRESULT_SUCCESS;
		m_un32DataSize = un32DataSize;
		stiASSERT(m_un32DataSize <= m_un32BufferSize);
		return hResult;
	}

	virtual bool FrameIsCompleteGet () { return m_bFrameIsComplete; }
	virtual void FrameIsCompleteSet (bool bFrameIsComplete) { m_bFrameIsComplete = bFrameIsComplete; }
	virtual void FrameIsKeyframeSet (bool bFrameIsKeyframe) { m_bFrameIsKeyframe = bFrameIsKeyframe; }
	virtual bool FrameIsKeyframeGet () { return m_bFrameIsKeyframe; }

	virtual uint32_t IDGet () const		// ID For finding the direct byte buffer in java.
	{
		return m_n32ID;
	}
    
    static CstiAndroidVideoPlaybackFrame* getByID(int32_t id) { return mapped_frames[id]; }
	
	static uint32_t CreatedGet() {
		return m_un32BufferCount;
	}

private:
	
	uint8_t *m_pBuffer;
	uint32_t m_un32BufferSize;
	uint32_t m_un32DataSize;
    int32_t  m_n32ID;
    CstiAndroidDisplay *m_pDisplay;
	static uint32_t m_un32BufferCount;
	bool m_bFrameIsComplete;
	bool m_bFrameIsKeyframe;
    static std::map<int32_t, CstiAndroidVideoPlaybackFrame*> mapped_frames;
};

uint32_t CstiAndroidVideoPlaybackFrame::m_un32BufferCount=0;
std::map<int32_t, CstiAndroidVideoPlaybackFrame*> CstiAndroidVideoPlaybackFrame::mapped_frames;


stiEVENT_MAP_BEGIN (CstiAndroidDisplay)
	stiEVENT_DEFINE (estiVP_DELIVER_FRAME, CstiAndroidDisplay::EventDeliverFrame)
stiEVENT_MAP_END (CstiAndroidDisplay)
stiEVENT_DO_NOW (CstiAndroidDisplay)

CstiAndroidDisplay::CstiAndroidDisplay ()
{
}

CstiAndroidDisplay::~CstiAndroidDisplay() {
 stiOSMutexDestroy(m_DecoderMutex);
 FreeFrames(0);
}

stiHResult CstiAndroidDisplay::InitDecoder()
{
    if (HardwareCodecSupportedGet() == estiFALSE) {
        CstiSoftwareDisplay::InitDecoder();
    }
    // Currently no setup.  Should we do a setup?
    return stiRESULT_SUCCESS;
}

IstiVideoPlaybackFrame *CstiAndroidDisplay::AllocateVideoPlaybackFrame()
{
    if (HardwareCodecSupportedGet() == estiFALSE) {
        return CstiSoftwareDisplay::AllocateVideoPlaybackFrame();
    }
    return new CstiAndroidVideoPlaybackFrame(this);
}

stiHResult CstiAndroidDisplay::VideoPlaybackCodecSet (
	EstiVideoCodec eVideoCodec)
{

	switch (eVideoCodec) {
		case estiVIDEO_H264:
			m_eVideoCodec = eVideoCodec;
			break;
		default:
			return stiRESULT_INVALID_CODEC;
	}
	return stiRESULT_SUCCESS;
}


stiHResult CstiAndroidDisplay::VideoPlaybackStart ()
{
    if (HardwareCodecSupportedGet() == estiFALSE) {
        return CstiSoftwareDisplay::VideoPlaybackStart();
    }
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiOSMutexLock thread_safe(m_DecoderMutex);
	stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
		stiTrace ( "VideoPlaybackStart\n" );
	);
	
	// TODO: Setup java by callback.

	gettimeofday(&m_lastIFrame,NULL);
	m_decoding=true;
STI_BAIL:
	return hResult;
}


stiHResult CstiAndroidDisplay::VideoPlaybackStop ()
{
    if (HardwareCodecSupportedGet() == estiFALSE) {
        return CstiSoftwareDisplay::VideoPlaybackStop();
    }
    
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiOSMutexLock thread_safe(m_DecoderMutex);
    stiDEBUG_TOOL (g_stiVideoPlaybackDebug,
		stiTrace ( "VideoPlaybackStop\n" );
	);

	// TODO: Teardown java callback.

	m_decoding=false;

    std::lock_guard<std::recursive_mutex> lock_safe ( m_LockMutex );
	FreeFrames(START_PLAYBACK_FRAMES);

	return hResult;
}
	

stiHResult CstiAndroidDisplay::VideoPlaybackFramePut (
	IstiVideoPlaybackFrame *pVideoFrame)
{
    if (HardwareCodecSupportedGet() == estiFALSE) {
        return CstiSoftwareDisplay::VideoPlaybackFramePut(pVideoFrame);
    }
    stiHResult hResult = stiRESULT_SUCCESS;
	
    CstiAndroidVideoPlaybackFrame *pFrame = (CstiAndroidVideoPlaybackFrame *)pVideoFrame;
	uint32_t un32BufferLength = pFrame->DataSizeGet();
	//static uint8_t *pTmp = p//new uint8_t[704*480*3/2];// TMP
	//memcpy(pTmp, pFrame->BufferGet(), un32BufferLength);
	uint8_t *pBuffer = pFrame->BufferGet();
    
    switch (m_eVideoCodec)
	{
		case estiVIDEO_H264:
		{
			uint32_t un32SliceSize;
			//uint32_t un8SliceType;
			//stiTrace ( "Received an H264 frame buffer len: %d.\n", un32BufferLength);
			
            uint32_t un32InitialLength = un32BufferLength;
			while (m_convertNalUnits && un32BufferLength > 5)
			{
				un32SliceSize = *(uint32_t *)pBuffer;
                if (m_swapSizeBytes) {
                    un32SliceSize = stiDWORD_BYTE_SWAP (un32SliceSize);
                }
                
				pBuffer[0] = 0;
				pBuffer[1] = 0;
				pBuffer[2] = 0;
				pBuffer[3] = 1;
				
				pBuffer += sizeof (uint32_t) + un32SliceSize;
				un32BufferLength -= sizeof (uint32_t) + un32SliceSize;
                
                if (un32BufferLength > un32InitialLength)
                {
                    // Bad data, bail.
                    hResult = stiRESULT_ERROR;
                    break;
                }
			}
            
            if (hResult == stiRESULT_SUCCESS)
            {
                CstiAndroidVideoPlaybackFrame *pFrame = (CstiAndroidVideoPlaybackFrame *)pVideoFrame;
                hResult = VideoPlaybackFramePutJava(pFrame->IDGet(), pFrame->FrameIsKeyframeGet(), un32InitialLength);
            }
			break;
		}

		default:
			
			stiASSERT (estiFALSE);
			
			break;
	}
    
	
	return hResult;
}

stiHResult CstiAndroidDisplay::VideoPlaybackFramePutJava (
    int32_t outId,
    bool isKeyFrame,
    uint32_t dataSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
    return stiRESULT_SUCCESS;
}


stiHResult CstiAndroidDisplay::VideoPlaybackFrameDiscard (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

    std::lock_guard<std::recursive_mutex> thread_safe ( m_LockMutex );

    if (m_decoding) {
		m_availableFrames.push(pVideoFrame);
	} else {
		delete (CstiAndroidVideoPlaybackFrame*)pVideoFrame;
	}
	
	return hResult;
}

stiHResult CstiAndroidDisplay::VideoPlaybackFrameDiscard (
    int bufferId)
{
    stiHResult hResult = stiRESULT_SUCCESS;
    
    std::lock_guard<std::recursive_mutex> thread_safe ( m_LockMutex );

    CstiAndroidVideoPlaybackFrame* pVideoFrame = CstiAndroidVideoPlaybackFrame::getByID(bufferId);
    if (m_decoding) {
        m_availableFrames.push(pVideoFrame);
	} else {
		delete (CstiAndroidVideoPlaybackFrame*)pVideoFrame;
	}
    
    return hResult;
}

extern int g_displayWidth;
extern int g_displayHeight;


///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.  If the preferred codec is to be returned, it should be placed in the
///\brief front of the list.
stiHResult CstiAndroidDisplay::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
    pCodecs->push_back(estiVIDEO_H264);

	return stiRESULT_SUCCESS;
}


///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.

extern int g_H264Profile;
extern int g_H264Level;
extern int g_H264MaxMBPS;
extern int g_H264MaxFS;


#if DEVICE == DEV_X86
void CstiAndroidDisplay::ScreenshotTake (EstiScreenshotType eType)
{
}
#endif

///\brief Allows Java to override and allocate a direct byte buffer
///\brief instead.
uint8_t* CstiAndroidDisplay::ReallocateBuffer (uint32_t bytes, uint32_t inId, int32_t *outId)
{
    // Note: This is overridden by Java to provide a direct ByteBuffer.
    
    CstiAndroidVideoPlaybackFrame* pVideoFrame = CstiAndroidVideoPlaybackFrame::getByID(inId);
    return (uint8_t*)realloc ( pVideoFrame->BufferGet(), bytes );
}

uint8_t* CstiAndroidDisplay::AllocateDirectByteBuffer (uint32_t bytes, int32_t *outId)
{
    aLOGD("CstiAndroidDisplay::AllocateDirectByteBuffer");
    
    return NULL;
}

///\brief Allows Java to override and allocate a direct byte buffer
///\brief instead.
uint8_t* CstiAndroidDisplay::AllocateBuffer (uint32_t bytes, int32_t *outId)
{
    aLOGD("CstiAndroidDisplay::AllocateBuffer");
    
    uint8_t* byteBuffer = AllocateDirectByteBuffer(bytes, outId);

    if (byteBuffer != NULL) {
      return byteBuffer;
    }
    
    *outId = -1;
    return (uint8_t*)malloc ( bytes );
}

void CstiAndroidDisplay::FreeBuffer(uint8_t* buffer, int bufferId)
{
    if (bufferId == -1) {
        free(buffer);
    }
}

///\brief Method that allow Java to hand us h264 flags.
void CstiAndroidDisplay::HandleFlags(uint8_t un8Flags) {
    if (un8Flags & IstiVideoDecoder::FLAG_KEYFRAME) {
		gettimeofday(&m_lastIFrame,NULL);
	}

    timeval current_time;
	gettimeofday(&current_time,NULL);
	time_t iFrameDelta = current_time.tv_sec - m_lastIFrame.tv_sec;

	if (((un8Flags & IstiVideoDecoder::FLAG_IFRAME_REQUEST)
         && iFrameDelta >= MIN_IFRAME_REQUEST_DELTA)
        || iFrameDelta >= MAX_IFRAME_REQUEST_DELTA)
	{
        keyframeNeededSignalGet().Emit ();
		stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
                       stiTrace ( "\n\nIFRAME REQUEST. %d seconds.\n", iFrameDelta );
                       );
	}
}

void CstiAndroidDisplay::HandleKeyframeRequest(){
    keyframeNeededSignalGet().Emit ();
	stiTrace ( "\n\nIFRAME REQUEST.\n");
}

void CstiAndroidDisplay::PreferredDisplaySizeGet(unsigned int *displayWidth, unsigned int *displayHeight) const{
    if(displayWidth != nullptr){
        *displayWidth = g_displayWidth;
    }
    if(displayHeight != nullptr){
        *displayHeight = g_displayHeight;
    }
}
