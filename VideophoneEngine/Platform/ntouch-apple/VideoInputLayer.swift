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
class VideoInputLayer: CALayer {
	@objc public static let stateChanged = Notification.Name("SCIVideoInputLayerStateChanged")
	@objc public static let videoDimensionsChanged = Notification.Name("SCIVideoInputVideoDimensionsChanged")
	private static let privacyImage = CrossPlatformUtilities.image(named: "VideoPrivacy")!
	private static let cameraFailedImage = CrossPlatformUtilities.image(named: "NoCamera")!
	@objc public var previewLayer: AVCaptureVideoPreviewLayer? = nil {
		didSet {
			assertOnQueue(.main)
			oldValue?.removeFromSuperlayer()
			if let previewLayer = previewLayer {
				previewLayer.videoGravity = .resizeAspectFill
				previewLayer.isHidden = true
				addSublayer(previewLayer)
			}
		}
	}

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

			contents = nil
			previewLayer?.isHidden = true
			previewLayer?.opacity = 1.0

			// Layer is now displaying nothing, change to display current state
			switch state {
			case .inactive:
				break
			case .cameraStarting:
				// FIX: If the preview layer is hidden while the camera finishes
				// starting up, it will appear with a frozen frame.
				previewLayer?.isHidden = false
				previewLayer?.opacity = 0.000000001
			case .error:
				contents = VideoInputLayer.cameraFailedImage
			case .privacy:
				contents = VideoInputLayer.privacyImage
				backgroundColor = #colorLiteral(red: 0.1333333333, green: 0.1333333333, blue: 0.1333333333, alpha: 1)
			case .video:
				previewLayer?.isHidden = false
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

			// Lay out video view if aspect ratio has changed.
			if videoDimensions.width * oldValue.height != videoDimensions.height * oldValue.width || oldValue.width == 0 || oldValue.height == 0 {
				setNeedsLayout()
			}

			NotificationCenter.default.post(name: VideoInputLayer.videoDimensionsChanged, object: self)
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

	deinit {
		isActive = false
#if TARGET_OS_IOS // This causes an internal retain cycle on macOS 12
		captureController?.teardown(self)
#endif
	}

	private func setup() {
		self.contentsGravity = .resizeAspect
		setNeedsLayout()
	}

	override func layoutSublayers() {
		if videoGravity == .resizeAspect && videoDimensions.width != 0 && videoDimensions.height != 0 {
			previewLayer?.frame = AVMakeRect(
				aspectRatio: CGSize(
					width: Int(videoDimensions.width),
					height: Int(videoDimensions.height)),
				insideRect: bounds)
		} else {
			previewLayer?.frame = bounds
		}
	}

	// MARK: Mirroring

	@objc(mirrored) var isMirrored = false {
		didSet {
			assertOnQueue(.main)
			if isMirrored {
				previewLayer?.setAffineTransform(CGAffineTransform(a: -1, b: 0, c: 0, d: 1, tx: 0, ty: 0))
			} else {
				previewLayer?.setAffineTransform(CGAffineTransform.identity)
			}
		}
	}
}
