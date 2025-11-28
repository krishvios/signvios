//
//  SCIServiceOutageMessageType.m
//  ntouch
//
//  Created by Kevin Selman on 2/12/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIServiceOutageMessageType.h"
#include "ServiceOutageMessage.h"

const SCIServiceOutageMessageType SCIServiceOutageMessageTypeNone = (NSInteger)vpe::ServiceOutageMessageType::None;
const SCIServiceOutageMessageType SCIServiceOutageMessageTypeError = (NSInteger)vpe::ServiceOutageMessageType::Error;
