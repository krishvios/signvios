
//
//  GeolocationService.mm
//  ntouch
//
//  Created by Bruce Croxall on 3/14/17.
//  Copyright 2017 Sorenson Communications. All rights reserved.
//

#import "GeolocationService.h"

#import "IstiPlatform.h"
#import "stiSVX.h"
#import "SCICall.h"
#import "SCICallCpp.h"

@interface GeolocationService ()

@property (retain) CLLocationManager *locationManager;
@property (retain) NSMutableSet *calls;
@property (assign) GeolocationAvailable availability;

@end

@implementation GeolocationService

- (id)init {
	if (self = [super init]) {
		self.calls = [NSMutableSet new];
	}

	return self;
}

- (void)addCall:(SCICall *)call {
	BOOL wasEmpty = (self.calls.count == 0);
	[self.calls addObject:call];
	
	if (wasEmpty && self.calls.count > 0) {
		self.availability = [self startUpdatingLocation];
	}
	
	[call setGeolocationRequestedWithAvailability:self.availability];
}

- (void)removeCall:(SCICall *)call {
	BOOL wasEmpty = (self.calls.count == 0);
	[self.calls removeObject:call];
	
	if (!wasEmpty && self.calls.count == 0) {
		[self stopUpdatingLocation];
		self.availability = GeolocationAvailable::NotDetermined;
		IstiPlatform::InstanceGet ()->geolocationClear ();
	}
}

- (void)dealloc
{
	if (self.locationManager) {
		self.locationManager.delegate = nil;
		[self.locationManager stopUpdatingLocation];
		self.locationManager = nil;
	}
}

- (void)stop {
	[self stopUpdatingLocation];
	IstiPlatform::InstanceGet ()->geolocationClear ();
	
}

- (GeolocationAvailable)startUpdatingLocation {
	
	// Check if location services are turned on for the device
	if (![CLLocationManager locationServicesEnabled]) {
		SCILog(@"Location services are disabled on the device.");
		return GeolocationAvailable::DisabledOnDevice;
	}
	
	// Check if location services are turned on for the application
	switch ([CLLocationManager authorizationStatus]) {
		case kCLAuthorizationStatusRestricted:
		case kCLAuthorizationStatusDenied:
		case kCLAuthorizationStatusNotDetermined:
			SCILog(@"Location services are not authorized for the application.");
			return GeolocationAvailable::ApplicationNotAuthorized;
			
		default:
			break;
	}
	
	SCILog(@"Starting Location Service");
	
	if (!self.locationManager) {
		self.locationManager = [[CLLocationManager alloc] init];
	}
	self.locationManager.delegate = self;
	self.locationManager.desiredAccuracy = kCLLocationAccuracyBestForNavigation;
	self.locationManager.distanceFilter = 20; // meters
	
	[self.locationManager startUpdatingLocation];
	
	return GeolocationAvailable::Available;
}

- (void)stopUpdatingLocation {
	
	if (self.locationManager) {
		SCILog(@"Stopping Location Service");
		self.locationManager.delegate = nil;
		[self.locationManager stopUpdatingLocation];
	}
}

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray<CLLocation *> *)locations {
	
	[self updateEngine:[locations lastObject]];
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error {
	SCILog(@"Error: %@", [error description]);
}

- (void)updateEngine:(CLLocation *)location {
	
	IstiPlatform::InstanceGet ()->geolocationUpdate (
										[location horizontalAccuracy] >= 0,
										[location coordinate].latitude,
										[location coordinate].longitude,
										[location horizontalAccuracy],
										[location verticalAccuracy] >= 0,
										[location altitude]);
}

@end
