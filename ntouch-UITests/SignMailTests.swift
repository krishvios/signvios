//
//  SignMailTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 5/15/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import XCTest

class SignMailTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["signMailAltNum"]!
	let altNum2 = UserConfig()["signMailAltNum2"]!
	let altName = "iOS Automation"
	let altName2 = "iOS Automation 2"
	let offlineNum = "15556335463"
	let fullMailboxNum = "15556335464"
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
	
	func test_0494_0545_ReceiveDeleteSignMail() {
		// Test Case: 494 - SignMail: Delete All
		// Test Case: 545 - Inbox View: Signmail Message lists automatically update when new messages arrive
		
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Place call to DUT num and send signmail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 5, response: "Send")
		
		// Place 2nd call to DUT num and send signmail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 5, response: "Send")
		
		// Test Case:
		// Login to DUT account to play message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Play SignMail
		ui.gotoSignMail()
		ui.verifySignMail(count: 2)
		
		// Test Case: 494 - Videos: Delete All
		ui.deleteAllSignMail()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_0495_0551_ReturnCallDeleteUnwatched() {
		// Test Case: 495 - SignMail: Delete an unwatched video
		// Test Case: 551 - SignMail: Video Details Screen: “Return Call” button initiates a call successfully
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

		// Login to DUT account to delete message
		ui.switchAccounts(number: dutNum, password: defaultPassword)

		// Test 551
		// Return Call
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.callFromSignMail(phnum: altNum)
		ui.hangup()
		
		// Test 495
		// Delete SignMail
		ui.gotoSignMail()
		ui.deleteSignMail(phnum: altNum)

		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
    }
	
	func test_0563_ContactInfo() {
		// Test Case: 563 - SignMail: Signmail details are that of a contact if found to match
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Delete SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Send SignMail
		ui.sendSignMailToNum(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 15, response: "Send")

		// Login DUT
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Verify SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.verifySignMail(phnum: altNum, name: altName)
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 563", homeNumber: altNum, workNumber: "", mobileNumber: "")

		// Verify SignMail
		ui.gotoSignMail()
		ui.verifySignMail(phnum: altNum, name: "Automate 563")
		
		// Edit Contact
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 563")
		ui.removeContactName(name: "Automate 563")
		ui.enterContact(name: "Automate 563A")
		ui.saveContact()
		
		// Verify SignMail
		ui.gotoSignMail()
		ui.verifySignMail(phnum: altNum, name: "Automate 563A")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 563A")
		
		// Verify SignMail
		ui.gotoSignMail()
		ui.verifySignMail(phnum: altNum, name: altName)
		
		// Cleanup
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_0565_0498_1991_DeleteWatchedUnwatchedSignMails() {
		// Test Case: 565 - SignMail: Deleting Videos
		// Test Case: 498 - SignMail: Orientation during playback
		// Test Case: 1991 - SignMail: Video Details View: After video/signmail playback the app stays on the signmail view
		
		// Delete SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login 1st alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Send 1st SignMail
		ui.sendSignMailToNum(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 15, response: "Send")
		
		// Login 2nd alt account to send message
		ui.switchAccounts(number: altNum2, password: defaultPassword)
		
		// Send 2nd SignMail
		ui.sendSignMailToNum(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 15, response: "Send")
		
		// Login DUT
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// View 1st SignMail
		ui.gotoSignMail()
		ui.playMessage(phnum: altNum)
		
		// Test 498 & 1991
		// Rotate while playing
		ui.rotate(dir: "Left")
		ui.rotate(dir: "Right")
		ui.rotate(dir: "Portrait")
		ui.selectDone()
		
		// Test 565
		// Delete SignMail watched and unwatched
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_0820_7482_PlaySignMailReceiveCall() {
		// Test Case: 820 - SignMail: Videophone receives call while viewing SignMail
		// Test Case: 7482 - SignMail: incoming calls don't disappear when received while watching SignMail
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Place call to DUT num
		ui.call(phnum: dutNum)

		// Skip greeting and send signmail
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 15, response: "Send")

		// Login to DUT account to delete message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Play SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.playMessage(phnum: altNum)
		
		// Receive Call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		ui.selectDone()
		
		// Cleanup
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_0883_2712_4063_5186_7150_PlaySignMailSelectDone() {
		// Test Case: 883 - SignMail: Video marked as viewed when not completely watched
		// Test Case: 2712 - SignMail: New SignMail notification message text
		// Test Case: 4063 - SignMail: Selecting the same signmail twice causes no issues
		// Test Case: 5186 - SignMail: User is able to view SignMail videos
		// Test Case: 7150 - SignMail: DUT will not crash randomly on SignMail when messages are occupying the list
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Place call to DUT num
		ui.call(phnum: dutNum)

		// Skip greeting and send signmail
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 15, response: "Send")

		// Login to DUT account to delete message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Test 2712
		// Verify SignMail Badge
		ui.ntouchActivate()
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.verifySignMailBadge(count: "1")
		
		// Test 5186
		// Play SignMail
		ui.verifySignMailUnwatched(phnum: altNum)
		ui.playMessage(phnum: altNum)

		// Test 883
		// Press Done and Verify watched
		ui.selectDone()
		ui.verifySignMailWatched(phnum: altNum)
		
		// Test 4063
		// Play SignMail Again
		ui.playMessage(phnum: altNum)
		sleep(10)
		ui.selectDone()
		
		// Test 7150
		// Switch tabs
		ui.gotoHome()
		ui.gotoSorensonContacts()
		ui.gotoCallHistory(list: "All")

		// Cleanup
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_2231_RingoutGreeting() {
		// Test Case: 2231 - SignMail: SignMail Greeting will begin after call reaches the set ring count for the remote endpoint
		// Place Call, no answer
		let endpoint = endpointsClient.checkOut()
		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		
		// Skip Greeting and Exit
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Exit")
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_2234_SignMailDisabled() {
//		// Test Case: 2234 - SignMail: Calling a remote endpoint with Video Center Disabled will not allow for SignMail to be left
//		// Logout
//		ui.logout()
//		
//		// Disable SignMail
//		core.setVideoCenter(value: "0")
//		
//		// Attempt to send SignMail
//		ui.login(phnum: altNum, password: defaultPassword)
//		ui.call(phnum: dutNum)
//		ui.verifyCallFailed()
//		
//		// Cleanup
//		core.setVideoCenter(value: "1")
//		ui.switchAccounts(number: dutNum, password: defaultPassword)
//		ui.ntouchActivate()
//		ui.gotoCallHistory(list: "All")
//		ui.deleteAllCallHistory()
//		ui.gotoHome()
//	}
	
	func test_2235_2460_2866_MailboxFull() {
		// Test Case: 2235 - SignMail: Notification will be received if a user is unable to leave a SignMail due to a full mailbox
		// Test Case: 2460 - SignMail: Clicking OK to mailbox full message will return user to previous window
		// Test Case: 2866 - SignMail: New SignMail notification message text
		// Place Call, no answer
		ui.call(phnum: fullMailboxNum)
		sleep(5)
		
		// Verify Mailbox full
		ui.verifyMailboxFull(response: "Cancel")
		ui.gotoHome()
	}
	
	func test_2445_2857_4075_4144_7169_7506_9538_VerifyRecordingSendOptions() {
		// Test Case: 2445 - SignMail: Device displays a red REC indicator with a blinking dot when recording a SignMail
		// Test Case: 2857 - SignMail: Verify the user is unable to Share Location during SignMail recording
		// Test Case: 4075 - SignMail: UI: Verify options to Send, Re-Record, or Exit are appearing after recording SignMail
		// Test Case: 4144 - SignMail: Clicking Record Again in SignMail allows you to record a new SignMail
		// Test Case: 7169 - SignMail: When recording a SignMail user cannot add call
		// Test Case: 7506 - Signmail: incoming calls don't disappear when received while watching signmail
		// Test Case: 9538 - Signmail: Send, rerecord, exit options displayed after user finished recording in either landscape or portrait mode
		
		// Place Call and verify greeting
		ui.call(phnum: offlineNum)
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		ui.skipGreeting()
		
		// Test 2445
		// Verify Recording
		sleep(10)
		ui.verifySignMailRecording()
		
		// Test 2857
		// Verify No Share Options
		ui.verifyNoShareOptions()
		
		// Test 7169 & 7506
		ui.verifyNoCallOptions()
		
		// Test 4075 + 9538
		// Verify Send Options
		ui.hangup()
		ui.verifySignMailSendOptions()
		ui.rotate(dir: "Left")
		ui.verifySignMailSendOptions()
		ui.rotate(dir: "Portrait")
		
		// Test 4144
		// Record Again
		ui.confirmSignMailSend(response: "Rerecord")
		
		// Cleanup
		ui.leaveSignMailRecording(time: 10, response: "Exit", doSkip: false)
		ui.gotoHome()
	}
	
	func test_2688_3186_3545_4095_4451_NoAnswerVerifyPSMG() {
		// Test Case: 2688 - SignMail: Multiple calls to unavailable phone will show SignMail greeting every time
		// Test Case: 3186 - SignMail: The DUT will play the correct signmail greeting
		// Test Case: 3545 - SignMail: Skip text only SignMail greeting
		// Test Case: 4095 - SignMail: Verify text only greeting shows
		// Test Case: 4451 - SignMail: When making repeated calls from call history, text on a PSMG with text and video will still appear
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Test 3186 & 4095
		// Place call and verify greeting
		ui.call(phnum: offlineNum)
		
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		ui.hangup()
		
		// Test 2688, 3445 & 4451
		// Call from history and verify greeting
		ui.gotoCallHistory(list: "All")
		ui.callFromHistory(num: offlineNum)
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		ui.leaveSignMailRecording(time: 10, response: "Exit")
		ui.gotoHome()
	}
	
	func test_3362_PlaySignMailReturnCall() {
		// Test Case: 3362 - SignMail: SignMail message plays after calling the caller back
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
		
		// Login to DUT account to play message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Play SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.playMessage(phnum: altNum)
		sleep(3)
		
		// Return call
		testUtils.calleeNum = altNum
		ui.callFromSignMail()
		sleep(3)
		ui.hangup()
		
		// Play SignMail
		ui.gotoSignMail()
		ui.playMessage(phnum: altNum)
		sleep(3)
		ui.selectDone()

		// Cleanup
		ui.gotoSignMail()
		ui.deleteSignMail(phnum: altNum)
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_3440_3952_DeleteSignMails() {
		// Test Case: 3440 - SignMail: Delete an individual signmail
		// Test Case: 3952 - SignMail: Verify that deleting SignMails does not cause any problems
		// Delete All SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Send 10 SignMails
		for i in 1...10 {
			print("Send SignMail: " +  String(i))
			ui.sendSignMailToNum(phnum: dutNum)
			ui.leaveSignMailRecording(time: 5, response: "Send", doSkip: false)
		}

		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Delete each SignMail
		ui.gotoSignMail()
		ui.verifySignMail(count: 10)
		ui.deleteAllSignMailInList()
		ui.gotoHome()
	}
	
	func test_3441_5289_PlaySignMailLandscape() {
		// Test Case: 3441 - SignMail: Rotating while viewing a SignMail
		// Test Case: 5289 - SignMail: Player controls display when user taps video in Landscape
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Send SignMail
		ui.sendSignMailToNum(phnum: dutNum)
		ui.leaveSignMailRecording(time: 20, response: "Send", doSkip: false)

		// Login to DUT account to play message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Play SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		
		// Verify Playback Controls
		ui.playMessage(phnum: altNum)
		ui.verifyPlaybackControls()
		
		// Full Screen - Verify Playback Controls and rotate
		ui.openFullScreen()
		ui.rotate(dir: "Left")
		ui.verifyPlaybackControls()
		ui.rotate(dir: "Portrait")
		ui.verifyPlaybackControls()
		
		// Cleanup
		app.swipeDown()
		ui.selectDone()
		ui.gotoSignMail()
		ui.deleteSignMail(phnum: altNum)
		ui.gotoHome()
	}
	
	func test_3986_4064_PlaybackControls() {
		// Test Case: 3986 - SignMail: Verify play button appears when video is not playing and pause button appears when video is playing
		// Test Case: 4064 - SignMail: Signmail controls will auto hide
		// Delete All SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Send SignMail
		ui.sendSignMailToNum(phnum: dutNum)
		ui.leaveSignMailRecording(time: 30, response: "Send", doSkip: false)

		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Play SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.playMessage(phnum: altNum)
		
		// Test 4064
		// Verify Playback Controls
		ui.verifyPlaybackControls()
		
		// Test 3986
		// Select Pause and Play buttons
		ui.pauseMessage()
		ui.playMessage()
		ui.selectDone()

		// Cleanup
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_4199_BlockedSignMail() {
		// Test Case: 4199 - SignMail: After deleting a Blocked Contact the blocked icon should not display on SignMail
		// Delete All SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Add Contact
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 4199", homeNumber: altNum, workNumber: "", mobileNumber: "")

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Send SignMail
		ui.sendSignMailToNum(phnum: dutNum)
		ui.leaveSignMailRecording(time: 10, response: "Send", doSkip: false)

		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)

		// Block Contact
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 4199")
		
		// Verify SignMail Blocked
		ui.heartbeat() // refresh UI
		ui.gotoSignMail()
		ui.verifyBlockedSignMail(name: "Automate 4199")
		
		// Delete Blocked Number
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 4199")

		// Verify SignMail not blocked
		ui.ntouchRestart() // refresh UI
		ui.gotoSignMail()
		ui.verifyNoBlockedSignMail(name: "Automate 4199")

		// Cleanup
		ui.deleteAllSignMail()
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 4199")
		ui.gotoHome()
	}
	
	func test_4395_4396_NoSignMail() {
		// Test Case: 4395 - SignMail: No SignMails appears once when searching SignMails without any SignMails available
		// Test Case: 4396 - SignMail: No SignMails message appears only when there are no SignMails
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Send SignMail
		ui.sendSignMailToNum(phnum: dutNum)
		ui.leaveSignMailRecording(time: 10, response: "Send", doSkip: false)
		
		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		
		// Test 4395
		// Search SignMail and Verify No SignMail
		ui.searchSignMail(item: vrsNum)
		ui.verifyNoSignMail()
		ui.selectCancel()
		
		// Test 4396
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Cleanup
		ui.gotoHome()
	}
	
	func test_7103_7473_AddContactFromSignMail() {
		// Test Case: 7103 - SignMail: Creating contact from signmail, name does  appear on contact to be created
		// Test Case: 7473 - Signmail: adding a new contact from signmail populates name
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Send SignMail
		ui.sendSignMailToNum(phnum: dutNum)
		ui.leaveSignMailRecording(time: 10, response: "Send", doSkip: false)
		
		// Login to DUT account to verify message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Wait for SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		
		// Test 7103
		// Add Contact From SignMail
		ui.gotoSignMail()
		ui.addContactFromSignMail(phnum: altNum, contactName: "")
		
		// Test 7473
		// verify Contact
		ui.gotoSorensonContacts()
		ui.selectContact(name: altName)
		ui.verifyContact(name: altName)
		ui.verifyContact(phnum: altNum, type: "home")
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: altName)
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_7757_7768_RecMaxSignMailOptions() {
		// Test Case: 7757 - SignMail: User is able to show options of the signmail
		// Test Case: 7768 - SignMail: User is able to record a full 2 minute signmail
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Test 7768
		// Send Message
		ui.sendSignMailToNum(phnum: dutNum)
		ui.leaveSignMailRecording(time: 120, response: "Send", doSkip: false)

		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)

		// Wait for SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		
		// Test 7757
		// Verify SignMail Options
		ui.selectSignMail(phnum: altNum)
		ui.verifySignMailOptions()
		
		// Cleanup
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_8441_ReRecordVerifySwitchCamera() {
		// Test Case: 8441 - SignMail: Switch Camera UI Button is displayed while re-recording a signmail

		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Record SignMail, select Rerecord
		ui.call(phnum: dutNum)
		ui.leaveSignMailRecording(time: 10,response: "Rerecord")

		// Verify Switch Camera Button and exit
		sleep(10)
		ui.verifySwitchCameraButton()
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")

		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}

	func test_8443_AddContactNameFromSignMail() {
		// Test Case: 8443 - SignMail: Contact Name is updated on the signmail list when adding as a contact
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Call DUT, skip greeting and send signmail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")
		
		// Login to DUT account to verify message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Wait for SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		
		// Add Contact From SignMail
		ui.gotoSignMail()
		ui.addContactFromSignMail(phnum: altNum, contactName: "Automate 8443")
		
		// verify Contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Automate 8443")
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 8443")
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_8640_AddCallSignMail() {
		// Test Case: 8640 - SignMail: When in a call, adding a new call and trying to leave a SignMail works
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		
		// Add Call to offline number
		ui.addCall(num: offlineNum)
		
		// Verify SignMail
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		ui.leaveSignMailRecording(time: 10, response: "Exit")
		
		// Hangup Call
		sleep(5)
		ui.hangup()
		ui.gotoHome()
	}
	
	func test_9506_PlaySignMailTwice() {
		// Test Case: 9506 - Signmail: App does not crash if you play 2 signmails in a row

		// Delete SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Log out and log in with a different account
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Send SignMail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")

		// Logout and back in with original account
		ui.switchAccounts(number: dutNum, password: defaultPassword)

		// Navigate to SignMail and play SignMail twice without app crashing at any point
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.playMessage(phnum: altNum)
		sleep(10)
		ui.selectDone()
		ui.waitForSignMail(phnum: altNum)
		ui.playMessage(phnum: altNum)
		sleep(5)
		ui.selectDone()
		
		// Clean up
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_9657_RotateSignMailWhileViewing() {
		// Test Case: 9657 - SignMail: Rotations on IOS 13+ are working while viewing SignMail

		// Delete SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Log out and log in with a different account
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Send SignMail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")

		// Logout and back in with original account and
		// verify if rotation is possible when viewing SignMail
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.playMessage(phnum: altNum)
		ui.verifyPlaybackControls()
		ui.openFullScreen()
		sleep(1)
		ui.rotate(dir: "Left")
		ui.verifyOrientation(isLandscape: true)
		
		// Cleanup
		ui.selectDone()
		ui.selectDone()
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
}
