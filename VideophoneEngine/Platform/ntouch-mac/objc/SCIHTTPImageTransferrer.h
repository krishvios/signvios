//
//  SCIHTTPImageTransferrer.h
//  ntouchMac
//
//  Created by Nate Chandler on 1/23/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SCIHTTPImageTransferrer : NSObject

@property (nonatomic) NSArray *URLs;

@property (nonatomic) NSDictionary *uploadParameters;
@property (nonatomic) NSString *uploadName;
@property (nonatomic) NSString *uploadFilename;
@property (nonatomic) NSString *uploadFormat;

- (void)downloadImageDataWithCompletion:(void (^)(NSData *data, NSError *error))block;
- (void)uploadImageData:(NSData *)data completion: (void (^)(BOOL success, NSError *error))block;

@end
