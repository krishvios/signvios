// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#include "CstiUserInput.h"

#define stiDISPLAYTASK_MAX_MESSAGES_IN_QUEUE 12
#define stiDISPLAYTASK_MAX_MSG_SIZE 512
#define stiDISPLAYTASK_TASK_NAME "CstiUserInput"
#define stiDISPLAYTASK_TASK_PRIORITY 151
#define stiDISPLAYTASK_STACK_SIZE 4096


CstiUserInput::CstiUserInput ()
	:
	CstiOsTaskMQ (
		NULL,
		0,
		(size_t)this,
		stiDISPLAYTASK_MAX_MESSAGES_IN_QUEUE,
		stiDISPLAYTASK_MAX_MSG_SIZE,
		stiDISPLAYTASK_TASK_NAME,
		stiDISPLAYTASK_TASK_PRIORITY,
		stiDISPLAYTASK_STACK_SIZE)
{
}


CstiUserInput::~CstiUserInput ()
{
	Shutdown ();
	WaitForShutdown ();
}


stiHResult CstiUserInput::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


int CstiUserInput::Task ()
{
	while (1) {}	// loop indefinitely
	return 0;
}



