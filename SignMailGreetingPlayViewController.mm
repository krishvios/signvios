//
//  SignMailGreetingPlayViewController.mm
//  ntouch
//
//  Created by Kevin Selman on 6/24/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SignMailGreetingPlayViewController.h"
#import "SignMailGreetingRecordViewController.h"
#import "AppDelegate.h"
#import "Alert.h"
#import "Utilities.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "SCISignMailGreetingType.h"
#import "SCIMessageViewer.h"
#import "SCIMessageViewer+AsynchronousCompletion.h"
#import "MBProgressHUD.h"
#import "stiSVX.h"
#import "CstiItemId.h"
#import "IstiMessageViewer.h"
#import "SignMailProgressView.h"
#import "SCIPersonalGreetingPreferences.h"
#import "UIDevice+Resolutions.h"
#import "ntouch-Swift.h"

#include "stiSVX.h"

@interface SignMailGreetingPlayViewController ()

@property (nonatomic, readonly) SCIVideophoneEngine *engine;
@property (nonatomic, readonly) SCIMessageViewer *viewer;
@property (nonatomic, readonly) NSNotificationCenter *notificationCenter;
@property (nonatomic, strong) NSDictionary *greetingInfo;
@property (nonatomic, strong) IBOutlet UIView *playRecordView;
@property (nonatomic, strong) IBOutlet UILabel *titleLabel;
@property (nonatomic, strong) IBOutlet SCIVideoCallRemoteView *remoteView;
@property (nonatomic, strong) IBOutlet UIButton *playButton;
@property (nonatomic, strong) IBOutlet UILabel *playLabel;
@property (nonatomic, strong) IBOutlet UIButton *recordButton;
@property (nonatomic, strong) IBOutlet UILabel *recordLabel;
@property (nonatomic, strong) IBOutlet UISegmentedControl *greetingTypeSegmentControl;
@property (nonatomic, strong) IBOutlet UITextView *greetingTextView;
@property (nonatomic, strong) SCIPersonalGreetingPreferences *personalGreetingPreferences;

@end

@implementation SignMailGreetingPlayViewController

#define LogCallback(name) SCILog(@"%@", [NSString stringWithCString:#name encoding:[NSString defaultCStringEncoding]]);

static NSString * const SignMailGreetingManagerKeyGreetingInfo = @"greetingInfo";
static NSString * const SignMailGreetingManagerGreetingInfoKeyGreetingViewURLs = @"greetingViewURLs";
static NSString * const SignMailGreetingManagerGreetingInfoKeyMaxRecordSeconds = @"maxRecordSeconds";

#pragma mark - IBActions

- (IBAction)recordButton:(id)sender
{
	// Prevent spamming of record button to open multiple record view controllers. Record button will be re-enabled when view re-appears
	self.recordButton.enabled = NO;
	[self.viewer closeWithCompletion:^(BOOL success) {
		[self showRecordViewController];
	}];
}

- (void)showRecordViewController
{
	SignMailGreetingRecordViewController *signMailGreetingView = [[SignMailGreetingRecordViewController alloc] init];
	signMailGreetingView.selectedGreetingType = self.personalGreetingPreferences.greetingType;
	[self.navigationController pushViewController:signMailGreetingView animated:YES];
}

- (IBAction)playButton:(id)sender
{
	if (self.viewer.state == SCIMessageViewerStatePaused)
	{
		[self.viewer play];
	}
	else if (self.viewer.state == SCIMessageViewerStatePlaying)
	{
		[self.viewer pause];
	}
	else
	{
		[self.viewer loadGreetingWithCompletion:^(BOOL success) {
			if(success)
				SCILog(@"loadGreetingWithCompletion Success");
		}];
	}
}

