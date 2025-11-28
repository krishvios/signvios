#include "CstiVideoInput.h"

#include "VideoExchange.h"
#include "PropertyManager.h"

void CstiVideoInput::CaptureEnabledSet (EstiBool bEnabled) {
	SetCaptureEnabled(bEnabled);
}

#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
EstiBool CstiVideoInput::VideoInputRead ( uint8_t **pBytes, EstiColorSpace * pColorSpace) {
	return GetVideoExchangeVideoOut(pBytes, pColorSpace) ? estiTRUE : estiFALSE;
}
#else
EstiBool CstiVideoInput::VideoInputRead ( uint8_t *pBytes) {
	return GetVideoExchangeVideoOut(pBytes) ? estiTRUE : estiFALSE;
}
#endif
void CstiVideoInput::VideoRecordSizeSet(uint32_t un32Width, uint32_t un32Height)
{
	SetVideoExchangeVideoOutSize(un32Width, un32Height);

}
void CstiVideoInput::VideoRecordFrameRateSet ( uint32_t un32FrameRate )
{
	SetVideoExchangeVideoOutRate(un32FrameRate);
}
/*!
 * \brief Get video codec
 *
 * \param pCodecs - a std::list where the codecs will be stored.
 *
 * \return stiHResult
 */
stiHResult CstiVideoInput::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	GetVideoExchangeEncodeCodecs(pCodecs);
	if (pCodecs->size() == 0) {
		// Add the supported video codecs to the list
#ifdef stiENABLE_H265_ENCODE
		WillowPM::PropertyManager *pm = WillowPM::PropertyManager::getInstance();
		int nEnableH265 = NULL;
		pm->propertyGet("EnableH265", &nEnableH265, 0, WillowPM::PropertyManager::Persistent);
		if (nEnableH265 == 1)
		{
			pCodecs->push_back(estiVIDEO_H265);
		}
#endif
		pCodecs->push_back (estiVIDEO_H264);
		pCodecs->push_back (estiVIDEO_H263);

	}
	return stiRESULT_SUCCESS;
}

/*!
 * \brief Get Codec Capability
 *
 * \param eCodec
 * \param pstCaps
 *
 * \return stiHResult
 */
stiHResult CstiVideoInput::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,
	SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;
	
	switch (eCodec)
	{
		case estiVIDEO_H263:
			stProfileAndConstraint.eProfile = estiH263_ZERO;
			pstCaps->Profiles.push_back (stProfileAndConstraint);
			break;
			
		case estiVIDEO_H264:
		{
			SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;
			
#if 0
			stProfileAndConstraint.eProfile = estiH264_EXTENDED;
			stProfileAndConstraint.un8Constraints = 0xf0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);
#endif
			stProfileAndConstraint.eProfile = estiH264_MAIN;
			stProfileAndConstraint.un8Constraints = 0xe0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);
			
			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xa0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);
			
			pstH264Caps->eLevel = estiH264_LEVEL_4;
			
			break;
		}
		case estiVIDEO_H265:
		{
			SstiH265Capabilities *pstH265Caps = (SstiH265Capabilities*)pstCaps;
			
			stProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH265Caps->Profiles.push_back(stProfileAndConstraint);
			pstH265Caps->eLevel = estiH265_LEVEL_4;
			pstH265Caps->eTier = estiH265_TIER_MAIN;
			
			break;
		}
			
		default:
			hResult = stiRESULT_ERROR;
	}
	return hResult;
}


