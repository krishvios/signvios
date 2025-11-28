/*!
* \file CAudioDeviceController.h
* \brief This file defines a class for the audio device controller.
*
* Class declaration a class for the Willow/Redwood audio device controller. 
* Provide APIs to communicate with device driver for audio playback/record.
*
*
* \author Ting-Yu Yang
*
*  Copyright (C) 2003-2004 by Sorenson Media, Inc.  All Rights Reserved
*/

//
// Includes
//
#ifndef WIN32
#include <sys/ioctl.h>
#endif
#include "CAudioDeviceController.h"
#include "stiTrace.h"
#include "error_dt.h"

#ifdef NTOUCH_DRIVER
	#include <linux/soundcard.h>
	#include <rcu1.h>
	#include "error_dt.h"
//	#include "rcuapi/rcuapi.h"
	
	
	#define AUDIO_IN_BUFFER_SIZE	USBRCU_MAX_AUDIO_BUFFER_SIZE
	
	// This is currently shared over from the video controller.  It looks really bad (and it is), but this will
	// be solved by the driver abstraction model.
	extern int hIrDriver;
#endif // NTOUCH_DRIVER
#ifdef VP200_DRIVER
	#include "vpx-ioctl.h"
#endif // VP200_DRIVER

//
// Constants
//

//
// Typedefs
//
typedef struct AudioSetting_st
{
	uint16_t un16Size;
	uint16_t un16Command;
} AudioSetting_st;

//
// Globals
//


//
// Locals
//

//
// Forward Declarations
//

// Constructor
CAudioDeviceController::CAudioDeviceController()
	:
	m_nOpenCount (0)
{
}


// Deconstructor
CAudioDeviceController::~CAudioDeviceController()
{
}


CAudioDeviceController * CAudioDeviceController::GetInstance()
{
     // Meyers singleton: a local static variable (cleaned up automatically at exit)
    static CAudioDeviceController oLocalAudioDeviceController;
    return &oLocalAudioDeviceController;
}

//
// For Audio Playback
//

HRESULT CAudioDeviceController::AudioPlaybackSetCodec(EstiAudioCodec eAudioCodec)
{	
	HRESULT errCode = NO_ERROR;

	// critical section
	Lock ();

#ifdef NTOUCH_DRIVER
	#define RATE 8000   /* the sampling rate */
	#define SIZE 8      /* sample size: 8 or 16 bits */
	#define CHANNELS 1  /* 1 = mono 2 = stereo */

	int arg = SIZE;
	int status = ioctl (m_nfd, SNDCTL_DSP_SETFMT, &arg);
	if (status == -1)
	{
		stiTrace ("sound_pcm_write_bits ioctl failed\n");
		errCode = E_DT_DEVICESET_FAIL;
	}
	if (arg != SIZE)
	{
		stiTrace ("unable to set sample size\n");
		errCode = E_DT_DEVICESET_FAIL;
	}
	
	arg = CHANNELS;
	status = ioctl (m_nfd, SNDCTL_DSP_CHANNELS, &arg);
	if (status == -1)
	{
		stiTrace ("sound_pcm_write_channels ioctl failed\n");
		errCode = E_DT_DEVICESET_FAIL;
	}
	if (arg != CHANNELS)
	{
		stiTrace ("unable to set number of channels\n");
		errCode = E_DT_DEVICESET_FAIL;
	}
	
	arg = RATE;
	status = ioctl (m_nfd, SNDCTL_DSP_SPEED, &arg);
	if (status == -1)
	{
		stiTrace ("sound_pcm_write_write ioctl failed\n");
		errCode = E_DT_DEVICESET_FAIL;
	}
#endif // NTOUCH_DRIVER
#ifdef VP200_DRIVER
	int retval;
	AudioSetting_st stSettings;
	int nSize = 0;
	uint16_t un16AudioCodec =  (uint16_t) eAudioCodec;

	stSettings.un16Size = sizeof(stSettings.un16Size);
	stSettings.un16Command = (uint16_t) esmdAUDIO_SET_DECODER;
	stSettings.un16Size += sizeof(stSettings.un16Command);
	stSettings.un16Size += sizeof(un16AudioCodec);

	nSize = 0;
	memcpy(&aSettingBuf[0], &stSettings.un16Size, sizeof(stSettings.un16Size));
	nSize += sizeof(stSettings.un16Size);
	memcpy(&aSettingBuf[nSize], &stSettings.un16Command, sizeof(stSettings.un16Command));
	nSize += sizeof(stSettings.un16Command );
	memcpy(&aSettingBuf[nSize], &un16AudioCodec, sizeof(uint16_t));
	nSize += sizeof(uint16_t);

#if 0
stiTrace("CAudioDeviceController:: AudioPlaybackSetCodecCodec = %d\n",eAudioCodec);
#endif
  
	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
		retval = ioctl(m_nfd, VPX_SET_AUDIO_PARAMS, &aSettingBuf[0]);
		
		if(retval == -1)
		{
			errCode = E_DT_DEVICESET_FAIL;
		}
	}
