//
//  SignMailGreetingRecordViewController.m
//  ntouch
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

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
#import "SCISignMailGreetingType.h"
#import "UIDevice+Resolutions.h"
#import "ntouch-Swift.h"

#include "stiSVX.h"

@interface SignMailGreetingRecordViewController () {
	CGFloat startYpos;
}

@property (nonatomic, readonly) SCIVideophoneEngine *engine;
@property (nonatomic, readonly) SCIMessageViewer *viewer;
@property (nonatomic, readonly) NSNotificationCenter *notificationCenter;
@property (nonatomic, strong) IBOutlet SignMailProgressView *recordProgressView;
@property (nonatomic, strong) IBOutlet SignMailProgressView *playbackProgressview;
@property (nonatomic, strong) IBOutlet UISegmentedControl *greetingTypeSegmentControl;
@property (nonatomic, strong) NSTimer *recordTimer;
@property (nonatomic, strong) NSDictionary *greetingInfo;
@property (nonatomic, strong) UIAlertController *saveGreetingActionSheet;
@property (nonatomic, assign) CGRect keyboardFrameRect;
@property (nonatomic, assign) BOOL isPlaying;
@property (nonatomic, assign) BOOL isReviewing;
@property (nonatomic, assign) BOOL isRecording;
@property (nonatomic, retain) SCIVideoInputLayer *previewLayer;
@property (nonatomic, strong) IBOutlet UIImageView *selfView;
@property (nonatomic, strong) IBOutlet UITextView *greetingTextView;
@property (nonatomic, strong) IBOutlet UIButton *goButton;
@property (nonatomic, strong) IBOutlet UIButton *stopButton;
@property (nonatomic, strong) IBOutlet SCIVideoCallRemoteView *remoteView;
@property (nonatomic, strong) UIBarButtonItem *saveButton;
@property (nonatomic, assign) BOOL restoreVideoPrivacyOnDisappear;

@end

static NSString * const SignMailGreetingManagerKeyGreetingInfo = @"greetingInfo";
static NSString * const SignMailGreetingManagerGreetingInfoKeyGreetingViewURLs = @"greetingViewURLs";
static NSString * const SignMailGreetingManagerGreetingInfoKeyMaxRecordSeconds = @"maxRecordSeconds";

@implementation SignMailGreetingRecordViewController

#define LogCallback(name) SCILog(@"%@", [NSString stringWithCString:#name encoding:[NSString defaultCStringEncoding]]);
#define kDefaultPreEncodeProcess useDirectMemoryAccess		// the default process for outgoing frames
#define kUseFastestPreEncodeProcess YES						// uses DMA process when in LL orientation and when DeafStar is turned OFF
#define kSignMailGreetingTextLimit 176

NSString * const SignMailPlaceHolderText = Localize(@"Enter text here.");

#pragma mark - IBActions

- (IBAction)goButtonClicked:(id)sender
{
	self.goButton.enabled = NO;
	self.navigationItem.hidesBackButton = YES;
	self.greetingTypeSegmentControl.enabled = NO;
	
	[self.greetingTextView resignFirstResponder];
	
	switch (self.selectedGreetingType) {
		case SCISignMailGreetingTypeNone: {
			//Do nothing.
		} break;
			
		case SCISignMailGreetingTypeDefault: {
			// Do nothing.
		} break;
		case SCISignMailGreetingTypePersonalVideoOnly: {
			// Bug 19064: Delete recording handled when user selects Record Again and on willDismiss
			[self.viewer startRecordingGreetingWithCompletion:^(BOOL success) {
				if(success) {
					self.isRecording = YES;
					
				}
				else {
					self.isRecording = NO;
					self.greetingTypeSegmentControl.enabled = YES;
					self.navigationItem.hidesBackButton = NO;
				}
			}];
		} break;
		case SCISignMailGreetingTypePersonalVideoAndText: {
			if ([self.greetingTextView.text isEqualToString:SignMailPlaceHolderText]) {
				Alert(Localize(@"No Greeting Text"), Localize(@"signmail.err.missingText"));
				self.goButton.enabled = YES;
				self.navigationItem.hidesBackButton = NO;
				self.greetingTypeSegmentControl.enabled = YES;
				if (![self.greetingTextView isFirstResponder])
					[self.greetingTextView becomeFirstResponder];
				
				return;
			}
			
			// Bug 19064: Delete recording handled when user selects Record Again and on willDismiss
			[self.viewer startRecordingGreetingWithCompletion:^(BOOL success) {
				if(success) {
					self.isRecording = YES;
					
				}
				else {
					self.isRecording = NO;
					self.greetingTypeSegmentControl.enabled = YES;
					self.navigationItem.hidesBackButton = NO;
				}
			}];
		} break;
		case SCISignMailGreetingTypePersonalTextOnly:
		{
			[self saveButtonClicked:nil];
		} break;
		case SCISignMailGreetingTypeUnknown: break;
		default: break;
	}
}

