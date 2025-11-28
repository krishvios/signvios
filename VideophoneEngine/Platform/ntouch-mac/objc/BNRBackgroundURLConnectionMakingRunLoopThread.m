//
//  BNRBackgroundURLConnectionMakingRunLoopThread.m
//  YelpDemo
//
//  Created by Nate Chandler on 6/24/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "BNRBackgroundURLConnectionMakingRunLoopThread.h"
#import "BNRBackgroundRunLoopThread_Internal.h"
#import "NSThread+PerformBlock.h"
#import "BNRUUID.h"
#import "BNRDispatchQueueWrapper.h"

@interface BNRBackgroundURLConnectionMakingRunLoopThread () <NSURLConnectionDataDelegate>
@property (nonatomic) NSMutableDictionary *queuesForConnections;
@property (nonatomic) NSMutableDictionary *completionsForConnections;
@property (nonatomic) NSMutableDictionary *responsesForConnections;
@property (nonatomic) NSMutableDictionary *dataForConnections;
@property (nonatomic) NSMutableDictionary *errorsForConnections;

@property (nonatomic) NSMutableDictionary *startedsForConnections;
@property (nonatomic) NSMutableDictionary *shouldNotStartsForConnections;

@property (nonatomic) NSMutableDictionary *connectionsForIdentifiers;
@property (nonatomic) NSMutableDictionary *beginURLRequestBlocksForIdentifiers;
@end

@implementation BNRBackgroundURLConnectionMakingRunLoopThread

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.queuesForConnections = [[NSMutableDictionary alloc] init];
		self.completionsForConnections = [[NSMutableDictionary alloc] init];
		self.responsesForConnections = [[NSMutableDictionary alloc] init];
		self.dataForConnections = [[NSMutableDictionary alloc] init];
		self.errorsForConnections = [[NSMutableDictionary alloc] init];
		
		self.startedsForConnections = [[NSMutableDictionary alloc] init];
		self.shouldNotStartsForConnections = [[NSMutableDictionary alloc] init];
		
		self.connectionsForIdentifiers = [[NSMutableDictionary alloc] init];
		self.beginURLRequestBlocksForIdentifiers = [[NSMutableDictionary alloc] init];
	}
	return self;
}

#pragma mark - Accessors

- (id<NSCopying>)keyForConnection:(NSURLConnection *)connection
{
	return [NSValue valueWithNonretainedObject:connection];
}

- (void)pushQueue:(dispatch_queue_t)queue forConnection:(NSURLConnection *)connection
{
	@synchronized(self.queuesForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			self.queuesForConnections[key] = [BNRDispatchQueueWrapper wrapperForDispatchQueue:queue];
		}
	}
}

- (dispatch_queue_t)popQueueForConnection:(NSURLConnection *)connection
{
	dispatch_queue_t res = nil;
	
	@synchronized(self.queuesForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			BNRDispatchQueueWrapper *wrapper = (BNRDispatchQueueWrapper *)self.queuesForConnections[key];
			res = wrapper.queue;
			[self.queuesForConnections removeObjectForKey:key];
		}
	}
	
	return res;
}

- (void)pushCompletion:(void (^)(NSURLResponse *response, NSData *data, NSError *error))completion forConnection:(NSURLConnection *)connection
{
	@synchronized(self.completionsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			self.completionsForConnections[key] = completion;
		}
	}
}

- (void (^)(NSURLResponse *response, NSData *data, NSError *error))popCompletionForConnection:(NSURLConnection *)connection
{
	void (^res)(NSURLResponse *response, NSData *data, NSError *error) = nil;
	
	@synchronized(self.completionsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			res = self.completionsForConnections[key];
			[self.completionsForConnections removeObjectForKey:key];
		}
	}
	
	return res;
}

- (void)pushURLResponse:(NSURLResponse *)response forConnection:(NSURLConnection *)connection
{
	@synchronized(self.responsesForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			self.responsesForConnections[key] = response;
		}
	}
}

