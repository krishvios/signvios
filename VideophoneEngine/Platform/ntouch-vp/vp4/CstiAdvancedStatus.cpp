// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAdvancedStatus.h"
#include "IstiPlatform.h"
#include "IPlatformVP.h"

stiHResult CstiAdvancedStatus::mpuSerialSet ()
{
	m_advancedStatus.mpuSerialNumber = dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->serialNumberGet ();
	return stiRESULT_SUCCESS;
}

void CstiAdvancedStatus::staticStatusGet ()
{
	// Nothing platform specific
}

void CstiAdvancedStatus::dynamicStatusGet ()
{
	// Nothing platform specific
}

