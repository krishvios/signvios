///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:  MP4Movie.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include <cstring>

//
// Sorenson includes
//
#include "SMCommon.h"
#include "SMTypes.h"
#include "SorensonErrors.h"
#include "SMMemory.h"

#include "SorensonMP4.h"
//#include "Objects.h"
#include "MoovAtom.h"
#include "MvhdAtom.h"
#include "IodsAtom.h"
#include "MdatAtom.h"
#include "TkhdAtom.h"
#include "UdtaAtom.h"
#include "CDataHandler.h"
#include "CFileDataHandler.h"
#include "CServerDataHandler.h"
#include "MP4Common.h"
#include "MP4Parser.h"
#include "MP4Signals.h"

//
// Constants
//

#define AVC_COMPRESSOR_NAME "AVC Coding"
#define MAX_TASK_CLOSE_WAIT 10  
#define AUDIO_ESDS_BOX_SIZE 51
#define AUDIO_ESDS_BOX_PRE_ALLOCATED_BYTES 12

//#define USE_LOCK_TRACKING
#ifdef USE_LOCK_TRACKING
extern int g_stiSInfoDebug;
	int g_nMP4LibLocks = 0;
	#define MP4_LOCK(A, B) \
		stiTrace ("<MP4Movie::Lock> Requesting lock from %s.  Lock = %p [%s]\n", A, B, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()));\
		stiOSMutexLock (B);\
		stiTrace ("<MP4Movie::Lock> Locks = %d, Lock = %p.  Exiting from %s\n", ++g_nMP4LibLocks, B, A);
	#define MP4_UNLOCK(A, B) \
		stiTrace ("<MP4Movie::Unlock> Freeing lock from %s, Lock = %p [%s]\n", A, B, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()));\
		stiOSMutexUnlock (B); \
		stiTrace ("<MP4Movie::Unlock> Locks = %d, Lock = %p.  Exiting from %s\n", --g_nMP4LibLocks, B, A);
#else
	#define MP4_LOCK(A, B) stiOSMutexLock (B);
	#define MP4_UNLOCK(A, B) stiOSMutexUnlock (B);
#endif

CstiSignalConnection::Collection mp4SignalConnections;          


///////////////////////////////////////////////////////////////////////////////
//; InitializeFileSpaceList
//
// Abstract:
//
// Returns:
//
static void InitializeFileSpaceList(
	Movie_t *pMovie)
{
	pMovie->FileSpaceList.pNext = &pMovie->FileSpaceList;
	pMovie->FileSpaceList.pPrev = &pMovie->FileSpaceList;

	pMovie->FileSpaceList.unType = 0;
	pMovie->FileSpaceList.un64Offset = 0;
	pMovie->FileSpaceList.un64Length = 0;
}


///////////////////////////////////////////////////////////////////////////////
//; FreeFileSpaceList
//
// Abstract:
//
// Returns:
//
static void FreeFileSpaceList(
	Movie_t *pMovie)
{
	FileSpace_t *pFileSpace;
	FileSpace_t *pNextFileSpace;

	for (pFileSpace = pMovie->FileSpaceList.pNext; pFileSpace != &pMovie->FileSpaceList; pFileSpace = pNextFileSpace)
	{
		pNextFileSpace = pFileSpace->pNext;
	
		SMFreePtr(pFileSpace);
	}

	pMovie->FileSpaceList.pNext = &pMovie->FileSpaceList;
	pMovie->FileSpaceList.pPrev = &pMovie->FileSpaceList;
}


