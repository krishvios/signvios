// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved

#include "FanControlBase.h"
#include "stiTools.h"


void FanControlBase::inCallConfigSet (bool inCall)
{
}

int FanControlBase::dutyCycleGet ()
{
	return -1;
}

stiHResult FanControlBase::dutyCycleSet (int dutyCycle)
{
	return stiRESULT_ERROR;
}
