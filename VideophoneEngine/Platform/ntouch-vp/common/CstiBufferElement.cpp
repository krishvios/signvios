/*!
*  \file CBufferElement.cpp
*  \brief This is the class to hold the buffer information
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

//
// Includes
//
#include "CstiBufferElement.h"

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


/*!\brief Sets the pointer and size to the buffer
 *
 *  \retval none
 */
void CBufferElement::BufferSet (
	uint8_t * pun8VirtBuf,
	uint32_t un32BufSize)
{
	m_pun8VirtBuf = pun8VirtBuf;
	m_un32BufSize = un32BufSize;
} // end CBufferElement::BufferSet


/*!\brief Returns a pointer to the buffer
 *
 *  \retval uint8_t * - pointer to buffer or NULL 
 */
uint8_t * CBufferElement::BufferGet () const
{
	return (m_pun8VirtBuf);
} // end CBufferElement::BufferGet


/*!\brief Returns the size of the buffer
 *
 *  \retval uint32_t - value of the buffer size
 */
uint32_t CBufferElement::BufferSizeGet () const
{
	return (m_un32BufSize);
} // end CBufferElement::BufferGet


/*!\brief Sets the data size in the buffer
 *
 *  \retval none
 */
stiHResult CBufferElement::DataSizeSet (
	uint32_t un32DataSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	m_un32DataSize = un32DataSize;
	
	return hResult;
} // end CBufferElement::DataSizeSet


/*!\brief Returns the size of data in the buffer
 *
 *  \retval uint32_t - value of the data size in the buffer
 */
uint32_t CBufferElement::DataSizeGet () const
{
	return (m_un32DataSize);
} // end CBufferElement::DataSizeGet

// end file CBufferElement.cpp
