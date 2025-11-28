/*!
 * \file IstiCall.h
 * \brief Declaration of a call object for maintaining calls in the videophone.
 *
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2008-2020 by Sorenson Communications, Inc. All rights reserved.
 *
 */
#pragma once

#include <string>
#include <chrono>
#include "stiSVX.h"
#include <vector>
#include "CstiItemId.h"
#include "stiTrace.h"
#include "ISignal.h"
#include "stiConfDefs.h"

class ICallInfo;

/*! 
 * \enum EstiInContacts 
 *  
 * \brief 
 *  
 */
enum EstiInContacts
{
	estiInContactsFalse = 0,  //! In contacts FALSE
	estiInContactsTrue = 1,   //! In contacts TRUE
	estiInContactsUnknown = 2 //! In contacts Unknown
};

enum EstiMcuType
{
	estiMCU_NONE = 0x01,
	estiMCU_GENERIC = estiMCU_NONE << 1,// The MCU that we are connected with is NOT for Group Video Chat
	estiMCU_GVC = estiMCU_NONE << 2,	// The MCU that we are connected with is for Group Video Chat

	// Add any new types of MCU above this line
	estiMCU_ANY = 0xfffffe				// Use when calling the getter and it doesn't matter which type
};

/*!
 * \brief Contains video stat information
 */
struct SstiStatistics
{
	EstiVideoCodec		eVideoCodec = estiVIDEO_NONE;	//!< Video codec
	int nVideoWidth = 0;		//!< Width of the video
	int nVideoHeight = 0;		//!< Height of the video
	
	int nTargetFrameRate = 0; 		//!< Frame rate we are trying to achieve.
	int nTargetVideoDataRate = 0;		//!< Video data rate we are trying to achieve.
	
	float fActualFrameRate = 0;		//!< Frame rate we are achieving (allowing for a float value)
	int nActualFrameRate = 0; 			//!< Frame rate we are achieving. (depricated)
	int nActualVideoDataRate = 0;		//!< Video data rate we are achieving.
	int nActualAudioDataRate = 0;		//!< Audio data rate we are achieving.
	int nActualTextDataRate = 0;		//!< Text data rate we are achieving.
	uint32_t actualRtxVideoDataRate = 0;		//!< Retransmission data rate being used.
	uint32_t numberAudioPackets = 0;		//!< Number of audio packets seen.
	uint32_t audioPacketsLost = 0;		//!< Number of audio packets lost.
	EstiAudioCodec audioCodec = estiAUDIO_NONE;	//!< Audio codec
	int keyframes {0};
	int keyframeRequests {0};
};

struct RecordStatistics : public SstiStatistics
{
	int nackRequestsReceived {0};								///< The total number of retransmission packets received.
	int totalNackRequestsReceived {0};								///< The total number of retransmission packets received.

	int rtxPacketsSent {0}; ///< The total number of retransmission packets  requested.
	int totalRtxPacketsSent {0}; ///< The total number of retransmission packets  requested.
};

struct PlaybackStatistics : public SstiStatistics
{
	int packetsReceived {0};
	int totalPacketsReceived {0};

	int packetsLost {0};
	int actualPacketsLost {0};
	int totalPacketsLost {0};

	int nackRequestsSent {0};									///< The total number of NACK requests sent.
	int totalNackRequestsSent {0};								///< The total number of NACK requests sent.

	int rtxPacketsReceived {0};									///< The total number of NACK requests received.
	int totalRtxPacketsReceived {0};							///< The total number of NACK requests received.
};

/*!
 * \brief Contains video call stat information
 */
struct SstiCallStatistics
{
	PlaybackStatistics Playback;                                           ///< Playback Statistics
	RecordStatistics Record;                                             ///< Record Statistics

	uint32_t			un32ReceivedPacketsLostPercent = 0;                     ///< Percent received packets lost (depricated)
	float				fReceivedPacketsLostPercent = 0;                        ///< Percent received packets lost (allow for fraction)
	
	//Flow control data...over entire call
	uint32_t			un32MinSendRate = 0;
	uint32_t			un32MaxSendRateWithAcceptableLoss = 0;
	uint32_t			un32MaxSendRate = 0;
	uint32_t			un32AveSendRate = 0;

