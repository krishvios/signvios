//
//  NotificationController.h
//   WatchKit Extension
//
//  Created by Kevin Selman on 5/15/15.
//  Copyright (c) 2015 Sorenson Communications. All rights reserved.
//

#import <WatchKit/WatchKit.h>
#import <Foundation/Foundation.h>
#import <WatchConnectivity/WatchConnectivity.h>
#import <UserNotifications/UserNotifications.h>
#import <MobileCoreServices/MobileCoreServices.h>

@interface NotificationController : WKUserNotificationInterfaceController <NSURLSessionDelegate>

@end

extern NSString * const WatchKitContactNameRequestKey;
extern NSString * const WatchKitContactNameDataKey;
extern NSString * const WatchKitContactPhotoRequestKey;
extern NSString * const WatchKitContactPhotoDataKey;

