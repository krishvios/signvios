// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2021 Sorenson Communications, Inc. -- All rights reserved

#include "CstiNetwork.h"

CstiNetwork::CstiNetwork(CstiBluetooth *bluetooth)
:
	CstiNetworkBase(bluetooth)
{
}

stiHResult CstiNetwork::Initialize()
{

	std::string pathName;

	stiOSDynamicDataFolderGet(&pathName);
	pathName += "/networking/lib/connman/";

	return CstiNetworkBase::Initialize(pathName, "/etc/init.d/connman stop &", "/etc/init.d/connman start &");
}
