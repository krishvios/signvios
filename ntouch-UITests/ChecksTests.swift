//
//  ChecksTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 11/25/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import XCTest

class ChecksTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let dutPassword = UserConfig()["Password"]!
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

	func test_Checks() {
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Logout
		ui.logout()
		
		// Failed Login
		ui.enterPassword(password: "1111")
		ui.selectLogin()
		ui.verifyInvalidLogin()
		
		// Login and Delete Call History
		ui.login(phnum: dutNum, password: dutPassword)
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Call Hold Server
		ui.call(phnum: "18015757669")
		ui.waitForCallOptions()
		ui.hangup()
		
		// Add Contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		
		// Receive Call From Contact
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		ui.waitForCallOptions()
		ui.verifyCallerID(text: "Automate")
		ui.hangup()
		
		// Call From History
		ui.gotoCallHistory(list: "All")
		ui.callFromHistory(name: "Automate", num: oepNum)
		con.answer()
		ui.waitForCallOptions()
		ui.hangup()
		
		// Search Contact
		ui.gotoSorensonContacts()
		ui.yelp(search: "Automate")
		ui.verifyContact(name: "Automate")
		ui.clearYelpSearch()
		
		// Delete Contact
		ui.deleteContact(name: "Automate")
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteCallHistory(name: "Automate", num: oepNum)
		ui.deleteAllCallHistory()
	}
}
