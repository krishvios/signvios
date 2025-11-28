////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	SMMemory.cpp
//
//  Abstract:	
//
////////////////////////////////////////////////////////////////////////////////

// Compiler includes
#include <cstdlib>
#include <cstdint>

// Sorenson includes
#include "SMMemory.h"
#include "SMTypes.h"
#include "SorensonErrors.h"
#include "SMCommon.h"

//#define MEMORY_TEST

#ifdef MEMORY_TEST
#include "SMUtils.h"

static int gs_nTotalAllocated;
static unsigned int gs_unId;
#endif // MEMORY_TEST


typedef struct PrivateHandle_t
{
	char *pData;
	unsigned int unSize;
	unsigned int unAllocated;
} PrivateHandle_t;


//
// Mac specific memory allocation stuff.
//

#ifdef __SMMac__

#include <Gestalt.h>
#include <MacMemory.h>

#define MIN_SIZE_FOR_TEMP_MEM (int)2048


////////////////////////////////////////////////////////////////////////////////
//; GetTempMemAvailMac
//
// Abstract:
//
// Returns:
//
static long GetTempMemAvailMac()
{
	long gestaltSez;
	
	if (Gestalt(gestaltOSAttr, &gestaltSez) == noErr)
	{
		return (((1 << gestaltRealTempMemory) & gestaltSez) != 0);
	}
	else
	{
		return(0);
	}
} // GetTempMemAvailMac

#ifndef SM_XCODE
using namespace std;
#endif

#endif // __SMMac__


////////////////////////////////////////////////////////////////////////////////
//; SMAllocPtr
//
// Abstract:
//
// Returns:
//
void *SMAllocPtr(
	unsigned int unSize)
{

#if defined(WIN32) || defined(_WIN32_WCE) || defined(stiLINUX)
#ifdef MEMORY_TEST
	void *pVoid;

	pVoid = malloc(unSize + sizeof(int) * 2);

	((unsigned int *)pVoid)[0] = unSize;
	((unsigned int *)pVoid)[1] = gs_unId++;

	gs_nTotalAllocated += unSize;

	svdprintf("Allocating:\t%d\t%d\t%d\n", gs_unId - 1, unSize, gs_nTotalAllocated);

	return (unsigned char *)pVoid + sizeof(int) * 2;
#else
	return malloc(unSize);
#endif // MEMORY_TEST
#endif // WIN32 || _WIN32_CE

#ifdef __SMMac__
	void *pReturn = nil;
	Handle hReturn = nil;
	OSErr Result; // Mac type

	//
	// Add on the size of the Handle which gets written
	//
	unSize += sizeof(Handle);

	if (unSize >= MIN_SIZE_FOR_TEMP_MEM)
	{
		//
		// If the size is greater than some small value (currenlty 2K) then	
		// try and allocate from temporary memory, then application memory,
		// and finally system memory.
		//
 
		if (GetTempMemAvailMac() != 0)
		{
			//
			// Try and allocate the handle from temporary memory
			//
			hReturn = TempNewHandle(unSize, &Result);

			if (hReturn != nil)
			{
				//
				// Lock the handle
				//
				HLockHi(hReturn);

				//
				// Set the pointer value
				//
				pReturn = *hReturn;
			}
		}

		if (hReturn == nil)
		{
			//
			// Try to allocate the pointer in the application heap
			//
			pReturn = NewPtr(unSize);

			//
			// Check for failure
			//
			if (pReturn == nil)
			{
				//
				// Try and allocate the pointer from the system heap
				//
				pReturn = NewPtrSys(unSize);
			}
		}
	}
	else
	{
		//
		// If the size is less than or equal to some small value (currenlty 2K)
		// then try and allocate from application memory, then temporary memory,
		// and finally system memory.
		//

		//
		// Try to allocate the pointer in the application heap
		//
		pReturn = NewPtr(unSize);

		if (pReturn == nil)
		{
			if (GetTempMemAvailMac() != 0)
			{
				//
				// Try and allocate the pointer from temporary memory
				//
				hReturn = TempNewHandle(unSize, &Result);

				if (hReturn != nil)
				{
					//
					// Lock the handle
					//
					HLockHi(hReturn);

					//
					// Set the pointer value
					//
					pReturn = *hReturn;
				}
			}

			if (hReturn == nil)
			{
				//
				// Try and allocate the pointer from the system heap
				//
				pReturn = NewPtrSys(unSize);
			}
		}
	}

	if (pReturn != nil)
	{
		//
		// hReturn will be nill if the handle allocation failed
		//
		*((Handle *)pReturn) = hReturn;
		((char *)pReturn) += sizeof(Handle);
	}

	//
	// Return the allocated Handle, it will be nil if there was a failure.
	//
	return pReturn;
#endif // __SMMac__
} // SMAllocPtr


