/*!
*  \file CGstreamerBufferElement.cpp
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

//
// Constants
//

//
// Typedefs
//
//#define LOG_ENTRY_NAME(name) printf("%s:pGstBuffer = %p\n", __FUNCTION__, m_pGstBuffer)
#define LOG_ENTRY_NAME(name)

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


/*! \brief Constructor
 *
 * This is the default constructor for the CGstreamerBufferElement class.
 *
 * \return None
 */
CGstreamerBufferElement::CGstreamerBufferElement ()
:
//	m_un32VideoWidth (0),
//	m_un32VideoHeight (0),
//	m_un32RowBytes (0),
	m_pGstBuffer (NULL),
	m_pFifo (NULL)
{
	LOG_ENTRY_NAME (CGstreamerBufferElement::CGstreamerBufferElement);
} // end CGstreamerBufferElement::CGstreamerBufferElement

/*! \brief Destructor
 *
 * This is the default destructor for the CGstreamerBufferElement class.
 *
 * \return None
 */
CGstreamerBufferElement::~CGstreamerBufferElement ()
{
	LOG_ENTRY_NAME (CGstreamerBufferElement::~CGstreamerBufferElement);
} // end CGstreamerBufferElement::~CGstreamerBufferElement


stiHResult CGstreamerBufferElement::GstreamerBufferSet (
	GstBuffer* pGstBuffer)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	m_pGstBuffer = pGstBuffer;
	
	LOG_ENTRY_NAME ();
	
	m_pun8VirtBuf = GST_BUFFER_DATA(pGstBuffer);
	m_un32PhysAddr = 0;  //May not need
	m_un32BufSize = GST_BUFFER_SIZE(pGstBuffer); //May not need
	m_un32DataSize = GST_BUFFER_SIZE(pGstBuffer);
	
	return hResult;
}


stiHResult CGstreamerBufferElement::FifoReturn ()
{
	LOG_ENTRY_NAME ();
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiTESTCOND (m_pFifo, stiRESULT_ERROR);
	
	gst_buffer_unref (m_pGstBuffer);
	m_pGstBuffer = NULL;
	
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
void CGstreamerBufferElement::VideoWidthSet (
	uint32_t un32VideoWidth)
{
//	m_un32VideoWidth = un32VideoWidth;
} 

/*!\brief Sets the video height
 *
 *  \retval none
 */
void CGstreamerBufferElement::VideoHeightSet (
	uint32_t un32VideoHeight)
{
//	m_un32VideoHeight = un32VideoHeight;
} 

/*!\brief Sets the video width
 *
 *  \retval none
 */
void CGstreamerBufferElement::VideoRowBytesSet (
	uint32_t un32RowBytes)
{
//	m_un32RowBytes = un32RowBytes;
} 

/*!\brief Gets the video width
 *
 *  \retval uint32_t - video width
 */
uint32_t CGstreamerBufferElement::VideoWidthGet (void)
{
	return 0;
//	return m_un32VideoWidth;
}

/*!\brief Gets the video height
 *
 *  \retval uint32_t - video width
 */
uint32_t CGstreamerBufferElement::VideoHeightGet (void)
{
	//return m_un32VideoHeight;
	return 0;
}

/*!\brief Gets the video width
 *
 *  \retval uint32_t - video width
 */
uint32_t CGstreamerBufferElement::VideoRowBytesGet (void)
{
	//return m_un32RowBytes;
	return 0;
}

EstiBool CGstreamerBufferElement::KeyFrameGet ()
{

	EstiBool bIsKeyFrame = estiFALSE;

	if (GST_BUFFER_FLAG_IS_SET(m_pGstBuffer, GST_BUFFER_FLAG_DELTA_UNIT))
	{
//		printf("CGstreamerBufferElement::KeyFrameGet GST_BUFFER_FLAG_DELTA_UNIT\n");
		bIsKeyFrame = estiFALSE;
	}
	else if (!GST_BUFFER_FLAG_IS_SET(m_pGstBuffer, GST_BUFFER_FLAG_IN_CAPS))
	{
//		printf("CGstreamerBufferElement::KeyFrameGet GST_BUFFER_FLAG_IS_SET(m_pGstBuffer, GST_BUFFER_FLAG_IN_CAPS) = %d\n",
//				GST_BUFFER_FLAG_IS_SET(m_pGstBuffer, GST_BUFFER_FLAG_IN_CAPS));
		//printf("CGstreamerBufferElement::KeyFrameGet GST_BUFFER_FLAG_IS_SET(m_pGstBuffer, GST_BUFFER_FLAG_IN_HEADER) = %d\n",
		//		GST_BUFFER_FLAG_IS_SET(m_pGstBuffer, GST_BUFFER_FLAG_IN_HEADER));
		bIsKeyFrame = estiTRUE;
	}

//	printf("CGstreamerBufferElement::KeyFrameGet bIsKeyFrame = %d\n", bIsKeyFrame);

	return bIsKeyFrame;
}

// end file CGstreamerBufferElement.cpp
