//
//  VideoInputLayer.swift
//  ntouch
//
//  Created by Daniel Shields on 2/14/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//
import Foundation
import AVFoundation

@objc(SCIVideoInputLayer)
class VideoInputLayer: AVCaptureVideoPreviewLayer {
	@objc public static let stateChanged = Notification.Name("SCIVideoInputLayerStateChanged")
	@objc public static let videoDimensionsChanged = Notification.Name("SCIVideoInputVideoDimensionsChanged")

	weak var captureController: CaptureController?
	@objc(active)
	var isActive = true {
		didSet {
			assertOnQueue(.main)
			if isActive != oldValue {
				captureController?.updatePreviewing()
			}
		}
	}

	@objc(SCIVideoInputLayerState)
	enum State: Int {
		case inactive
		case cameraStarting
		case error
		case privacy
		case video
	}

	@objc public var state = State.inactive {
		didSet {
			assertOnQueue(.main)
			if state == oldValue {
				return
			}
			
			NotificationCenter.default.post(name: VideoInputLayer.stateChanged, object: self)
		}
	}

	public var error: Error? = nil {
		didSet {
			assertOnQueue(.main)
			if error != nil {
				state = .error
			}
		}
	}

	@objc public var videoDimensions: CMVideoDimensions = CMVideoDimensions(width: 0, height: 0) {
		didSet {
			assertOnQueue(.main)
			if videoDimensions.width == oldValue.width && videoDimensions.height == oldValue.height {
				return
			}

			NotificationCenter.default.post(name: VideoInputLayer.videoDimensionsChanged, object: self)
		}
	}
}
