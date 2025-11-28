//
//  SCICoreEvent.h
//  ntouch
//
//  Created by Kevin Selman on 6/18/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum { // in priority order
	coreEventUpdateRequired,
	coreEventPasswordChanged,
	coreEventAddressChanged,
	coreEventShowProviderAgreement,
	coreEventNewVideo,
	coreEventLogMessage,
	coreEventUIAddContact
} CoreEventType;

@interface SCICoreEvent : NSObject <NSCoding>

@property (nonatomic, copy) NSNumber * priority;
@property (nonatomic, copy) NSNumber * type;

@end
