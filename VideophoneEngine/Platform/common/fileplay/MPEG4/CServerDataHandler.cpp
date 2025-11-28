////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CServerDataHandler
//
//  File Name:	CServerDataHandler.cpp
//
//  Owner: Ting-Yu Yang
//
//	Abstract: A MP4 movie data handler which is responsible to download 
//			  movie data from a server.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#if defined(stiLINUX) || defined(WIN32)
#include "stiTools.h"
#include <sstream>
#endif

#include <cctype> // for isdigit
#include <cerrno>
#include "stiTrace.h"
#include "CServerDataHandler.h"
#include "stiHTTPServiceTools.h"
#include "CstiExtendedEvent.h"
#include "stiTaskInfo.h"

#ifdef WIN32
#define close stiOSClose
#define __func__ __FUNCTION__
#endif

//
// Constants
//
static const uint32_t un32FIRST_FRAME = 1;
static const unsigned unSKIPBACK_BUFFER_SECONDS = 10;

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//#define stiBUFFERING_DEBUG
#ifdef stiBUFFERING_DEBUG
EstiBool g_bStartPlaying = estiFALSE;
#endif


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::CServerDataHandler
//
//  Description: Class constructor.
//
//  Abstract:
//
//  Returns: None.
//
CServerDataHandler::CServerDataHandler (
	Movie_t *pMovie)
	:
	CstiEventQueue("CServerDataHandler"),
	m_unOpenCount (0),
	m_un32FrameIndexToFree (un32FIRST_FRAME),
	m_unSkipBackTimeBuf (0),
	m_un64DownloadDataTarget (0),
	m_un64ParseOffset (0),
	m_un64MetaDataLength (0),
	m_pMovie (pMovie)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::CServerDataHandler);

} // end CServerDataHandler::CServerDataHandler


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::~CServerDataHandler
//
//  Description: Class destructor.
//
//  Abstract:
//
//  Returns: None.
//
CServerDataHandler::~CServerDataHandler ()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::~CServerDataHandler);

	delete m_pDataTransfer;
	m_pDataTransfer = nullptr;
} // end CServerDataHandler::~CServerDataHandler


stiHResult CServerDataHandler::Initialize (
	stiMUTEX_ID mIdMP4LibProtect)
{
	m_muxMP4LibProtect = mIdMP4LibProtect;
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace ("<CServerDataHandler::Init>\n");
	);

	hResult = m_oCircBuf.InitCircularBuffer ();
	stiTESTRESULT ();

	m_pDataTransfer = new CDataTransferHandler();
	stiTESTCOND (m_pDataTransfer, stiRESULT_ERROR);

	hResult = m_pDataTransfer->Initialize(&m_oCircBuf);
	stiTESTRESULT ();

	dataTransferHandlerSignalsConnect();

STI_BAIL:

	return (hResult);	
}


stiHResult CServerDataHandler::UploadMovieInfoSet(
	const std::string &server,
	const std::string &uploadGUID,
	const std::string &uploadFileName)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	hResult = m_pDataTransfer->UploadMovieInfoSet(server,
												  uploadGUID,
												  uploadFileName);

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::UploadOptimizedMovieInfoSet
//
// Description: Initialize the ServerDataHandler to upload an optimized file.
//
// Abstract:
//
// Returns:
//	estiRESULT_SUCCESS - Success
//	estiRESULT_ERROR - Failure
//
stiHResult CServerDataHandler::UploadOptimizedMovieInfoSet(
	const std::string &server,
	const std::string &uploadGUID,
	uint32_t un32Width,
	uint32_t un32Height,
	uint32_t un32Bitrate,
	uint32_t un32Duration)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	hResult = m_pDataTransfer->UploadOptimizedMovieInfoSet(server,
														   uploadGUID,
														   un32Width,  
														   un32Height, 
														   un32Bitrate,
														   un32Duration);

	return hResult;
}


