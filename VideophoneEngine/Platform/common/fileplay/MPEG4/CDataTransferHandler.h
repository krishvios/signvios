////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CDataTransferHandler
//
//  File Name:	CDataTransferHandler.h
//
//  Owner:
//
//	Abstract: See CServerDataHandler.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CDATATRANSFERHANDLER_H
#define CDATATRANSFERHANDLER_H

//
// Includes
//
#include "stiSVX.h"
#ifndef WIN32
#include <csignal>
#else
#include "stiOSWin32Signal.h"
#endif
#include <sstream>

//
// Sorenson includes
//
#include "SorensonErrors.h"
#include "SMMemory.h"
#include "SMCommon.h"
#include "SMTypes.h"
#include "CstiEventQueue.h"

#include "CDataHandler.h"
#include "CVMCircularBuffer.h"
#include "CServerDataHandler.h"
#include "MP4Signals.h"
#include "stiHTTPServiceTools.h"

//POCO
#include <Poco/URI.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/SSLManager.h>

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
class CServerDataHandler;

//
// Globals
//

class CDataTransferHandler : public CstiEventQueue 
{
public:

	enum EState
	{
		estiDOWNLOADING = 1,
		estiREADFILE, 
		estiUPLOADING,
		estiHOLDING,
		estiUPLOAD_COMPLETE,
		estiUPLOAD_ERROR,
		estiGUID_ERROR,
		estiDOWNLOAD_ERROR,
		estiGUID_EXPIRED,
	};

	enum ETransferType
	{
		ePAUSED,
		eDOWNLOAD_NORMAL,
		eDOWNLOAD_SKIP_BACK,
		eDOWNLOAD_SKIP_FORWARD,
		eUPLOAD_CONTINUOUS,
		eUPLOAD_FILE,
	};

	CDataTransferHandler ();
		
	stiHResult Initialize (
		CVMCircularBuffer *pCircBuf);
	
	CDataTransferHandler (const CDataTransferHandler &other) = delete;
	CDataTransferHandler (CDataTransferHandler &&other) = delete;
	CDataTransferHandler &operator= (const CDataTransferHandler &other) = delete;
	CDataTransferHandler &operator= (CDataTransferHandler &&other) = delete;

	~CDataTransferHandler () override;

	void shutdown();

	stiHResult Startup ();

	// Overridden base class (CDataHandler) functions
	virtual SMResult_t DHGetFileSize(
		uint64_t *pun64Size) const;

	// Functions
	stiHResult MovieFileInfoSet (
		const std::string &server, 
		const std::string &fileGUID,
		uint32_t un32DownloadSpeed,
		uint64_t un64Filesize,
		const std::string &clientToken);

	stiHResult DownloadSpeedSet(
		uint32_t un32DownloadSpeed);

	stiHResult FileChunkGet (
		Movie_t* pMovie = nullptr, 
		uint64_t beginByte = 0, 
		uint64_t endByte = 0);
	
	stiHResult GetSkipBackData(
		uint64_t un64SampleOffset,
		uint64_t un64EndOffset);

	stiHResult GetSkipForwardData(
		uint64_t un64ReleaseStartOffset,
		uint64_t un64ReleaseEndOffset,
		uint64_t un64SampleOffset,
		uint64_t un64EndOffset);

	bool IsSampleDownloading(
		uint64_t un64SampleOffset,
		uint32_t un32DataLength,
		bool bSkippingBack);

	uint32_t MaxBufferingSizeGet () const;

	stiHResult NotifyParseError();

	void ResetSendDownloadProgress();

	stiHResult ReleaseBuffer (
		uint32_t un32Offset, 
		uint32_t un32NumberOfBytes);

	stiHResult ResetExpiredDownloadGUID(
		const std::string &fileGUID);

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

	stiHResult ReadyToPlaySet (
		bool bReadyToPlay);

	stiHResult downloadResume(
		Movie_t* pMovie);

	void logInfoToSplunk ();

private:

	struct SSkipDataOffsets
	{
		uint64_t un64ReleaseStartOffset;
		uint64_t un64ReleaseEndOffset;
		uint64_t un64SampleOffset;
		uint64_t un64EndOffset;
	}; 

	std::string m_server;
	std::string m_fileGUID;
	std::string m_uploadFileName;
	std::string m_clientToken;
	std::stringstream m_uploadEndBoundary;
	uint32_t m_un32Width{0};
	uint32_t m_un32Height{0};
	uint32_t m_un32Bitrate{0};
	uint32_t m_un32Duration{0};
	FILE *m_pMovieFile{nullptr};