///////////////////////////////////////////////////////////////////////////////
//; mp4SignalsConnect
//
// Abstract: Connectets MP4 signals to the CServerDataHandler signals.
//
// Returns:
//
void mp4SignalsConnect(CServerDataHandler *serverDataHandler)
{
	if (mp4SignalConnections.empty())
	{
		mp4SignalConnections.push_back(
			serverDataHandler->mediaSampleReadySignal.Connect (
				[](const SMediaSampleInfo &sampleInfo){
					mediaSampleReadySignal.Emit(sampleInfo);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->mediaSampleNotReadySignal.Connect (
				[](int notReadyState){
					mediaSampleNotReadySignal.Emit(notReadyState);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->errorSignal.Connect (
				[](stiHResult hResult){
					errorSignal.Emit(hResult);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->movieReadySignal.Connect (
				[](){
					movieReadySignal.Emit();
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->messageSizeSignal.Connect (
				[](uint64_t messageSize){
					messageSizeSignal.Emit(messageSize);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->messageIdSignal.Connect (
				[](std::string messageId){
					messageIdSignal.Emit(messageId);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->fileDownloadProgressSignal.Connect (
				[](const SFileDownloadProgress &downloadProgress){
					fileDownloadProgressSignal.Emit(downloadProgress);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->skipDownloadProgressSignal.Connect (
				[](const SFileDownloadProgress &skipProgress){
					skipDownloadProgressSignal.Emit(skipProgress);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->bufferUploadSpaceUpdateSignal.Connect (
				[](const SFileUploadProgress &uploadProgress){
					bufferUploadSpaceUpdateSignal.Emit(uploadProgress);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->uploadProgressSignal.Connect (
				[](uint32_t uploadProgress){
					uploadProgressSignal.Emit(uploadProgress);
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->uploadErrorSignal.Connect (
				[](){
					uploadErrorSignal.Emit();
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->downloadErrorSignal.Connect (
				[](){
					downloadErrorSignal.Emit();
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->uploadCompleteSignal.Connect (
				[](){
					uploadCompleteSignal.Emit();
				}));

		mp4SignalConnections.push_back(
			serverDataHandler->movieClosedSignal.Connect (
				[](Movie_t *movie){
					movieClosedSignal.Emit(movie);
				}));

	}
}

///////////////////////////////////////////////////////////////////////////////
//; mp4SignalsDisconnect
//
// Abstract: Disconnectets MP4 signals from the CServerDataHandler signals.
//
// Returns:
//
void mp4SignalsDisconnect()
{
	mp4SignalConnections.clear();
}

///////////////////////////////////////////////////////////////////////////////
//; MP4SetMovieBrands
//
// Abstract:
//
// Returns:
//
SMResult_t MP4SetMovieBrands(
	Movie_t *pMovie,
	uint32_t *pBrands,
	unsigned int unBrandLength)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4SetMovieBrands", pMovie->Mutex);
	
	ASSERTRT(unBrandLength >= 2, -1);

	if (pMovie->pFtypAtom == nullptr)
	{
		pMovie->pFtypAtom = new FtypAtom_c;

		ASSERTRT(pMovie->pFtypAtom != nullptr, SMRES_ALLOC_FAILED);
	}

	SMResult = pMovie->pFtypAtom->SetBrands(pBrands, unBrandLength);

	TESTRESULT();

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4SetMovieBrands", pMovie->Mutex);
	}
	
	return SMResult;
}

#if !defined(stiLINUX) && !defined(WIN32)
///////////////////////////////////////////////////////////////////////////////
//; MP4CreateMovieFile
//
// Abstract:
//
// Returns:
//
SMResult_t MP4CreateMovieFile(
#ifdef __SMMac__
	short vRefNum,
	long parID,
	Str255 fileName,
#endif // __SMMac__
#ifdef WIN32
	const char *pszFileName,
#endif // WIN32
	uint32_t *pBrands,
	unsigned int unBrandLength,
	Movie_t **ppMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	Movie_t *pMovie = nullptr;

	MP4NewMovie(&pMovie);

	ASSERTRT(pMovie != nullptr, SMRES_ALLOC_FAILED);
	MP4_LOCK ("MP4CreateMovieFile", pMovie->Mutex);
	
	SMResult = MP4SetMovieBrands(pMovie, pBrands, unBrandLength);

	TESTRESULT();

#ifdef __SMMac__
	pMovie->pDataHandler = new CFileDataHandler (vRefNum, parID, fileName);
#endif // SMMac

#ifdef WIN32
	pMovie->pDataHandler = new CFileDataHandler (pszFileName);
#endif // WIN32

	ASSERTRT(pMovie->pDataHandler != nullptr, SMRES_ALLOC_FAILED);

	SMResult = (pMovie->pDataHandler)->OpenDataHandlerForWrite();

	TESTRESULT();

	SMResult = WriteFileType(pMovie);

	TESTRESULT();

	*ppMovie = pMovie;

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4CreateMovieFile", pMovie->Mutex);
	}
	
	return SMResult;
} // MP4CreateMovieFile


///////////////////////////////////////////////////////////////////////////////
//; MP4CreateNewMovieFile
//
// Abstract:
//
// Returns:
//
SMResult_t MP4CreateNewMovieFile(
	Movie_t *pMovie,
#ifdef SMMac
	short vRefNum,
	long parID,
	Str255 fileName
#endif // SMMac
	const char *pszFileName
	)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	MP4_LOCK ("MP4CreateNewMovieFile", pMovie->Mutex);
	
	FreeFileSpaceList(pMovie);

	if (pMovie->pDataHandler)
	{
		(pMovie->pDataHandler)->CloseDataHandler();

		delete pMovie->pDataHandler;

		pMovie->pDataHandler = nullptr;
	}

#ifdef SMMac
	pMovie->pDataHandler = new CFileDataHandler(vRefNum, parID, fileName);
#endif // SMMac

#ifdef WIN32
	pMovie->pDataHandler = new CFileDataHandler(pszFileName);
#endif // WIN32

	ASSERTRT(pMovie->pDataHandler != nullptr, SMRES_ALLOC_FAILED);

	SMResult = (pMovie->pDataHandler)->OpenDataHandlerForWrite();

	TESTRESULT();

	SMResult = WriteFileType(pMovie);

	TESTRESULT();

smbail:

	MP4_UNLOCK ("MP4CreateNewMovieFile", pMovie->Mutex);
	
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4OpenMovieFile
//
// Abstract:
//
// Returns:
//
SMResult_t MP4OpenMovieFile(
#ifdef SMMac
	short vRefNum,
	long parID,
	Str255 fileName,
#endif // SMMac
#ifdef WIN32
	const char *pszFileName,
#endif // WIN32
	Movie_t **ppMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	Movie_t *pMovie = nullptr;

	MP4NewMovie(&pMovie);

	ASSERTRT(pMovie != nullptr, SMRES_ALLOC_FAILED);
	MP4_LOCK ("MP4OpenMovieFile", pMovie->Mutex);
	
#ifdef SMMac
	pMovie->pDataHandler = new CFileDataHandler(vRefNum, parID, fileName);
#endif // SMMac

#ifdef WIN32
	pMovie->pDataHandler = new CFileDataHandler(pszFileName);
#endif // WIN32

	ASSERTRT(pMovie->pDataHandler != nullptr, SMRES_ALLOC_FAILED);

	SMResult = (pMovie->pDataHandler)->OpenDataHandlerForRead();

	TESTRESULT();

	SMResult = ParseMovie(pMovie);

	TESTRESULT();

	pMovie->pMoovAtom->SetMovie(pMovie);

	*ppMovie = pMovie;

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4OpenMovieFile", pMovie->Mutex);
	}
	
	return SMResult;
} // MP4OpenMovieFile

#endif //#if !defined(stiLINUX)

///////////////////////////////////////////////////////////////////////////////
//; MP4OpenMovieFileAsync
//
// Abstract:
//
// Returns:
//
SMResult_t MP4OpenMovieFileAsync(
	const std::string &server,
	const std::string &fileGUID,
	uint64_t un64FileSize,
	const std::string &clientToken,
	uint32_t un32DownloadSpeed,
	Movie_t **ppMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;
	Movie_t *pMovie = nullptr;

	MP4NewMovie(&pMovie);

	ASSERTRT(pMovie != nullptr, SMRES_ALLOC_FAILED);
	MP4_LOCK ("MP4OpenMovieFileAsync", pMovie->Mutex);
	
	pMovie->pDataHandler = new CServerDataHandler(pMovie);
	ASSERTRT(pMovie->pDataHandler != nullptr, SMRES_ALLOC_FAILED);

	mp4SignalsConnect((CServerDataHandler *)(pMovie->pDataHandler));

	hResult = ((CServerDataHandler *) (pMovie->pDataHandler))->Initialize (pMovie->Mutex);
	ASSERTRT(stiRESULT_SUCCESS == hResult, SMRES_FILE_OPEN_FAILED);

	hResult = ((CServerDataHandler *) (pMovie->pDataHandler))->Startup ();
	ASSERTRT(stiRESULT_SUCCESS == hResult, SMRES_FILE_OPEN_FAILED);

	hResult = ((CServerDataHandler *) (pMovie->pDataHandler))->MovieFileInfoSet(server, 
																				fileGUID,
																				un32DownloadSpeed,
																				un64FileSize,
																				clientToken);
	ASSERTRT(stiRESULT_SUCCESS == hResult, SMRES_FILE_OPEN_FAILED);

	hResult = ((CServerDataHandler *) (pMovie->pDataHandler))->FileChunkGet (0, 1048576);
	ASSERTRT(stiRESULT_SUCCESS == hResult, SMRES_FILE_OPEN_FAILED);

	*ppMovie = pMovie;

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4OpenMovieFileAsync", pMovie->Mutex);
	}
	
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4ResetMovieDownloadGUID
//
// Abstract:
//
// Returns:
//
SMResult_t MP4ResetMovieDownloadGUID(
	const std::string &fileGUID,
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4ResetMovieDownloadGUID", pMovie->Mutex);

	if (pMovie->pDataHandler)
	{
		SMResult = ((CServerDataHandler *)(pMovie->pDataHandler))->ResetExpiredDownloadGUID(fileGUID);
	}

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4ResetMovieDownloadGUID", pMovie->Mutex);
	}

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4MovieDownloadSpeedSet
//
// Abstract:
//
// Returns:
//
SMResult_t MP4MovieDownloadSpeedSet(
	uint32_t un32DownloadSpeed,
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4MovieDownloadSpeedSet", pMovie->Mutex);

	if (pMovie->pDataHandler)
	{
		SMResult = ((CServerDataHandler *)(pMovie->pDataHandler))->DownloadSpeedSet(un32DownloadSpeed);
		ASSERTRT(SMResult == SMRES_SUCCESS, SMRES_INVALID_MOVIE);

		SMResult = ((CServerDataHandler *) (pMovie->pDataHandler))->FileChunkGet (0, 1048576);
	}


smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4MovieDownloadSpeedSet", pMovie->Mutex);
	}

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4MovieRestartDownload
//
// Abstract:
//
// Returns:
//
SMResult_t MP4MovieDownloadResume(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);

	MP4_LOCK ("MP4MovieRestartDownload", pMovie->Mutex);

	if (pMovie->pDataHandler)
	{
		SMResult = dynamic_cast<CServerDataHandler *>(pMovie->pDataHandler)->downloadResume();
	}


smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4MovieRestartDownload", pMovie->Mutex);
	}

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4CloseMovieFileAsync
//
// Abstract:
//
// Returns:
//
SMResult_t MP4CloseMovieFileAsync(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4CloseMovieFileAsync", pMovie->Mutex);
	
	if (pMovie->pDataHandler)
	{
		SMResult = (pMovie->pDataHandler)->CloseDataHandler();
	}

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4CloseMovieFileAsync", pMovie->Mutex);
	}

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4CleanupMovieFileAsync
//
// Abstract:
//
// Returns:
//
SMResult_t MP4CleanupMovieFileAsync(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);

	MP4_LOCK ("MP4CleanupMovieFileAsync", pMovie->Mutex);

	mp4SignalsDisconnect();

	if (pMovie->pMoovAtom)
	{
		pMovie->pMoovAtom->CloseDataHandler();
	}

	MP4_UNLOCK ("MP4CleanupMovieFileAsync", pMovie->Mutex);

	MP4DisposeMovie(pMovie);

smbail:

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4OpenMovieFileAsync
//
// Abstract:
//
// Returns:
//
SMResult_t MP4UploadMovieFileAsync(
	const std::string &server,
	const std::string &uploadGUID,
	const std::string &uploadFileName,
	Movie_t **ppMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;
	Movie_t *pMovie = nullptr;

	MP4NewMovie(&pMovie);
	
	ASSERTRT(pMovie != nullptr, SMRES_ALLOC_FAILED);
	MP4_LOCK ("MP4UploadMovieFileAsync", pMovie->Mutex);
	
	pMovie->pDataHandler = new CServerDataHandler(pMovie);

	ASSERTRT(pMovie->pDataHandler != nullptr, SMRES_ALLOC_FAILED);

	mp4SignalsConnect((CServerDataHandler *)(pMovie->pDataHandler));

	hResult = ((CServerDataHandler *) (pMovie->pDataHandler))->Initialize (pMovie->Mutex);

	ASSERTRT(stiRESULT_SUCCESS == hResult, SMRES_FILE_OPEN_FAILED);

	hResult = ((CServerDataHandler *) (pMovie->pDataHandler))->Startup ();
	
	ASSERTRT(stiRESULT_SUCCESS == hResult, SMRES_FILE_OPEN_FAILED);

	hResult = ((CServerDataHandler *) (pMovie->pDataHandler))->UploadMovieInfoSet(server, 
																				  uploadGUID,
																				  uploadFileName);
	ASSERTRT(stiRESULT_SUCCESS == hResult, SMRES_FILE_OPEN_FAILED);

	*ppMovie = pMovie;

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4UploadMovieFileAsync", pMovie->Mutex);
	}
	
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4OpenMovieFileAsync
//
// Abstract:
//
// Returns:
//
SMResult_t MP4UploadOptimizedMovieFileAsync(
	const std::string &server,
	const std::string &uploadGUID,
	uint32_t un32Width,
	uint32_t un32Height,
	uint32_t un32Bitrate,
	uint32_t un32Duration,
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_ALLOC_FAILED);
	MP4_LOCK ("MP4UploadOptimizedMovieFileAsync", pMovie->Mutex);
	
	ASSERTRT(pMovie->pDataHandler != nullptr, SMRES_ALLOC_FAILED);

	hResult = ((CServerDataHandler *) (pMovie->pDataHandler))->UploadOptimizedMovieInfoSet(server, 
																						   uploadGUID,
																						   un32Width,  
																						   un32Height, 
																						   un32Bitrate,
																						   un32Duration);
	ASSERTRT(stiRESULT_SUCCESS == hResult, SMRES_FILE_OPEN_FAILED);

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4UploadOptimizedMovieFileAsync", pMovie->Mutex);
	}
	
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4MovieWriteMdatAtom
//
// Abstract:
//
// Returns:
//
SMResult_t MP4MovieWriteMdatAtom(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	// Write out a mdat atom.
	SMResult = (pMovie->pDataHandler)->DHWrite4(0);
	TESTRESULT();

	SMResult = (pMovie->pDataHandler)->DHWrite4(eAtom4CCmdat);
	TESTRESULT();

smbail:
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4MovieWriteFtypAtom
//
// Abstract:
//
// Returns:
//
SMResult_t MP4MovieWriteFtypAtom(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	// Write out a Moov atom.
	if (pMovie->pFtypAtom)
	{
		SMResult = pMovie->pFtypAtom->Write(pMovie->pDataHandler);
	}
	TESTRESULT();

smbail:
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4MovieWriteMoovAtom
//
// Abstract:
//
// Returns:
//
SMResult_t MP4MovieWriteMoovAtom(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	// Write out a Moov atom.
	SMResult = pMovie->pMoovAtom->Write(pMovie->pDataHandler);
	TESTRESULT();

smbail:
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4GetMaxBufferingSize
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMaxBufferingSize(
	Movie_t *pMovie,
	uint32_t *punMaxBufferingSize)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4GetMaxBufferingSize", pMovie->Mutex);

	ASSERTRT(pMovie->pDataHandler != nullptr, SMRES_INVALID_MOVIE);

	*punMaxBufferingSize = ((CServerDataHandler *) (pMovie->pDataHandler))->MaxBufferingSizeGet();

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4GetMaxBufferingSize", pMovie->Mutex);
	}
	
	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4NewMovie
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewMovie(
	Movie_t **ppMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	   
	Movie_t *pMovie;
	MvhdAtom_c *pMvhd;

	pMovie = (Movie_t *)SMAllocPtrClear(sizeof(Movie_t));

	ASSERTRT(pMovie != nullptr, SMRES_ALLOC_FAILED);

	pMovie->Mutex = stiOSNamedMutexCreate ("MP4");

	InitializeFileSpaceList(pMovie);

	pMovie->pMoovAtom = new MoovAtom_c;

	ASSERTRT(pMovie->pMoovAtom != nullptr, SMRES_ALLOC_FAILED);

	pMovie->pMoovAtom->SetMovie(pMovie);

	pMvhd = new MvhdAtom_c;

	ASSERTRT(pMvhd != nullptr, SMRES_ALLOC_FAILED);
	
	SMResult = pMovie->pMoovAtom->AddAtom(pMvhd);
	
	TESTRESULT();

	pMvhd->SetTimeScale(600);
	
#if !defined(stiLINUX) && !defined(WIN32)
	pIods = new IodsAtom_c;
	
	ASSERTRT(pIods != nullptr, SMRES_ALLOC_FAILED);

	SMResult = pMovie->pMoovAtom->AddAtom(pIods);
	
	TESTRESULT();
#endif
	*ppMovie = pMovie;

smbail:
	if (SMResult != SMRES_SUCCESS)
	{
		MP4DisposeMovie(pMovie);
	}

	return SMResult;	
}


///////////////////////////////////////////////////////////////////////////////
//; MP4SetMovieTimeScale
//
// Abstract:
//
// Returns:
//
SMResult_t MP4SetMovieTimeScale(
	Movie_t *pMovie,
	uint32_t unTimeScale)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MoovAtom_c *pMoov;
	MvhdAtom_c *pMvhd;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4SetMovieTimeScale", pMovie->Mutex);

	ASSERTRT(pMovie->pMoovAtom != nullptr, SMRES_INVALID_MOVIE);

	pMoov = pMovie->pMoovAtom;

	pMvhd = (MvhdAtom_c *)pMoov->GetChildAtom(eAtom4CCmvhd);

	ASSERTRT(pMvhd != nullptr, SMRES_INVALID_MOVIE);

	pMvhd->SetTimeScale(unTimeScale);

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4SetMovieTimeScale", pMovie->Mutex);
	}
	
	return SMResult;
}



///////////////////////////////////////////////////////////////////////////////
//; MP4GetMovieTimeScale
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMovieTimeScale(
	Movie_t *pMovie,
	MP4TimeScale *punTimeScale)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MoovAtom_c *pMoov;
	MvhdAtom_c *pMvhd;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4GetMovieTimeScale", pMovie->Mutex);

	ASSERTRT(pMovie->pMoovAtom != nullptr, SMRES_INVALID_MOVIE);

	pMoov = pMovie->pMoovAtom;

	pMvhd = (MvhdAtom_c *)pMoov->GetChildAtom(eAtom4CCmvhd);

	ASSERTRT(pMvhd != nullptr, SMRES_INVALID_MOVIE);

	*punTimeScale = pMvhd->GetTimeScale();

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4GetMovieTimeScale", pMovie->Mutex);
	}
	
	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4NewMovieTrack
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewMovieTrack(
	Movie_t *pMovie,
	uint32_t unWidth,  		// Fixed 16.16
	uint32_t unHeight, 		// Fixed 16.16
	int16_t sVolume,   		// Fixed 8.8
	Track_t **ppTrack)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	TrakAtom_c *pTrak = nullptr;
	TkhdAtom_c *pTkhd = nullptr;
	MvhdAtom_c *pMvhd;
	MoovAtom_c *pMoov;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4NewMovieTrack", pMovie->Mutex);

	ASSERTRT(pMovie->pMoovAtom != nullptr, SMRES_INVALID_MOVIE);

	pMoov = pMovie->pMoovAtom;

	pTrak = new TrakAtom_c;

	ASSERTRT(pTrak != nullptr, SMRES_ALLOC_FAILED);

	pTkhd = new TkhdAtom_c;

	ASSERTRT(pTkhd != nullptr, SMRES_ALLOC_FAILED);

	pTkhd->SetDimensions(unWidth, unHeight);

	pTkhd->SetVolume(sVolume);

	pMvhd = (MvhdAtom_c *)pMoov->GetChildAtom(eAtom4CCmvhd);

	ASSERTRT(pMvhd != nullptr, SMRES_INVALID_MOVIE);

	pTkhd->SetTrackID(pMvhd->GetNextTrackID());

	SMResult = pTrak->AddAtom(pTkhd);

	TESTRESULT();

	// The pTkhd atom was added successfully so null it out so we don't 
	// delete it if there is an error. 
	pTkhd = nullptr;

	SMResult = pMoov->AddAtom(pTrak);

	TESTRESULT();

	*ppTrack = (Track_t *)pTrak;

smbail:
	if (SMResult != SMRES_SUCCESS)
	{
		if (pTrak)
		{
			pTrak->FreeAtom();

			// If pTkhd exists then we created it but didn't get it added to pTrak.
			if (pTkhd)
			{
				delete pTkhd;
				pTkhd = nullptr;
			}

			delete pTrak;
			pTrak = nullptr;
		}
	}

	if (pMovie)
	{
		MP4_UNLOCK ("MP4NewMovieTrack", pMovie->Mutex);
	}
	
	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; MP4NewAudioSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewAACSampleDescription(
	uint16_t sampleRate,
	uint32_t encodeBitrate,
	SMHandle hAudioSampleDescr)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	AudioSampleDescr_t *pSampleDescr;

	// The AUDIO_ESDS_BOX_PRE_ALLOCATED_BYTES bytes accounts for unESSize, unReserved10 and unReserved11 
	// which are already counted in the size of AudioSampleDescr_t 
	uint8_t esdsBoxData[AUDIO_ESDS_BOX_SIZE - AUDIO_ESDS_BOX_PRE_ALLOCATED_BYTES];
	uint8_t *pesdsBoxData = esdsBoxData;

	ASSERTRT(hAudioSampleDescr != nullptr, SMRES_ALLOC_FAILED);

	//	ES_Descriptor explaination (documented in ISO 14496-1)
	//  4 Tag descriptors
	//		0x03 ES_DescrTag
	//		0x04 DecoderConfigDescrTag
	//		0x05 DecSpecificInfoTag
	//		0x06 SLConfigDescrTag
	//	Each Tag descriptors is followed by 3 ExtDescrTagStartRange (0x80)
	// 
	//	ES_DescrTag: 0x03
	//		3 ExtDescrTagStartRange (3 bytes): 0x80 0x80 0x80
	// 		BaseDescriptor: (1 byte): 0x22
	// 		ES_ID (2 bytes): 0x00 0x00
	// 		streamDependenceFlag: (1 bit) 0
	//		URL_FLAG (1 bit): 0
	//		OCRstreamFlag (1 bit): 0
	// 		streamPriority (5 bits): 0 => 0x00
	pesdsBoxData[0] = 0x03;
	pesdsBoxData[1] = 0x80;
	pesdsBoxData[2] = 0x80;
	pesdsBoxData[3] = 0x80;
	pesdsBoxData[4] = 0x22;
	pesdsBoxData[5] = 0x00;
	pesdsBoxData[6] = 0x00;
	pesdsBoxData[7] = 0x00;
	pesdsBoxData += 8;
	 
	// 	DecoderConfigDescrTag: 0x04
	// 		3 ExtDescrTagStartRange (3 bytes): 0x80 0x80 0x80
	// 		BaseDescriptor: (1 byte) 0x14
	// 		ObjectTypeIdication: (1 byte) 0x40
	// 		streamType: (6 bit) 000101 -> 0x05
	// 		upstream: (1 bit) 0
	// 		const bit reserved: (1 bit) 1
	// 		bufferSizeDB: (3 bytes) - setting to 0.
	// 		maxBitrate: (4 bytes) - because we encode at a constant bitrate maxBitrate and avgBitrate are the same
	// 		avgBitrate: (4 bytes)
    pesdsBoxData[0] = 0x04;
	pesdsBoxData[1] = 0x80;
	pesdsBoxData[2] = 0x80;
	pesdsBoxData[3] = 0x80;
	pesdsBoxData[4] = 0x14;
	pesdsBoxData[5] = 0x40;
	pesdsBoxData[6] = 0x15;
	pesdsBoxData[7] = 0x00;
	pesdsBoxData[8] = 0x00;
	pesdsBoxData[9] = 0x00;
	pesdsBoxData += 10;
	*((uint32_t *)pesdsBoxData) = BigEndian4(encodeBitrate);
	pesdsBoxData += 4;
	*((uint32_t *)pesdsBoxData) = BigEndian4(encodeBitrate);
	pesdsBoxData += 4;
	 
	// 	DecoderSpecificInfoTag: 0x05
	// 		3 EstDescrTagStartRange (3 bytes): 0x80 0x80 0x80
	// 		DecoderSpecificInfo size: (1 byte) 0x02
	// 		Object type: 5 bits (00010 - 2 AAC LC)	  
	// 		SamplingFrequencyIndex: 4 bits (0011 - 3 48000)
	// 		ChannelConfiguration: 4 bits (0010 - 2 Left and Right)
	//      frameLengthFlag: 1 bit (0)
	//		dependsOnCoreCoder: 1 bit (0)
	//		extensionFlag: 1 bit (0)
	pesdsBoxData[0] = 0x05;
	pesdsBoxData[1] = 0x80;
	pesdsBoxData[2] = 0x80;
	pesdsBoxData[3] = 0x80;
	pesdsBoxData[4] = 0x02;
	pesdsBoxData[5] = 0x11;
	pesdsBoxData[6] = 0x90;
	pesdsBoxData += 7;
 
	// 	SLConfigDescrTag: 0x06
	// 		2 EstDescrTagStartRange (3 bytes): 0x80 0x80 0x80
	// 		BaseDescriptor: (1 byte) 0x01
	// 		predefined: (1 byte) 0x02
	pesdsBoxData[0] = 0x06;
	pesdsBoxData[1] = 0x80;
	pesdsBoxData[2] = 0x80;
	pesdsBoxData[3] = 0x80;
	pesdsBoxData[4] = 0x01;
	pesdsBoxData[5] = 0x02;

	SMSetHandleSize(hAudioSampleDescr, sizeof(AudioSampleDescr_t) + AUDIO_ESDS_BOX_SIZE - AUDIO_ESDS_BOX_PRE_ALLOCATED_BYTES);
	pSampleDescr = (AudioSampleDescr_t *)*hAudioSampleDescr;

	pSampleDescr->unSize = BigEndian4(sizeof(AudioSampleDescr_t) + AUDIO_ESDS_BOX_SIZE - AUDIO_ESDS_BOX_PRE_ALLOCATED_BYTES);
	pSampleDescr->unDescrType = BigEndian4(eAudioSampleDesc);
	memset(pSampleDescr->aucReserved, 0, sizeof(pSampleDescr->aucReserved));
	pSampleDescr->usDataRefIndex = 0;
	memset(pSampleDescr->aunReserved1, 0, sizeof(pSampleDescr->aunReserved1));
	pSampleDescr->usChannelCount = BigEndian2(2);
	pSampleDescr->usSampleSize = BigEndian2(16);

	pSampleDescr->unReserved4 = 0;
	pSampleDescr->usSampleRate = BigEndian2(sampleRate);
	pSampleDescr->usReserved5 = 0;
	pSampleDescr->unESSize = BigEndian4(AUDIO_ESDS_BOX_SIZE);
	pSampleDescr->unReserved10 = BigEndian4(eAtom4CCesds);
	pSampleDescr->unReserved11 = 0;
	memcpy(pSampleDescr->aucESDescriptor, esdsBoxData, AUDIO_ESDS_BOX_SIZE - AUDIO_ESDS_BOX_PRE_ALLOCATED_BYTES);

smbail:

	return SMResult;
}


#if !defined(stiLINUX) && !defined(WIN32)
///////////////////////////////////////////////////////////////////////////////
//; MP4SetMovieIOD
//
// Abstract:
//
// Returns:
//
SMResult_t MP4SetMovieIOD(
	Movie_t *pMovie,
	InitialObjectDescr_i *pIOD)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MoovAtom_c *pMoovAtom;
	IodsAtom_c *pIodsAtom;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);

	MP4_LOCK ("MP4SetMovieIOD", pMovie->Mutex);
	
	pMoovAtom = (MoovAtom_c *)pMovie->pMoovAtom;

	ASSERTRT(pMoovAtom != nullptr, SMRES_INVALID_MOVIE);

	pIodsAtom = (IodsAtom_c *)pMoovAtom->GetChildAtom(eAtom4CCiods);

	ASSERTRT(pIodsAtom != nullptr, SMRES_INVALID_MOVIE);

	SMResult = pIodsAtom->SetIOD(pIOD);

	TESTRESULT();

smbail:

	MP4_UNLOCK ("MP4SetMovieIOD", pMovie->Mutex);
	
	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMovieIOD
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMovieIOD(
	Movie_t *pMovie,
	InitialObjectDescr_i **ppIOD)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MoovAtom_c *pMoovAtom;
	IodsAtom_c *pIodsAtom;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);

	MP4_LOCK ("MP4GetMovieIOD", pMovie->Mutex);
	
	pMoovAtom = (MoovAtom_c *)pMovie->pMoovAtom;

	ASSERTRT(pMoovAtom != nullptr, SMRES_INVALID_MOVIE);

	pIodsAtom = (IodsAtom_c *)pMoovAtom->GetChildAtom(eAtom4CCiods);

	ASSERTRT(pIodsAtom != nullptr, SMRES_INVALID_MOVIE);

	*ppIOD = pIodsAtom->GetIOD();

smbail:

	MP4_UNLOCK ("MP4GetMovieIOD", pMovie->Mutex);
	
	return SMResult;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//; MP4SaveMovie
//
// Abstract:
//
// Returns:
//
SMResult_t MP4SaveMovie(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4SaveMovie", pMovie->Mutex);

	ASSERTRT(pMovie->pMoovAtom != nullptr, SMRES_INVALID_MOVIE);

	//
	// Write the moov atom and it's children to the file.
	//
	SMResult = WriteMetaData(pMovie);

	TESTRESULT();

	//
	// Write the sizes of each section (mdat, free, etc) to the file.
	//
	SMResult = WriteFileSpaceSizes(pMovie);

	TESTRESULT();

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4SaveMovie", pMovie->Mutex);
	}
	
	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; MP4CloseMovieFile
//
// Abstract:
//
// Returns:
//
SMResult_t MP4CloseMovieFile(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4CloseMovieFile", pMovie->Mutex);
	
	SMResult = (pMovie->pDataHandler)->CloseDataHandler();

	if (pMovie->pMoovAtom)
	{
		pMovie->pMoovAtom->CloseDataHandler();
	}

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4CloseMovieFile", pMovie->Mutex);
	}
	
	return SMResult;
}

#if !defined(stiLINUX) && !defined(WIN32)
///////////////////////////////////////////////////////////////////////////////
//; MP4NewAudioSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewAudioSampleDescription(
	ESDescriptor_i *pESDescriptor,
	SMHandle *phSampleDescription)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	ObjectEncoder_i *pObjectEncoder = nullptr;
	uint8_t aucBuffer[128];
	uint32_t unLength;
	SMHandle hSampleDescr;
	AudioSampleDescr_t *pSampleDescr;

	pObjectEncoder = CreateObjectEncoder();

	ASSERTRT(pObjectEncoder != nullptr, SMRES_ALLOC_FAILED);

	unLength = sizeof(aucBuffer);

	SMResult = pObjectEncoder->EncodeDescriptor(pESDescriptor, aucBuffer, &unLength);

	TESTRESULT();

	hSampleDescr = SMNewHandle(sizeof(AudioSampleDescr_t) + unLength - 1);

	ASSERTRT(hSampleDescr != nullptr, SMRES_ALLOC_FAILED);

	pSampleDescr = (AudioSampleDescr_t *)*hSampleDescr;

	pSampleDescr->unSize = BigEndian4(sizeof(AudioSampleDescr_t) + unLength - 1);
	pSampleDescr->unDescrType = BigEndian4(eAudioSampleDesc);
	memset(pSampleDescr->aucReserved, 0, sizeof(pSampleDescr->aucReserved));
	pSampleDescr->usDataRefIndex = 0;
	memset(pSampleDescr->aunReserved1, 0, sizeof(pSampleDescr->aunReserved1));
	pSampleDescr->usChannelCount = BigEndian2(2);
	pSampleDescr->usSampleSize = BigEndian2(16);
	pSampleDescr->unReserved4 = 0;
	pSampleDescr->usSampleRate = 0;
	pSampleDescr->usReserved5 = 0;
	pSampleDescr->unESSize = BigEndian4(unLength + sizeof(uint32_t) * 3);
	pSampleDescr->unReserved10 = BigEndian4(eAtom4CCesds);
	pSampleDescr->unReserved11 = 0;
	memcpy(pSampleDescr->aucESDescriptor, aucBuffer, unLength);

	*phSampleDescription = hSampleDescr;

smbail:

	if (pObjectEncoder)
	{
		DestroyObjectEncoder(pObjectEncoder);
	}

	return SMResult;
} // MP4NewAudioSampleDescription


///////////////////////////////////////////////////////////////////////////////
//; MP4NewMPEGSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewMPEGSampleDescription(
	ESDescriptor_i *pESDescriptor,
	SMHandle *phSampleDescription)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	ObjectEncoder_i *pObjectEncoder = nullptr;
	uint8_t aucBuffer[128];
	uint32_t unLength;
	SMHandle hSampleDescr;
	MPEGSampleDescr_t *pSampleDescr;

	pObjectEncoder = CreateObjectEncoder();

	ASSERTRT(pObjectEncoder != nullptr, SMRES_ALLOC_FAILED);

	unLength = sizeof(aucBuffer);

	SMResult = pObjectEncoder->EncodeDescriptor(pESDescriptor, aucBuffer, &unLength);

	TESTRESULT();

	hSampleDescr = SMNewHandle(sizeof(MPEGSampleDescr_t) + unLength - 1);

	ASSERTRT(hSampleDescr != nullptr, SMRES_ALLOC_FAILED);

	pSampleDescr = (MPEGSampleDescr_t *)*hSampleDescr;

	pSampleDescr->unSize = BigEndian4(sizeof(MPEGSampleDescr_t) + unLength - 1);
	pSampleDescr->unDescrType = BigEndian4(eMP4SampleDesc);
	memset(pSampleDescr->aucReserved, 0, sizeof(pSampleDescr->aucReserved));
	pSampleDescr->usDataRefIndex = 0;
	pSampleDescr->unESSize = BigEndian4(unLength + sizeof(uint32_t) * 3);
	pSampleDescr->unReserved10 = BigEndian4(eAtom4CCesds);
	pSampleDescr->unReserved11 = 0;
	memcpy(pSampleDescr->aucESDescriptor, aucBuffer, unLength);

	*phSampleDescription = hSampleDescr;

smbail:

	if (pObjectEncoder)
	{
		DestroyObjectEncoder(pObjectEncoder);
	}

	return SMResult;
} // MP4NewMPEGSampleDescription


///////////////////////////////////////////////////////////////////////////////
//; MP4NewVisualSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewVisualSampleDescription(
	ESDescriptor_i *pESDescriptor,
	uint16_t usWidth,
	uint16_t usHeight,
	SMHandle *phSampleDescription)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	ObjectEncoder_i *pObjectEncoder = nullptr;
	uint8_t aucBuffer[128];
	uint32_t unLength;
	SMHandle hSampleDescr;
	VisualSampleDescr_t *pSampleDescr;

	pObjectEncoder = CreateObjectEncoder();

	ASSERTRT(pObjectEncoder != nullptr, SMRES_ALLOC_FAILED);

	unLength = sizeof(aucBuffer);

	SMResult = pObjectEncoder->EncodeDescriptor(pESDescriptor, aucBuffer, &unLength);

	TESTRESULT();

	hSampleDescr = SMNewHandle(sizeof(VisualSampleDescr_t) + unLength - 1);

	ASSERTRT(hSampleDescr != nullptr, SMRES_ALLOC_FAILED);

	pSampleDescr = (VisualSampleDescr_t *)*hSampleDescr;

	pSampleDescr->unSize = BigEndian4(sizeof(VisualSampleDescr_t) + unLength - 1);
	pSampleDescr->unDescrType = BigEndian4(eVisualSampleDesc);
	memset(pSampleDescr->aucReserved, 0, sizeof(pSampleDescr->aucReserved));
	pSampleDescr->usDataRefIndex = 0;
	memset(pSampleDescr->aunReserved1, 0, sizeof(pSampleDescr->aunReserved1));
	pSampleDescr->usWidth = BigEndian2(usWidth);
	pSampleDescr->usHeight = BigEndian2(usHeight);
	pSampleDescr->unHorizRes = BigEndian4(0x00480000);
	pSampleDescr->unVertRes = BigEndian4(0x00480000);
	pSampleDescr->unReserved5 = 0;
	pSampleDescr->usFrameCount = BigEndian2(1);
	memset(pSampleDescr->aucCompressorName, 0, sizeof(pSampleDescr->aucCompressorName));
	pSampleDescr->usDepth = BigEndian2(24);
	pSampleDescr->sReserved9 = BigEndian2(-1);
	pSampleDescr->unESSize = BigEndian4(unLength + sizeof(uint32_t) * 3);
	pSampleDescr->unReserved10 = BigEndian4(eAtom4CCesds);
	pSampleDescr->unReserved11 = 0;
	memcpy(pSampleDescr->aucESDescriptor, aucBuffer, unLength);

	*phSampleDescription = hSampleDescr;

smbail:

	if (pObjectEncoder)
	{
		DestroyObjectEncoder(pObjectEncoder);
	}

	return SMResult;
} // MP4NewVisualSampleDescription

///////////////////////////////////////////////////////////////////////////////
//; MP4NewRTPSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewRTPSampleDescription(
	SMHandle *phSampleDescription)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	SMHandle hSampleDescr;
	RTPSampleDescr_t *pSampleDescr;

	hSampleDescr = SMNewHandle(sizeof(RTPSampleDescr_t));

	ASSERTRT(hSampleDescr != nullptr, SMRES_ALLOC_FAILED);

	pSampleDescr = (RTPSampleDescr_t *)*hSampleDescr;

	pSampleDescr->unSize = BigEndian4(sizeof(RTPSampleDescr_t));
	pSampleDescr->unDescrType = BigEndian4(eRTPSampleDesc);
	memset(pSampleDescr->aucReserved, 0, sizeof(pSampleDescr->aucReserved));
	pSampleDescr->usDataRefIndex = 0;
	pSampleDescr->usHintTrackVersion = BigEndian2(1);
	pSampleDescr->usHighestCompatibleVersion = BigEndian2(1);
	pSampleDescr->unMaxPacketSize = 0;
	pSampleDescr->unReserved2 = BigEndian4(12);
	pSampleDescr->unReserved3 = BigEndian4(eAtom4CCtims);
	pSampleDescr->unTimeScale = 0;

	*phSampleDescription = hSampleDescr;

smbail:

	return SMResult;
} // MP4NewRTPSampleDescription


///////////////////////////////////////////////////////////////////////////////
//; MP4GetESDescriptor
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetESDescriptor(
	SMHandle hSampleDescription,
	ESDescriptor_i **ppESDescriptor)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	ObjectDecoder_i *pObjectDecoder = nullptr;
	VisualSampleDescr_t *pSampleDescr;
	uint8_t *pESDescrData;
	uint32_t unESDescrLength;
	ESDescriptor_i *pESDescriptor;

	ASSERTRT(hSampleDescription != nullptr, SMRES_INVALID_SAMPLE_DESCRIPTION);
	ASSERTRT(*hSampleDescription != nullptr, SMRES_INVALID_SAMPLE_DESCRIPTION);

	pSampleDescr = (VisualSampleDescr_t *)*hSampleDescription;

	switch (BigEndian4(pSampleDescr->unDescrType))
	{
		case eAudioSampleDesc:

			pESDescrData = ((AudioSampleDescr_t *)pSampleDescr)->aucESDescriptor;
			unESDescrLength = BigEndian4(((AudioSampleDescr_t *)pSampleDescr)->unESSize) - sizeof(uint32_t) * 3;

			break;

		case eVisualSampleDesc:

			pESDescrData = ((VisualSampleDescr_t *)pSampleDescr)->aucESDescriptor;
			unESDescrLength = BigEndian4(((VisualSampleDescr_t *)pSampleDescr)->unESSize) - sizeof(uint32_t) * 3;

			break;

		case eMP4SampleDesc:

			pESDescrData = ((MPEGSampleDescr_t *)pSampleDescr)->aucESDescriptor;
			unESDescrLength = BigEndian4(((MPEGSampleDescr_t *)pSampleDescr)->unESSize) - sizeof(uint32_t) * 3;

			break;

		default:

			ASSERTRT(eFalse, SMRES_UNSUPPORTED_SAMPLE_DESCRIPTION);
	}

	pObjectDecoder = CreateObjectDecoder();

	ASSERTRT(pObjectDecoder != nullptr, SMRES_ALLOC_FAILED);

	pESDescriptor = (ESDescriptor_i *)pObjectDecoder->DecodeDescriptor((char *)pESDescrData, unESDescrLength, nullptr, nullptr);
	
	ASSERTRT(pESDescriptor != nullptr, SMRES_ALLOC_FAILED);

	*ppESDescriptor = pESDescriptor;

smbail:

	if (pObjectDecoder)
	{
		DestroyObjectDecoder(pObjectDecoder);
	}

	return SMResult;
} // MP4GetESDescriptor


///////////////////////////////////////////////////////////////////////////////
//; MP4SetESDescriptor
//
// Abstract:
//
// Returns:
//
SMResult_t MP4SetESDescriptor(
	SMHandle hSampleDescription,
	ESDescriptor_i *pESDescriptor)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	ObjectEncoder_i *pObjectEncoder = nullptr;
	AudioSampleDescr_t *pAudioSampleDescr;
	VisualSampleDescr_t *pVisualSampleDescr;
	MPEGSampleDescr_t *pMPEGSampleDescr;
	uint8_t aucBuffer[128];
	uint32_t unLength;

	ASSERTRT(hSampleDescription != nullptr, SMRES_INVALID_SAMPLE_DESCRIPTION);
	ASSERTRT(*hSampleDescription != nullptr, SMRES_INVALID_SAMPLE_DESCRIPTION);

	pObjectEncoder = CreateObjectEncoder();

	ASSERTRT(pObjectEncoder != nullptr, SMRES_ALLOC_FAILED);

	unLength = sizeof(aucBuffer);

	SMResult = pObjectEncoder->EncodeDescriptor(pESDescriptor, aucBuffer, &unLength);

	TESTRESULT();

	switch (BigEndian4(((SampleDescr_t *)*hSampleDescription)->unDescrType))
	{
		case eAudioSampleDesc:

			SMResult = SMSetHandleSize(hSampleDescription, sizeof(AudioSampleDescr_t) + unLength - 1);

			TESTRESULT();

			pAudioSampleDescr = (AudioSampleDescr_t *)*hSampleDescription;

			pAudioSampleDescr->unSize = BigEndian4(sizeof(AudioSampleDescr_t) + unLength - 1);
			pAudioSampleDescr->unESSize = BigEndian4(unLength + sizeof(uint32_t) * 3);
			memcpy(pAudioSampleDescr->aucESDescriptor, aucBuffer, unLength);

			break;

		case eVisualSampleDesc:

			SMResult = SMSetHandleSize(hSampleDescription, sizeof(VisualSampleDescr_t) + unLength - 1);

			TESTRESULT();

			pVisualSampleDescr = (VisualSampleDescr_t *)*hSampleDescription;

			pVisualSampleDescr->unSize = BigEndian4(sizeof(VisualSampleDescr_t) + unLength - 1);
			pVisualSampleDescr->unESSize = BigEndian4(unLength + sizeof(uint32_t) * 3);
			memcpy(pVisualSampleDescr->aucESDescriptor, aucBuffer, unLength);

			break;

		case eMP4SampleDesc:

			SMResult = SMSetHandleSize(hSampleDescription, sizeof(MPEGSampleDescr_t) + unLength - 1);

			TESTRESULT();

			pMPEGSampleDescr = (MPEGSampleDescr_t *)*hSampleDescription;

			pMPEGSampleDescr->unSize = BigEndian4(sizeof(MPEGSampleDescr_t) + unLength - 1);
			pMPEGSampleDescr->unESSize = BigEndian4(unLength + sizeof(uint32_t) * 3);
			memcpy(pMPEGSampleDescr->aucESDescriptor, aucBuffer, unLength);

			break;

		default:

			ASSERTRT(eFalse, SMRES_UNSUPPORTED_SAMPLE_DESCRIPTION);
	}

smbail:

	if (pObjectEncoder)
	{
		DestroyObjectEncoder(pObjectEncoder);
	}

	return SMResult;
} // MP4SetESDescriptor
#endif

///////////////////////////////////////////////////////////////////////////////
//; MP4NewVisualSampleDescription
//
// Abstract:
//
// Returns:
//
SMResult_t MP4NewAVCSampleDescription(
	const uint8_t *pConfigRecord,
	unsigned int unConfigRecordLength,
	uint16_t usWidth,
	uint16_t usHeight,
	SMHandle *phSampleDescription)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	SMHandle hSampleDescr;
	AVCSampleDescr_t *pSampleDescr;

	hSampleDescr = SMNewHandle(sizeof(AVCSampleDescr_t) + unConfigRecordLength - 1);

	ASSERTRT(hSampleDescr != nullptr, SMRES_ALLOC_FAILED);

	pSampleDescr = (AVCSampleDescr_t *)*hSampleDescr;

	pSampleDescr->unSize = BigEndian4(sizeof(AVCSampleDescr_t) + unConfigRecordLength - 1);
	pSampleDescr->unDescrType = BigEndian4(eAVCSampleDesc);
	memset(pSampleDescr->aucReserved, 0, sizeof(pSampleDescr->aucReserved));
	pSampleDescr->usDataRefIndex = 0;
	memset(pSampleDescr->aunReserved1, 0, sizeof(pSampleDescr->aunReserved1));
	pSampleDescr->usWidth = BigEndian2(usWidth);
	pSampleDescr->usHeight = BigEndian2(usHeight);
	pSampleDescr->unHorizRes = BigEndian4(0x00480000);
	pSampleDescr->unVertRes = BigEndian4(0x00480000);
	pSampleDescr->unReserved5 = 0;
	pSampleDescr->usFrameCount = BigEndian2(1);
	memset(pSampleDescr->aucCompressorName, 0, sizeof(pSampleDescr->aucCompressorName));
	pSampleDescr->aucCompressorName[0] = static_cast<uint8_t>(strlen(AVC_COMPRESSOR_NAME));
	strcpy((char *)&pSampleDescr->aucCompressorName[1], AVC_COMPRESSOR_NAME);
	pSampleDescr->usDepth = BigEndian2(24);
	pSampleDescr->sReserved9 = BigEndian2(~0);
	pSampleDescr->unConfigSize = BigEndian4(unConfigRecordLength + 2 * sizeof(uint32_t));
	pSampleDescr->unConfigType = BigEndian4(eAtom4CCavcC);
	memcpy(pSampleDescr->aucConfig, pConfigRecord, unConfigRecordLength);

	*phSampleDescription = hSampleDescr;

smbail:

	return SMResult;
} // MP4NewVisualSampleDescription

///////////////////////////////////////////////////////////////////////////////
//; MP4DisposeMovie
//
// Abstract:
//
// Returns:
//
SMResult_t MP4DisposeMovie(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	if (pMovie)
	{
		MP4_LOCK ("MP4DisposeMovie", pMovie->Mutex);
		
		FreeFileSpaceList(pMovie);

		if (pMovie->pMoovAtom)
		{
			pMovie->pMoovAtom->FreeAtom();
			pMovie->pMoovAtom = nullptr;
		}

		if (pMovie->pDataHandler)
		{
			delete pMovie->pDataHandler;
			pMovie->pDataHandler = nullptr;
		}

		if (pMovie->pFtypAtom)
		{
			pMovie->pFtypAtom->FreeAtom();
			
			pMovie->pFtypAtom = nullptr;
		}

		// The Mutex must be unlocked before it can be destroyed.

		MP4_UNLOCK ("MP4DisposeMovie", pMovie->Mutex);

		if (pMovie->Mutex)
		{
			#ifdef USE_LOCK_TRACKING
			stiTrace ("<MP4Movie::MP4DisposeMovie> Deleting lock %p, Locks = %d.\n",
				 pMovie->Mutex, --g_nMP4LibLocks);
			#endif
			stiOSMutexDestroy (pMovie->Mutex);
			
			pMovie->Mutex = nullptr;
		}
		
		SMFreePtr(pMovie);
	}

//smbail:
	
	return SMResult;
} // MP4DisposeMovie


///////////////////////////////////////////////////////////////////////////////
//; MP4GetMovieDuration
//
// Abstract:
//
// Returns:
//
SMResult_t MP4GetMovieDuration(
	Movie_t *pMovie,
	MP4TimeValue *pMovieDuration)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MoovAtom_c *pMoov;
	MvhdAtom_c *pMvhd;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4GetMovieDuration", pMovie->Mutex);
	
	pMoov = pMovie->pMoovAtom;

	ASSERTRT(pMoov != nullptr, SMRES_INVALID_MOVIE);

	pMvhd = pMoov->m_pMvhd;

	ASSERTRT(pMvhd != nullptr, SMRES_INVALID_MOVIE);

	*pMovieDuration = pMvhd->GetDuration();

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4GetMovieDuration", pMovie->Mutex);
	}
	
	return SMResult;
} // MP4GetMovieDuration


