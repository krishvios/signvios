#include "CstiAudioInput.h"
#include <err.h>


#include "CstiAudioOutput.h"


/*!
 * \brief Default Constructor
 */
CstiAudioInput::CstiAudioInput (CstiAudioOutput *pAudioOutput)
	:
	m_bPrivacy (estiFALSE),
	m_pAudioOutput(pAudioOutput),
	m_fdSignal(NULL)
	{
}

stiHResult CstiAudioInput::Initialize() {
	stiHResult hResult = stiRESULT_SUCCESS;
	if (NULL == m_fdSignal) {
		EstiResult eResult = stiOSSignalCreate (&m_fdSignal);
		stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);		
		stiOSSignalSet (m_fdSignal);
	}
	
STI_BAIL:
	return hResult;

}

stiHResult CstiAudioInput::Uninitialize() {
	if (NULL != m_fdSignal) {
		stiOSSignalClose(&m_fdSignal);
		m_fdSignal=NULL;
	}
	return stiRESULT_SUCCESS;
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

  if (pAudioFrame && pAudioFrame->unBufferMaxSize && pAudioFrame->pun8Buffer) {
		hResult = m_pAudioOutput->AudioRecordPacketGet( pAudioFrame );
	}
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
	stiTrace ( "AUDIO IN: samples per frame/packet: %d\n", pAudioRecordSettings->un16SamplesPerPacket );
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
	
		//
		// Call the callbacks
		//
		hResult = m_oCallbacks.Call (estiMSG_AUDIO_PRIVACY_SET, bEnable);
		
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
 * \brief register object with callback 
 * 
 * \param PstiObjectCallback pfnCallback 
 * \param size_t CallbackParam 
 * \param size_t CallbackFromId 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::CallbackRegister (
	PstiObjectCallback pfnCallback,
	size_t CallbackParam,
	size_t CallbackFromId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_oCallbacks.Register (pfnCallback, CallbackParam, CallbackFromId);

	return hResult;
}

/*!
 * \brief Unregister object from callback function
 * 
 * \param PstiObjectCallback pfnCallback 
 * \param size_t CallbackParam 
 * 
 * \return stiHResult 
 */
stiHResult CstiAudioInput::CallbackUnregister (
	PstiObjectCallback pfnCallback,
	size_t CallbackParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_oCallbacks.Unregister (pfnCallback, CallbackParam);

	return hResult;
}


/*!
 * \brief Get Device File Descriptor
 * 
 * \return int 
 */
int CstiAudioInput::GetDeviceFD () const
{
	return stiOSSignalDescriptor (m_fdSignal);
}
