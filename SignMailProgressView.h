//
//  SignMailProgressView.h
//  ntouch
//
//  Created by Kevin Selman on 1/24/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SignMailProgressView : UIView

@property (nonatomic, assign) float recordLimitSeconds;
@property (nonatomic, strong) UILabel *statusLabel;

- (void)setProgress:(NSUInteger)seconds;
- (void)layoutViews:(CGRect)frame;
- (void)setEndLabel;
- (void)showRecordLabel:(bool)isRecording;

@end
