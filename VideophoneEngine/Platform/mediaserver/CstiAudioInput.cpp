#include "CstiAudioInput.h"
#ifndef WIN32
	#include <err.h>
#endif //WIN32

/*!
 * \brief Default Constructor
 */
CstiAudioInput::CstiAudioInput ()
	:
	m_bPrivacy (estiFALSE)
{
	stiOSSignalCreate (&m_fdPSignal);
}

CstiAudioInput::~CstiAudioInput ()
{
	stiOSSignalClose (&m_fdPSignal);
}


/*!
 * \brief Starts Audio Record
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

/*!
 * \brief Stops Audio Record
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


/*!
 * \brief Gets audio record packet
 * 
 * \param SsmdAudioFrame* pAudioFrame 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordPacketGet (
	SsmdAudioFrame * pAudioFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


/*!
 * \brief Sets audio record codec 
 * This sets which of the audio record 
 * codecs to be used for recording an audio 
 * stream 
 * 
 * \param EstiAudioCodec eAudioCodec 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordCodecSet (
	EstiAudioCodec eAudioCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

	
/*!
 * \brief Audio Record Setting Set 
 * uses the SstiAudioRecordSettings struct to setup 
 * the audio stream 
 * 
 * \param const SstiAudioRecordSettings* pAudioRecordSettings 
 *    contains un16SamplesPerPacket which holds
 *    samples per audio frame/packet
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::AudioRecordSettingsSet (
	const SstiAudioRecordSettings * pAudioRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


/*!
 * \brief Sets Echo mode
 * 
 * \param EsmdEchoMode eEchoMode 
 * enum setting echo mode to on or off 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::EchoModeSet (
	EsmdEchoMode eEchoMode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

	
/*!
 * \brief Sets Privady to on or off
 * 
 * \param EstiBool bEnable 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::PrivacySet (
	EstiBool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;
	
		audioPrivacySignal.Emit (bEnable);
		
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
}

	
/*!
 * \brief Gets the privacy status
 * 
 * \param EstiBool* pbEnabled 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::PrivacyGet (
	EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	*pbEnabled = m_bPrivacy;
	
	return hResult;
}

/*!
 * \brief Get Device File Descriptor
 * 
 * \return int 
 */
int CstiAudioInput::GetDeviceFD () const
{
	return stiOSSignalDescriptor (m_fdPSignal);
}