- (IBAction)greetingTypeChanged:(id)sender
{
	UISegmentedControl *segmentControl = (UISegmentedControl *)sender;
	SCIPersonalGreetingPreferences *newPref = [[SCIVideophoneEngine sharedEngine] personalGreetingPreferences];
	
	switch (segmentControl.selectedSegmentIndex) {
		case 0:
		{
			newPref.greetingType = SCISignMailGreetingTypeDefault;
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"sorenson_greeting" }];
		} break;
		case 1:
		{
			if (newPref.personalType != SCISignMailGreetingTypePersonalVideoAndText &&
				newPref.personalType != SCISignMailGreetingTypePersonalVideoOnly &&
				newPref.personalType != SCISignMailGreetingTypePersonalTextOnly)
			{
				newPref.personalType = SCISignMailGreetingTypePersonalVideoOnly; // Default to Video Only.
				newPref.greetingType = SCISignMailGreetingTypePersonalVideoOnly;
			}
			else
			{
				newPref.greetingType = newPref.personalType;
			}
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"personal_greeting" }];
		} break;
		case 2:
		{
			newPref.greetingType = SCISignMailGreetingTypeNone;
			[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"no_greeting" }];
		} break;
		default:newPref.greetingType = SCISignMailGreetingTypeDefault; break;
	}
	
	// Set selected preference in VPE.
	[[SCIVideophoneEngine sharedEngine] setPersonalGreetingPreferences:newPref];
	self.personalGreetingPreferences = newPref;
	
	// Layout view for preference
	[self layoutViews:self.personalGreetingPreferences];
	
	[self.viewer closeWithCompletion:^(BOOL success) {
		[self loadGreetingInfoWithCompletion:^{
			//
			// BUG 19445: When changing tabs while playing a PSMG, the last frame from the previous
			// video was being drawn over the placeholder image.
			// Wait until close completes before showing placeholder.
			//
			[self showPlaceHolderForGreetingType:newPref.greetingType];
		}];

	}];
}

#pragma mark - Accessors: Convenience

- (SCIVideophoneEngine *)engine
{
	return [SCIVideophoneEngine sharedEngine];
}

- (SCIMessageViewer *)viewer
{
	return [SCIMessageViewer sharedViewer];
}

- (NSNotificationCenter *)notificationCenter
{
	return [NSNotificationCenter defaultCenter];
}

#pragma mark - Notifications

- (void)notifyMessageViewerStateChanged:(NSNotification *)note // SCINotificationMessageViewerStateChanged
{
	SCILog(@"Start");
	NSUInteger state = [[[note userInfo] objectForKey:SCINotificationKeyState] unsignedIntegerValue];
	NSUInteger error = [[[note userInfo] objectForKey:SCINotificationKeyError] unsignedIntegerValue];
	[self filePlayStateChanged:state error:error];
}

- (void)filePlayStateChanged:(int)state error:(int)error {
	SCILog(@"State: %d", state);
	switch (state) {
		case IstiMessageViewer::estiOPENING:
			LogCallback(IstiMessageViewer::estiOPENING);
			[[MBProgressHUD showHUDAddedTo:self.view animated:YES] setLabelText:Localize(@"Loading")];
			break;
		case IstiMessageViewer::estiRELOADING:
			LogCallback(IstiMessageViewer::estiRELOADING); break;
		case IstiMessageViewer::estiPLAY_WHEN_READY:
			LogCallback(IstiMessageViewer::estiPLAY_WHEN_READY);
			[MBProgressHUD hideHUDForView:self.view animated:YES];
			break;
		case IstiMessageViewer::estiPLAYING:
			LogCallback(IstiMessageViewer::estiPLAYING);
			[self.playButton setImage:[UIImage imageNamed:@"icon-pause"] forState:UIControlStateNormal];
			self.playLabel.text = Localize(@"Pause");
			break;
		case IstiMessageViewer::estiPAUSED:
			LogCallback(IstiMessageViewer::estiPAUSED);
			[self.playButton setImage:[UIImage imageNamed:@"icon-play"] forState:UIControlStateNormal];
			self.playLabel.text = Localize(@"Play");
			break;
		case IstiMessageViewer::estiCLOSING:
			LogCallback(IstiMessageViewer::estiCLOSING); break;
		case IstiMessageViewer::estiCLOSED:
			LogCallback(IstiMessageViewer::estiCLOSED);
			[self.playButton setImage:[UIImage imageNamed:@"icon-play"] forState:UIControlStateNormal];
			self.playLabel.text = Localize(@"Play");
			break;
		case IstiMessageViewer::estiERROR:
			LogCallback(IstiMessageViewer::estiERROR);
			[MBProgressHUD hideHUDForView:self.view animated:YES];
			break;
		case IstiMessageViewer::estiVIDEO_END:
			LogCallback(IstiMessageViewer::estiVIDEO_END);
			[self.playButton setImage:[UIImage imageNamed:@"icon-play"] forState:UIControlStateNormal];
			self.playLabel.text = Localize(@"Play");
			break;
		case IstiMessageViewer::estiREQUESTING_GUID:
			LogCallback(IstiMessageViewer::estiREQUESTING_GUID); break;
		case IstiMessageViewer::estiRECORD_CONFIGURE:
			LogCallback(IstiMessageViewer::estiRECORD_CONFIGURE); break;
		case IstiMessageViewer::estiWAITING_TO_RECORD:
			LogCallback(IstiMessageViewer::estiWAITING_TO_RECORD); break;
		case IstiMessageViewer::estiRECORDING:
			LogCallback(IstiMessageViewer::estiRECORDING); break;
		case IstiMessageViewer::estiRECORD_FINISHED:
			LogCallback(IstiMessageViewer::estiRECORD_FINISHED);
			break;
		case IstiMessageViewer::estiUPLOADING:
			LogCallback(IstiMessageViewer::estiUPLOADING); break;
		case IstiMessageViewer::estiUPLOAD_COMPLETE:
			LogCallback(IstiMessageViewer::estiUPLOAD_COMPLETE);
			break;
		case IstiMessageViewer::estiPLAYER_IDLE:
			LogCallback(IstiMessageViewer::estiPLAYER_IDLE); break;
		default:
			SCILog(@"filePlayStateChanged UNKNOWN");
			break;
	}
}

