//
//  EnhancedSignMailTests.swift
//  ntouch
//
//  Created by Mikael Leppanen on 5/3/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import XCTest

class EnhancedSignMailTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum =  UserConfig()["enhancedSignMailAltNum"]!
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
	
	func test_8300_8303_8328_8474_SendOptions() {
		// Test Case: 8300 - Enhanced SignMail: Tapping send more than once
		// Test Case: 8303 - Enhanced SignMail: Tapping outside of the Direct SignMail send options will not cause the Enhanced SignMail to end or be dismissed
		// Test Case: 8328 - Direct SignMail:  UI: Verify options to Send, Re-Record, or Exit are appearing after recording Direct SignMail.
		// Test Case: 8474 - Enhanced SignMail: Enhanced Signmail Icon is visible in the SignMail List
		
		// Delete SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Send SignMail to DUT Number
		ui.sendSignMailToNum(phnum: dutNum)
		sleep(15)
		ui.hangup()
		
		// Test 8303
		// Tap outside of send options
		ui.selectTop()
		
		// Test 8328
		ui.verifySignMailSendOptions()
		
		// Test 8300
		ui.confirmSignMailSendDoubleTap(response: "Send")
		
		// Test 8474
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSignMail()
		
		// Verify Enhanced SignMail icon
		ui.waitForSignMail(phnum: altNum)
		ui.verifyEnhancedSignMailIcon()
		
		// Cleanup
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_8301_MailboxFull() {
		// Test Case: 8301 - Enhanced SignMail: Send Enhanced SignMail to DUT with full inbox
		// Send SignMail to number with full mailbox
		ui.sendSignMailToNum(phnum: "15556335464")
		ui.verifyMailboxFull(response: "Cancel")
		ui.gotoHome()
	}
	
	func test_8302_SkipReadySetGo() {
		// Test Case: 8302 - Enhanced SignMail: User  can skip "Ready , Set, Go!"
		
		if #available(iOS 14, *) {
			
			// Send SignMail to number
			ui.sendSignMailToNum(phnum: altNum)
			
			// Skip greeting
			ui.waitForGreeting()
			ui.leaveSignMailRecording(time: 5, response: "Exit")
			ui.gotoHome()
		}
	}
	
	func test_8305_RecordAgainSend() {
		// Test Case: 8305 - Enhanced SignMail: Verify user is able to send a re-recorded Enhanced Signmail.
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Send SignMail to DUT Number but rerecord after recording
		ui.sendSignMailToNum(phnum: dutNum)
		ui.leaveSignMailRecording(time: 15, response: "Rerecord", doSkip: false)
		
		// Record Again
		ui.leaveSignMailRecording(time: 15, response: "Send", doSkip: false)
		
		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_8307_8325_8331_RecordAgain() {
		// Test Case: 8307 - Enhanced SignMail: Clicking Record Again in Enhanced SignMail allows you to record a new Enhanced SignMail
		// Test Case: 8325 - Enhanced SignMail: Recording again loads the UI properly after pressing end call
		// Test Case: 8331 - Enhanced SignMail: Device displays a red REC indicator with a blinking dot when recording a SignMail
		// Send SignMail to number and begin recording again
		ui.sendSignMailToNum(phnum: cfg["nvpNum"]!)
		ui.leaveSignMailRecording(time: 15, response: "Rerecord", doSkip: false)

		// Exit from rerecorded signmail
		ui.leaveSignMailRecording(time: 15, response: "Exit", doSkip: false)
		ui.gotoHome()
	}
	
	func test_8308_RecordMax() {
		// Test Case: 8308 - Enhanced SignMail: device can record Enhanced SignMail up to the limit set by core
		// Send SignMail to number
		ui.sendSignMailToNum(phnum: altNum)
		sleep(115)
		
		// Verify Recording
		ui.verifySignMailRecording()
		sleep(10)
		
		// Exit
		ui.confirmSignMailSend(response: "Exit")
		ui.gotoHome()
	}
	
	func test_8310_CallOptionsDialpadDisabled() {
		// Test Case: 8310 - Enhanced SignMail: Dial pad icon is greyed out when recording a Direct SignMail
		// Send SignMail to number
		ui.sendSignMailToNum(phnum: cfg["nvpNum"]!)
		sleep(8)
		
		// Verify dialpad disabled in call options
		ui.verifyNoCallOptions()
		
		// Cleanup
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")
		ui.gotoHome()
	}
	
	func test_8322_8330_SendSignMailNoCallOptions() {
		// Test Case: 8322 - Enhanced SignMail: When recording a Enhanced SignMail user cannot add call
		// Test Case: 8330 - Enhanced SignMail: Verify the user is unable to Share Location during Direct SignMail recording
		// Send SignMail to number
		ui.sendSignMailToNum(phnum: altNum)
		sleep(10)
		
		// Verify No Call Options
		ui.verifyNoCallOptions()
		
		// Hangup
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")
		ui.gotoHome()
	}
	
	func test_8336_HangupReadySetGo() {
		// Test Case: 8336 - Enhanced SignMail: Verify hanging up during SignMail "Ready, Set, Go"  is not causing history entries
		// Send SignMail to number
		ui.sendSignMailToNum(phnum: cfg["nvpNum"]!)
		ui.selectTop()
		
		// Hangup during Ready, Set, Go
		ui.hangup()
		ui.gotoHome()
	}
	
	func test_8348_SendSignMailFromCallHistory() {
		// Test Case: 8348 - Enhanced SignMail - History: Send Enhanced SignMail from the Enhanced SignMail's call history "ALL" list
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Add Contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 8348", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		
		// Call VP
		ui.gotoHome()
		ui.call(phnum: cfg["nvpNum"]!)
		ui.hangup()
		
		// Send SignMail to Call History item
		ui.sendSignMailToCallHistoryItem(name: "Automate 8348", num: cfg["nvpNum"]!)
		ui.leaveSignMailRecording(time: 10, response: "Exit", doSkip: false)
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 8348")
		ui.gotoHome()
	}
	
	func test_8349_RecordingDirectSignMailReceiveCall() {
		// Test Case: 8349 - Enhanced SignMail: DUT will be able to send a Enhanced SignMail while another endpoint attempts to call the DUT
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Send SignMail to number
		ui.sendSignMailToNum(phnum: oepNum)
		sleep(10)
		ui.verifySignMailRecording()
		
		// Receive call while recording
		con.dial(number: dutNum)
		sleep(3)
		con.hangup()
		sleep(30)
		
		// Hangup and exit
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")
		
		// Verify Missed Call
		ui.waitForMissedCallBadge(count: "1")
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
	}
	
	func test_8351_8352_SendSignMailFromPhonebook() {
		// Test Case: 8351 - Enhanced SignMail - Contacts: Send Enhanced SignMail from the Enhanced SignMail's Contacts "Favorites"  list
		// Test Case: 8352 - Enhanced SignMail - Contacts: Send Direct SignMail from the Enhanced SignMail's Contacts "Contacts"  list
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact and Favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 8351", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 8351")
		ui.addToFavorites()
		
		// Test 8351
		// Send SignMail to Favorite
		ui.sendSignMailToFavorite(name: "Automate 8351")
		ui.leaveSignMailRecording(time: 10, response: "Exit", doSkip: false)
		
		// Test 8352
		// Send SignMail to Contact
		ui.sendSignMailToContact(name: "Automate 8351")
		ui.leaveSignMailRecording(time: 10, response: "Exit", doSkip: false)
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 8351")
		ui.gotoHome()
	}
	
	func test_8355_SendSignMailFromDialer() {
		// Test Case: 8355 - Enhanced SignMail - DialPad: Send Enhanced  SignMail from the Enhanced SignMail's DialPad tab
		// Send SignMail to number
		ui.sendSignMailToNum(phnum: cfg["nvpNum"]!)
		ui.leaveSignMailRecording(time: 10, response: "Exit", doSkip: false)
		ui.gotoHome()
	}
	
	func test_8364_EnhancedSignMailEnabled() {
		// Test Case: 8364 - Enhanced SignMail - Compatibility: Enhanced SignMail is available in Customer mode
		
		// Verify Enhanced SignMail Enabled
		ui.gotoSignMail()
		ui.verifyEnhancedSignMailEnabled()
		ui.gotoHome()
	}
	
	func test_8372_EnhancedSignMailSearch() {
		// Test Case: 8372 - Enhanced SignMail: Direct SignMail Contacts Tab displays a "Search" icon
		// Go to Enhanced SignMail
		ui.gotoEnhancedSignMailContacts()
		
		// Verify Search on Contacts tab
		ui.verifySearchField()
		
		// Cleanup
		ui.selectCancel()
		ui.gotoHome()
	}
	
	func test_8376_SendSignMailToSelf() {
		// Test Case: 8376 - Enhanced SignMail - Compatibility: User can not select themselves as a recipient of a Enhanced SignMail in Customer Mode
		// Add Contact of self
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 8376", homeNumber: dutNum, workNumber: "", mobileNumber: "")
		
		// Send SignMail to self contact
		ui.sendSignMailToContact(name: "Automate 8376")
		
		// Verify error
		ui.verifyCannotSendSignMailToSelf()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 8376")
	}
	
	func test_8384_SendNoCallHistory() {
		// Test Case: 8384 - Enhanced SignMail - History: Sending a Enhanced SignMail does not result in a call history item
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Send SignMail to DUT Number
		ui.sendSignMailToNum(phnum: dutNum)
		sleep(15)
		ui.hangup()
		ui.confirmSignMailSendDoubleTap(response: "Send")
		
		// Verify no call history
		ui.verifyNoCallHistory(num: dutNum)
		
		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_8389_DisableEnhancedSignMail() {
//		// Test Case: 8389 - Enhanced SignMail: UI button is not displayed when disabled in Core
//		// Disable Enhanced SignMail
//		core.setEnhancedSignMail(value: "0")
//		ui.heartbeat()
//		
//		// Verify Enhanced SignMail Disabled
//		ui.gotoSignMail()
//		ui.verifyEnhancedSignMailDisabled()
//		
//		// Cleanup
//		ui.gotoHome()
//		core.setEnhancedSignMail(value: "1")
//		ui.heartbeat()
//		ui.gotoHome()
//	}
	
	func test_8393_CantSendn11() {
		// Test Case: 8393 - Enhanced SignMail: User is not able to send a Enhanced SignMail to n-11 numbers
		// Attempt to send SignMail to 411
		ui.sendSignMailToNum(phnum: "411")
		
		// Select Cancel after can't send message
		ui.cantSendSignMail(response: "Cancel")
		
		// Cleanup
		ui.selectCancel()
		ui.gotoHome()
	}
	
	func test_8420_EnhancedSignMailSwitchView() {
		// Test Case: 8420 - Direct SignMail: Switch Camera is available while recording a Enhanced SignMail

		// Begin sending SignMail to endpoint
		ui.sendSignMailToNum(phnum: altNum)

		// Verify the switch camera button is visible and can be interacted with
		ui.waitForSignMailRecording()
		ui.verifySwitchCameraButton()
		sleep(3)
		ui.clickButton(name: "SwitchCamera")
		sleep(3)

		// Hang up and cancel signmail
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")
		ui.gotoHome()
	}
	
	func test_8429_EnhancedSignMailDialerBtn() {
		// Test Case: 8429 - Direct SignMail: send button on dialer window is signmail specific
		// Go to Enhanced SignMail Dialer
		ui.gotoEnhancedSignMail()
		ui.verifyEnhancedSignMailDialerBtn()
		ui.selectCancel()
		ui.gotoHome()
	}
	
	func test_8458_SignMailtoHearingNumber() {
		// Test Case: 8458 - Enhanced SignMail: Hearing Numbers are not available as recipients for Enhanced SignMail
		// Create a contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate_8458", homeNumber: "18012879439", workNumber: "", mobileNumber: "")
		
		// Send enhanced SignMail to hearing caller via Contact
		ui.sendSignMailToContact(name: "Automate_8458")
		ui.cantSendSignMail(response: "Cancel")
		
		// Send enhanced SignMail to hearing caller via Dialer
		ui.sendSignMailToNum(phnum: "18012879439")
		ui.cantSendSignMail(response: "Cancel")
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate_8458")
		ui.gotoHome()
	}
	
	func test_8459_EnhancedSignMailCIR() {
		// Test Case: 8459 - Enhanced SignMail: CIR is not available as recipient for Enhanced SignMail
		// Send Enhanced SignMail to CIR via Contact
		ui.sendSignMailToContact(name: "Customer Care")
		ui.cantSendSignMail(response: "Cancel")
		
		// Send enhanced SignMail to CIR via Dailer
		ui.sendSignMailToNum(phnum: "611")
		sleep(3)
		ui.cantSendSignMail(response: "Cancel")
		ui.gotoHome()
	}
	
	func test_8460_EnhancedSignMailTS() {
		// Test Case: 8460 - Enhanced SignMail: Tech Support is not available as recipients for Enhanced SignMail
		
		// send enhanced SignMail to TS via Dialer
		ui.sendSignMailToNum(phnum: "18012879403")
		sleep(3)
		ui.cantSendSignMail(response: "Cancel")
		ui.gotoHome()
	}
	
	func test_8598_MailboxFullCall() {
		// Test Case: 8598 - Enhanced SignMail: Call is placed after selecting Ok when attempting to send a SignMail to User Offline
		// Send Enhanced SignMail
		ui.sendSignMailToNum(phnum: "15556335464")
		
		// Select call after can't send message
		ui.verifyMailboxFull(response: "Call Directly")
		
		// Cancel offline call
		ui.verifyMailboxFull(response: "Cancel")
	}
	
	func test_9329_SendingSignMailFromFavorites() {
		// Test Case: 9329 - Enhanced SignMail: Selecting a favorite can send signmail directly

		// Remove existing contacts
		ui.gotoSorensonContacts()
		ui.deleteAllContacts()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "9329_favorite", homeNumber: altNum, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "9329_favorite")

		// Add to Favorite and Verify Send SignMail from Favorite
		ui.addToFavorites()
		ui.gotoFavorites()
		ui.verifyFavorite(name: "9329_favorite", type: "home")
		ui.clickButton(name: "More Info")
		ui.clickStaticText(text: "Send SignMail")
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 4, response: "Exit", doSkip: false)

		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "9329_favorite")
		ui.gotoHome()
	}
}