- (NSURLResponse *)popURLResponseForConnection:(NSURLConnection *)connection
{
	NSURLResponse *res = nil;
	
	@synchronized(self.responsesForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			res = self.responsesForConnections[key];
			[self.responsesForConnections removeObjectForKey:key];
		}
	}
	
	return res;
}

- (void)appendData:(NSData *)data forConnection:(NSURLConnection *)connection
{
	@synchronized(self.dataForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			NSMutableData *mutableData = self.dataForConnections[key];
			if (!mutableData) {
				mutableData = [[NSMutableData alloc] init];
				self.dataForConnections[key] = mutableData;
			}
			[mutableData appendData:data];
		}
	}
}

- (NSData *)popDataForConnection:(NSURLConnection *)connection
{
	NSData *res = nil;
	
	@synchronized(self.dataForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			NSMutableData *mutableRes = self.dataForConnections[key];
			[self.dataForConnections removeObjectForKey:key];
			res = [mutableRes copy];
		}
	}
	
	return res;
}

- (void)pushError:(NSError *)error forConnection:(NSURLConnection *)connection
{
	@synchronized(self.errorsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			self.errorsForConnections[key] = error;
		}
	}
}

- (NSError *)popErrorForConnection:(NSURLConnection *)connection
{
	NSError *res = nil;
	
	@synchronized(self.errorsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			res = self.errorsForConnections[key];
			[self.errorsForConnections removeObjectForKey:key];
		}
	}
	
	return res;
}

- (void)pushStarted:(BOOL)started forConnection:(NSURLConnection *)connection
{
	@synchronized(self.startedsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			self.startedsForConnections[key] = @(started);
		}
	}
}

- (BOOL)startedForConnection:(NSURLConnection *)connection
{
	BOOL res = NO;
	
	@synchronized(self.startedsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			NSNumber *resNumber = self.startedsForConnections[key];
			res = resNumber.boolValue;
		}
	}
	
	return res;
}

- (BOOL)popStartedForConnection:(NSURLConnection *)connection
{
	BOOL res = NO;
	
	@synchronized(self.startedsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		if (key) {
			NSNumber *resNumber = self.startedsForConnections[key];
			[self.startedsForConnections removeObjectForKey:key];
			res = resNumber.boolValue;
		}
	}
	
	return res;
}

- (void)pushShouldNotStart:(BOOL)shouldNotStart forConnection:(NSURLConnection *)connection
{
	@synchronized(self.shouldNotStartsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		self.shouldNotStartsForConnections[key] = @(shouldNotStart);
	}
}

- (BOOL)popShouldNotStartForConnection:(NSURLConnection *)connection
{
	BOOL res = NO;
	
	@synchronized(self.shouldNotStartsForConnections)
	{
		id<NSCopying> key = [self keyForConnection:connection];
		NSNumber *resNumber = self.shouldNotStartsForConnections[key];
		[self.shouldNotStartsForConnections removeObjectForKey:key];
		res = resNumber.boolValue;
	}
	
	return res;
}

- (void)performForConnection:(NSURLConnection *)connection addShouldNotStartWithBlock:(BOOL (^)())block
{
	@synchronized(self.shouldNotStartsForConnections)
	{
		BOOL shouldNotStart = block();
		if (shouldNotStart) {
			id<NSCopying> key = [self keyForConnection:connection];
			self.shouldNotStartsForConnections[key] = @(shouldNotStart);
		}
	}
}

- (void)setConnection:(NSURLConnection *)connection forIdentifier:(id<NSCopying>)theIdentifier
{
	@synchronized(self.connectionsForIdentifiers)
	{
		self.connectionsForIdentifiers[theIdentifier] = connection;
	}
}