	// The rest of the values are accumulated over the duration of call activity (reset when put on hold and resumed).
	double	 			dCallDuration = 0;                                      ///< Call duration (in seconds)
	uint32_t			videoPacketQueueEmptyErrors = 0;       		            ///< Number of times there were no available buffers to read into
	uint32_t			videoPacketsRead = 0;                       	        ///< Packets read
	uint32_t			videoUnknownPayloadTypeErrors = 0;                      ///< Unknown payload type errors
	uint32_t			videoKeepAlivePackets = 0;								///< Number of video keep alive packets
	uint32_t			videoPacketsDiscardedPlaybackMuted = 0;					///< Number of packets discarded because the playback data task is muted
	uint32_t			videoPacketsDiscardedReadMuted = 0;						///< Number of packets discarded because the read data task is muted
	uint32_t			videoPacketsDiscardedEmpty = 0;							///< Number of video packets with no payload
	uint32_t			videoPacketsUsingPreviousSSRC = 0;						///< Number of video packets using the previous SSRC
	uint32_t			un32OutOfOrderPackets = 0;                              ///< Out of order packets
	uint32_t			un32MaxOutOfOrderPackets = 0;                           ///< Maximun out of order packets
	uint32_t			un32DuplicatePackets = 0;                               ///< Duplicate packets
	uint32_t			un32PayloadHeaderErrors = 0;                            ///< Payload header errors
	uint32_t			un32VideoPlaybackFrameGetErrors = 0;                    ///< Number of VideoPlaybackGet errors (platform is too slow processing frames)
	uint32_t			un32KeyframesRequestedVP = 0;                           ///< Keyframes requested by local system (of the remote system)
	uint32_t 			un32KeyframeMaxWaitVP = 0;                              ///< The maximum time waited for a keyframe from the remote system (in milliseconds)
	uint32_t 			un32KeyframeMinWaitVP = 0;                              ///< The minimum time waited for a keyframe from the remote system (in milliseconds)
	uint32_t 			un32KeyframeAvgWaitVP = 0;                              ///< The average time waited for a keyframe from the remote system (in milliseconds)
	float				fKeyframeTotalWaitVP = 0;                               ///< The total time spent waiting for keyframes from the remote system (in seconds)
	uint32_t			un32KeyframesRequestedVR = 0;                           ///< Keyframes requested by remote system (of the local system)
	uint32_t 			un32KeyframeMaxWaitVR = 0;                              ///< The maximum time waited for a keyframe from the RCU (in milliseconds)
	uint32_t 			un32KeyframeMinWaitVR = 0;                              ///< The minimum time waited for a keyframe from the RCU (in milliseconds)
	uint32_t 			un32KeyframeAvgWaitVR = 0;                              ///< The average time waited for a keyframe from the RCU (in milliseconds)
	uint32_t			un32KeyframesReceived = 0;                              ///< Keyframes received from the remote system
	uint32_t			un32KeyframesSent = 0;                                  ///< Keyframes sent to the remote system
	uint32_t			videoPacketsSent = 0;									///< Video packets sent
	uint32_t			videoPacketSendErrors = 0;								///< Number of video packet send errors
	uint32_t			un32ErrorConcealmentEvents = 0;                         ///< Error Concealment Events that have occurred (playback)
	uint32_t			un32PFramesDiscardedIncomplete = 0;                     ///< P-frames that have been discarded due to incomplete (playback)
	uint32_t			un32IFramesDiscardedIncomplete = 0;                     ///< I-frames that have been discarded due to incomplete (playback)
	uint32_t			un32GapsInFrameNumber = 0;                              ///< The count of gaps in frame numbers observed (playback)
	uint32_t 			un32FrameCount = 0;                                     ///< The number of frames received and sent to the decoder (playback)
	uint32_t 			un32FramesLostFromRcu = 0;                              ///< The number of frames expected but never delivered from the RCU
	uint32_t			aun32PacketPositions[MAX_STATS_PACKET_POSITIONS] = {};   ///< Packet positions
	bool				bTunneled = false;
	uint32_t 			un32ShareTextCharsSent = 0;                             ///< The number of share text chars sent
	uint32_t 			un32ShareTextCharsReceived = 0;                         ///< The number of share text chars received
	uint32_t 			un32ShareTextCharsSentFromSavedText = 0;                ///< The number of share text chars sent
	uint32_t			un32ShareTextCharsSentFromKeyboard = 0;					///< The number of share text chars sent using some kind of keyboard
	uint32_t			countOfCallsToPacketLossCountAdd = 0;
	uint32_t			highestPacketLossSeen = 0;
	uint32_t			countOfPartialFramesSent = 0;
	std::chrono::milliseconds videoTimeMutedByRemote = std::chrono::milliseconds::zero ();				///< Total time video has been muted by either privacy or hold

