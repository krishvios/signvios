//
//  CallDirectoryHandler.swift
//  DirectoryExtension
//
//  Created by Daniel Shields on 11/3/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import Foundation
import CallKit

class CallDirectoryHandler: CXCallDirectoryProvider {

	enum CallDirectoryError: Error {
		case invalidPhoneNumber(dialString: String?)
		case invalidLabel(label: Any?)
	}

	override func beginRequest(with context: CXCallDirectoryExtensionContext) {
		context.delegate = self

		do {
			try addIdentificationPhoneNumbers(to: context)
		} catch {
			NSLog("Unable to add identification phone numbers")
			context.cancelRequest(withError: error)
			return
		}

		context.completeRequest()
	}

	private func addIdentificationPhoneNumbers(to context: CXCallDirectoryExtensionContext) throws {

		let defaults = UserDefaults(suiteName: "group.sorenson.ntouch.directoryextension-data-sharing")
		
		// If the app hasn't launched and populated the callDirectory dictionary, we still want to be able to
		// return something, otherwise enabling Call Directory in iOS settings will give an error.
		var directory = defaults?.dictionary(forKey: "callDirectory") ?? [:]
		if directory.isEmpty {
			directory["18012879403"] = "Sorenson Technical Support"
		}

		// Numbers must be provided in numerically ascending order.
		try directory
			.map { (dialString, value) -> (phoneNumber: CXCallDirectoryPhoneNumber, label: String) in
				// Ensure the data is in the format that we're expecting
				guard let phoneNumber = CXCallDirectoryPhoneNumber(dialString) else {
					NSLog("Invalid phone number format, could not convert to CXCallDirectoryPhoneNumber: \(dialString)")
					throw CallDirectoryError.invalidPhoneNumber(dialString: dialString)
				}
				guard let label = value as? String else {
					NSLog("Invalid label, expecting string: \(value)")
					throw CallDirectoryError.invalidLabel(label: value)
				}

				return (phoneNumber, label)
			}
			.sorted { $0.phoneNumber < $1.phoneNumber }
			.forEach { context.addIdentificationEntry(withNextSequentialPhoneNumber: $0.phoneNumber, label: $0.label) }

		defaults?.removeObject(forKey: "errorMessage")
	}
}

extension CallDirectoryHandler: CXCallDirectoryExtensionContextDelegate {

	func requestFailed(for extensionContext: CXCallDirectoryExtensionContext, withError error: Error) {
		// An error occurred while adding blocking or identification entries, check the NSError for details.
		// For Call Directory error codes, see the CXErrorCodeCallDirectoryManagerError enum in <CallKit/CXError.h>.
		//
		// This may be used to store the error details in a location accessible by the extension's containing app, so that the
		// app may be notified about errors which occured while loading data even if the request to load data was initiated by
		// the user in Settings instead of via the app itself.
		
		let defaults = UserDefaults(suiteName: "group.sorenson.ntouch.directoryextension-data-sharing")
		defaults?.set(error.localizedDescription, forKey: "errorMessage")
		defaults?.synchronize()
	}
}
