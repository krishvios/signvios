#include "CstiAudio.h"

#include <algorithm>

#include "NSApplication+SystemVersion.h"

#include "SCIDefaults.h"

bool debugAudioQueue=false;

#define stiAudioTrace(...) if (debugAudioQueue) stiTrace(__VA_ARGS__)


#define CopySamples(dst, src, samples) memcpy(dst, src, samples/2)
#define MoveSamples(dst, src, samples) memmove(dst, src, samples/2)


/*!
 * \brief default constructor
 */
CstiAudio::CstiAudio () : 
	m_audioOpened(false), 
	m_playbackEnabled(false), 
	m_recordEnabled(false),
	m_recordPrivacy(false),
	m_audioSessionMutex(NULL),
	m_playbackMutex(NULL),
	m_recordMutex(NULL),
	mInputDevID(kAudioObjectUnknown),
	mOutputDevID(kAudioObjectUnknown),
	mAudioComponent(NULL),
	mAudioUnit(NULL),
	m_bAudioOutEnabled(false)
{
		
		muLawFormat.mSampleRate = 8000;
		muLawFormat.mFormatID = kAudioFormatULaw;
		muLawFormat.mFormatFlags = 0; // no flags
		muLawFormat.mBytesPerPacket = 1;
		muLawFormat.mFramesPerPacket = 1;
		muLawFormat.mBytesPerFrame = 1;
		muLawFormat.mChannelsPerFrame = 1;
		muLawFormat.mBitsPerChannel = 8;

}

static uint8_t uLaw2pcmBuffer[8000]; 
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


OSStatus CstiAudioInputuLaw2PCM(AudioConverterRef             inAudioConverter,
								UInt32                        *ioNumberDataPackets,
								AudioBufferList               *ioData,
								AudioStreamPacketDescription  **outDataPacketDescription,
								void                          *inUserData)
{
	CstiAudio *self = static_cast<CstiAudio*>(inUserData);
	
	CstiOSMutexLock threadSafe ( self->m_playbackMutex );
	
	ioData->mNumberBuffers=1;
	ioData->mBuffers[0].mNumberChannels = 1;
	
	uint32_t availableBytes;
	char *buffer = (char*)TPCircularBufferTail(&self->m_playbackBuffer, &availableBytes);
	while (availableBytes <= *ioNumberDataPackets ) {
		TPCircularBufferProduceBytes(&self->m_playbackBuffer, reinterpret_cast<const char* >(silence), sizeof(silence));
		buffer = (char*)TPCircularBufferTail(&self->m_playbackBuffer, &availableBytes);
	}
	
	memcpy((char*)uLaw2pcmBuffer, buffer, availableBytes);
	TPCircularBufferConsume(&self->m_playbackBuffer, availableBytes);
	ioData->mBuffers[0].mData = uLaw2pcmBuffer;
	ioData->mBuffers[0].mDataByteSize = availableBytes;
	*ioNumberDataPackets = availableBytes;
	
	return noErr;
}



OSStatus CstiAudioOuputBusCallback(void                      *inRefCon,
                                         AudioUnitRenderActionFlags *ioActionFlags,
                                         const AudioTimeStamp       *inTimeStamp,
                                         UInt32                      inBusNumber,
                                         UInt32                      inNumberFrames,
                                         AudioBufferList            *ioData)
{	
	CstiAudio *self = static_cast<CstiAudio*>(inRefCon);
	
		
	OSStatus error = AudioConverterFillComplexBuffer(self->muLaw2PCM,
													 CstiAudioInputuLaw2PCM,
													 self,
													 &inNumberFrames,  /* input capacity in packets output number of packets converted */
													 ioData,
													 nil);

  
	return error;

}
	
