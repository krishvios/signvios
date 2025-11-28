////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Signals.cpp
//
//	Abstract: 
//			  
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
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

//
// Globals
//


CstiSignal<const SMediaSampleInfo &> mediaSampleReadySignal;          
CstiSignal<int> mediaSampleNotReadySignal;                      
CstiSignal<stiHResult> errorSignal;                             
CstiSignal<> movieReadySignal;                                  
CstiSignal<uint64_t> messageSizeSignal;                         
CstiSignal<std::string> messageIdSignal;                        
CstiSignal<const SFileDownloadProgress &> fileDownloadProgressSignal; 
CstiSignal<const SFileDownloadProgress &> skipDownloadProgressSignal; 
CstiSignal<const SFileUploadProgress &> bufferUploadSpaceUpdateSignal;
CstiSignal<uint32_t> uploadProgressSignal;                      
CstiSignal<> uploadErrorSignal;                                 
CstiSignal<> downloadErrorSignal;                               
CstiSignal<> uploadCompleteSignal;                              
CstiSignal<Movie_t *> movieClosedSignal;                        
