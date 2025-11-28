/*!
* \file CVideoDeviceController.h
* \brief This file defines a class for the video device controller.
*
* Class declaration a class for VP-Next device controller.
* Provide APIs to communicate with video pipeline tasks
*
*
* \author Ting-Yu Yang
*
*  Copyright (C) 2003-2009 by Sorenson Communications, Inc.  All Rights Reserved
*/

//
// Includes
//
#include "stiTrace.h"
#include "CVideoDeviceController.h"
#include "error_dt.h"

#ifdef stiDRIVER_READY
	#include "VideoTasksCommon.h"
	#include "CDisplayTask.h"
	#include "CCaptureTask.h"
	#include "CBufferElement.h"
	#include "CVideoEncodeTask.h"
	#include "CVideoDecodeTask.h"

	#ifdef USBRCU_MAGIC_NUMBER
			// this is going to want to be redefined.
		#undef USBRCU_MAGIC_NUMBER
	#endif
	#include <slicring.h>
	#include "srapi/srapi.h"
//#include "rcuapi/rcuapi.h"


#endif

// Use old or new PTZ settings
#define PTZ_NEW 1


// Turn on/off traces for this module here
//#define BSP_TRACE
#ifdef BSP_TRACE
	#define _stiTrace stiTrace
#else
	#define _stiTrace(x)
#endif


//
// Constants
//
SsmdIndicatorSettings g_stIndicatorSettings =
{
 estiTONE_NONE, // SLIC tone
// estiOFF,       // Ring switch (on/off)
 estiTONE_NONE, // Buzzer tone
 estiON,        // Power LED
 estiOFF,       // Status LED
 estiOFF,       // Camera Active LED
 0              // Light Ring BitMask
};


//
// Typedefs
//
//
// These are possible commands that the host can issue to the video DSP
typedef enum EsmdVideoCommand
{
	esmdVIDEO_SET_ENCODER = 0,
	esmdVIDEO_SET_ENCODE_SETTINGS = 1,
	esmdVIDEO_START_RECORD = 2,
	esmdVIDEO_STOP_RECORD = 3,

	esmdVIDEO_SET_DECODER = 4,
	esmdVIDEO_SET_DECODE_SETTINGS = 5,
	esmdVIDEO_START_PLAYBACK = 6,
	esmdVIDEO_STOP_PLAYBACK = 7,

	esmdVIDEO_SET_DISPLAY_SETTINGS = 8,
	esmdVIDEO_SET_CAPTURE_SETTINGS = 9,
	esmdVIDEO_KEY_FRAME_REQUEST = 10,

	esmdVIDEO_BRIGHTNESS_SET = 11,
	esmdVIDEO_SATURATION_SET = 12,
	esmdVIDEO_BLUETINT_SET = 13,
	esmdVIDEO_REDTINT_SET = 14,
} EsmdVideoCommand;

typedef struct VideoSetting_st
{
	int nSize;
	EsmdVideoCommand eCommand;
} VideoSetting_st;

// These are possible requests that the host can request from the video DSP
typedef enum EsmdVideoRequest
{
	esmdVIDEO_GET_PLAYBACK_SIZE = 0

} EsmdVideoRequest;

typedef struct VideoRequest_st
{
	int nSize;
	EsmdVideoRequest eCommand;
} VideoRequest_st;


//#define USE_RCU_FOR_OTHER_THAN_VIDEO_TASKS 1

#define MAX_TASK_SHUTDOWN_WAIT 30


// Globals
//
// This is a bad thing, but necessary to share the RCU driver with the remotecontrol layer in PEG until
// we move to the new driver scheme.
int hIrDriver = 0;

//
// Locals
//

//
// Forward Declarations
//



// Constructor
CVideoDeviceController::CVideoDeviceController ()
:	m_hRCUDevice (0),
	m_hSlicDevice (0)
{
	m_sTasksRunning.iAll = 0;
	m_LockMutex = stiOSMutexCreate();
	m_stIndicatorSettings = g_stIndicatorSettings;
}

