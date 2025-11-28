//
//  SCICallStatistics+SstiCallStatistics.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/30/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICallStatistics+SstiCallStatistics.h"

@implementation SCITransmissionStatistics (SstiStatistics)
+ (SCITransmissionStatistics *)transmissionStatisticsWithSstiStatistics:(SstiStatistics)sstiStatistics
{
	SCITransmissionStatistics *transmissionStatistics = [[SCITransmissionStatistics alloc] init];

	[transmissionStatistics setVideoCode:sstiStatistics.eVideoCodec];
	[transmissionStatistics setVideoWidth:sstiStatistics.nVideoWidth];
	[transmissionStatistics setVideoHeight:sstiStatistics.nVideoHeight];
	[transmissionStatistics setTargetFrameRate:sstiStatistics.nTargetFrameRate];
	[transmissionStatistics setTargetVideoDataRate:sstiStatistics.nTargetVideoDataRate];
	[transmissionStatistics setActualFrameRate:sstiStatistics.fActualFrameRate];
	[transmissionStatistics setActualVideoDataRate:sstiStatistics.nActualVideoDataRate];
	[transmissionStatistics setActualAudioDataRate:sstiStatistics.nActualAudioDataRate];
	
	return transmissionStatistics;
}
@end

@implementation SCICallStatistics (SstiCallStatistics)
+ (SCICallStatistics *)callStatisticsWithSstiCallStatistics:(SstiCallStatistics)sstiCallStatistics
{
	SCICallStatistics *callStatistics = [[SCICallStatistics alloc] init];
	
	[callStatistics setPlaybackStatistics:[SCITransmissionStatistics transmissionStatisticsWithSstiStatistics:sstiCallStatistics.Playback]];
	[callStatistics setRecordStatistics:[SCITransmissionStatistics transmissionStatisticsWithSstiStatistics:sstiCallStatistics.Record]];
	
	[callStatistics setReceivedPacketsLostPercent:sstiCallStatistics.fReceivedPacketsLostPercent];
	[callStatistics setLostPackets:sstiCallStatistics.Playback.totalPacketsLost];
	
	[callStatistics setCallDuration:sstiCallStatistics.dCallDuration];
	[callStatistics setTotalVideoPackets:sstiCallStatistics.Playback.totalPacketsReceived];
	[callStatistics setLostVideoPackets:sstiCallStatistics.Playback.totalPacketsLost];
	
	[callStatistics setVideoPacketQueueEmptyErrors:sstiCallStatistics.videoPacketQueueEmptyErrors];
	[callStatistics setUnknownPayloadTypeErrors:sstiCallStatistics.videoUnknownPayloadTypeErrors];
	
	[callStatistics setOutOfOrderPackets:sstiCallStatistics.un32OutOfOrderPackets];
	[callStatistics setMaxOutOfOrderPackets:sstiCallStatistics.un32MaxOutOfOrderPackets];
	[callStatistics setDuplicatePackets:sstiCallStatistics.un32DuplicatePackets];
	
	[callStatistics setPayloadHeaderErrors:sstiCallStatistics.un32PayloadHeaderErrors];
//	[callStatistics setDiscardedNoBufferPackets:sstiCallStatistics.un32DiscardedPacketsNobuf];
	
	[callStatistics setKeyframesLocallyRequested:sstiCallStatistics.un32KeyframesRequestedVP];
	[callStatistics setMaximumKeyframeLocalWaitTime:sstiCallStatistics.un32KeyframeMaxWaitVP];
	[callStatistics setMinimumKeyframeLocalWaitTime:sstiCallStatistics.un32KeyframeMinWaitVP];
	[callStatistics setAverageKeyframeLocalWaitTime:sstiCallStatistics.un32KeyframeAvgWaitVP];
	[callStatistics setTotalKeyframeLocalWaitTime:sstiCallStatistics.fKeyframeTotalWaitVP];
	
	[callStatistics setKeyframesRemotelyRequested:sstiCallStatistics.un32KeyframesRequestedVR];
	[callStatistics setMaximumKeyframeRemoteWaitTime:sstiCallStatistics.un32KeyframeMaxWaitVR];
	[callStatistics setMinimumKeyframeRemoteWaitTime:sstiCallStatistics.un32KeyframeMinWaitVR];
	[callStatistics setAverageKeyframeRemoteWaitTime:sstiCallStatistics.un32KeyframeAvgWaitVR];
	
	[callStatistics setKeyframesReceived:sstiCallStatistics.un32KeyframesReceived];
	[callStatistics setKeyframesSent:sstiCallStatistics.un32KeyframesSent];
	
	[callStatistics setReadPackets:sstiCallStatistics.videoPacketsRead];
	[callStatistics setErrorConcealmentEvents:sstiCallStatistics.un32ErrorConcealmentEvents];
	
	[callStatistics setIncompletePFramesDiscarded:sstiCallStatistics.un32PFramesDiscardedIncomplete];
	[callStatistics setIncompleteIFramesDiscarded:sstiCallStatistics.un32IFramesDiscardedIncomplete];
	
	[callStatistics setFrameNumberGap:sstiCallStatistics.un32GapsInFrameNumber];
	[callStatistics setFramesReceived:sstiCallStatistics.un32FrameCount];
	[callStatistics setFramesLost:sstiCallStatistics.un32FramesLostFromRcu];
	
	[callStatistics setPartialFramesSent:sstiCallStatistics.countOfPartialFramesSent];
	
	NSMutableArray *mutablePacketPositions = [NSMutableArray arrayWithCapacity:MAX_STATS_PACKET_POSITIONS];
	for (int i = 0; i < MAX_STATS_PACKET_POSITIONS; i++) {
		[mutablePacketPositions addObject:[NSNumber numberWithUnsignedInteger:sstiCallStatistics.aun32PacketPositions[i]]];
	}
	NSArray *packetPositions = [mutablePacketPositions copy];
	[callStatistics setPacketPositions:packetPositions];
	
	return callStatistics;
}
@end
