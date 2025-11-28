//
//  CallWaitingTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 3/30/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class CallWaitingTests: XCTestCase {

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
	
	func test_3024_3035_3041_CallWaitingHoldAnswer() {
		// Test Case: 3024 - Call Waiting: Switch between calls
		// Test Case: 3035 - Call Waiting: Hold + Answer incoming call
		// Test Case: 3041 - Call Waiting: Hold + Answer is an option
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Add Contacts
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 3035", homeNumber: oepNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 3024", homeNumber: oepNum2, workNumber: "", mobileNumber: "")
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Call DUT
		con2.dial(number: dutNum)
	
		// Test 3035 + 3041
		// Call Waiting "Hold + Answer"
		ui.callWaiting(response: "Hold & Answer")
	
		// Test 3024
		// Switch Calls and verify
		ui.switchCall()
		ui.verifyCallerID(text: "Automate 3035")
		ui.switchCall()
		ui.verifyCallerID(text: "Automate 3024")
	
		// Hangup both calls
		ui.hangup()
		ui.hangup()
	
		// Delete Contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 3035")
		ui.deleteContact(name: "Automate 3024")
		ui.gotoHome()
	}
	
	func test_3031_CallWaitingDecline() {
		// Test Case: 3031 - Call Waiting: Decline an incoming call
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Add Contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 3031", homeNumber: oepNum1, workNumber: "", mobileNumber: "")
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		
		// Answer and verify
		con1.answer()
		sleep(3)
		ui.verifyCallerID(text: "Automate 3031")
		
		// Call DUT
		con2.dial(number: dutNum)
	
		// Call Waiting "Decline" and verify
		ui.callWaiting(response: "Decline")
		ui.verifyCallerID(text: "Automate 3031")
		sleep(3)
	
		// Hangup
		ui.hangup()
	
		// Verify Missed Call
		ui.waitForMissedCallBadge(count: "1")
		ui.gotoCallHistory(list: "Missed")
	
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 3031")
		ui.gotoHome()
	}
	
	func test_3039_3040_CallWaitingHangupAnswer() {
		// Test Case: 3039 - Call Waiting: Hangup + Answer incoming call
		// Test Case: 3040 - Call Waiting: Hangup + Answer is an option
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Add Contacts
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 3040", homeNumber: oepNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 3039", homeNumber: oepNum2, workNumber: "", mobileNumber: "")
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		
		// Answer and verify
		con1.answer()
		sleep(3)
		ui.verifyCallerID(text: "Automate 3040")
		
		// Call DUT
		con2.dial(number: dutNum)
	
		// Call Waiting "Hangup + Answer" and verify
		ui.callWaiting(response: "Hangup & Answer")
		ui.verifyCallerID(text: "Automate 3039")
	
		// Hangup
		ui.hangup()
	
		// Delete Contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 3040")
		ui.deleteContact(name: "Automate 3039")
		ui.gotoHome()
	}
	
	func test_7085_CallWaitingHangup() {
		// Test Case: 7085 - Call Waiting: Hanging up with a call waiting call does not dismiss both calls
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		
		// Answer and verify
		con1.answer()
		sleep(3)
		
		// Call DUT
		con2.dial(number: dutNum)
		sleep(3)
	
		// Call Waiting hangup 1st call
		ui.waitForCallWaiting()
		ui.hangup()
		
		// Answer 2nd call
		ui.incomingNtouchCall(response: "Answer")
		sleep(3)
		ui.hangup()
	}
	
	func test_7128_ShareTextCallWaiting() {
		// Test Case: 7128 - Share Text: Switching back to the first call, the shared text is not cleared.
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		
		// Answer and share text
		con1.answer()
		sleep(3)
		ui.shareText(text: "7128")
		ui.verifySelfViewText(text: "7128")
		
		// Call DUT
		con2.dial(number: dutNum)
	
		// Call Waiting "Answer" and verify no text
		ui.callWaiting(response: "Hold & Answer")
		ui.verifyNoSharedText(text: "7128")
	
		// Switch Calls and verify text
		ui.switchCall()
		ui.verifySelfViewText(text: "7128")
	
		// Hangup both calls
		ui.hangup()
		ui.hangup()
	}
	
	func test_11253_CallWaitingFail() {
		// Test Case: 11253 - Call Waiting: DUT's on hold Screen is not displaying properly after call transfer or add call fails
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Call trasnfer failed
		ui.transferCall(toNum: "11111111111")
		ui.clickButton(name: "OK")
		
		// Add call fails
		ui.addCall(num: "1800000000")
		ui.clickButton(name: "OK")
		
		// Add call does not answer
		ui.addCall(num: "18013568460")
		ui.hangup()
		ui.waitForCallOptions()
		
		// Have OEP put DUT on hold by calling an unassociated number
		con.dial(number: "18012923066", assert: false)
		ui.verifyHold()
		
		// Clean up
		ui.hangup()
		con.hangup()
		ui.gotoHome()
	}
}
