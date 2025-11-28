//
//  HearingInterfaceTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 3/30/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class HearingInterfaceTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["hearingInterfaceAltNum"]!
	let vrsNum = "18015757669"
	let endpointsClient = TestEndpointsClient.shared
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: UserConfig()["Orientation"]!)
		continueAfterFailure = false
		ui.ntouchActivate()
		ui.clearAlert()
		ui.waitForMyNumber()
	}
	
	override func tearDown() {
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}
	
	func test_3562_HearingModeCallCIR() {
		// Test Case: 3562: - Hearing Interface: DUT is able to call CIR while in Hearing Interface Mode from the Dialer
		// Login Hearing user
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Call CIR
		ui.call(phnum: "611")
		//ui.call(phnum: "18013868500")
		sleep(3)
		ui.verifyCallerID(text: "Customer Care")
		ui.hangup()
		
		// Login DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_3594_HearingModeCallVRS() {
		// Test Case: 3594: - Hearing Interface: DUT is unable to place a VRS call from dialer
		// Login Hearing user
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Call VRS and Verify Unable to place VRS Calls
		ui.call(phnum: "411")
		ui.verifyUnableToPlaceVRSCalls()
		
		// Login DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_6460_HearingModeNoSpanish() {
		// Test Case: 6460 - Hearing Interface: Spanish features is not present in hearing mode.
		// Login Hearing user
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Verify no Spanish
		ui.gotoSettings()
		ui.verifyNoSpanishFeatures()
		
		// Login DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_6553_HearingModeBlockCallerIDAvailable() {
		// Test Case: 6553 - Hearing Interface: Block Caller ID check box is available in Hearing Mode

		// Switch to an alternate account with interface group set to "Hearing"
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoSettings()
		
		// Scroll down to text and verify "Hide My Caller ID" is available
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: false)
		ui.verifyText(text: "Hide My Caller ID", expected: true)
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_7341_ShareContactHearingMode() {
		// Test Case: 7341 - Hearing Interface: Verify that Share Contact is available in Hearing mode
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Login Hearing user
		ui.switchAccounts(number: altNum, password: "1234")
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7341", homeNumber: vrsNum, workNumber: "", mobileNumber: "")
		
		// Place call
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Share Contact
		ui.shareContact(name: "Automate 7341")
		ui.hangup()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7341")
		
		// Login DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_8365_HearingModeEnhancedSignMailEnabled() {
		// Test Case: 8365 - Hearing Interface: Enhanced SignMail is available in Hearing mode
		
		// Login Hearing user
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Verify Enhanced SignMail Enabled
		ui.gotoSignMail()
		ui.verifyEnhancedSignMailEnabled()
		
		// Login DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_8377_HearingModeSendSignMailToSelf() {
		// Test Case: 8377 - Hearing Interface: User can not select themselves as a recipient of a Enhanced SignMail in Hearing Mode
		
		// Login Hearing user
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Add Contact of self
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 8377", homeNumber: altNum, workNumber: "", mobileNumber: "")
		
		// Send SignMail to self contact
		ui.sendSignMailToContact(name: "Automate 8377")
		
		// Verify error
		ui.verifyCannotSendSignMailToSelf()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 8377")
		
		// Login DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
}