void CServerDataHandler::shutdown()
{
	PostEvent([this]{EventShutdownHandler();});
}


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::EventShutdownHandler
//
// Description: This is an empty function that is used to receive the Shutdown 
// 				event so that the "EventDoNow()" function does not throw an 
// 				assert. 
//
// Abstract:
//
// Returns:
//	 stiHRESULT_SUCCESS
//
void CServerDataHandler::EventShutdownHandler ()
{
	// Disconnect the dataTransferHandlerSignals.
	dataTransferHandlerSignalsDisconnect();

	m_oCircBuf.CleanupCircularBuffer (estiTRUE);

	// Give up handle to the MP4 Library mutex
	m_muxMP4LibProtect = nullptr;

	// Inform File Play that this data handler has been successfully closed
	movieClosedSignal.Emit(m_pMovie);
}


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::EventFrameGet
//
// Description:
//
// Abstract:
//
//
void CServerDataHandler::EventFrameGet (
	const SFrameGetInfo &frameGetInfo) 
{
	stiLOG_ENTRY_NAME (CServerDataHandler::EventFrameGet);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t un32NewSampleDescIndex = 0;
	uint32_t un32Size = 0;
	Media_t* pMedia = nullptr;
	uint32_t un32FrameIndex = 0;
	SMHandle hFrame = nullptr;
	MP4TimeValue sampleTime = 0;
	MP4TimeValue sampleToFreeTime = 0;
	MP4TimeValue sampleDuration = 0;
	MP4TimeScale mediaTimeScale = 0;
	uint64_t un64SampleOffset = 0;

	pMedia = frameGetInfo.pMedia;
	un32FrameIndex = frameGetInfo.un32FrameIndex;
	hFrame = frameGetInfo.hFrame;

	if (pMedia)
	{
		uint8_t* pun8SampleData;
		uint32_t un32SampleSize;
		uint32_t un32SampleDuration;
		bool_t    bSyncFrame;
		pun8SampleData = nullptr;
		un32SampleSize = 0;
		un32SampleDuration = 0;

		ASSERTRT(hFrame != nullptr, SMRES_ALLOC_FAILED);

		SMResult = MP4GetMediaSampleByIndex (pMedia, un32FrameIndex, hFrame, 
											 &un64SampleOffset, &un32Size, 
											 &bSyncFrame, &un32NewSampleDescIndex,
											 !m_useRangeRequest);
		TESTRESULT ();
		
		SMResult = MP4GetSampleDuration (pMedia, un32FrameIndex, &un32SampleDuration);
	
		TESTRESULT ();

		pun8SampleData = (uint8_t*)*hFrame;
		
		un32SampleSize = SMGetHandleSize (hFrame);
			
#ifdef stiBUFFERING_DEBUG
		g_bStartPlaying = estiTRUE;
#endif
		// Inform FilePlay that the frame is ready
		SMediaSampleInfo mediaSampleInfo;
		
		mediaSampleInfo.un32SampleIndex = un32FrameIndex;
		mediaSampleInfo.pun8SampleData = pun8SampleData;
		mediaSampleInfo.un32SampleSize = un32SampleSize;
		mediaSampleInfo.un32SampleDuration = un32SampleDuration;
		mediaSampleInfo.un32SampleDescIndex = un32NewSampleDescIndex;
		mediaSampleInfo.keyFrame = bSyncFrame ? estiTRUE: estiFALSE;
		
		mediaSampleReadySignal.Emit(mediaSampleInfo);

		if (m_useRangeRequest)
		{
			// Check to see if we need to release any frames.
			if (un32FrameIndex >= m_un32FrameIndexToFree)
			{
				// Calculate the time length of the skipback buffer.
				SMResult = MP4GetMediaTimeScale(pMedia, &mediaTimeScale);
				m_unSkipBackTimeBuf = mediaTimeScale * unSKIPBACK_BUFFER_SECONDS;

				SMResult = MP4SampleIndexToMediaTime(pMedia, un32FrameIndex, 
													 &sampleTime, &sampleDuration);
				SMResult = MP4SampleIndexToMediaTime(pMedia, m_un32FrameIndexToFree,
													 &sampleToFreeTime, &sampleDuration);
				while (sampleTime - sampleToFreeTime > m_unSkipBackTimeBuf)
				{
					un64SampleOffset = 0;
					un32Size = 0;
					SMResult = MP4GetMediaSampleByIndex (pMedia, m_un32FrameIndexToFree, nullptr, 
														 &un64SampleOffset, &un32Size, 
														 nullptr, nullptr, false);
					TESTRESULT ();
					hResult = m_pDataTransfer->ReleaseBuffer(static_cast<uint32_t>(un64SampleOffset), un32Size);
					stiTESTRESULT();
					m_un32FrameIndexToFree++;
					SMResult = MP4SampleIndexToMediaTime(pMedia, m_un32FrameIndexToFree,
														 &sampleToFreeTime, &sampleDuration);
				}
			}
			else
			{
				m_un32FrameIndexToFree = un32FrameIndex;
			}
		}
	}

smbail:

	switch (SMResult)
	{
		case SMRES_SUCCESS:
			// Do nothing ... it was already handled above.
			break;
			
		case SMRES_OFFSET_OVERWRITTEN:
		{
			if (m_useRangeRequest)
			{
				// We need to go get the data that was overwritten send the event to start
				// the process.
				// Don't free the frame info we need it later.
				if (!m_pDataTransfer->IsSampleDownloading(un64SampleOffset, un32Size, estiTRUE))
				{
					PostEvent([this, frameGetInfo]{EventSkipBackDataGet(frameGetInfo);});
				}
			}

			// Inform FilePlay that the frame is not ready
			mediaSampleNotReadySignal.Emit(eSAMPLE_IN_PAST);
		}
			break;
		
		case SMRES_OFFSET_NO_YET_AVAIL:
		{
#ifdef stiBUFFERING_DEBUG
			g_bStartPlaying = estiFALSE;
#endif
			if (m_useRangeRequest)
			{
				// Check to see if we have just caught up to the end of the buffer and
				// just need some download time.
				if (!m_pDataTransfer->IsSampleDownloading(un64SampleOffset, un32Size, estiFALSE))
				{
					// If we are skipping into the future and we don't have the frame yet,
					// we will need to clear space in the buffer before we download it.
					// Don't free the frame info we need it later.
					PostEvent([this, frameGetInfo]{EventSkipForwardDataGet(frameGetInfo);});
				}
			}

			// Inform FilePlay that the frame is not ready
			mediaSampleNotReadySignal.Emit(eSAMPLE_IN_FUTURE);
		}
			break;
		
		case SMRES_READ_ERROR:
		{
			if (m_useRangeRequest)
			{
				uint64_t un64EndSampleOffset;
				uint32_t un32EndFrame;
				MP4GetMediaSampleCount (pMedia, &un32EndFrame);
				un32EndFrame = un32EndFrame > un32FrameIndex + 300 ? un32FrameIndex + 300 : un32EndFrame;
	
				SMResult = MP4GetMediaSampleByIndex (pMedia, un32EndFrame, hFrame, 
													 &un64EndSampleOffset, &un32Size, 
													 nullptr, &un32NewSampleDescIndex);	
	
				m_un32FrameIndexToFree = un32FrameIndex;
				m_pDataTransfer->GetSkipForwardData(0, 0, 
													un64SampleOffset, un64EndSampleOffset - 1);
	
				// Inform FilePlay that the frame is not ready
				mediaSampleNotReadySignal.Emit(eSAMPLE_IN_FUTURE);
				break;
			}
		}
		// FALLTHRU
		// If we arn't using range requests then fall through to the default.
		case SMRES_OFFSET_PAST_EOF:
		default:
			// Force an error to be logged
			stiTESTCOND (estiFALSE, stiRESULT_SRVR_DATA_HNDLR_DOWNLOAD_ERROR);
			break;
	}
	
STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		errorSignal.Emit(hResult);
	}
} // end CServerDataHandler::EventFrameGet


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::EventSkipBackDataGet
//
// Description: Initializes members and starts the process of getting data
// 				that has been overwritten but is need for a skipback request.
//
// Abstract:
//
// Returns: stiHResult
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Failure
//
void CServerDataHandler::EventSkipBackDataGet (
	const SFrameGetInfo &frameGetInfo)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	Media_t* pMedia = nullptr;
	uint32_t un32FrameIndex = 0;

	pMedia = frameGetInfo.pMedia;
	un32FrameIndex = frameGetInfo.un32FrameIndex;

	if (pMedia)
	{
		uint64_t un64SampleOffsetBegin;
		uint64_t un64SampleOffsetEnd;

		// Get the area of the ring buffer that we need to free.
		SMResult = MP4GetMediaSampleByIndex (pMedia, un32FrameIndex, nullptr, 
											 &un64SampleOffsetBegin, nullptr, 
											 nullptr, nullptr, false);
		TESTRESULT();

		SMResult = MP4GetMediaSampleByIndex (pMedia, m_un32FrameIndexToFree, nullptr,
											  &un64SampleOffsetEnd, nullptr,
											  nullptr, nullptr, false);
		TESTRESULT();

		m_un32FrameIndexToFree = un32FrameIndex;

		m_pDataTransfer->GetSkipBackData(un64SampleOffsetBegin, un64SampleOffsetEnd);
	}
	
