//
//  SignMailProgressView.m
//  ntouch
//
//  Created by Kevin Selman on 1/24/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SignMailProgressView.h"
#import "Utilities.h"

@interface SignMailProgressView ()

@property (nonatomic, strong) IBOutlet UIProgressView *progressView;
@property (nonatomic, strong) IBOutlet UILabel *startLabel;
@property (nonatomic, strong) IBOutlet UILabel *endLabel;
@property (nonatomic, strong) IBOutlet UILabel *recordingDotLabel;
@property (nonatomic, strong) NSString *recordDotOff;
@property (nonatomic, strong) NSString *recordDotOn;

@end

@implementation SignMailProgressView

const NSString *recordDotOn = @"";


- (id)initWithFrame:(CGRect)frame {
	if ((self = [super initWithFrame:frame])) {
		NSArray *subviewArray = [[NSBundle mainBundle] loadNibNamed:@"SignMailProgressView" owner:self options:nil];
		UIView *mainView = [subviewArray objectAtIndex:0];
		self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		mainView.frame = frame;
		
		// Set default record limit
		self.recordLimitSeconds = 60.0f;
		[self addSubview:mainView];
		
		// U+00B7 Middle dot
		self.recordDotOff = @" ";
		self.recordDotOn  = @"\u00b7";
		
		// Fix alignment of blinking dot on iPhone/iPod.  iPad matches XIB, but on iphone it's too low.
		if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone) {
			CGRect frame = self.recordingDotLabel.frame;
			frame.origin.y -= 3; 
			self.recordingDotLabel.frame = frame;
		}
	}
	return self;
}

-(void) setEndLabel{
    self.endLabel.text = [NSString stringWithFormat:@"%.1i:%.2i", (((int)self.recordLimitSeconds / 60) % 60), (((int)self.recordLimitSeconds) % 60)];
}

- (void)showRecordLabel:(bool)isRecording {
    if (!isRecording) {
        self.recordingDotLabel.hidden = YES;
        self.statusLabel.hidden = YES;
    } else {
        self.recordingDotLabel.hidden = NO;
        self.statusLabel.hidden = NO;
    }
}

- (void)layoutViews:(CGRect)frame {
	self.frame = frame;
	[[self.subviews objectAtIndex:0] setFrame:frame];
	
	// Set end label to max recording time.
	[self setEndLabel];
    
	// Dont display start/end labels when view is small
	self.startLabel.hidden = (frame.size.width < 200) ? YES : NO;
	self.endLabel.hidden = (frame.size.width < 200) ? YES : NO;
}

- (void)setProgress:(NSUInteger)seconds {
	if (seconds == 0)
	{
		[self.progressView setProgress:0.0];
		self.startLabel.text = [NSString stringWithFormat:@"%.2i:%.2i", 0, 0];
	}
	else
	{
		// setProgress:animated: introduced in iOS 5.0, using default operation for older version.
		if([self.progressView respondsToSelector:@selector(setProgress:animated:)])
			[self.progressView setProgress:(float)seconds / self.recordLimitSeconds animated:YES];
		else
			[self.progressView setProgress:(float)seconds / self.recordLimitSeconds];
		
		self.startLabel.text = [NSString stringWithFormat:@"%.2lu:%.2lu", (unsigned long)((seconds / 60) % 60), (unsigned long)((seconds) % 60)];
		
		// Toggle REC dot
		if(seconds % 2 == 0)
			self.recordingDotLabel.text = self.recordDotOn;
		else
			self.recordingDotLabel.text = self.recordDotOff;
	}
	

}

@end