- (void)notifyLoggedIn:(NSNotification *)note // SCINotificationLoggedIn
{
	SCILog(@"Start");
	[self.navigationController popToRootViewControllerAnimated:NO];
}

- (void)applicationWillResignActive:(NSNotification *)notification {
	[self prepareToClose];
}

#pragma mark - View lifecycle

- (void)loadGreetingInfoWithCompletion:(void (^)(void))block;
{
	NSString *greetingText = self.personalGreetingPreferences.greetingText;
	self.greetingTextView.text = greetingText;
	
	[self.viewer fetchGreetingInfoWithCompletion:^(SCISignMailGreetingInfo *info, NSError *error) {
		if (!error)
		{
			NSMutableDictionary *mutableGreetingInfo = [[NSMutableDictionary alloc] init];
			NSArray *greetingViewURLs = [info URLsOfType:self.engine.signMailGreetingPreference];
			int maxRecordSeconds = info.maxRecordingLength;
			
			if (greetingViewURLs)
				mutableGreetingInfo[SignMailGreetingManagerGreetingInfoKeyGreetingViewURLs] = greetingViewURLs;
			
			mutableGreetingInfo[SignMailGreetingManagerGreetingInfoKeyMaxRecordSeconds] = [NSNumber numberWithInt:maxRecordSeconds];
			self.greetingInfo = [mutableGreetingInfo copy];
		}
		else
		{
			Alert(Localize(@"signmail.err.record"), Localize(@"signmail.err.characteristics"));
			self.recordButton.hidden = YES;
		}
		
		if (block)
			block();
	}];
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	
	[UISegmentedControl appearanceWhenContainedInInstancesOfClasses:@[self.class]].tintColor = UIColor.whiteColor;
	
	self.playButton.imageView.contentMode = UIViewContentModeScaleAspectFit;
	self.recordButton.imageView.contentMode = UIViewContentModeScaleAspectFit;

	self.title = Localize(@"Greeting");
	
	if ([self respondsToSelector:@selector(edgesForExtendedLayout)])
        self.edgesForExtendedLayout = UIRectEdgeNone;
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applyTheme)
												 name:Theme.themeDidChangeNotification
											   object:nil];
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	
	self.greetingTextView.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:.6];
	
	[self.notificationCenter removeObserver:self];
	[self.notificationCenter addObserver:self
								selector:@selector(notifyMessageViewerStateChanged:)
									name:SCINotificationMessageViewerStateChanged
								  object:[SCIMessageViewer sharedViewer]];
	
	[self.notificationCenter addObserver:self
								selector:@selector(notifyLoggedIn:)
									name:SCINotificationLoggedIn
								  object:self.engine];
	
	[self.notificationCenter addObserver:self
								selector:@selector(applicationWillResignActive:)
									name:UIApplicationWillResignActiveNotification
								  object:nil];
	
	self.personalGreetingPreferences = [[SCIVideophoneEngine sharedEngine] personalGreetingPreferences];
	[self loadGreetingInfoWithCompletion:^{
		[self layoutViews:self.personalGreetingPreferences];
		[self showPlaceHolderForGreetingType:self.personalGreetingPreferences.greetingType];
		
	}];
	[self applyTheme];
	[AnalyticsManager.shared trackUsage:AnalyticsEventSettings properties:@{ @"description" : @"signmail_greeting" }];
}

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];
	[self.notificationCenter removeObserver:self];

	[self prepareToClose];
}

