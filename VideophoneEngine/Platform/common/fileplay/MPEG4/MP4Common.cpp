///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Common.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include <ctime>

#ifdef __SMMac__
#include "DateTimeUtils.h"
#endif // __SMMac__

//
// Sorenson includes
//
#include "SMMemory.h"
#include "SorensonErrors.h"
#include "SMTypes.h"
#include "SMCommon.h"

#include "SorensonMP4.h"
#include "MP4Common.h"


#define MAC_EPOCH 2082758400


///////////////////////////////////////////////////////////////////////////////
//; MP4GetCurrentTime
//
// Abstract:
//
// Returns:
//
void MP4GetCurrentTime(
	uint64_t *pun64Time)
{
	unsigned long calctime = 0;

#ifdef __SMMac__
	GetDateTime(&calctime);
#endif // __SMMac__

#ifdef WIN32
	calctime = static_cast<unsigned long>(time(NULL) + MAC_EPOCH);
#endif

	*pun64Time = calctime;
} // MP4GetCurrenTime



///////////////////////////////////////////////////////////////////////////////
//; GetDataPtr
//
// Abstract:
//
// Returns:
//
char *GetDataPtr(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint32_t unSize)
{
	char *pData;

	pData = (char *)SMAllocPtr(unSize);

	if (pData != nullptr)
	{
		pDataHandler->DHRead(un64Offset, unSize, pData);
	}

	return pData;
} // GetDataPtr


///////////////////////////////////////////////////////////////////////////////
//; GetInt8
//
// Abstract:
//
// Returns:
//
int8_t GetInt8(
	CDataHandler* pDataHandler,
	uint64_t un64Offset)
{
	int8_t cData;

	pDataHandler->DHRead(un64Offset, sizeof(cData), &cData);

	return cData;
} // GetInt8


///////////////////////////////////////////////////////////////////////////////
//; GetInt16
//
// Abstract:
//
// Returns:
//
int16_t GetInt16(
	CDataHandler* pDataHandler,
	uint64_t un64Offset)
{
	int16_t sData;

	pDataHandler->DHRead(un64Offset, sizeof(sData), &sData);

	sData = BigEndian2(sData);

	return sData;
} // GetInt16


///////////////////////////////////////////////////////////////////////////////
//; GetInt32
//
// Abstract:
//
// Returns:
//
int32_t GetInt32(
	CDataHandler* pDataHandler,
	uint64_t un64Offset)
{
	int32_t nData;

	pDataHandler->DHRead(un64Offset, 4, &nData);

	nData = BigEndian4(nData);

	return nData;
} // GetInt32


///////////////////////////////////////////////////////////////////////////////
//; GetInt64
//
// Abstract:
//
// Returns:
//
int64_t GetInt64(
	CDataHandler* pDataHandler,
	uint64_t un64Offset)
{
	int64_t n64Data;

	pDataHandler->DHRead(un64Offset, sizeof(n64Data), &n64Data);

	n64Data = BigEndian8(n64Data);

	return n64Data;
} // GetInt64