#endif
	// end of critical section
	Unlock ();   
	return errCode;
}

HRESULT CAudioDeviceController::AudioPlaybackGetCodec(EstiAudioCodec &eAudioCodec)
{
	
	HRESULT errCode = NO_ERROR;
#if 0
	int retval;
  
	// critical section
	Lock ();
  
	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
		retval = ioctl(m_nfd, DEVICE_AP_CODECGET, &eAudioCodec);
		
		if(retval == -1)
		{
			errCode = E_DT_DEVICEGET_FAIL;
		}
	}

	// end of critical section
	Unlock ();  
#endif

	return errCode;
}


HRESULT CAudioDeviceController::DeviceOpen ()
{
	return NO_ERROR;
}


HRESULT CAudioDeviceController::DeviceClose()
{

	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY   
	int retval;

	Lock ();
	
	if (m_nOpenCount > 0)
	{
		m_nOpenCount--;
		
		if(m_nOpenCount == 0 && m_nfd != 0)
		{
			retval = close(m_nfd);
			
			if(retval == -1)
			{
				errCode = E_DT_DEVICCLOSE_FAIL;
			}
			else
			{
				m_nfd = 0;
			}
		}
	}
	
	Unlock ();
#endif
	return errCode;
}


HRESULT CAudioDeviceController::SoundcardSetup (int bStereo, int nFormat, int nSampleRate)
{
	HRESULT errCode = NO_ERROR;


	// critical section
	Lock ();

	// Setup stereo mode
	if (errCode == NO_ERROR)
	{
#if 0
		if (ioctl (m_nfd, SNDCTL_DSP_STEREO, &bStereo) == -1) 
		{
			printf ("ERROR: setting audio out stereo mode\n");
			errCode = E_DT_DEVICEWRITE_FAIL;
		}
#else
#ifdef NTOUCH_DRIVER
		++bStereo; // This ioctl expects number of channels, so we change the stereo bool into that.
		if (ioctl (m_nfd, SNDCTL_DSP_CHANNELS, &bStereo) == -1) 
		{
			printf ("ERROR: setting audio out stereo mode\n");
			errCode = E_DT_DEVICEWRITE_FAIL;
		}
		--bStereo;
#endif
#endif
		printf ("\t* Stereo = %d\n", bStereo);
	}
	// Setup data format
	if (errCode == NO_ERROR)
	{
#ifdef NTOUCH_DRIVER
		int nFormat = AFMT_U8; 
		if (ioctl (m_nfd, SNDCTL_DSP_SETFMT, &nFormat) == -1) 
		{
			printf ("ERROR: setting audio out format\n");
			errCode = E_DT_DEVICEWRITE_FAIL;
		}
		printf ("\t* Format = %d (8bit = %d)\n", nFormat, AFMT_U8);
#endif
	}
	// Setup sample rate
	if (errCode == NO_ERROR)
	{
#ifdef NTOUCH_DRIVER
		if (ioctl (m_nfd, SNDCTL_DSP_SPEED, &nSampleRate) == -1) 
		{
			printf ("ERROR: setting audio out sample rate\n");
			errCode = E_DT_DEVICEWRITE_FAIL;
		}
		else
		{
			printf ("\t* Sample rate = %d\n", nSampleRate);
		}
#endif		
	}
	
	// end of critical section
	Unlock ();   
	
	return errCode;
}


