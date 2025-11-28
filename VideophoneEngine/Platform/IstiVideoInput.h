/*!
 *
 *\file IstiVideoInput.h
 *\brief Declaration of the Video Input interface 
 * 
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2010-2013 by Sorenson Communications, Inc. All rights reserved.
 */
#ifndef ISTIVIDEOINPUT_H
#define ISTIVIDEOINPUT_H

// Includes
//
#include "stiSVX.h"
#include "stiError.h"

#include "ISignal.h"
#include "VideoControl.h"
#include "VideoFrame.h"

#include <list>
#include <vector>

//
// Constants
//

//
// Typedefs
//
	
/*! 
* \struct SsmdPacketInfo
* 
* \brief This structure defines header that is prepended to all video packets
* 
*/ 
typedef struct SsmdPacketInfo
{
	uint32_t size;		//!< Size of packet including this header
	uint32_t f;		//!< 0 = GOB-fragmented, 1 = MB-fragmented
	uint32_t p;		//!< 0 = normal I or P, 1 = PB frame
	uint32_t sBit;		//!< Number of bits to ignore at the start of this packet
	uint32_t eBit;		//!< Number of bits to ignore at the start of this packet
	uint32_t src;		//!< Source format defined by H.263
	uint32_t i;		//!< 1 = Intra frame, 0 = other
	uint32_t a;		//!< 1 = Advanced prediction, 0 = off
	uint32_t s;		//!< 1 = Sytax based Arithmetic Code, 0 = off
	uint32_t dbq;		//!< Differential quant (DBQUANT), 0 if PB frame option is off.
	uint32_t trb;		//!< Temporal reference for B frame, 0 if PB frame option is off
	uint32_t tr;		//!< Temporal reference for P frame, 0 if PB frame option is off
	uint32_t gobN;		//!< GOB number in effect at the start of the packet
	uint32_t mbaP;		//!< Starting MB of this packet
	uint32_t quant;	//!< Quantization value at packet start
	uint32_t hMv1;		//!< Horizontal MV predictor
	uint32_t vMv1;		//!< Vertical MV predictor
	uint32_t hMv2;		//!< Horizontal 4MV predictor, 0 if advanced prediction is off
	uint32_t vMv2;		//!< Vertical 4MV predictor, 0 if advanced prediction is off
} SsmdPacketInfo;

enum EstiVideoFrameFormat
{
	estiLITTLE_ENDIAN_FOUR_BYTE_ALIGNED = 0,
	estiLITTLE_ENDIAN_PACKED = 1,
	estiBIG_ENDIAN_PACKED = 2,
	estiBYTE_STREAM = 3, // No NAL Unit sizes present.  Each NAL Unit is preceded by a byte stream header (00 00 00 01)
	estiPACKET_INFO_FOUR_BYTE_ALIGNED = 4,
	estiPACKET_INFO_PACKED = 5,
	estiRTP_H263_RFC2190 = 6,
	estiRTP_H263_RFC2429 = 7,
	estiRTP_H263_MICROSOFT = 8,
	estiRTP_H263_RFC2190_FOUR_BYTE_ALIGNED = 9
};

/*! 
*  \brief Video frame record management
*/ 

struct SstiRecordVideoFrame : public vpe::VideoFrame
{
	uint32_t unBufferMaxSize{0};		//!< total size of the allocated buffer
	EstiVideoFrameFormat eFormat{estiLITTLE_ENDIAN_PACKED};	//!< The format of the frame
	uint64_t timeStamp = 0;			//!< The time stamp for the frame in nano seconds.
};

struct SstiCapturedVideoImage
{
	enum Format
	{
		YUV422
	};

	Format format;
	uint32_t unWidth;
	uint32_t unHeight;
	uint8_t* pun8Image;
	uint32_t unImageByteSize;
};

typedef struct SstiImageCaptureSize
{
	int32_t n32Xorigin;
	int32_t n32Yorigin;
	int32_t n32Width;
	int32_t n32Height;
} SstiImageCaptureSize;


//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

/*!
 * \ingroup PlatformLayer
 * \brief This class represents the video input device.
 */
class IstiVideoInput
{
public:

