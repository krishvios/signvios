//
//  GradientUIButton.m
//  ntouch
//
//  Created by Kevin Selman on 10/18/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "GradientUIButton.h"

@implementation GradientUIButton

- (id)initWithFrame:(CGRect)frame
{
    if((self = [super initWithFrame:frame])){
        [self setupView];
    }
	
    return self;
}

- (void)awakeFromNib {
	[super awakeFromNib];
    [self setupView];
}

- (void)setupView
{
	NSArray *gradientColors;
	if (self.tag == 1)
		gradientColors = [self learnMoreColors];
	else
		gradientColors = [self defaultColors];
	
	CGFloat cornerRadius = 7.5;
    self.layer.cornerRadius = cornerRadius;
    self.layer.borderWidth = 1.0;
    self.layer.borderColor = [[UIColor blackColor] colorWithAlphaComponent:0.25].CGColor;
    self.layer.shadowColor = [UIColor blackColor].CGColor;
    self.layer.shadowRadius = 1;
    [self clearHighlightView];
	
    CAGradientLayer *gradient = [CAGradientLayer layer];
    gradient.frame = self.layer.bounds;
    gradient.cornerRadius = cornerRadius;
    gradient.colors = gradientColors ? gradientColors : [self defaultColors];
    float height = gradient.frame.size.height;
    gradient.locations = [NSArray arrayWithObjects:
						  [NSNumber numberWithFloat:0.0f],
						  [NSNumber numberWithFloat:0.2 * 30 / height],
						  [NSNumber numberWithFloat:1.0 - 0.1 * 30 / height],
						  [NSNumber numberWithFloat:1.0f],
						  nil];
	
    gradient.cornerRadius = cornerRadius;
    gradient.borderWidth = 1.0;
    gradient.borderColor = [[UIColor blackColor] colorWithAlphaComponent:0.25].CGColor;
	
	[self.layer insertSublayer:gradient atIndex:0];
}

- (void)highlightView
{
    self.layer.shadowOffset = CGSizeMake(1.0f, 1.0f);
    self.layer.shadowOpacity = 0.25;
}

- (void)clearHighlightView
{
    self.layer.shadowOffset = CGSizeMake(2.0f, 2.0f);
    self.layer.shadowOpacity = 0.5;
}

- (void)setHighlighted:(BOOL)highlighted
{
    if (highlighted) {
        [self highlightView];
    } else {
        [self clearHighlightView];
    }
    [super setHighlighted:highlighted];
}

- (NSArray *)defaultColors
{
	return [NSArray arrayWithObjects:
			(id)[UIColor colorWithWhite:1.0f alpha:1.0f].CGColor,
			(id)[UIColor colorWithWhite:1.0f alpha:0.0f].CGColor,
			(id)[UIColor colorWithWhite:0.0f alpha:0.0f].CGColor,
			(id)[UIColor colorWithWhite:0.0f alpha:0.4f].CGColor,
			nil];
}

- (NSArray *)learnMoreColors
{
	UIColor *lightColor = [UIColor colorWithRed:141.0/255.0 green:174.0/255.0 blue:72.0/255.0 alpha:1.0];
	UIColor *darkColor = [UIColor colorWithRed:25.0/255.0 green:57.0/255.0 blue:3.0/255.0 alpha:1.0];
	
	return [NSArray arrayWithObjects:
			(id)lightColor.CGColor,
			(id)lightColor.CGColor,
			(id)darkColor.CGColor,
			(id)darkColor.CGColor,
			nil];
}

@end