	uint32_t			videoFramesSentToPlatform = 0;						///< The number of frames sent to platform (of any type, partial or whole)
	uint32_t			videoPartialKeyFramesSentToPlatform = 0;			///< The number of partial key frames sent to platform
	uint32_t			videoPartialNonKeyFramesSentToPlatform = 0;			///< The number of partial non-key frames sent to platform
	uint32_t			videoWholeKeyFramesSentToPlatform = 0;				///< The number of whole key frames sent to platform
	uint32_t			videoWholeNonKeyFramesSentToPlatform = 0;			///< The number of whole non-key frames sent to platform
	
	uint32_t			rtxPacketsIgnored = 0;								///< The total number of NACK packets ignored due to time they were last sent
	uint32_t			rtxPacketsNotFound = 0;								///< The total number of retransmission packets requested but not found to be retransmitted.
	uint32_t			rtxKbpsSent = 0;								///< The amount of data sent to the remote endpoint.
	uint32_t			avgPlaybackDelay = 0;								///< The average delay added to playback during a call.
	uint32_t			actualVPPacketLoss = 0;								///< The amount of packets lost before repair/retransmission.
	uint32_t			duplicatePacketsReceived = 0;								///< The number of duplicate packets received.
	uint32_t			rtxFromNoData = 0;								///< The number of packets we send a NACK anticipating loss due to delay in receiving data.
	uint32_t			burstPacketsDropped = 0;								///< The number of packets we dropped due to a burst of data.

	uint32_t 			avgRtcpJitter = 0;								// Average jitter reported through RTCP.
	uint32_t 			avgRtcpRTT = 0;								// Average RTT reported throught RTCP.
};

/*!
 * \brief Contains contact info being shared
 */
enum EPhoneNumberType // This is the same as in CoreServices/CstiContactListItem.cpp
{
	ePHONE_HOME = 0,
	ePHONE_WORK,
	ePHONE_CELL,
	ePHONE_UNKNOWN,

	ePHONE_MAX
};
 
struct SstiSharedContact
{
	std::string Name;
	std::string CompanyName;
	std::string DialString;
	EPhoneNumberType eNumberType{ePHONE_HOME};
};

/*!
 * \ingroup VideophoneEngineLayer
 * \brief This class represents a call.
 */
class IstiCall
{
public:

	IstiCall (const IstiCall &other) = delete;
	IstiCall (IstiCall &&other) = delete;
	IstiCall &operator= (const IstiCall &other) = delete;
	IstiCall &operator= (IstiCall &&other) = delete;

	//
	// Action methods
	//
	/*!
	 * \brief Answers the call
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult Answer () = 0;
	
	/*!
	 * \brief 
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult ContinueDial () = 0;
	
	/*!
	 * \brief Hangs up call
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult HangUp () = 0;
	
	/*!
	 * \brief Places call on Hold
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult Hold () = 0;
	
	/*!
	 * \brief Resumes call from hold
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult Resume () = 0;
	
	/*!
	 * \brief Rejects an incoming call
	 * 
	 * \param eCallResult The reason the call is being rejected
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult Reject (
		EstiCallResult eCallResult) = 0;

	/*!
	 * \brief Flashes the remote light ring. 
	 * This is used to alert callee while in a call. 
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult RemoteLightRingFlash () = 0;

	virtual void DtmfToneSend (
		EstiDTMFDigit eDTMFDigit) = 0;
	/*!
	 * \brief Transfers current call to specified number
	 *
	 * \param pszDialString The number to transfer to
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult Transfer (
		const std::string &dialString) = 0;
	/*!
	 * \brief Is the call able to be transferred
	 * 
	 * \return True if the call is transferable. 
	 */
	virtual EstiBool IsTransferableGet () const = 0;

	/*!
	 * \brief Is the call connected with an MCU of type eType?
	 *
	 * \param unType - The type of MCU to check for.  Pass in estiMCU_ANY if
	 * 				the type doesn't matter.  This can be the ORed product of
	 * 				two or more types also (e.g. estiMCU_NONE | estiMCU_GVC).
	 * \return True if connected with an MCU of the EstiMcuType passed in.
	 */
	virtual EstiBool ConnectedWithMcuGet (unsigned int unType) const = 0;

