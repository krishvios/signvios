//
//  SCICallStatistics.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/30/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICallStatistics.h"

@implementation SCITransmissionStatistics
@synthesize videoCode = mVideoCode;
@synthesize videoWidth = mVideoWidth;
@synthesize videoHeight = mVideoHeight;
@synthesize targetFrameRate = mTargetFrameRate;
@synthesize targetVideoDataRate = mTargetVideoDataRate;
@synthesize actualFrameRate = mActualFrameRate;
@synthesize actualVideoDataRate = mActualVideoDataRate;
@synthesize actualAudioDataRate = mActualAduioDataRate;
@end

@implementation SCICallStatistics
@synthesize playbackStatistics = mPlaybackStatistics;
@synthesize recordStatistics = mRecordStatistics;

@synthesize receivedPacketsLostPercent = mReceivedPacketsLostPercent;
@synthesize lostPackets = mLostPackets;

@synthesize callDuration = mCallDuration;
@synthesize totalVideoPackets = mTotalVideoPackets;
@synthesize lostVideoPackets = mLostVideoPackets;

@synthesize videoPacketQueueEmptyErrors = mVideoPacketQueueFullErrors;
@synthesize unknownPayloadTypeErrors = mUnknownPayloadTypeErrors;

@synthesize outOfOrderPackets = mOutOfOrderPackets;
@synthesize maxOutOfOrderPackets = mMaxOutOfOrderPackets;
@synthesize duplicatePackets = mDuplicatePackets;

@synthesize payloadHeaderErrors = mPayloadHeaderErrors;

@synthesize discardedNoBufferPackets = mDiscardedNoBufferPackets;
@synthesize discardedOutOfOrderPackets = mDiscardedOutOfOrderPackets;
@synthesize discardedLatePackets = mDiscardedLatePackets;

@synthesize keyframesLocallyRequested = mKeyframesLocallyRequested;

@synthesize maximumKeyframeLocalWaitTime = mMaximumKeyframeLocalWaitTime;
@synthesize minimumKeyframeLocalWaitTime = mMinimumKeyframeLocalWaitTime;
@synthesize averageKeyframeLocalWaitTime = mAverageKeyframeLocalWaitTime;
@synthesize totalKeyframeLocalWaitTime = mTotalKeyframeLocalWaitTime;

@synthesize keyframesRemotelyRequested = mKeyframesRemotelyRequested;

@synthesize maximumKeyframeRemoteWaitTime = mMaximumKeyframeRemoteWaitTime;
@synthesize minimumKeyframeRemoteWaitTime = mMinimumKeyframeRemoteWaitTime;
@synthesize averageKeyframeRemoteWaitTime = mAverageKeyframeRemoteWaitTime;

@synthesize keyframesReceived = mKeyframesReceived;
@synthesize keyframesSent = mKeyframesSent;

@synthesize readPackets = mReadPackets;
@synthesize errorConcealmentEvents = mErrorConcealmentEvents;

@synthesize incompletePFramesDiscarded = mIncompletePFramesDiscarded;
@synthesize incompleteIFramesDiscarded = mIncompleteIFramesDiscarded;

@synthesize frameNumberGap = mFrameNumberGap;
@synthesize framesReceived = mFramesReceived;
@synthesize framesLost = mFramesLost;
@synthesize partialFramesSent = mPartialFramesSent;
@synthesize packetPositions = mPacketPositions;
@end
