//
//  ExtensionDelegate.swift
//  ntouch
//
//  Created by Daniel Shields on 5/19/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import Foundation
import WatchKit
import WatchConnectivity
import UserNotifications

class ExtensionDelegate: NSObject, WKExtensionDelegate, WCSessionDelegate {
	var connectivityTask: WKWatchConnectivityRefreshBackgroundTask?
	var sentToken: Bool = false
	var deviceToken: Data? {
		didSet {
			invalidateDeviceToken()
		}
	}
	
	func session(_ session: WCSession, activationDidCompleteWith activationState: WCSessionActivationState, error: Error?) {
		if let error = error {
			debugPrint("WatchKit ExtensionDelegate WCSession activation error: \(error.localizedDescription)")
		}
	}

	func sessionReachabilityDidChange(_ session: WCSession) {
		DispatchQueue.main.async {
			self.invalidateDeviceToken()
		}
	}

	func invalidateDeviceToken() {
		if let deviceToken = deviceToken, WCSession.default.activationState == .activated && WCSession.default.isReachable {
			WCSession.default.sendMessage(["watchDeviceToken": deviceToken], replyHandler: nil, errorHandler: { error in
				NSLog("!!! Failed to send watchOS device token: \(error)")
			})

			sentToken = true
			if let connectivityTask = connectivityTask, !WCSession.default.hasContentPending {
				connectivityTask.setTaskCompleted()
			}
		}
	}
	
	// For debugging
	func applicationDidFinishLaunching() {
		WCSession.default.delegate = self
		WCSession.default.activate()
		try! SignMailDownloadManager.shared.clearCache()

		if #available(watchOSApplicationExtension 6.0, *) {
			WKExtension.shared().registerForRemoteNotifications()
			UNUserNotificationCenter.current().requestAuthorization(options: [.sound, .alert], completionHandler: { _, _ in })
		}
	}

	@available(watchOSApplicationExtension 6.0, *)
	func didRegisterForRemoteNotifications(withDeviceToken deviceToken: Data) {
		DispatchQueue.main.async {
			self.deviceToken = deviceToken
		}
	}

	@available(watchOSApplicationExtension 6.0, *)
	func didFailToRegisterForRemoteNotificationsWithError(_ error: Error) {
		NSLog("\(error)")
	}
	
	func applicationDidEnterBackground() {
		SignMailDownloadManager.shared.didEnterBackground()
	}

	func session(_ session: WCSession, didReceiveUserInfo userInfo: [String : Any] = [:]) {
		DispatchQueue.main.async {
			if WCSession.default.hasContentPending && self.sentToken {
				self.connectivityTask?.setTaskCompleted()
			}
		}
	}
	
	func handle(_ backgroundTasks: Set<WKRefreshBackgroundTask>) {
		// We use background tasks to download signmail videos. When a signmail
		// video is done downloading, move it to the temporary directory so that
		// the user can view the video quickly next time they have the app open.
		for task in backgroundTasks {
			switch task {
			case let urlSessionTask as WKURLSessionRefreshBackgroundTask where urlSessionTask.sessionIdentifier == SignMailDownloadManager.sessionIdentifier:
				SignMailDownloadManager.shared.didAwakeForBackgroundTask(urlSessionTask)

			case let connectivityTask as WKWatchConnectivityRefreshBackgroundTask:
				if WCSession.default.activationState == .activated && !WCSession.default.hasContentPending && sentToken {
					connectivityTask.setTaskCompleted()
				} else {
					self.connectivityTask = connectivityTask
				}

			default:
				task.setTaskCompleted()
			}
		}
	}
}
