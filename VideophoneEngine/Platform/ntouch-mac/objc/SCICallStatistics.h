//
//  SCICallStatistics.h
//  ntouchMac
//
//  Created by Nate Chandler on 3/30/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SCITransmissionStatistics : NSObject
@property (nonatomic, assign, readwrite) int videoCode;			//EstiVideoCodec		eVideoCodec;			//!< Video codec
@property (nonatomic, assign, readwrite) NSUInteger videoWidth;				//int					nVideoWidth;			//!< Width of the video
@property (nonatomic, assign, readwrite) NSUInteger videoHeight;			//int					nVideoHeight;			//!< Height of the video
@property (nonatomic, assign, readwrite) NSUInteger targetFrameRate;		//int          		nTargetFrameRate; 		//!< Frame rate we are trying to achieve.
@property (nonatomic, assign, readwrite) NSUInteger targetVideoDataRate;	//int         		nTargetVideoDataRate;	//!< Video data rate we are trying to achieve.
@property (nonatomic, assign, readwrite) float actualFrameRate;				//float					fActualFrameRate;			//!< Frame rate we are achieving (allowing for a float value)
@property (nonatomic, assign, readwrite) NSUInteger actualVideoDataRate;	//int          		nActualVideoDataRate;	//!< Video data rate we are achieving.
@property (nonatomic, assign, readwrite) NSUInteger actualAudioDataRate;	//int          		nActualAudioDataRate;	//!< Audio data rate we are achieving.
@end

@interface SCICallStatistics : NSObject
@property (nonatomic, strong, readwrite) SCITransmissionStatistics *playbackStatistics;		//SstiStatistics	Playback;						///< Playback Statistics
@property (nonatomic, strong, readwrite) SCITransmissionStatistics *recordStatistics;		//SstiStatistics	Record;							///< Record Statistics

@property (nonatomic, assign, readwrite) float receivedPacketsLostPercent;					//float				fReceivedPacketsLostPercent;	///< Percent received packets lost (allow for fraction)
@property (nonatomic, assign, readwrite) NSUInteger lostPackets;							//uint32_t 		un32PacketsLost;				///< Packets not received but waited for

@property (nonatomic, assign, readwrite) double callDuration;								//double	 		dCallDuration;					///< Call duration (in seconds)
@property (nonatomic, assign, readwrite) NSUInteger totalVideoPackets;						//uint32_t			videoPacketsReceived;				///< Total video packets
@property (nonatomic, assign, readwrite) NSUInteger lostVideoPackets;						//uint32_t			un32NumLostVideoPackets;			///< Number of lost video packets         

@property (nonatomic, assign, readwrite) NSUInteger videoPacketQueueEmptyErrors;			//uint32_t			videoPacketQueueEmptyErrors;		///< Video packet queue full errors
@property (nonatomic, assign, readwrite) NSUInteger unknownPayloadTypeErrors;				//uint32_t			videoUnknownPayloadTypeErrors		///< Unknown payload type errors

@property (nonatomic, assign, readwrite) NSUInteger outOfOrderPackets;						//uint32_t			un32OutOfOrderPackets;				///< Out of order packets           
@property (nonatomic, assign, readwrite) NSUInteger maxOutOfOrderPackets;					//uint32_t			un32MaxOutOfOrderPackets;			///< Maximun out ff order packets        
@property (nonatomic, assign, readwrite) NSUInteger duplicatePackets;						//uint32_t			un32DuplicatePackets;				///< Duplicate packets            

@property (nonatomic, assign, readwrite) NSUInteger payloadHeaderErrors;					//uint32_t			un32PayloadHeaderErrors;			///< Payload header errors         

@property (nonatomic, assign, readwrite) NSUInteger discardedNoBufferPackets;				//uint32_t			un32DiscardedPacketsNobuf;			///< Packets discarded because there weren't enough buffers
@property (nonatomic, assign, readwrite) NSUInteger discardedOutOfOrderPackets;				//uint32_t			un32DiscardedPacketsOoo;			///< Packets discarded because they were too far out of order
@property (nonatomic, assign, readwrite) NSUInteger discardedLatePackets;					//uint32_t			un32DiscardedPacketsLate;			///< Packets discarded because they were too late to be of use

