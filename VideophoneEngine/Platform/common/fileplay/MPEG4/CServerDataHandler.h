////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CServerDataHandler
//
//  File Name:	CServerDataHandler.h
//
//  Owner:
//
//	Abstract: See CServerDataHandler.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSERVERDATAHANDLER_H
#define CSERVERDATAHANDLER_H

//
// Includes
//
#include "stiSVX.h"
#include <csignal>

//
// Sorenson includes
//
#include "SorensonErrors.h"
#include "SMMemory.h"
#include "SMCommon.h"
#include "SMTypes.h"

#include "CDataHandler.h"
#include "CVMCircularBuffer.h"
#include "CDataTransferHandler.h"
#include "MP4Signals.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

class CDataTransferHandler;

//
// Globals
//

class CServerDataHandler : public CDataHandler, public CstiEventQueue 
{
public:

	enum
	{
		eSAMPLE_IN_FUTURE = 0,
		eSAMPLE_IN_PAST = 1
	};
	
	CServerDataHandler (
		Movie_t *pMovie);

	CServerDataHandler (const CServerDataHandler &other) = delete;
	CServerDataHandler (CServerDataHandler &&other) = delete;
	CServerDataHandler &operator= (const CServerDataHandler &other) = delete;
	CServerDataHandler &operator= (CServerDataHandler &&other) = delete;

	~CServerDataHandler () override;

	stiHResult Initialize (stiMUTEX_ID mIdMP4LibProtect);
	
	void shutdown();

	// Overridden base class (CDataHandler) functions
	SMResult_t OpenDataHandlerForWrite () override;

	SMResult_t OpenDataHandlerForRead () override;

	stiHResult Startup ();

	SMResult_t CloseDataHandler() override;

	SMResult_t DHAdjustOffsets(
		uint32_t un32AdjustOffset) override;

	SMResult_t DHCreatePrefixBuffer(
		uint32_t un32EndingOffset,
		uint32_t un32PrefixBufSize) override;

	SMResult_t DHWrite(
		const void *pData,
		uint32_t unSize) override;

	SMResult_t DHWrite2(
		uint16_t usData) override;

	SMResult_t DHWrite4(
		uint32_t unData) override;

	SMResult_t DHWrite8(
		uint64_t un64Data) override;

	SMResult_t DHRead(
		uint64_t un64Offset,
		uint32_t unLength,
		void *pBuffer,
		bool bReleaseBuffer = false) override;

	SMResult_t DHGetPosition(
		uint64_t *pun64Position) const override;

	SMResult_t DHSetPosition(
		uint64_t un64Position) override;

	SMResult_t DHGetFileSize(
		uint64_t *pun64Size) const override;

	// Functions
	stiHResult MovieFileInfoSet (
		const std::string &server, 
		const std::string &fileGUID,
		uint32_t un32DownloadSpeed,
		uint64_t un64Filesize,
		const std::string &clientToken);

	stiHResult FileChunkGet (
		uint64_t beginByte = 0, 
		uint64_t endByte = 0);
	
	stiHResult FrameGet (
		Media_t* pMedia, 
		uint32_t un32FrameIndex, 
		SMHandle hHandle);

	bool IsDataInBuffer (
		uint32_t un32Offset, 
		uint32_t un32NumberOfBytes);

	uint32_t MaxBufferingSizeGet ();
	stiHResult UploadMovieInfoSet(
		const std::string &server,
		const std::string &uploadGUID,
		const std::string &uploadFileName);

	stiHResult UploadOptimizedMovieInfoSet(
		const std::string &server,
		const std::string &uploadGUID,
		uint32_t un32Width,
		uint32_t un32Height,
		uint32_t un32Bitrate,
		uint32_t un32Duration);

	bool ParseAtoms (
		uint32_t un32Bytes);
	
	stiHResult ResetExpiredDownloadGUID(
		const std::string &fileGUID);

	stiHResult DownloadSpeedSet(
		uint32_t un32DownloadSpeed);

	stiHResult ReadyForDownloadUpdate();

	stiHResult downloadResume();

	void logInfoToSplunk ();

private:

	typedef struct SFrameGetInfo
	{
		Media_t* pMedia{nullptr};
		uint32_t un32FrameIndex{0};
		uint32_t lastAudioIndex{0};
		SMHandle hFrame{};
	} SFrameGetInfo;

	unsigned int m_unOpenCount;

	CVMCircularBuffer m_oCircBuf;
	CDataTransferHandler *m_pDataTransfer{nullptr};

	uint32_t m_un32FrameIndexToFree;	// The frame that will be released if it falls outside of the skipback buffer.
	MP4TimeValue m_unSkipBackTimeBuf;   // Time scale of the current movie.
	uint64_t m_un64DownloadDataTarget;
	uint64_t m_un64ParseOffset;		// The offset relative to the beginning of the file
	uint64_t m_un64MetaDataLength;		// The length of the parsed meta data (in bytes)

	bool m_moovAtom {false};
	bool m_ftypAtom {false};
	bool m_mdatAtom {false};
	bool m_readyToPlay {false};

	bool m_waitingForRelease {false};
	bool m_useRangeRequest {false};

	Movie_t *m_pMovie;

	mutable stiMUTEX_ID m_muxMP4LibProtect{};
	
	//
	// Supporting functions
	//
	stiHResult Lock() const;
	void Unlock () const;
	
	stiHResult ReleaseBuffer (
		uint32_t un32Offset, 
		uint32_t un32NumberOfBytes);

	//
	// Event Handlers
	//
	void EventSkipBackDataGet (const SFrameGetInfo &frameGetInfo);
	void EventSkipForwardDataGet (const SFrameGetInfo &frameGetInfo);
	void EventFrameGet (const SFrameGetInfo &frameGetInfo);
	void EventShutdownHandler ();
	void EventReadyForDownloadUpdate ();
	void eventDownloadResume ();

	void dataTransferHandlerSignalsConnect ();
	void dataTransferHandlerSignalsDisconnect ();

	CstiSignalConnection::Collection m_dataTransferHandlerSignalConnections;

public:
	CstiSignal<const SMediaSampleInfo &> mediaSampleReadySignal;
	CstiSignal<int> mediaSampleNotReadySignal;
	CstiSignal<stiHResult> errorSignal;
	CstiSignal<> movieReadySignal;
	CstiSignal<uint64_t> messageSizeSignal;
	CstiSignal<std::string> messageIdSignal;
	CstiSignal<const SFileDownloadProgress &> fileDownloadProgressSignal;
	CstiSignal<const SFileDownloadProgress &> skipDownloadProgressSignal;
	CstiSignal<const SFileUploadProgress> bufferUploadSpaceUpdateSignal;
	CstiSignal<uint32_t> uploadProgressSignal;
	CstiSignal<> uploadErrorSignal;
	CstiSignal<> downloadErrorSignal;
	CstiSignal<> uploadCompleteSignal;
	CstiSignal<Movie_t *> movieClosedSignal;
};

#endif // CSERVERDATAHANDLER_H