// Uncomment this if you want to play back the given file instead of the normal stream
//#define USE_PLAY_FILE "/bin/test.wav"
HRESULT CAudioDeviceController::AudioPlaybackPacketPut (SsmdAudioFrame * pAudioFrame)
{
	HRESULT errCode = NO_ERROR;

	// critical section
	Lock ();

#if 0
Whys is this done this way?  Shouldn't the audio device be initialized already.  If it's not
why not?

	while (m_nfd <= 0)
	{
		printf ("audio out not initialized. initializing...\n");
		Unlock ();
		errCode = DeviceOpen (WILLOW_AUDIO_DEVICE);
#ifdef NTOUCH_DRIVER
		if (errCode == NO_ERROR)
		{
			errCode = SoundcardSetup (1, AFMT_U8, 11025);
		}
		Lock ();
#endif // NTOUCH_DRIVER
	}
#endif

	if (pAudioFrame->unFrameSizeInBytes > 0)
	{
#ifdef NTOUCH_DRIVER

#ifdef USE_PLAY_FILE
		static FILE *pFile = 0;
		static int nDataStart = 0;
		#define RIFF_RAW_DATA 0x18 // 0x08 + 0x18 + 0x08
		#define RIFF_FORMAT_CHUNK 0x08 + 0x04
		if(pFile<=0)
		{
			printf("no sound test file.  Opening...\n");
			pFile=fopen(USE_PLAY_FILE,"rb");
			if(pFile<=0) {
				printf("ERR: failed to open sound test file\n");
				return NO_ERROR;
			}
			char szHdr[5];
			fread (szHdr,1,4,pFile); szHdr[4] = '\0';
			int bStereo = 0;
			int nFormat = 0;
			int nSampleRate = 0;
			if (0 == strcmp (szHdr, "RIFF"))
			{
				printf("read wave file\n");
				// These values are not quite right yet
				fseek(pFile,RIFF_FORMAT_CHUNK + 0x0a,SEEK_SET); fread (&((char*)&bStereo)[2],1,2,pFile); --bStereo;// Get stereo flag (short int) from file
				nSampleRate = 8;//fseek(pFile,RIFF_FORMAT_CHUNK + 0x08,SEEK_SET); fread ((&(char*)&nSampleRate)[2],1,2,pFile); // Get format (short int) from file
				fseek(pFile,RIFF_FORMAT_CHUNK + 0x0c,SEEK_SET); fread (&((char*)&nSampleRate)[2],1,2,pFile); // Get sample rate (short int) from file
				
				
				bStereo = 1;
				nFormat = 8;
				nSampleRate = 11025;
				
				nDataStart = RIFF_RAW_DATA;
			}
			else
			{
				printf("read raw file\n");
				bStereo = 1;
				nFormat = 8;
				nSampleRate = 8000;
				nDataStart = 0;
			}
			Unlock ();
			SoundcardSetup (bStereo, nFormat, nSampleRate);
			Lock ();
			fseek (pFile, nDataStart, SEEK_SET); // Skip to start of wave data
		} else {
			unsigned int got=fread(pAudioFrame->pun8Buffer,1,pAudioFrame->unFrameSizeInBytes,pFile);
			if(got!=pAudioFrame->unFrameSizeInBytes || feof(pFile)) {
				pAudioFrame->unFrameSizeInBytes = got;
				fseek (pFile, nDataStart, SEEK_SET); // Skip to start of wave data (repeat)
			}
		}
#endif

		int retval;
		printf ("\t->%d\n", (unsigned int)pAudioFrame->unFrameSizeInBytes);
		retval = write (m_nfd, pAudioFrame->pun8Buffer, pAudioFrame->unFrameSizeInBytes); 
		
		if(retval != -1)
		{
			// Wait till all samples have been played (could cause "click" sound)
			//retval = ioctl (m_nfd, SNDCTL_DSP_SYNC, 0); 
		}
		else
		{
			printf ("audio out write failed\n");
		}

		if(retval == -1)
		{
			errCode = E_DT_DEVICEWRITE_FAIL;
			printf ("audio out SNDCTL_DSP_SYNC failed\n");
		}
#endif // NTOUCH_DRIVER

#ifdef VP200_DRIVER 
		int retval;
		retval = write (m_nfd, pAudioFrame->pun8Buffer, pAudioFrame->unFrameSizeInBytes); 

		if(retval == -1)
		{
			errCode = E_DT_DEVICEWRITE_FAIL;
		}
#endif // VP200_DRIVER
	}

	// end of critical section
	Unlock ();   

	return errCode;
}