	//
	// Informational methods
	//
	/*!
	 * \brief Is remote video privacy enabled
	 *
	 * \ return estiON if the remote endpoint has video privacy
	 *          enabled and estiOFF otherwise.
	 */
	virtual inline EstiSwitch VideoPlaybackMuteGet () = 0;
	
	/*!
	 * \brief Is sending text enabled for this call
	 *
	 * \ return estiTRUE if the remote endpoint has the ability 
	 *          to receive text.
	 */
	virtual EstiBool TextSupportedGet () const = 0;
	
	/*!
	 * \brief Is sending DTMF enabled for this call
	 *
	 * \ return estiTRUE if the remote endpoint has the ability
	 *          to receive DTMF.
	 */
	virtual EstiBool DtmfSupportedGet () const = 0;
	
	/*!
	 * \brief Is a hang up allowed
	 * 
	 * \return True if the application is allowed to hanup the call
	 */
	virtual EstiBool AllowHangUpGet () const = 0;
	
	/*!
	 * \brief Gets the name of the person the remote endpoint is calling
	 * 
	 * \param pCalledName The name of the person the remote endpoint is calling
	 */
	virtual void CalledNameGet (std::string *pCalledName) const = 0;

	/*!
	 * \brief Gets a boolean indicating whether the call was started as a Direct SignMail
	 *
	 * \return True if the call is a Direct SignMail
	 */
	virtual bool directSignMailGet () const = 0;

	/*!
	 * \brief Call direction
	 * 
	 * \return The direction of the call: inbound or outbound 
	 */
	virtual EstiDirection DirectionGet () const = 0;
	
	/*!
	 * \brief Is the call able to be placed on hold
	 * 
	 * \return True if the call is holdable. 
	 */
	virtual EstiBool IsHoldableGet () const = 0;
	
	/*!
	 * \brief What protocol is being used for this call? 
	 *  
	 * Is this call using SIP or SIPS
	 * 
	 * \return The conferening protocol (SIPS or SIP)
	 */
	virtual EstiProtocol ProtocolGet () const = 0;
	
	/*!
	 * \brief Determine product type
	 * 
	 * \param nDeviceType 
	 * 
	 * \return True if the remote device is of the type passed in the nDeviceType parameter. 
	 */
	virtual EstiBool RemoteDeviceTypeIs (
		int nDeviceType) const = 0;
		
	/*!
	 * \brief How was call dialed?
	 * 
	 * \return The dial method used to dial the call. 
	 */
	virtual EstiDialMethod DialMethodGet () const = 0;
	
	/*!
	 * \brief What is the dial string for this call?
	 * 
	 * \param peDialMethod The dial method
	 * \param pDialString The dial string
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult DialStringGet (
		EstiDialMethod *peDialMethod,
		std::string *pDialString) const = 0;
		
	/*!
	 * \brief New Dial String Get
	 * 
	 * \param pDialString The dial string
	 */
	virtual void NewDialStringGet (
		std::string *pDialString) const = 0;
	
	/*!
	 * \brief Get the remote name
	 * 
	 * \param pRemoteName The remote name.  This will be the name from the contact list,
	 * the remote user name or the name from the call history list if the call was dialed from the call history list.
	 */
	virtual void RemoteNameGet (
		std::string *pRemoteName) const = 0;

	/*!
	 * \brief get the alternate remote name
	 * 
	 * \param pRemoteAlternateName The alternate name of the remote use (typically this is interpreter id)
	 */
	virtual void RemoteAlternateNameGet (
		std::string *pRemoteAlternateName) const = 0;

	/*!
	 * \brief Get the remote interface mode
	 * 
	 * \return The interface mode of the remote endpoint 
	 */
	virtual EstiInterfaceMode RemoteInterfaceModeGet () const = 0;

	/*!
	 * \brief Get the remote IP Address
	 * 
	 * \param pRemoteIpAddress The IP address of the remote endpoint.
	 */
	virtual std::string RemoteIpAddressGet () const = 0;

	/*!
	* \brief Get the remote Product Name
	*
	* \param pRemoteProductName The ProductName/UserAgent of the remote endpoint.
	*/
	virtual void RemoteProductNameGet(std::string *pRemoteProductName) const = 0;

	/*!
	* \brief Get the remote DialString
	*
	* \param pRemoteDialString The DialString of the remote endpoint.
	*/
	virtual void RemoteDialStringGet(std::string *pRemoteDialString) const = 0;

