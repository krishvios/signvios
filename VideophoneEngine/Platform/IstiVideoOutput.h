/*!
 * \file IstiVideoOutput.h
 * \brief Interface for the Video Output
 *
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2009-2011 by Sorenson Communications, Inc. All rights reserved.
 */

#ifndef IstiVideoOutput_H
#define IstiVideoOutput_H

//
// Includes
//
#include "VideoControl.h"
#include "ISignal.h"
#include "stiSVX.h"
#include "IstiVideoPlaybackFrame.h"
#include <list>


//
// Forward Declarations
//

//
// Constants
//
#define NUM_DISPLAY_WINDOWS 4

#define MAX_DISPLAY_WINDOW0_WIDTH 1280
#define MAX_DISPLAY_WINDOW0_HEIGHT 720

// Since this window cannot be disabled
// we allow for a default size setting
// that will be enabled initially
#define DEFAULT_DISPLAY_WINDOW0_MODE eDISPLAY_MODE_COMPOSITE_720_BY_480

#define MAX_DISPLAY_WINDOW1_WIDTH 1280
#define MAX_DISPLAY_WINDOW1_HEIGHT 270

#define MAX_DISPLAY_WINDOW2_WIDTH 0
#define MAX_DISPLAY_WINDOW2_HEIGHT 0

#define MAX_DISPLAY_WINDOW3_WIDTH 0
#define MAX_DISPLAY_WINDOW3_HEIGHT 0

enum EDisplayModes
{
	eDISPLAY_MODE_UNKNOWN = 0,
	eDISPLAY_MODE_COMPOSITE_720_BY_480 = 0x1,
	eDISPLAY_MODE_HDMI_640_BY_480 = 0x2,
	eDISPLAY_MODE_HDMI_720_BY_480 = 0x4,
	eDISPLAY_MODE_HDMI_1280_BY_720 = 0x8,
	eDISPLAY_MODE_HDMI_1920_BY_1080 = 0x10
};

enum EDisplayWindow
{
	eDISPLAY_WINDOW0 = 0,
	eDISPLAY_WINDOW1 = 1,
	eDISPLAY_WINDOW2 = 2,
	eDISPLAY_WINDOW3 = 3
};

#if DEVICE == DEV_NTOUCH_VP2 || DEVICE == DEV_NTOUCH_VP3 || DEVICE == DEV_NTOUCH_VP4 || \
	DEVICE == DEV_X86 || DEVICE == DEV_NTOUCH_MAC || DEVICE == DEV_NTOUCH_PC
enum EstiScreenshotType
{
	eScreenshotFull = 0,
	eScreenshotSelf,
	eScreenshotRemote,

	eScreenshotNone
};
#endif

enum EPlayRate
{
	ePLAY_PAUSED = 0,
	ePLAY_NORMAL = 1,
	ePLAY_FAST = 2,
	ePLAY_SLOW = 3,
};

enum EstiH264FrameFormat
{
	eH264FrameFormatByteStream = 0,
	eH264FrameFormatNALUnitSizes = 1
};


//
// Forward Declarations
//


//
// Globals
//


/*!
 * \brief 
 * 
 * \param eDisplayMode 
 * \param pWidth 
 * \param pHeight 
 * 
 * \return stiHResult 
 */
stiHResult DisplayModeToResolution (
	EDisplayModes eDisplayMode,
	uint32_t * pWidth,
	uint32_t * pHeight);
		


/*! 
 * \ingroup PlatformLayer
 * \brief This class represents the video output device.
 * 
 */
class IstiVideoOutput
{
public:
	
	IstiVideoOutput (const IstiVideoOutput &other) = delete;
	IstiVideoOutput (IstiVideoOutput &&other) = delete;
	IstiVideoOutput &operator= (const IstiVideoOutput &other) = delete;
	IstiVideoOutput &operator= (IstiVideoOutput &&other) = delete;

	/*!
	* \enum EstiKeyframeNeededReason
	*/
	enum EstiKeyframeNeededReason
	{
		eOTHER_REASON,                         //! Some other reason that is undefined
		eERROR_CONCEALMENT_EVENT,              //! An error concealment event occurred
		eP_FRAME_DISCARDED_INCOMPLETE,         //! A P-frame was discarded because it was incomplete
		eI_FRAME_DISCARDED_INCOMPLETE,         //! A I-frame was discarded because it was incomplete
		eGAP_IN_FRAME_NUMBERS,                 //! A gap in frame numbers occurred
	};
	
