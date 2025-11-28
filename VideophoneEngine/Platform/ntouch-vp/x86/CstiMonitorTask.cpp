#include "CstiMonitorTask.h"

stiHResult CstiMonitorTask::Initialize ()
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiMonitorTask::Startup ()
{
	return stiRESULT_SUCCESS;
}

bool CstiMonitorTask::headphoneConnectedGet()
{
	return false;
}

bool CstiMonitorTask::microphoneConnectedGet()
{
	return false;
}

stiHResult CstiMonitorTask::RcuConnectedStatusGet (
	bool *pbRcuConnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pbRcuConnected = true;

	return hResult;
}
