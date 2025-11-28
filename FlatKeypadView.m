//
//  FlatKeypadView.m
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2010 - 2015 Sorenson Communications, Inc. -- All rights reserved
//

#import "FlatKeypadView.h"
#import "CGPoint+Additions.h"
#import "CGRect+BNRAdditions.h"
#import "NSString+BNRAdditions.h"
#import <QuartzCore/QuartzCore.h>
#import <AVFoundation/AVFoundation.h>


@interface FlatKeypadView ()

@property (nonatomic, strong) NSMutableArray *buttons;

@end

#define KEYPAD_BKG_COLOR [[UIColor blackColor] colorWithAlphaComponent:0.50];
#define KEYPAD_BKG_COLOR_SELECTED [[UIColor whiteColor] colorWithAlphaComponent:0.80];
#define KEYPAD_MARGIN 10.0
#define KEYPAD_PADDING 10.0


IB_DESIGNABLE
@implementation FlatKeypadView

- (void)awakeFromNib {
	[super awakeFromNib];
	[self initializeAnimation];
	[self initializeButtons];
}

- (id)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self) {
		[self initializeAnimation];
		[self initializeButtons];
	}
	return self;
}

- (void)initializeButtons
{
	self.buttons = [NSMutableArray arrayWithCapacity:12];
	for (FlatKeypadViewButton buttonType = 0; buttonType < 12; ++buttonType) {
	
		UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
		button.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.50];
		button.tag = buttonType;
		button.layer.borderWidth = 1.0;
		button.layer.borderColor = [[UIColor blackColor] colorWithAlphaComponent:0.25].CGColor;
		button.layer.shadowColor = [UIColor blackColor].CGColor;
		button.layer.shadowRadius = 1;
	
		[[button titleLabel] setNumberOfLines:2];
		[button.titleLabel setTextAlignment: NSTextAlignmentCenter];

		[button addTarget:self action:@selector(buttonTouchDown:) forControlEvents:UIControlEventTouchDown];
		[button addTarget:self action:@selector(buttonTouchUpInside:) forControlEvents:UIControlEventTouchUpInside];
		[button addTarget:self action:@selector(buttonTouchUpOutside:) forControlEvents:UIControlEventTouchUpOutside];
		[button addTarget:self action:@selector(buttonTouchCancel:) forControlEvents:UIControlEventTouchCancel];
		
		[self.buttons addObject:button];
		[self addSubview:button];
	}
	
	[self setNeedsLayout];
}

- (void)layoutSubviews
{
	CGRect usableBounds = AVMakeRectWithAspectRatioInsideRect(CGSizeMake(3, 4), self.bounds);
	CGFloat digitFontSize = usableBounds.size.width / 8;
	CGFloat alphaFontSize = usableBounds.size.width / 23;
	CGFloat buttonSize = (usableBounds.size.width - 2 * KEYPAD_MARGIN - 2 * KEYPAD_PADDING) / 3;
	
	for (UIButton *button in self.buttons)
	{
		NSString *digitString = [self digitStringForFlatKeypadViewButton:button.tag];
		NSString *alphaString = [self alphaStringForFlatKeypadViewButton:button.tag];
		NSString *labelText = [NSString stringWithFormat:@"%@\n%@", digitString, alphaString];
		
		// Normal state, first line is the digit, second line is alpha
		NSMutableAttributedString *titleText = [[NSMutableAttributedString alloc] initWithString:labelText];
		[titleText addAttribute:NSFontAttributeName value:[self keypadFontWithSize:digitFontSize] range:NSMakeRange(0, 1)];
		[titleText addAttribute:NSForegroundColorAttributeName value:[UIColor whiteColor] range:NSMakeRange(0, 1)];
		
		if (alphaString.length > 1) {
			[titleText addAttribute:NSFontAttributeName value:[self keypadFontWithSize:alphaFontSize] range:NSMakeRange(2, labelText.length-2)];
			[titleText addAttribute:NSForegroundColorAttributeName value:[UIColor whiteColor] range:NSMakeRange(2, labelText.length-2)];
		}
		
		[button setAttributedTitle:titleText forState:UIControlStateNormal];
		
		// Highlighted state, first line is the digit, second line is alpha
		NSMutableAttributedString *highlightedText = [[NSMutableAttributedString alloc] initWithString:labelText];
		[highlightedText addAttribute:NSFontAttributeName value:[self keypadFontWithSize:digitFontSize]range:NSMakeRange(0, 1)];
		[highlightedText addAttribute:NSForegroundColorAttributeName value:[UIColor blackColor] range:NSMakeRange(0, 1)];
		
		if (alphaString.length > 1) {
			[highlightedText addAttribute:NSFontAttributeName value:[self keypadFontWithSize:alphaFontSize] range:NSMakeRange(2, labelText.length-2)];
			[highlightedText addAttribute:NSForegroundColorAttributeName value:[UIColor blackColor] range:NSMakeRange(2, labelText.length-2)];
		}
		[button setAttributedTitle:highlightedText forState:UIControlStateHighlighted];
		
	}
	
	CGPoint position = CGPointMake(0,KEYPAD_MARGIN + usableBounds.origin.y);
	for (int row = 0; row < 3; row++) {
		position.x = KEYPAD_MARGIN + usableBounds.origin.x;
		
		for (int col = 0; col < 3; col++) {
			
			UIButton *button = [self.buttons objectAtIndex:row * 3 + col + 1];
			button.frame = CGRectMake(position.x, position.y, buttonSize, buttonSize);
			button.layer.cornerRadius = buttonSize / 2;
			
			position.x += buttonSize + KEYPAD_PADDING;
		}
		
		position.y += buttonSize + KEYPAD_PADDING;
	}
	
	position.x = KEYPAD_MARGIN + usableBounds.origin.x;
	
	UIButton *button = [self.buttons objectAtIndex:FlatKeypadViewButtonAsterisk];
	button.frame = CGRectMake(position.x, position.y, buttonSize, buttonSize);
	button.layer.cornerRadius = buttonSize / 2;
	position.x += buttonSize + KEYPAD_PADDING;
	
	button = [self.buttons objectAtIndex:FlatKeypadViewButton0];
	button.frame = CGRectMake(position.x, position.y, buttonSize, buttonSize);
	button.layer.cornerRadius = buttonSize / 2;
	position.x += buttonSize + KEYPAD_PADDING;
	
	button = [self.buttons objectAtIndex:FlatKeypadViewButtonPound];
	button.frame = CGRectMake(position.x, position.y, buttonSize, buttonSize);
	button.layer.cornerRadius = buttonSize / 2;
	position.x += buttonSize + KEYPAD_PADDING;
}


