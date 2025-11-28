#ifndef CSTIDISPLAY_H
#define CSTIDISPLAY_H

#include "CstiSoftwareDisplay.h"




class CstiDisplay : public CstiSoftwareDisplay
{

	public:
  	void DisplayYUVImage ( uint8_t *pBytes, uint32_t nWidth, uint32_t nHeight );
    virtual stiHResult VideoCodecsGet (std::list<EstiVideoCodec> *pCodecs);
	void PreferredDisplaySizeGet(unsigned int *displayWidth, unsigned int *displayHeight) const;
	void OrientationChanged ();
};

#endif
