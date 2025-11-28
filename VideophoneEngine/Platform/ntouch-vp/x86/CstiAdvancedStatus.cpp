#include "CstiAdvancedStatus.h"

stiHResult CstiAdvancedStatus::initialize (
		CstiMonitorTask *monitorTask,
		CstiAudioHandler *audioHandler,
		CstiAudioInput *audioInput,
		CstiVideoInput *videoInput,
		CstiVideoOutput *videoOutput,
		CstiBluetooth *bluetooth)
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiAdvancedStatus::advancedStatusGet (
	SstiAdvancedStatus &advancedStatus)
{
	return stiRESULT_SUCCESS;
}

std::string CstiAdvancedStatus::mpuSerialGet ()
{
	return std::string();
}


std::string CstiAdvancedStatus::rcuSerialGet ()
{
	return std::string();
}

