//
//  AnalyticsManager.swift
//  ntouch
//
//  Created by mmccoy on 3/24/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Pendo

@objc class AnalyticsManager: NSObject {
	@objc static let shared = AnalyticsManager()
	private static var started = false
	private override init() {
		PendoManager.shared().setup(pendoAPIKey)
	}
	
	let pendoAPIKey = "eyJhbGciOiJSUzI1NiIsImtpZCI6IiIsInR5cCI6IkpXVCJ9.eyJkYXRhY2VudGVyIjoidXMiLCJrZXkiOiI1ZjRiOTRmOTYxOTdkZjdhNmM5OTgyMmVhNjYyZmU1ZmVlNjUzZTMyNGVkNGFiOTdlYjZmZjg3NmI0MjA2ZGI3NjliNjJiN2U3MTQ4ZmY3MTFkMDJlMWQwYjQwYTA4NjYzMWM5MzBmYzdmY2U5ZWY4NDk2MGQ4ZGUxYTA3OWQ4Ni5hYWRhMmQyMTgxMWJjNWQ1ZTZiYjMyOGQ1Nzg4NzNhMy5lNjk2ZGQ2YzJlY2FmNGQ0YTBmOThhNDI1ZDBiN2RmMWZjOGEzNjA5ZWU5NWJlNGUxMzdmZmEyOTdiZDhlZTk5In0.Mwdxa8ZLyylm4QReS8Fm22zfXNasZMl2LJZGDM-osSdlkn5Zx3DquM2294NYko68TxAK1wefrPvrP2qeDYd6Ptv_aWizTRphgdKvoOw9o_RDyBEY1m4cSiyloXzqx2DYkhlY9vWOIPKXl1URnyDvG4tiHKIiuHFPOd1pzMIw07M"
	
	@objc func start(visitorID: String?) {
		PendoManager.shared().startSession(visitorID, accountId: nil, visitorData: nil, accountData: nil)
		AnalyticsManager.started = true
	}
	
	@objc func start() {
		start(visitorID: nil)
	}
	
	@objc func stop() {
		PendoManager.shared().endSession()
		AnalyticsManager.started = false
	}
	
	@objc func trackUsage(_ event: AnalyticsEvent, properties: [String : Any]?) {
		if AnalyticsManager.started {
			PendoManager.shared().track(event.stringValue, properties: properties)
		}
	}
	
	static func pendoStringFromDialSource(dialSource: SCIDialSource) -> String {
		var result = "Unknown"
		switch dialSource {
		case .unknown: 						result = "Unknown"
		case .contact: 						result = "contact"
		case .callHistory: 					result = "call_history"
		case .signMail: 					result = "signMail"
		case .favorite: 					result = "favorite"
		case .adHoc:						result = "dialpad"			// Dialpad
		case .VRCL:							result = "vrcl"
		case .svrsButton:					result = "svrs_button"
		case .recentCalls:					result = "recent_calls"		// Recent Summary (iPad home screen)
		case .transferred:					result = "transferred"
		case .internetSearch:				result = "internet_search" 	// Yelp or Google Places
		case .webDial:						result = "web_dial"
		case .pushNotificationMissedCall:	result = "missed_call_notification"
		case .pushNotificationSignMailCall:	result = "sign_mail_notification"
		case .LDAP:							result = "LDAP"
		case .fastSearch:					result = "fast_search"
		case .myPhone:						result = "myPhone"
		case .uiButton:						result = "uiButton"
		case .svrsEnglishContact:			result = "svrs_english_contact"
		case .svrsSpanishContact:			result = "svrs_spanish_contact"
		case .deviceContact:				result = "device_contact"
		case .directSignMail:				result = "direct_signmail"
		case .directSignMailFailure:		result = "direct_signmail_failure"
		case .callHistoryDisconnected:		result = "call_history_disconnected"
		case .recentCallsDisconnected:		result = "recent_calls_disconnected"
		case .nativeDialer:					result = "native_dialer"
		case .blockList:					result = "block_list"
		case .callWithEncryption:			result = "call_with_encryption"
		case .spanishAdHoc: 				result = "spanish_slider"
		case .directory:					result = "directory"
		case .helpUI:						result = "help"
		case .loginUI:						result = "login"
		case .errorUI:						result = "error_alert"
		case .callCustService:				result = "call_customer_svc_alert"	
		@unknown default: 					result = "Unknown"
		}
		return result
	}
}

@objc enum AnalyticsEvent: Int {
	case addCall
	case appSharing
	case blockedList
	case call
	case callInitiationSource
	case callHistory
	case contacts
	case corpDirectory
	case deviceContacts
	case dialer
	case ens
	case favorites
	case help
	case inCall
	case initialized
	case login
	case logout
	case mainTab
	case search
	case searchContactOrBusiness
	case settings
	case signMail
	case signMailInitiationSource
	case signMailPlayer
	case terminated
	case transferCall
	case videoCenter
	case videoPlayer
	
	var stringValue: String {
		switch self {
		case .addCall:
			return "add_call_usage"
		case .appSharing:
			return "app_sharing_usage"
		case .blockedList:
			return "block_list_usage"
		case .call:
			return "call_usage"
		case .callInitiationSource:
			return "call_initiation_source"
		case .callHistory:
			return "call_history_usage"
		case .contacts:
			return "contacts_usage"
		case .corpDirectory:
			return "ldap_usage"
		case .deviceContacts:
			return "device_contacts_usage"
		case .dialer:
			return "dialer_usage"
		case .ens:
			return "ens_usage"
		case .favorites:
			return "favorites_usage"
		case .help:
			return "help_usage"
		case .inCall:
			return "in_call_usage"
		case .initialized:
			return "initialized"
		case .login:
			return "login"
		case .logout:
			return "logout"
		case.mainTab:
			return "main_tab_usage"
		case .search:
			return "search_usage"
		case .searchContactOrBusiness:
			return "search_contact_or_business"
		case .signMail:
			return "signmail_usage"
		case .signMailInitiationSource:
			return "signmail_initiation_source"
		case .signMailPlayer:
			return "signmail_player_usage"
		case .settings:
			return "settings_usage"
		case .terminated:
			return "app_terminated"
		case .transferCall:
			return "transfer_call_usage"
		case .videoCenter:
			return "video_center_usage"
		case .videoPlayer:
			return "video_player_usage"
		default:
			return ""
		}
	}
	static func getEventForDetailsViewModel(_ detailsViewModel: DetailsViewModel? ) -> AnalyticsEvent? {
		guard let viewModel = detailsViewModel else { return nil }
		switch viewModel.type {
		case .contact:
			guard let contact = viewModel.contact else { return nil }
			if contact.isLDAPContact { return .corpDirectory }
			if let detailsViewController = viewModel.delegate as? UIViewController,
				detailsViewController.navigationController?.navigationItem.backBarButtonItem?.title == "Favorites" {
				return .favorites
			}
			return .contacts
		case .callHistory:
			return .callHistory
		case .signMail:
			return .signMail
		}
	}
	
	
}