smbail:

	return;
}

////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::EventSkipForwardDataGet
//
// Description: Initializes members and starts the process of getting data
// 				that is in the future.
//
// Abstract:
//
// Returns: stiHResult
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Failure
//
void CServerDataHandler::EventSkipForwardDataGet (
	const SFrameGetInfo &frameGetInfo)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	Media_t* pMedia = nullptr;
	uint32_t un32FrameIndex = 0;

	pMedia = frameGetInfo.pMedia;
	un32FrameIndex = frameGetInfo.un32FrameIndex;

	if (pMedia)
	{
		MP4TimeValue sampleTime = 0;
		MP4TimeValue sampleDuration = 0;
		MP4TimeScale mediaTimeScale = 0;

		// We will probably need to release some data so find the offset of the current
		// FrameIndexToFree.

		uint64_t un64ReleaseBufferStartOffset = 0; 
		SMResult = MP4GetMediaSampleByIndex (pMedia, m_un32FrameIndexToFree, nullptr, 
											 &un64ReleaseBufferStartOffset, nullptr, 
											 nullptr, nullptr, false);

		// We want a 20 second window that the desired frame is sitting in the middle of.
		// Get the time scale and then find the frames that are on either end of the window.
		// Skipback buffer is the left side of the window (10 seconds).
		SMResult = MP4GetMediaTimeScale(pMedia, &mediaTimeScale);

		SMResult = MP4SampleIndexToMediaTime(pMedia, un32FrameIndex, 
											 &sampleTime, &sampleDuration);
		uint32_t un32LeftFrameIndex = 1;
		if (sampleTime > 0)
		{
			SMResult = MP4MediaTimeToSampleIndex(pMedia, 
												 sampleTime, 
												 &un32LeftFrameIndex);
		}

		// The leftFrameOffset will be the new next frame to free.
		m_un32FrameIndexToFree = un32LeftFrameIndex;

		// Now see if any of the left side of the window is already in the buffer. 
		uint64_t un64LeftFrameOffset = 0;
		uint64_t un64ReleaseBufferEndOffset = 0;
		uint32_t un32FrameSize = 0;
		SMResult = MP4GetMediaSampleByIndex (pMedia, un32LeftFrameIndex, nullptr, 
											 &un64LeftFrameOffset, &un32FrameSize, 
											 nullptr, nullptr, false);
		
		if (m_oCircBuf.IsDataInBuffer (static_cast<uint32_t>(un64LeftFrameOffset), un32FrameSize))
		{
			// We actually want the end of the frame prior to the un32LeftFrameIndex.
			un64ReleaseBufferEndOffset = (un64LeftFrameOffset == un64ReleaseBufferStartOffset) ? un64LeftFrameOffset : un64LeftFrameOffset - 1;
		}
		else
		{
			// The left side is not in the buffer so we can just clear the buffer and download 
			// the whole chunk of the file.
			un64ReleaseBufferStartOffset = 0;
			un64ReleaseBufferEndOffset = 0;
		}

		// Get the total number of frames in the file.  Then add the number of frames from the 
		// left side to the right side.  If that is more then the total number of frames then 
		// addjust the number back to the total number of frames.
		uint32_t un32TotalFrames = 0; 
		MP4GetMediaSampleCount (pMedia, &un32TotalFrames);
		uint32_t un32RightFrameIndex = 0;

		un32RightFrameIndex = un32FrameIndex + (un32FrameIndex - un32LeftFrameIndex);
		un32RightFrameIndex = (un32RightFrameIndex <  un32TotalFrames) ? un32RightFrameIndex - 1 : un32TotalFrames;


		// Now find the end of the point of the chunk we want to download.
		uint64_t un64RightFrameOffset = 0;
		SMResult = MP4GetMediaSampleByIndex (pMedia, un32RightFrameIndex, nullptr, 
											 &un64RightFrameOffset, &un32FrameSize, 
											 nullptr, nullptr, false);
		TESTRESULT ();
		
		if (un32RightFrameIndex == un32TotalFrames)
		{
			un64RightFrameOffset += un32FrameSize;
		}

		// Tell the DataTransferHandler to get the skip forward data.
		m_pDataTransfer->GetSkipForwardData(un64ReleaseBufferStartOffset, un64ReleaseBufferEndOffset, 
											un64LeftFrameOffset, un64RightFrameOffset - 1);
	}