OSStatus CstiAudioInputPCM2uLaw(AudioConverterRef             inAudioConverter,
					  UInt32                        *ioNumberDataPackets,
					  AudioBufferList               *ioData,
					  AudioStreamPacketDescription  **outDataPacketDescription,
					  void                          *inUserData)
{
		CstiAudio *self = static_cast<CstiAudio*>(inUserData);
	
    if (*ioNumberDataPackets > self->mPCM2uLawBuffer.mCount)
			*ioNumberDataPackets = self->mPCM2uLawBuffer.mCount;
			
		if (*ioNumberDataPackets == 0) return -1; // out of data but not end of stream (we never reach end of stream)
	
    ioData->mNumberBuffers = 1;
    ioData->mBuffers[0].mNumberChannels = self->mStreamFormat.mChannelsPerFrame;
    ioData->mBuffers[0].mData = self->mPCM2uLawBufferPtr;
    ioData->mBuffers[0].mDataByteSize = *ioNumberDataPackets * self->mStreamFormat.mBytesPerPacket;
		
		self->mPCM2uLawBufferPtr += *ioNumberDataPackets * self->mStreamFormat.mBytesPerPacket;
		self->mPCM2uLawBuffer.mCount -= *ioNumberDataPackets;
	
    return noErr;
}


OSStatus CstiAudioInputBusCallback(void                       *inRefCon,
                                        AudioUnitRenderActionFlags *ioActionFlags,
                                        const AudioTimeStamp       *inTimeStamp,
                                        UInt32                      inBusNumber,
                                        UInt32                      inNumberFrames,
                                        AudioBufferList            *ioData) {
					

  CstiAudio *pAudio = (CstiAudio*)inRefCon;
  AudioBufferList abl = pAudio->mAudioBuffer;
	abl.mNumberBuffers=1;
	abl.mBuffers[0].mNumberChannels=pAudio->mStreamFormat.mChannelsPerFrame;
	abl.mBuffers[0].mData=NULL;
	abl.mBuffers[0].mDataByteSize = inNumberFrames * pAudio->mStreamFormat.mBytesPerFrame;

	stiAudioTrace("AUDIO IN: Render incoming audio...\n" );
	OSStatus status = AudioUnitRender(pAudio->mAudioUnit,
	                                  ioActionFlags,
	                                  inTimeStamp,
	                                  inBusNumber,
	                                  inNumberFrames,
	                                  &abl);
	
	
	if (0 == status) {
		char* buffer = (char*)abl.mBuffers[0].mData;
		UInt32 bufferSize = abl.mBuffers[0].mDataByteSize;


		CstiOSMutexLock threadSafe ( pAudio->m_recordMutex );
		if (!pAudio->m_recordEnabled) return 0;
		if (!pAudio->m_recordPrivacy) {
		
			// source input
			pAudio->mPCM2uLawBuffer.mCount = bufferSize / pAudio->mStreamFormat.mBytesPerPacket;
			pAudio->mPCM2uLawBufferPtr = (uint8_t*)buffer;
		
			while(1)
			{
				// wrap the destination buffer in an AudioBufferList
				// convert recorded pcm to uLaw
				static uint8_t* pcm2uLawBuffer[8000];
				
				AudioBufferList uLaw;
				uLaw.mNumberBuffers=1;
				uLaw.mBuffers[0].mNumberChannels = 1;
				uLaw.mBuffers[0].mDataByteSize = 8000;
				uLaw.mBuffers[0].mData = pcm2uLawBuffer;
				
				
				UInt32 ioOutputDataPackets = 8000;
				OSStatus error = AudioConverterFillComplexBuffer(pAudio->mPCM2uLaw,
																 CstiAudioInputPCM2uLaw,
																 pAudio,
																 &ioOutputDataPackets,  /* input capacity in packets output number of packets converted */
																 &uLaw,
																 nil);
				if ((error && error != -1) || !ioOutputDataPackets)
				{
					//		fprintf(stderr, "err: %ld, packets: %ld\n", err, ioOutputDataPackets);
					break;	// this is our termination condition
				}

				TPCircularBufferProduceBytes(&pAudio->m_recordBuffer, (const char*)uLaw.mBuffers[0].mData, ioOutputDataPackets);
				if (debugAudioQueue) {
						UInt8* buffer = (UInt8*)uLaw.mBuffers[0].mData;
						stiTrace( "Input Audio Buffer: %02x%02x%02x%02x%02x%02x%02x%02x... Total buffer size: %d\n",
							buffer[0],
							buffer[1],
							buffer[2],
							buffer[3],
							buffer[4],
							buffer[5],
							buffer[6],
							buffer[7], uLaw.mBuffers[0].mDataByteSize);
				}
			}
		} else {
			TPCircularBufferProduceBytes(&pAudio->m_recordBuffer, reinterpret_cast<const char* >(silence), sizeof(silence) );
		}

		uint32_t availableBytes;
		TPCircularBufferTail(&pAudio->m_recordBuffer, &availableBytes);
		if (availableBytes > AUDIO_LATENCY_PACKETS) {
			TPCircularBufferConsume(&pAudio->m_recordBuffer, availableBytes - AUDIO_LATENCY_PACKETS);
			TPCircularBufferTail(&pAudio->m_recordBuffer, &availableBytes);
		}
		if (availableBytes >= 240) {
			pAudio->packetReadyStateSignalGet ().Emit (true);
		}
	} else {
		stiTrace ( "Bad Render Status: %08x\n", status );
		return status;
	}
	return 0;
}

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

	packetReadyStateSignalGet ().Emit (false);
	readyStateChangedSignalGet ().Emit (false);

	m_audioSessionMutex = stiOSMutexCreate();
	stiTESTCOND(m_audioSessionMutex != NULL, stiRESULT_ERROR);
	
	m_playbackMutex = stiOSMutexCreate();
	stiTESTCOND(m_playbackMutex != NULL, stiRESULT_ERROR);
	
	m_recordMutex = stiOSMutexCreate();
	stiTESTCOND(m_recordMutex != NULL, stiRESULT_ERROR);
	
	TPCircularBufferInit(&m_playbackBuffer, 9000);
	TPCircularBufferInit(&m_recordBuffer, 9000);
	