	/*!
	 * \brief Get the call duration
	 * 
	 * \return double
	 */
	virtual double CallDurationGet () = 0;

	/*!
	 * \brief Set local VCO active
	 */
	virtual void LocalIsVcoActiveSet (bool isVcoActive) = 0;

	/*!
	 * \brief Get the call result
	 * 
	 * \return The call result or why the call was disconnected
	 */
	virtual EstiCallResult ResultGet () const = 0;

	/*!
	 * \brief Get the Name
	 * 
	 * \return The name of the call result
	 */
	virtual std::string ResultNameGet () const = 0;

	/*!
	 * \brief Result sent to VRCL
	 * 
	 * \return Whether or not the result was sent to VRCL.  User interfaces can use this to determine if
	 * a message should be displayed or not. 
	 */
	virtual EstiBool ResultSentToVRCLGet () const = 0;

	/*!
	 * \brief Get the call state
	 * 
	 * \return The curretn state of the call. 
	 */
	virtual EsmiCallState StateGet () const = 0;

	/*!
	 * \brief Get the "in contacts" state
	 *
	 * \return Indicates whether or nto the remote use is in the local user's contact list.
	 */
	virtual EstiInContacts IsInContactsGet () const = 0;

	/*!
	 * \brief Retrieves the ItemId of the contact list item
	 *
	 * \return The ItemId of the contact
	 */
	virtual CstiItemId ContactIdGet() const = 0;

	/*!
	 * \brief Retrieves the ItemId of the phone number within the contact
	 *
	 * \return The ItemId of the phone number in a contact
	 */
	virtual CstiItemId ContactPhoneNumberIdGet() const = 0;

	/*!
	 * \brief Retrieves whether or not SignMail is available to be sent.
	 *
	 * \return True if SignMail is available.  Otherwise, false.
	 */
	virtual bool IsSignMailAvailableGet () const = 0;

	/*!
	 * \brief Skips from dialing and goes directly to recording a SignMail.
	 *
	 */
	virtual stiHResult SignMailSkipToRecord () = 0;


	/*!
	 * \brief Get the MessageGreetingType for this callee
	 *
	 * \return The greeting type
	 */
	virtual eSignMailGreetingType MessageGreetingTypeGet () const = 0;
	
	/*!
	 * \brief Gets the MessageGreetingText for this callee
	 *
	 * \Deprecated Applications should not use this method.
	 *
	 * \return The text for the greeting.
	 */
	virtual std::string MessageGreetingTextGet() const = 0;

	/*!
	 * \brief Gets the MessageGreetingURL for this callee
	 *
	 * \return The URL for the greeting.
	 */
	virtual std::string MessageGreetingURLGet() const = 0;
	
	/*!
	 * \brief Returns whether the SignMailBox is full or not.
	 *
	 * \return true if SignMailBox is full otherwise false
	 */
	virtual bool signMailBoxFullGet () const = 0;
		
	/*!
	 * \brief Retrieves whether or not SignMail has been initiated.
	 *
	 * \return True if SignMail has been initiated.  Otherwise, false.
	 */
	virtual bool SignMailInitiatedGet () const = 0;

	/*!
	 * \brief Get Video Record Size
	 * 
	 * \param un32Width The width of the video being sent to the remote endpoint.
	 * \param un32Height The height of the video being sent to the remote endpoint.
	 */
	virtual void VideoRecordSizeGet (
		uint32_t *un32Width,
		uint32_t *un32Height) const = 0;

	/*!
	 * \brief Get the video playback size
	 * 
	 * \param un32Width The width of the video being receieved from the remote endpoint.
	 * \param un32Height The height of the video being received from the remote endpoint.
	 */
	virtual void VideoPlaybackSizeGet (
		uint32_t *un32Width,
		uint32_t *un32Height) const = 0;

	/*!
	 * \brief Send text to the remote endpoint
	 * \param pun16Text The text to be sent (a null-terminated Unicode character string).
	 */
	virtual stiHResult TextSend (const uint16_t *pun16Text, EstiSharedTextSource sharedTextSource) = 0;
	
	/*!
	 * \brief Ask remote endpoint to remove their text share
	 *
	 * TODO VRCL has the ability to specify between dialed digits (DTMF) and text share
	 * as of now it is understood that they are mutually exclusive so this should
	 * clear whatever.  If it isn't the case in the future either two methods are needed
	 * or an enum value could perhaps be passed as a paremeter to differentiate which 
	 * text needs cleared.
	 */
	virtual void TextShareRemoveRemote() = 0;