////////////////////////////////////////////////////////////////////////////////
//; SMAllocPtrSet
//
// Abstract:
//
// Returns:
//
void *SMAllocPtrSet(
	unsigned int unSize,
	unsigned char ucChar)
{
	void *pReturn;

	pReturn = SMAllocPtr(unSize);

	if (pReturn)
	{
		memset(pReturn, ucChar, unSize);
	}

	return pReturn;
} // SMAllocPtrSet


////////////////////////////////////////////////////////////////////////////////
//; SMRealloc
//
// Abstract:
//
// Returns:
//
void *SMRealloc(
	void *pVoid,
	unsigned int unSize)
{
#if defined(WIN32) || defined(_WIN32_WCE) || defined(stiLINUX)
#ifdef MEMORY_TEST
	unsigned int unOldSize;
	unsigned int unId;

	if (pVoid)
	{
		pVoid = (unsigned char *)pVoid - sizeof(int) * 2;

		unOldSize = ((unsigned int *)pVoid)[0];
		unId = ((unsigned int *)pVoid)[1];
	}
	else
	{
		unOldSize = 0;
		unId = gs_unId++;
	}

	pVoid = realloc(pVoid, unSize + sizeof(int) * 2);

	((unsigned int *)pVoid)[0] = unSize;
	((unsigned int *)pVoid)[1] = unId;

	if (unOldSize != 0)
	{
		svdprintf("Freeing:\t%d\t%d\t%d\n", unId, -(signed)unOldSize, gs_nTotalAllocated - unOldSize);
	}

	svdprintf("Allocating:\t%d\t%d\t%d\n", unId, unSize, gs_nTotalAllocated - unOldSize + unSize);

	gs_nTotalAllocated += unSize - unOldSize;

	return (unsigned char *)pVoid + sizeof(int) * 2;
#else
	return realloc(pVoid, unSize);
#endif // MEMORY_TEST
#endif // WIN32 || _WIN32_CE

#ifdef __SMMac__
	void *pNewMem;
	uintptr_t unOffset;
	Size sizeMem;


	if(pVoid == NULL)
	{	
		// The documentation for realloc() under Windows states that if the passed
		// in pointer is NULL then it acts like Malloc().  Do the same here.
		pNewMem = SMAllocPtr(unSize); 
	}
	else if(unSize > 0)
	{
		pNewMem = SMAllocPtr(unSize);

		if (pNewMem)
		{
			//
			// Determine the size of the original memory.
			//
			unOffset = *(uintptr_t *)((char *)pVoid - sizeof(Handle));

			if (unOffset != 0)
			{
				sizeMem = GetHandleSize((Handle)unOffset);
			}
			else
			{
				sizeMem = GetPtrSize((Ptr)((char *)pVoid - sizeof(Handle)));
			}

			sizeMem -= sizeof(Handle);

			if ((unsigned)sizeMem > unSize)
			{
				sizeMem = unSize;
			}

			//
			// Copy the data from the old memory to the new memory.
			//
			memcpy(pNewMem, pVoid, sizeMem);

			//
			// Free the old pointer.
			//
			SMFreePtr(pVoid);
		}
		else
		{
			//
			// Free the old pointer.
			//
			SMFreePtr(pVoid);
		}
	}
	else
	{
		// pVoid is not NULL and unSize is == 0
		SMFreePtr(pVoid);
		pNewMem = NULL;
	}

	return pNewMem;
#endif // __SMMac__

}


////////////////////////////////////////////////////////////////////////////////
//; SMFreePtr
//
// Abstract:
//
// Returns:
//
void SMFreePtr(
	void *pVoid)
{
#if defined(WIN32) || defined(_WIN32_WCE) || defined (stiLINUX)
#ifdef MEMORY_TEST	
	unsigned int unSize;
	unsigned int unId;

	pVoid = (unsigned char *)pVoid - sizeof(int) * 2;

	unSize = ((unsigned int *)pVoid)[0];
	unId = ((unsigned int *)pVoid)[1];

	gs_nTotalAllocated -= unSize;

	svdprintf("Freeing:\t%d\t%d\t%d\n", unId, -(signed)unSize, gs_nTotalAllocated);

	free(pVoid);
#else
	free(pVoid);
#endif // MEMORY_TEST
#endif // WIN32 || _WIN32_CE

#ifdef __SMMac__
	uintptr_t unOffset;

	//
	// Return the pointer to the base address
	//
	((char*)pVoid) -= sizeof(Handle);

	//
	// Get the handle from the memory block
	//
	unOffset = *((uintptr_t *) pVoid);

	//
	// See if a handle was used to allocate the pointer
	//
	if (unOffset != 0)
	{
		//
		// Unlock the handle and dispose of it
		//
		HUnlock((Handle) unOffset);
		DisposeHandle((Handle) unOffset);
	}
	else
	{
		//
		// It was allocated as a pointer so free the pointer
		//
		DisposePtr((char *) pVoid);
	}
#endif // __SMMac__
} // SMFreePtr