smbail:

	return;
}

////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::MovieFileInfoSet
//
// Description: Set server and file information
//
// Abstract:
//
// Returns: stiHResult
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Failure
//
stiHResult CServerDataHandler::MovieFileInfoSet (
	const std::string &server,
	const std::string &fileGUID,
	uint32_t un32DownloadSpeed,
	uint64_t un64FileSize,
	const std::string &clientToken)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	hResult = m_pDataTransfer->MovieFileInfoSet(server,
												fileGUID,
												un32DownloadSpeed,
												un64FileSize,
												clientToken);
		
	return hResult;
} // end CServerDataHandler::MovieFileInfoSet


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::FileChunkGet
//
// Description: Get a video message from the media server.
//
// Abstract:
//
// Returns: stiHResult
//
//
stiHResult CServerDataHandler::FileChunkGet (
	uint64_t beginByte, 
	uint64_t endByte)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::FileChunkGet);
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	stiHResult hResult = stiRESULT_SUCCESS;

	// If FileChunkGet is called we are using range requests.
	m_useRangeRequest = true;

	hResult = m_pDataTransfer->FileChunkGet(m_pMovie,
											beginByte,
											endByte);

	return (hResult);
} // end CServerDataHandler::FileChunkGet


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::FrameGet
//
// Description: Get a video frame from the circular buffer.
//
// Abstract:
//
// Returns: stiHResult
//
//
stiHResult CServerDataHandler::FrameGet (
	Media_t* pMedia,
	uint32_t un32FrameIndex,
	SMHandle hFrame)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::FrameGet);
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	stiHResult hResult = stiRESULT_SUCCESS;

	SFrameGetInfo frameGetInfo;
  
	frameGetInfo.pMedia = pMedia;
	frameGetInfo.un32FrameIndex = un32FrameIndex;
	frameGetInfo.hFrame = hFrame;
	  
	// Did we get a valid pointer?
	PostEvent([this, frameGetInfo]{EventFrameGet(frameGetInfo);});
	
	return (hResult);
} // end CServerDataHandler::FrameGet


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::IsDataInBuffer
//
// Description: 
//
// Abstract:
//
// Returns: EstiBool
//
//
bool CServerDataHandler::IsDataInBuffer (
	uint32_t un32Offset, 
	uint32_t un32NumberOfBytes)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::IsDataInBuffer);
	
	bool bRet;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	bRet = m_oCircBuf.IsDataInBuffer (un32Offset, un32NumberOfBytes);
	
	return (bRet);
} // end CServerDataHandler::IsDataInBuffer


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::MaxBufferingSizeGet
//
// Description: Get the maximum size for buffering
//
// Abstract:
//
// Returns: 
//  uint32_t - maximum size for buffering
//
//
uint32_t CServerDataHandler::MaxBufferingSizeGet ()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::MaxBufferingSizeGet);

	uint32_t un32stMaxBuffSize;
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	un32stMaxBuffSize = CVMCircularBuffer::MaxBufferingSizeGet ();
	
	return (un32stMaxBuffSize);
} // end CServerDataHandler::MaxBufferingSizeGet


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::ParseAtom
//
// Description: This function is called to parse the various atoms when successfully
// 		parsed the file player is notified that Movie is ready to play.
//
// Abstract:
//
// Returns: EstiBool
//
//
bool CServerDataHandler::ParseAtoms (
	uint32_t un32Bytes)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::MoreDataAvailable);
	
	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CServerDataHandler::MoreDataAvailable> un32Bytes = %d\n", un32Bytes);
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	SMResult_t SMResult = SMRES_SUCCESS;
	uint64_t un64DataParsed = 0;
	uint32_t un32AtomType = 0;
	uint32_t un32AtomHeaderSize = 0;

	if (un32Bytes > 0 &&
		m_pMovie)
	{
		// Parse Atom
		for (;;)
		{
			if (m_mdatAtom)
			{
				// If we already have the Mdat atom, we don't want to parse any more since that will cause errors
				// with GetBuffer.  MP4ParseAtom ends up calling DHRead which calls GetBuffer for the offset of
				// the EOF marker with a size of 4 bytes which puts us past EOF.
				
				// This assumes that the MDat atom will always come after we have the other necessary atoms.  It also
				// assumes that we will only get a single MDat atom.
				break;
			}	
			
			SMResult = MP4ParseAtom (m_pMovie, m_un64ParseOffset, &un64DataParsed, &un32AtomType, &un32AtomHeaderSize);
			if (0 == un64DataParsed ||
				SMResult != SMRES_SUCCESS)
			{
				break;	
			}
			
			switch (un32AtomType)
			{
			case eAtom4CCmoov:
				stiDEBUG_TOOL (g_stiMP4FileDebug,
					stiTrace("CServerDataHandler::ParsAtom - Get eAtom4CCmoov atom\n");
				);

				stiTESTCOND(m_moovAtom == false, stiRESULT_INVALID_MEDIA);
				m_moovAtom = true;

				m_un64MetaDataLength += un64DataParsed;

				// Release Buffer
				m_pDataTransfer->ReleaseBuffer (static_cast<uint32_t>(m_un64ParseOffset), static_cast<uint32_t>(un64DataParsed));

				break;
				
			case eAtom4CCftyp:
				stiDEBUG_TOOL (g_stiMP4FileDebug,
					stiTrace("CServerDataHandler::ParsAtom - Get eAtom4CCftyp atom\n");
				);

			   	stiTESTCOND(m_ftypAtom == false, stiRESULT_INVALID_MEDIA);
				m_ftypAtom = true;

				m_un64MetaDataLength += un64DataParsed;

				// Release Buffer
				m_pDataTransfer->ReleaseBuffer (static_cast<uint32_t>(m_un64ParseOffset), static_cast<uint32_t>(un64DataParsed));

				break;
				
			case eAtom4CCfree:

				stiDEBUG_TOOL (g_stiMP4FileDebug,
					stiTrace("CServerDataHandler::ParsAtom - Get eAtom4CCfree atom\n");
				);

				m_un64MetaDataLength += un64DataParsed;

				// Release Buffer
				m_pDataTransfer->ReleaseBuffer (static_cast<uint32_t>(m_un64ParseOffset), static_cast<uint32_t>(un64DataParsed));

				break;
				
			case eAtom4CCskip:

				stiDEBUG_TOOL (g_stiMP4FileDebug,
					stiTrace("CServerDataHandler::ParsAtom - Get eAtom4CCskip atom\n");
				);

				m_un64MetaDataLength += un64DataParsed;

				// Release Buffer
				m_pDataTransfer->ReleaseBuffer (static_cast<uint32_t>(m_un64ParseOffset), static_cast<uint32_t>(un64DataParsed));

				break;

			case eAtom4CCwide:

				stiDEBUG_TOOL (g_stiMP4FileDebug,
					stiTrace("CServerDataHandler::ParsAtom - Get eAtom4CCwide atom\n");
				);

				m_un64MetaDataLength += un64DataParsed;

				// Release Buffer
				m_pDataTransfer->ReleaseBuffer (static_cast<uint32_t>(m_un64ParseOffset), static_cast<uint32_t>(un64DataParsed));

				break;

			case eAtom4CCmdat:
				stiDEBUG_TOOL (g_stiMP4FileDebug,
					stiTrace("CServerDataHandler::ParsAtom - Get eAtom4CCmdat atom\n");
				);

				stiTESTCOND(m_mdatAtom == false, stiRESULT_INVALID_MEDIA);
				m_mdatAtom = true;
				m_un64MetaDataLength += un32AtomHeaderSize;

				// Release Buffer
				m_pDataTransfer->ReleaseBuffer (static_cast<uint32_t>(m_un64ParseOffset), un32AtomHeaderSize);

				break;

			default:
				break;
			}
			
			m_un64ParseOffset += un64DataParsed;
		}
		
		// Start updating the progress as soon as we have eAtom4CCftyp and eAtom4CCmoov atom
		if (m_moovAtom && m_ftypAtom &&
			SMResult == SMRES_SUCCESS)
		{
			if (!m_readyToPlay)
			{
				m_readyToPlay = true;

				// We have got the moov atom. Movie is ready to play
				movieReadySignal.Emit();
			}
		}
	}

