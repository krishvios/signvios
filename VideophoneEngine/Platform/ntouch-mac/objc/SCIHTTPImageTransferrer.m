//
//  SCIHTTPImageTransferrer.m
//  ntouchMac
//
//  Created by Nate Chandler on 1/23/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIHTTPImageTransferrer.h"
#import "SCIVideophoneEngine.h"

const NSTimeInterval kSCIHTTPImageTransferrerTimeoutInterval = 5.0f;

typedef enum SCIHTTPImageTransferrerMode {
	SCIHTTPImageTransferrerModeNone,
	SCIHTTPImageTransferrerModeUpload,
	SCIHTTPImageTransferrerModeDownload,
} SCIHTTPImageTransferrerMode;

@interface SCIHTTPImageTransferrer () <NSURLConnectionDelegate>
@property (nonatomic, copy) void (^uploadCompletion)(BOOL success, NSError *error);
@property (nonatomic, copy) void (^downloadCompletion)(NSData *data, NSError *error);
@property (nonatomic) NSData *uploadData;
@property (nonatomic) NSMutableData *downloadData;
@property (nonatomic) NSURLConnection *connection;
@property (nonatomic) SCIHTTPImageTransferrerMode mode;
@property (nonatomic) NSMutableArray *unusedURLs;
@end

@implementation SCIHTTPImageTransferrer

#pragma mark - Helpers: Beginning Transfer

- (NSURL *)popURL
{
	NSURL *res = nil;
	
	if (self.unusedURLs.count > 0)
	{
		NSString *urlString = self.unusedURLs[0];
		[self.unusedURLs removeObjectAtIndex:0];
		res = [NSURL URLWithString:urlString];
	}
	
	return res;
}

//This method return NO just in case there are no URLs remaining in self.unusedURLs to pop.
- (BOOL)popURLAndDownloadImageData
{
	BOOL res = NO;
	
	NSURL *url = nil;
	if ((url = [self popURL]))
	{
		res = YES;
		NSLog(@"%s:  Attempting to download image from %@", __PRETTY_FUNCTION__, url);
		
		self.downloadData = [NSMutableData data];
		
		NSURLRequest *request = [NSURLRequest requestWithURL:url
												 cachePolicy:NSURLRequestUseProtocolCachePolicy
											 timeoutInterval:kSCIHTTPImageTransferrerTimeoutInterval];
		
		self.connection = [NSURLConnection connectionWithRequest:request delegate:self];
		
		
	}
	
	return res;
}

