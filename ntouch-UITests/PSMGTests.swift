//
//  PSMGTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 3/30/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class PSMGTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["psmgAltNum"]!
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
		ui.gotoPSMG()
		ui.selectGreeting(type: "Sorenson")
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}
	
 	func test_3391_PSMGAfterDiscardedRecording(){
		// Test Case: 3391 - Review uploaded message after reviewing and discarding a PSMG.
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.createGreeting(time: 5, text: "3391")
		
		// Record SignMail and cancel saving it
		ui.recordGreeting(time: 5, text: "Test")
		ui.greetingRecorded(response: "Cancel")
		ui.verifyGreeting(text: "3391")
	}
	
	func test_3430_3435_3804_RecordPSMG() {
		// Test Case: 3430 - PSMG: Play button enabled after new greeting is recorded
		// Test Case: 3435 - PSMG: Custom recording takes full 30 seconds length
		// Test Case: 3804 - PSMG: DUT is able to record a PSMG. Bug# 16558
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting
		ui.createGreeting(time: 30)
	}
	
	func test_3539_3551_PSMGTextOnly() {
		// Test Case: 3539 - PSMG: Create a Text Only Greeting
		// Test Case: 3551 - PSMG: Text only SignMail greeting has 176 characters
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting with text
		ui.createTextGreeting(text: "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456")
		ui.verifyGreetingTextLength(count: 176)
	}
	
	func test_3542_PSMGTextOnly(){
		// Test Case: 3542 - View Text only SignMail greeting in portrait mode
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.createTextGreeting(text: "3542")
		ui.rotate(dir: "Portrait")
		ui.verifyGreeting(text: "3542")
	}
	
	func test_3552_PSMGTextAndVideoOnly176Characters() {
		// Test Case: 3552 - PSMG: Personal SignMail greeting with text has 176 characters
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Attempt to create a text and video greeting with 177 characters
		ui.recordGreeting(time: 5, text: "176Characters1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345CharactersVerified?")
		ui.greetingRecorded(response: "Save")
		
		// Check to make sure only 176 characters are present. Example: ? is excluded.
		ui.verifyGreetingTextLength(count: 176)
		ui.verifyNoGreeting(text: "176Characters1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345CharactersVerified?")
		ui.verifyGreeting(text: "176Characters1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345CharactersVerified")
	}
	
	func test_3553_3554_RecordPSMGText() {
		// Test Case: 3553 - PSMG: Create a Personal SignMail Greeting with text
		// Test Case: 3554 - PSMG: Upload a Personal SignMail Greeting with text
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting with text
		ui.createGreeting(time: 30, text: "3553 3554")
	}
	
	func test_3558_PSMGGTextDuration() {
		// Test Case: 3558 - Personal SignMail greeting with text duration

		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.createTextGreeting(text: "3558")
		ui.createGreeting(time: 30, text: "3558")
		
		// Switch accounts
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Verify recording
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.verifyGreeting(text: "3558")
		ui.leaveSignMailRecording(time: 3, response: "Exit")
		
		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_3654_PSMGCategories() {
		// Test Case: 3654 -  Initial presented options in PSMG screen are "Sorenson Greeting, Personal Greeting, and No Greeting"
		
		// Setup
		ui.gotoPSMG()
		
		// Verify Greeting types and Only 3 catagories
		ui.verifySignMailGreetingTypeCount(count: 3)
		ui.verifyGreetingBtn(type: "Sorenson")
		ui.verifyGreetingBtn(type: "Personal")
		ui.verifyGreetingBtn(type: "No Greeting")
	}
	
	func test_3659_PSMGEditTextOnly() {
		// Test Case: - 3659 - Text options should have a save feature that allows the text to be edited.

		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.createGreeting(time: 5, text: "3659")
		ui.editTextGreeting(text: "3659_Test")
		
		// Switch accounts
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Call DUT and verify text change
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.verifyGreeting(text: "3659_Test")
		ui.leaveSignMailRecording(time: 2, response: "Exit")
		
		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_3805_NoReturnKeyPSMGTextOnly() {
		// Test Case: - 3805 -  Verify no return button during PSMG text recording is limited.
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.editAndSelectPersonalRecordType(type: "Text Only")
		ui.verifyNoReturnOnKeyboard()
	}
	
	func test_4124_NoRecordOnSorensenDefault() {
		// Test Case: 4124 - Verify record button is not displayed when default greeting is selected.
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Check for disabled editing and no record button on Sorenson default
		ui.selectGreeting(type: "Sorenson")
		ui.verifyIconPencilDisabled()
		ui.verifyNoRecordButton()
	}
	
	func test_4131_PSMGTypeVisibleAfterRecording() {
		// Test Case: 4131 - PSMG: After recording PSMG, greeting type remains on Custom selection.
		
		// Go to PSMG and select Personal Greeting with text
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")

		// Create Greeting an Review
		ui.createGreeting(time: 5)
		ui.playPSMG()
		sleep(10)

		// Verify Greeting remains on the Custom selection
		ui.verifyButtonSelected(name: "Personal", expected: true)
	}
	
	func test_4138_4481_RotateOnPSMGRecordScreen() {
		// Test Case: 4138 - PSMG: DUT can handle rotating while in the Greeting Type page without app crashing.
		// Test Case: 4481 - PSMG: Rotating on the PSMG record screen does not cause the text box to disappear

		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// 4138 - Rotate on Greetings Selection Page and verify app does not crash
		ui.rotate(dir: "Left")
		ui.rotate(dir: "Portrait")
		
		// 4481 - Select a Greeting Type with text, then rotate screen and verify text box is still visible and functional
		ui.editAndSelectPersonalRecordType(type: "Video With Text")
		ui.rotate(dir: "Left")
		ui.rotate(dir: "Portrait")
		ui.createGreeting(time: 5, text: "4481")
	}
	
	func test_4310_PSMGStatusBarVisibleOnRecordAgain() {
		// Test Case: 4310 - PSMG: Status bar does not disappear when recording again
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")

		// Verify timer bar does not disappear on re-record
		ui.createGreeting(time: 30)
		ui.recordGreeting()
		sleep(5)
		ui.verifyProgressBar()

		// Cleanup
		ui.clickButton(name: "Stop")
		ui.greetingRecorded(response: "Cancel")
	}
	
	func test_4370_PSMGVideoWithTextBackspaceOnly() {
		// Test Case: 4370 - PSMG: Video with text, when going to enter text and only selecting backspace then selecting Done app is stable
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.editAndSelectPersonalRecordType(type: "Video With Text")
		
		// Go into enter text but only type backspace/delete
		ui.clearAndEnterText(text: XCUIKeyboardKey.delete.rawValue)
		
		// Click Done button and verify app does not crash after 30 seconds
		ui.clickButton(name: "Done")
		sleep(30)
		ui.gotoHome()
	}
	
	func test_4371_PSMGTypeUnavailableDuringRecording() {
		// Test Case: 4371 - PSMG: PSMG Selector "Video Only, Video with Text, Text Only" is not available once the recording starts
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Check disabled type buttons during recording with Video Only
		ui.editAndSelectPersonalRecordType(type: "Video Only")
		ui.clickButton(name: "Record")
		ui.waitForStopBtn()
		ui.verifyButtonEnabled(name: "Video With Text", expected: false)
		ui.verifyButtonEnabled(name: "Text Only", expected: false)
		sleep(2)
		ui.clickButton(name: "Stop")
		ui.greetingRecorded(response: "Cancel")
		
		// Check disabled buttons during recording with Video With Text
		ui.editAndSelectPersonalRecordType(type: "Video With Text")
		ui.clickButton(name: "Record")
		ui.waitForStopBtn()
		ui.verifyButtonEnabled(name: "Video Only", expected: false)
		ui.verifyButtonEnabled(name: "Text Only", expected: false)
		sleep(2)
		ui.clickButton(name: "Stop")
		ui.greetingRecorded(response: "Cancel")
	}
	
	func test_4637_PSMGRecordButtonVisibleRotation() {
		// Test Case: 4637 - PSMG: On all OS's, the record button does not disappear on rotation.

		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.editAndSelectPersonalRecordType(type: "Video With Text")


		// Attempt to rotate and verify record button is still visible
		ui.rotate(dir: "Left")
		ui.verifyButtonExists(name: "Record")
		ui.rotate(dir: "Right")
		ui.verifyButtonExists(name: "Record")
		ui.rotate(dir: "Portrait")
		ui.verifyButtonExists(name: "Record")
	}
	
	func test_5183_VideoOnlyDoesNotDisplayText() {
		// Test Case: 5183 - PSMG: Verify no text appears when previewing a 'Video only' PSMG when text was previously entered.
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Create a greeting with text, then create a video only and verify no text in preview
		ui.createGreeting(time: 5, text: "5183")
		ui.editAndSelectPersonalRecordType(type: "Video Only")
		ui.recordGreeting(time: 5)
		ui.greetingRecorded(response: "Preview")
		ui.verifyNoGreeting(text: "5183")
	}
	
	func test_5199_MinimizeAppDuringRecordCountdown() {
		// Test Case: 5199 - PSMG: Verify if the app is minimized during PSMG record countdown, ntouch will not crash.
		
		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Begin recording, then immediately minimize app
		ui.editAndSelectPersonalRecordType(type: "Video Only")
		ui.clickButton(name: "Record")
		ui.pressHomeBtn()
		
		// User can return to app to create a PSMG
		ui.ntouchActivate()
		ui.clickButton(name: "Stop")
		ui.greetingRecorded(response: "Save")
	}
	
	func test_5200_PSMGFastStopCancel() {
		// Test Case: 5200 - PSMG: Selecting record, stop, cancel in quick succession, User is still able to record PSMG or receive SignMails
		
		// Select Personal Signmail
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Go through record, stop, cancel in rapidly, then verify can still create new PSMG
		ui.editAndSelectPersonalRecordType(type: "Video Only")
		ui.clickButton(name: "Record")
		ui.waitForStopBtn()
		ui.clickButton(name: "Stop")
		ui.greetingRecorded(response: "Cancel")
		ui.createGreeting(time: 5)
	}

	func test_5234_PSMGFastStopSave() {
		// Test Case: 5234 - PSMG: Recording a new PSMG and selecting stop then save fast saves the PSMG geeting.
		
		// Select Personal Signmail
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Go through record, stop, save in rapidly without app crashing
		ui.editAndSelectPersonalRecordType(type: "Video Only")
		ui.clickButton(name: "Record")
		ui.waitForStopBtn()
		ui.clickButton(name: "Stop")
		ui.greetingRecorded(response: "Save")
	}

	func test_5269_PSMGRecordClickTwice() {
		// Test Case: 5269 - PSMG: ntouch will not crash when record button is selected twice
		
		// Select Personal Signmail
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Record video but double tap record button and ensure app does not crash
		ui.editAndSelectPersonalRecordType(type: "Video Only")
		ui.record(time: 5, doubleTapRecord: true)
		ui.greetingRecorded(response: "Save")
	}

	func test_5272_PSMGTextOnlyMenuSave() {
		// Test Case: 5272 - PSMG: Selecting menu save button with "text only" option does not remove text.
		
		// Select Personal Signmail
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Create text with top right Save button
		ui.createTextGreeting(text: "5272")
		
		// Create text with bottom middle Save button
		ui.createTextGreeting(text: "5272_2", alternateSaveButton: true)
	}

	func test_5273_PSMGTextOnlyRotate() {
		// Test Case: 5273 - PSMG: Saving personal greeting text only and rotating, app will not crash.
		
		// Go to Personal Greeting and create text greeting
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.createTextGreeting(text: "5273")
		
		// Rotate the app and ensure it has not crashed
		ui.rotate(dir: "Right")
		ui.rotate(dir: "Left")
		ui.rotate(dir: "Portrait")
		ui.gotoHome()
	}

	func test_5292_GoToPSMGPersonalAndBackOut() {
		// Test Case: 5292 - PSMG: User is able to Select personal greeting and then back out without the app crashing.
		
		// Go to Personal Greeting page and back out without crashing
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.gotoHome()
	}

	func test_5295_PSMGMenuRotate() {
		// Test Case: 5295 - Rotating while in Greeting Type menu does not crash app.
		
		// Rotate Orientation
		ui.gotoPSMG()
		ui.rotate(dir: "Right")
		ui.rotate(dir: "Left")
		ui.rotate(dir: "Portrait")
		ui.gotoHome()
	}

	func test_7138_7139_SwitchPSMGAfterCallOnPage() {
		// Test Case: 7138 - PSMG: Switching greeting type after receiving a call while on the Signmail Greeting type page works as expected.
		// Test Case: 7139 - PSMG: PSMG functions as expected after receiving a call while on the Signmail Greeting type page.
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Go to PSMG
		ui.gotoPSMG()
		
		// Decline call from OEP
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		
		// 7139 - Check still on PSMG Screen and UI is functional
		ui.playPSMG()
		sleep(25)
		
		// 7138/7139 - Check changing greeting type is also functional
		ui.selectGreeting(type: "Personal")
		ui.selectGreeting(type: "No Greeting")
		ui.selectGreeting(type: "Sorenson")
	}

	func test_7251_SavePSMGTextAfterKeyboardDismissal() {
		// Test Case: 7251 - PSMG: Dismissing keyboard after editing text causes text to be savable.
		
		// Go to PSMG
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		
		// Save text by tapping away and using the "Save" button instead
		ui.recordGreeting(time: 5, text: "7251", alternateTextSave: true)
		ui.greetingRecorded(response: "Save")
	}
	
	func test_7474_TextInCall() {
		// Test Case: 7474 - PSMG: Text only greeting with another call on hold doesn't crash
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Create Greeting with text
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.createTextGreeting(text: "7474")
		
		// Switch accounts
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Call DUT for PSMG Text
		ui.addCall(num: dutNum)
		ui.waitForGreeting(textOnly: true, text: "7474")
		
		// Interact with Screen and note app does not crash
		ui.leaveSignMailRecording(time: 2, response: "Exit")
		ui.hangup()

		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}

	func test_7475_TextAndVideoInCall() {
		// Test Case: 7475 - PSMG: view a text and video PSMG while in a call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Create Greeting with text
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.createGreeting(time: 10, text: "7475")
		
		// Switch accounts
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Call DUT for PSMG Text Video
		ui.addCall(num: dutNum)
		ui.waitForGreeting()
 		ui.verifyGreeting(text: "7475")
		ui.leaveSignMailRecording(time: 2, response: "Exit")
		ui.hangup()

		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_7495_RecordPSMGInPortraitOrientation() {
		// Test Case: 7495 - PSMG: User is able to record a PSMG while in portrait mode
		
		// Go to PSMG and Record PSMG
		ui.gotoPSMG()
		ui.rotate(dir: "Portrait")
		ui.selectGreeting(type: "Personal")
		ui.recordGreeting(time: 5)
		ui.greetingRecorded(response: "Cancel")
	}
	
	func test_8140_IncomingCallWhileUploadPSMG() {
		// Test Case: 8140 - SignMail: Incoming call while on upload greeting
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Record SignMail and receive call
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")
		ui.recordGreeting(time: 5)
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		ui.gotoHome()
	}
	
	func test_8163_PSMGSkipOnCall() {
		// Test Case: 8163 - After playing the DUT PSMG, skip greeting works on the next call.
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Play Personal/Soresnon PSMG
		ui.gotoPSMG()
		ui.selectGreeting(type: "Sorenson")
		ui.playPSMG()
		sleep(25)
		
		// Call OEP and skip the greeting
		ui.call(phnum: oepNum)
		con.rejectCall()
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 2, response: "Exit")
	}
	
	func test_9532_NoGreetingPSMGNoPlayVideo() {
		// Test Case: 9532 - PSMG: Play button does not initiate video when PSMG is set to 'No greeting'

		// Setup
		ui.gotoPSMG()
		ui.selectGreeting(type: "No Greeting")
		
		// Icon for play video is disabled
		ui.verifyButtonEnabled(name: "icon play", expected: false)
	}
}
