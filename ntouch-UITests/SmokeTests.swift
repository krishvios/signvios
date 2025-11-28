//
//  SmokeTests.swift
//  ntouch
//
//  Created by Mikael Leppanen on 11/25/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

import XCTest

class SmokeTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let endpointsClient = TestEndpointsClient.shared
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["smokeTestsAltNum"]!
	
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
	
	func test_0558_AddEditDeleteContact() {
		// Test Case: Test Case: 558 - Contacts: Add/Edit/Delete
		// Delete All Contacts
		ui.deleteAllContacts()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 558", homeNumber: "18015757669", workNumber: "", mobileNumber: "")
		
		// Edit Contact
		ui.editContact(name: "Automate 558")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: "15553334444", type: "home")
		ui.saveContact()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 558")
		ui.gotoHome()
	}
	
	func test_0598_VideoPrivacy() {
		// Test Case: 598 - Calling: Video Privacy - Hold and Resume
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }

		// Call nVP
		ui.call(phnum: vpNum)
		con.answer()
		sleep(5)

		// Enable Video Privacy
		con.videoPrivacy(setting: "On")

		// Verify Video Privacy
		ui.verifyVideoPrivacyEnabled()

		// Disable Video Privacy
		con.videoPrivacy(setting: "Off")
		sleep(5)

		// Verify Video Privacy Disabled
		ui.verifyVideoPrivacyDisabled()

		// Hangup
		ui.hangup()
	}

	func test_0968_CallCIR() {
		// Test Case: 968 - Customer Care Call - logged in
		// Call CIR
		ui.call(phnum: "611")
//		ui.call(phnum: "18013868500")
		ui.waitForCallOptions()
		
		// Verify CallerID
		ui.verifyCallerID(text: "Customer Care")
		
		// Hangup
		ui.hangup()
	}
	
	func test_1351_CallFromNVP() {
		// Test Case: 1351 - Calling: Receiving a Call - nVP
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		
		// Hangup
		ui.hangup()
	}
	
	func test_1352_CallFromNPC() {
		// Test Case: 1352 - Calling: Receiving a Call - nPC
		let endpoint = endpointsClient.checkOut(.pc)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }

		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_1353_CallFromAndroid() {
		// Test Case: 1353 - Calling: Receiving a Call - Android
		let endpoint = endpointsClient.checkOut(.android)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }

		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_1355_CallFromiOS() {
		// Test Case: 1355 - Calling: Receiving a Call - iOS
		let endpoint = endpointsClient.checkOut(.ios)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }

		// Call DUT
		con.dial(number: dutNum)

		// Answer
		ui.incomingCall(response: "Answer")
		sleep(5)

		// Hangup
		ui.hangup()
	}

	func test_2171_CallAndroid() {
		// Test Case: 2171 - Calling: Call to Android
		let endpoint = endpointsClient.checkOut(.android)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let androidNum = endpoint.number else { return }

		// Call Android
		ui.call(phnum: androidNum)
		con.answer()
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_2173_CallNVP() {
		// Test Case: 2173 - Calling: Call to NVP
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }

		// Call nVP
		ui.call(phnum: vpNum)
		con.answer()
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_2175_CallNPC() {
		// Test Case: 2175 - Calling: Call to NPC
		let endpoint = endpointsClient.checkOut(.pc)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let pcNum = endpoint.number else { return }

		// Call nPC
		ui.call(phnum: pcNum)
		con.answer()
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_2176_CalliOS() {
		// Test Case: 2176 - Calling: Call to iOS
		let endpoint = endpointsClient.checkOut(.ios)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let iosNum = endpoint.number else { return }

		// Call iOS
		ui.call(phnum: iosNum)
		con.answer()
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_2705_CallHoldServer() {
		// Test Case: 2705 - VRS: SIP VRS calls connect to hold server
		// Place VRS Call
		ui.call(phnum: "18015757669")
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_2753_Login() {
		// Test Case: 2753 - Sign In: Log out and back in gives no error message
		// Logout
		ui.logout()

		// Login
		ui.login()

		// Go to home
		ui.gotoHome()
	}

	func test_4218_CallFromMac() {
		// Test Case: 1355 - Calling: Receiving a Call - Mac
		let endpoint = endpointsClient.checkOut(.mac)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }

		// Call DUT
		con.dial(number: dutNum)

		// Answer
		ui.incomingCall(response: "Answer")
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_4219_CallMac() {
		// Test Case: 4219 - Calling: Call to Mac
		let endpoint = endpointsClient.checkOut(.mac)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let macNum = endpoint.number else { return }

		// Call Mac
		ui.call(phnum: macNum)
		con.answer()
		sleep(3)

		// Hangup
		ui.hangup()
	}

	func test_7335_496_RecordPlaySignMail() {
		// Test Case: 7335 - Record a SignMail message
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Place call to DUT num
		ui.call(phnum: dutNum)

		// Skip greeting and send signmail
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")

		// Test Case: 496 - Delete a Viewed SignMail
		// Login to DUT account to play message
		ui.switchAccounts(number: dutNum, password: defaultPassword)

		// Play SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.playMessage(phnum: altNum)
		sleep(15)
		ui.selectDone()

		// Cleanup
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
}
