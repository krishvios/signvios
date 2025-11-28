//
//  VideoCallRemoteView.swift
//  ntouch
//
//  Created by Daniel Shields on 12/28/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import Foundation

@objc(SCIVideoCallRemoteView)
class VideoCallRemoteView: UIView {

	@objc public static let showingVideoChanged = Notification.Name("SCIVideoCallRemoteViewShowingVideoChanged")
	@objc public static let videoDimensionsChanged = Notification.Name("SCIVideoCallLocalViewVideoDimensionsChanged")

	private var videoLayer = VideoOutputLayer()
	private var imageLayer = CALayer()
	private var notifyStateChangedToken: NSObjectProtocol? = nil
	private var notifyVideoDimensionsChangedToken: NSObjectProtocol? = nil

	private var temporaryView: UIView? = nil {
		didSet {
			if let oldValue = oldValue {
				if oldValue.superview == self {
					oldValue.removeFromSuperview()
				}
			}

			if let temporaryView = temporaryView {
				videoLayer.state = .inactive
				videoLayer.isHidden = true
				imageLayer.isHidden = true
				addSubview(temporaryView)
			}
		}
	}

	@objc public var isShowingVideo: Bool {
		return videoLayer.state == .video
	}

	@objc public var videoDimensions: CMVideoDimensions {
		return videoLayer.videoDimensions
	}

	@objc public var videoGravity: AVLayerVideoGravity {
		get { return videoLayer.videoGravity }
		set {
			videoLayer.videoGravity = newValue
			setNeedsLayout()
		}
	}

	@objc func showImageUntilVideo(_ image: UIImage) {
		if videoLayer.state != .video {
			imageLayer.isHidden = (temporaryView != nil)
			imageLayer.contents = image.cgImage
		}
	}

	@objc func showViewUntilVideo(_ view: UIView) {
		if videoLayer.state != .video {
			temporaryView = view
		}
	}

	func setup() {
		videoLayer.isHidden = true
		videoLayer.zPosition = -2
		imageLayer.isHidden = true
		imageLayer.zPosition = -1
		imageLayer.contentsGravity = .resizeAspect
		layer.addSublayer(videoLayer)
		layer.addSublayer(imageLayer)
		
		self.accessibilityIdentifier = "RemoteViewInactive"
		if #available(iOS 11.0, *) {
			self.accessibilityIgnoresInvertColors = true
		}

		// Subscribe to video layer events
		notifyStateChangedToken = NotificationCenter.default.addObserver(
			forName: VideoOutputLayer.stateChanged,
			object: videoLayer,
			queue: OperationQueue.main)
		{ [unowned self] _ in
			if self.videoLayer.state != .inactive {
				self.temporaryView = nil
				self.imageLayer.isHidden = true
				self.videoLayer.isHidden = false
			}
			
			// Used for test automation
			switch self.videoLayer.state {
			case .inactive:
				self.accessibilityIdentifier = "RemoteViewInactive"
			case .privacy:
				self.accessibilityIdentifier = "RemoteViewPrivacy"
			case .hold:
				self.accessibilityIdentifier = "RemoteViewHold"
			case .video:
				self.accessibilityIdentifier = "RemoteViewVideo"
			case .error:
				self.accessibilityIdentifier = "RemoteViewError"
			}

			NotificationCenter.default.post(name: VideoCallRemoteView.showingVideoChanged, object: self)
		}

		notifyVideoDimensionsChangedToken = NotificationCenter.default.addObserver(
			forName: VideoOutputLayer.videoDimensionsChanged,
			object: videoLayer,
			queue: OperationQueue.main)
		{ [unowned self] _ in
			NotificationCenter.default.post(name: VideoCallLocalView.videoDimensionsChanged, object: self)
			self.invalidateIntrinsicContentSize()
			self.setNeedsLayout()
		}

		setNeedsLayout()
		DisplayController.shared.register(videoLayer)
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
		imageLayer.frame = layer.bounds
	}
	
	override var intrinsicContentSize: CGSize {
		let dimensions = videoLayer.videoDimensions
		return CGSize(width: Int(dimensions.width), height: Int(dimensions.height))
	}
}