- (IBAction)saveButtonClicked:(id)sender
{
	if (self.greetingTextView.text.length > kSignMailGreetingTextLimit) {
		self.goButton.enabled = YES;
		self.greetingTypeSegmentControl.enabled = YES;
		self.navigationItem.hidesBackButton = NO;
		Alert(Localize(@"Greeting Text Too Long"), Localize(@"signmail.err.textTooLong"));
		return;
	} else if ([self isPlaceHolderText]) {
		self.goButton.enabled = YES;
		self.greetingTypeSegmentControl.enabled = YES;
		self.navigationItem.hidesBackButton = NO;
		Alert(Localize(@"No Greeting Text"), Localize(@"signmail.err.missingText"));
		return;
	}
	
	NSString *textToSave = [self isPlaceHolderText] ?  @"" : self.greetingTextView.text;
	SCIPersonalGreetingPreferences *pref = [[SCIVideophoneEngine sharedEngine] personalGreetingPreferences];
	pref.greetingText = textToSave;
	[[SCIVideophoneEngine sharedEngine] setPersonalGreetingPreferences:pref];
	
	[self.navigationController popViewControllerAnimated:YES];
}

- (IBAction)stopButtonClicked:(id)sender
{
	if (self.viewer.state == SCIMessageViewerStateRecording) {
		[self.viewer stopRecordingWithCompletion:^(BOOL success) {
			if(success) {
				SCILog(@"stopRecordingWithCompletion Success");
			}
		}];
	}
	else
		[self.viewer closeWithCompletion:^(BOOL success) {
			if(success)
				SCILog(@"closeWithCompletion Success");
		}];
}

- (IBAction)greetingTypeChanged:(id)sender
{
	UISegmentedControl *segmentControl = (UISegmentedControl *)sender;
	switch (segmentControl.selectedSegmentIndex) {
		case 0:
			self.greetingTextView.hidden = YES;
			self.selectedGreetingType = SCISignMailGreetingTypePersonalVideoOnly;
			break;
		case 1:
			self.greetingTextView.hidden = NO;
			self.selectedGreetingType = SCISignMailGreetingTypePersonalVideoAndText;
			break;
		case 2:
			self.greetingTextView.hidden = NO;
			self.selectedGreetingType = SCISignMailGreetingTypePersonalTextOnly;
			break;
		default:
			self.greetingTextView.hidden = YES;
			self.selectedGreetingType = SCISignMailGreetingTypePersonalVideoOnly;
			break;
	}
	
	SCIPersonalGreetingPreferences *pref = [[SCIVideophoneEngine sharedEngine] personalGreetingPreferences];
	pref.personalType = self.selectedGreetingType;
	pref.greetingType = self.selectedGreetingType;
	[[SCIVideophoneEngine sharedEngine] setPersonalGreetingPreferences:pref];
	
	[self layoutViews:self.selectedGreetingType];
}

- (void)displayPlaceHolderText {
	self.greetingTextView.text = SignMailPlaceHolderText;
	self.greetingTextView.textColor = [UIColor lightTextColor];
}

- (BOOL)isPlaceHolderText {
	if([self.greetingTextView.text isEqualToString:SignMailPlaceHolderText])
		return YES;
	else
		return NO;
}