STI_BAIL:
	return hResult;
}

#define CheckStatus(stmt) status = (stmt); stiTESTCOND(status == noErr, stiRESULT_ERROR)

AudioDeviceID CstiAudio::FindAudioDeviceByUUID(const char* uuid) {

	OSStatus status;
	stiHResult hResult;
	AudioObjectPropertyAddress theAddress;
	UInt32 propertySize, nDevices;
	AudioDeviceID *devids=NULL, devid=kAudioObjectUnknown;

	theAddress.mSelector = kAudioHardwarePropertyDevices;
	theAddress.mScope    = kAudioObjectPropertyScopeGlobal;
	theAddress.mElement  = kAudioObjectPropertyElementMaster;
	
	CheckStatus(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, NULL, &propertySize));
	nDevices = propertySize / sizeof(AudioDeviceID);
	devids = new AudioDeviceID[nDevices];
	CheckStatus(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propertySize, devids));
			
	for (int i = 0; i < nDevices; ++i) {
		CFStringRef     uidString;
		propertySize = sizeof(uidString);
		theAddress.mSelector = kAudioDevicePropertyDeviceUID;
		theAddress.mScope = kAudioObjectPropertyScopeGlobal;
		theAddress.mElement = kAudioObjectPropertyElementMaster;
		if (AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propertySize, &uidString) == noErr) {
			 CFStringRef desiredUuid = CFStringCreateWithCString(NULL, uuid, kCFStringEncodingUTF8);
			 bool equal=CFStringCompare( uidString, desiredUuid, 0)==0;
			 CFRelease(desiredUuid);
			 CFRelease(uidString);
			 if (equal) {
					devid=devids[i];
					break;
			 }
		}
	}
STI_BAIL:
  if(devids) delete [] devids;
	return devid;
}