	/*!
	 * \brief Get the remote call info
	 * 
	 * \return Additional call information. 
	 */
	virtual ICallInfo *RemoteCallInfoGet () = 0;

	/*!
	 * \brief Get the remote Pan Tilt Zoom capabilities.
	 * 
	 * \return The Pan/Tilt/Zoom capabilities of the remote endpoint (see ::EstiCameraControlBits). 
	 */
	virtual int RemotePtzCapsGet () const = 0;

	/*!
	 * \brief Get the call Statistics
	 * 
	 * \param pStats The statistics of the call.
	 */
	virtual void StatisticsGet (
		SstiCallStatistics *pStats) const = 0;

	/*!
	 * \brief Is the call conferenced?
	 * 
	 * \return True if the call reached a conferencing state. 
	 */
	virtual bool ConferencedGet () const = 0;
	
	/*!
	 * \brief Are we transferred by relay?
	 *
	 * \return True if the call was transferred by relay.
	 */
	virtual bool RelayTransferGet () const = 0;

	/*!
	 * \brief Are we transfered?
	 * 
	 * \return True if the call was transferred (the current session is not the original call). 
	 */
	virtual EstiBool TransferredGet () const = 0;

	/*!
	 * \brief Get the message record time
	 * 
	 * \return The maximum time in seconds to record a message. 
	 */
	virtual int MessageRecordTimeGet() = 0;

	/*!
	 * \brief Gets the CallId GUID
	 * 
	 * \return The CallId GUID if there is one, otherwise an empty string
	 */
	virtual void CallIdGet (std::string *pCallId) = 0;

	virtual int CallIndexGet () = 0;

	/*!
	 * \brief Gets the Intended Dial String
	 *
	 * \param Variable to store intended dial string
	 */
	virtual void IntendedDialStringGet (std::string *pOriginalDialString) const = 0;

	/*!
	 * \brief Shares contact information with the connected party
	 *
	 * \param The contact to be shared
	 */
	virtual stiHResult ContactShare (const SstiSharedContact&) = 0;
	
	/*!
	 * \brief Tells us whether contact sharing is enabled for the current call
	 *
	 * \param Variable to store intended dial string
	 */
	virtual bool ShareContactSupportedGet () = 0;
	
	/*!
	 * \brief Gets the hearings status for the current call.
	 */
	virtual EstiHearingCallStatus HearingCallStatusGet () = 0;
	
	/*!
	 * \brief Gets whether geolocation is requested for the current call.
	 */
	virtual bool geolocationRequestedGet () const = 0;
	
	/*!
	 * \brief Sets whether geolocation is requested for the current call.
	 */
	virtual void geolocationRequestedSet (bool geolocationRequested) = 0;
	
	/*!
	 * \brief Sets whether geolocation is available for the current call.
	 */
	virtual void geolocationAvailableSet (GeolocationAvailable geolocationAvailable) = 0;
	
	enum class DhviState
	{
		NotAvailable, // We checked the number and it is not available for DHVI.
		Capable, // We can create a DHVI call from this call.
		Connecting, // We are in the process of connecting a DHVI call.
		Connected, // We have a DHVI call established using this call.
		Failed, // Failed to request a conference room or connect to one.
	};
	
	virtual ISignal<DhviState>& dhviStateChangeSignalGet() = 0;
	
	virtual IstiCall::DhviState dhviStateGet () = 0;

	virtual std::shared_ptr<IstiCall> sharedPointerGet () = 0;
	
	virtual std::string vrsAgentIdGet () const = 0;
	
	virtual stiHResult dhviMcuDisconnect () = 0;

	virtual vpe::EncryptionState encryptionStateGet () const = 0;

	virtual std::string LocalPreferredLanguageGet() const = 0;

	virtual std::string RemotePreferredLanguageGet() const = 0;

	virtual uint32_t SubstateGet() const = 0;
	
	virtual const std::vector<SstiSipHeader>& additionalHeadersGet() const = 0;

	virtual void additionalHeadersSet(const std::vector<SstiSipHeader>& additionalHeaders) = 0;

	virtual void additionalHeadersSet(const std::vector<SstiSipHeader>&& additionalHeaders) = 0;


protected:
	IstiCall () = default;
	virtual ~IstiCall () = default;

};


struct SstiTextMsg
{
	IstiCall *poCall;
	uint16_t *pwszMessage;
};