//This method return NO just in case there are no URLs remaining in self.unusedURLs to pop.
- (BOOL)popURLAndUploadImageData
{
	BOOL res = NO;
	
	NSURL *url = nil;
	if ((url = [self popURL]))
	{
		res = YES;
		NSLog(@"%s:  Attempting to upload image to %@", __PRETTY_FUNCTION__, url);
		
		NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url
															   cachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData
														   timeoutInterval:kSCIHTTPImageTransferrerTimeoutInterval];
		
		[request setCachePolicy:NSURLRequestReloadIgnoringLocalCacheData];
		[request setHTTPShouldHandleCookies:NO];
		[request setTimeoutInterval:30];
		[request setHTTPMethod:@"POST"];
		
		NSString *boundaryString = @"BoundaryString";
		
		// set Content-Type in HTTP header
		NSString *contentType = [NSString stringWithFormat:@"multipart/form-data; boundary=%@", boundaryString];
		[request setValue:contentType forHTTPHeaderField: @"Content-Type"];
		
		// post body
		NSMutableData *body = [NSMutableData data];
		
		// add params (all params are strings)
		for (NSString *param in self.uploadParameters) {
			[body appendData:[[NSString stringWithFormat:@"--%@\r\n", boundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
			[body appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"%@\"\r\n\r\n", param] dataUsingEncoding:NSUTF8StringEncoding]];
			[body appendData:[[NSString stringWithFormat:@"%@\r\n", [self.uploadParameters objectForKey:param]] dataUsingEncoding:NSUTF8StringEncoding]];
		}
		
		// add image data
		NSData *data = self.uploadData;
		if (data) {
			[body appendData:[[NSString stringWithFormat:@"--%@\r\n", boundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
			[body appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"%@\"; filename=\"%@\"\r\n", self.uploadName, self.uploadFilename] dataUsingEncoding:NSUTF8StringEncoding]];
			[body appendData:[[NSString stringWithFormat:@"Content-Type: image/%@\r\n\r\n", self.uploadFormat] dataUsingEncoding:NSUTF8StringEncoding]];
			[body appendData:data];
			[body appendData:[[NSString stringWithFormat:@"\r\n"] dataUsingEncoding:NSUTF8StringEncoding]];
		}
		
		[body appendData:[[NSString stringWithFormat:@"--%@--\r\n", boundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
		
		// setting the body of the post to the reqeust
		[request setHTTPBody:body];
		
		self.connection = [NSURLConnection connectionWithRequest:request delegate:self];
	}
		
	return res;
}

#pragma mark - Public API

- (void)downloadImageDataWithCompletion:(void (^)(NSData *data, NSError *error))block
{
	NSAssert(self.mode == SCIHTTPImageTransferrerModeNone, @"Called %s before finishing previous operation.", __PRETTY_FUNCTION__);
	self.mode = SCIHTTPImageTransferrerModeDownload;
	self.downloadCompletion = block;
	self.unusedURLs = [self.URLs mutableCopy];
	BOOL success = [self popURLAndDownloadImageData];
	NSAssert(success, @"Called %s before setting any URLs.", __PRETTY_FUNCTION__);
}

- (void)uploadImageData:(NSData *)data completion: (void (^)(BOOL success, NSError *error))block
{
	NSAssert(self.mode == SCIHTTPImageTransferrerModeNone, @"Called %s before finishing previous operation.", __PRETTY_FUNCTION__);
	self.mode = SCIHTTPImageTransferrerModeUpload;
	self.uploadCompletion = block;
	self.uploadData = data;
	self.unusedURLs = [self.URLs mutableCopy];
	BOOL success = [self popURLAndUploadImageData];
	NSAssert(success, @"Called %s before setting any URLs.", __PRETTY_FUNCTION__);
}

#pragma mark - Helpers: Ending Transfer

- (void)retryOrPerformDownloadCompletionWithData:(NSData *)data error:(NSError *)error
{
	BOOL shouldPerformCompletion = NO;
	
	if (data || ![self popURLAndDownloadImageData])
	{
		shouldPerformCompletion = YES;
	}
	
	if (shouldPerformCompletion)
	{
		[self performDownloadCompletionWithData:data error:error];
	}
}

- (void)retryOrPerformUploadCompletionWithSuccess:(BOOL)success error:(NSError *)error
{
	BOOL shouldPerformCompletion = NO;
	
	if (success || ![self popURLAndUploadImageData])
	{
		shouldPerformCompletion = YES;
	}
	
	if (shouldPerformCompletion)
	{
		[self performUploadCompletionWithSuccess:success error:error];
	}
}

- (void)performDownloadCompletionWithData:(NSData *)data error:(NSError *)error
{
	self.downloadCompletion(data, error);
	self.downloadCompletion = nil;
	self.downloadData = nil;
	self.unusedURLs = nil;
	self.connection = nil;
	self.mode = SCIHTTPImageTransferrerModeNone;
}

- (void)performUploadCompletionWithSuccess:(BOOL)success error:(NSError *)error
{
	self.uploadCompletion(success, error);
	self.uploadCompletion = nil;
	self.uploadData = nil;
	self.unusedURLs = nil;
	self.connection = nil;
	self.mode = SCIHTTPImageTransferrerModeNone;
}


- (void)stopDownloadWithError:(NSError *)error
// Shuts down the connection and sends the error
{
    if (self.connection != nil) {
        [self.connection cancel];
        self.connection = nil;
    }
    if (self.downloadData != nil) {
        self.downloadData = nil;
    }
	
	switch (self.mode) {
		case SCIHTTPImageTransferrerModeDownload: {
			[self retryOrPerformDownloadCompletionWithData:nil error:error];
		} break;
		case SCIHTTPImageTransferrerModeUpload: {
			[self retryOrPerformUploadCompletionWithSuccess:NO error:error];
		} break;
		case SCIHTTPImageTransferrerModeNone: {
			NSAssert(0, @"%s with self.mode == SCIHTTPImageTransferrerModeNone", __PRETTY_FUNCTION__);
		} break;
	}

}

#pragma mark - NSURLConnectionDelegate

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
	switch (self.mode) {
		case SCIHTTPImageTransferrerModeDownload: {
			[self retryOrPerformDownloadCompletionWithData:nil error:error];
		} break;
		case SCIHTTPImageTransferrerModeUpload: {
			[self retryOrPerformUploadCompletionWithSuccess:NO error:error];
		} break;
		case SCIHTTPImageTransferrerModeNone: {
			NSAssert(0, @"%s with self.mode == SCIHTTPImageTransferrerModeNone", __PRETTY_FUNCTION__);
		} break;
	}
}

#pragma mark - NSURLConnectionDataDelegate

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
	switch (self.mode) {
		case SCIHTTPImageTransferrerModeDownload: {
			[self retryOrPerformDownloadCompletionWithData:[self.downloadData copy] error:nil];
		} break;
		case SCIHTTPImageTransferrerModeUpload: {
			[self retryOrPerformUploadCompletionWithSuccess:YES error:nil];
		} break;
		case SCIHTTPImageTransferrerModeNone: {
			NSAssert(0, @"%s with self.mode == SCIHTTPImageTransferrerModeNone", __PRETTY_FUNCTION__);
		} break;
	}
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
	[self.downloadData appendData:data];
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
	switch (self.mode) {
		case SCIHTTPImageTransferrerModeDownload: {
			NSHTTPURLResponse *httpResponse;
			NSString *contentTypeHeader;
			NSError *error;
			NSDictionary *userInfo;

			assert(connection == self.connection);
			
			httpResponse = (NSHTTPURLResponse *) response;
			assert( [httpResponse isKindOfClass:[NSHTTPURLResponse class]] );
			
			if ((httpResponse.statusCode / 100) != 2) {
				userInfo = @{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"HTTP error %zd", (ssize_t) httpResponse.statusCode]};
				error  = [NSError errorWithDomain:SCIErrorDomainHTTPResponse
											 code:SCIHTTPResponseErrorCodeErrorResponse
										 userInfo:userInfo];
				[self stopDownloadWithError:error];
			} else {
				// -MIMEType strips any parameters, strips leading or trailer whitespace, and lower cases
				// the string, so we can just use -isEqual: on the result.
				contentTypeHeader = [httpResponse MIMEType];
				if (contentTypeHeader == nil) {
					userInfo = @{NSLocalizedDescriptionKey: @"No Content-Type!"};
					error  = [NSError errorWithDomain:SCIErrorDomainHTTPResponse
												 code:SCIHTTPResponseErrorCodeNoContentType
											 userInfo:userInfo];
					[self stopDownloadWithError:error];
				} else if ( ! [contentTypeHeader isEqual:@"image/jpeg"]
						   && ! [contentTypeHeader isEqual:@"image/png"]
						   && ! [contentTypeHeader isEqual:@"image/gif"] ) {
					userInfo = @{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Unsupported Content-Type (%@)", contentTypeHeader]};
					error  = [NSError errorWithDomain:SCIErrorDomainHTTPResponse
												 code:SCIHTTPResponseErrorCodeUnsupportedContentType
											 userInfo:userInfo];
					[self stopDownloadWithError:error];
				}
			}
		} break;
		case SCIHTTPImageTransferrerModeUpload: {
		} break;
		case SCIHTTPImageTransferrerModeNone: {
			NSAssert(0, @"%s with self.mode == SCIHTTPImageTransferrerModeNone", __PRETTY_FUNCTION__);
		} break;
	}
}

// From <https://blackpixel.com/writing/2012/05/caching-and-nsurlconnection.html>
- (NSCachedURLResponse *)connection:(NSURLConnection *)connection willCacheResponse:(NSCachedURLResponse *)cachedResponse {
	
	NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse*)[cachedResponse response];
	
	if (![connection respondsToSelector:@selector(currentRequest)]) {
		// We don't have access to the current request in 10.7 so we can't check the cache policy is to see whether or not to cache the response,
		// so we don't cache the response
		return nil; // don't cache this
	}
	
	// Look up the cache policy used in our request
	if([connection currentRequest].cachePolicy == NSURLRequestUseProtocolCachePolicy) {
		NSDictionary *headers = [httpResponse allHeaderFields];
		NSString *cacheControl = [headers valueForKey:@"Cache-Control"];
		NSString *expires = [headers valueForKey:@"Expires"];
		BOOL cacheControlContainsMaxAgeInformation = NO;
		if ([cacheControl isEqualToString:@"max-age"] || [cacheControl isEqualToString:@"s-maxage"]) {
			cacheControlContainsMaxAgeInformation = YES;
		}
		if(!cacheControlContainsMaxAgeInformation && (expires == nil)) {
			NSLog(@"server does not provide expiration information and we are using NSURLRequestUseProtocolCachePolicy");
			return nil; // don't cache this
		}
	}
	return cachedResponse;
}

@end

