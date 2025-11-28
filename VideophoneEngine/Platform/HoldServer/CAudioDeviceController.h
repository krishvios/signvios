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

#ifndef CAUDIODEVICECONTROLLER_H
#define CAUDIODEVICECONTROLLER_H

#include "stiDefs.h"
#include "stiAssert.h"
#include "CDeviceController.h"
#include "AudioControl.h"

//
// Constants
//
typedef enum EsmdAudioCommand
{
	esmdAUDIO_SET_ENCODER = 0,
	esmdAUDIO_SET_ENCODE_SETTINGS = 1,
	esmdAUDIO_START_RECORD = 2,
	esmdAUDIO_STOP_RECORD = 3,

	esmdAUDIO_SET_DECODER = 4,
	esmdAUDIO_SET_DECODE_SETTINGS = 5,
	esmdAUDIO_START_PLAYBACK = 6,
	esmdAUDIO_STOP_PLAYBACK = 7,
	
	esmdAUDIO_SET_ECHO_MODE = 8,

} EsmdAudioCommand;

//
// Typedefs
//
typedef struct SsmdAudioEncodeSettings
{
	unsigned short un16SamplesPerPacket;   // Samples per audio frame/packet
} SsmdAudioEncodeSettings;

// 
// Forward Declarations
//

//
// Globals
//

class CAudioDeviceController : public CDeviceController
{
public:
	
	CAudioDeviceController ();
	virtual ~CAudioDeviceController ();
	
	/*!
    * \brief Retrieves the only reference to the audio device controller (singleton obj support)
    */
	static CAudioDeviceController * GetInstance();

	virtual HRESULT DeviceOpen ();

	virtual HRESULT DeviceClose ();
	
	//
	// Playback
	// 

	/*!
	* \brief Set Codec Mode for audio playback
	* \param EstiAudioCodec eAudioCodec - audio codec to set
	* \retval 0 - success, otherwize error occured
	*/	
	HRESULT AudioPlaybackSetCodec (
		EstiAudioCodec eAudioCodec);

	/*!
	* \brief Get current Codec Mode in audio playback device
	* \param EstiAudioCodec &eAudioCodec - audio codec to get
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioPlaybackGetCodec (
		EstiAudioCodec &eAudioCodec);
	
	/*!
	* \brief Write a audio frame to audio device for playback
	* \param SsmdAudioFrame * pNewPacket - pointer to audio buffer for playback
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame);

	/*!
	* \brief Set to inform that the audio playback starts
	* \param NONE
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioPlaybackStartCall ();

	/*!
	* \brief Set to inform that the audio playback stops
	* \param NONE
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioPlaybackStopCall ();  

	//
	// Record
	//
	/*!
	* \brief Set Codec Mode for audio record
	* \param EstiAudioCodec eAudioCodec - audio codec to set
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioRecordSetCodec (
		EstiAudioCodec eAudioCodec);   

	/*!
	* \brief set setting to audio device for recording
	* \param SsmdAudioEncodeSettings * pAudioSettings - pointer to an audio setting structure 
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioRecordSetSettings (
		SsmdAudioEncodeSettings * pAudioSettings);

	/*!
	* \brief Read a compressed audio frame from audio device
	* \param SsmdAudioFrame * pAudioFrame - pointer to audio buffer for record 
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioRecordPacketGet (
		SsmdAudioFrame * pAudioFrame);
	
	/*!
	* \brief Set the echo cancellation on/off to audio device for recording
	* \param EsmdEchoMode eEchoMode - echo cancellation mode to set
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioRecordSetEchoMode (
		EsmdEchoMode eEchoMode);

	/*!
	* \brief Set to inform that the audio record starts
	* \param NONE
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioRecordStartCall ();

	/*!
	* \brief Set to inform that the audio record stops
	* \param NONE
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT AudioRecordStopCall ();  

	//
	// Control
	//

	/*!
	* \brief Get control messages from device drvier to CM/DTs
	* \param uint32_t un32CtrlMsg - control message
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT ControlMsgGet (
		EsmdControlMsg &eCtrlMsg);

	/*!
	* \brief Generate the DTMF tone based on the DTMF digit passed in
	* \param EstiDTMFDigit eDTMFDigit - the DTMF digit
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT DTMFToneSet (
		EstiDTMFDigit eDTMFDigit);
		
		
	HRESULT SoundcardSetup (int bStereo, int nFormat, int nSampleRate);
	
private:
	
	int m_nOpenCount;
	
};

#endif
