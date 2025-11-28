#pragma once

#include "stiSVX.h"

#include "IstiPlatform.h"

class CstiMonitorTask;
class CstiAudioHandler;
class CstiAudioInput;
class CstiVideoInput;
class CstiVideoOutput;
class CstiBluetooth;

class CstiAdvancedStatus
{
public:

	stiHResult initialize (
			CstiMonitorTask *monitorTask,
			CstiAudioHandler *audioHandler,
			CstiAudioInput *audioInput,
			CstiVideoInput *videoInput,
			CstiVideoOutput *videoOutput,
			CstiBluetooth *bluetooth);

	stiHResult advancedStatusGet (
		SstiAdvancedStatus &advancedStatus);

	std::string mpuSerialGet ();
	std::string rcuSerialGet ();
};