// Deconstructor
CVideoDeviceController::~CVideoDeviceController ()
{
	//TODO: Destroy m_tTasksShutdownCond and m_LockMutex
}

CVideoDeviceController * CVideoDeviceController::GetInstance ()
{
	// Meyers singleton: a local static variable (cleaned up automatically at exit)
	static CVideoDeviceController oLocalVideoDeviceController;
	return &oLocalVideoDeviceController;
}

/// \brief Callback from sub-threads
///
/// This function is used mainly for monitoring the shutdown status of child threads.
///
/// \returns An EstiResult value containing success or failure codes
///
stiHResult CVideoDeviceController::ThreadCallback (
	int32_t n32Message,	///< holds the type of message
 	size_t MessageParam,	///< holds data specific to the message
	size_t CallbackParam)	///< points to the instantiated this object
{
	//stiTrace ("CVideoDeviceController::ThreadCallback\n");

	EstiResult eResult = estiOK;

	CVideoDeviceController *pThis = (CVideoDeviceController*)CallbackParam;

	// extract the message
	EstiCmMessage eMessage = (EstiCmMessage) n32Message;

	switch (eMessage)
	{
		case estiMSG_CB_ERROR_REPORT:
		{
			stiTrace ("\testiMSG_CB_ERROR_REPORT\n");
			fflush (stdout);
			// Extract data and call ErrorReport
			SstiErrorLog *psStiErrorLog = (SstiErrorLog*)MessageParam;
			eResult = pThis->ErrorReport(psStiErrorLog);
			break;
		}
		default:
			stiTrace("\tDefault\n");
			fflush (stdout);
			break;
	}

	return stiRESULT_CONVERT(eResult);

} // CVideoDeviceController::ThreadCallback

/// \brief Report errord
///
/// This utility function prints a SstiErrorLog message in a friendly way.
///
/// \returns An EstiResult value containing success or failure codes
///
EstiResult CVideoDeviceController::ErrorReport (
	const SstiErrorLog *pstErrorLog) ///> Pointer to the structure containing the error data
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::ErrorReport);

	EstiResult eResult = estiOK;

	// Send the message to the client application. Should we also send the
	// message to the user?
	if (NULL != pstErrorLog)
	{
		stiTrace("Error: task name = %s (%z) line = %d stierr = %ld errno = %ld\n",
				 pstErrorLog->szTaskName,
				pstErrorLog->tidTaskId,
	  			pstErrorLog->nLineNbr,
   				pstErrorLog->un32StiErrorNbr,
   				pstErrorLog->un32SysErrorNbr);

	} // end if

	return (eResult);
} // CVideoDeviceController::ErrorReport

HRESULT CVideoDeviceController::VideoTasksStartup ()
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoTasksStartup);

	stiHResult hResult = stiRESULT_SUCCESS;
#ifdef stiDRIVER_READY
	stiHResult hCurrent;

	stiDEBUG_TOOL (1,
				   stiTrace ("CCVideoDeviceController::VideoTasksStartup\n");
				  );

	// Capture Task
	hCurrent = g_oCaptureTask.Startup (ThreadCallback, 0);

	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
		stiDEBUG_TOOL (1,
					   stiTrace ("CCVideoDeviceController::VideoTasksStartup - g_oCaptureTask.Startup failed\n");
					  );
	}
	else
	{
		// display task is running
		m_sTasksRunning.tasks.bCaptureTask = true;
	}

	// Video Encode Task
	hCurrent = g_oVideoEncodeTask.Startup (ThreadCallback, 0);

	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
		stiDEBUG_TOOL (1,
					   stiTrace ("CCVideoDeviceController::VideoTasksStartup - g_oVideoEncodeTask.Startup failed\n");
					  );
	}
	else
	{
		// display task is running
		m_sTasksRunning.tasks.bVideoEncodeTask = true;
	}

	// Video Decode Task
	hCurrent = g_oVideoDecodeTask.Startup (ThreadCallback, 0);

	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
		stiDEBUG_TOOL (1,
					   stiTrace ("CCVideoDeviceController::VideoTasksStartup - g_oVideoDecodeTask.Startup failed\n");
					  );
	}
	else
	{
		// video decode task is running
		m_sTasksRunning.tasks.bVideoDecodeTask = true;

		g_oVideoDecodeTask.KeyframeRequestCBSet (CVideoDeviceController::VideoPlaybackKeyframeRequest, this);
		g_oVideoDecodeTask.KeyframeReceivedCBSet (CVideoDeviceController::VideoPlaybackKeyframeReceived, this);
	}

	m_hRCUDevice = g_oCaptureTask.DriverHandleGet ();