- (void)displaySaveGreetingActionSheet {
	self.saveGreetingActionSheet = [UIAlertController alertControllerWithTitle:Localize(@"Greeting Recorded") message:nil preferredStyle:UIAlertControllerStyleActionSheet];
	
	[self.saveGreetingActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Save") style:UIAlertActionStyleDestructive handler:^(UIAlertAction *Action) {
		[[MBProgressHUD showHUDAddedTo:self.view animated:YES] setLabelText:Localize(@"Uploading")];
		
		SCIPersonalGreetingPreferences *pref = [[SCIVideophoneEngine sharedEngine] personalGreetingPreferences];
		
		if (self.greetingTypeSegmentControl.selectedSegmentIndex == 0) { // Video Only
			pref.greetingType = SCISignMailGreetingTypePersonalVideoOnly;
			pref.personalType = SCISignMailGreetingTypePersonalVideoOnly;
		}
		else {
			pref.personalType = SCISignMailGreetingTypePersonalVideoAndText;
			pref.greetingType = SCISignMailGreetingTypePersonalVideoAndText;
			
			if (![self isPlaceHolderText]) {
				pref.greetingText = self.greetingTextView.text;
			}
		}
		
		[[SCIVideophoneEngine sharedEngine] setPersonalGreetingPreferences:pref];
		
		[self.viewer uploadRecordedGreetingWithCompletion:^(BOOL success)
		 {
			 [MBProgressHUD hideHUDForView:self.view animated:YES];
			 if(success)
			 {
				 SCILog(@"uploadRecordedGreetingWithCompletion Success");
				 [self.navigationController popViewControllerAnimated:YES];
			 }
			 else
				 Alert(Localize(@"Upload Error"), Localize(@"signmail.err.upload"));
		 }];
	}]];
	[self.saveGreetingActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Record Again") style:UIAlertActionStyleDefault handler:^(UIAlertAction *Action) {
		// Bug 19064: Delete previous recording now, instead of when recording starts
		[self.viewer deleteRecordedGreetingWithCompletion:^(BOOL success) {
			self.goButton.enabled = YES;
			[self.recordProgressView setProgress:0.0];
			self.greetingTypeSegmentControl.enabled = YES;
			self.greetingTextView.editable = YES;
			self.goButton.hidden = NO;
		}];
	}]];
	[self.saveGreetingActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Preview") style:UIAlertActionStyleDefault handler:^(UIAlertAction *Action) {
		self.navigationItem.hidesBackButton = YES;
		self.isReviewing = YES;
		self.greetingTypeSegmentControl.enabled = NO;
		self.greetingTextView.editable = NO;
		[self.viewer playFromBeginning];
	}]];
	[self.saveGreetingActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleDefault handler:^(UIAlertAction *Action) {
		[self.viewer deleteRecordedGreetingWithCompletion:^(BOOL success) {
			[self.navigationController popViewControllerAnimated:YES];
		}];
	}]];
	
	if(iPadB) {
		[self.saveGreetingActionSheet addAction:[UIAlertAction actionWithTitle:Localize(@"Cancel") style:UIAlertActionStyleCancel handler:^(UIAlertAction *Action) {
			[self displaySaveGreetingActionSheet];
		}]];
	}
	
	UIPopoverPresentationController *popover = self.saveGreetingActionSheet.popoverPresentationController;
	if (popover)
	{
		popover.sourceView = self.selfView;
		popover.sourceRect = CGRectMake(self.selfView.bounds.size.width / 2, self.selfView.bounds.size.height, 1, 1);
		popover.permittedArrowDirections = UIPopoverArrowDirectionLeft;
	}
	
	[self presentViewController:self.saveGreetingActionSheet animated:YES completion:nil];
}

#pragma mark - Notifications

- (void)notifyUserAccountInfoUpdated:(NSNotification *)note // SCINotificationUserAccountInfoUpdated
{
	SCILog(@"Start");
}

- (void)notifyMessageViewerStateChanged:(NSNotification *)note // SCINotificationMessageViewerStateChanged
{
	NSUInteger state = [[[note userInfo] objectForKey:SCINotificationKeyState] unsignedIntegerValue];
	NSUInteger error = [[[note userInfo] objectForKey:SCINotificationKeyError] unsignedIntegerValue];
	[self filePlayStateChanged:state error:error];
}