	IstiVideoInput (const IstiVideoInput &other) = delete;
	IstiVideoInput (IstiVideoInput &&other) = delete;
	IstiVideoInput &operator= (const IstiVideoInput &other) = delete;
	IstiVideoInput &operator= (IstiVideoInput &&other) = delete;

	/*! 
	 * \struct SstiVideoRecordSettings
	 *  
	 * \brief This structure defines the common encoder settings for all video codecs 
	 */
	struct SstiVideoRecordSettings
	{
		unsigned int unMaxPacketSize = 0;       //!< Max size of each video packet (slice)
		unsigned int unTargetBitRate = 0;       //!< Target Bitrate (kbps)
		unsigned int unTargetFrameRate = 0;     //!< Target frame rate (used for bitrate calculations)
		unsigned int unIntraRefreshRate = 0;    /*!< \brief How often to produce intra macroblocks.
											 *
											 * Each macroblocks will be encoded as intra mode
											 * every unIntraRefreshRate frames
											 * 0 = no intra blocks, 
											 * 1 = all intra
											 * 5 = almost all intra blocks
											 * 132 = some intra blocks, 
											 * 1000 = very few intra blocks
											 */
		unsigned int unIntraFrameRate = 0;      /*!< \brief How often to produce intra frames
											 *
											 * An intra frame is coded every
											 * unIntraFrameRate frame(s)
											 * (0=no automatic I-frames)
											 */
		unsigned int unRows = 0;                //!< Height of the source frame.
		unsigned int unColumns = 0;             //!< Width of the source frame.
		EstiPacketizationScheme ePacketization = estiPktUnknown; //!<Packetization Scheme Supported
		EstiVideoCodec codec {estiVIDEO_NONE};
		EstiVideoProfile eProfile = estiPROFILE_NONE;			//!< Profile to be used.
		unsigned int unConstraints = 0;			//!< Constraints that go along with the profile.
		unsigned int unLevel = 0;				//!< Level to be used with the encoder
		unsigned int remoteDisplayWidth = 0;  //!< Display width remote endpoint wants
		unsigned int remoteDisplayHeight = 0; //!< Display height remote endpoint wants
		bool recordFastStartFile = false;	  //!< Specifies if the recorded file should be a fast start file

		bool operator != (const SstiVideoRecordSettings &other)
		{
			return (unMaxPacketSize != other.unMaxPacketSize
			 || unTargetBitRate != other.unTargetBitRate
			 || unTargetFrameRate != other.unTargetFrameRate
			 || unIntraRefreshRate != other.unIntraRefreshRate
			 || unIntraFrameRate != other.unIntraFrameRate
			 || unRows != other.unRows
			 || unColumns != other.unColumns
			 || ePacketization != other.ePacketization
			 || eProfile != other.eProfile
			 || unConstraints != other.unConstraints
			 || unLevel != other.unLevel
			 || remoteDisplayWidth != other.remoteDisplayWidth
			 || remoteDisplayHeight != other.remoteDisplayHeight);
		}
	};

#ifdef stiENABLE_MULTIPLE_VIDEO_INPUT
	/*!
	 * \brief Create an instance of a video input driver
	 * 
	 * \remarks This method is only used by the Hold Server.
	 * 
	 * \return A pointer to the created instance of the Video Input driver. 
	 */
	static IstiVideoInput *instanceCreate(uint32_t callIndex);

	/*!
	 * \brief Release the video input driver instance
	 * 
	 * \remarks This method is only used by the Hold Server.
	 * 
	 * \return none
	 */
	virtual void instanceRelease () = 0;


#endif //stiENABLE_MULTIPLE_VIDEO_INPUT

#ifdef stiHOLDSERVER
	/*!
	 * \brief Inquire if the instance of the input driver is in use or not.
	 * 
	 * \remarks This method is only used by the Hold Server.
	 * 
	 * \return estiTRUE if in use, estiFALSE if not in use.
	 */
	virtual EstiBool InUseGet () = 0;

	/*!
	 * \brief Set the instance of the input driver to "in use".
	 * 
	 * \remarks This method is only used by the Hold Server.
	 * 
	 * \return none.
	 */
	virtual void InUseSet () = 0;

