////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Abstract:
//		This header file contains the class declaration for the Settings List class.
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>

struct SstiSettingItem
{
	enum EstiSettingType
	{
		estiString,
		estiInt,
		estiBool,
		estiEnum
	};
	
	std::string Name;
	EstiSettingType eType {estiInt};
	std::string Value;
};