	/*!
	* \enum EstiRemoteVideoMode
	*/
	enum EstiRemoteVideoMode
	{
		eRVM_BLANK,                         //! Blank view used when setting up or tearing down the remote view.
		eRVM_VIDEO,                         //! Displaying video
		eRVM_PRIVACY,                       //! Displaying video privacy image
		eRVM_HOLD,                          //! Displaying call on hold image
	};

	/*!
	 * \struct SstiImageSettings
	 *  
	 * \brief This structure defines a display image 
	 */
	struct SstiImageSettings
	{
		int bVisible;   			//! Visible?
		unsigned int unWidth;   	//! Width
		unsigned int unHeight;  	//! Height
		unsigned int unPosX;    	//! Pos X 
		unsigned int unPosY;    	//! Pos Y 
	};
	
	/*! 
	*  \enum  EDisplaySettingsMaskValues
	* 
	*  \brief Masking values
	*/ 
	enum EDisplaySettingsMaskValues 
	{
		eDS_SELFVIEW_VISIBILITY = 1,     //!   Selfview visibility 
		eDS_SELFVIEW_SIZE = 2,           //!   Selfview size 
		eDS_SELFVIEW_POSITION =  4,      //!   Selfview position 
		eDS_REMOTEVIEW_VISIBILITY = 8,   //!   Remoteview visibility
		eDS_REMOTEVIEW_SIZE = 16,        //!   Remoteview size 
		eDS_REMOTEVIEW_POSITION =  32,   //!   Remoteview position 
		eDS_TOPVIEW = 64                 //!   Topview
	};
	
	/*! 
	*  \enum EstiView
	* 
	*  \brief which view to control
	*/ 
	enum EstiView
	{
		eSelfView 	= 0,   //! Self View  
		eRemoteView	= 1    //! Remote View
	};
	
	/*! 
	 * \struct SstiDisplaySettings
	 *  
	 * \brief This structure defines the display settings 
	 */
	struct SstiDisplaySettings
	{
		uint32_t un32Mask;             //! Mask
		SstiImageSettings stSelfView;   //! Self View
		SstiImageSettings stRemoteView; //! Remote View
		EstiView eTopView;              //! Top View
	};

#ifdef stiENABLE_MULTIPLE_VIDEO_OUTPUT
	/*!
	* \brief Create instance
	*
	* \return IstiVideoOutput*
	*/
	static IstiVideoOutput *instanceCreate (
		uint32_t callIndex);

	/*!
	* \brief Delete instance
	*
	*/
	virtual void instanceRelease () = 0;
#endif
	
	/*!
	 * \brief Get instance 
	 * 
	 * \return IstiVideoOutput* 
	 */
	static IstiVideoOutput *InstanceGet ();

#if APPLICATION == APP_NTOUCH_VP2
	/*!
	 * \brief Get display mode capabilities
	 * 
	 * \param pun32CapBitMask 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult DisplayModeCapabilitiesGet (
		uint32_t *pun32CapBitMask) = 0;

	virtual stiHResult DisplayModeSet (
		EDisplayModes eDisplayMode) = 0;
		
	
#endif
		
#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP4
	virtual stiHResult DisplayResolutionSet (
		unsigned int width,
		unsigned int height) = 0;
#endif

	/*!
	 * \brief Get the display settings
	 * 
	 * \param pDisplaySettings 
	 */
	virtual void DisplaySettingsGet (
		SstiDisplaySettings * pDisplaySettings) const = 0;
	
