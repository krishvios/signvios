////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	SMMemory.h
//
//  Abstract:	
//
////////////////////////////////////////////////////////////////////////////////
#ifndef SMMEMORY_H
#define SMMEMORY_H

// Compiler includes
#ifdef stiLINUX
#include <memory.h>
#endif

#ifdef WIN32
#include <memory.h>
#endif

#ifdef __SMMac__
#include <cstring>
#include <MacMemory.h>
#endif

#include "SMTypes.h"

#ifdef __ANDROID__
#include <cstdlib>
#endif


void *SMAllocPtr(
	unsigned int unSize);

void *SMAllocPtrSet(
	unsigned int unSize,
	unsigned char ucChar);

void *SMAllocPtrAligned(
	unsigned int unSize,
	int nByteAligned);

void *SMAllocPtrSetAligned(
	unsigned int unSize,
	unsigned char ucChar,
	int nByteAligned);

void *SMRealloc(
	void *pVoid,
	unsigned int unSize);

void SMFreePtr(
	void *pVoid);

void SMFreePtrAligned(
	void *pPtrToFree);


typedef char **SMHandle;


SMHandle SMNewHandle(
	unsigned int unSize);


SMResult_t SMSetHandleSize(
	SMHandle hHandle,
	unsigned int unSize);


unsigned int SMGetHandleSize(
	SMHandle hHandle);


void SMDisposeHandle(
	SMHandle hHandle);



#define SMAllocPtrClear(unSize) SMAllocPtrSet(unSize, 0);

#define SMAllocPtrClearAligned(unSize, nByteAligned) SMAllocPtrSetAligned(unSize, 0, nByteAligned);

#ifdef __SMMac__
#define SMMemCopy(dst, src, size) BlockMoveData(src, dst, size);
#else
#define SMMemCopy(dst, src, size) memcpy(dst, src, size);
#endif

#define SMClearMem(ptr, size) memset(ptr, 0, size);


enum ByteAligned_e
{
	e8ByteAligned = 8,			// 8-byte alignment
	e16ByteAligned = 16,		// 16-byte alignment
	e32ByteAligned = 32			// 32-byte alignment
};


class SMMemory_c
{
public:

void *operator new(
	size_t size);

void operator delete(
	void *pVoid);

};
#endif // SMMEMORY_H