STI_BAIL:

	// We don't want to send an error on a READ_ERROR becaue we may have a read error
	// because the entire atom has not been downloaded to the buffer.
	if (stiIS_ERROR(hResult) ||
		(SMResult != SMRES_SUCCESS &&
		SMResult != SMRES_READ_ERROR &&
		SMResult != SMRES_OFFSET_NO_YET_AVAIL))
	{
		errorSignal.Emit(hResult);

		if (m_pDataTransfer)
		{
			m_pDataTransfer->NotifyParseError();
		}

		stiASSERTMSG(estiFALSE, "Failed to parse atom ERROR: %d\n", SMResult);
	} // end if
	
	return m_readyToPlay;
} // end CServerDataHandler::MoreDataAvailable


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::ResetExpiredGUID
//
// Abstract: Resets an expired GUID and restarts the download.
//
// Returns: 
//
stiHResult CServerDataHandler::ResetExpiredDownloadGUID(
		const std::string &fileGUID)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (!fileGUID.empty(), stiRESULT_ERROR);

	hResult = m_pDataTransfer->ResetExpiredDownloadGUID(fileGUID);
	stiTESTRESULT ();

	hResult = m_pDataTransfer->FileChunkGet();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::CloseDataHandler
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::CloseDataHandler()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::CloseDataHandler);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CServerDataHandler::CloseDataHandler>\n");
	);

	SMResult_t SMResult = SMRES_SUCCESS;

	m_pDataTransfer->shutdown ();
	
	return SMResult;
} // end CServerDataHandler::CloseDataHandler


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHAdjustOffsets
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHAdjustOffsets(
	uint32_t un32AdjustOffset)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHAdjustOffsets);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = m_oCircBuf.AdjustOffsets(un32AdjustOffset);

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHCreatePrefixBuffer
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHCreatePrefixBuffer(
	uint32_t un32EndingOffset,
	uint32_t un32PrefixBufSize)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHCreatePrefixBuffer);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = m_oCircBuf.CreatePrefixBuffer(un32EndingOffset, 
											 un32PrefixBufSize);

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHRead
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHRead(
	uint64_t un64Offset,
	uint32_t unLength,
	void *pBuffer,
	bool bReleaseBuffer) // Default is false
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHRead);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	SMResult_t SMResult = SMRES_SUCCESS;
	uint8_t *pReadBuffer;
	CVMCircularBuffer::EstiBufferResult eResult = CVMCircularBuffer::estiBUFFER_OK;
	eResult = m_oCircBuf.GetBuffer(static_cast<uint32_t>(un64Offset), unLength, &pReadBuffer);
	
	switch (eResult)
	{
		case CVMCircularBuffer::estiBUFFER_OK:
			SMResult = SMRES_SUCCESS;
	
			memcpy (pBuffer, pReadBuffer, unLength);
			
			if (bReleaseBuffer)
			{
				m_pDataTransfer->ReleaseBuffer (static_cast<uint32_t>(un64Offset), unLength);
			}
			break;
			
		case CVMCircularBuffer::estiBUFFER_OFFSET_OVERWRITTEN:
			SMResult = SMRES_OFFSET_OVERWRITTEN;
			break;
			
		case CVMCircularBuffer::estiBUFFER_OFFSET_NOT_YET_AVAILABLE:
			SMResult = SMRES_OFFSET_NO_YET_AVAIL;
			break;
			
		case CVMCircularBuffer::estiBUFFER_OFFSET_PAST_EOF:
			SMResult = SMRES_OFFSET_PAST_EOF;
			break;
			
		case CVMCircularBuffer::estiBUFFER_ERROR:
		default:
			SMResult = SMRES_READ_ERROR;
			break;
			
	}
	
	return SMResult;
} // end CServerDataHandler::DHRead


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHGetFileSize
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHGetFileSize(
	uint64_t *pun64Size) const
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHGetFileSize);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = m_pDataTransfer->DHGetFileSize(pun64Size);

	return SMResult;
} // end CServerDataHandler::DHGetFileSize


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::Startup
//
// Abstract:
//
// Returns:
//
stiHResult CServerDataHandler::Startup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool started = false;

	started  = CstiEventQueue::StartEventLoop();
	stiTESTCOND (started, stiRESULT_ERROR);

	hResult = m_pDataTransfer->Startup();
	stiTESTRESULT ();

