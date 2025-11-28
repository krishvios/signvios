// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAudioHandler.h"
#include <BluetoothAudio.h>
#include "stiTrace.h"
#include "AudioPlaybackPipelineX86.h"
#include "AudioRecordPipelineX86.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

CstiAudioHandler::CstiAudioHandler()
{
}