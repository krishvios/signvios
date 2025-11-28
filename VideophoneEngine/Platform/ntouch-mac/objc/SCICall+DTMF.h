//
//  SCICall+DTMF.h
//  ntouchMac
//
//  Created by Nate Chandler on 12/20/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICall.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	NSArray *SCIDTMFCodesForNSString(NSString *characters);
	NSString *DisplayableNSStringForSCIDTMFCode(SCIDTMFCode code);
	
#ifdef __cplusplus
}
#endif