HRESULT CAudioDeviceController::AudioPlaybackStartCall(void)
{
	HRESULT errCode = NO_ERROR;

	// critical section
	Lock ();
	
#ifdef NTOUCH_DRIVER

#endif // NTOUCH_DRIVER

#ifdef VP200_DRIVER
	int retval = 0;
	AudioSetting_st stSettings;
	int nSize = 0;

	stSettings.un16Size = sizeof(stSettings.un16Size);
	stSettings.un16Command = (uint16_t) esmdAUDIO_START_PLAYBACK;
	stSettings.un16Size += sizeof(stSettings.un16Command);

	nSize = 0;
	memcpy(&aSettingBuf[0], &stSettings.un16Size, sizeof(stSettings.un16Size));
	nSize += sizeof(stSettings.un16Size);
	memcpy(&aSettingBuf[nSize], &stSettings.un16Command , sizeof(stSettings.un16Command));
	nSize += sizeof(stSettings.un16Command);

	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{

#if 0
stiTrace("CAudioDeviceController:: AudioPlaybackStartCall\n");
#endif
		retval = ioctl(m_nfd, VPX_SET_AUDIO_PARAMS, &aSettingBuf[0]);
		if(retval == -1)
		{
			errCode = E_DT_DEVICESET_FAIL;
		}
	}
#endif // VP200_DRIVER

	// end of critical section
	Unlock ();
	
	return errCode;
}

HRESULT CAudioDeviceController::AudioPlaybackStopCall(void)
{
	HRESULT errCode = NO_ERROR;
#ifdef VP200_DRIVER
	int retval = 0;
	AudioSetting_st stSettings;
	int nSize = 0;

	// critical section
	Lock ();

	stSettings.un16Size = sizeof(stSettings.un16Size);
	stSettings.un16Command = esmdAUDIO_STOP_PLAYBACK;
	stSettings.un16Size += sizeof(stSettings.un16Command);

	memcpy(&aSettingBuf[0], &stSettings.un16Size, sizeof(stSettings.un16Size));
	nSize += sizeof(stSettings.un16Size);
	memcpy(&aSettingBuf[nSize], &stSettings.un16Command , sizeof(stSettings.un16Command));
	nSize += sizeof(stSettings.un16Command);

	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
#if 0
stiTrace("CAudioDeviceController:: AudioPlaybackStopCall\n");
#endif
		retval = ioctl(m_nfd, VPX_SET_AUDIO_PARAMS, &aSettingBuf[0]);

		if(retval == -1)
		{
			errCode = E_DT_DEVICESET_FAIL;
		}
	}

	// end of critical section
	Unlock ();
#endif
	return errCode;
}

//
// For Record
// 
HRESULT CAudioDeviceController::AudioRecordStartCall(void)
{
	HRESULT errCode = NO_ERROR;

	// critical section
	Lock ();
	
#ifdef NTOUCH_DRIVER
	struct usbrcu_ab_control_t ab_control;
	memset (&ab_control, 0, sizeof (usbrcu_ab_control_t));
	ab_control.audio_buffer_size = AUDIO_IN_BUFFER_SIZE;
	ab_control.audio_pkt_timeout = 1000;
	if (usbrcu_audio_capture_init (hIrDriver, &ab_control))
	{
		printf ("Failed to init audio capture\n");
		errCode = E_DT_INVALIDEVICE; 
	}
	else if (usbrcu_audio_capture_start (hIrDriver))
	{
		printf ("Failed to start audio capture\n");
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
		printf ("audio capture ready.\n");
	}
#endif // NTOUCH_DRIVER
	
#ifdef VP200_DRIVER
	int retval = 0;
	AudioSetting_st stSettings;
	int nSize = 0;

	stSettings.un16Size = sizeof(stSettings.un16Size);
	stSettings.un16Command = esmdAUDIO_START_RECORD;
	stSettings.un16Size += sizeof(stSettings.un16Command);

	nSize = 0;
	memcpy(&aSettingBuf[0], &stSettings.un16Size, sizeof(stSettings.un16Size));
	nSize += sizeof(stSettings.un16Size);
	memcpy(&aSettingBuf[nSize], &stSettings.un16Command , sizeof(stSettings.un16Command));
	nSize += sizeof(stSettings.un16Command);

	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
#if 0
stiTrace("CAudioDeviceController:: AudioRecordStartCall\n");
#endif
		retval = ioctl(m_nfd, VPX_SET_AUDIO_PARAMS, &aSettingBuf[0]);
		if(retval == -1)
		{
			errCode = E_DT_DEVICESET_FAIL;
		}
	}
