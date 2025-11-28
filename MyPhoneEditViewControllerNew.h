//
//  MyPhoneEditViewControllerNew.h
//  ntouch
//
//  Created by Kevin Selman on 9/13/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SCIRingGroupInfo.h"

@interface MyPhoneEditViewControllerNew : UIViewController

@property (nonatomic, strong) IBOutlet UILabel *titleLabel;
@property (nonatomic, strong) IBOutlet UILabel *explanationLabel;


@property (nonatomic, strong) NSArray *ringGroupInfos;
@property (nonatomic, strong) IBOutlet UIScrollView *scrollView;
@property (nonatomic, strong) IBOutlet UIButton *callCIRButton;
@property (nonatomic, strong) IBOutlet UIView *groupView;
@property (nonatomic, strong) IBOutlet UIView *phone1View;
@property (nonatomic, strong) IBOutlet UIView *phone2View;
@property (nonatomic, strong) IBOutlet UIView *phone3View;
@property (nonatomic, strong) IBOutlet UIView *phone4View;
@property (nonatomic, strong) IBOutlet UIView *phone5View;
@property (nonatomic, strong) IBOutlet UILabel *ringGroupNumberTitleLabel;
@property (nonatomic, strong) IBOutlet UILabel *ringGroupNumberLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone0NameLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone0StatusLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone1NameLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone1StatusLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone2NameLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone2StatusLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone3NameLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone3StatusLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone4NameLabel;
@property (nonatomic, strong) IBOutlet UILabel *phone4StatusLabel;

@property (nonatomic, strong) IBOutlet UIButton *detailButton0;
@property (nonatomic, strong) IBOutlet UIButton *detailButton1;
@property (nonatomic, strong) IBOutlet UIButton *detailButton2;
@property (nonatomic, strong) IBOutlet UIButton *detailButton3;
@property (nonatomic, strong) IBOutlet UIButton *detailButton4;

@property (nonatomic, strong) IBOutlet UIButton *addButton0;
@property (nonatomic, strong) IBOutlet UIButton *addButton1;
@property (nonatomic, strong) IBOutlet UIButton *addButton2;
@property (nonatomic, strong) IBOutlet UIButton *addButton3;
@property (nonatomic, strong) IBOutlet UIButton *addButton4;

@property (nonatomic, strong) IBOutlet UILabel *button0Label;
@property (nonatomic, strong) IBOutlet UILabel *button1Label;
@property (nonatomic, strong) IBOutlet UILabel *button2Label;
@property (nonatomic, strong) IBOutlet UILabel *button3Label;
@property (nonatomic, strong) IBOutlet UILabel *button4Label;

@property (nonatomic, strong) SCIRingGroupInfo *selectedRingGroupInfo;
@property (nonatomic, assign) BOOL hasAuthenticated;
@property (nonatomic, assign) BOOL isSetupMode;

@end