#if USE_RCU_FOR_OTHER_THAN_VIDEO_TASKS
	// Use tne ntouch's rcu driver
	// Share this across modules like an idiot
	hIrDriver = m_hRCUDevice;
#endif //USE_RCU_FOR_OTHER_THAN_VIDEO_TASKS

#endif //stiDRIVER_READY



	//TODO deal with error codes
	hResult = stiRESULT_SUCCESS;

	return hResult;

}

stiHResult CVideoDeviceController::VideoTasksShutdown ()
{
	printf("CVideoDeviceController::VideoTasksShutdown\n");

	m_hRCUDevice = 0;

	stiHResult hResult = stiRESULT_SUCCESS;
#ifdef stiDRIVER_READY
	stiHResult hCurrent;

	hCurrent = g_oCaptureTask.Shutdown ();

	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	hCurrent = g_oVideoEncodeTask.Shutdown ();

	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	hCurrent = g_oVideoDecodeTask.Shutdown ();

	if ((hResult == stiRESULT_SUCCESS) && (hCurrent != stiRESULT_SUCCESS))
	{
		hResult = hCurrent;
	}

	//
	// Wait for everything to shut itsself down
	//
	Lock ();

	int iRunningThreads = m_sTasksRunning.iAll;

	while (iRunningThreads)
	{
		if (estiOK != stiOSCondWait (m_tTasksShutdownCond, m_LockMutex, MAX_TASK_SHUTDOWN_WAIT))
		{
			if (hResult == stiRESULT_SUCCESS)
			{
				hResult = stiRESULT_TASK_SHUTDOWN_HANDLE_FAILED;
			}
			break;
		}
		iRunningThreads = m_sTasksRunning.iAll;
	}
	Unlock ();
#endif
	return hResult;
}

/// \brief Enables the video capture stream
///
void CVideoDeviceController::VideoCaptureStart ()
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoCaptureStart);
#ifdef stiDRIVER_READY
	// TODO: I have NO IDEA why this doesn't work the right way!
	EsmdCaptureSource eSource = (EsmdCaptureSource)0;//esmdCAPTURE_SOURCE_MT9V111;

	SsmdCaptureSettings stCapSettings =
	{
		eSource,
		{
			0,
			DEFAULT_RCU_CAP_WIDTH,	// input width
			MAX_ENCODE_WIDTH,	// output width
			0,
			DEFAULT_RCU_CAP_HEIGHT,	// input height
			MAX_ENCODE_HEIGHT	// output height
		}
	};

	g_oCaptureTask.CaptureSettingsSet (&stCapSettings);

	g_oCaptureTask.VideoCaptureStart ();
#endif
} // CVideoDeviceController::VideoCaptureStart


/// \brief Disables the video capture stream
///
void CVideoDeviceController::VideoCaptureStop ()
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoCaptureStop);
#ifdef stiDRIVER_READY
	g_oCaptureTask.VideoCaptureStop ();
#endif
}


//
// For Video Playback
//
HRESULT CVideoDeviceController::VideoPlaybackSetCodec (
	EstiVideoCodec eVideoCodec)
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoPlaybackSetCodec);
	HRESULT errCode = NO_ERROR;

