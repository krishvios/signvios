//
//  VideoCallLocalView.swift
//  ntouch
//
//  Created by Dan Shields on 10/18/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation


@objc(SCIVideoCallLocalView)
class VideoCallLocalView: UIView {
	
	@objc public static let showingVideoChanged = Notification.Name("SCIVideoCallLocalViewShowingVideoChanged")
	@objc public static let videoDimensionsChanged = Notification.Name("SCIVideoCallLocalViewVideoDimensionsChanged")
	
	private var notifyStateChangedToken: NSObjectProtocol? = nil
	private var notifyVideoDimensionsChangedToken: NSObjectProtocol? = nil
	
	private var videoLayer = VideoInputLayer()
	@objc public var videoGravity: AVLayerVideoGravity {
		get { return videoLayer.videoGravity }
		set {
			videoLayer.videoGravity = newValue
			setNeedsLayout()
		}
	}

	func setup() {
		videoLayer.zPosition = -1
		videoLayer.isHidden = true
		layer.addSublayer(videoLayer)
		
		self.accessibilityIdentifier = "LocalViewInactive"
		
		// Subscribe to video layer events
		notifyStateChangedToken = NotificationCenter.default.addObserver(
			forName: VideoInputLayer.stateChanged,
			object: videoLayer,
			queue: OperationQueue.main)
		{ [unowned self] _ in
			if self.videoLayer.state != .inactive {
				self.videoLayer.isHidden = false
			}
			
			// Used for test automation
			switch self.videoLayer.state {
			case .inactive:
				self.accessibilityIdentifier = "LocalViewInactive"
			case .cameraStarting:
				self.accessibilityIdentifier = "LocalViewCameraStarting"
			case .privacy:
				self.accessibilityIdentifier = "LocalViewPrivacy"
			case .video:
				self.accessibilityIdentifier = "LocalViewVideo"
			case .error:
				self.accessibilityIdentifier = "LocalViewError"

			}
			
			NotificationCenter.default.post(name: VideoCallLocalView.showingVideoChanged, object: self)
		}
		
		notifyVideoDimensionsChangedToken = NotificationCenter.default.addObserver(
			forName: VideoInputLayer.videoDimensionsChanged,
			object: videoLayer,
			queue: OperationQueue.main)
		{ [unowned self] _ in
			NotificationCenter.default.post(name: VideoCallLocalView.videoDimensionsChanged, object: self)
			self.invalidateIntrinsicContentSize()
			self.setNeedsLayout()
		}
		
		setNeedsLayout()
		CaptureController.shared.register(videoLayer)
	}
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		setup()
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		setup()
	}
	
	deinit {
		NotificationCenter.default.removeObserver(notifyStateChangedToken as Any)
		NotificationCenter.default.removeObserver(notifyVideoDimensionsChangedToken as Any)
	}
	
	override func layoutSubviews() {
		videoLayer.frame = layer.bounds
	}
	
	override var intrinsicContentSize: CGSize {
		let dimensions = videoLayer.videoDimensions
		return CGSize(width: Int(dimensions.width), height: Int(dimensions.height))
	}
	
	override func didMoveToWindow() {
		videoLayer.isActive = (window != nil)
	}
}
