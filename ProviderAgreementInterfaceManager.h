//
//  ProviderAgreementInterfaceManager.h
//  ntouch
//
//  Created by Nate Chandler on 9/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIProviderAgreementInterfaceManager.h"

@interface ProviderAgreementInterfaceManager : NSObject <SCIProviderAgreementInterfaceManager>

@property (class, readonly) ProviderAgreementInterfaceManager *sharedManager;

@end
