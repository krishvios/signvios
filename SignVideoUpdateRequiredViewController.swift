//
//  SignVideoUpdateRequiredViewController.swift
//  ntouch
//
//  Created by mmccoy on 7/1/25.
//  Copyright © 2025 Sorenson Communications. All rights reserved.
//

import UIKit

class SignVideoUpdateRequiredViewController: UIViewController, AVPlayerViewControllerDelegate {
	@IBOutlet weak var videoView: UIView!
	@IBOutlet weak var titleLabel: UILabel!
	@IBOutlet weak var bodyLabel: UILabel!
	@IBOutlet weak var updateNowButton: UIButton!
	@IBOutlet weak var callSupportButton: UIButton!
	@IBOutlet weak var emergencyCallButton: UIButton!
	
	@IBOutlet weak var contentStackView: UIStackView!
	
	let playerViewController = AVPlayerViewController()
	var queuePlayer: AVQueuePlayer?
	var playerLooper: AVPlayerLooper?
	var originalPlayerViewBounds: CGRect?
	var observePlayerStatus: NSKeyValueObservation?
	
	@IBOutlet weak var contentStackViewWidthConstraint: NSLayoutConstraint?
	@IBOutlet weak var contentStackViewCenterXConstraint: NSLayoutConstraint?
	
	override func viewDidLoad() {
		super.viewDidLoad()
		titleLabel.text = "update.required.title".localized
		titleLabel.textColor = .white
		titleLabel.lineBreakMode = .byWordWrapping
		titleLabel.numberOfLines = 0
		titleLabel.backgroundColor = .black
		bodyLabel.lineBreakMode = .byWordWrapping
		bodyLabel.numberOfLines = 0
		bodyLabel.backgroundColor = .black
		let mutableAttributedString = NSMutableAttributedString()
		let paragraphStyle = NSMutableParagraphStyle()
		paragraphStyle.lineSpacing = 1.5

		let boldAttribute = [
			NSAttributedString.Key.font: UIFont.boldSystemFont(ofSize: 16),
			NSAttributedString.Key.foregroundColor: UIColor.white
		]

		let regularAttribute = [
			NSAttributedString.Key.font: UIFont.systemFont(ofSize: 16),
			NSAttributedString.Key.foregroundColor: UIColor.white
		]

		let boldAttributedString = NSAttributedString(string: "update.required.body.bold".localized, attributes: boldAttribute)
		let regularAttributedString = NSAttributedString(string: "update.required.body.regular".localized, attributes: regularAttribute)
		mutableAttributedString.append(boldAttributedString)
		mutableAttributedString.append(regularAttributedString)
		mutableAttributedString.addAttribute(.paragraphStyle, value: paragraphStyle, range: NSRange(location: 0, length: mutableAttributedString.length))

		bodyLabel.attributedText = mutableAttributedString
		updateNowButton.setTitle("update.required.now.button".localized, for: .normal)
		callSupportButton.setTitle("update.required.cc.button".localized, for: .normal)
		emergencyCallButton.setTitle("update.required.911.button".localized, for: .normal)
		let urlpath = Bundle.main.path(forResource: "signvideo-animation-v2", ofType: "mp4")
		let url = URL(fileURLWithPath: urlpath!)
		let asset = AVAsset(url: url)
		let item = AVPlayerItem(asset: asset)
		queuePlayer = AVQueuePlayer(playerItem: item)
		playerLooper = AVPlayerLooper(player: queuePlayer!, templateItem: item)
		playerViewController.allowsPictureInPicturePlayback = false
		playerViewController.player = queuePlayer
		playerViewController.delegate = self
		playerViewController.showsPlaybackControls = false
		playerViewController.updatesNowPlayingInfoCenter = true
		playerViewController.updatesNowPlayingInfoCenter = false
		guard let playerView = playerViewController.view else { return }
		originalPlayerViewBounds = playerView.bounds
		setupContentConstraints()
	}
	
	private func setupContentConstraints() {
		contentStackViewWidthConstraint?.priority = UILayoutPriority(999)
		contentStackViewWidthConstraint?.constant = 500 // Initial max width
		contentStackViewCenterXConstraint?.isActive = true
		contentStackViewWidthConstraint?.isActive = true
		overrideStackViewElementMargins()
	}
	
