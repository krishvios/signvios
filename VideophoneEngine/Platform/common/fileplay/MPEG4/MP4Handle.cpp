// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "SMMemory.h"


SMHandle MP4NewHandle(
	unsigned int unSize)
{
	return SMNewHandle(unSize);
}


SMResult_t MP4SetHandleSize(
	SMHandle hHandle,
	unsigned int unSize)
{
	return SMSetHandleSize(hHandle, unSize);
}


unsigned int MP4GetHandleSize(
	SMHandle hHandle)
{
	return SMGetHandleSize(hHandle);
}


void MP4DisposeHandle(
	SMHandle hHandle)
{
	SMDisposeHandle(hHandle);
}