////////////////////////////////////////////////////////////////////////////////
//; SMAllocPtrAligned
//
// Abstract:
//	Allocates a pointer of input size, on a specified-byte boundary.
//
//	Allocated memory is aligned on a specified-byte boundary.  The specified
//	byte boundary, however, must be a multiple of 4-bytes (4, 8, 16, 32, etc)
//	or the function returns NULL.
//
//	This function assumes that the address of the returned pointer, after
//	calling SMAllocPtr(), is a multiple of 4 (which is a correct assumption
//	for a 32-bit OS).  Otherwise the calculated offset could be negative.
//	The offset would be negative if
//	((uintptr_t)pui8Pointer % nByteAligned) > nByteAligned - sizeof(uintptr_t)
//	For example: if nByteAligned == 32 and the result of the % operation
//	could be 29, 30, or 31, then the calculated offset would be negative.
//
// Returns:
//	Allocated pointer, or NULL on error.
//
void *SMAllocPtrAligned(
	unsigned int unSize,
	int nByteAligned)
{
	uint8_t *pui8Pointer = nullptr;
	uintptr_t ui32Offset;

	//
	// Check if specified-byte boundary is a multiple of 4, if not return NULL
	//
	if (nByteAligned & 0x3)
	{
		return nullptr;
	}

	//
	// Increase the size by the number of bytes needed to ensure alignment on
	// the specified-byte boundary, and allocate memory of the new size.
	//
	unSize += nByteAligned;

	pui8Pointer = (uint8_t *)SMAllocPtr(unSize);

	if (pui8Pointer == nullptr)
	{
		return nullptr;
	}

	//
	// Calculate the offset, in bytes, from the returned pointer location to
	// the next specified-byte boundary, minus the space of 32 bits (where
	// the offset will be stored in memory).  Increment the pointer this far.
	//
	ui32Offset = nByteAligned - ((uintptr_t)pui8Pointer % nByteAligned) - sizeof(uintptr_t);

	pui8Pointer += ui32Offset;

	//
	// Store the calculated offset in memory, so free() will know how far to
	// back-track to the original pointer location.  Increment the pointer past
	// the stored offset.
	//
	*((uintptr_t *)pui8Pointer) = ui32Offset;

	pui8Pointer += sizeof(uintptr_t);

	return (void *)pui8Pointer;
} // SMAllocPtrAligned


////////////////////////////////////////////////////////////////////////////////
//; SMFreePtrAligned
//
// Abstract:
//	Frees a pointer allocated with SMAllocPtrAligned().
//
//	The pointer passed in to this function is aligned on a specified-byte
//	boundary.  The offset needed to back-track to the original spot was stored
//	in memory just before the aligned pointer.
//
// Returns:
//	None.
//
void SMFreePtrAligned(
	void *pPtrToFree)
{
	auto pui8Pointer = (uint8_t *)pPtrToFree;
	uintptr_t ui32Offset;

	//
	// Back-track to point where offset was stored, read from memory what
	// the offset is, decrement pointer to original location, and free the
	// pointer.
	//
	pui8Pointer -= sizeof(uintptr_t);

	ui32Offset = *((uintptr_t *)pui8Pointer);

	pui8Pointer -= ui32Offset;

	SMFreePtr(pui8Pointer);
}