///////////////////////////////////////////////////////////////////////////////
//; WriteFileType
//
// Abstract:
//
// Returns:
//
SMResult_t WriteFileType(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	FileSpace_t *pFileSpace;
	FtypAtom_c *pFtypAtom;
	uint32_t *pBrands;
	unsigned int unBrandLength;

	//
	// The file type atom must be the first
	// atom in the file.
	//
	ASSERTRT(pMovie->FileSpaceList.pNext == &pMovie->FileSpaceList, -1);

	//
	// Make sure the ftyp atom is present.
	//
	ASSERTRT(pMovie->pFtypAtom != nullptr, -1);

	pFtypAtom = pMovie->pFtypAtom;

	pFtypAtom->GetBrands(&pBrands, &unBrandLength);

	//
	// The parameter unBrandLength must at least two.
	//
	ASSERTRT(unBrandLength >= 2, -1);

	pFileSpace = (FileSpace_t *)SMAllocPtr(sizeof(FileSpace_t));

	ASSERTRT(pFileSpace != nullptr, SMRES_ALLOC_FAILED);

	pFileSpace->pNext = &pMovie->FileSpaceList;
	pFileSpace->pPrev = pMovie->FileSpaceList.pPrev;

	pMovie->FileSpaceList.pPrev->pNext = pFileSpace;
	pMovie->FileSpaceList.pPrev = pFileSpace;

	pFileSpace->unType = eAtom4CCftyp;
	pFileSpace->un64Offset = 0;
	pFileSpace->un64Length = pFtypAtom->GetSize();

	pFtypAtom->Write(pMovie->pDataHandler);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; WriteMediaData
//
// Abstract:
//
// Returns:
//
SMResult_t WriteMediaData(
	Movie_t *pMovie,
	const uint8_t *pData,
	uint32_t unLength,
	uint64_t *pun64Offset)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	FileSpace_t *pFileSpace;
	FileSpace_t *pFreeSpace;
	uint64_t un64Offset = 0;

	//
	// Find a place to write the data.
	//
	for (pFileSpace = pMovie->FileSpaceList.pNext; pFileSpace != &pMovie->FileSpaceList; pFileSpace = pFileSpace->pNext)
	{
		if (pFileSpace->unType == eAtom4CCmdat)
		{
			if (pFileSpace->pNext == &pMovie->FileSpaceList)
			{
				//
				// This mdat is the last atom in the file so we can freely append the data 
				// to this mdat.
				//
				un64Offset = pFileSpace->un64Offset + pFileSpace->un64Length;

				break;
			}

			if (pFileSpace->pNext->unType == eAtom4CCfree &&
				(pFileSpace->pNext->un64Length == unLength ||
				pFileSpace->pNext->un64Length - 2 * sizeof(uint32_t) > unLength))
			{
				pFreeSpace = pFileSpace->pNext;

				//
				// Here we can increase the size of the mdat and decrease the size of the free atom that follows it.
				//
				un64Offset = pFreeSpace->un64Offset;

				pFreeSpace->un64Offset += unLength;
				pFreeSpace->un64Length -= unLength;

				if (pFreeSpace->un64Length == 0)
				{
					//
					// Delete the free space node from the list.
					//
					pFreeSpace->pNext->pPrev = pFreeSpace->pPrev;
					pFreeSpace->pPrev->pNext = pFreeSpace->pNext;

					SMFreePtr(pFreeSpace);
				}

				break;
			}
		}
	}

	if (pFileSpace == &pMovie->FileSpaceList)
	{
		//
		// We reached the end of the file so if the data is written it
		// will be written to a new mdat at the end of the file.
		//
		un64Offset = pMovie->FileSpaceList.pPrev->un64Offset + pMovie->FileSpaceList.pPrev->un64Length;
	
		pFileSpace = (FileSpace_t *)SMAllocPtr(sizeof(FileSpace_t));

		ASSERTRT(pFileSpace != nullptr, SMRES_ALLOC_FAILED);

		pFileSpace->pNext = &pMovie->FileSpaceList;
		pFileSpace->pPrev = pMovie->FileSpaceList.pPrev;

		pMovie->FileSpaceList.pPrev->pNext = pFileSpace;
		pMovie->FileSpaceList.pPrev = pFileSpace;

		pFileSpace->unType = eAtom4CCmdat;
		pFileSpace->un64Offset = un64Offset;
		pFileSpace->un64Length = 2 * sizeof(uint32_t);

		//
		// Write two mdat atoms at the beginning of this section.  The second will
		// be overwritten if the section grows beyond a 32-bit size.
		//
		SMResult = (pMovie->pDataHandler)->DHSetPosition(un64Offset);
	
		TESTRESULT();
		
		SMResult = (pMovie->pDataHandler)->DHWrite4(0);

		TESTRESULT();

		SMResult = (pMovie->pDataHandler)->DHWrite4(eAtom4CCmdat);

		TESTRESULT();

		//
		// The additional 4 four byte values are for the header of the new mdat atom.
		//
		un64Offset += 2 * sizeof(uint32_t);
	}

	//
	// Write the data.
	//
	SMResult = (pMovie->pDataHandler)->DHSetPosition(un64Offset);
	TESTRESULT();

	SMResult = (pMovie->pDataHandler)->DHWrite(pData, unLength);
	TESTRESULT();

	pFileSpace->un64Length += unLength;

	*pun64Offset = un64Offset;

smbail:

	return SMResult;
}