stiHResult CstiAudio::OpenAudio() {

	stiHResult hResult = stiRESULT_SUCCESS;

	OSStatus status = noErr;
	char dUUId[512];
	
	AudioDeviceID defaultDevice = kAudioObjectUnknown;
	UInt32 propertySize;
	AudioObjectPropertyAddress theAddress;
	AudioComponentDescription desc = {0};

	if (m_audioOpened)
		goto STI_BAIL;
	
	stiTrace ( "AUDIO OUT: AudioOpen\n" );
	
	
	mStreamFormat.mFormatID = kAudioFormatLinearPCM;
	// mBitsPerChannel and mSampleRate are required for stream format on voip unit or it will fail with unsupported format
	// Canonical format flag was required before it was deprecated, but can now be replaced with native float packed
	mStreamFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
	mStreamFormat.mBitsPerChannel = 32;
	mStreamFormat.mChannelsPerFrame = 1;
	mStreamFormat.mBytesPerFrame = mStreamFormat.mChannelsPerFrame * (mStreamFormat.mBitsPerChannel / 8);
	mStreamFormat.mFramesPerPacket = 1;
	mStreamFormat.mBytesPerPacket = mStreamFormat.mBytesPerFrame * mStreamFormat.mFramesPerPacket;
	mStreamFormat.mSampleRate = 44100; // this will be set from the hardware sample rate
	
	
	// get the audio component	
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = desc.componentFlagsMask = 0;
	mAudioComponent = AudioComponentFindNext(NULL, &desc);
	if (mAudioComponent == NULL)
	{
		hResult = stiRESULT_ERROR;
		goto STI_BAIL;
	}
	

	// Obtain the device ID:
	
	if (!getUserAudioDeviceUUID(dUUId, sizeof(dUUId),true)) {
		mInputDevID=FindAudioDeviceByUUID(dUUId);
	}
	if (!getUserAudioDeviceUUID(dUUId, sizeof(dUUId),false)) {
		mOutputDevID=FindAudioDeviceByUUID(dUUId);
	}
	
	if (mInputDevID==kAudioObjectUnknown) {
		
		propertySize = sizeof(defaultDevice);
		theAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;
		theAddress.mScope = kAudioObjectPropertyScopeGlobal;
		theAddress.mElement = kAudioObjectPropertyElementMaster;
		
		CheckStatus(AudioObjectGetPropertyData(kAudioObjectSystemObject,
											 &theAddress,
											 0,
											 NULL,
											 &propertySize,
											 &mInputDevID));
	}
	
	if (mOutputDevID==kAudioObjectUnknown) {
		
		propertySize = sizeof(defaultDevice);
		theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
		theAddress.mScope = kAudioObjectPropertyScopeGlobal;
		theAddress.mElement = kAudioObjectPropertyElementMaster;
		
		CheckStatus(AudioObjectGetPropertyData(kAudioObjectSystemObject,
											 &theAddress,
											 0,
											 NULL,
											 &propertySize,
											 &mOutputDevID));
	}	
	
	hResult = CreateAudioUnit();
	stiTESTRESULT();

	m_audioOpened=true;
	
STI_BAIL:
	if (status != 0) {
		stiTrace ( "Failed Status %08x\n", status );
	}
  return hResult;
}

void CstiAudio::CloseAudio() {
	
	if (mAudioUnit) {
		AudioUnitUninitialize(mAudioUnit);
		AudioComponentInstanceDispose(mAudioUnit);
		mAudioUnit=NULL;
	}
	
	if (muLaw2PCM) {
		AudioConverterDispose(muLaw2PCM);
		muLaw2PCM=NULL;
	}
	if (mPCM2uLaw) {
		AudioConverterDispose(mPCM2uLaw);
		mPCM2uLaw=NULL;
	}


	mAudioComponent = NULL;
	m_audioOpened=false;

}

void CstiAudio::Reinit() {
  CstiOSMutexLock threadSave ( m_audioSessionMutex);
	if (m_playbackEnabled || m_recordEnabled) {
		CloseAudio();
		OpenAudio();
		AudioOutputUnitStart(mAudioUnit);
	}
}


stiHResult CstiAudio::Uninitialize() {
	stiOSMutexDestroy(m_audioSessionMutex);
	stiOSMutexDestroy(m_playbackMutex);
	stiOSMutexDestroy(m_recordMutex);

	TPCircularBufferCleanup(&m_playbackBuffer);
	TPCircularBufferCleanup(&m_recordBuffer);

	return stiRESULT_SUCCESS;
}