- (void)filePlayStateChanged:(NSUInteger)state error:(NSUInteger)error {
	switch (state) {
		case IstiMessageViewer::estiOPENING:
			LogCallback(IstiMessageViewer::estiOPENING);
			self.remoteView.hidden = NO;
			break;
		case IstiMessageViewer::estiRELOADING:
			LogCallback(IstiMessageViewer::estiRELOADING); break;
		case IstiMessageViewer::estiPLAY_WHEN_READY:
			LogCallback(IstiMessageViewer::estiPLAY_WHEN_READY);
			break;
		case IstiMessageViewer::estiPLAYING:
			LogCallback(IstiMessageViewer::estiPLAYING);
			self.remoteView.hidden = NO;
			self.isPlaying = YES;
			self.greetingTextView.hidden = (self.greetingTypeSegmentControl.selectedSegmentIndex == 0);
            NSTimeInterval playbackDuration;
            if ([self.viewer getDuration:&playbackDuration currentTime:nil]) {
                self.playbackProgressview.recordLimitSeconds = (int)playbackDuration;
                [self.playbackProgressview setEndLabel];
            }
            [self startRecordTimer];
			
			break;
		case IstiMessageViewer::estiPAUSED:
			LogCallback(IstiMessageViewer::estiPAUSED);
            [self stopRecordTimer];
			break;
		case IstiMessageViewer::estiCLOSING:
			LogCallback(IstiMessageViewer::estiCLOSING); break;
		case IstiMessageViewer::estiCLOSED:
			LogCallback(IstiMessageViewer::estiCLOSED);
			self.remoteView.hidden = YES;
			break;
		case IstiMessageViewer::estiERROR:
			LogCallback(IstiMessageViewer::estiERROR);
			[MBProgressHUD hideHUDForView:self.view animated:YES];
			self.navigationItem.hidesBackButton = NO;
			self.greetingTypeSegmentControl.enabled = YES;
			self.isRecording = NO;
			self.isPlaying = NO;
			break;
		case IstiMessageViewer::estiVIDEO_END: {
			LogCallback(IstiMessageViewer::estiVIDEO_END);
			self.remoteView.hidden = YES;
			self.isPlaying = NO;
			if(self.isReviewing) {
				[self displaySaveGreetingActionSheet];
				self.isReviewing = NO;
			}
		} break;
		case IstiMessageViewer::estiREQUESTING_GUID:
			LogCallback(IstiMessageViewer::estiREQUESTING_GUID); break;
		case IstiMessageViewer::estiRECORD_CONFIGURE:
			LogCallback(IstiMessageViewer::estiRECORD_CONFIGURE); break;
		case IstiMessageViewer::estiWAITING_TO_RECORD:
			LogCallback(IstiMessageViewer::estiWAITING_TO_RECORD);
			self.navigationController.navigationItem.backBarButtonItem.enabled = NO;
			self.navigationItem.rightBarButtonItem = nil;
			self.greetingTextView.hidden = (self.greetingTypeSegmentControl.selectedSegmentIndex == 0);
			self.greetingTextView.editable = NO;
			self.remoteView.hidden = YES;
			[self.recordProgressView setProgress:0.0];
			[self.selfView bringSubviewToFront:self.recordProgressView];
			self.recordProgressView.hidden = NO;
			break;
		case IstiMessageViewer::estiRECORDING:
			LogCallback(IstiMessageViewer::estiRECORDING);
			self.navigationController.navigationItem.backBarButtonItem.enabled = YES;
			self.goButton.hidden = YES;
			self.greetingTypeSegmentControl.enabled = NO;
			[self startRecordTimer];
			break;
		case IstiMessageViewer::estiRECORD_FINISHED:
			LogCallback(IstiMessageViewer::estiRECORD_FINISHED);
			//
			// BUG 19424: User was able to interupt recording by swiping back, then cancelling the swipe.
			// Disallow going back until recording has finished or error.
			//
			self.navigationItem.hidesBackButton = NO;
			
			//
			// BUG 19383: Only handle this event if capture has been started.
			// This event may be the result of a previously cancelled capture session.
			//
			
			if (self.isRecording) {
				self.isRecording = NO;
				[self stopRecordTimer];
				self.goButton.hidden = YES;
				self.stopButton.hidden = YES;
				self.greetingTextView.editable = YES;
				self.greetingTextView.hidden = (self.greetingTypeSegmentControl.selectedSegmentIndex == 0);
				self.greetingTypeSegmentControl.enabled = YES;
				[self displaySaveGreetingActionSheet];
			}
			break;
		case IstiMessageViewer::estiUPLOADING:
			LogCallback(IstiMessageViewer::estiUPLOADING);
			break;
		case IstiMessageViewer::estiUPLOAD_COMPLETE:
			LogCallback(IstiMessageViewer::estiUPLOAD_COMPLETE);
			break;
		case IstiMessageViewer::estiPLAYER_IDLE:
			LogCallback(IstiMessageViewer::estiPLAYER_IDLE); break;
		default:
			SCILog(@"State: %d  Error: %d", state, error);
			break;
	}
}

- (void)notifyLoggedIn:(NSNotification *)note // SCINotificationLoggedIn
{
	SCILog(@"Start");
	[self.navigationController popViewControllerAnimated:NO];
}

- (void)notifyCallIncoming:(NSNotification *)note // SCINotificationCallIncoming
{
	[self viewInterupted];
}