static SMResult_t FindMoovLocation(
	Movie_t *pMovie,
	uint64_t un64DataLength,
	uint64_t *pun64Offset)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	FileSpace_t *pFileSpace;
	FileSpace_t *pFreeSpace;
	FileSpace_t *pNextFreeSpace;
	uint64_t un64FreeSpace;

	//
	// Find the moov atom.
	//
	for (pFileSpace = pMovie->FileSpaceList.pNext; pFileSpace != &pMovie->FileSpaceList; pFileSpace = pFileSpace->pNext)
	{
		if (pFileSpace->unType == eAtom4CCmoov)
		{
			break;
		}
	}

	if (pFileSpace != &pMovie->FileSpaceList)
	{
		//
		// moov atom was found.
		//

		//
		// See if new moov atom can fit in old location.
		//
		if (un64DataLength != pFileSpace->un64Length)
		{
			if (un64DataLength <= pFileSpace->un64Length - 2 * sizeof(uint32_t))
			{
				//
				// Create free file space.
				//
				pFreeSpace = (FileSpace_t *)SMAllocPtr(sizeof(FileSpace_t));

				ASSERTRT(pFreeSpace != nullptr, SMRES_ALLOC_FAILED);

				pFreeSpace->unType = eAtom4CCfree;
				pFreeSpace->un64Offset = pFileSpace->un64Offset + un64DataLength;
				pFreeSpace->un64Length = pFileSpace->un64Length - un64DataLength;

				pFreeSpace->pNext = &pMovie->FileSpaceList;
				pFreeSpace->pPrev = pMovie->FileSpaceList.pPrev;

				pMovie->FileSpaceList.pPrev->pNext = pFreeSpace;
				pMovie->FileSpaceList.pPrev = pFreeSpace;

				pFileSpace->un64Length = un64DataLength;
			}
			else
			{
				//
				// Change the type of this file space to eAtom4CCfree and look for enough free space
				// to write the moov atom.
				//
				pFileSpace->unType = eAtom4CCfree;

				for (pFileSpace = pMovie->FileSpaceList.pNext; pFileSpace != &pMovie->FileSpaceList; pFileSpace = pFileSpace->pNext)
				{
					if (pFileSpace->unType == eAtom4CCfree)
					{
						un64FreeSpace = pFileSpace->un64Length;

						//
						// Add up all the consecutive free space from this point.
						//
						for (pFreeSpace = pFileSpace->pNext; pFreeSpace != &pMovie->FileSpaceList; pFreeSpace = pFreeSpace->pNext)
						{
							if (pFreeSpace->unType != eAtom4CCfree)
							{
								break;
							}

							un64FreeSpace += pFreeSpace->un64Length;
						}

						//
						// If we reached the end of the list then there is an unlimited amount
						// of free space.
						//
						if (pFreeSpace == &pMovie->FileSpaceList)
						{
							un64FreeSpace = un64DataLength;
						}

						if (un64DataLength == un64FreeSpace)
						{
							//
							// Convert this file space to eAtom4CCmoov and delete the other free space
							// atoms that immediately follow.
							//
							for (pFreeSpace = pFileSpace->pNext; pFreeSpace != &pMovie->FileSpaceList; pFreeSpace = pNextFreeSpace)
							{
								if (pFreeSpace->unType != eAtom4CCfree)
								{
									break;
								}

								pNextFreeSpace = pFreeSpace->pNext;

								pFreeSpace->pNext->pPrev = pFreeSpace->pPrev;
								pFreeSpace->pPrev->pNext = pFreeSpace->pNext;

								SMFreePtr(pFreeSpace);
							}

							pFileSpace->unType = eAtom4CCmoov;
							pFileSpace->un64Length = un64DataLength;

							break;
						}

						if (un64DataLength <= un64FreeSpace - 2 * sizeof(uint32_t))
						{
							un64FreeSpace = pFileSpace->un64Length;

							//
							// Convert this file space to eAtom4CCmoov and delete the other free space
							// atoms that immediately follow.
							//
							for (pFreeSpace = pFileSpace->pNext; pFreeSpace != &pMovie->FileSpaceList; pFreeSpace = pNextFreeSpace)
							{
								if (pFreeSpace->unType != eAtom4CCfree)
								{
									break;
								}

								un64FreeSpace += pFreeSpace->un64Length;

								if (un64FreeSpace - 2 * sizeof(uint32_t) > un64DataLength)
								{
									pFreeSpace->un64Length = pFileSpace->un64Offset + un64DataLength - (pFreeSpace->un64Offset + pFreeSpace->un64Length);
									pFreeSpace->un64Offset = pFileSpace->un64Offset + un64DataLength;

									break;
								}

								pNextFreeSpace = pFreeSpace->pNext;

								pFreeSpace->pNext->pPrev = pFreeSpace->pPrev;
								pFreeSpace->pPrev->pNext = pFreeSpace->pNext;

								SMFreePtr(pFreeSpace);
							}

							pFileSpace->unType = eAtom4CCmoov;
							pFileSpace->un64Length = un64DataLength;

							break;
						}

						pFileSpace = pFreeSpace;
					}
				}
			}
		}
	}

	if (pFileSpace == &pMovie->FileSpaceList)
	{
		//
		// We reached the end of the file so if the data is written it
		// will be written to a new moov atom at the end of the file.
		//
		pFileSpace = (FileSpace_t *)SMAllocPtr(sizeof(FileSpace_t));

		ASSERTRT(pFileSpace != nullptr, SMRES_ALLOC_FAILED);

		pFileSpace->unType = eAtom4CCmoov;
		pFileSpace->un64Offset = pMovie->FileSpaceList.pPrev->un64Offset + pMovie->FileSpaceList.pPrev->un64Length;
		pFileSpace->un64Length = un64DataLength;

		pFileSpace->pNext = &pMovie->FileSpaceList;
		pFileSpace->pPrev = pMovie->FileSpaceList.pPrev;

		pMovie->FileSpaceList.pPrev->pNext = pFileSpace;
		pMovie->FileSpaceList.pPrev = pFileSpace;
	}

	*pun64Offset = pFileSpace->un64Offset;

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; WriteMetaData
//
// Abstract:
//
// Returns:
//
SMResult_t WriteMetaData(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint64_t un64DataLength;
	uint64_t un64Offset;

	//
	// Get the meta data size
	//
	un64DataLength = pMovie->pMoovAtom->GetSize();

	SMResult = FindMoovLocation(pMovie, un64DataLength, &un64Offset);

	TESTRESULT();

	//
	// Write the data.
	//
	(pMovie->pDataHandler)->DHSetPosition(un64Offset);

	SMResult = pMovie->pMoovAtom->Write(pMovie->pDataHandler);

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4ReserveMoovSpace
//
// Abstract:
//
// Returns:
//
SMResult_t MP4ReserveMoovSpace(
	Movie_t *pMovie,
	uint32_t unLength)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	const uint32_t unZero = 0;
	uint32_t i;
	uint64_t un64Offset;

	SMResult = FindMoovLocation(pMovie, unLength, &un64Offset);

	TESTRESULT();

	//
	// Write the data.
	//
	(pMovie->pDataHandler)->DHSetPosition(un64Offset);

	for (i = 0; i < (unLength >> 2); i++)
	{
		(pMovie->pDataHandler)->DHWrite((void *)&unZero, sizeof(unZero));
	}

	for (i = 0; i < (unLength & 0x03); i++)
	{
		(pMovie->pDataHandler)->DHWrite((void *)&unZero, sizeof(uint8_t));
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; WriteFileSpaceSizes
//
// Abstract:
//
// Returns:
//
SMResult_t WriteFileSpaceSizes(
	const Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	FileSpace_t *pFileSpace;

	//
	// Traverse the file space linked list and write the sizes of each section to the file.
	//
	for (pFileSpace = pMovie->FileSpaceList.pNext; pFileSpace != &pMovie->FileSpaceList; pFileSpace = pFileSpace->pNext)
	{
		(pMovie->pDataHandler)->DHSetPosition(pFileSpace->un64Offset);

		if (pFileSpace->un64Length > 0xFFFFFFFF)
		{
			(pMovie->pDataHandler)->DHWrite4(1);

			(pMovie->pDataHandler)->DHWrite4(pFileSpace->unType);

			(pMovie->pDataHandler)->DHWrite8(pFileSpace->un64Length);
		}
		else
		{
			(pMovie->pDataHandler)->DHWrite4((uint32_t)pFileSpace->un64Length);

			(pMovie->pDataHandler)->DHWrite4(pFileSpace->unType);
		}
	}

	return SMResult;
}