	/*!
	* \brief Stop the current playback and start playing media for the indicated Service support state.
	*
	* \remarks This method is only used by the Hold Server.
	*
	* \return stiRESULT_SUCCESS or an error code.
	*/
	virtual stiHResult HSEndService(EstiMediaServiceSupportStates eMediaServiceSupportState) = 0;

	virtual void ServiceStartStateSet(EstiMediaServiceSupportStates state) = 0;

#endif

	/*!
	 * \brief Get the Video Input driver instance 
	 * 
	 * \return A pointer to the Video Input driver
	 */
	static IstiVideoInput *InstanceGet ();

	/*!
	 * \brief Start Video recording on the device
	 * 
	 * \remarks This method informs the driver that it should start producing audio packets for the data task.
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult VideoRecordStart () = 0;

	/*!
	 * \brief Stop video recording on the device
	 * 
	 * \remarks This method informs the driver that it should no longer produce video packets for the data task.
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult VideoRecordStop () = 0;

	/*!
	 * \brief This tells the video driver values to use for video recording
	 *  
	 * \param pVideoRecordSettings - the structure containing the settings to be used
	 * by the video driver for recording.
	 *  
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult VideoRecordSettingsSet (
		const SstiVideoRecordSettings *pVideoRecordSettings) = 0;

	/*!
	 * \brief Retrieves a frame from the driver
	 * 
	 * \param pVideoFrame - a structure which the driver is to fill in with the frame and data about the frame.
	 * \remarks The buffer will contain the full frame, separated into packets with meta data about each packet 
	 * preceding it.
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult VideoRecordFrameGet (
		SstiRecordVideoFrame *pVideoFrame) = 0;

	/*!
	 * \brief Set privacy mode to be implemented by the driver.
	 * 
	 * \param bEnable - The desired setting (enabled or not)
	 * 
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult PrivacySet (
		EstiBool bEnable) = 0;

	/*!
	 * \brief Get Privacy mode currently in use by the driver.
	 * 
	 * \param pbEnabled - a pointer to a variable to be set with the current setting in use by the driver.
	 * 
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult PrivacyGet (
		EstiBool *pbEnabled) const = 0;

	/*!
	 * \brief Request a key frame from the driver
	 * 
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult KeyFrameRequest () = 0;

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

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	/*!
	 * \enum EstiVideoCaptureDelay
	 */
	enum EstiVideoCaptureDelay
	{
	 	eCaptureNow,
		eCaptureOnRecordStart
	};

	/*!
	 * \brief Requests a frame capture
	 *
	 * \return stiHResult
	 */
	virtual stiHResult FrameCaptureRequest (
		EstiVideoCaptureDelay eDelay,
		SstiImageCaptureSize *pImageSize) = 0;
#endif
	
public:
	// Event Signals
	virtual ISignal<>& usbRcuConnectionStatusChangedSignalGet() = 0;
	virtual ISignal<int>& focusCompleteSignalGet () = 0;
	virtual ISignal<bool>& videoPrivacySetSignalGet () = 0;
	virtual ISignal<>& videoRecordSizeChangedSignalGet () = 0;
	virtual ISignal<>& videoInputSizePosChangedSignalGet () = 0;
	virtual ISignal<const std::vector<uint8_t> &>& frameCaptureAvailableSignalGet () = 0;
	virtual ISignal<>& packetAvailableSignalGet () = 0;
	virtual ISignal<>& holdserverVideoCompleteSignalGet () = 0;
	virtual ISignal<>& captureCapabilitiesChangedSignalGet () = 0;
	virtual ISignal<>& cameraMoveCompleteSignalGet () = 0;
	virtual	ISignal<uint64_t, uint64_t>& videoRecordPositionSignalGet () = 0;
	virtual	ISignal<const std::string, const std::string>& videoRecordFinishedSignalGet () = 0;
	virtual	ISignal<>& videoRecordEncodeCameraErrorSignalGet () = 0;
	virtual	ISignal<>& videoRecordErrorSignalGet () = 0;

protected:

	IstiVideoInput () = default;
	virtual ~IstiVideoInput () = default;

private:

};

#endif //#ifndef ISTIVIDEOINPUT_H
