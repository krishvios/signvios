//
//  SCINotificationManager.swift
//  ntouch
//
//  Created by Kevin Selman on 4/11/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation


@objc class SCINotificationManager : NSObject {
	
	@objc public var networkingCount = 0
	@objc static let shared = SCINotificationManager()
	let defaults = SCIDefaults.shared
	
	enum Category: String {
		case incomingCall = "INCOMING_CALL_CATEGORY"
		case missedCall = "MISSED_CALL_CATEGORY"
		case signMail = "SIGNMAIL_CATEGORY"
		case memberRemoved = "MYPHONE_MEMBER_REMOVED"
	}
	
	enum Action: String {
		case answer = "ANSWER_IDENTIFIER"
		case decline = "DECLINE_IDENTIFIER"
		case returnCall = "RETURN_CALL_IDENTIFIER"
		case returnSignMailCall = "RETURN_SIGNMAIL_CALL_IDENTIFIER"
		case viewSignMail = "VIEW_SIGNMAIL_IDENTIFIER"
	}
	
	public override init() {
		super.init()

		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyAggregateCallListChanged),
			name: .SCINotificationAggregateCallListItemsChanged,
			object: nil)
	}
	
	@objc public func didStartNetworking() {
		networkingCount += 1
		DispatchQueue.main.async {
			UIApplication.shared.isNetworkActivityIndicatorVisible = true
		}
	}
	
	@objc public func didStopNetworking() {
		if networkingCount > 0 {
			networkingCount -= 1
		}
		
		DispatchQueue.main.async {
			UIApplication.shared.isNetworkActivityIndicatorVisible = (self.networkingCount > 0)
		}
	}
	
	@objc public var signMailBadgeCount: Int {
		let catID = SCIVideophoneEngine.shared.signMailMessageCategoryId
		return SCIVideophoneEngine.shared.messages(forCategoryId: catID)?.lazy.filter { !$0.viewed }.count ?? 0
	}
	
	@objc public var recentCallsBadgeCount: Int {
		let lastCleared = SCIVideophoneEngine.shared.lastMissedViewTime
		if lastCleared != Date(timeIntervalSince1970: 0) {
			return Int(SCICallListManager.shared.missedCalls(after: lastCleared))
		}
		return 0
	}
	
	@objc public func updateAppBadge() {
		UIApplication.shared.applicationIconBadgeNumber = Int(signMailBadgeCount + recentCallsBadgeCount)
	}
	
	@objc public func requestPermissions() {
		UNUserNotificationCenter.current().requestAuthorization(options: [.sound, .badge, .alert]) { (success, error) in
			if !success {
				print("Could not post notifications: \(String(describing: error))")
			}
		}
	}
	
	@objc public func registerInteractiveNotifications() {
		// Incoming Call interactive notification
		let incomingCallCategory = UNNotificationCategory(
			identifier: Category.incomingCall.rawValue,
			actions: [
				UNNotificationAction(identifier: Action.answer.rawValue, title: "Answer Call".localized, options: .foreground),
				UNNotificationAction(identifier: Action.decline.rawValue, title: "Decline Call".localized, options: [])
			],
			intentIdentifiers: [])
		
		// Missed Call interactive local notification.
		let missedCallCategory = UNNotificationCategory(
			identifier: Category.missedCall.rawValue,
			actions: [
				UNNotificationAction(identifier: Action.returnCall.rawValue, title: "Return Call".localized, options: .foreground)
			],
			intentIdentifiers: [])
		
		// SignMail notification.
		let signMailCategory = UNNotificationCategory(
			identifier: Category.signMail.rawValue,
			actions: [
				UNNotificationAction(identifier: Action.viewSignMail.rawValue, title: "View".localized, options: .foreground),
				UNNotificationAction(identifier: Action.returnSignMailCall.rawValue, title: "Return Call".localized, options: .foreground)
			],
			intentIdentifiers: [])
		
		UNUserNotificationCenter.current().setNotificationCategories([incomingCallCategory, missedCallCategory, signMailCategory])
		UNUserNotificationCenter.current().delegate = self
	}
	
	@objc func notifyAggregateCallListChanged(note: NSNotification) { // SCINotificationAggregateCallListItemsChanged
		let application = UIApplication.shared
		if application.applicationState != .active {
			updateAppBadge()
		}
	}
}

extension SCINotificationManager: UNUserNotificationCenterDelegate {
	
	func userNotificationCenter(_ center: UNUserNotificationCenter, willPresent notification: UNNotification, withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
		// We're in the app so most notifications won't be displayed
		let category = Category(rawValue: notification.request.content.categoryIdentifier)

		if category == .missedCall || category == .memberRemoved {
			completionHandler([.alert, .sound])
		}
		else {
			// TODO: Some notifications should still produce a sound in app.
			completionHandler([])
		}
	}
	
	
	func userNotificationCenter(_ center: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
		
		let appDelegate = UIApplication.shared.delegate as! AppDelegate
		let tabBarController = appDelegate.tabBarController as? MainTabBarController
		let category = Category(rawValue: response.notification.request.content.categoryIdentifier)
		if category == .signMail {
			SCIVideophoneEngine.shared.heartbeat()
			tabBarController?.select(tab: .signmail)
		}
		if category == .missedCall {
			SCIVideophoneEngine.shared.heartbeat()
			tabBarController?.select(tab: .recents)
		}
		
		switch Action(rawValue: response.actionIdentifier) {
		case .answer?:
			let callId = response.notification.request.identifier
			if let call = CallController.shared.callForCallId(callId) {
				CallController.shared.answer(call)
			}
			
		case .decline?:
			let callId = response.notification.request.identifier
			if let call = CallController.shared.callForCallId(callId) {
				CallController.shared.end(call)
			}
			
		case .viewSignMail?:
			tabBarController?.select(tab: .signmail)
			
		case .returnCall?:
			if let dialString = response.notification.request.content.userInfo["dialstring"] as? String {
				CallController.shared.makeOutgoingCall(to: dialString, dialSource: .pushNotificationMissedCall)
			}
			
		case .returnSignMailCall?:
			if let dialString = response.notification.request.content.userInfo["dialstring"] as? String {
				CallController.shared.makeOutgoingCall(to: dialString, dialSource: .pushNotificationSignMailCall)
			}
			
		case nil:
			break
		}
		
		completionHandler()
	}
}