- (void)keyboardWillChangeFrame:(NSNotification *)notification
{
	NSDictionary *userInfo = [notification userInfo];
	CGRect keyboardFrameScreen = [[userInfo objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
	CGRect keyboardFrame = [self.view convertRect:keyboardFrameScreen fromCoordinateSpace:UIScreen.mainScreen.coordinateSpace];
	
	CFTimeInterval duration = [[userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
	UIViewAnimationCurve curve = (UIViewAnimationCurve)[[userInfo objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue];
	
	// calculate the area of own frame that is covered by keyboard
	self.keyboardFrameRect = keyboardFrame;
	
	// Animate view up for Keyboard
	[UIView beginAnimations:@"SlideUp" context:nil];
	[UIView setAnimationDuration:duration];
	[UIView setAnimationCurve:curve];
	
	[self layoutGreetingTextView];
	
	[UIView commitAnimations];
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

- (void)startRecordTimer {
	if(!self.recordTimer)
		self.recordTimer = [NSTimer scheduledTimerWithTimeInterval:0.25 target:self selector:@selector(updateRecording) userInfo:nil repeats:YES];
}

- (void)stopRecordTimer {
	[self.recordTimer invalidate];
	self.recordTimer = nil;
}

- (void)updateRecording {
	
	NSTimeInterval currentPosition = [self recordingPosition];
	NSTimeInterval maxLength = [self maxRecordingLength];
	
	SCILog(@"SignMail Greeting recorded %0.2f of %0.2f seconds.", currentPosition, maxLength);
	
	if (self.viewer.state == SCIMessageViewerStateRecording) {
		[self.recordProgressView setProgress:currentPosition];
		if (currentPosition >= 1.0)
		{
			self.stopButton.hidden = NO;
		}
		if (currentPosition >= maxLength) {
			[self stopButtonClicked:nil];
			[self.recordTimer invalidate];
			self.recordTimer = nil;
		}
	}
	else if (self.viewer.state == SCIMessageViewerStatePlaying) {
		[self.playbackProgressview setProgress:currentPosition];
		if (currentPosition >= maxLength) {
			[self stopButtonClicked:nil];
			[self.recordTimer invalidate];
			self.recordTimer = nil;
		}
	}
	else {
		[self.recordProgressView setProgress:0.0];
	}
}

- (NSTimeInterval)maxRecordingLength
{
	NSTimeInterval res = 0.0;
	
	if (self.greetingInfo) {
		NSNumber *maxRecordSecondsNumber = self.greetingInfo[SignMailGreetingManagerGreetingInfoKeyMaxRecordSeconds];
		int maxRecordSeconds = [maxRecordSecondsNumber intValue];
		res = (NSTimeInterval)maxRecordSeconds;
	}
	
	return res;
}

- (NSTimeInterval)recordingPosition
{
	NSTimeInterval res = 0.0;
	NSTimeInterval currentTime = 0.0;
	__unused BOOL success = [self.viewer getDuration:NULL currentTime:&currentTime];
	res = currentTime;
	return res;
}

- (NSTimeInterval)recordedLength
{
	NSTimeInterval res = 0.0;
	NSTimeInterval duration = 0.0;
	__unused BOOL success = [self.viewer getDuration:&duration currentTime:NULL];
	res = duration;
	return res;
}

#pragma mark - TextField delegate

- (BOOL)textViewShouldBeginEditing:(UITextView *)textView
{
	if([self isPlaceHolderText]) {
		textView.text = @"";
		textView.textColor = Theme.tintColor;
	}
	return YES;
}

- (BOOL)textViewShouldEndEditing:(UITextView *)textView
{
	[textView resignFirstResponder];
	return YES;
}

- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text
{
	// If the user hit the "Done"/return key, hide the keyboard.
	if ([text isEqualToString:@"\n"]) {
		[textView resignFirstResponder];
		return NO;
	}
	
	// If the text is empty it is likely that backspace was pressed which should be allowed even if the text is too long.
	if ([text isEqual:@""]) {
		return YES;
	}
	
	BOOL allowed = ((textView.text.length + text.length) <= kSignMailGreetingTextLimit);
	
	if (allowed) {
		self.navigationItem.rightBarButtonItem = self.saveButton;
		
		// create final version of textView after the current text has been inserted
		NSMutableString *updatedText = [[NSMutableString alloc] initWithString:textView.text];
		[updatedText insertString:text atIndex:range.location];
		
		NSRange replaceRange = range, endRange = range;
		
		if (text.length > 1) {
			// handle paste
			replaceRange.length = text.length;
		}
		else {
			// handle normal typing
			replaceRange.length = 2;
			replaceRange.location -= 1;
		}
		
		
		NSUInteger replaceCount = 0;
		if (updatedText.length) {
			replaceCount = [updatedText replaceOccurrencesOfString:@"\n" withString:@" " options:NSCaseInsensitiveSearch range:replaceRange];
		}
		
		if (replaceCount > 0) {
			// update the textView's text
			textView.text = updatedText;
			
			// leave cursor at end of inserted text
			endRange.location += text.length;
			textView.selectedRange = endRange;
			allowed = NO;
		}
		
	}
	
	return allowed;
}

- (void)textViewDidEndEditing:(UITextView *)textView
{
	if (!textView.text.length) {
		[self displayPlaceHolderText];
	}
}

-(void)textViewDidChange:(UITextView *)textView
{
	self.saveButton.enabled = textView.text.length ? YES : NO;
	
	[UIView beginAnimations:@"GreetingTextViewSize" context:nil];
	[self layoutGreetingTextView];
	[UIView commitAnimations];
}

#pragma mark - View Lifecycle

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)customizeButtonBorder:(UIButton *)button {
	button.layer.cornerRadius = 10.0f;
	button.layer.borderColor = [UIColor whiteColor].CGColor;
	button.layer.borderWidth = 1.0;
}

- (void)viewDidLayoutSubviews {
	[self layoutViews:self.selectedGreetingType];
}

- (void)layoutViews:(SCISignMailGreetingType)greetingType
{
	[self customizeButtonBorder:self.goButton];
	[self customizeButtonBorder:self.stopButton];
	
	CGSize selfSize = self.view.frame.size;
	CGFloat pad = 10.0f;
	
	// Position Start Stop buttons
	self.goButton.center = CGPointMake(selfSize.width / 2, selfSize.height - (self.goButton.frame.size.height / 2) - pad);
	self.stopButton.center = self.goButton.center;

	// Position UISegmentedControl
	CGFloat segmentXpos = selfSize.width / 2;
	CGFloat segmentYpos = CGRectGetMinY(self.goButton.frame) - (self.greetingTypeSegmentControl.frame.size.height / 2) - pad;
	self.greetingTypeSegmentControl.center = CGPointMake(segmentXpos, segmentYpos);

	// Position SelfView and RemoteView
	CGRect frame = self.remoteView.frame;
	frame.size.width = selfSize.width;
	frame.size.height = selfSize.width * (3.0 / 4.0);
	self.remoteView.frame = frame;
	self.remoteView.center = CGPointMake(selfSize.width / 2, (self.greetingTypeSegmentControl.frame.origin.y - pad) / 2);
	self.selfView.frame = frame;
	self.selfView.center = CGPointMake(selfSize.width / 2, (self.greetingTypeSegmentControl.frame.origin.y - pad) / 2);
	
	[self layoutGreetingTextView];
	
	// Position and size progressView
	if(self.recordProgressView) {
		CGRect selfFrame = CGRectMake(0,0, self.selfView.frame.size.width, 46);
		self.recordProgressView.recordLimitSeconds = [self maxRecordingLength];
		[self.recordProgressView layoutViews:selfFrame];
	}
    
    // Position and size playbackProgressView
    if(self.playbackProgressview) {
        CGRect remoteFrame = CGRectMake(0, 0, self.remoteView.frame.size.width, 46);
        self.playbackProgressview.recordLimitSeconds = [self maxRecordingLength];
        [self.playbackProgressview layoutViews:remoteFrame];
    }
	
	switch (greetingType) {
		case SCISignMailGreetingTypeDefault:
		{
			[self.goButton setTitle:Localize(@"Record") forState:UIControlStateNormal];
			self.greetingTextView.hidden = YES;
			self.selfView.hidden = NO;
			self.remoteView.hidden = YES;
			
		}	break;
		case SCISignMailGreetingTypePersonalVideoAndText:
		{
			[self.goButton setTitle:Localize(@"Record") forState:UIControlStateNormal];
			self.greetingTypeSegmentControl.selectedSegmentIndex = 1;
			// Bug 19052: If reviewing, leave remoteView visible.
			self.selfView.hidden = self.isReviewing ? YES : NO;
			self.remoteView.hidden = self.isReviewing || self.isPlaying ? NO : YES;
			
			self.greetingTextView.hidden = NO;
			self.greetingTextView.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.6];
			
		}	break;
		case SCISignMailGreetingTypePersonalVideoOnly:
		{
			[self.goButton setTitle:Localize(@"Record") forState:UIControlStateNormal];
			self.greetingTextView.hidden = YES;
			self.greetingTypeSegmentControl.selectedSegmentIndex = 0;
			// Bug 19052: If reviewing, leave remoteView visible.
			self.selfView.hidden = self.isReviewing ? YES : NO;
			self.remoteView.hidden = self.isReviewing ? NO : YES;
			
		}	break;
		case SCISignMailGreetingTypePersonalTextOnly:
		{
			[self.goButton setTitle:Localize(@"Save") forState:UIControlStateNormal];
			self.greetingTypeSegmentControl.selectedSegmentIndex = 2;
			self.selfView.hidden = YES;
			// Bug 19149: Hide remoteView to prevent last frame of previewed video from appearing behind Text Only Greeting.
			self.remoteView.hidden = YES;
			
			self.greetingTextView.hidden = NO;
			self.greetingTextView.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.2];
			
		} break;
		case SCISignMailGreetingTypeNone:
		{
			self.greetingTextView.hidden = YES;
			
		}	break;
		default: break;
	}

	[self layoutPreviewLayer];
}

- (void)layoutPreviewLayer {
	CMVideoDimensions videoDimensions = SCICaptureController.shared.targetDimensions;
	CGSize videoSize = CGSizeMake(videoDimensions.width, videoDimensions.height);
	self.previewLayer.frame = AVMakeRectWithAspectRatioInsideRect(videoSize, self.selfView.bounds);
}

- (void)layoutGreetingTextView {
	
	CGRect frame = self.remoteView.frame;
	frame.origin.x = 0;
	self.greetingTextView.frame = frame;
	
	// Determine font size to fit in a row that is 8% of selfView height
	CGSize rowSize = CGSizeMake(self.greetingTextView.frame.size.width, self.selfView.frame.size.height * 0.08);
	UIFont *font = [self fontSizedForAreaSize:rowSize withString:@"GreetingText" usingFont:[UIFont systemFontOfSize:12]];
	self.greetingTextView.font = font;
	
	NSString *text = self.greetingTextView.text.length > 0 ? self.greetingTextView.text : SignMailPlaceHolderText;
	CGFloat textHeight = [self textViewHeightForString:text
											  andWidth:self.remoteView.frame.size.width
											   andFont:font];
	
	if (textHeight > self.selfView.frame.size.height)
		textHeight = self.selfView.frame.size.height;
	
	frame = self.greetingTextView.frame;
	
	switch (self.selectedGreetingType) {
		case SCISignMailGreetingTypePersonalTextOnly:
			frame.size.height = textHeight;
			self.greetingTextView.frame = frame;
			self.greetingTextView.center = self.selfView.center;
			break;
			
		case SCISignMailGreetingTypePersonalVideoAndText:
			frame.size.height = textHeight;
			frame.origin.x = self.selfView.frame.origin.x;
			frame.origin.y = (self.remoteView.frame.origin.y + self.remoteView.frame.size.height) - frame.size.height;
			self.greetingTextView.frame = frame;
			break;
			
		default:
			break;
	}
	
	// If the keyboard is up, we may need to move the greeting text such that the greeting text is visible.
	// Move the greeting text view such that the bottom is above the keyboard.
	if (self.greetingTextView.isFirstResponder) {
		frame = self.greetingTextView.frame;
		frame.origin.y = fmin(CGRectGetMinY(frame), CGRectGetMinY(self.keyboardFrameRect) - CGRectGetHeight(frame));
		self.greetingTextView.frame = frame;
	}
}

- (CGFloat)textViewHeightForString:(NSString *)text andWidth:(CGFloat)width andFont:(UIFont *)font
{
	NSMutableAttributedString *attributedString = [[NSMutableAttributedString alloc] initWithString:text];
	[attributedString addAttribute:NSFontAttributeName
							 value:font
							 range:NSMakeRange(0, text.length)];
	
    UITextView *textView = [[UITextView alloc] init];
    [textView setAttributedText:attributedString];
    CGSize size = [textView sizeThatFits:CGSizeMake(width, FLT_MAX)];
    return size.height;
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

- (void)loadGreetingInfo {
	NSString *greetingText = [[SCIVideophoneEngine sharedEngine] personalGreetingText];
	self.greetingTextView.text = greetingText;
	self.greetingTextView.textColor = Theme.tintColor;
	
	[MBProgressHUD showHUDAddedTo:self.view animated:YES];
	[self.viewer fetchGreetingInfoWithCompletion:^(SCISignMailGreetingInfo *info, NSError *error) {
		[MBProgressHUD hideHUDForView:self.view animated:YES];
		if (!error) {
			NSMutableDictionary *mutableGreetingInfo = [[NSMutableDictionary alloc] init];
			
			NSArray *greetingViewURLs = [info URLsOfType:self.engine.signMailGreetingPreference];
			int maxRecordSeconds = info.maxRecordingLength;
			if (greetingViewURLs) mutableGreetingInfo[SignMailGreetingManagerGreetingInfoKeyGreetingViewURLs] = greetingViewURLs;
			mutableGreetingInfo[SignMailGreetingManagerGreetingInfoKeyMaxRecordSeconds] = [NSNumber numberWithInt:maxRecordSeconds];
			
			self.greetingInfo = [mutableGreetingInfo copy];
			
			[self layoutViews:self.selectedGreetingType];
			
			
		} else {
			Alert(Localize(@"signmail.err.record"), Localize(@"signmail.err.characteristics"));
		}
	}];
}

-(void)dismissKeyboard {
	[self.greetingTextView resignFirstResponder];
}

- (void)viewDidLoad
{
	SCILog(@"Start");
    [super viewDidLoad];
	
	[UISegmentedControl appearanceWhenContainedInInstancesOfClasses:@[self.class]].tintColor = UIColor.whiteColor;
	
	self.title = Localize(@"Record Greeting");
	
	self.greetingTextView.backgroundColor = [[UIColor darkGrayColor] colorWithAlphaComponent:0.6];
	[self displayPlaceHolderText];

	UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(dismissKeyboard)];
	tapRecognizer.cancelsTouchesInView = NO;
	[self.view addGestureRecognizer:tapRecognizer];
	
	//self.selfView.backgroundColor = [UIColor clearColor];
	
	// Add navigationBar button to save -- Keyboard hides existing save button.
	self.saveButton = [[UIBarButtonItem alloc] initWithTitle:Localize(@"Save") style:UIBarButtonItemStyleDone target:self action:@selector(saveButtonClicked:)];
	
	// Position ProgressView over SelfView
	CGRect selfFrame = CGRectMake(0,0, self.selfView.frame.size.width, 46);
	self.recordProgressView = [[SignMailProgressView alloc] initWithFrame:selfFrame];
	self.recordProgressView.recordLimitSeconds = [self maxRecordingLength];
	[self.recordProgressView layoutViews:selfFrame];
	self.recordProgressView.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.6];
	[self.selfView addSubview:self.recordProgressView];
	self.recordProgressView.hidden = YES;
    
    // Position ProgressView over RemoteView
    CGRect remoteFrame = CGRectMake(0,0, self.remoteView.frame.size.width, 46);
    self.playbackProgressview = [[SignMailProgressView alloc] initWithFrame:remoteFrame];
    [self.playbackProgressview showRecordLabel:false];
    [self.recordProgressView layoutViews:remoteFrame];
    self.recordProgressView.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.6];
    [self.remoteView addSubview:self.playbackProgressview];
    self.playbackProgressview.hidden = NO;
    
	
	if ([self respondsToSelector:@selector(edgesForExtendedLayout)])
        self.edgesForExtendedLayout = UIRectEdgeNone;

	self.previewLayer = [[SCIVideoInputLayer alloc] init];
	[SCICaptureController.shared registerVideoLayer:self.previewLayer];
	[self.selfView.layer addSublayer:self.previewLayer];
	[self layoutPreviewLayer];
}

- (void)didEnterBackground
{
	if(self.isRecording)
	{
		[self stopButtonClicked:nil];
	}
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	
	self.view.backgroundColor = Theme.backgroundColor;
	self.greetingTypeSegmentControl.tintColor = Theme.tintColor;
	
	[self.notificationCenter removeObserver:self];
	
	[self.notificationCenter addObserver:self selector:@selector(notifyUserAccountInfoUpdated:)
									name:SCINotificationUserAccountInfoUpdated object:self.engine];
	
	[self.notificationCenter addObserver:self selector:@selector(notifyMessageViewerStateChanged:)
									name:SCINotificationMessageViewerStateChanged object:self.viewer];
	
	[self.greetingTypeSegmentControl addTarget:self action:@selector(greetingTypeChanged:)
							  forControlEvents:UIControlEventValueChanged];
	
	[self.notificationCenter addObserver:self selector:@selector(keyboardWillChangeFrame:)
									name:UIKeyboardWillChangeFrameNotification object:nil];
	
	[self.notificationCenter addObserver:self selector:@selector(notifyLoggedIn:)
									name:SCINotificationLoggedIn object:self.engine];
	
	[self.notificationCenter addObserver:self selector:@selector(notifyCallIncoming:)
									name:SCINotificationCallIncoming object:self.engine];

	[self.notificationCenter addObserver:self selector:@selector(notifyTargetCaptureDimensionsChanged:)
									name:SCICaptureController.targetDimensionsChanged object:SCICaptureController.shared];
	
	[self.notificationCenter addObserver:self selector:@selector(didEnterBackground)
									name:UIApplicationDidEnterBackgroundNotification object:nil];
	
	[self loadGreetingInfo];

	self.previewLayer.active = YES;
	
	// Always use the front facing camera for PSMG.
	SCICaptureController.shared.captureDevice = [AVCaptureDevice
												 defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera
												 mediaType:AVMediaTypeVideo
												 position:AVCaptureDevicePositionFront];
	SCICaptureController.shared.targetDimensions = { 640, 480 };
}

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];
	UIApplication.sharedApplication.idleTimerDisabled = YES;
	startYpos = self.view.frame.origin.y;
	
	NSError *error = nil;
	[DevicePermissions checkAndAlertVideoPermissionsFromView:self error:&error withCompletion:^(BOOL granted) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCICaptureController.captureExclusiveChanged object:nil
														  userInfo:@{ SCICaptureController.captureExclusiveAllowPrivacyKey : [NSNumber numberWithBool:NO] }];
	}];

}

