/*!
*  \file GstreamerBufferElement.cpp
*  \brief This is the class to hold the gstreamer buffer information
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

//
// Includes
//
#include "GstreamerBufferElement.h"
#include "stiTrace.h"

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
// Locals
//

//
// Class Definitions
//


stiHResult GstreamerBufferElement::GstreamerBufferSet (
	GStreamerBuffer &gstBuffer,
	bool bKeyFrame,
	EstiVideoCodec eVideoCodec,
	EstiVideoFrameFormat eVideoFrameFormat)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_gstBuffer = gstBuffer;

	m_pun8VirtBuf = m_gstBuffer.dataGet ();
	m_un32PhysAddr = 0;  //May not need
	m_un32BufSize = m_gstBuffer.sizeGet (); //May not need
	m_un32DataSize = m_gstBuffer.sizeGet ();
	m_bKeyFrame = bKeyFrame;
	m_eVideoCodec = eVideoCodec;
	m_eVideoFrameFormat = eVideoFrameFormat;
	m_timeStamp = m_gstBuffer.timestampGet ();

	return hResult;
}


stiHResult GstreamerBufferElement::FifoReturn ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (m_pFifo, stiRESULT_ERROR);

	m_gstBuffer.clear ();

	m_bKeyFrame = false;

	DataSizeSet (0);
	VideoWidthSet (0);
	VideoHeightSet (0);
	VideoRowBytesSet (0);

	hResult = m_pFifo->Put (this);

STI_BAIL:

	return hResult;
}

/*!\brief Sets the video width
 *
 *  \retval none
 */
void GstreamerBufferElement::VideoWidthSet (
	uint32_t un32VideoWidth)
{
//	m_un32VideoWidth = un32VideoWidth;
}

/*!\brief Sets the video height
 *
 *  \retval none
 */
void GstreamerBufferElement::VideoHeightSet (
	uint32_t un32VideoHeight)
{
//	m_un32VideoHeight = un32VideoHeight;
}

/*!\brief Sets the video width
 *
 *  \retval none
 */
void GstreamerBufferElement::VideoRowBytesSet (
	uint32_t un32RowBytes)
{
//	m_un32RowBytes = un32RowBytes;
}

/*!\brief Gets the video width
 *
 *  \retval uint32_t - video width
 */
uint32_t GstreamerBufferElement::VideoWidthGet ()
{
	return 0;
//	return m_un32VideoWidth;
}

/*!\brief Gets the video height
 *
 *  \retval uint32_t - video width
 */
uint32_t GstreamerBufferElement::VideoHeightGet ()
{
	//return m_un32VideoHeight;
	return 0;
}

/*!\brief Gets the video width
 *
 *  \retval uint32_t - video width
 */
uint32_t GstreamerBufferElement::VideoRowBytesGet ()
{
	//return m_un32RowBytes;
	return 0;
}

bool GstreamerBufferElement::KeyFrameGet ()
{
	return m_bKeyFrame;
}

EstiVideoCodec GstreamerBufferElement::CodecGet ()
{
	return m_eVideoCodec;
}

EstiVideoFrameFormat GstreamerBufferElement::VideoFrameFormatGet ()
{
	return m_eVideoFrameFormat;
}

/*!\brief Gets the time stamp of the frame in nano seconds
 *
 *  \retval uint64_t - frame timestamp
 */
uint64_t GstreamerBufferElement::TimeStampGet ()
{
	return m_timeStamp;
}


// end file GstreamerBufferElement.cpp