	private func overrideStackViewElementMargins() {
		guard let contentStackView = contentStackView else { return }
		
		// force the stack view to respect its width constraint (particularly in landscape orientation)
		// using compression resistance and content hugging priorities
		contentStackView.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
		contentStackView.setContentHuggingPriority(.defaultHigh, for: .horizontal)
	}
	
	override func viewDidLayoutSubviews() {
		super.viewDidLayoutSubviews()
		updateLayoutForCurrentOrientation()
		layoutVideoPlayer()
	}
	
	private func updateLayoutForCurrentOrientation() {
		let isIPad = UIDevice.current.userInterfaceIdiom == .pad
		let isLandscape = view.bounds.width > view.bounds.height
		
		if isIPad && isLandscape {
			let maxWidth = min(500, view.bounds.width * 0.6)
			contentStackViewWidthConstraint?.constant = maxWidth
		} else {
			let availableWidth = view.safeAreaLayoutGuide.layoutFrame.width - 32
			contentStackViewWidthConstraint?.constant = availableWidth
		}
		view.layoutIfNeeded()
	}
	
	private func layoutVideoPlayer() {
		guard let playerView = playerViewController.view else { return }
		
		playerView.removeFromSuperview()
		
		let containerWidth = videoView.bounds.width
		let containerHeight = videoView.bounds.height
		
		let videoAspectRatio: CGFloat = 2.0 / 3.0
		
		var videoWidth: CGFloat
		var videoHeight: CGFloat
		
		if containerWidth / containerHeight > videoAspectRatio {
			videoHeight = containerHeight
			videoWidth = videoHeight * videoAspectRatio
		} else {
			videoWidth = containerWidth
			videoHeight = videoWidth / videoAspectRatio
		}
		
		let x = (containerWidth - videoWidth) * 0.5
		let y = (containerHeight - videoHeight) * 0.5
		
		let frame = CGRect(x: x, y: y, width: videoWidth, height: videoHeight)
		playerView.frame = frame
		playerView.autoresizingMask = [.flexibleLeftMargin, .flexibleTopMargin, .flexibleRightMargin, .flexibleBottomMargin]
		
		playerViewController.videoGravity = .resizeAspectFill
		
		videoView.addSubview(playerView)
		videoView.sendSubviewToBack(playerView)
		playerViewController.didMove(toParent: self)
		
		if #available(iOS 15.0, *) {
			queuePlayer?.audiovisualBackgroundPlaybackPolicy = .continuesIfPossible
		}
		setupObservers()
		play()
	}
	
	private func setupObservers() {
		NotificationCenter.default.addObserver(self, selector: #selector(pause), name: UIApplication.didEnterBackgroundNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(play), name: UIApplication.willEnterForegroundNotification, object: nil)
	}
	
	@objc private func play() {
		queuePlayer?.play()
	}
	
	@objc private func pause() {
		queuePlayer?.pause()
	}
	
	@IBAction func updateNowButton(_ sender: UIButton) {
		if let url = URL(string: "itms-apps://itunes.apple.com/app/id441554954"), UIApplication.shared.canOpenURL(url) {
			if #available(iOS 10.0, *) {
				UIApplication.shared.open(url, options: [:], completionHandler: nil)
			} else {
				UIApplication.shared.openURL(url)
			}
		}
	}
	
	@IBAction func callSupportButton(_ sender: UIButton) {
		CallController.shared.makeOutgoingCall(to: SCIContactManager.customerServicePhone(), dialSource: .uiButton)
	}
	
	@IBAction func emergencyCallButton(_ sender: UIButton) {
		let alert = UIAlertController.init(title: "update.required.911.alert.title".localized, message: nil, preferredStyle: .actionSheet)
		alert.addAction(UIAlertAction(title: "update.required.911.alert.confirm.button".localized, style: .destructive) { action in
			CallController.shared.makeOutgoingCall(to: SCIContactManager.emergencyPhone(), dialSource: .uiButton)
		})
		
		alert.addAction(UIAlertAction(title: "update.required.911.alert.cancel.button".localized, style: .cancel, handler: nil))
		
		if let popover = alert.popoverPresentationController {
			popover.sourceView = sender;
			popover.sourceRect = sender.bounds;
			popover.permittedArrowDirections = .any;
		}
		
		present(alert, animated: true, completion: nil)
	}
	
	deinit {
		NotificationCenter.default.removeObserver(self, name: UIApplication.didEnterBackgroundNotification, object: nil)
		NotificationCenter.default.removeObserver(self, name: UIApplication.willEnterForegroundNotification, object: nil)
	}
}
