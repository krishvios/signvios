/*!
 * \file IstiAudioInput.h
 * 
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef ISTIAUDIOINPUT_H
#define ISTIAUDIOINPUT_H


// Includes
//
#include "stiSVX.h"
#include "stiError.h"
#include "ISignal.h"

#include "AudioControl.h"

#include <memory>

//
// Constants
//

//
// Typedefs
//

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
 * \brief This class represents the audio input device
 *  
 */
class IstiAudioInput
{
public:

	IstiAudioInput (const IstiAudioInput &other) = delete;
	IstiAudioInput (IstiAudioInput &&other) = delete;
	IstiAudioInput &operator= (const IstiAudioInput &other) = delete;
	IstiAudioInput &operator= (IstiAudioInput &&other) = delete;

	/*! 
	 * \struct SstiAudioRecordSettings 
	 *  
	 * \brief Contains samples per audio frame 
	 * or packet 
	 *  
	 */
	struct SstiAudioRecordSettings
	{
		unsigned short un16SamplesPerPacket = 0;   //! Samples per audio frame/packet
		unsigned int encodingBitrate = 0; 	   //! Encoding bitrate.
		unsigned int sampleRate = 0; 		   //! Encoding sample rate.
	};

	/*!
	 * \brief Obtain the handle to the AudioInput driver.  Through this handle, all other calls will be made. 
	 * 
	 * \return The handle to the AudioInput driver. 
	 */
	static IstiAudioInput *InstanceGet ();

	/*!
	 * \brief Start audio recording on the device.
	 * \remarks This method informs the driver that it should start producing audio packets for the data task.
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult AudioRecordStart () = 0;

	/*!
	 * \brief Stop audio recording on the device
	 * \remarks This method informs the driver that it should no longer produce audio packets for the data task.
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult AudioRecordStop () = 0;

	/*!
	 * \brief Retrieves a packet from the driver
	 * 
	 * \param pAudioFrame - a structure which the driver is to fill in with the packet and data about the packet.
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult AudioRecordPacketGet (
		SsmdAudioFrame * pAudioFrame) = 0;

	/*!
	 * \brief This tells the audio driver which codec to use to encode
	 * 
	 * \param eAudioCodec 
	 *  \li estiAUDIO_NONE
	 *  \li estiAUDIO_G711_ALAW
	 *  \li estiAUDIO_G711_MULAW
	 *  \li estiAUDIO_G722
	 *  \li esmdAUDIO_G723_5 (not currently supported)
	 *  \li esmdAUDIO_G723_6 (not currently supported)
	 *  \li estiAUDIO_RAW Raw Audio (8000 hz, signed 16 bits, mono)
	 *  
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult AudioRecordCodecSet (
		EstiAudioCodec eAudioCodec) = 0;
		
	/*!
	 * \brief This tells the audio driver values to use for audio recording
	 *  
	 * \param pAudioRecordSettings - the structure containing the settings to be used
	 * by the audio driver for recording.
	 *  
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult AudioRecordSettingsSet (
		const SstiAudioRecordSettings * pAudioRecordSettings) = 0;

	/*!
	 * \brief Set to turn on/off echo cancellation
	 * 
	 * \param eEchoMode - The echo cancellation mode desired.
	 * 
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult EchoModeSet (
		EsmdEchoMode eEchoMode) = 0;
		
	/*!
	 * \brief Sets audio privacy condition (for mute or unmute).
	 * 
	 * \param bEnable - a boolean to inform the audio driver to mute or not the audio channel.
	 * 
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult PrivacySet (
		EstiBool bEnable) = 0;
		
	/*!
	 * \brief Gets the current privacy status from the audio driver.
	 * 
	 * \param pbEnabled - This parameter will be set by the driver with the current privacy setting.
	 * 
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult PrivacyGet (
		EstiBool *pbEnabled) const = 0;

	/*!
	* \brief  Get the file descriptor for the audio Input driver
	* \remarks This method returns a file descriptor that can be used with select () to know when a packet
	* has been generated and is ready to be obtained by the audio record data task.
	* 
	* \return The file descriptor.
	 *
	 * NOTE: deprecated
	*/
	virtual int GetDeviceFD () const = 0;

	
	/*!
	 * \brief Get the maximum sample rate (in milliseconds) that we are capable of producing.
	 * \remarks This method is optional.  If not implemented, the value returned will be 30 milliseconds.
	 * 
	 * \return Returns the max sample rate for the g711 codec
	 */
	virtual int MaxPacketDurationGet () const {return 30;};

	/*!
	 * \brief Get the input audio levels received from the microphone
	 *
	 * \return The current audio level coming from the microphone
	 */
	virtual int AudioLevelGet () const {return 0;};

public:
	// Signals when a packet can be fetched (signals on state change only)
	virtual ISignal<bool>& packetReadyStateSignalGet () = 0;

	// Signals true when privacy is enabled, and false when it's disabled
	virtual ISignal<bool>& audioPrivacySignalGet () = 0;

protected:
	IstiAudioInput () = default;
	virtual ~IstiAudioInput () = default;
};

using IstiAudioInputSharedPtr = std::shared_ptr<IstiAudioInput>;

#endif //#ifndef ISTIAUDIOINPUT_H