- (NSURLConnection *)connectionForIdentifier:(id<NSCopying>)theIdentifier
{
	NSURLConnection *res = nil;
	
	@synchronized(self.connectionsForIdentifiers)
	{
		res = self.connectionsForIdentifiers[theIdentifier];
	}
	
	return res;
}

- (void)removeConnectionForIdentifier:(id<NSCopying>)theIdentifier
{
	@synchronized(self.connectionsForIdentifiers)
	{
		[self.connectionsForIdentifiers removeObjectForKey:theIdentifier];
	}
}

- (void)removeIdentifierForConnection:(NSURLConnection *)connection
{
	@synchronized(self.connectionsForIdentifiers)
	{
		__block id theKey = nil;
		[self.connectionsForIdentifiers enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
			if (obj == connection) {
				theKey = key;
				*stop = YES;
			}
		}];
		[self.connectionsForIdentifiers removeObjectForKey:theKey];
	}
}

- (void)pushForIdentifier:(id<NSCopying>)identifier beginURLRequestBlock:(void (^)(void))block
{
	@synchronized(self.beginURLRequestBlocksForIdentifiers)
	{
		self.beginURLRequestBlocksForIdentifiers[identifier] = block;
	}
}

- (void)removeBeginURLRequestBlockForIdentifier:(id<NSCopying>)identifier
{
	@synchronized(self.beginURLRequestBlocksForIdentifiers)
	{
		[self.beginURLRequestBlocksForIdentifiers removeObjectForKey:identifier];
	}
}

- (void)popBeginURLRequestBlockForIdentifier:(id<NSCopying>)identifier withBlock:(void (^)(void (^)(void)))block
{
	@synchronized(self.beginURLRequestBlocksForIdentifiers)
	{
		void (^res)(void) = nil;
		res = self.beginURLRequestBlocksForIdentifiers[identifier];
		[self.beginURLRequestBlocksForIdentifiers removeObjectForKey:identifier];
		
		if (block) {
			block(res);
		}
	}
}

- (void)runBeginURLRequestBlockForIdentifier:(id<NSCopying>)identifier afterDelay:(NSTimeInterval)delay
{
	if (delay > 0.0) {
		__unsafe_unretained BNRBackgroundURLConnectionMakingRunLoopThread *weak_self = self;
		[self addRunloopSourceAddingBlock:^(BNRBackgroundRunLoopThread *thread, NSRunLoop *runloop) {
			__strong BNRBackgroundURLConnectionMakingRunLoopThread *strong_self = weak_self;
			@synchronized(strong_self.beginURLRequestBlocksForIdentifiers)
			{
				if (strong_self.beginURLRequestBlocksForIdentifiers[identifier])
				{
					[strong_self performSelector:@selector(internal_runBeginURLRequestBlockForIdentifier:) withObject:identifier afterDelay:delay];
				}
			}
		}];
	} else {
		[self popBeginURLRequestBlockForIdentifier:identifier withBlock:^(void (^beginURLRequestBlock)(void)) {
			if (beginURLRequestBlock) {
				beginURLRequestBlock();
			}
		}];
	}
}

- (void)internal_runBeginURLRequestBlockForIdentifier:(id<NSCopying>)identifier
{
	[self popBeginURLRequestBlockForIdentifier:identifier withBlock:^(void (^beginURLRequestBlock)(void)) {
		if (beginURLRequestBlock) {
			beginURLRequestBlock();
		}
	}];
}

#pragma mark - Public API

