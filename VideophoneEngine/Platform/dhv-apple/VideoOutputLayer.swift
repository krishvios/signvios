//
//  VideoOutputLayer.swift
//  ntouch
//
//  Created by Daniel Shields on 2/14/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import AVFoundation

@objc(SCIVideoOutputLayer)
class VideoOutputLayer: AVSampleBufferDisplayLayer {
	@objc public static let stateChanged = Notification.Name("SCIVideoOutputLayerStateChanged")
	@objc public static let videoDimensionsChanged = Notification.Name("SCIVideoOutputVideoDimensionsChanged")
	@objc public static let didLayout = Notification.Name("SCIVideoOutputLayerDidLayout")
	private var notifyFailedToDecodeToken: NSObjectProtocol?

	@objc(SCIVideoOutputLayerState)
	enum State: Int {
		case inactive
		case privacy
		case hold
		case video
		case error
	}

	@objc public var state = State.inactive {
		didSet {
			assertOnQueue(.main)
			if state == oldValue {
				return
			}
			
			NotificationCenter.default.post(name: VideoOutputLayer.stateChanged, object: self)
		}
	}

	@objc public private(set) var videoDimensions: CMVideoDimensions = CMVideoDimensions(width: 0, height: 0) {
		didSet {
			assertOnQueue(.main)
			if videoDimensions.width == oldValue.width && videoDimensions.height == oldValue.height {
				return
			}

			NotificationCenter.default.post(name: VideoOutputLayer.videoDimensionsChanged, object: self)
		}
	}

	override init() {
		super.init()
		setup()
	}

	required init?(coder decoder: NSCoder) {
		super.init(coder: decoder)
		setup()
	}

	// Overriding this method to avoid crashes when setFrame is called in iOS 11. 2/27/2018
	override init(layer: Any) {
		super.init(layer: layer)
	}

	private func setup() {
		if #available(macOS 10.10, *) {
			notifyFailedToDecodeToken = NotificationCenter.default.addObserver(
				forName: .AVSampleBufferDisplayLayerFailedToDecode,
				object: self,
				queue: OperationQueue.main) { [weak self] _ in
					self?.state = .error
			}
		}
	}

	override func layoutSublayers() {
		super.layoutSublayers()
		NotificationCenter.default.post(name: VideoOutputLayer.didLayout, object: self)
	}

	deinit {
		if #available(macOS 10.10, *) {
			NotificationCenter.default.removeObserver(notifyFailedToDecodeToken as Any)
		}
	}

	// MARK: AVSampleBufferDisplayLayer proxy methods
	
	override func enqueue(_ sampleBuffer: CMSampleBuffer) {
		if #available(macOS 10.10, *) {
			if status == .failed {
				print("SCIVideoOutputLayer: Video layer decode failed. Attempting recovery. Error: \(String(describing: error))")
				flush()
			}
		}
		
		guard isReadyForMoreMediaData else {
			return
		}
		
		super.enqueue(sampleBuffer)
		
		let doNotDisplay =
			(CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, createIfNecessary: false) as? [[CFString:Any]])?
				.first?[kCMSampleAttachmentKey_DoNotDisplay] as? Bool ?? false

		if !doNotDisplay {
			var dimensions: CMVideoDimensions?
			if let format = CMSampleBufferGetFormatDescription(sampleBuffer) {
				dimensions = CMVideoFormatDescriptionGetDimensions(format)
			}
			
			if let dimensions = dimensions {
				self.videoDimensions = dimensions
			}

			if self.state != .video {
				self.state = .video
			}
		}
	}
}
