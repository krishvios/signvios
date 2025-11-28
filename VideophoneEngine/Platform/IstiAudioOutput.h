/*!
* \file IstiAudioOutput.h
* \brief Audio output control
* \brief This class handles decoding and playing audio received from the remote system.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#ifndef ISTIAUDIOOUTPUT_H
#define ISTIAUDIOOUTPUT_H

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
#define smdAUDIO_G711_MAX_SIZE		480		// Max supported G.711 frame size
#define smdAUDIO_G711_MIN_SIZE		80		// Min supported G.711 frame size

//
// Typedefs
//

/*! 
 * \struct SstiAudioDecodeSettings
 *  
 * \brief Decode settings read from the audio track and passed to decoder.
 */
struct SstiAudioDecodeSettings
{
	uint16_t channels = 0;		//! number of channels in the stream.
	uint16_t sampleRate = 0;	//! sample rate of the stream
};

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
 * \brief This class represents the audio output device
 *  
 */
class IstiAudioOutput
{
public:

	IstiAudioOutput (const IstiAudioOutput &other) = delete;
	IstiAudioOutput (IstiAudioOutput &&other) = delete;
	IstiAudioOutput &operator= (const IstiAudioOutput &other) = delete;
	IstiAudioOutput &operator= (IstiAudioOutput &&other) = delete;
	
	enum ECallbackType
	{
		eCALLBACK_DTMF = 1
	};
	

#ifdef stiENABLE_MULTIPLE_AUDIO_OUTPUT
	/*!
	* \brief Create instance
	*
	* \return IstiAudioOutput*
	*/
	static IstiAudioOutput *InstanceCreate (uint32_t nCallIndex);

	/*!
	* \brief Delete instance
	*
	*/
	static void InstanceDelete (IstiAudioOutput*);
#endif

	/*!
	 * \brief Obtain the handle to the AudioOutput driver.  Through this handle, all other calls will be made. 
	 * 
	 * \return The handle to the AudioOutput driver. 
	 */
	static IstiAudioOutput *InstanceGet ();
	
	/*!
	 * \brief Start audio playback.
	 * \remarks This method informs the driver that it should be ready to start receiving audio packets.
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult AudioPlaybackStart () = 0;

	/*!
	 * \brief Stops audio playback
	 * \remarks This method informs the driver that it should no longer expect to receive audio packets.
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult AudioPlaybackStop () = 0;

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4 || defined(stiMVRS_APP)
	/*!
	 * \brief Switches Audio output on and off
	 * 
	 * \param eSwitch 
	 * Valid switch types are: 
	 * 	\li estiON 
	 * 	\li estiOFF 
	 * 
	 * \return stiRESULT_SUCCESS or an error code. 
	 */
	virtual stiHResult AudioOut (
		EstiSwitch eSwitch) = 0;
#endif

	/*!
	 * \brief This tells the audio driver which codec to use to decode
	 * 
	 * \param eAudioCodec 
	 *  \li estiAUDIO_NONE
	 *  \li estiAUDIO_G711_ALAW
	 *  \li estiAUDIO_G711_MULAW
	 *  \li esmdAUDIO_G723_5 (not currently supported)
	 *  \li esmdAUDIO_G723_6 (not currently supported)
	 *  \li estiAUDIO_RAW Raw Audio (8000 hz, signed 16 bits, mono)
	 *  
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult AudioPlaybackCodecSet (
		EstiAudioCodec eAudioCodec) = 0;

	/*!
	 * \brief called for each packet of audio data to be sent
	 * 
	 * \param pAudioFrame - A structure containing the audio frame to be sent
	 * \param unSeqId - The RTP sequence number (not all implementations of this method use this parameter).
	 * 
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame) = 0;
		
		
	/*!
	 * \brief Forward a DTMF event to the platform layer to be handled.
	 * 
	 * \remarks When a DTMF event is received in the audio stream, the audio data task
	 * passes it onto the AudioOutput class to be handled by the platform layer.
	 * 
	 * \param eDtmfDigit - The DTMF digit received
	 * 
	 * \return none
	 */
	virtual void DtmfReceived (EstiDTMFDigit eDtmfDigit) = 0;
	
		
	/*!
	 * \brief  Get the file descriptor for the audio status
	 * \remarks This method returns a file descriptor that can be used with select () to know when status
	 * information has been written to the device and to be able to get the status of such.
	 * \remarks *** Note *** that while the method needs to be implemented, not all compilations of the data task
	 * actually use this file descriptor.  Look in CstiAudioPlayback.cpp to determine if your product will
	 * use this descriptor or not.
	 * 
	 * \return The file descriptor.
	 *
	 * NOTE: deprecated
	 */
	virtual int GetDeviceFD () = 0;

	/*!
	 * \brief Get the maximum sample rate (in milliseconds) that we are capable of.
	 * \remarks This method is optional.  If not implemented, the value returned will be 30 milliseconds.
	 * 
	 * \return Returns the max sample rate for the g711 codec
	 */
	virtual int MaxPacketDurationGet () const {return 30;};

	/*!
	 * \brief Get audio codecs supported
	 *
	 * \param pCodecs
	 * \param ePreferredCodec
	 *
	 * \return stiHResult
	 */
	virtual stiHResult CodecsGet (
		std::list<EstiAudioCodec> *pCodecs)
	{
		pCodecs->push_back (estiAUDIO_G711_MULAW);
		pCodecs->push_back (estiAUDIO_G711_ALAW);
		return stiRESULT_SUCCESS;
	};

#ifdef stiSIGNMAIL_AUDIO_ENABLED
	virtual stiHResult AudioPlaybackSettingsSet(
		SstiAudioDecodeSettings *pAudioDecodeSettings) = 0;
#endif

public:
	virtual ISignal<bool>& readyStateChangedSignalGet () = 0;  // Ready to receive more data packets to play
	virtual ISignal<EstiDTMFDigit>& dtmfReceivedSignalGet () = 0;

protected:

	IstiAudioOutput () = default;
	virtual ~IstiAudioOutput () = default;

private:
	
};

using IstiAudioOutputSharedPtr = std::shared_ptr<IstiAudioOutput>;

#endif //#ifndef ISTIAUDIOOUTPUT_H

