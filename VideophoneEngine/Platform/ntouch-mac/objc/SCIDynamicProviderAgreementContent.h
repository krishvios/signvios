//
//  SCIDynamicProviderAgreementContent.h
//  ntouchMac
//
//  Created by Nate Chandler on 6/7/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SCIDynamicProviderAgreementContent : NSObject

+ (instancetype)agreementContentWithPublicID:(NSString *)publicID text:(NSString *)text type:(NSString *)type;

@property (nonatomic) NSString *publicID;
@property (nonatomic) NSString *text;
@property (nonatomic) NSString *type;

@end