STI_BAIL:
	return hResult;
}
 

///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::OpenDataHandlerForWrite
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::OpenDataHandlerForWrite()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::OpenDataHandlerForWrite);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	SMResult_t SMResult = SMRES_UNSUPPORTED_OBJECT_COMMAND;

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CServerDataHandler::OpenDataHandlerForWrite> UNSUPPORTED COMMAND!\n");
	);

	return SMResult;
} // end CServerDataHandler::OpenDataHandlerForWrite


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler:::OpenDataHandlerForRead
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::OpenDataHandlerForRead()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::OpenDataHandlerForRead);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	SMResult_t SMResult = SMRES_UNSUPPORTED_OBJECT_COMMAND;

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CServerDataHandler::OpenDataHandlerForRead> UNSUPPORTED COMMAND!\n");
	);

	return SMResult;
} // end CServerDataHandler::OpenDataHandlerForRead


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHWrite
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHWrite(
	const void *pData,
	uint32_t unSize)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHWrite);
	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;
	uint8_t *pWriteBuffer;
	uint32_t un32Bytes;
	uint8_t *pDataPointer;
	uint32_t un32DataLength;
	EstiBool bDataCopied = estiFALSE;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Check the buffer space to make sure we have enough space to write the
	// data.
	if (!m_oCircBuf.IsBufferSpaceAvailable(unSize))
	{
		stiTHROW (stiRESULT_BUFFER_TOO_SMALL);
	}

	pDataPointer = (uint8_t*)pData;
	un32DataLength = unSize;

	while (!bDataCopied)
	{
		// Get the buffer to write to.
		un32Bytes = m_oCircBuf.GetCurrentWritePosition (&pWriteBuffer);
		if (un32Bytes >= un32DataLength)
		{
			memcpy(pWriteBuffer, pDataPointer, un32DataLength);
			hResult = m_oCircBuf.InsertIntoCircularBufferLinkedList (pWriteBuffer, un32DataLength);
			stiTESTRESULT();
			bDataCopied = estiTRUE;
		}
		else
		{
			memcpy(pWriteBuffer, pDataPointer, un32Bytes);
			pDataPointer += un32Bytes;
			un32DataLength -= un32Bytes;
			hResult = m_oCircBuf.InsertIntoCircularBufferLinkedList (pWriteBuffer, un32Bytes);
			stiTESTRESULT();
		}
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		SMResult = SMRES_WRITE_ERROR;
	}
	
	return SMResult;
} // CServerDataHandler::DHWrite


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHWrite2
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHWrite2(
	uint16_t usData)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHWrite2);
	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;
	uint16_t un16LittleEndianData;
	uint8_t *pWriteBuffer;
	uint32_t un32Bytes;
	uint8_t *pDataPointer;
	uint32_t un32DataLength;
	EstiBool bDataCopied = estiFALSE;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Check the buffer space to make sure we have enough space to write the
	// data.
	if (!m_oCircBuf.IsBufferSpaceAvailable(sizeof(usData)))
	{
		stiTHROW (stiRESULT_BUFFER_TOO_SMALL);
	}

	// Swap the byte order for little endian.
	un16LittleEndianData = BYTESWAP(usData);
	pDataPointer = (uint8_t *)&un16LittleEndianData;
	un32DataLength = sizeof(un16LittleEndianData);

	while (!bDataCopied)
	{
		// Get the buffer to write to.
		un32Bytes = m_oCircBuf.GetCurrentWritePosition (&pWriteBuffer);
		if (un32Bytes >= un32DataLength)
		{
			memcpy(pWriteBuffer, pDataPointer, un32DataLength);
			hResult = m_oCircBuf.InsertIntoCircularBufferLinkedList (pWriteBuffer, un32DataLength);
			stiTESTRESULT();
			bDataCopied = estiTRUE;
		}
		else
		{
			memcpy(pWriteBuffer, pDataPointer, un32Bytes);
			pDataPointer += un32Bytes;
			un32DataLength -= un32Bytes;
			hResult = m_oCircBuf.InsertIntoCircularBufferLinkedList (pWriteBuffer, un32Bytes);
			stiTESTRESULT();
		}
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		SMResult = SMRES_WRITE_ERROR;
	}

	return SMResult;
} // end CServerDataHandler::DHWrite2


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHWrite4
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHWrite4(
	uint32_t unData)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHWrite4);
	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32LittleEndianData;
	uint8_t *pWriteBuffer;
	uint32_t un32Bytes;
	uint8_t *pDataPointer;
	uint32_t un32DataLength;
	EstiBool bDataCopied = estiFALSE;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Check the buffer space to make sure we have enough space to write the
	// data.
	if (!m_oCircBuf.IsBufferSpaceAvailable(sizeof(uint32_t)))
	{
		stiTHROW (stiRESULT_BUFFER_TOO_SMALL);
	}

	// Swap the byte order for little endian.
	un32LittleEndianData = DWORDBYTESWAP(unData);
	pDataPointer = (uint8_t *)&un32LittleEndianData;
	un32DataLength = sizeof(un32LittleEndianData);

	while (!bDataCopied)
	{
		// Get the buffer to write to.
		un32Bytes = m_oCircBuf.GetCurrentWritePosition (&pWriteBuffer);
		if (un32Bytes >= un32DataLength)
		{
			memcpy(pWriteBuffer, pDataPointer, un32DataLength);
			bDataCopied = estiTRUE;
			hResult = m_oCircBuf.InsertIntoCircularBufferLinkedList (pWriteBuffer, un32DataLength);
			stiTESTRESULT();
		}
		else
		{
			memcpy(pWriteBuffer, pDataPointer, un32Bytes);
			pDataPointer += un32Bytes;
			un32DataLength -= un32Bytes;
			hResult = m_oCircBuf.InsertIntoCircularBufferLinkedList (pWriteBuffer, un32Bytes);
			stiTESTRESULT();
		}
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		SMResult = SMRES_WRITE_ERROR;
	}

	return SMResult;
} // end CServerDataHandler::DHWrite4


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHWrite8
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHWrite8(
	uint64_t un64Data)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHWrite8);
	SMResult_t SMResult = SMRES_SUCCESS;
	stiHResult hResult = stiRESULT_SUCCESS;
	uint64_t un64LittleEndianData;
	uint8_t *pWriteBuffer;
	uint32_t un32Bytes;
	uint8_t *pDataPointer;
	uint32_t un32DataLength;
	EstiBool bDataCopied = estiFALSE;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// Check the buffer space to make sure we have enough space to write the
	// data.
	if (!m_oCircBuf.IsBufferSpaceAvailable(sizeof(un64Data)))
	{
		stiTHROW (stiRESULT_BUFFER_TOO_SMALL);
	}

	// Swap the byte order for little endian.
	un64LittleEndianData = QWORDBYTESWAP(un64Data);
	pDataPointer = (uint8_t *)&un64LittleEndianData;
	un32DataLength = sizeof(un64LittleEndianData);

	while (!bDataCopied)
	{
		// Get the buffer to write to.
		un32Bytes = m_oCircBuf.GetCurrentWritePosition (&pWriteBuffer);
		if (un32Bytes >= un32DataLength)
		{
			memcpy(pWriteBuffer, pDataPointer, un32DataLength);
			bDataCopied = estiTRUE;
			hResult = m_oCircBuf.InsertIntoCircularBufferLinkedList (pWriteBuffer, un32DataLength);
			stiTESTRESULT();
		}
		else
		{
			memcpy(pWriteBuffer, pDataPointer, un32Bytes);
			pDataPointer += un32Bytes;
			un32DataLength -= un32Bytes;
			hResult = m_oCircBuf.InsertIntoCircularBufferLinkedList (pWriteBuffer, un32Bytes);
			stiTESTRESULT();
		}
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		SMResult = SMRES_WRITE_ERROR;
	}

	return SMResult;
} // end CServerDataHandler::DHWrite8


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHGetPosition
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHGetPosition(
	uint64_t *pun64Position) const
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHGetPosition);
	SMResult_t SMResult = SMRES_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	*pun64Position = m_oCircBuf.GetCurrentWriteOffest();

	stiDEBUG_TOOL (g_stiMP4FileDebug,
		stiTrace("<CServerDataHandler::DHGetPosition>\n");
	);
	
	return SMResult;
} // CServerDataHandler::DHGetPosition


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DHSetPosition
//
// Abstract:
//
// Returns:
//
SMResult_t CServerDataHandler::DHSetPosition(
	uint64_t un64Position)
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DHSetPosition);

	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	// We are always writing to the end of the circular buffer so just return
	// SMRES_SUCCESS.
	SMResult_t SMResult = SMRES_SUCCESS;

	return SMResult;
} // CServerDataHandler::DHSetPosition