- (void)applyTheme {
	self.view.backgroundColor = Theme.backgroundColor;
	self.titleLabel.textColor = Theme.textColor;
	self.playButton.tintColor = Theme.tintColor;
	self.playLabel.textColor = Theme.tintColor;
	self.recordButton.tintColor = Theme.tintColor;
	self.recordLabel.textColor = Theme.tintColor;
	self.recordLabel.tintColor = Theme.tintColor;
	self.greetingTypeSegmentControl.tintColor = Theme.tintColor;
}

- (void)prepareToClose {
	// Close player and manually reset button. We're no longer
	// listening to events so it wont happen on its own.
	[self.viewer closeWithCompletion:^(BOOL success) {
		[self.playButton setImage:[UIImage imageNamed:@"icon-play"] forState:UIControlStateNormal];
		self.playLabel.text = Localize(@"Play");
		[self showPlaceHolderForGreetingType:self.personalGreetingPreferences.greetingType];
	}];

}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (void)layoutViews:(SCIPersonalGreetingPreferences *)preferences
{
	UIInterfaceOrientation orientation;
	if (@available(iOS 13.0, *))
	{
		orientation = self.view.window.windowScene.interfaceOrientation;
	}
	else
	{
		orientation = [[UIApplication sharedApplication] statusBarOrientation];
	}
	SCILog(@"GreetingType: %d Orientation: %d", preferences.greetingType, orientation);
	
	SCISignMailGreetingType signMailGreetingType = preferences.greetingType;
	CGSize selfSize = self.view.frame.size;
	CGFloat pad = 10.0f;
	
	self.titleLabel.hidden = NO;
	
	// Position UISegmentedControl
	CGFloat segmentXpos = selfSize.width / 2;
	CGFloat segmentYpos = self.greetingTypeSegmentControl.frame.size.height / 2 + (CGRectGetMaxY(self.titleLabel.frame) + pad);
	self.greetingTypeSegmentControl.center = CGPointMake(segmentXpos, segmentYpos);
	
	// Position PlayRecordView
	self.playRecordView.center = CGPointMake(selfSize.width / 2, selfSize.height - (self.playRecordView.frame.size.height / 2));
	
	// Position RemoteView
	CGRect frame = self.remoteView.frame;
	frame.size.width = selfSize.width;
	frame.size.height = frame.size.width * (3.0 / 4.0);
	self.remoteView.frame = frame;
	CGFloat remoteXpos = selfSize.width / 2;
	CGFloat remoteYpos = CGRectGetMaxY(self.greetingTypeSegmentControl.frame) + pad + (CGRectGetMinY(self.playRecordView.frame) - CGRectGetMaxY(self.greetingTypeSegmentControl.frame)) / 2;
	self.remoteView.center = CGPointMake(remoteXpos, remoteYpos);
	
	// Calculate font size at 8% based on remoteView's height
	CGSize remoteTextViewRowSize = CGSizeMake(self.greetingTextView.frame.size.width, self.remoteView.frame.size.height * 0.08);
	UIFont *remoteTextViewFont = [self fontSizedForAreaSize:remoteTextViewRowSize withString:@"GreetingText" usingFont:[UIFont systemFontOfSize:12]];
	self.greetingTextView.font = remoteTextViewFont;
	
	// Set greetingTextView's height to fit text using font size
	CGFloat textViewHeight = self.greetingTextView.contentSize.height;
	
	textViewHeight = [self textViewHeightForString:self.greetingTextView.text andWidth:self.remoteView.frame.size.width andFont:remoteTextViewFont];
	
	if (textViewHeight > self.remoteView.frame.size.height)
		textViewHeight = self.remoteView.frame.size.height;
	
	switch (signMailGreetingType) {
		case SCISignMailGreetingTypeDefault:
			self.greetingTypeSegmentControl.selectedSegmentIndex = 0;
			self.recordButton.enabled = NO;
			self.playButton.enabled = YES;
			self.greetingTextView.hidden = YES;
			break;
		case SCISignMailGreetingTypePersonalVideoOnly:
		case SCISignMailGreetingTypePersonalVideoAndText:
		{
			self.greetingTypeSegmentControl.selectedSegmentIndex = 1;
			self.recordButton.enabled = YES;
			self.playButton.enabled = YES;
			
			if (signMailGreetingType == SCISignMailGreetingTypePersonalVideoAndText)
				self.greetingTextView.hidden = NO;
			else
				self.greetingTextView.hidden = YES;
			
			CGRect frame = self.greetingTextView.frame;
			frame.size.height = textViewHeight;
			frame.size.width = self.remoteView.frame.size.width;
			frame.origin.x = self.remoteView.frame.origin.x;
			frame.origin.y = (self.remoteView.frame.origin.y + self.remoteView.frame.size.height) - frame.size.height;
			self.greetingTextView.frame = frame;
			
		}	break;
		case SCISignMailGreetingTypePersonalTextOnly:
		{
			self.greetingTypeSegmentControl.selectedSegmentIndex = 1;
			self.recordButton.enabled = YES;
			self.playButton.enabled = NO;
			
			self.greetingTextView.hidden = NO;
			CGRect frame = self.greetingTextView.frame;
			frame.size.height = textViewHeight;
			frame.size.width = self.remoteView.frame.size.width;
			self.greetingTextView.frame = frame;
			self.greetingTextView.center = self.remoteView.center;
			
		} break;
		case SCISignMailGreetingTypeNone:
		{
			self.greetingTypeSegmentControl.selectedSegmentIndex = 2;
			self.recordButton.enabled = NO;
			self.playButton.enabled = NO;
			self.greetingTextView.hidden = YES;
			
		} break;
		default:
		{
			self.greetingTypeSegmentControl.selectedSegmentIndex = 0;
			self.recordButton.enabled = NO;
			self.playButton.enabled = NO;
		} break;
	}
}