- (UIFont *)keypadFontWithSize:(CGFloat)size {
	
	UIFont *font = [UIFont fontWithName:@"HelveticaNeue" size:size];
	return font ? font : [UIFont systemFontOfSize:size];
}

- (NSString *)digitStringForFlatKeypadViewButton:(FlatKeypadViewButton)buttonType
{
	switch (buttonType) {
		case FlatKeypadViewButton0: return @"0"; break;
		case FlatKeypadViewButton1: return @"1"; break;
		case FlatKeypadViewButton2: return @"2"; break;
		case FlatKeypadViewButton3: return @"3"; break;
		case FlatKeypadViewButton4: return @"4"; break;
		case FlatKeypadViewButton5: return @"5"; break;
		case FlatKeypadViewButton6: return @"6"; break;
		case FlatKeypadViewButton7: return @"7"; break;
		case FlatKeypadViewButton8: return @"8"; break;
		case FlatKeypadViewButton9: return @"9"; break;
		case FlatKeypadViewButtonAsterisk: return @"*"; break;
		case FlatKeypadViewButtonPound: return @"#"; break;
		default: return @" "; break;
	}
}

- (NSString *)alphaStringForFlatKeypadViewButton:(FlatKeypadViewButton)buttonType
{
	switch (buttonType) {
		case FlatKeypadViewButton0: return @"  "; break;
		case FlatKeypadViewButton1: return @"  "; break;
		case FlatKeypadViewButton2: return @"A B C"; break;
		case FlatKeypadViewButton3: return @"D E F"; break;
		case FlatKeypadViewButton4: return @"G H I"; break;
		case FlatKeypadViewButton5: return @"J K L"; break;
		case FlatKeypadViewButton6: return @"M N O"; break;
		case FlatKeypadViewButton7: return @"P Q R S"; break;
		case FlatKeypadViewButton8: return @"T U V"; break;
		case FlatKeypadViewButton9: return @"W X Y Z"; break;
		case FlatKeypadViewButtonAsterisk: return @"  "; break;
		case FlatKeypadViewButtonPound: return @"  "; break;
		default: return @" "; break;
	}
}

- (IBAction)buttonTouchUpInside:(id)sender
{
	UIButton *button = (UIButton *)sender;
	SCILog(@"Dialpad button %d", button.tag);
	button.backgroundColor = KEYPAD_BKG_COLOR;
}
- (IBAction)buttonTouchUpOutside:(id)sender
{
	UIButton *button = (UIButton *)sender;
	button.backgroundColor = KEYPAD_BKG_COLOR;
}
- (IBAction)buttonTouchCancel:(id)sender
{
	UIButton *button = (UIButton *)sender;
	button.backgroundColor = KEYPAD_BKG_COLOR;
}

- (IBAction)buttonTouchDown:(id)sender
{
	UIButton *button = (UIButton *)sender;
	button.backgroundColor = KEYPAD_BKG_COLOR_SELECTED;
}

- (void)initializeAnimation {
	

}

- (void)drawRect:(CGRect)rect
{
 // Custom Drawing
}

- (void)setFrame:(CGRect)frameRect
{
	[super setFrame:frameRect];
	
}


@end

