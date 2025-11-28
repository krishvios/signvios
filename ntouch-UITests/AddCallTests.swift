//
//  AddCallTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 11/1/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import XCTest

class AddCallTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let dutName = UserConfig()["Name"]!
	let altNum = UserConfig()["addCallAltNum"]!
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
	
	func test_5917_AddCallCallSwitching() {
		// Test Case: 5917 - Calling - Add Call: User is able to switch between the calls without getting a black screen
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call OEP from DUT
		ui.call(phnum: oepNum1)
		con1.answer()
		ui.waitForCallOptions()
		ui.showCallStats()

		// On DUT connect to second call by selecting add call
		ui.addCall(num: oepNum2)
		con2.answer()
		ui.waitForCallOptions()
		ui.showCallStats()

		// Switch between endpoint 1 and 2 and verify call is still transferring data
		for _ in 0...2 {
			ui.switchCall()
			ui.verifyFrameRateGreaterThan(value: 0)
		}

		// Hang up and clean up
		con2.hangup()
		con1.hangup()
		ui.gotoHome()
	}
	
	func test_5920_AddCallAvailableP2P() {
		// Test Case: 5920 - Calling - Add Call: Add call will be available in point to point calls
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let _ = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call OEP and Add call
		ui.call(phnum: oepNum1)
		con1.answer()
		ui.addCall(num: oepNum2)
		
		// Hangup Calls
		con1.hangup()
		ui.hangup()
	}
	
	func test_5922_AddCallToBlockingOEP() {
		// Test Case: 5922 - Calling - Add Call:  When adding a callee that has blocked you, a message will be displayed that the person you called could not be reached
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }

		// Login to blocking account and go to blocked numbers
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoBlockedList()

		// Delete all blocked contacts to prevent errors
		ui.deleteBlockedList()

		// Block DUT Account
		ui.blockNumber(name: dutName, num: dutNum)
		ui.gotoHome()

		// Log back into DUT Account
		ui.switchAccounts(number: dutNum, password: defaultPassword)

		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()

		// Add Call to blocking account
		ui.addCall(num: altNum)

		// Verify Call Disconnected message and hangup
		ui.verifyCallFailed()
		ui.hangup()
	}

	func test_5923_AddCallFromContacts() {
		// Test Case: 5923 - Calling - Add Call: A user may add a call from his/her phonebook
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Add Contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5923", homeNumber: oepNum2, workNumber: "", mobileNumber: "")
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Add Call from Contacts
		testUtils.calleeNum = oepNum2
		ui.addCallFromContacts(name: "Automate 5923")
		con2.answer()
		sleep(3)
		
		// Hangup calls
		ui.hangup()
		ui.hangup()
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5923")
		ui.gotoHome()
	}

	func test_5925_AddCallFromCallHistory() {
		// Test Case: 5925 - Calling - Add Call: A user may add a call from his/her call history
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Call OEP2
		ui.call(phnum: oepNum2)
		con2.answer()
		sleep(3)
		ui.hangup()
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Add Call from Call History
		ui.addCallFromHistory(num: oepNum2)
		con2.answer()
		sleep(3)
		
		// Hangup calls
		ui.hangup()
		ui.hangup()
	}

	func test_5926_AddCallFromDialer() {
		// Test Case: 5926 - Calling - Add Call: A user may add a call from the dial pad
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
	
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Add Call to OEP2
		ui.addCall(num: oepNum2)
		con2.answer()
		sleep(3)
		
		// Hangup calls
		ui.hangup()
		ui.hangup()
	}

	func test_5927_AddCallLimit() {
		// Test Case: 5927 - Calling - Add Call:  The user may only add one additional call
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		
		// Add Call to OEP2
		ui.addCall(num: oepNum2)
		con2.answer()
		sleep(3)
		
		// Verify no add call option
		ui.verifyNoAddCallOption()
		
		// Hangup calls
		ui.hangup()
		ui.hangup()
	}
	
	func test_5929_6586_GVC() {
		// Test Case: 5929 - GVC: Add Call: User can add a call to another endpoint
		// Test Case: 6586 - GVC: Clicking on Join Call will move all call to the MCU
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		
		// Answer
		con1.answer()
		sleep(3)

		// Test 5929
		// Add Call
		ui.addCall(num: oepNum2)
		
		// Answer
		con2.answer()
		sleep(3)
		
		// Test 6586
		// Join Calls
		ui.selectCall(option: "Join Calls")
		sleep(25)
		ui.hangup()
		con1.hangup()
		con2.hangup()
	}

	func test_5930_HangupDuringAddCall() {
		// Test Case: 5930 - Calling - Add Call: The call will end if the remote user hangs up and the user has the add call window open but hasn't added another call yet
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call VP
		ui.call(phnum: oepNum)
		con.answer()
		
		// Select Add Call
		ui.selectAddCall()
		
		// Hangup
		con.hangup()
		ui.gotoHome()
	}

	func test_5933_HoldServerNoAddCall() {
		// Test Case: 5933 - Calling - Add Call: Cannot add a call while connected to the hold server
		// Call Hold Server
		ui.call(phnum: "18015757669")
		
		// Verify no call options
		ui.verifyNoCallOptions()
		
		// Hangup
		ui.hangup()
	}

	func test_5934_AddCallHangUpWhileRinging() {
		// Test Case: 5934 - Calling - Add Call:  The user can add a call and then hang up while still ringing and the first call will resume
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let _ = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
	
		// Call to connect OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		
		// Add call to call OEP2
		ui.addCall(num: oepNum2)
		
		// Wait a bit and Hang up while the call is ringing
		sleep(3)
		ui.hangup()
		
		// Disconnect the original call with OEP1
		ui.hangup()
	}
	
	func test_5936_AddCallto411() {
		// Test Case: 5936 - Calling - Add Call: The user can add a call to 411
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
	
		// Call to OEP
		ui.call(phnum: oepNum)
		con.answer()
		
		// Add Call to call 411
		ui.addCall(num: "411")
		
		// Disconnect the call with 411 or with VRS hold server
		ui.hangup()
		
		// Disconnect the call with OEP
		ui.hangup()
	}
	
	func test_5937_AddCalltoCIR() {
		// Test Case: 5937 - Calling - Add Call: The user can add a call to CIR
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call to OEP
		ui.call(phnum: oepNum)
		con.answer()
		
		// Add Call to call 611
		ui.addCall(num: "611")
		
		// Disconnect the call with 611 or with VRS hold server
		ui.hangup()
		
		// Disconnect the call with OEP
		ui.hangup()
	}
	
	func test_7172_AddCallRingCountAfterHangup() {
		// Test Case: 7172 - Add Call: When the 1st endpoint hangs up,  while DUT  is connecting to another call, the ring count continues on the 2nd endpoint.
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let _ = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call first endpoint and answer
		ui.call(phnum: oepNum1)
		con1.answer()

		// Calls second endpoint, hang up first endpoint and verify ring count still visible
		ui.addCall(num: oepNum2)
		sleep(1)
		con1.hangup()
		ui.verifyCalling(num: oepNum2)
		
		// Hang up and go home
		ui.hangup()
		ui.gotoHome()
	}
	
	func test_7182_AddCallCalleeIDDuringRing() {
		// Test Case: Test Case: 7182 - Add Call: When a second call is ringing, the name of the correct callee is displayed.
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let _ = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call first endpoint and answer
		ui.call(phnum: oepNum1)
		con1.answer()

		// DUT calls second endpoint and verify callee number visible when ringing
		ui.addCall(num: oepNum2)
		sleep(3)
		ui.verifyCalling(num: oepNum2)
		
		// Hang up and go home
		ui.hangup()
		con1.hangup()
		ui.gotoHome()
	}
	
	func test_8075_AddCalltoVRS() {
		// Test Case: 8075 - Calling - Add Call: The user can add a call to VRS
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call to OEP
		ui.call(phnum: oepNum)
		con.answer()
		
		// Add Call to call VRS
		ui.addCall(num: "18015757669")
		
		// Disconnect the call with with VRS hold server
		ui.hangup()
		
		// Disconnect the call with OEP
		ui.hangup()
	}
	
	func test_8076_AddCallVerifyCallHistory() {
		// Test Case: 8076 - Calling - Add Call:  When the user adds a call, the call history will appear in the correct order
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number, let oepName1 = endpoint1.userName,
			let _ = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }

		// Add Contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 8076", homeNumber: oepNum2, workNumber: "", mobileNumber: "")

		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()

		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()

		// Add Call to OEP2
		ui.addCall(num: oepNum2)
		con1.hangup()
		ui.hangup()
	
		// Verify Call History
		ui.gotoCallHistory(list: "All")
		let nvpCallHistory = ui.formatPhoneNumber(num: oepNum1) + " " + oepName1
		let npcCallHistory = ui.formatPhoneNumber(num: oepNum2) + " " + "Automate 8076"
		let callHistoryList = [npcCallHistory, nvpCallHistory]
		ui.verifyCallHistoryOrder(list: callHistoryList)
		ui.verifyCallHistory(name: oepName1, num: oepNum1)
		ui.verifyCallHistory(name: "Automate 8076", num: oepNum2)
		
		// Cleanup
		ui.deleteAllCallHistory()
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 8076")
		ui.gotoHome()
	}
	
	func test_8081_AddCallCantAddSelf() {
		// Test Case: 8081 - Calling - Add Call: When a User adds a call to another endpoint but recieves an error the user is sent back to the first call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		
		// Add call to self
		ui.addCall(num: dutNum)
		ui.verifyCannotDialOwnNumber()
		
		// Hangup
		ui.hangup()
	}
	
	func test_8095_AddCallCantSwitchCalls() {
		// Test Case: 9085 - Calling - Add Call: The user is not able to switch cals, while ringing
		// to the remote endpoint
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let _ = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Connect the call with OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		
		// Add call to OEP2 and still ringing
		ui.addCall(num: oepNum2)
		
		// Verify that the call switch button is not enabled while the call is ringing with nPC
		ui.verifySwitchCallNotEnabled()
		sleep(3)
		
		// Hang up to disconnect the outbound call to nPC
		ui.hangup()
		
		// Call disconnect with nVP
		ui.hangup()
	}
	
	func test_8096_AddCallSendingSignMail() {
		// Test Case: 8096 - Calling - Add Call: When adding a call if the recipient
		// declines the call, the call will go to signmail and after signmail is finished
		// the original call will resume
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
	
		// Connect the original call with OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
	
		// Add call to OEP2
		ui.addCall(num: oepNum2)
		con2.rejectCall()
	
		// Standby for a bit to allow the signmail to record, then exit
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 15, response: "Exit")
	
		// Goes back to the original call and standby a bit to hang up
		sleep(3)
		ui.hangup()
	}
	
	func test_8098_AddCallLeavingSignMail() {
		// Test Case: 8098 - Calling - Add Call: If the party on hold drops
		// the call while caller is leaving SignMail, the call with the
		// SignMail will continue
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }

		// Connect the call with OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		
		// Add Call to OEP2
		ui.addCall(num: oepNum2)
		con2.rejectCall()
		ui.waitForGreeting()
		ui.skipGreeting()
		ui.waitForSignMailRecording()
		sleep(15)
		
		// Wait until the call is declined and verify that it goes to SignMail Greeting
		// The original call will be disconnected while at the SignMail process
		con1.hangup()
		
		// Select Hangup and Exit
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")
	}
	
	func test_8445_AddCallNotDisplayedBlockList() {
		// Test case 8445: Verify that the 'block list' doesn't exist in the add call
		// interface
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Connect the call with OEP
		ui.call(phnum: oepNum)
		con.answer()
		
		// Open up the Add Call Interface
		ui.selectAddCall()
		
		// Verify taht the block list is not showing in the Add Call interface
		ui.verifyBlockListNotDisplaying()
		
		// Close the Add CAll interface and go back to the original call
		ui.selectCancel()
		
		// Disconnect the call
		ui.hangup()
	}
	
	func test_8448_AddCallShareTextAvailability() {
		// Test Case: Calling - Add Call: Shared text is available
		// after returning from a VRS Call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Connect the call with OEP
		ui.call(phnum: oepNum)
		con.answer()
		
		// Add call to call VRS and be in the hold server
		ui.addCall(num: "15155159999")
		sleep(3)
		
		// Disconnect the add call to leave the hold server and go
		// back to the original call
		ui.hangup()
		
		// Type something in the shared text
		ui.shareText(text: "Can you read this message?")
		
		// Disconnect the call
		ui.hangup()
	}
	
	func test_9553_AddCallOffline() {
		// Test Case: 9553 - Add Call: Options after add call are correct
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)

		// Add Call to offline number and hangup
		ui.addCall(num: "15556335463")
		ui.hangup()

		// Hangup
		ui.hangup()
		ui.gotoHome()
	}
}
