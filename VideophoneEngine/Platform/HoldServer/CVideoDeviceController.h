/*!
* \file CVideoDeviceController.h
* \brief This file defines a class for the video device controller.
*
* Class declaration a class for VP-Next video device controller. 
* Provide APIs to communicate video pipeline tasks
*
*
* \author Ting-Yu Yang
*
*  Copyright (C) 2003-2009 by Sorenson Communications, Inc.  All Rights Reserved
*/

#ifndef CVIDEODEVICECONTROLLER_H
#define CVIDEODEVICECONTROLLER_H

#include "stiDefs.h"
#include "stiError.h"
#include "CDeviceController.h"
#include "VideoControl.h"
#include "IstiVideoOutput.h"
#include "IstiVideoInput.h"
#include <condition_variable>

//
// Constants
//


//
// Typedefs
//

typedef EstiResult (*VideoPlaybackKeyframeRequestCB) (
	void *pData);

typedef EstiResult (*VideoPlaybackKeyframeReceivedCB) (
	void *pData);

// This structure defines a display image
typedef struct SsmdImageSettings
{
	int bVisible;
	int bMirrored;
	int bHold;
	int bPrivacy;
	unsigned int unWidth;
	unsigned int unHeight;
	unsigned int unPosX;
	unsigned int unPosY;
} SsmdImageSettings;

//
// This structure defines the display settings
//
typedef enum EsmdTopView
{
	esmdSelfView 	= 0,
	esmdRemoteView	= 1
} EsmdTopView;

// This structure defines the display settings
typedef struct SsmdDisplaySettings
{
	SsmdImageSettings stSelfView;
	SsmdImageSettings stRemoteView;
	EsmdTopView eTopView;
} SsmdDisplaySettings;

typedef enum EsmdCaptureSource
{
	esmdCAPTURE_SOURCE_MT9V111 	= 0,
	esmdCAPTURE_SOURCE_TVP5150	= 1
} EsmdCaptureSource;

//	This structure defines the settings for the internal
// 	camera pan/tilt/zoom.
typedef struct SsmdPtzSettings
{
	unsigned int unHorzPos;		// horizontal pan
	unsigned int unHorzZoom;	// horizontal zoom
	unsigned int unHorzSize;	// horizontal size
	unsigned int unVertPos;		// vertical pan
	unsigned int unVertZoom;		// vertical zoom
	unsigned int unVertSize;		// vertical size
} SsmdPtzSettings;

// This structure defines the capture settings
typedef struct SsmdCaptureSettings
{

	EsmdCaptureSource eCaptureSource;
	SsmdPtzSettings stPtzSettings;

} SsmdCaptureSettings;

// This structure defines the playback size structure
typedef struct SsmdVideoPlaybackSize
{
	unsigned int unWidth;
	unsigned int unHeight;
} SsmdVideoPlaybackSize;

//
// This structure defines the common encoder settings for all video codecs
//
typedef struct SsmdVideoEncodeSettings
{
		unsigned int unMaxPacketSize;       // Max size of each video packet (slice)
		unsigned int unTargetBitRate;       // Target Bitrate (kbps)
		unsigned int unTargetFrameRate;     // Target frame rate (used for bitrate calculations)
		unsigned int bKeyFrameRequest;      // Request a keyframe (0=no, 1=yes)
		unsigned int unIntraRefreshRate;    // Each macroblocks will be encoded
                                        // as intra mode
                                        // every unIntraRefreshRate frames
                                        // 0 = no intra blocks, 1000 = very few
                                        // intra blocks
                                        // 132=some intra blocks, 5=almost all
                                        // intra blocks
                                        // 1=all intra
		unsigned int unIntraFrameRate;      // An intra frame is coded every
                                        // unIntraFrameRate frame(s)
                                        // (0=no automatic I-frames)
		unsigned int unRows;                // Number of rows of source frame to be encoded
		unsigned int unColumns;             // Number of columns of source frame to be encoded
		unsigned int bDeblockingFilterMode; // Deblocking filter mode (0=off, 1=on)
} SsmdVideoEncodeSettings;

// 
// Forward Declarations
//


//
// Globals
//


class CVideoDeviceController : public CDeviceController
{
public:
	
	CVideoDeviceController ();
	virtual ~CVideoDeviceController ();
	
	/*!
	* \brief Retrieves the only reference to the video device controller (singleton obj support)
	*/
	static CVideoDeviceController * GetInstance ();
	
	HRESULT VideoTasksStartup ();
	
	stiHResult VideoTasksShutdown ();
	
	void VideoCaptureStart ();
	
	void VideoCaptureStop ();
	
	//
	// Playback
	//

	/*!
	 * \brief Set callback to be called requesting a keyframe
	 * \param VideoPlaybackKeyframeRequestCB pfnVideoPlaybackKeyframeRequestCB - The callback to be called
	 * \param void *pCallbackData - Opaque data to be passed to the callback
	 * \retval 0 - success, otherwize error occured
	 */
	void VideoPlaybackKeyframeRequestCBSet (
		VideoPlaybackKeyframeRequestCB pfnVideoPlaybackKeyframeRequestCB,
		void *pCallbackData);
	
	/*!
	 * \brief Set callback to be called when we have recieved a keyframe
	 * \param VideoPlaybackKeyframeReceivedCB pfnVideoPlaybackKeyframeRecievedCB - The callback to be called
	 * \param void *pCallbackData - Opaque data to be passed to the callback
	 * \retval 0 - success, otherwize error occured
	 */
	void VideoPlaybackKeyframeReceivedCBSet (
		VideoPlaybackKeyframeReceivedCB pfnVideoPlaybackKeyframeReceivedCB,
		void *pCallbackData);
	