	/*!
	 * \brief Set display settings
	 * 
	 * \param pDisplaySettings 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult DisplaySettingsSet (
	   const SstiDisplaySettings * pDisplaySettings) = 0;
	   
	/*!
	 * \brief Get the size of the recorded video
	 *
	 * \param pun32Width
	 * \param pun32Height
	 *
	 * \return stiHResult
	 */
	virtual stiHResult VideoRecordSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height) const
    {
		stiUNUSED_ARG (pun32Width);
		stiUNUSED_ARG (pun32Height);

        return stiRESULT_SUCCESS;
    }

	/*!
	 * \brief Set remote privacy view
	 * 
	 * \param bPrivacy 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult RemoteViewPrivacySet (
		EstiBool bPrivacy) = 0;
		
	/*!
	 * \brief Set remove view hold
	 * 
	 * \param bHold 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult RemoteViewHoldSet (
		EstiBool bHold) = 0;
		
	/*!
	 * \brief Get the video playback size
	 * 
	 * \param pun32Width 
	 * \param pun32Height 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height) const = 0;

	/*!
	 * \brief Set video playback codec
	 * 
	 * \param eVideoCodec 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec) = 0;
		
	/*!
	 * \brief Start Video playback
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackStart () = 0;

	/*!
	 * \brief Stop video playback
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackStop () = 0;

	/*!
	 * \brief Get a video frame
	 * 
	 * \param ppVideoFrame 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackFrameGet (
		IstiVideoPlaybackFrame **ppVideoFrame) = 0;
		
	/*!
	 * \brief Put Video playback
	 * 
	 * \param pVideoFrame 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackFramePut (
		IstiVideoPlaybackFrame *pVideoFrame) = 0;

	/*!
	 * \brief Retrieves the format of the H.264 frames that video output is expecting
	 *
	 * Platform implementations can return the type that best meets the needs of the
	 * hardware or software codecs to improve efficiency.
	 *
	 */
	virtual EstiH264FrameFormat H264FrameFormatGet ()
	{
		return eH264FrameFormatNALUnitSizes;
	}

	/*!
	 * \brief Discard a frame
	 * 
	 * \param pVideoFrame 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame) = 0;

	/*!
	 * \brief Get the device file descriptor
	 * 
	 * \return int 
	 */
	virtual int GetDeviceFD () const = 0;

	/*!
	 * \brief Get video codec 
	 * 
	 * \param pCodecs - a std::list where the codecs will be stored.
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs) = 0;

	/*!
	 * \brief Get Codec Capability 
	 * 
	 * \param eCodec 
	 * \param pstCaps 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps) = 0;

	/*!
	* \brief Set Codec Capability
	*
	* \param eCodec
	* \param pstCaps
	*
	* \return stiHResult
	*/
	virtual stiHResult CodecCapabilitiesSet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps)
	{
		stiUNUSED_ARG (eCodec);
		stiUNUSED_ARG (pstCaps);

		return stiRESULT_SUCCESS;
	}

		
#if DEVICE == DEV_NTOUCH_VP2 || DEVICE == DEV_NTOUCH_VP3 || DEVICE == DEV_NTOUCH_VP4 || DEVICE == DEV_X86
	/*!
	 * \brief Take a screen shot
	 */
	virtual void ScreenshotTake (EstiScreenshotType eType = eScreenshotFull) = 0;
#endif

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	/*!
	 * \brief Turn on/off TV thru HDMI
	 */
	virtual stiHResult DisplayPowerSet (bool bOn) = 0;
	/*!
	 * \brief Determine on/off TV state thru HDMI
	 */
	virtual bool DisplayPowerGet () = 0;
	/*!
	 * \brief return if CEC commands are supported
	 */
	 virtual bool CECSupportedGet() = 0;
	 
	/*!
	 * \brief Starts the video screen saver
	 */
	 virtual stiHResult VideoFilePlay(
		 const std::string &filePath) = 0;

	/*!
	 * \brief Stops the video screen saver
	 */
	 virtual stiHResult VideoFileStop() = 0;
#endif

#if (APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID)
	/*!
	 * \brief Notify orientation change.
	 */
	virtual void OrientationChanged () = 0;
#endif

	/*!
	 * \brief Get the preferred display size.
	 *
	 * A display size of 0, 0 indicates that the platform does not have a
	 * preferred display size.
	 */
	virtual void PreferredDisplaySizeGet(
		unsigned int *displayWidth,
		unsigned int *displayHeight) const = 0;

public:
	// Event Signals
	virtual ISignal<EstiRemoteVideoMode>& remoteVideoModeChangedSignalGet () = 0;
	virtual ISignal<uint32_t, uint32_t>& decodeSizeChangedSignalGet () = 0;
	virtual ISignal<>& videoRemoteSizePosChangedSignalGet () = 0;
	virtual ISignal<>& keyframeNeededSignalGet () = 0;
	virtual ISignal<bool>& cecSupportedSignalGet () = 0;
	virtual ISignal<>& hdmiOutStatusChangedSignalGet () = 0;
	virtual ISignal<>& videoScreensaverStartFailedSignalGet () = 0;
	virtual ISignal<>& preferredDisplaySizeChangedSignalGet () = 0;
	virtual ISignal<bool>& frameBufferReadySignalGet () = 0;
	virtual ISignal<>& videoOutputPlaybackStoppedSignalGet () = 0;

protected:

	IstiVideoOutput () = default;
	virtual ~IstiVideoOutput () = default;

private:

};

#endif //#ifndef IstiVideoOutput_H
