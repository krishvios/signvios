//
//  SCIUserAccountInfo+CstiUserAccountInfo.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/23/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIUserAccountInfo.h"
#import "CstiUserAccountInfo.h"

@interface SCIUserAccountInfo (CstiUserAccountInfo)
+ (SCIUserAccountInfo *)userAccountInfoWithCstiUserAccountInfo:(CstiUserAccountInfo *)cstiUserAccountInfo;
- (CstiUserAccountInfo *)cstiUserAccountInfo;
@end