@property (nonatomic, assign, readwrite) NSUInteger keyframesLocallyRequested;				//uint32_t			un32KeyframesRequestedVP;			///< Keyframes requested by local system (of the remote system)

@property (nonatomic, assign, readwrite) NSUInteger maximumKeyframeLocalWaitTime;			//uint32_t 		un32KeyframeMaxWaitVP;			///< The maximum time waited for a keyframe from the remote system (in milliseconds)
@property (nonatomic, assign, readwrite) NSUInteger minimumKeyframeLocalWaitTime;			//uint32_t 		un32KeyframeMinWaitVP;			///< The minimum time waited for a keyframe from the remote system (in milliseconds)
@property (nonatomic, assign, readwrite) NSUInteger averageKeyframeLocalWaitTime;			//uint32_t 		un32KeyframeAvgWaitVP;			///< The average time waited for a keyframe from the remote system (in milliseconds)
@property (nonatomic, assign, readwrite) float		totalKeyframeLocalWaitTime;				//float				fKeyframeTotalWaitVP;			///< The total time spent waiting for keyframes from the remote system (in seconds)

@property (nonatomic, assign, readwrite) NSUInteger keyframesRemotelyRequested;				//uint32_t			un32KeyframesRequestedVR;			///< Keyframes requested by remote system (of the local system)

@property (nonatomic, assign, readwrite) NSUInteger maximumKeyframeRemoteWaitTime;			//uint32_t 		un32KeyframeMaxWaitVR;			///< The maximum time waited for a keyframe from the RCU (in milliseconds)
@property (nonatomic, assign, readwrite) NSUInteger minimumKeyframeRemoteWaitTime;			//uint32_t 		un32KeyframeMinWaitVR;			///< The minimum time waited for a keyframe from the RCU (in milliseconds)
@property (nonatomic, assign, readwrite) NSUInteger averageKeyframeRemoteWaitTime;			//uint32_t 		un32KeyframeAvgWaitVR;			///< The average time waited for a keyframe from the RCU (in milliseconds)

@property (nonatomic, assign, readwrite) NSUInteger keyframesReceived;						//uint32_t			un32KeyframesReceived;				///< Keyframes received from the remote system
@property (nonatomic, assign, readwrite) NSUInteger keyframesSent;							//uint32_t			un32KeyframesSent;					///< Keyframes sent to the remote system

@property (nonatomic, assign, readwrite) NSUInteger readPackets;							//uint32_t			videoPacketsRead;					///< Packets read
@property (nonatomic, assign, readwrite) NSUInteger errorConcealmentEvents;					//uint32_t			un32ErrorConcealmentEvents;			///< Error Concealment Events that have occurred (playback)

@property (nonatomic, assign, readwrite) NSUInteger incompletePFramesDiscarded;				//uint32_t			un32PFramesDiscardedIncomplete;		///< P-frames that have been discarded due to incomplete (playback)
@property (nonatomic, assign, readwrite) NSUInteger incompleteIFramesDiscarded;				//uint32_t			un32IFramesDiscardedIncomplete;		///< I-frames that have been discarded due to incomplete (playback)

@property (nonatomic, assign, readwrite) NSUInteger frameNumberGap;							//uint32_t			un32GapsInFrameNumber;				///< The count of gaps in frame numbers observed (playback)
@property (nonatomic, assign, readwrite) NSUInteger framesReceived;							//uint32_t 		un32FrameCount;					///< The number of frames received and sent to the decoder (playback)
@property (nonatomic, assign, readwrite) NSUInteger framesLost;								//uint32_t 		un32FramesLostFromRcu;			///< The number of frames expected but never deliverd from the RCU

@property (nonatomic, assign, readwrite) NSUInteger partialFramesSent;						//uint32_t			countOfPartialFramesSent;

@property (nonatomic, assign, readwrite) NSArray	*packetPositions;						//uint32_t			aun32PacketPositions[MAX_STATS_PACKET_POSITIONS];   ///< Packet positions
@end
