//
//  GeolocationService.h
//  ntouch
//
//  Created by Bruce Croxall on 3/14/17.
//  Copyright 2017 Sorenson Communications. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>

@class SCICall;

@interface GeolocationService : NSObject <CLLocationManagerDelegate>

- (void)addCall:(SCICall *)call;
- (void)removeCall:(SCICall *)call;

@end