stiHResult CstiAudio::CreateAudioUnit()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = noErr;
	AudioStreamBasicDescription deviceFormat;
	UInt32 size;
	AURenderCallbackStruct input_cb, output_cb;
	
	/* Create an audio unit to interface with the device */
	CheckStatus(AudioComponentInstanceNew(mAudioComponent, &mAudioUnit));
	
	/* Set audio unit's properties for capture device */
	
	/* Enable input */
	CheckStatus(AudioUnitSetProperty(mAudioUnit,
									 kAudioOutputUnitProperty_CurrentDevice,
									 kAudioUnitScope_Global,
									 1,
									 &mInputDevID,
									 sizeof(mInputDevID)));
	
	
	size = sizeof(AudioStreamBasicDescription);
	CheckStatus(AudioUnitGetProperty(mAudioUnit,
									 kAudioUnitProperty_StreamFormat,
									 kAudioUnitScope_Input,
									 1,
									 &deviceFormat,
									 &size));
	mStreamFormat.mSampleRate = deviceFormat.mSampleRate;
	
	
	CheckStatus(AudioUnitSetProperty(mAudioUnit,
									 kAudioOutputUnitProperty_CurrentDevice,
									 kAudioUnitScope_Global,
									 0,
									 &mOutputDevID,
									 sizeof(mOutputDevID)));
	
	
	
	CheckStatus(AudioUnitSetProperty(mAudioUnit,
									 kAudioUnitProperty_StreamFormat,
									 kAudioUnitScope_Output,
									 1,
									 &mStreamFormat,
									 sizeof(mStreamFormat)));
	
	CheckStatus(AudioUnitSetProperty(mAudioUnit,
									 kAudioUnitProperty_StreamFormat,
									 kAudioUnitScope_Input,
									 0,
									 &mStreamFormat,
									 sizeof(mStreamFormat)));
	
	
	hResult = CreatePCM2uLaw(&mStreamFormat);
	stiTESTRESULT();
	hResult = CreateuLaw2PCM(&mStreamFormat);
	stiTESTRESULT();
	
	
	output_cb.inputProc = CstiAudioOuputBusCallback;
	output_cb.inputProcRefCon = this;
	
	CheckStatus(AudioUnitSetProperty(
									 mAudioUnit,
									 kAudioUnitProperty_SetRenderCallback,
									 kAudioUnitScope_Input,
									 0,
									 &output_cb,
									 sizeof(output_cb)));
	
	input_cb.inputProc = CstiAudioInputBusCallback;
	input_cb.inputProcRefCon = this;
	CheckStatus(AudioUnitSetProperty(
									 mAudioUnit,
									 kAudioOutputUnitProperty_SetInputCallback,
									 kAudioUnitScope_Global,
									 1,
									 &input_cb,
									 sizeof(input_cb)));
	
	/* Initialize the audio unit */
	CheckStatus(AudioUnitInitialize(mAudioUnit));
	
	
STI_BAIL:
	if (status != noErr) {
		stiTrace ( "Failed Status %08x\n", status );
	}
	
	return hResult;
}


stiHResult CstiAudio::CreatePCM2uLaw(AudioStreamBasicDescription *desc)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = noErr;
	
		
	/* Create the audio converter */
	CheckStatus(AudioConverterNew(desc, &muLawFormat, &mPCM2uLaw));
	
STI_BAIL:
	if (status != noErr) {
		stiTrace ( "Failed Status %08x\n", status );
	}
	
	return hResult;
}

stiHResult CstiAudio::CreateuLaw2PCM(AudioStreamBasicDescription *desc)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status = noErr;
	
		
	/* Create the audio converter */
	CheckStatus(AudioConverterNew(&muLawFormat, desc, &muLaw2PCM));
	
STI_BAIL:
	if (status != noErr) {
		stiTrace ( "Failed Status %08x\n", status );
	}
	
	return hResult;
}


/*!
 * \brief Start audio playback
 * 
 * \return stiHResult 
 */
stiHResult CstiAudio::AudioPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status;

// lock both for start/stop operations
	CstiOSMutexLock threadSafe(m_audioSessionMutex);
	CstiOSMutexLock threadSafe2(m_playbackMutex);

	hResult = OpenAudio();
	stiTESTCOND(hResult == stiRESULT_SUCCESS, stiRESULT_ERROR);
	
	readyStateChangedSignal.Emit (true);

	stiTrace ( "AUDIO OUT: AudioPlaybackStart\n" );

	if (m_playbackEnabled) goto STI_BAIL;
	
	// NOTE
	// since there is a hack to not stop the playback unit, does this
	// break subsequent starts?
	// disabling the status check for now
	if (!m_recordEnabled) {
		status = AudioOutputUnitStart(mAudioUnit);
	}
	//stiTESTCOND(!status, stiRESULT_ERROR);
	
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
//		// start of calls with some sip transfers
//		// for now, just don't stop the playback device.

	if (!m_recordEnabled){
		AudioOutputUnitStop(mAudioUnit);
	}
#endif
	m_playbackEnabled=false;
	
	TPCircularBufferClear(&m_playbackBuffer);
	
	if (!m_recordEnabled)
		CloseAudio();

