//
//  ShareContactTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 7/13/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class ShareContactTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let dutPassword = UserConfig()["Password"]!
	let vrsNum1 = "18015757669"
	let vrsNum2 = "18015757889"
	let vrsNum3 = "18015757999"
	let hearingUser = "19323384327"
	let endpointsClient = TestEndpointsClient.shared
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: cfg["Orientation"]!)
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
	
	func test_2544_ShareContactLongName() {
		// Test Case: 2544 - Share Contact: Long Contact Name
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact
		let longContactName = "Automate2544ContactWithAReallyLongName12345678902234567890323456"
		ui.gotoSorensonContacts()
		ui.addContact(name: longContactName, homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Place call
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Share Contact
		ui.shareContact(name: longContactName)
		ui.hangup()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: longContactName)
		ui.gotoHome()
	}
	
	func test_2545_ShareContactLandscape() {
		// Test Case: 2544 - Share Contact: Share contact in landscape
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2545", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Place call
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Share Contact in landscape
		ui.rotate(dir: "Left")
		ui.shareContact(name: "Automate 2545")
		con.hangup()
		ui.rotate(dir: "Portrait")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2545")
		ui.gotoHome()
	}
	
	func test_3360_7338_7340_ShareContact() {
		// Test Case: 3360 - Share Contact: Sorenson only contacts can share any of three numbers
		// Test Case: 7338 - Share Contact: Verify that if Share text is enabled, DUT can share contact
		// Test Case: 7340 - Share Contact: Verify that Share Contact is available in Customer mode
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7338", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		
		// Place call
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Share Contact home/mobile/work
		ui.shareContact(name: "Automate 7338", type: "home")
		ui.shareContact(name: "Automate 7338", type: "work")
		ui.shareContact(name: "Automate 7338", type: "mobile")
		ui.hangup()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7338")
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_7339_SharedOptionsDisabled() {
//		// Test Case: 7339 - Share Contact: Verify that if Share text is disabled, DUT cannot share contact
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
//		
//		// Disable Shared Text
//		core.setShareText(value: "0")
//		ui.heartbeat()
//		
//		// Place call and verify Share Options are disabled
//		ui.call(phnum: oepNum)
//		con.answer()
//		ui.verifyNoShareText()
//		
//		// Hang Up and set Shared Text to enabled
//		ui.hangup()
//		core.setShareText(value: "1")
//		ui.heartbeat()
//		ui.gotoHome()
//	}
}
