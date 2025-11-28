//
//  VideoPlayer.swift
//  ntouch Mac
//
//  Created by mmccoy on 4/9/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import AVFoundation

// Subclassing AVPlayer in order to log when a user scrubs the video, as there aren't any notifications/kvo to observe for this
@objc class VideoPlayer: AVPlayer {
	@objc var analyticsEvent = AnalyticsEvent.signMailPlayer
	@objc var scrubberUseReported = false
	@objc override func seek(to time: CMTime, toleranceBefore: CMTime, toleranceAfter: CMTime, completionHandler: @escaping (Bool) -> Void) {
		super.seek(to: time, toleranceBefore: toleranceBefore, toleranceAfter: toleranceAfter, completionHandler: completionHandler)
		if !scrubberUseReported {
			AnalyticsManager.shared.trackUsage(analyticsEvent, properties: ["description" : "scrubber_used"])
			scrubberUseReported = true
		}
	}
}
