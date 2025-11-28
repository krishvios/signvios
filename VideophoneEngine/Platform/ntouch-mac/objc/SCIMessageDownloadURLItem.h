//
//  SCIMessageDownloadURLItem.h
//  ntouch
//
//  Created by Kevin Selman on 3/25/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "CstiMessageResponse.h"
#import "stiSVX.h"

@interface SCIMessageDownloadURLItem : NSObject <NSCopying>

@property (nonatomic) NSString *downloadURL;
@property (nonatomic) NSUInteger fileSize;
@property (nonatomic) NSUInteger maxBitRate;
@property (nonatomic) NSUInteger width;
@property (nonatomic) NSUInteger height;
@property (nonatomic) NSString *m_eVideoCodec;
@property (nonatomic) NSString *m_eProfile;
@property (nonatomic) NSString *m_eLevel;

+ (SCIMessageDownloadURLItem *)itemWithCstiMessageDownloadURLItem:(CstiMessageResponse::CstiMessageDownloadURLItem const&)item;

@end
