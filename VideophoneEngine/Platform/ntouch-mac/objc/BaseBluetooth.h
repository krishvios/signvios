//
//  BaseBluetooth.h
//  ntouch
//
//  Created by Kevin Selman on 7/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

#pragma once

#include "IstiBluetooth.h"
#include "IstiLightRing.h"
#include "IstiStatusLED.h"

class BaseBluetooth : public IstiBluetooth, public IstiLightRing, public IstiStatusLED
{
	
};