- (void)viewWillDisappear:(BOOL)animated
{
	SCILog(@"Start");
	[super viewWillDisappear:animated];
	UIApplication.sharedApplication.idleTimerDisabled = NO;
	[self viewInterupted];
}

- (void)viewDidDisappear:(BOOL)animated
{
	self.previewLayer.active = NO;
	[super viewDidDisappear:animated];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:SCICaptureController.captureExclusiveChanged
														object:nil
													  userInfo:@{ SCICaptureController.captureExclusiveAllowPrivacyKey : [NSNumber numberWithBool:YES] }];
}

- (void)viewInterupted
{
	[self.notificationCenter removeObserver:self];
	
	if (self.saveGreetingActionSheet) {
		[self dismissViewControllerAnimated:NO completion:nil];
	}
	
	if (self.recordTimer) {
		[self.recordTimer invalidate];
		self.recordTimer = nil;
	}

	if (self.viewer.state == SCIMessageViewerStateWaitingToRecord ||
		self.viewer.state == SCIMessageViewerStateRecording)
	{
		[self.viewer stopRecordingWithCompletion:^(BOOL success) {
			[self.viewer deleteRecordedGreetingWithCompletion:^(BOOL success) {
				[self.navigationController popViewControllerAnimated:NO];
			}];
		}];
	}
	else if (self.viewer.state == SCIMessageViewerStatePlaying) {
		[self.viewer stopPlayingCountdownForRecordingGreetingWithCompletion:^(BOOL) {
			[self.viewer closeWithCompletion:^(BOOL success) {
				[self.navigationController popViewControllerAnimated:NO];
			}];
		}];
	}
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	if ([self.greetingTextView isFirstResponder]) {
		[self.greetingTextView resignFirstResponder];
	}
	
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 [UIView beginAnimations:@"Rotate" context:nil];
		 [self layoutViews:self.selectedGreetingType];
		 [UIView commitAnimations];
		 
		 [self layoutPreviewLayer];
		 
		 if(self.isPlaying)
		 {
			 self.remoteView.hidden = NO;
		 }
	 } completion:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 
	 }];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

- (void)notifyTargetCaptureDimensionsChanged:(NSNotification *)note
{
	[self layoutPreviewLayer];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

@end