- (id)performURLRequest:(NSURLRequest *)request
		completionQueue:(dispatch_queue_t)queue
				  delay:(NSTimeInterval)delay
			 completion:(void (^)(NSURLResponse *response, NSData *data, NSError *error))block
{
	BNRUUID *connectionIdentifier = [BNRUUID UUID];
	
	[self pushForIdentifier:connectionIdentifier beginURLRequestBlock:^{
		NSURLConnection *connection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:NO];
		[self pushStarted:NO forConnection:connection];
		[self setConnection:connection forIdentifier:connectionIdentifier];
		[self pushQueue:queue forConnection:connection];
		[self pushCompletion:block forConnection:connection];
		[self addRunloopSourceAddingBlock:^(BNRBackgroundRunLoopThread *thread, NSRunLoop *runloop) {
			BNRBackgroundURLConnectionMakingRunLoopThread *strong_self = (BNRBackgroundURLConnectionMakingRunLoopThread *)thread;
			BOOL shouldNotStart = [strong_self popShouldNotStartForConnection:connection];
			if (shouldNotStart) {
				[strong_self popStartedForConnection:connection];
				[strong_self removeConnectionForIdentifier:connectionIdentifier];
			} else {
				[strong_self pushStarted:YES forConnection:connection];
				[connection scheduleInRunLoop:runloop forMode:NSRunLoopCommonModes];
				[connection start];
			}
		}];
	}];
	[self runBeginURLRequestBlockForIdentifier:connectionIdentifier afterDelay:delay];
	
	return connectionIdentifier;
}

- (id)performURLRequest:(NSURLRequest *)request
		completionQueue:(dispatch_queue_t)queue
			 completion:(void (^)(NSURLResponse *response, NSData *data, NSError *error))block
{
	return [self performURLRequest:request completionQueue:queue delay:0.0 completion:block];
}

- (BOOL)cancelURLRequestWithIdentifier:(id<NSCopying>)identifier
{
	__block BOOL res = NO;
	
	[self popBeginURLRequestBlockForIdentifier:identifier withBlock:^(void (^beginURLRequestBlock)(void)) {
		if (beginURLRequestBlock) {
			res = YES;
		} else {
			__unsafe_unretained BNRBackgroundURLConnectionMakingRunLoopThread *weak_self = self;
			
			__block BOOL shouldRemoveConnectionForIdentifier = NO;
			
			NSURLConnection *connection = [self connectionForIdentifier:identifier];
			if (connection) {
				[self performForConnection:connection addShouldNotStartWithBlock:^BOOL{
					BOOL addShouldNotStart = NO;
					
					BOOL started = [weak_self startedForConnection:connection];
					if (started) {
						[self performBlock:^{
							[connection cancel];
							[connection unscheduleFromRunLoop:self.runloop forMode:NSRunLoopCommonModes];
						}
							 waitUntilDone:YES];
						addShouldNotStart = NO;
						shouldRemoveConnectionForIdentifier = YES;
					} else {
						addShouldNotStart = YES;
						shouldRemoveConnectionForIdentifier = NO;
					}
					
					return addShouldNotStart;
				}];
				
				if (shouldRemoveConnectionForIdentifier) {
					[self removeConnectionForIdentifier:identifier];
				}
				res = YES;
			} else {
				res = NO;
			}
		}
	}];
	
	return res;
}

#pragma mark - Helpers

- (void)dispatchCompletionForConnection:(NSURLConnection *)connection
{
	[connection unscheduleFromRunLoop:self.runloop forMode:NSRunLoopCommonModes];
	
	dispatch_queue_t queue = [self popQueueForConnection:connection];
	void (^block)(NSURLResponse *response, NSData *data, NSError *error) = [self popCompletionForConnection:connection];
	NSURLResponse *response = [self popURLResponseForConnection:connection];
	NSData *data = [self popDataForConnection:connection];
	NSError *error = [self popErrorForConnection:connection];
	[self popStartedForConnection:connection];
	[self removeIdentifierForConnection:connection];
	
	if (block) {
		dispatch_async(queue, ^{
			block(response, data, error);
		});
	}
}

#pragma mark - NSURLConnectionDataDelegate

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
	[self pushURLResponse:response forConnection:connection];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
	[self appendData:data forConnection:connection];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
	[self pushError:error forConnection:connection];
	[self dispatchCompletionForConnection:connection];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
	[self dispatchCompletionForConnection:connection];
}

@end