	CVMCircularBuffer *m_pCircBuf{nullptr};

	bool m_abortTransfer {false};
	EState m_eState{estiDOWNLOADING};

	ETransferType m_eTransferType{ePAUSED};
	uint64_t m_un64CurrentChunkStart{0};
	uint64_t m_un64CurrentChunkEnd{0};
	uint64_t m_un64ChunkLengthRemaining{0};
	uint64_t m_un64SkipChunkStart{0};
	uint64_t m_un64SkipChunkEnd{0};
	uint64_t m_un64SkipDataSize{0};	
	uint64_t m_un64DownloadDataTarget{0};
	int m_nSkipPercent{0};

	uint32_t m_un32DownloadSpeed{0};
	uint64_t m_un64CurrentOffset{0};
	uint64_t m_un64ParseOffset{0};		// The offset relative to the beginning of the file
	uint64_t m_un64YetToDownload{0};	// The number of bytes in the file to still be downloaded 
	uint64_t m_un64FileSize{0};			// The size of the file to be downloaded (in bytes)
	uint32_t m_un32DataRate{0};			// The actual data rate of the file transfer (in bytes per second)

	bool m_readyToPlay {false};
	bool m_waitingForParsableCheck {false};
	uint32_t m_un32MoreDataForParsableCheck{0};

	uint32_t m_un32TicksPerSecond{0};
	uint32_t m_un32LastUpdateTicks{0};
	uint32_t m_un32NextUpdateTicks{0};
	uint32_t m_bytesDuringLastInterval{0};

	bool m_useRangeRequest {false};
	bool m_waitingForRelease {false};
	bool m_sendDownloadProgress {true};
	
	uint32_t m_un32DataToRelease{0};

	stiHResult HTTPDownloadRangeRequest(uint64_t startRange, uint64_t endRange);
	stiHResult MoreDataAvailable (
		uint32_t un32Bytes);

	stiHResult DownloadProgressUpdate ();

	stiHResult Download ();
	stiHResult FinishUpload ();
	stiHResult InitializeDataRateTracking();
	stiHResult StartUpload ();
	bool UpdateDataRate ();
	stiHResult Upload (bool allowDataRateUpdate);
	void UploadProgressUpdate();
	stiHResult getHTTPResponse(std::string & responseBody);

	void dataProcess();

	void cleanupSocket ();

	std::unique_ptr<Poco::Net::HTTPRequest> getDownloadRequest (
		uint64_t rangeStart,
		uint64_t rangeEnd);

	std::unique_ptr<Poco::Net::HTTPClientSession> m_clientSession;
	std::unique_ptr<Poco::Net::HTTPResponse> m_response;
	std::unique_ptr<std::ostream> m_requestBody;
	std::unique_ptr<std::istream> m_responseBody;
	
	Poco::URI m_URL;

	//
	// Event Handlers
	//
	void EventShutdownHandle ();
	void EventFileChunkGet (Movie_t *movie);
	void EventFileReadChunkGet (Movie_t *movie);
	void eventDownloadResume (Movie_t *movie);
	void EventReadyToPlaySet (bool readyToPlay);
	void EventResetSendDownloadProgress ();
	void EventGetSkipBackData (const SSkipDataOffsets &skipDataOffsets);
	void EventGetSkipForwardData (const SSkipDataOffsets &skipDataOffsets);
	void EventStateSet (EState state);
							
	vpe::EventTimer m_dataTransferTimer;

	CstiSignalConnection::Collection m_dataTransferSignalConnections;

public:
	CstiSignal<> uploadCompleteSignal;
	CstiSignal<const SFileUploadProgress &> bufferUploadSpaceUpdateSignal;
	CstiSignal<> uploadErrorSignal;
	CstiSignal<uint32_t> uploadProgressSignal;
	CstiSignal<const SFileDownloadProgress &> fileDownloadProgressSignal;
	CstiSignal<const SFileDownloadProgress &> skipDownloadProgressSignal;
	CstiSignal<> downloadErrorSignal;
	CstiSignal<stiHResult> errorSignal;
	CstiSignal<uint64_t> messageSizeSignal;
	CstiSignal<std::string> messageIdSignal;
	CstiSignal<uint32_t> parsableMoovCheckSignal;
	CstiSignal<> transferCompleteSignal;
	CstiSignal<> shutdownCompleteSignal;
};

#endif // CDATATRANSFERHANDLER_H
