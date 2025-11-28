//
//  IntentHandler.swift
//  IntentsExtension
//
//  Created by Dan Shields on 2/6/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import Intents

@objc(SCIIntentHandler)
class IntentHandler: INExtension {
	static let sharedSuiteName = "group.com.sorenson.ntouch.contacts"
	let userDefaults  = UserDefaults(suiteName: sharedSuiteName)
	
	func searchSorensonContacts(_ contacts: [[String:Any]], for person: INPerson) -> INPersonResolutionResult {
		let matchingPeople = contacts.compactMap { contact -> INPerson? in
			var searchResult: String?
			if let name = contact["name"] as? String, name.lowercased().contains(person.displayName.lowercased()) {
				searchResult = name
			}
			else if let company = contact["companyName"] as? String, company.lowercased().contains(person.displayName.lowercased()) {
				searchResult = company
			}
			guard let displayName = searchResult else {
				return nil
			}
			
			guard let numbers = contact["numbers"] as? [String: String] else {
				return nil
			}
			
			var matchingNumberKeys: [String] = ["home", "work", "cell"]
			
			// If the user specifically said a particular number of the contact, filter on that.
			if #available(iOSApplicationExtension 10.2, *), let label = person.personHandle?.label {
				if label == .home {
					matchingNumberKeys = ["home"]
				}
				else if label == .work {
					matchingNumberKeys = ["work"]
				}
				else if label == .mobile || label == .iPhone {
					matchingNumberKeys = ["cell"]
				}
				else {
					matchingNumberKeys = []
				}
			}
			
			let personHandles = matchingNumberKeys.compactMap { key -> INPersonHandle? in
				if let number = numbers[key] {
					return INPersonHandle(value: number, type: .phoneNumber)
				}
				else {
					return nil
				}
			}
			
			guard !personHandles.isEmpty else {
				return nil
			}
			
			return INPerson(personHandle: personHandles.first!, nameComponents: nil, displayName: displayName, image: nil, contactIdentifier: nil, customIdentifier: personHandles.first!.value!, aliases: Array(personHandles.dropFirst()), suggestionType: .socialProfile)
		}
		
		if matchingPeople.count == 1 {
			return .success(with: matchingPeople.first!)
		}
		else if matchingPeople.count > 1 {
			return .disambiguation(with: matchingPeople)
		}
		else {
			return .unsupported()
		}
	}
}

extension IntentHandler: INStartAudioCallIntentHandling {
	func handle(intent: INStartAudioCallIntent, completion: @escaping (INStartAudioCallIntentResponse) -> Void) {
		if (intent.contacts?.count ?? 0) > 1 {
			// We don't support making group calls through siri yet.
			completion(INStartAudioCallIntentResponse(code: .failureContactNotSupportedByApp, userActivity: nil))
		}
		else if intent.contacts?.first?.personHandle == nil {
			if userDefaults?.array(forKey: "contacts") == nil {
				completion(INStartAudioCallIntentResponse(code: .failureRequiringAppLaunch, userActivity: nil))
			}
			else if #available(iOS 11.0, *) {
				completion(INStartAudioCallIntentResponse(code: .failureNoValidNumber, userActivity: nil));
			}
			else {
				completion(INStartAudioCallIntentResponse(code: .failureContactNotSupportedByApp, userActivity: nil));
			}
		}
		else {
			let userActivity = NSUserActivity(activityType: INStartAudioCallIntentIdentifier)
			completion(INStartAudioCallIntentResponse(code: .continueInApp, userActivity: userActivity))
		}
	}
	
	func resolveContacts(for intent: INStartAudioCallIntent, with completion: @escaping ([INPersonResolutionResult]) -> Void) {
		
		guard let contacts = userDefaults?.array(forKey: "contacts") as? [[String : Any]] else {
			completion((intent.contacts ?? []).map { _ in INPersonResolutionResult.notRequired() })
			return
		}
		
		let sorensonMatches = intent.contacts?.map { searchSorensonContacts(contacts, for: $0) } ?? []
		completion(sorensonMatches)
	}
}

extension IntentHandler: INStartVideoCallIntentHandling {
	func handle(intent: INStartVideoCallIntent, completion: @escaping (INStartVideoCallIntentResponse) -> Void) {
		if (intent.contacts?.count ?? 0) > 1 {
			// We don't support making group calls through siri yet.
			completion(INStartVideoCallIntentResponse(code: .failureContactNotSupportedByApp, userActivity: nil))
		}
		else if intent.contacts?.first?.personHandle == nil {
			if userDefaults?.array(forKey: "contacts") == nil {
				completion(INStartVideoCallIntentResponse(code: .failureRequiringAppLaunch, userActivity: nil))
			}
			else if #available(iOS 11.0, *) {
				completion(INStartVideoCallIntentResponse(code: .failureInvalidNumber, userActivity: nil));
			}
			else {
				completion(INStartVideoCallIntentResponse(code: .failureContactNotSupportedByApp, userActivity: nil));
			}
		}
		else {
			let userActivity = NSUserActivity(activityType: INStartVideoCallIntentIdentifier)
			completion(INStartVideoCallIntentResponse(code: .continueInApp, userActivity: userActivity))
		}
	}
	
	func resolveContacts(for intent: INStartVideoCallIntent, with completion: @escaping ([INPersonResolutionResult]) -> Void) {
		
		guard let contacts = userDefaults?.array(forKey: "contacts") as? [[String : Any]] else {
			completion((intent.contacts ?? []).map { _ in INPersonResolutionResult.notRequired() })
			return
		}
		
		let sorensonMatches = intent.contacts?.map { searchSorensonContacts(contacts, for: $0) } ?? []
		completion(sorensonMatches)
	}
}
