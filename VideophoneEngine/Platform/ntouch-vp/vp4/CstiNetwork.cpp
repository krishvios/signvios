// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2021 Sorenson Communications, Inc. -- All rights reserved

#include "CstiNetwork.h"

CstiNetwork::CstiNetwork(CstiBluetooth *bluetooth)
:
	CstiNetworkBase2(bluetooth)
{
}

stiHResult CstiNetwork::Initialize()
{
	return CstiNetworkBase2::Initialize("/var/lib/connman/", "systemctl stop connman &", "systemctl start connman &");
}