///////////////////////////////////////////////////////////////////////////////
//; MP4AddMovieUesrData
//
// Abstract:
//
// Returns:
//
SMResult_t MP4AddMovieUserData(
	Movie_t *pMovie,
	uint32_t unType,
	const uint8_t *pucData,
	uint32_t unLength)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	MoovAtom_c *pMoovAtom;
	UdtaAtom_c *pUdtaAtom;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4AddMovieUesrData", pMovie->Mutex);
	
	pMoovAtom = pMovie->pMoovAtom;

	ASSERTRT(pMoovAtom != nullptr, SMRES_INVALID_MOVIE);

	pUdtaAtom = (UdtaAtom_c *)pMoovAtom->GetChildAtom(eAtom4CCudta);

	if (pUdtaAtom == nullptr)
	{
		pUdtaAtom = new UdtaAtom_c;

		ASSERTRT(pUdtaAtom != nullptr, SMRES_ALLOC_FAILED);

		pMoovAtom->AddAtom(pUdtaAtom);
	}

	pUdtaAtom->AddUserData(unType, pucData, unLength);

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4AddMovieUesrData", pMovie->Mutex);
	}
	
	return SMResult;
} // MP4AddMovieUserData


///////////////////////////////////////////////////////////////////////////////
//; SMResult_t MP4ParseAtom
//
// Abstract:
//
// Returns:
//
SMResult_t MP4ParseAtom (
	Movie_t *pMovie,
	uint64_t un64FileOffset,
	uint64_t *pun64DataParsed,
	uint32_t *pun32AtomType,
	uint32_t *pun32HeaderSize)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	ASSERTRT(pMovie != nullptr, SMRES_INVALID_MOVIE);
	MP4_LOCK ("MP4ParseAtom", pMovie->Mutex);
	
	SMResult = ParseAtom (pMovie, un64FileOffset, pun64DataParsed, pun32AtomType, pun32HeaderSize);

smbail:

	if (pMovie)
	{
		MP4_UNLOCK ("MP4ParseAtom", pMovie->Mutex);
	}
	
	return SMResult;
}

void MP4LogInfoToSplunk (
	Movie_t *movie)
{
	((CServerDataHandler *) (movie->pDataHandler))->logInfoToSplunk ();
}