#ifdef stiDRIVER_READY
	// critical section
	Lock ();

	// end of critical section
	Unlock ();
#endif

	return errCode;
}

HRESULT CVideoDeviceController::VideoPlaybackGetCodec (
	EstiVideoCodec &eVideoCodec)
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoPlaybackGetCodec);
	HRESULT errCode = NO_ERROR;

	return errCode;
}

HRESULT CVideoDeviceController::VideoPlaybackStartCall ()
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoPlaybackStartCall);

	HRESULT errCode = NO_ERROR;

#ifdef stiDRIVER_READY
	// critical section
	Lock ();

	g_oVideoDecodeTask.DecodeStart ();

	// end of critical section
	Unlock ();
#endif
	return errCode;
}

HRESULT CVideoDeviceController::VideoPlaybackStopCall ()
{
	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY
	// critical section
	Lock ();

	g_oVideoDecodeTask.DecodeStop ();

	// end of critical section
	Unlock ();
#endif
	return errCode;
}

EstiBool CVideoDeviceController::IsReadyToDecode ()
{
	EstiBool bReadyToDecode = estiFALSE;

#ifdef stiDRIVER_READY

	// critical section
	Lock ();

	bReadyToDecode = g_oVideoDecodeTask.IsReadyToDecode ();

	// end of critical section
	Unlock ();

#endif

	return bReadyToDecode;
}

HRESULT CVideoDeviceController::VideoPlaybackFramePut (
	vpe::VideoFrame * pVideoFrame)
{
	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY

	stiHResult hResult = stiRESULT_SUCCESS;

	// critical section
	Lock ();

	//printf("CVideoDeviceController::VideoPlaybackPacketPut\n");

	if (pVideoPacket)
	{
		hResult = g_oVideoDecodeTask.BitstreamAvailable (pVideoPacket->pun8Buffer, pVideoPacket->unPacketSizeInBytes);
	}

	if (stiIS_ERROR(hResult))
	{
		errCode = E_FAIL;
	}

	// end of critical section
	Unlock ();
#endif
	return errCode;
}

HRESULT CVideoDeviceController::VideoPlaybackSizeGet (
	SsmdVideoPlaybackSize * pVideoPlaybackSize)
{
	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY

	// critical section
	Lock ();


	// end of critical section
	Unlock ();
#endif
	return errCode;

}


void CVideoDeviceController::VideoPlaybackKeyframeRequestCBSet (
	VideoPlaybackKeyframeRequestCB pfnVideoPlaybackKeyframeRequestCB,
	void *pCallbackData)
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoPlaybackKeyframeRequestCBSet);

	m_pfnVideoPlaybackKeyframeRequestCB = pfnVideoPlaybackKeyframeRequestCB;
	m_pVideoPlaybackKeyframeRequestCBData = pCallbackData;

}


void CVideoDeviceController::VideoPlaybackKeyframeReceivedCBSet (
	VideoPlaybackKeyframeReceivedCB pfnVideoPlaybackKeyframeReceivedCB,
	void *pCallbackData)
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoPlaybackKeyframeReceivedCBSet);

	m_pfnVideoPlaybackKeyframeReceivedCB = pfnVideoPlaybackKeyframeReceivedCB;
	m_pVideoPlaybackKeyframeReceivedCBData = pCallbackData;

}

EstiResult CVideoDeviceController::VideoPlaybackKeyframeRequest (
	void *pObject)
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoPlaybackKeyframeRequest);

	stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
				   stiTrace ("CVideoDeviceController::VideoPlaybackKeyframeRequest - Requesting keyframe\n");
				  );

	CVideoDeviceController *pThis = (CVideoDeviceController *)pObject;
	EstiResult eResult = estiOK;

	if (pThis->m_pfnVideoPlaybackKeyframeRequestCB)
	{
		eResult = pThis->m_pfnVideoPlaybackKeyframeRequestCB (pThis->m_pVideoPlaybackKeyframeRequestCBData);
	}

	return eResult;
}

