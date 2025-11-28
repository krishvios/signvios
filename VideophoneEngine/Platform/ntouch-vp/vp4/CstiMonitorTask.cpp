// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiMonitorTask.h"

bool CstiMonitorTask::isCameraAvailable ()
{
	const std::string cameraDetectCommand = "i2cdetect -y -r 1 | grep 30: | awk '{ print $9 }'";

	std::string result;
	// This is what will be the result if there is no camera
	std::string noCamera ("--"); 

	// See if camera is plugged in
	// This is the only way we can tell if the camera is plugged in
	// We currently dont have a way of triggering if things change after initialization
	result = systemExecute (cameraDetectCommand);
	
	stiDEBUG_TOOL (g_stiMonitorDebug,
		vpe::trace (cameraDetectCommand, " = ", result, "\n");
	);
	
	return ((result.compare(0, 2, noCamera)) != 0);
}