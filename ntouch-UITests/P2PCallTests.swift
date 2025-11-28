//
//  P2PCallTests.swift
//  ntouch
//
//  Created by Mikael Leppanen on 10/13/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import XCTest

class P2PCallTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let offlineNum = "15556335463"
	let fullMailbox = "15556335464"
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
	
	func test_0554_CallFromVP2() {
		// Test Case: 554 - Calling: Receiving a Call - VP2
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
	
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		
		// Hangup
		ui.hangup()
	}
	
	func test_0555_CallContact() {
		// Test Case: 555 - Calling: Calling Contact
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.mac)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let vpNum = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let macNum = endpoint2.number else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 555", homeNumber: vpNum, workNumber: macNum, mobileNumber: "")
		
		// Call Contact home number
		testUtils.calleeNum = vpNum
		ui.callContact(name: "Automate 555", type: "home")
		con1.answer()
		sleep(3)
		ui.verifyCallerID(text: "Automate 555")
		ui.hangup()
		
		// Call Contact from history
		ui.gotoCallHistory(list: "All")
		ui.callFromHistory(name: "Automate 555", num: vpNum)
		con1.answer()
		sleep(3)
		ui.verifyCallerID(text: "Automate 555")
		ui.hangup()
		
		// Call Contact work number
		ui.gotoSorensonContacts()
		testUtils.calleeNum = macNum
		ui.callContact(name: "Automate 555", type: "work")
		con2.answer()
		sleep(3)
		ui.verifyCallerID(text: "Automate 555")
		ui.hangup()

		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 555")
	}
	
	func test_0744_0745_CallerID() {
		// Test Case: 744 - Calling: Incoming caller ID (Contact)
		// Test Case: 745 - Calling: Incoming caller ID (not a contact)
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number, let vpName = endpoint.userName else { return }
		
		// Test 744
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 744", homeNumber: vpNum, workNumber: "", mobileNumber: "")
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		
		// Verify Caller ID and Hangup
		ui.verifyCallerID(text: "Automate 744")
		ui.hangup()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 744")
		ui.gotoHome()
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		
		// Verify Caller ID and Hangup
		ui.verifyCallerID(text: vpName)
		ui.hangup()
	}
	
	func test_0747_2736_CallRejectNewCall() {
		// Test Case: 747 - Calling: Making a second call after the first was rejected
		// Test Case: 2736 - Calling: Backgrounding the app after calling and declining the call on remote endpoint
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let vpNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let vpNum2 = endpoint2.number else { return }
		
		// Test 2736
		// Call OEP1
		ui.call(phnum: vpNum1)
		con1.waitForIncomingCall()

		// Background
		ui.pressHomeBtn()

		// Decline
		con1.rejectCall()
		sleep(3)
		ui.ntouchActivate()

		// Exit from SignMail
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 5, response: "Exit")

		// Test 747
		// Call OEP2
		ui.call(phnum: vpNum2)
		con2.answer()
		sleep(3)
		ui.hangup()
		ui.gotoHome()
	}
	
	func test_0748_VideoPrivacy() {
		// Test Case: 748 - Calling: Privacy enabled on remote end
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Call nVP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(5)
		
		// Enable Video Privacy
		con.videoPrivacy(setting: "On")
		sleep(5)
		
		// Verify Video Privacy
		ui.verifyVideoPrivacyEnabled()
		
		// Disable Video Privacy
		con.videoPrivacy(setting: "Off")
		sleep(5)
		
		// Hangup
		ui.hangup()
	}
	
	func test_0749_CallBackgroundHangupCall() {
		// Test Case: 749 - Calling: Make a second call after the first - VP
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Call nVP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Background
		ui.pressHomeBtn()

		// Hangup
		con.hangup()

		// Call nVP
		ui.ntouchActivate()
		ui.hangup()
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}

	func test_0856_CallDisconnectReconnect() {
		// Test Case: 856 - Calling: connect call disconnect reconnect - nVP
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Call nVP and hangup
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		con.hangup()
		
		// Reconnect
		ui.redial()
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_1341_VerifyOutgoingCallToContactCallerID() {
		// Test Case: 1341 - Calling: Place a Point to Point call to a contact shows the name
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Delete All Contacts
		ui.deleteAllContacts()

		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 1341", homeNumber: oepNum, workNumber: "", mobileNumber: "")

		// Call contact
		ui.call(phnum: oepNum)

		// Verify outgoing caller ID
		ui.verifyCallerID(text: "Automate 1341")

		// Cleanup
		ui.hangup()
		ui.deleteAllContacts()
		ui.gotoHome()
	}
	
	func test_1344_VerifyIncomingContactCallerID() {
		// Test Case: 1344 - Calling: Receive a Point to Point call from a contact shows the name
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Delete All Contacts
		ui.deleteAllContacts()

		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 1344", homeNumber: oepNum, workNumber: "", mobileNumber: "")

		// Contact Calls DUT
		con.dial(number: dutNum)

		// Verify incoming caller ID
		ui.waitForIncomingCall(callerID: "Automate 1344")

		// cleanup
		con.hangup()
		ui.deleteAllContacts()
		ui.gotoHome()
	}
	
	func test_1356_EndCallNVP() {
		// Test Case: 1356 - Calling: End a Call - NVP
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Call remote and hangup
		ui.call(phnum: vpNum)
		sleep(3)
		ui.hangup()
		
		// Call remote and reject
		ui.redial()
		sleep(3)
		con.rejectCall()
		ui.hangup()
		
		// Call DUT, answer, DUT hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call DUT, answer, remote hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		con.hangup()
	}
	
	func test_1357_EndCallNPC() {
		// Test Case: 1357 - Calling: End a Call - NPC
		let endpoint = endpointsClient.checkOut(.pc)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let pcNum = endpoint.number else { return }
		
		// Call remote and hangup
		ui.call(phnum: pcNum)
		sleep(3)
		ui.hangup()
		
		// Call remote and reject
		ui.redial()
		sleep(3)
		con.rejectCall()
		ui.hangup()
		
		// Call DUT, answer, DUT hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call DUT, answer, remote hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		con.hangup()
	}
	
	func test_1358_EndCallAndroid() {
		// Test Case: 1358 - Calling: End a Call - Android
		let endpoint = endpointsClient.checkOut(.android)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let andNum = endpoint.number else { return }
		
		// Call remote and hangup
		ui.call(phnum: andNum)
		sleep(3)
		ui.hangup()
		
		// Call remote and reject
		ui.redial()
		sleep(3)
		con.rejectCall()
		ui.hangup()
		
		// Call DUT, answer, DUT hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call DUT, answer, remote hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		con.hangup()
	}
	
	func test_1360_EndCalliOS() {
		// Test Case: 1360 - Calling: End a Call - iOS
		let endpoint = endpointsClient.checkOut(.ios)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let iosNum = endpoint.number else { return }
		
		// Call remote and hangup
		ui.call(phnum: iosNum)
		sleep(3)
		ui.hangup()
		
		// Call remote and reject
		ui.redial()
		sleep(3)
		con.rejectCall()
		ui.hangup()
		
		// Call DUT, answer, DUT hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call DUT, answer, remote hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		con.hangup()
	}
	
	func test_1371_CallNVP4x() {
		// Test Case: 1371 - Calling: Call back multiple times - NVP
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(20)
		ui.hangup()
		
		// Redial 3x
		for _ in 1...3 {
			ui.redial()
			con.answer()
			ui.waitForCallOptions()
			sleep(20)
			ui.hangup()
		}
	}
	
	func test_1372_CallNPC4x() {
		// Test Case: 1372 - Calling: Call back multiple times - NPC
		let endpoint = endpointsClient.checkOut(.pc)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let pcNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: pcNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(20)
		ui.hangup()
		
		// Redial 3x
		for _ in 1...3 {
			ui.redial()
			con.answer()
			ui.waitForCallOptions()
			sleep(20)
			ui.hangup()
		}
	}

	func test_1373_CallAndroid4x() {
		// Test Case: 1373 - Calling: Call back multiple times - Android
		let endpoint = endpointsClient.checkOut(.android)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let andNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: andNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(20)
		ui.hangup()
		
		// Redial 3x
		for _ in 1...3 {
			ui.redial()
			con.answer()
			ui.waitForCallOptions()
			sleep(20)
			ui.hangup()
		}
	}

	func test_1375_CalliOS4x() {
		// Test Case: 1375 - Calling: Call back multiple times - iOS
		let endpoint = endpointsClient.checkOut(.ios)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let iosNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: iosNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(20)
		ui.hangup()
		
		// Redial 3x
		for _ in 1...3 {
			ui.redial()
			con.answer()
			ui.waitForCallOptions()
			sleep(20)
			ui.hangup()
		}
	}
	
	func test_1376_CallHangupCallNVP() {
		// Test Case: 1376 - Calling: connect call disconnect reconnect - NVP
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
		
		// Redial
		ui.redial()
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_1377_CallHangupCallNPC() {
		// Test Case: 1377 - Calling: connect call disconnect reconnect - NPC
		let endpoint = endpointsClient.checkOut(.pc)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let pcNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: pcNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
		
		// Redial
		ui.redial()
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_1378_CallHangupCallAndroid() {
		// Test Case: 1378 - Calling: connect call disconnect reconnect - Android
		let endpoint = endpointsClient.checkOut(.android)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let andNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: andNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
		
		// Redial
		ui.redial()
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_1380_CallHangupCalliOS() {
		// Test Case: 1380 - Calling: connect call disconnect reconnect - iOS
		let endpoint = endpointsClient.checkOut(.ios)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let iosNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: iosNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
		
		// Redial
		ui.redial()
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_2172_CalltoVP2() {
		// Test Case: 2172 - Calling: Call to VP2
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Call NVP2
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_2497_NoAnswer() {
		// Test Case: 2497 - Calling: Calling a phone number where the user doesn't answer
		// Place call
		ui.call(phnum: offlineNum)
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		
		// Hangup
		ui.hangup()
	}
	
	func test_2691_DialOwnNumber() {
		// Test Case: 2691 - Calling: Calling own number displays error
		// Verify cannot dial own phone number
		ui.call(phnum: dutNum)
		ui.verifyCannotDialOwnNumber()
		ui.gotoHome()
	}
	
	func test_2701_DisplayCallStats() {
		// Test Case: 2701 - Calling: Calling: Video stats are displayed
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Confirm Stats displayed
		ui.showCallStats()
		sleep(5)
		ui.verifyFrameRateGreaterThan(value: 0)
		ui.hangup()
	}
	
	func test_2755_RejectCalls() {
		// Test Case: 2755 - Calling: Ignoring calls after outbound call ignored
		let endpoint1 = endpointsClient.checkOut(.mac)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let macNum = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Call Mac and Reject
		ui.call(phnum: macNum)
		con1.rejectCall()
		ui.hangup()
		
		// Call DUT and Decline
		con2.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
	}
	
	func test_2909_2CallsFromNVP() {
		// Test Case: 2909 - Calling: Receive two calls from same endpoint
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call 2
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
	}
	
	func test_3021_MailboxFull() {
		// Test Case: 3021 - Calling: Being sent to signmail when calling a device with 200 signmails does not prevent the device from placing additional calls
		// Call
		ui.call(phnum: fullMailbox)
		
		// Verify Mailbox full
		ui.verifyMailboxFull(response: "Cancel")
	}
	
	func test_4220_EndCallMac() {
		// Test Case: 4220 - Calling: End a Call - Mac
		let endpoint = endpointsClient.checkOut(.mac)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let macNum = endpoint.number else { return }
		
		// Call remote and hangup
		ui.call(phnum: macNum)
		sleep(3)
		ui.hangup()
		
		// Call remote and reject
		ui.redial()
		sleep(3)
		con.rejectCall()
		ui.hangup()
		
		// Call DUT, answer, DUT hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call DUT, answer, remote hangup
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		con.hangup()
	}
	
	func test_4223_CallMac4x() {
		// Test Case: 4223 - Calling: Call back multiple times - Mac
		let endpoint = endpointsClient.checkOut(.mac)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let macNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: macNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(20)
		ui.hangup()
		
		// Redial 3x
		for _ in 1...3 {
			ui.redial()
			con.answer()
			ui.waitForCallOptions()
			sleep(20)
			ui.hangup()
		}
	}
	
	func test_4224_CallHangupCallMac() {
		// Test Case: 4224 - Calling: connect call disconnect reconnect - Mac
		let endpoint = endpointsClient.checkOut(.mac)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let macNum = endpoint.number else { return }
		
		// Call
		ui.call(phnum: macNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
		
		// Redial
		ui.redial()
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_7764_CallBackgroundDeclineCall() {
		// Test Case: 7764 - Calling: User is able to call the Remote endpoint back after disconnecting from Signmail while the app was in the background
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Call nVP
		ui.call(phnum: vpNum)
		con.waitForIncomingCall()

		// Background
		ui.pressHomeBtn()

		// Reject and hangup
		con.rejectCall()
		ui.ntouchActivate()
		ui.hangup()

		// Call nVP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_9580_EndCallBeforeAnswer() {
		// Test Case: 9580 - Calling: Hanging up on one end stops ringing on the other
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()

		// Call DUT
		con.dial(number: dutNum)
		ui.waitForIncomingCall()
		con.hangup()

		// Verify Call History
		ui.waitForMissedCallBadge(count: "1")
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(num: vpNum)
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
}
