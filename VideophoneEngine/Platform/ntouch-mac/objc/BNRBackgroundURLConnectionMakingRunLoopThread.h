//
//  BNRBackgroundURLConnectionMakingRunLoopThread.h
//  YelpDemo
//
//  Created by Nate Chandler on 6/24/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "BNRBackgroundRunLoopThread.h"

@interface BNRBackgroundURLConnectionMakingRunLoopThread : BNRBackgroundRunLoopThread

- (id)performURLRequest:(NSURLRequest *)request
		completionQueue:(dispatch_queue_t)queue
				  delay:(NSTimeInterval)delay
			 completion:(void (^)(NSURLResponse *response, NSData *data, NSError *error))block;
- (id)performURLRequest:(NSURLRequest *)request
		completionQueue:(dispatch_queue_t)queue
			 completion:(void (^)(NSURLResponse *response, NSData *data, NSError *error))block;
- (BOOL)cancelURLRequestWithIdentifier:(id)identifier;

@end