#pragma mark - UI Helpers

- (CGFloat)textViewHeightForString:(NSString *)text andWidth:(CGFloat)width andFont:(UIFont *)font
{
	NSMutableAttributedString *attributedString = [[NSMutableAttributedString alloc] initWithString:text];
	[attributedString addAttribute:NSFontAttributeName
							 value:font
							 range:NSMakeRange(0, self.greetingTextView.text.length)];
	
    UITextView *textView = [[UITextView alloc] init];
    [textView setAttributedText:attributedString];
    CGSize size = [textView sizeThatFits:CGSizeMake(width, FLT_MAX)];
    return size.height;
}

- (void)showPlaceHolderForGreetingType:(SCISignMailGreetingType)greetingType
{
	if (greetingType == SCISignMailGreetingTypePersonalVideoAndText ||
		greetingType == SCISignMailGreetingTypePersonalVideoOnly ||
		greetingType == SCISignMailGreetingTypePersonalTextOnly) {
		[self.remoteView showImageUntilVideo:[UIImage imageNamed:@"contact_black.png"]];
	}
	else if (greetingType == SCISignMailGreetingTypeNone) {
		[self.remoteView showImageUntilVideo:[UIImage imageNamed:@"nogreeting_black.png"]];
	}
	else {
		[self.remoteView showImageUntilVideo:[UIImage imageNamed:@"sbug_black.png"]];
	}
}

- (float)scaleToAspectFit:(CGSize)source into:(CGSize)into padding:(float)padding
{
    return into.height / source.height;
}

- (UIFont *)fontSizedForAreaSize:(CGSize)size withString:(NSString*)string usingFont:(UIFont*)font;
{
	NSAttributedString *attributedText = [[NSAttributedString alloc] initWithString:string attributes:@{NSFontAttributeName: font}];
	CGRect rect = [attributedText boundingRectWithSize:CGSizeMake(size.width, 1000.0) options:NSStringDrawingUsesLineFragmentOrigin context:nil];
	CGSize sampleSize = rect.size;
	
	CGFloat scale = [self scaleToAspectFit:sampleSize into:size padding:10];
	return [UIFont systemFontOfSize:scale * font.pointSize];
}

#pragma mark - Rotate Methods

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 SCILog(@"Start");
		 [self layoutViews:self.personalGreetingPreferences];
		 [self showPlaceHolderForGreetingType:self.personalGreetingPreferences.greetingType];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

@end
