//
//  SpanishFeatureTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 11/14/22.
//  Copyright © 2022 Sorenson Communications. All rights reserved.
//

import XCTest

final class SpanishFeatureTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let dutPassword = UserConfig()["Password"]!
	let endpointsClient = TestEndpointsClient.shared
	let spanishNum = "18669877528"
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: cfg["Orientation"]!)
		continueAfterFailure = false
		// TODO: Server for CoreAccess currently not available
//		core.setSpanishFeatures(value: "0")
		ui.ntouchActivate()
		ui.clearAlert()
		ui.waitForMyNumber()
	}
	
	override func tearDown() {
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		// TODO: Server for CoreAccess currently not available
//		core.setSpanishFeatures(value: "0")
		ui.ntouchTerminate()
	}
	
	func test_2435_ContactSpanishDisplayed() {
		// Test 2435 - Contacts: Spanish Interpreter option for contacts
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Enable Spanish Features
		ui.gotoSettings()
		ui.enableSpanishFeatures(setting: true)
		
		// Add Contact to PhoneBook
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2435", homeNumber: "", workNumber: cfg["nvpNum"]!, mobileNumber: "")
		
		// Select Contact and Enable Spanish Contact Feature
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 2435")
		ui.toggleSpanishContact(setting: 1)
		ui.selectEdit()
		ui.verifySpanishSwitchState(state: "1")
		ui.selectCancel()
		
		// Select Contact and Disable Spanish Contact Feature
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 2435")
		ui.toggleSpanishContact(setting: 0)
		ui.selectEdit()
		ui.verifySpanishSwitchState(state: "0")
		ui.selectCancel()
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2435")
		ui.gotoSettings()
		ui.enableSpanishFeatures(setting: false)
		ui.gotoHome()
	}
	
	func test_6240_6245_6246_6459_EnableSpanish() {
		// Test Case: 6240 - Settings: Verify that the UI updates to correctly reflect if the Spanish feature is on or off
		// Test Case: 6245 - Settings: Verify that the SVRS Espanol contact appears under the SVRS contact when enabled
		// Test Case: 6246 - Settings: Verify that the SVRS Espanol contact is removed from contacts when disabled
		// Test Case: 6459 - Settings: Spanish Features is present in customer mode
		
		// Test 6240 & 6459
		// Enable Spanish Features
		ui.gotoSettings()
		ui.enableSpanishFeatures(setting: true)
		
		// Test 6245
		// Verify Spanish Contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "SVRS Español")
		
		// Disable Spanish Features
		ui.gotoSettings()
		ui.enableSpanishFeatures(setting: false)
		
		// Test 6246
		// Verify no Spanish Conact
		ui.gotoSorensonContacts()
		ui.verifyNoContact(name: "SVRS Español")
		ui.gotoHome()
	}
	
	func test_6243_SpanishFeatureTitle() {
		// Test Case: 6243 - Contacts: Verify that the feature title is "Enable Spanish Features" for SVRS Espanol contact
		// Navigate to Settings and Verify Spanish Feature Title is displayed
		ui.gotoSettings()
		ui.verifyText(text: "Enable Spanish Features", expected: true)
		
		// Cleanup
		ui.gotoHome()
	}
	
	func test_7809_SVRSEspanolCallHistoryEntry() {
		// Test Case: 7809 - Call History: SVRS Espanol will appear in the Call History Tab
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Enable Spanish Features
		ui.gotoSettings()
		ui.enableSpanishFeatures(setting: true)
		
		// generate Spanish call history entry
		ui.gotoSorensonContacts()
		testUtils.calleeNum = spanishNum
		ui.callContact(name: "SVRS Español", type: "work")
		ui.waitForCallOptions()
		ui.hangup()
		
		// Verify call history entry
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "SVRS Español", num: spanishNum)
		
		// Cleanup
		ui.deleteAllCallHistory()
		ui.gotoSettings()
		ui.enableSpanishFeatures(setting: false)
		ui.gotoHome()
	}
	
	func test_C102419_LangTerpSwitcherDisplay() {
		// Test Case: C102419 - Terp Language Switcher Display Verification
		
		// Go to settings to enable Spanish Features
		// Go to Home to verify that the switcher is displaying
		ui.gotoSettings()
		ui.enableSpanishFeatures(setting: true)
		ui.gotoHome()
		ui.verifyText(text: "Spanish", expected: true)
		
		// Go to Setting and verify that the Spanish Features is disabled
		// Then go to Dialer View and see if there's no switcher
		ui.gotoSettings()
		ui.enableSpanishFeatures(setting: false)
		ui.gotoHome()
		ui.verifyText(text: "Spanish", expected: false)
	}
}
