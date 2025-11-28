// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#include "CstiUserInput.h"
#include "CstiMonitorTask.h"
#include "stiTrace.h"
#include <fcntl.h>
#include <linux/input.h>


CstiUserInput::CstiUserInput ()
:
	CstiUserInputBase ("icd_uinput", "gpio-keys")
{
}


stiHResult CstiUserInput::Initialize(
	CstiMonitorTask *monitorTask)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_monitorTask = monitorTask;

	hResult = CstiUserInputBase::Initialize();
	stiTESTRESULT();

	m_signalConnections.push_back (m_monitorTask->mfddrvdStatusChangedSignal.Connect (
		[this]()
	{
		PostEvent (std::bind(&CstiUserInput::eventMfddrvdStatusChanged, this));
	}));

STI_BAIL:
	return hResult;
}


MonitorTaskBase *CstiUserInput::monitorTaskBaseGet ()
{
	return m_monitorTask;
}


/*
 * this is the message that we receive when udev detects a change
 * if it is that MFDDRVD has re-started , then re-open the remote control device
 */
void CstiUserInput::eventMfddrvdStatusChanged ()
{
	m_monitorTask->MfddrvdRunningGet (&m_mfddrvdRunning);

	if (m_mfddrvdRunning && m_remoteFd == -1)
	{
		int tmpFd = -1;
		char fileName[stiMAX_FILENAME_LENGTH + 1];
		char returnedName[stiMAX_FILENAME_LENGTH + 1];

		for (int i = 0; i < stiMAX_EVENT_FILES; i++)
		{
			sprintf(fileName, "/dev/input/event%d", i);

			tmpFd = open(fileName, O_RDONLY);

			if (tmpFd < 0)
			{
				break;
			}

			if (ioctl(tmpFd, EVIOCGNAME(sizeof(returnedName)), returnedName) < 0)
			{
				close (tmpFd);
				tmpFd = -1;
			}
			else if (!strcmp(returnedName, m_remoteInputName.c_str()))
			{
				m_remoteFd = tmpFd;
				if (!FileDescriptorAttach (m_remoteFd, std::bind(&CstiUserInput::eventReadRemote, this)))
				{
					stiASSERTMSG(estiFALSE, "%s: Can't attach remoteFd %d\n", __func__, m_remoteFd);
				}
				break;
			}
		}

		stiASSERT (m_remoteFd != -1);
	}
}


void CstiUserInput::eventReadRemote ()
{
	struct input_event event;

	auto numberBytesRead = stiOSRead (m_remoteFd, (void *)&event, sizeof (struct input_event));

	if (numberBytesRead < 0)
	{
		stiDEBUG_TOOL(g_stiUserInputDebug,
			stiTrace("CstiUserInputBase::Task: Read error. Assuming MFDDRVD stopped. errno = %d\n", errno);
		);

		FileDescriptorDetach (m_remoteFd);

		// in VP2 this indicates that MFDDRVD has stopped
		close(m_remoteFd);
		m_remoteFd = -1;
	}
	else if (numberBytesRead < (signed int)sizeof(struct input_event))
	{
		stiASSERTMSG (false, "Number of bytes read: %d\n", numberBytesRead);
	}
	else
	{
		remoteEventProcess (event, MSC_RAW);
	}
	
}
