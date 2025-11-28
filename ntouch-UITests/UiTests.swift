//
//  UiTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 2/25/21.
//  Copyright © 2021 Sorenson Communications. All rights reserved.
//

import XCTest

class UiTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let endpointsClient = TestEndpointsClient.shared
	let vcNum = "19323388677"
	let altWithTollFreeNum = "19328653733"
	let tollFreeNum = "18004573733"
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: UserConfig()["Orientation"]!)
		continueAfterFailure = false
		ui.ntouchActivate()
		ui.waitForMyNumber()
	}
	
	override func tearDown() {
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}
	
	func test_2541_tollFreeDialer() {
		// Test Case: 2590 - UI: Toll free number will displayed if selected
		// Go to alt number with toll free number
		ui.switchAccounts(number: altWithTollFreeNum, password: defaultPassword)

		// Switch account number to use toll free number
		ui.gotoPersonalInfo()
		ui.setSettingsSwitch(setting: "Use Toll-Free number", value: true, save: true)
		ui.gotoHome()
		
		//Verify toll free phone number on dialer
		ui.verifyDialerNumber(num: tollFreeNum)
		
		// Clean up
		ui.gotoPersonalInfo()
		ui.setSettingsSwitch(setting: "Use Toll-Free number", value: false, save: true)
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_3699_n11DialerLabels() {
		// Test Case: 3699 - UI: 911/411/611 produce a contact label
		
		// Verify 911 contact label
		ui.dial(phnum: "911")
		ui.verifyDialerContact(name: "Emergency")
		ui.clearDialer()
		
		// Verify 411 contact label
		ui.dial(phnum: "411")
		ui.verifyDialerContact(name: "Information")
		ui.clearDialer()
		
		// Verify 611 contact label
		ui.dial(phnum: "611")
		ui.verifyDialerContact(name: "Customer Care")
		ui.clearDialer()
	}
	
	func test_3764_LandscapePortraitTabChange() {
		// Test Case: 3764 - UI: Rotating and changing tabs does not crash the app.

		// Go into Landscape, then return to Portrait and attempt to change tabs
		ui.rotate(dir: "Left")
		sleep(3)
		ui.rotate(dir: "Portrait")
		ui.gotoCallHistory(list: "All")
		ui.gotoSorensonContacts()
		ui.gotoSignMail()
		ui.gotoSettings()
		ui.gotoHome()
	}
	
	func test_3785_NavigateToAndFromContacts() {
		// Test Case: 3785 - UI: Verify that navigating to Sorenson Contacts and then to Home performs normally
		
		// Navigate between Contacts and Home
		for _ in 1...5 {
			ui.gotoSorensonContacts()
			ui.gotoHome()
		}
	}
	
	func test_7649_SVRSLabelFromContacts() {
		// Test Case: 7649 - UI: Selecting "SVRS" from the Directory Page page will display "SVRS"
		
		// Go to Contacts and verify SVRS will display properly in the contacts page
		ui.gotoSorensonContacts()
		ui.selectContact(name: "SVRS")
		ui.verifyText(text: "SVRS", expected: true)
		ui.gotoHome()
	}

	func test_9309_DialerSelfView() {
		// Test Case: 9309 - Self View: Self-View Displayed while user is on the Dialer

		// Go to dialer and verify self view
		ui.gotoHome()
		ui.verifyDialerSelfView()
	}
}
