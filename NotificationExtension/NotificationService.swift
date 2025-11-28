//
//  NotificationService.swift
//  NotificationExtension
//
//  Created by Kevin Selman on 4/25/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import UserNotifications
import MobileCoreServices

class NotificationService: UNNotificationServiceExtension {

	var contentHandler: ((UNNotificationContent) -> Void)?
	var bestAttemptContent: UNMutableNotificationContent?

	override func didReceive(_ request: UNNotificationRequest, withContentHandler contentHandler: @escaping (UNNotificationContent) -> Void) {
		self.contentHandler = contentHandler
		bestAttemptContent = (request.content.mutableCopy() as? UNMutableNotificationContent)
		
		
		// Grab the attachment
		let notificationData = request.content.userInfo["ntouch"] as? [String: String]
		let notificationMessage = self.bestAttemptContent?.body ?? "No message"
		if let urlString = notificationData?["video-url"], let fileUrl = URL(string: urlString) {
			downloadMedia(fromURL: fileUrl) { tmpUrl in
				if let tmpUrl = tmpUrl {
					// Add the attachment to the notification content
					let options = [UNNotificationAttachmentOptionsTypeHintKey: kUTTypeMPEG4]
					if let attachment = try? UNNotificationAttachment(identifier: "", url: tmpUrl, options: options) {
						self.bestAttemptContent?.attachments = [attachment]
					}
				}

				self.contentHandler!(self.bestAttemptContent!)
			}
		}
		else if let urlString = notificationData?["image-url"], let fileUrl = URL(string: urlString) {
			downloadMedia(fromURL: fileUrl) { tmpUrl in
				if let tmpUrl = tmpUrl {
					// Add the attachment to the notification content
					let options = [UNNotificationAttachmentOptionsTypeHintKey: kUTTypeJPEG]
					if let attachment = try? UNNotificationAttachment(identifier: "", url: tmpUrl, options: options) {
						self.bestAttemptContent?.attachments = [attachment]
					}

				}
				
				self.contentHandler!(self.bestAttemptContent!)
			}
		}
		else {
			// Serve the notification content
			self.contentHandler!(self.bestAttemptContent!)
		}
	}
	
	override func serviceExtensionTimeWillExpire() {
		// Called just before the extension will be terminated by the system.
		// Use this as an opportunity to deliver your "best attempt" at modified content, otherwise the original push payload will be used.
		if let contentHandler = contentHandler, let bestAttemptContent = bestAttemptContent {
			contentHandler(bestAttemptContent)
		}
	}

	func downloadMedia(fromURL url: URL, withCompletion completion: @escaping (URL?) -> ())
	{
		URLSession.shared.downloadTask(with: url) { (location, response, error) in
			
			var tmpUrl: URL? = nil
			if let location = location {
				// Move temporary file before returning from the completion handler
				tmpUrl = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
				do {
					try FileManager.default.moveItem(at: location, to: tmpUrl!)
				}
				catch {
					print("Error moving downloaded file to temporary location. \(error)")
				}
			}
			
			completion(tmpUrl)
		}.resume()
	}
}
extension String {
    var localized: String {
        return NSLocalizedString(self, comment:"")
    }
}
/******************** New SignMail JSON

{
  "aps": {
	"alert": {
	 "loc-key" => "SignMail from %@",
	 "loc-args" => ["Glade"],
	},
	"category": "SIGNMAIL_CATEGORY",
	"mutable-content": 1,
	"sound": "default"
  },
  "dialstring": "19323383014",
  "ntouch": {
	"image-url": "http://message-uat.sorensonqa.biz/videocenter/MessageDownload/ImageDownload.aspx?messageID=a7d3ed3f-713b-eb11-9fb4-0050f22a09fa",
	"video-url": "https://message-uat.sorensonqa.biz/files/video-messages/1/9/3/2/3/3/3/8/8/8/8/Video_870f025c-e122-431a-ba46-7e4f2c673d2eH265.mp4?i=0&VideoFileInfoId=58946808"
  }
}

********************/

/******************** Missed Call JSON

{
  "aps": {
	"alert": {
	 "loc-key" => "Missed call from %@",
	 "loc-args" => ["Glade"],
	},
	"category": "MISSED_CALL_CATEGORY",
	"mutable-content": 1,
	"sound": "default"
  },
  "dialstring": "19323338888",
  "ntouch": {
	"image-url": "https://message-uat.sorensonqa.biz/videocenter/MessageDownload/ImageDownload.aspx?messageID=a7d3ed3f-713b-eb11-9fb4-0050f22a09fa",
	"video-url": ""
  }
}

****************/
