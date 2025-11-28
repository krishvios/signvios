#ifndef CSTIVIDEOINPUT_H
#define CSTIVIDEOINPUT_H

#include "CstiSoftwareInput.h"


class CstiVideoInput : public CstiSoftwareInput
{
	protected:
		void VideoRecordSizeSet(uint32_t un32Width, uint32_t un32Height);
		void VideoRecordFrameRateSet ( uint32_t un32FrameRate );
		virtual stiHResult VideoCodecsGet (std::list<EstiVideoCodec> *pCodecs);
		virtual stiHResult CodecCapabilitiesGet (EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps);


	public:
		void CaptureEnabledSet (EstiBool bEnabled);
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
		EstiBool VideoInputRead (uint8_t **pBytes, EstiColorSpace * pColorSpace);
#else
		EstiBool VideoInputRead ( uint8_t *pBytes);
#endif
};

#endif