///////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DownloadSpeedSet
//
// Abstract: Sets a download speed and restarts the download.
//
// Returns: 
//
stiHResult CServerDataHandler::DownloadSpeedSet(
	uint32_t un32DownloadSpeed)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pDataTransfer->DownloadSpeedSet(un32DownloadSpeed);

	return hResult;
}


void CServerDataHandler::dataTransferHandlerSignalsConnect ()
{
	if (m_dataTransferHandlerSignalConnections.empty())
	{
		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->parsableMoovCheckSignal.Connect (
				[this](uint32_t dataForParsableCheck){
					bool parsable = ParseAtoms(dataForParsableCheck);

					m_pDataTransfer->ReadyToPlaySet(parsable);
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->messageSizeSignal.Connect (
				[this](uint64_t messageSize){
					messageSizeSignal.Emit(messageSize);
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->messageIdSignal.Connect (
				[this](std::string messageId){
					messageIdSignal.Emit(messageId);
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->fileDownloadProgressSignal.Connect (
				[this](const SFileDownloadProgress &downloadProgress){
					fileDownloadProgressSignal.Emit(downloadProgress);
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->skipDownloadProgressSignal.Connect (
				[this](const SFileDownloadProgress &downloadProgress){
					skipDownloadProgressSignal.Emit(downloadProgress);
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->bufferUploadSpaceUpdateSignal.Connect (
				[this](const SFileUploadProgress &uploadSpace){
					bufferUploadSpaceUpdateSignal.Emit(uploadSpace);
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->uploadProgressSignal.Connect (
				[this](uint32_t uploadProgress){
					uploadProgressSignal.Emit(uploadProgress);
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->errorSignal.Connect (
				[this](stiHResult hResult){
					errorSignal.Emit(hResult);
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->uploadErrorSignal.Connect (
				[this](){
					uploadErrorSignal.Emit();
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->downloadErrorSignal.Connect (
				[this](){
					downloadErrorSignal.Emit();
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->uploadCompleteSignal.Connect (
				[this](){
					uploadCompleteSignal.Emit();
				}));

		m_dataTransferHandlerSignalConnections.push_back(
			m_pDataTransfer->shutdownCompleteSignal.Connect (
				[this](){
					PostEvent([this]{EventShutdownHandler();});
				}));
	}
}

void CServerDataHandler::dataTransferHandlerSignalsDisconnect ()
{
	m_dataTransferHandlerSignalConnections.clear();
}


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::ReadyForDownloadUpdate
//
// Description: Notifies the ServerDataHandler that the FilePlayer is ready for 
//              another download update.
//
// Abstract:
//
// Returns: EstiResult
//
//
stiHResult CServerDataHandler::ReadyForDownloadUpdate()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::ReadyForDownloadUpdate);
	
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this]{EventReadyForDownloadUpdate();});
											
	return (hResult);
}


void CServerDataHandler::EventReadyForDownloadUpdate ()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::EventReadyForDownloadUpdate);

    m_pDataTransfer->ResetSendDownloadProgress();
}


////////////////////////////////////////////////////////////////////////////////
//; CServerDataHandler::DownloadResume
//
// Description: Restarts a download that has stopped. 
//              
//
// Abstract:
//
// Returns: EstiResult
//
//
stiHResult CServerDataHandler::downloadResume ()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::DownloadResume);
	
	stiHResult hResult = stiRESULT_SUCCESS;

	PostEvent([this]{eventDownloadResume();});
	
	return (hResult);
}

void CServerDataHandler::eventDownloadResume ()
{
	stiLOG_ENTRY_NAME (CServerDataHandler::EventDownloadResume);

	m_pDataTransfer->downloadResume(m_pMovie);
}

void CServerDataHandler::logInfoToSplunk ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	stiTESTCOND (m_pDataTransfer, stiRESULT_ERROR);

	m_pDataTransfer->logInfoToSplunk ();

STI_BAIL:

	return;
}
