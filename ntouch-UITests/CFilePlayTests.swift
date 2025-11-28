//
//  CFilePlayTests.swift
//  ntouch
//
//  Created by Mikael Leppanen on 7/15/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import XCTest

class CFilePlayTests: XCTestCase {
	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["cfilePlayAltNum"]!
	let offlineNum = "15556335463"
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
	
	func test_2238_RecordingSignMailReceiveCall() {
		// Test Case: 2238 - SignMail: While recording a SignMail the DUT will not receive incoming calls
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call and reject
		ui.call(phnum: oepNum)
		con.rejectCall()
		
		// Skip greeting and record
		ui.waitForGreeting()
		ui.skipGreeting()
		sleep(10)
		ui.verifySignMailRecording()
		
		// Receive call while recording
		con.dial(number: dutNum)
		sleep(30)
	
		// Hangup and exit
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")
	}
	
	func test_2251_RecordSignMailPlaceCall() {
		// Test Case: 2251 - SignMail: Placing a call after uploading a SignMail
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Place call to DUT num
		ui.call(phnum: dutNum)
		
		// Skip greeting and send signmail
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		ui.hangup()
		
		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_2253_RejectEndCall() {
		// Test Case: 2253 - SignMail: Choosing "End Call" during SignMail Greeting will cancel out of leaving the SignMail Greeting
		// Verify greeting and End Call
		ui.call(phnum: offlineNum)
		ui.verifyGreeting(text: "remote test greeting")
		ui.hangup()
	}
	
	func test_2461_2756_SkipGreeting() {
		// Test Case: 2461 - SignMail: Hitting skip greeting quickly will still skip the SignMail greeting
		// Test Case: 2756 - SignMail: SignMail when endpoint is unavailable
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Test 2756
		// Call and reject
		ui.call(phnum: oepNum)
		con.rejectCall()
		
		// Test 2461
		// Skip greeting and record, then exit without sending
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Exit", doSkip: true)
	}
	
	func test_2491_2865_5256_SendOptions() {
		// Test Case: 2491 - SignMail: Tapping outside of the SignMail send options will not cause the call to end
		// Test Case: 2865 - SignMail: Tapping send more than once
		// Test Case: 5256 - SignMail: After recording a SignMail user is able to send, record again or exit
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Place call and record
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.skipGreeting()
		sleep(10)
		
		// Hangup
		ui.hangup()
		
		// Test 2491
		// Tap outside of send options
		ui.selectTop()
		
		// Test 5256
		ui.verifySignMailSendOptions()
		
		// Test 2865
		// Double Tap Send
		ui.confirmSignMailSendDoubleTap(response: "Send")
		
		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_2867_RecordSignMailRotate() {
		// Test Case: 2867 - SignMail: Orientation changes during SignMail recording
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call and reject
		ui.call(phnum: oepNum)
		con.rejectCall()
		
		// Skip greeting and record
		ui.waitForGreeting()
		ui.skipGreeting()
		ui.waitForSignMailRecording()
		
		// Rotate
		ui.rotate(dir: "Left")
		sleep(3)
		ui.rotate(dir: "Right")
		sleep(3)
		ui.rotate(dir: "Portrait")
		sleep(3)
		
		// Hangup and exit
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")
	}
	
	func test_2959_OfflineSendSignMail() {
		// Test Case: 2959 - SignMail: able to leave SignMail when endpoint is offline
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Place call and send signmail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")
		
		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_3184_3232_OfflineViewPSMG() {
		// Test Case: 3184 - PSMG will be viewable if the remote endpoint is offline
		// Test Case: 3232 - SignMail: View Personal SignMail greeting of device that's logged out
		// Place call
		ui.call(phnum: offlineNum)
		
		// Test 3184
		// Verify greeting
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		
		// Test 3232 - Wait for Greeting
		// Then clean up and exit signmail
		ui.leaveSignMailRecording(time: 2, response: "Exit", doSkip: false)
	}
	
	func test_3231_RejectViewPSMG() {
		// Test Case: 3231 - SignMail: View Personal SignMail Greeting
		
		// Call and Verify greeting
		ui.call(phnum: offlineNum)
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		
		// Hangup and Exit
		ui.leaveSignMailRecording(time: 2, response: "Exit", doSkip: false)
	}
	
	func test_3233_3234_3357_RejectSkipPSMG() {
		// Test Case: 3233 - SignMail: Skip personal greeting
		// Test Case: 3234 - SignMail: Skip personal greeting of logged out device
		// Test Case: 3357 - PSMG: Skip Personal greeting with text SignMail greeting
		
		// Call and Skip Greeting, then
		// Hangup and Exit
		ui.call(phnum: offlineNum)
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		ui.leaveSignMailRecording(time: 10, response: "Exit", doSkip: true)
	}
	
	func test_3238_RotatePSMG() {
		// Test Case: 3238 - SignMail: Rotate while playing a personal SignMail greeting
		// Place call
		ui.call(phnum: offlineNum)
		
		// Verify greeting
		ui.verifyGreeting(text: "remote test greeting")
		
		// Set Landscape Left
		ui.rotate(dir: "Left")
		ui.verifyGreeting(text: "remote test greeting")
		
		// Set Landscape Right
		ui.rotate(dir: "Right")
		ui.verifyGreeting(text: "remote test greeting")
		
		// Set Portrait
		ui.rotate(dir: "Portrait")
		ui.verifyGreeting(text: "remote test greeting")
		sleep(20)
		
		// Hangup and Exit
		ui.rotate(dir: "Portrait")
		ui.hangup()
		ui.confirmSignMailSend(response: "Exit")
	}
	
	func test_3390_RecordCancelReviewPSMG() {
		// Test Case: 3390 - Review a PSMG, after backing out of the record window
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting
		ui.createGreeting(time: 10)
		
		// Review Greeting
		ui.playPSMG()
		sleep(10)
		
		// Record Greeting and Cancel
		ui.recordGreeting(time: 10)
		ui.greetingRecorded(response: "Cancel")
		
		// Review Greeting
		ui.playPSMG()
		sleep(10)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_3392_RecordPreviewRecordPSMG() {
		// Test Case: 3392 - Re-record uploaded message after reviewing and discarding a PSMG
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Record Greeting and Preview
		ui.recordGreeting(time: 10)
		ui.greetingRecorded(response: "Preview")
		sleep(15)
		
		// Record Again
		ui.greetingRecorded(response: "Record Again")
		ui.record(time: 10)
		ui.greetingRecorded(response: "Save")
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_3393_4120_PlayDefaultSelectPSMG() {
		// Test Case: 3393 - Being able to switch between custom and default recording while recording is being reviewed
		// Test Case: 4120 - Verify user can view selected PSMG without watching currently running greeting in its entirety
		// Go to PSMG and select Sorenson Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		
		// Play Greeting
		ui.playPSMG()
		sleep(5)
		
		// Select Personal Greeting
		ui.selectGreeting(type:"Personal")
		
		// Test 4120
		ui.playPSMG()
		sleep(5)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_3555_RecordReviewPSMGText() {
		// Test Case: 3555 - Text appears over video in a Personal SignMail with text
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting with text
		ui.createGreeting(time: 10, text: "3555")
		
		// Play Greeting and Verify Text
		ui.playPSMG()
		sleep(5)
		ui.verifyGreeting(text: "3555")
		sleep(5)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_3662_RecordingPSMGReceiveCall() {
		// Test Case: 3662 - PSMG: Receiving an incoming call interrupts a PSMG recording
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Start Recording
		ui.recordGreeting()
		sleep(3)
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Verify PSMG Screen and select Personal
		ui.selectGreeting(type:"Personal")
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_3880_3919_PlayPauseDefaultPSMG() {
		// Test Case: 3380 - PSMG: The Sorenson option shows the Default greeting when previewed
		// Test Case: 3919 - PSMG: Pause/Play button title changes when pressed
		
		// Go to PSMG and select Sorenson Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		
		// Play
		ui.playPSMG()
		sleep(5)
		
		// Pause
		ui.pausePSMG()
		sleep(5)
		
		// Play
		ui.playPSMG()
		sleep(5)
		ui.gotoHome()
	}
	
	func test_4005_RecordPSMGRotate() {
		// Test Case: 4005 - PSMG: Verify PSMG is able to record in any orientation
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Set Landscape Left
		ui.rotate(dir: "Left")
		ui.createGreeting(time: 10)
		
		// Set Landscape Right
		ui.rotate(dir: "Right")
		ui.createGreeting(time: 10)
		
		// Set Portrait
		ui.rotate(dir: "Portrait")
		ui.createGreeting(time: 10)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_4095_RecordPSMGTextRemoved() {
		// Test Case: 4095 - PSMG: Verify text is displayed on custom SignMail greeting when played back on separate device
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting with text
		ui.createGreeting(time: 10, text: "4095")
		
		// Login to alt account and call DUT number
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.call(phnum: dutNum)
		
		// Verify Greeting and End call
		ui.waitForGreeting()
		ui.verifyGreeting(text: "4095")
		ui.hangup()
		
		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_4115_RecordPSMGTextRemoved() {
		// Test Case: 4115 - PSMG: Custom text is no longer displayed from previous message when new recording is uploaded
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting with text
		ui.createGreeting(time: 10, text: "4115")
		
		// Create Greeting with text
		ui.createGreeting(time: 10)
		
		// Play Greeting and Verify
		ui.playPSMG()
		sleep(5)
		ui.verifyNoGreeting(text: "4115")
		sleep(5)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_4119_RecordAgainPSMG() {
		// Test Case: 4119 - PSMG: Verify selecting again will start the recording from the beginning
		
		// Go to PSMG and Record Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		ui.recordGreeting(time: 10)
		
		// Record Again
		ui.greetingRecorded(response: "Record Again")
		ui.record(time: 10)
		ui.greetingRecorded(response: "Cancel")
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_4133_RecordPreviewRotate() {
		// Test Case: 4133 - PSMG: Verify PSMG plays seamlessly during orientation change
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Record Greeting
		ui.recordGreeting(time: 10)
		
		// Preview
		ui.rotate(dir: "Portrait")
		ui.greetingRecorded(response: "Preview")
		sleep(15)
		
		// Rotate left
		ui.rotate(dir: "Left")
		ui.greetingRecorded(response: "Preview")
		sleep(15)
		ui.greetingRecorded(response: "Cancel")
		ui.rotate(dir: "Portrait")
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_4134_PlayPauseRecPlayPSMG() {
		// Test Case: 4134 - PSMG: viewing of PSMG is always available in the Greeting Type menu
		// Go to PSMG and Create Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		ui.createGreeting(time: 10)
		
		// Play
		ui.playPSMG()
		sleep(5)
		
		// Pause
		ui.pausePSMG()
		sleep(5)
		
		// Record
		ui.recordGreeting(time: 5)
		ui.greetingRecorded(response: "Cancel")
		
		// Play
		ui.playPSMG()
		sleep(15)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_4309_RecordPlayPSMG() {
		// Test Case: 4309 - PSMG video can be previewed after uploading it before device leaves PSMG settings view
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting
		ui.createGreeting(time: 10)
		
		// Play Greeting
		ui.playPSMG()
		sleep(15)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_4348_RecordPlayRecordPSMG() {
		// Test Case: 4348 - PSMG: When you finish uploading a SignMail greeting the App will not crash when you try to re-record a new message
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Create Greeting
		ui.createGreeting(time: 5)
		
		// Play Greeting
		ui.playPSMG()
		sleep(10)
		
		// Record Again
		ui.createGreeting(time: 10)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_4410_RecordBackgroundSendSignMail() {
		// Test Case: 4410 - SignMail: clicking the home button before sending
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Call remote and record
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.skipGreeting()
		sleep(10)

		// Hangup and Background
		ui.hangup()
		ui.pressHomeBtn()

		// Select Notification and Send
		ui.ntouchActivate()
		ui.confirmSignMailSend(response: "Send")
		
		// Verify SignMail
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		
		// Cleanup
		ui.deleteAllSignMail()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_4711_SignMailRecordMax() {
		// Test Case: 4711 - SignMail: device can record SignMail up to the limit
		// Place call
		ui.call(phnum: offlineNum)
		
		// Skip greeting, record for 120 seconds then exit
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		ui.leaveSignMailRecording(time: 120, response: "Exit")
	}
	
	func test_4712_5181_SignMailRecordAgain() {
		// Test Case: 4712 - SignMail: Recording again loads the UI properly after pressing end call
		// Test Case: 5181 - SignMail: Verify user is able to send a rerecorded SignMail
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Place call
		ui.call(phnum: dutNum)
		
		// Test 4712
		// Begin recording and then Record Again
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Rerecord")
		
		// Test 5181
		// Send rerecorded signmail
		ui.leaveSignMailRecording(time: 10, response: "Send", doSkip: false)
		
		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_5221_RecordPreviewPSMG() {
		// Test Case: 5221 - PSMG: Record button disabled while previewing PSMG
		
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		
		// Record Greeting
		ui.recordGreeting(time: 10)
		
		// Preview
		ui.greetingRecorded(response: "Preview")
		sleep(5)
		ui.verifyNoRecordButton()
		sleep(10)
		
		// Preview again
		ui.greetingRecorded(response: "Preview")
		sleep(5)
		ui.verifyNoRecordButton()
		sleep(10)
		ui.greetingRecorded(response: "Cancel")
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}

	func test_5244_PlayPSMGSelectDefault() {
		// Test Case: 5244 - PSMG: Playing personal greeting then selecting Sorenson the last personal image captured is not displayed on Sorenson tab
		// Create Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		ui.createGreeting(time: 10)
		
		// Play Greeting
		ui.playPSMG()
		sleep(5)
		
		// Select Sorenson Greeting
		ui.selectGreeting(type:"Sorenson")
		ui.playPSMG()
		sleep(15)
		ui.gotoHome()
	}
	
	func test_5245_PlayPSMGReceiveCall() {
		// Test Case: 5245 - PSMG: Receiving call while Playing PSMG, User is able to play PSMG after call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Create Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		ui.createGreeting(time: 10)
	
		// Play Greeting
		ui.playPSMG()
		sleep(5)
		
		// Receive call while recording
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(5)
		ui.hangup()
	
		// Play Greeting
		ui.playPSMG()
		sleep(15)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}

	func test_5284_PlayPSMGRotate() {
		// Test Case: 5284 - PSMG: Selecting play on Sorenson greeting then rotating device
		// Create Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		ui.createGreeting(time: 30)
		
		// Play Greeting
		ui.playPSMG()
		sleep(5)
		
		// Rotate left
		ui.rotate(dir: "Left")
		sleep(10)
		
		// Rotate right
		ui.rotate(dir: "Right")
		sleep(10)
		
		// Set Portait
		ui.rotate(dir: "Portrait")
		sleep(5)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_5335_RecordRecordPSMG() {
		// Test Case: 5335 - PSMG: Verify user is able to save a re-recorded PSMG
		// Create Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type:"Personal")
		ui.createGreeting(time: 5)
		ui.createGreeting(time: 10)
		
		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
	
	func test_7061_RecordPreviewPSMG() {
		// Test Case: 7061 - Record and preview the greeting
		// Go to PSMG and select Personal Greeting
		ui.gotoPSMG()
		ui.selectGreeting(type: "Personal")

		// Record Greeting and Preview
		ui.recordGreeting(time: 10)
		ui.greetingRecorded(response: "Preview")
		sleep(11)
		ui.greetingRecorded(response: "Cancel")

		// Cleanup
		ui.gotoPSMG()
		ui.selectGreeting(type:"Sorenson")
		ui.gotoHome()
	}
}
