//
//  SignVideoTeaserViewController.swift
//  ntouch
//
//  Created by mmccoy on 11/24/24.
//  Copyright © 2024 Sorenson Communications. All rights reserved.
//

import UIKit
import AVKit

class SignVideoTeaserViewController: UIViewController, AVPlayerViewControllerDelegate {
	
	@IBOutlet weak var videoView: UIView!
	let playerViewController = AVPlayerViewController()
	var queuePlayer: AVQueuePlayer?
	var playerLooper: AVPlayerLooper?
	@IBOutlet weak var continueButton: UIButton!
	var observePlayerStatus: NSKeyValueObservation?
	
	@objc convenience init(url: URL) {
		self.init()
		let asset = AVAsset(url: url)
		let item = AVPlayerItem(asset: asset)
		queuePlayer = AVQueuePlayer(playerItem: item)
		playerLooper = AVPlayerLooper(player: queuePlayer!, templateItem: item)
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		continueButton.setTitle("Continue to ntouch".localized, for: .normal)
		playerViewController.allowsPictureInPicturePlayback = false
		playerViewController.player = queuePlayer
		playerViewController.delegate = self
		playerViewController.showsPlaybackControls = false
		playerViewController.updatesNowPlayingInfoCenter = false
		guard let playerView = playerViewController.view else { return }
		let playerViewWidth = playerView.bounds.width
		let playerViewHeight = playerView.bounds.height
		let x = (videoView.bounds.width - playerViewWidth) * 0.5
		let y = (videoView.bounds.height - playerViewHeight) * 0.5
		let frame = CGRect(x: x, y: y, width: playerViewWidth, height: playerViewHeight)
		playerView.frame = frame
		playerView.autoresizingMask = [.flexibleLeftMargin, .flexibleTopMargin, .flexibleRightMargin, .flexibleBottomMargin]
		videoView.addSubview(playerView)
		videoView.sendSubviewToBack(playerView)
		playerViewController.didMove(toParent: self)
		
		// Keep it playing regardless until the user dismisses
		if #available(iOS 15.0, *) {
			queuePlayer?.audiovisualBackgroundPlaybackPolicy = .continuesIfPossible
		}
		observePlayerStatus = playerViewController.player?.observe(\.rate, options: [.new], changeHandler: { [weak self] player, _ in
			if (player.rate == 0.0) {
				self?.queuePlayer?.play()
			}
		})
		queuePlayer?.play()
	}
	
	@IBAction func continueButton(_ sender: UIButton) {
		let date = Int(Date().timeIntervalSince1970)
		SCIVideophoneEngine.shared.sendRemoteLogEvent("AppEvent=SignVideoTeaserVideoLastViewedTime=\(date)")
		UserDefaults.standard.set(date, forKey: "kLastTeaserViewTime")
		dismiss(animated: true)
	}
}
