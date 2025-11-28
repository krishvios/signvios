///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Signals.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef MP4SIGNALS_H
#define MP4SIGNALS_H

//
// Sorenson includes.
//
#include "SMTypes.h"
#include "SorensonMP4.h"
#include "CstiSignal.h"

struct SFileDownloadProgress
{
	bool_t bBufferFull{false};
	uint64_t un64FileSizeInBytes{0};		// The file size of the whole file in bytes
	uint64_t un64BytesYetToDownload{0};		// The number of bytes yet to download
	uint64_t m_bytesInBuffer{0};			// The number of bytes in the ring buffer.
	uint32_t un32DataRate{0};				// The actual data rate of the file transfer (in bytes per second)
}; 

struct SFileUploadProgress
{
	uint64_t m_bufferBytesAvailable{0};		// The number of bytes yet to download
	uint32_t un32DataRate{0};				// The actual data rate of the file transfer (in bytes per second)
}; 

struct SMediaSampleInfo
{
	uint32_t un32SampleIndex{0};
	uint32_t un32SampleDuration{0};
	uint8_t* pun8SampleData{nullptr};
	uint32_t un32SampleSize{0};
	uint32_t un32SampleDescIndex{0};
	bool  keyFrame{false};
}; 

extern CstiSignal<const SMediaSampleInfo &> mediaSampleReadySignal;
extern CstiSignal<int> mediaSampleNotReadySignal;
extern CstiSignal<stiHResult> errorSignal;
extern CstiSignal<> movieReadySignal;
extern CstiSignal<uint64_t> messageSizeSignal;
extern CstiSignal<std::string> messageIdSignal;
extern CstiSignal<const SFileDownloadProgress &> fileDownloadProgressSignal;
extern CstiSignal<const SFileDownloadProgress &> skipDownloadProgressSignal;
extern CstiSignal<const SFileUploadProgress &> bufferUploadSpaceUpdateSignal;
extern CstiSignal<uint32_t> uploadProgressSignal;
extern CstiSignal<> uploadErrorSignal;
extern CstiSignal<> downloadErrorSignal;
extern CstiSignal<> uploadCompleteSignal;
extern CstiSignal<Movie_t *> movieClosedSignal;

#endif // MP4SIGNALS_H