#endif

	// end of critical section
	Unlock ();

	return errCode;
}

HRESULT CAudioDeviceController::AudioRecordStopCall(void)
{
	HRESULT errCode = NO_ERROR;
#ifdef VP200_DRIVER
	int retval = 0;
	AudioSetting_st stSettings;
	int nSize = 0;

	// critical section
	Lock ();

	stSettings.un16Size = sizeof(stSettings.un16Size);
	stSettings.un16Command = esmdAUDIO_STOP_RECORD;
	stSettings.un16Size += sizeof(stSettings.un16Command);

	memcpy(&aSettingBuf[0], &stSettings.un16Size, sizeof(stSettings.un16Size));
	nSize += sizeof(stSettings.un16Size);
	memcpy(&aSettingBuf[nSize], &stSettings.un16Command , sizeof(stSettings.un16Command));
	nSize += sizeof(stSettings.un16Command);

	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
#if 0
stiTrace("CAudioDeviceController:: AudioRecordStopCall\n");
#endif
		retval = ioctl(m_nfd, VPX_SET_AUDIO_PARAMS, &aSettingBuf[0]);

		if(retval == -1)
		{
			errCode = E_DT_DEVICESET_FAIL;
		}
	}

	// end of critical section
	Unlock ();
#endif
	return errCode;
}

HRESULT CAudioDeviceController::AudioRecordSetCodec(EstiAudioCodec eAudioCodec)
{

	HRESULT errCode = NO_ERROR;
#ifdef VP200_DRIVER   
	int retval;
	AudioSetting_st stSettings;
	int nSize = 0;
	uint16_t un16AudioCodec =  (uint16_t) eAudioCodec;
 
	// critical section
	Lock ();

	stSettings.un16Size = sizeof(stSettings.un16Size);
	stSettings.un16Command = (uint16_t) esmdAUDIO_SET_ENCODER;
	stSettings.un16Size += sizeof(stSettings.un16Command);
	stSettings.un16Size += sizeof(un16AudioCodec);

	nSize = 0;
	memcpy(&aSettingBuf[0], &stSettings.un16Size, sizeof(stSettings.un16Size));
	nSize += sizeof(stSettings.un16Size);
	memcpy(&aSettingBuf[nSize], &stSettings.un16Command, sizeof(stSettings.un16Command));
	nSize += sizeof(stSettings.un16Command );
	memcpy(&aSettingBuf[nSize], &un16AudioCodec, sizeof(uint16_t));
	nSize += sizeof(uint16_t);

#if 0
stiTrace("CAudioDeviceController:: AudioRecordSetCodec Codec = %d\n",eAudioCodec);
#endif

	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
		retval = ioctl(m_nfd, VPX_SET_AUDIO_PARAMS, &aSettingBuf[0]);
		if(retval == -1)
		{
			errCode = E_DT_DEVICESET_FAIL;
		}
	}

	// end of critical section
	Unlock ();
#endif
	return errCode;
}

