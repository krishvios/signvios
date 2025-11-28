#include "CstiDisplay.h"

extern void putVideoExchangeVideoIn(uint8_t *pTmp, int width, int height);

void CstiDisplay::DisplayYUVImage(uint8_t *pBytes, uint32_t nWidth, uint32_t nHeight) {
    putVideoExchangeVideoIn(pBytes, nWidth, nHeight);
}

extern int g_displayWidth;
extern int g_displayHeight;

stiHResult CstiDisplay::VideoCodecsGet(std::list<EstiVideoCodec> *pCodecs) {
    stiTrace("Enter VideoCodecsGet\n");
    // Add the supported video codecs to the list
    pCodecs->push_back(estiVIDEO_H264);

    return stiRESULT_SUCCESS;
}

void CstiDisplay::PreferredDisplaySizeGet(unsigned int *displayWidth,
                                          unsigned int *displayHeight) const {
    if (displayWidth != nullptr) {
        *displayWidth = g_displayWidth;
    }
    if (displayHeight != nullptr) {
        *displayHeight = g_displayHeight;
    }

}

void CstiDisplay::OrientationChanged() {
    preferredDisplaySizeChangedSignalGet().Emit();
}
