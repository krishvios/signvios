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
class VideoOutputLayer: CALayer {
	@objc public static let stateChanged = Notification.Name("SCIVideoOutputLayerStateChanged")
	@objc public static let videoDimensionsChanged = Notification.Name("SCIVideoOutputVideoDimensionsChanged")
	@objc public static let didLayout = Notification.Name("SCIVideoOutputLayerDidLayout")
	private static let holdImage = CrossPlatformUtilities.image(named: "Hold")!
	private static let privacyImage = CrossPlatformUtilities.image(named: "VideoPrivacy")!
	private var videoLayer = AVSampleBufferDisplayLayer()
	private var notifyFailedToDecodeToken: NSObjectProtocol?
	private var fallbackTimebase: CMTimebase!

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

			contents = nil
			videoLayer.isHidden = true

			// Layer is now displaying nothing, change to display current state
			switch state {
			case .inactive:
				break
			case .privacy:
				contents = VideoOutputLayer.privacyImage
			case .hold:
				contents = VideoOutputLayer.holdImage
			case .video:
				videoLayer.isHidden = false
			case .error:
				// Just display black in case of error.
				break
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

			// Lay out video view if aspect ratio has changed.
			if videoDimensions.width * oldValue.height != videoDimensions.height * oldValue.width || oldValue.width == 0 || oldValue.height == 0 {
				setNeedsLayout()
			}

			NotificationCenter.default.post(name: VideoOutputLayer.videoDimensionsChanged, object: self)
		}
	}

	@objc public var videoGravity: AVLayerVideoGravity = .resizeAspect {
		didSet {
			assertOnQueue(.main)
			if videoGravity == oldValue {
				return
			}

			setNeedsLayout()
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
		self.contentsGravity = .resizeAspect

		videoLayer.videoGravity = .resizeAspectFill
		videoLayer.isHidden = true
		addSublayer(videoLayer)

		var _timebase: CMTimebase?
		CMTimebaseCreateWithSourceClock(
			allocator: kCFAllocatorDefault,
			sourceClock: CMClockGetHostTimeClock(),
			timebaseOut: &_timebase)
		CMTimebaseSetRateAndAnchorTime(_timebase!, rate: 1.0, anchorTime: .zero, immediateSourceTime: .zero)
		fallbackTimebase = _timebase
		videoLayer.controlTimebase = fallbackTimebase

		if #available(macOS 10.10, *) {
			notifyFailedToDecodeToken = NotificationCenter.default.addObserver(
				forName: .AVSampleBufferDisplayLayerFailedToDecode,
				object: videoLayer,
				queue: OperationQueue.main) { [weak self] _ in
					self?.state = .error
			}
		}

		setNeedsLayout()
	}

	override func layoutSublayers() {
		if videoGravity == .resizeAspect && videoDimensions.width != 0 && videoDimensions.height != 0 {
			videoLayer.frame = AVMakeRect(
				aspectRatio: CGSize(
					width: Int(videoDimensions.width),
					height: Int(videoDimensions.height)),
				insideRect: bounds)
		} else {
			videoLayer.frame = bounds
		}

		NotificationCenter.default.post(name: VideoOutputLayer.didLayout, object: self)
	}

	deinit {
		if #available(macOS 10.10, *) {
			NotificationCenter.default.removeObserver(notifyFailedToDecodeToken as Any)
		}
	}

	// MARK: AVSampleBufferDisplayLayer proxy methods

	@available(macOS 10.10, *)
	@objc public var status: AVQueuedSampleBufferRenderingStatus {
		assertOnQueue(.main)
		return videoLayer.status
	}

	@available(macOS 10.10, *)
	@objc public var error: Error? {
		assertOnQueue(.main)
		return videoLayer.error
	}

	@objc public func flushAndRemoveImage() {
		assertOnQueue(.main)
		videoLayer.flushAndRemoveImage()
		if self.state == .video {
			self.state = .inactive
		}
	}

	@objc public func flush() {
		assertOnQueue(.main)
		videoLayer.flush()
	}

	@objc(enqueueSampleBuffer:)
	public func enqueue(_ sampleBuffer: CMSampleBuffer) {
		assertOnQueue(.main)

		if #available(macOS 10.10, *) {
			if videoLayer.status == .failed {
				print("SCIVideoOutputLayer: Video layer decode failed. Attempting recovery. Error: \(String(describing: videoLayer.error))")
				videoLayer.flush()
			}
		}

		guard videoLayer.isReadyForMoreMediaData else {
			return
		}

		videoLayer.enqueue(sampleBuffer)

		let doNotDisplay =
			(CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, createIfNecessary: false) as? [[CFString:Any]])?
				.first?[kCMSampleAttachmentKey_DoNotDisplay] as? Bool ?? false

		if !doNotDisplay {
			var dimensions: CMVideoDimensions?
			if let format = CMSampleBufferGetFormatDescription(sampleBuffer) {
				dimensions = CMVideoFormatDescriptionGetDimensions(format)
			}
			
			let timebase: CMTimebase! = videoLayer.controlTimebase ?? fallbackTimebase

			// Taken from kCMTimebaseVeryLongCFTimeInterval. ~256 years.
			let veryLongInterval: CFTimeInterval = CFTimeInterval(256.0) * 365.0 * 24.0 * 60.0 * 60.0
			let timer = CFRunLoopTimerCreateWithHandler(
				kCFAllocatorDefault,
				CFAbsoluteTime(veryLongInterval),
				veryLongInterval,
				0, 0) { [weak self] timer in
				
				if let timer = timer {
					CFRunLoopTimerInvalidate(timer)
					CMTimebaseRemoveTimer(timebase, timer: timer)
				}
				
				guard let self = self else { return }
				
				if let dimensions = dimensions {
					self.videoDimensions = dimensions
				}

				if self.state != .video && !DisplayController.shared.privacy {
					self.state = .video
					self.setNeedsLayout()
				}
			}

			CFRunLoopAddTimer(CFRunLoopGetMain(), timer!, .defaultMode)

			let presentationTime = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
			CMTimebaseAddTimer(timebase, timer: timer!, runloop: CFRunLoopGetMain())

			if CMTimeCompare(CMTimebaseGetTime(timebase), presentationTime) < 0 {
				CMTimebaseSetTimerNextFireTime(
					timebase, timer: timer!,
					fireTime: presentationTime, flags: 0)
			}
			// If the presentation time is in the past, fire immediately
			else {
				CMTimebaseSetTimerToFireImmediately(timebase, timer: timer!)
			}
		}
	}

	// MARK: Mirroring

	@objc(mirrored) var isMirrored = false {
		didSet {
			assertOnQueue(.main)
			if isMirrored {
				videoLayer.setAffineTransform(CGAffineTransform(a: -1, b: 0, c: 0, d: 1, tx: 0, ty: 0))
			} else {
				videoLayer.setAffineTransform(CGAffineTransform.identity)
			}
		}
	}
}