HRESULT CAudioDeviceController::AudioRecordSetSettings (SsmdAudioEncodeSettings * pAudioSettings)
{
	HRESULT errCode = NO_ERROR;
#ifdef VP200_DRIVER   
	int retval;
	AudioSetting_st stSettings;
	int nSize = 0;  
  
	// critical section
	Lock ();

	stSettings.un16Size = sizeof(stSettings.un16Size);
	stSettings.un16Command = (uint16_t) esmdAUDIO_SET_ENCODE_SETTINGS;
	stSettings.un16Size += sizeof(stSettings.un16Command);
	stSettings.un16Size += sizeof(SsmdAudioEncodeSettings);

	nSize = 0;
	memcpy(&aSettingBuf[0], &stSettings.un16Size, sizeof(stSettings.un16Size));
	nSize += sizeof(stSettings.un16Size);
	memcpy(&aSettingBuf[nSize], &stSettings.un16Command, sizeof(stSettings.un16Command));
	nSize += sizeof(stSettings.un16Command );
	memcpy(&aSettingBuf[nSize], pAudioSettings, sizeof(SsmdAudioEncodeSettings));
	nSize += sizeof(SsmdAudioEncodeSettings);

#if 0
stiTrace("CAudioDeviceController:: AudioRecordSetSettings SamplesPerPackets = %d\n",
pAudioSettings->un16SamplesPerPacket);
#endif
  
	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE; 
	}
	else
	{
		retval = ioctl(m_nfd, VPX_SET_AUDIO_PARAMS, &aSettingBuf[0]);	
		
		if(retval == -1)
		{
			errCode = E_DT_DEVICESET_FAIL;
		}
	}
	// end of critical section
	Unlock ();
#endif
	return errCode;
}


HRESULT CAudioDeviceController::AudioRecordPacketGet (SsmdAudioFrame * pAudioFrame)
{
	HRESULT errCode = NO_ERROR;
  
	// critical section
	Lock ();
	
#ifdef NTOUCH_DRIVER	
	// get audio data
	struct usbrcu_ab_t ab;
	int ret;
	pAudioFrame->unFrameSizeInBytes = 0;
	bool bWigout = false;
	while (1)
	{
		ret = usbrcu_audio_capture_buffer_available (hIrDriver);
		if (ret < 0)
		{
			printf ("audio capture frame available error\n");
			errCode = E_DT_INVALIDEVICE;
			break;
		}
		else if (ret == USBRCU_DATA_NOT_AVAILABLE)
		{
			// continue loop
		}
		else if (ret == USBRCU_DATA_IS_AVAILABLE)
		{
			if(bWigout)
			{
				bWigout = false;
				printf ("audio capture is wigout ceased\n");
			}
			// get capture frame
			memset(&ab, 0, sizeof(ab));
			if (usbrcu_audio_capture_get_buffer (hIrDriver, &ab) < 0)
			{
				printf ("audio capture failed to get a frame\n");
				errCode = E_DT_INVALIDEVICE;
				break;
			}

			// Copy out the data
			if (ab.buf_mmap == NULL)
			{
				printf (".");
			}
			else
			{
				printf ("\t<-%d\n", ab.bytes);
				memcpy (pAudioFrame->pun8Buffer, (const void*)ab.buf_mmap, ab.bytes);
				pAudioFrame->unFrameSizeInBytes = ab.bytes;
			}
			
			// free audio capture frame
			if (usbrcu_audio_capture_free_buffer (hIrDriver, &ab) < 0)
			{
				printf ("audio capture free buffer failed\n");
				errCode = E_DT_INVALIDEVICE;
			}
			break;
		}
		else if(!bWigout)
		{
			bWigout = true;
			printf ("audio capture is wiggin out (%d)...\n", ret);
			break;
		}
	}
	
#endif // NTOUCH_DRIVER
	
#ifdef VP200_DRIVER 
	int retval = 0;
  
	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE; 
	}
	else
	{
	
		pAudioFrame->unFrameSizeInBytes = 0;
		
		retval = read(m_nfd, pAudioFrame->pun8Buffer, pAudioFrame->unBufferMaxSize); 
		

		if(retval == -1)
		{
			errCode = E_DT_DEVICEWRITE_FAIL;
		}
		else
		{
			pAudioFrame->unFrameSizeInBytes = retval;
		}
	}
#endif  //  VP200_DRIVER 

	// end of critical section
	Unlock ();
	
	return errCode;
}