////////////////////////////////////////////////////////////////////////////////
//; SMAllocPtrSetAligned
//
// Abstract:
//	Allocates a pointer of input size, on a specified-byte boundary.
//
//	Allocated memory is aligned on a specified-byte boundary.  The specified
//	byte boundary, however, must be a multiple of 4-bytes (4, 8, 16, 32, etc)
//	or the function returns NULL.
//
//	This function assumes that the address of the returned pointer, after
//	calling SMAllocPtr(), is a multiple of 4 (which is a correct assumption
//	for a 32-bit OS).  Otherwise the calculated offset could be negative.
//	The offset would be negative if
//	((uintptr_t)pui8Pointer % nByteAligned) > nByteAligned - sizeof(uintptr_t)
//	For example: if nByteAligned == 32 and the result of the % operation
//	could be 29, 30, or 31, then the calculated offset would be negative.
//
// Returns:
//	Allocated pointer, or NULL on error.
//
void *SMAllocPtrSetAligned(
	unsigned int unSize,
	unsigned char ucChar,
	int nByteAligned)
{
	uint8_t *pui8Pointer = nullptr;
	uintptr_t ui32Offset;

	//
	// Check if specified-byte boundary is a multiple of 4, if not return NULL
	//
	if (nByteAligned & 0x3)
	{
		return nullptr;
	}

	//
	// Increase the size by the number of bytes needed to ensure alignment on
	// the specified-byte boundary, and allocate memory of the new size.
	//
	unSize += nByteAligned;

	pui8Pointer = (uint8_t *)SMAllocPtrSet(unSize, ucChar);

	if (pui8Pointer == nullptr)
	{
		return nullptr;
	}

	//
	// Calculate the offset, in bytes, from the returned pointer location to
	// the next specified-byte boundary, minus the space of 32 bits (where
	// the offset will be stored in memory).  Increment the pointer this far.
	//
	ui32Offset = nByteAligned - ((uintptr_t)pui8Pointer % nByteAligned) - sizeof(uintptr_t);

	pui8Pointer += ui32Offset;

	//
	// Store the calculated offset in memory, so free() will know how far to
	// back-track to the original pointer location.  Increment the pointer past
	// the stored offset.
	//
	*((uintptr_t *)pui8Pointer) = ui32Offset;

	pui8Pointer += sizeof(uintptr_t);

	return (void *)pui8Pointer;
} // SMAllocPtrSetAligned


////////////////////////////////////////////////////////////////////////////////
//; new
//
// Abstract:
//	Overload new operator.
//
// Returns:
//	Void pointer to allocated memory, NULL if failed.
//
void *SMMemory_c::operator new(
	size_t size)
{
	return SMAllocPtrClear(size);	// BUGBUG - why clear?
}


////////////////////////////////////////////////////////////////////////////////
//; delete
//
// Abstract:
//	Overload delete operator.
//
// Returns:
//	None.
//
void SMMemory_c::operator delete(
	void *pVoid)
{
	if (pVoid)
	{
		SMFreePtr(pVoid);
	}
}

///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMHandle SMNewHandle(
	unsigned int unSize)
{
	PrivateHandle_t *pPrivate;

	pPrivate = (PrivateHandle_t *)SMAllocPtrClear(sizeof(PrivateHandle_t));

	if (pPrivate)
	{
		pPrivate->pData = nullptr;
		pPrivate->unSize = 0;
		pPrivate->unAllocated = 0;

		if (unSize != 0)
		{
			pPrivate->pData = (char *)SMAllocPtr(unSize * sizeof(char));

			if (pPrivate->pData == nullptr)
			{
				SMFreePtr(pPrivate);

				pPrivate = nullptr;
			}
			else
			{
				pPrivate->unSize = unSize;
				pPrivate->unAllocated = unSize;
			}
		}
	}

	return (SMHandle)pPrivate;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
SMResult_t SMSetHandleSize(
	SMHandle hHandle,
	unsigned int unSize)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	PrivateHandle_t *pPrivate;

	pPrivate = (PrivateHandle_t *)hHandle;

	if (pPrivate)
	{
		if (unSize > pPrivate->unAllocated)
		{
			pPrivate->pData = (char *)SMRealloc(pPrivate->pData, unSize * sizeof(char));

			if (pPrivate->pData == nullptr)
			{
				pPrivate->unSize = 0;
				pPrivate->unAllocated = 0;

				ASSERTRT(eFalse, SMRES_ALLOC_FAILED);
			}

			pPrivate->unSize = unSize;
			pPrivate->unAllocated = unSize;
		}
		else
		{
			pPrivate->unSize = unSize;
		}
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
unsigned int SMGetHandleSize(
	SMHandle hHandle)
{
	PrivateHandle_t *pPrivate;
	unsigned int unSize = 0;

	pPrivate = (PrivateHandle_t *)hHandle;

	if (pPrivate)
	{
		unSize = pPrivate->unSize;
	}
	
	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; 
//
// Abstract:
//
// Returns:
//
void SMDisposeHandle(
	SMHandle hHandle)
{
	PrivateHandle_t *pPrivate;

	pPrivate = (PrivateHandle_t *)hHandle;

	if (pPrivate)
	{
		if (pPrivate->unAllocated > 0)
		{
			SMFreePtr(pPrivate->pData);

			pPrivate->pData = nullptr;
		}

		SMFreePtr(pPrivate);
	}
}


