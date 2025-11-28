// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAdvancedStatus.h"
#include "IstiPlatform.h"
#include "IPlatformVP.h"



/*
 * just give it something for now
 */
stiHResult CstiAdvancedStatus::mpuSerialSet()
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
	std::lock_guard<std::recursive_mutex> lock (m_statusMutex);
	m_advancedStatus.audioJack = m_monitorTask->headphoneConnectedGet();
}



