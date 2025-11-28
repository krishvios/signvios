//
//  CallTransferTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 9/7/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import XCTest

class CallTransferTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let vrsNum1 = "18015757669"
	let vrsNum2 = "18015757889"
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
	
	func test_3602_4452_CallTransferMenuHangup() {
		// Test Case: 3602 - Call Transfer: Open the call transfer window
		// Test Case: 4452 - Call Transfer: After hanging up a call in the call transfer contact menu, the DUT doesn't lock up on the call screen
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
	
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		
		// Select Transfer
		ui.selectCall(option: "Transfer Call")
		ui.gotoCallTab(button: "Contacts")
		
		//  Remote Hangup
		con.hangup()
		ui.gotoHome()
	}
	
	func test_3642_3883_4002_4029_4431_4636_CallTransfer() {
		// Test Case: 3642 - Call Transfer: Transfer sorenson endpoint that does not belong to a myPhone group
		// Test Case: 3883 - Call Transfer: Number pad is displayed properly when viewed in Portrait orientation
		// Test Case: 4002 - Call Transfer: Call disconnects and once transfer call is made the call window closes and goes back to the main screen
		// Test Case: 4029 - Call Transfer: Clicking call transfer button once will transfer call to endpoint
		// Test Case: 4431 - Call Transfer: When transferring a call using the dial pad phone number will not be truncated
		// Test Case: 4636 - Call Transfer: End call properly after transferring
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Transfer Call
		ui.transferCall(toNum: oepNum2)
		ui.gotoHome()
		
		// Answer Transfer
		con2.answer()
		sleep(3)
		con2.hangup()
	}
	
	func test_3694_CallTransferContact() {
		// Test Case: 3694 - Call Transfer: Verify that selecting a contact with multiple numbers does not crash when using Call Transfer
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Add Contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 3694", homeNumber: oepNum2, workNumber: vrsNum1, mobileNumber: vrsNum2)
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)

		// Transfer Call
		testUtils.calleeNum = oepNum2
		ui.transferCall(toContact: "Automate 3694", type: "home")
		con2.answer()
		sleep(3)
		con2.hangup()

		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 3694")
	}
	
	func test_3884_CallTransferMenuLandscape() {
		// Test Case: 3884 - Call Transfer: Rotation is working after viewing Call Transfer menu in landscape orientation
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		
		// Rotate Landscape and select Transfer
		ui.rotate(dir: "Left")
		ui.selectCall(option: "Transfer Call")
		
		// Remote Hangup and Rotate Portrait
		con.hangup()
		ui.rotate(dir: "Portrait")
		ui.gotoHome()
	}
	
	func test_4577_IncomingCallTransfer() {
		// Test Case: 4577 - Call Transfer: Transferred call rings on remote endpoint
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1),
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// OEP1 Call OEP2
		con1.dial(number: oepNum2)
		con2.answer()
		sleep(3)
		
		// VP Transfer Call to DUT
		con1.transferCall(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
	}
	
	func test_4579_IncomingCallTransferDecline() {
		// Test Case: 4579 - Call Transfer: Transferring to a device which ignores (declines) the call properly sends call to signmail
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1),
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// OEP1 Call DUT
		con1.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		
		// VP Transfer Call to OEP2 and reject
		con1.transferCall(number: oepNum2)
		con2.rejectCall()
		
		// Verify SignMail
		ui.leaveSignMailRecording(time: 10, response: "Exit")
	}
	
	func test_4581_CallTransfer2x() {
		// Test Case: 4581 - Call Transfer: Device can be transferred multiple times during one call
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// VP Transfer Call to OEP2
		ui.transferCall(toNum: oepNum2)
		con2.answer()
		sleep(3)
		
		// PC Transfer back to DUT
		con2.transferCall(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
	}
	
	func test_4582_4583_BlockedCallTransfer() {
		// Test Case: 4582 - Call Transfer: Transfer call to a blocked end user
		// Test Case: 4583 - Call Transfer: Transfer from a blocked end user fails
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Add Blocked Number
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Auto 4582", num: oepNum1)
		
		// Call OEP2
		ui.call(phnum: oepNum2)
		con2.answer()
		sleep(3)
		
		// Test 4582
		// DUT Transfer Call to Blocked
		ui.transferCall(toNum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Test 4583
		// Attempt to transfer blocked to DUT
		con2.transferCall(number: dutNum)
		ui.gotoHome()
		con2.hangup()
		
		// Delete Blocked List
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoHome()
	}
	
	func test_4584_TransferScreenIncomingCall() {
		// Test Case: 4584 - Call Transfer: Receiving a call while on the transfer screen
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Open Transfer Screen
		ui.selectCall(option: "Transfer Call")
		
		// OEP2 Call DUT
		con2.dial(number: dutNum)
		ui.selectCancel()
		ui.callWaiting(response: "Hangup & Answer")
		sleep(3)
		ui.hangup()
	}
	
	func test_4591_TransferCallToOepInCall() {
		// Test Case: 4591 - Call Transfer: Transfer to an endpoint already in a call
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		let endpoint3 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1),
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2),
			let con3 = endpointsClient.tryToConnectToEndpoint(endpoint3), let oepNum3 = endpoint3.number else { return }
				
		// OEP2 Call OEP3
		con2.dial(number: oepNum3)
		con3.answer()
		sleep(3)
		
		// OEP1 Call DUT
		con1.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		
		// OEP2 Transfer OEP3 to DUT
		con2.transferCall(number: dutNum)
		ui.callWaiting(response: "Hangup & Answer")
		sleep(3)
		ui.hangup()
	}
	
	func test_4595_4600_CallTransferTextGreeting() {
		// Test Case: 4595 - Call Transfer: Transfer to a device with PSMG with text
		// Test Case: 4600 - Call Transfer: Call Transfer: Transfer to a person with text only greeting
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		let con2 = Connection(host: cfg["nvpIP"]!)
		do {
			try con2.connect()
			
			// Call OEP
			ui.call(phnum: oepNum)
			con.answer()
			ui.waitForCallOptions()
			sleep(3)
			
			// Transfer DUT to con2 and reject
			con.transferCall(number: cfg["nvpNum"]!)
			con2.rejectCall()
			
			// Exit SignMail
			ui.waitForGreeting(textOnly: true, text: "remote test greeting")
			ui.leaveSignMailRecording(time: 5, response: "Exit")
		}
		catch {
			XCTFail("Failed to connect to: " + cfg["nvpIP"]!)
		}
	}
	
	func test_4597_4598_CallTransferGreeting() {
		// Test Case: 4597 - Call Transfer: Transfer to a device with PSMG only
		// Test Case: 4598 - Call Transfer: Transfer to a device with Default Sorenson SMG only
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNumm2 = endpoint2.number else { return }
				
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Transfer DUT to OEP2
		con1.transferCall(number: oepNumm2)
		con2.rejectCall()
		
		// Verify Greeting
		ui.leaveSignMailRecording(time: 10, response: "Exit")
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_4599_CallTransferNoGreeting() {
//		// Test Case: 4599 - Call Transfer: Transfer to a person with no signmail greeting
//		let endpoint1 = endpointsClient.checkOut()
//		let endpoint2 = endpointsClient.checkOut()
//		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
//		let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
//		
//		// Set OEP2 Greeting to No Greeting
//		core.setGreetingType(phoneNumber: oepNum2, value: "0")
//		
//		// Call OEP1
//		ui.call(phnum: oepNum1)
//		con1.answer()
//		ui.waitForCallOptions()
//		sleep(3)
//		
//		// OEP1 Transfer DUT to OEP2 and reject
//		con1.transferCall(number: oepNum2)
//		con2.rejectCall()
//		
//		// Exit SignMail
//		ui.leaveSignMailRecording(time: 5, response: "Exit", doSkip: false)
//		
//		// Set OEP2 Greeting to Sorenson Greeting
//		core.setGreetingType(phoneNumber: oepNum2, value: "1")
//	}
	
	func test_7419_CallTransferVRS() {
		// Test Case: 7419 - Call Transfer: Transfer deaf call to a VRS Hold Server
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		
		// Transfer Call
		ui.transferCall(toNum: vrsNum1)
		con.waitForCallConnected(callerID: "Sorenson VRS Hold Server")
		
		// Hangup
		con.hangup()
	}

	func test_7434_CallTransferToOwnNumber() {
		// Test Case: 7434 - Call Transfer: The user is not able to transfer a call to their own number
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Transfer DUT to Own Number
		ui.transferCall(toNum: dutNum)
		
		// Verify Transfer Error is Displayed
		ui.verifyTransferError()
		ui.hangup()
	}
}