EstiResult CVideoDeviceController::VideoPlaybackKeyframeReceived (
	void *pObject)
{
	stiLOG_ENTRY_NAME (CVideoDeviceController::VideoPlaybackKeyframeReceived);

	stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
				   stiTrace ("CVideoDeviceController::VideoPlaybackKeyframeRecieved - Received keyframe\n");
				  );

	CVideoDeviceController *pThis = (CVideoDeviceController *)pObject;
	EstiResult eResult = estiOK;

	if (pThis->m_pfnVideoPlaybackKeyframeReceivedCB)
	{
		eResult = pThis->m_pfnVideoPlaybackKeyframeReceivedCB (pThis->m_pVideoPlaybackKeyframeReceivedCBData);
	}

	return eResult;
}


//
// for Micro MT9V111 imager register settings
//


HRESULT CVideoDeviceController::VideoRecordSetBrightness (
	uint32_t un32Brightness)
{

	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY

	// critical section
	Lock ();


	// end of critical section
	Unlock ();
#endif
	return errCode;
}

HRESULT CVideoDeviceController::VideoRecordSetSaturation (
	uint32_t un32Saturation)
{

	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY

	// critical section
	Lock ();


	// end of critical section
	Unlock ();
#endif
	return errCode;
}

HRESULT CVideoDeviceController::VideoRecordSetTint (
	EsmdTintChannel eTintChannel,
	uint32_t un32TintValue)
{

	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY

	// critical section
	Lock ();


	// end of critical section
	Unlock ();
#endif
	return errCode;
}

HRESULT CVideoDeviceController::CameraMove (
	SsmdCaptureSettings * pstCaptureSettings)
{
	HRESULT errCode = NO_ERROR;

#ifdef stiDRIVER_READY

	// critical section
	Lock ();

	//g_oCaptureTask.CaptureSettingsSet (pstCaptureSettings);
	// end of critical section
	Unlock ();
#endif
	return errCode;

}

HRESULT CVideoDeviceController::CameraSelect (
	uint8_t un8VideoSrcDev, uint8_t un8VideoSrcMode)
{
	HRESULT errCode = NO_ERROR;


	return errCode;

}

HRESULT CVideoDeviceController::CameraParamGet (
	SsmdCameraParam * pstCameraParams)
{
	HRESULT errCode = NO_ERROR;


	return errCode;
}

HRESULT CVideoDeviceController::CameraParamSet (
	SsmdCameraParam * pstCameraParams)
{
	HRESULT errCode = NO_ERROR;


	return errCode;
}

HRESULT CVideoDeviceController::AsyncMessageGet (
	uint32_t &un32Msg)
{
	HRESULT errCode = NO_ERROR;

#ifdef stiDRIVER_READY

	// critical section
	Lock ();


	// end of critical section
	Unlock ();
#endif

	return errCode;
}

HRESULT CVideoDeviceController::IndicatorSettingsGet(
	SsmdIndicatorSettings * pstIndicatorSetting)
{
	HRESULT errCode = NO_ERROR;
	
	*pstIndicatorSetting = m_stIndicatorSettings;
	
	return errCode;
}


HRESULT CVideoDeviceController::IndicatorSettingsSet(
 	const SsmdIndicatorSettings * pstIndicatorSetting)
{
	HRESULT errCode = NO_ERROR;


	// critical section
	Lock ();

	m_stIndicatorSettings = *pstIndicatorSetting;
	
	// end of critical section
	Unlock ();
	return errCode;
}


HRESULT CVideoDeviceController::CameraSet (
	EstiSwitch eSwitch)
{
	HRESULT errCode = NO_ERROR;

	return errCode;
}


HRESULT CVideoDeviceController::ResetButtonStateGet (
	EstiButtonState &eButtonStatus)
{
	HRESULT errCode = NO_ERROR;


	return errCode;

}

