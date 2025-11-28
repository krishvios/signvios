//
//  SCIMessageViewerCpp.h
//  ntouchMac
//
//  Created by Nate Chandler on 5/29/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageViewer.h"

#import "stiDefs.h"
class CstiItemId;
class CstiMessageResponse;
#import "IstiMessageViewer.h"

@interface SCIMessageViewer (Cpp)
- (void)updateStateWithEstiState:(IstiMessageViewer::EState)state;
- (IstiMessageViewer::EState)estiState;
@end