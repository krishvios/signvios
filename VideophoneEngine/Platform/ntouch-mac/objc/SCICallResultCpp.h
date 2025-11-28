//
//  SCICallResultCpp.h
//  ntouchMac
//
//  Created by Nate Chandler on 8/15/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCICallResult.h"
#import "stiSVX.h"

extern SCICallResult SCICallResultFromEstiCallResult(EstiCallResult cr);
extern EstiCallResult EstiCallResultFromSCICallResult(SCICallResult cr);
