/*!
 * \file CBufferElement.h
 * \brief See CBufferElement.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef CBUFFERELEMENT_H
#define CBUFFERELEMENT_H

// Includes
//
#include "stiSVX.h"
#include "stiTrace.h"

#include "CFifo.h"


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

class CBufferElement
{
public:
	CBufferElement ();
	virtual ~CBufferElement ();

	void BufferSet (
		uint8_t *pun8VirtBuf,
		uint32_t un32BufSize);

	uint8_t *BufferGet () const;

	uint32_t BufferSizeGet () const;

	stiHResult DataSizeSet (
		uint32_t un32DataSize);

	uint32_t DataSizeGet () const;

protected:
	
	uint8_t *m_pun8VirtBuf;
	uint32_t m_un32PhysAddr;
	uint32_t m_un32BufSize;
	uint32_t m_un32DataSize;

}; // end CBufferElement Class

#endif //#ifndef CBUFFERELEMENT_H