	/*!
	* \brief Set Codec Mode for video playback
	* \param EstiVideoCodec eVideoCodec - video codec to set
	* \retval 0 - success, otherwize error occured
	*/	
	HRESULT VideoPlaybackSetCodec (
		EstiVideoCodec eVideoCodec);

	/*!
	* \brief Get current Codec Mode in video playback device
	* \param EstiVideoCodec &eVideoCodec - video codec to get
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT VideoPlaybackGetCodec (
		EstiVideoCodec &eVideoCodec);
	
	/*!
	* \brief Write a video frame to video device for playback
	* \param SstiPlaybackVideoFrame * pVideoFrame - pointer to video buffer for playback
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT VideoPlaybackFramePut (
		vpe::VideoFrame * pVideoFrame);

	/*!
	* \brief Set to inform that the video playback starts
	* \param NONE
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT VideoPlaybackStartCall ();

	/*!
	* \brief Set to inform that the video playback stops
	* \param NONE
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT VideoPlaybackStopCall ();

	/*!
	* \brief Get the video playback size
	* \param SsmdVideoPlaybackSize * pVideoPlaybackSizec - video playback size to get
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT VideoPlaybackSizeGet (
		SsmdVideoPlaybackSize * pVideoPlaybackSize);

	/*! \brief Check if there is empty bitstream buffer for decoding
	*
	*  \retval estiTRUE if there is buffer for decoding
	*  \retval estiFALSE otherwise
	*/
	EstiBool IsReadyToDecode ();

	//
	// Record
	//

	/*!
	* \brief Set the brightness to video record device
	* \param uint32_t un32Brightness - value to set
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT VideoRecordSetBrightness (
		uint32_t un32Brightness);

	/*!
	* \brief Set the Saturation to video record device
	* \param uint32_t Saturation - value to set
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT VideoRecordSetSaturation (
		uint32_t un32Saturation);  

	/*!
	* \brief Set the color tint to video record device
	* \param uint32_t EsmdTintChannel - blue channel or red channel to set  
	* \param uint32_t un32TintValue - value to set
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT VideoRecordSetTint (
		EsmdTintChannel eTintChannel,
		uint32_t un32TintValue);

	//
	// For camera control
	//

	/*!
	* \brief control the camera movement (Pan/Tilt/Zoom/Focus)
	* \param SsmdPtzSettings pstPtzSettings - described the movement
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT CameraMove (
		SsmdCaptureSettings *pstCaptureSettings);


	/*!
	* \brief select video source with specified mode
	* \param uint8_t un8VideoSrcDev - video source 
	* \param uint8_t un8VideoSrcMode - video source mode
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT CameraSelect(
		uint8_t un8VideoSrcDev,
		uint8_t un8VideoSrcMode);

	/*!
	* \brief get the current camera parameters
	* \param SsmdCameraParams * pstCameraParams - camera parameters to get 

	* \retval 0 - success, otherwize error occured
	*/
	HRESULT CameraParamGet (
		SsmdCameraParam * pstCameraParams);

	/*!
	* \brief set the current camera parameters
	* \param SsmdCameraParams * pstCameraParams - camera parameters to set 
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT CameraParamSet (
		SsmdCameraParam * pstCameraParams);

	//
	// Control
	//
	/*!
	* \brief Get control messages from device drvier
	* \param uint32_t un32Msg - control message
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AsyncMessageGet (
		uint32_t &un32Msg);


	/*!
	* \brief set the indicator, including Ring, Buzzer, Power LED, Status LED, Camera Active LED, Light Ring LEDs.
	* \param SsmdIndicatorSettings * pstIndicatorSetting - Indicator parameters to set
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT IndicatorSettingsSet (
		const SsmdIndicatorSettings * pstIndicatorSetting);

	HRESULT IndicatorSettingsGet (
		SsmdIndicatorSettings * pstIndicatorSetting);

	/*!
	* \brief set the camera on/off
	* \param EstiSwitch eSwitch - camera on/off to set
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT CameraSet (
		EstiSwitch eSwitch);


	/*!
	* \brief get the reset button status
	* \param EstiButtonState &eButtonStatus -  reset button status to get
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT ResetButtonStateGet (
		EstiButtonState &eButtonStatus);
	
private:

	union TVideoPipeSubtasks
	{
		int iAll;
		struct
		{
			int bDisplayTask : 1;
			int bCaptureTask : 1;
			int bVideoEncodeTask : 1;
			int bVideoDecodeTask : 1;
		} tasks;
	};

	
	TVideoPipeSubtasks m_sTasksRunning;
	std::condition_variable m_tTasksShutdownCond;

	static stiHResult ThreadCallback (
		int32_t n32Message,
  		size_t MessageParam,
		size_t CallbackParam);
	
	EstiResult ErrorReport (
		const SstiErrorLog *pstErrorLog);

	static EstiResult VideoPlaybackKeyframeRequest (
		void *pObject);
	
	VideoPlaybackKeyframeRequestCB m_pfnVideoPlaybackKeyframeRequestCB;
	void *m_pVideoPlaybackKeyframeRequestCBData;
	
	static EstiResult VideoPlaybackKeyframeReceived (
		void *pObject);
			
	VideoPlaybackKeyframeReceivedCB m_pfnVideoPlaybackKeyframeReceivedCB;
	void *m_pVideoPlaybackKeyframeReceivedCBData;
	
	int m_hRCUDevice;
	int m_hSlicDevice;

	SsmdIndicatorSettings m_stIndicatorSettings;
};

#endif