HRESULT CAudioDeviceController::AudioRecordSetEchoMode (EsmdEchoMode eEchoMode)
{
	HRESULT errCode = NO_ERROR;
#ifdef VP200_DRIVER 
	int retval;
	AudioSetting_st stSettings;
	int nSize = 0;
  uint16_t un16EchoMode = (uint16_t) eEchoMode;

	// critical section
	Lock ();

	stSettings.un16Size = sizeof(stSettings.un16Size);
	stSettings.un16Command = (uint16_t) esmdAUDIO_SET_ECHO_MODE;
	stSettings.un16Size += sizeof(stSettings.un16Command);
	stSettings.un16Size += sizeof(un16EchoMode);

	nSize = 0;
	memcpy(&aSettingBuf[0], &stSettings.un16Size, sizeof(stSettings.un16Size));
	nSize += sizeof(stSettings.un16Size);
	memcpy(&aSettingBuf[nSize], &stSettings.un16Command, sizeof(stSettings.un16Command));
	nSize += sizeof(stSettings.un16Command );
	memcpy(&aSettingBuf[nSize], &un16EchoMode, sizeof(un16EchoMode));
	nSize += sizeof(un16EchoMode);

#if 0
stiTrace("CAudioDeviceController:: AudioRecordSetEchoMode EchoMode = %d\n",un16EchoMode);
#endif
   
	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
		uint32_t un32EchoMode = 0;

		if(esmdAUDIO_ECHO_ON == eEchoMode)
		{
			un32EchoMode = 1;
		}

		retval = ioctl(m_nfd, VPX_SET_AUDIO_PARAMS, &aSettingBuf[0]);	
	       		
		if(retval == -1)
		{
			errCode = E_DT_DEVICEGET_FAIL;
		}
	}
	// end of critical section
	Unlock ();
#endif
	return errCode;
}

HRESULT CAudioDeviceController::ControlMsgGet(EsmdControlMsg &eCtrlMsg)
{
	HRESULT errCode = NO_ERROR;

#if 0  
	int retval;

	// critical section
	Lock ();
    
	uint32_t un32Msg;

	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
		retval = ioctl(m_nfd, DEVICE_CTRLMSG_GET, &un32Msg);
		
		if(retval == -1)
		{
			errCode = E_DT_DEVICEGET_FAIL;
		}
		else
		{
			// TODO: determine the message type
			eCtrlMsg = (EsmdControlMsg) un32Msg;

		}
	}
	// end of critical section
	Unlock (); 
#endif

	return errCode;
}

HRESULT CAudioDeviceController::DTMFToneSet(EstiDTMFDigit eDTMFDigit)
{
	HRESULT errCode = NO_ERROR;

#if 0   
	int retval;
	uint32_t un32Msg;

	// critical section
	Lock ();
    
	if(m_nfd == 0)
	{
		errCode = E_DT_INVALIDEVICE;
	}
	else
	{
		uint32_t un32DTMFInfo = 0;
		int nValidSignal = 1;

		switch(eDTMFDigit)
		{
		case estiDTMF_DIGIT_0:
			un32DTMFInfo = 0;
			break;
		case estiDTMF_DIGIT_1:
			un32DTMFInfo = 1;
			break;
		case estiDTMF_DIGIT_2:
			un32DTMFInfo = 2;
			break;
		case estiDTMF_DIGIT_3:
			un32DTMFInfo = 3;
			break;
		case estiDTMF_DIGIT_4:
			un32DTMFInfo = 4;
			break;
		case estiDTMF_DIGIT_5:
			un32DTMFInfo = 5;
			break;
		case estiDTMF_DIGIT_6:
			un32DTMFInfo = 6;
			break;
		case estiDTMF_DIGIT_7:
			un32DTMFInfo = 7;
			break;
		case estiDTMF_DIGIT_8:
			un32DTMFInfo = 8;
			break;
		case estiDTMF_DIGIT_9:
			un32DTMFInfo = 9;
			break;
		case estiDTMF_DIGIT_S:
			un32DTMFInfo = 10;
			break;
		case estiDTMF_DIGIT_P:
			un32DTMFInfo = 11;
			break;
		default:
			nValidSignal = 0;
			break;
		}

		if(nValidSignal)
		{
			retval = ioctl(m_nfd, VP2_SET_AUDIO, un32DTMFInfo);
			if(retval == -1)
			{
				errCode = E_DT_DEVICEGET_FAIL;
			}
		}
	}
	// end of critical section
	Unlock ();
#endif

	return errCode;
}