STI_BAIL:
	return hResult;
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
	
	if (debugAudioQueue) {
		UInt8 *buffer = pAudioFrame->pun8Buffer;
		stiTrace( "AUDIO OUT PUT: %02x%02x%02x%02x%02x%02x%02x%02x... (%d)\n",
					buffer[0],
					buffer[1],
					buffer[2],
					buffer[3],
					buffer[4],
					buffer[5],
					buffer[6],
					buffer[7], pAudioFrame->unFrameSizeInBytes);
	}
#if 0
	// NOTE re-enable later when SIP calls don't stop the audio playback when they're not supposed to.
	if (m_playbackEnabled) {
#endif
		// at 8k samples per second 1000 packets = 1/4 second
		uint32_t availableBytes;
		TPCircularBufferTail(&m_playbackBuffer, &availableBytes);
		if (availableBytes + pAudioFrame->unFrameSizeInBytes > 5000) { // AUDIO_LATENCY_PACKETS ) {
			stiAudioTrace ( "AUDIO OUT: dropping %d bytes\n", pAudioFrame->unFrameSizeInBytes );
			//		m_playbackQueue.skip( m_playbackQueue.size() - AUDIO_LATENCY_PACKETS);
		} else if (m_bAudioOutEnabled){
			TPCircularBufferProduceBytes(&m_playbackBuffer, (char*)pAudioFrame->pun8Buffer, pAudioFrame->unFrameSizeInBytes);
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
	return 0;
}


#pragma mark Input Methods


stiHResult CstiAudio::AudioRecordStart () {

	stiHResult hResult = stiRESULT_SUCCESS;
	OSStatus status;

// lock both for start/stop operations
	CstiOSMutexLock threadSafe(m_audioSessionMutex);
	hResult = OpenAudio();
	stiTESTCOND(hResult == stiRESULT_SUCCESS, stiRESULT_ERROR);

	stiTrace ( "AUDIO IN: AudioRecordStart\n" );
	if (m_recordEnabled) goto STI_BAIL;

	
	if (!m_playbackEnabled) {
		status = AudioOutputUnitStart(mAudioUnit);
		stiTESTCOND(!status, stiRESULT_ERROR);
	}
		
	m_recordEnabled=true;

STI_BAIL:
	return hResult;
}

stiHResult CstiAudio::AudioRecordStop () {
	stiHResult hResult = stiRESULT_SUCCESS;	
	CstiOSMutexLock threadSafe(m_audioSessionMutex);
	
	stiTrace ( "AUDIO IN: AudioRecordStop\n" );
	if (!m_recordEnabled) goto STI_BAIL;
	
	if (!m_playbackEnabled) {
		AudioOutputUnitStop(mAudioUnit);
	}
	m_recordEnabled=false;
	
	TPCircularBufferClear(&m_recordBuffer);
	
	if (!m_playbackEnabled)
		CloseAudio();

STI_BAIL:
	return hResult;
}
stiHResult CstiAudio::AudioRecordCodecSet (
	EstiAudioCodec eAudioCodec) {
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiAudio::AudioRecordPacketGet (SsmdAudioFrame * pAudioFrame) {
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiOSMutexLock threadSafe ( m_recordMutex );
	
	pAudioFrame->unFrameSizeInBytes=0;
	
	uint32_t availableBytes;
	char *buffer = (char*)TPCircularBufferTail(&m_recordBuffer, &availableBytes);
	if (m_recordEnabled && availableBytes >= 240)  {
		memcpy((char*)pAudioFrame->pun8Buffer, buffer, 240);
		TPCircularBufferConsume(&m_recordBuffer, 240);
		pAudioFrame->unFrameSizeInBytes=240;
		if (debugAudioQueue) {
			UInt8 *buffer = pAudioFrame->pun8Buffer;
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
		stiAudioTrace( "AUDIO IN GET: recordEnabled %d bytes available: %d\n", m_recordEnabled ? 1 : 0, availableBytes);
	}
	
	TPCircularBufferTail(&m_recordBuffer, &availableBytes);
	if (availableBytes < 240) {
		packetReadyStateSignal.Emit(false);
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
	bool enable = bEnable==estiTRUE?true:false;
	if (m_recordPrivacy != enable) {
		audioPrivacySignal.Emit(enable);
		m_recordPrivacy= enable;
	}
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
	dtmfReceivedSignal.Emit(eDtmfDigit);
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
