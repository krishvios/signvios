//
//  SignMailPlayerController.swift
//  ntouch
//
//  Created by Daniel Shields on 5/17/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import Foundation
import WatchKit
import WatchConnectivity

class SignMailPlayerController: WKInterfaceController, SignMailDownloadDelegate {
	
	@IBOutlet weak var progressLabel: WKInterfaceLabel!
	@IBOutlet weak var progressIndicator: WKInterfaceLabel!
	
	override func awake(withContext context: Any?) {
		
		let options = context as? [String: Any]
		if let signmailUrl = options?["url"] as? URL,
			let signmailId = options?["id"] as? String
		{
			progressLabel.setText("Downloading...")
			progressIndicator.setText("")

			SignMailDownloadManager.shared.downloadSignMailFrom(signmailUrl,
			                                                    messageId: signmailId,
			                                                    delegate: self)
		}
	}
	
	func downloadProgressed(receivedBytes: Int64, ofTotalBytes totalBytes: Int64) {
		progressIndicator.setText("\(100 * receivedBytes / totalBytes)%")
	}
	
	func downloadDidFinishDownloadingToURL(_ url: URL) {
		progressLabel.setText("Downloaded")
		progressIndicator.setText("")

		presentMediaPlayerController(with: url, options: nil) { (_, _, error) in
			if let error = error {
				print("Ended playing with error: \(error)")
			}

			self.dismiss()
		}
	}
	
	func downloadFailed(withError error: Error?) {
		print("Error downloading signmail: \(String(describing: error))")

		progressLabel.setText("Download Failed")
		progressIndicator.setText("")
	}
}
