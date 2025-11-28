/*!
 * \file CGstreamerBufferElement.h
 * \brief See CGstreamerBufferElement.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef CGSTREAMERBUFFERELEMENT_H
#define CGSTREAMERBUFFERELEMENT_H

//
// Includes
//
#include "stiSVX.h"
#include "CstiBufferElement.h"
#include <gst/gst.h>

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

class CGstreamerBufferElement : public CBufferElement
{
public:
	CGstreamerBufferElement ();
	virtual	~CGstreamerBufferElement ();

	stiHResult GstreamerBufferSet (
		GstBuffer* pGstBuffer);
	
	uint32_t VideoWidthGet ();
	
	void VideoWidthSet (
		uint32_t un32VideoWidth);

	uint32_t VideoHeightGet ();
	
	void VideoHeightSet (
		uint32_t un32VideoHeight);

	uint32_t VideoRowBytesGet ();
	
	void VideoRowBytesSet (
		uint32_t un32RowBytes);
	
	void FifoAssign (CFifo <CGstreamerBufferElement> *pFifo)
	{
		m_pFifo = pFifo;
	}
	
	stiHResult FifoReturn ();

	EstiBool KeyFrameGet ();

	timeval m_ArrivalTime;
	
private:
	
//	uint32_t m_un32VideoWidth;
//	uint32_t m_un32VideoHeight;
//	uint32_t m_un32RowBytes;
	GstBuffer* m_pGstBuffer;
	
	CFifo <CGstreamerBufferElement> *m_pFifo;

	

}; // end CGstreamerBufferElement Class

#endif //#ifndef CGSTREAMERBUFFERELEMENT_H

