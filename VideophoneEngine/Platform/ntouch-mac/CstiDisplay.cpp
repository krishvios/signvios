#include "CstiDisplay.h"

#include "VideoExchange.h"
#include "PropertyManager.h"

void CstiDisplay::DisplayYUVImage (  uint8_t *pBytes, uint32_t nWidth, uint32_t nHeight)
{
	PutVideoExchangeVideoIn(pBytes, nWidth, nHeight );
}

#if APPLICATION == APP_NTOUCH_MAC
void CstiDisplay::DisplayCMSampleBuffer (  uint8_t *pBytes, uint32_t nWidth, uint32_t nHeight)
{
	PutVideoExchangeVideoInCMSampleBuffer(pBytes, nWidth, nHeight );
}
#endif

stiHResult CstiDisplay::VideoCodecsGet (std::list<EstiVideoCodec> *pCodecs)
{
	stiTrace("Enter VideoCodecsGet\n");
	GetVideoExchangeDecodeCodecs(pCodecs);
	if (pCodecs->size() == 0) {
		// Add the supported video codecs to the list
	// Add the supported video codecs to the list
#if defined(stiENABLE_H265_DECODE) && APPLICATION == APP_NTOUCH_MAC
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

void CstiDisplay::PreferredDisplaySizeGet (
	unsigned int *displayWidth,
	unsigned int *displayHeight) const
{
	GetPreferredDisplaySize(displayWidth, displayHeight);
}
