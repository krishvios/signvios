//
//  SingleDeviceCallingTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 3/30/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class SingleDeviceCallingTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let dutPassword = UserConfig()["Password"]!
	let redirectedNum = "19638010007"
	let vrsNum1 = "18015757669"
	let offlineNum = "15556335463"
	let dndNum = "17326875000"
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
	
	func test_0593_3711_CallUnavailablePhones() {
		// Test Case: 0593 - Calling: Verify the app is able to handle calling a phone that is unavailable
		// Test Case: 3711 - Calling: Verify that placing a call to a videophone that is not online, DUT logs that call.
		let endpoint = endpointsClient.checkOut()
		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Delete Contacts and Call History
		ui.deleteAllContacts()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Test 593
		// Call OEP Unassociated Number
		ui.call(phnum: vrsNum1)
		ui.hangup()
		
		// Call OEP that is turned off
		ui.call(phnum: offlineNum)
		ui.waitForGreeting(textOnly: true, text: "remote test greeting")
		ui.leaveSignMailRecording(time: 2, response: "Exit")
		
		// Call OEP and never answer
		ui.call(phnum: oepNum)
		sleep(10)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 2, response: "Exit")
		
		// Call OEP that is set to reject calls (VP1 Lab: "17326875000" set to DND)
		ui.call(phnum: dndNum)
		
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 2, response: "Exit")
		
		// Test 3711
		// Verify Call History
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(num: dndNum)
		ui.verifyCallHistory(num: offlineNum)
		ui.verifyCallHistory(num: oepNum)
		ui.verifyCallHistory(num: vrsNum1)
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}

	func test_0596_OutBoundFromVariousUIPoints() {
		// Test Case: 596 - Calling: Verify the app is handling all outbound calls
		let endpoint = endpointsClient.checkOut()
		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Use Dialer to call
		ui.gotoHome()
		ui.call(phnum: oepNum)
		ui.hangup()
		sleep(1)
		
		// Use Call Histroy to Call
		ui.gotoCallHistory(list: "All")
		ui.callFromHistory(num: oepNum)
		ui.hangup()
		
		// Add Contact and Call From Contact List
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 0596", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		testUtils.calleeNum = oepNum
		ui.callContact(name: "Automate 0596", type: "home")
		ui.hangup()
		
		// Clean up
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 0596")
		
		// Return to home screen
		ui.gotoHome()
	}

	func test_0787_CallVRS() {
		// Test Case: 0787 - Calling: VRS Call
		// Place VRS Call
		ui.call(phnum: "411")
		ui.waitForCallOptions()
		
		// Verify CallerID
		ui.verifyCallerID(text: "Information")
		
		// Hangup
		ui.hangup()
	}
	
	func test_0869_LoggedOutReceiveCall() {
		// Test Case 869 - Calling: Verify the app will not accept calls when the user is logged out
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Setup for OEP
		ui.logout()
		
		// OEP Calls Logged out DUT
		con.dial(number: dutNum, assert: false)
		sleep(5)
		ui.verifyForgotPassword()
		
		// Return To home
		ui.login()
		ui.gotoHome()
	}
		
	func test_0872_RotateIncomingCall() {
		// Test Case 872 - Calling: Verify that when in the inbox and you get a call, the call does not become disconnected by rotating the phone
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Call from OEP
		con.dial(number: dutNum)
		sleep(1)
		
		// Rotate device
		ui.rotate(dir: "Left")
		sleep(1)
		ui.rotate(dir: "Right")
		sleep(1)
		ui.rotate(dir: "Portrait")
		sleep(1)
		
		// Verify call is connected
		ui.verifyIncomingCall()
		con.hangup()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_0921_MissedCallNotification() {
		// Test Case: 921 - Calling: Declinging a call will cause missed call notification to appear
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Setup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.verifyNoMissedCallBadge()
		ui.gotoHome()
		
		// OEP Calls DUT for DUT to decline
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")


		// Verify missed call notification
		ui.waitForMissedCallBadge(count: "1")
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistory(num: oepNum)
		
		// Clean up
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_1543_HangingCallUp() {
		// Test Case: 1543 Calling: Hanging Call Up
		let endpoint = endpointsClient.checkOut()
		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		sleep(1)

		// Hang UP with DUT
		ui.hangup()

		// Place Call to Non-Core Number
		ui.call(phnum: "411")
		sleep(1)

		// Hang UP with DUT
		ui.hangup()
		ui.gotoHome()
	}
	
	func test_1607_OutGoingCalltoRedirectedNumber() {
		// Test Case 1607: Calling: Outgoing call to redirected number
		
		// Place call to Redirected number
		ui.call(phnum: redirectedNum)
		sleep(1)

		// Verify that Redirected Message is displayed
		ui.redirectedMessage(response: "Cancel")
		ui.gotoHome()
	}
	
	func test_2009_CallBackgroundHangupVerifyUI() {
		// Test Case: 2009 - Calling: The dialer and UI should be complete when returning to a call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Background the app and hangup OEP
		ui.pressHomeBtn()
		con.hangup()

		// Open ntouch and Dial tab selected
		ui.ntouchActivate()
		ui.verifyTabSelected(tab: "Dial")
	}
	
	func test_2136_2138_2926_RedialContact() {
		// Test Case: 2136 - Calling: hitting the call button when the field is empty after dialing a contact brings up that contacts phone number
		// Test Case: 2138 - Calling:Sorenson contact displays correct "contact name" under the dialer
		// Test Case: 2926 - Calling: Redialing a Sorenson Contact
		
		// Setup
		ui.deleteAllContacts()
		
		// Add and call Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2136", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		testUtils.calleeNum = vrsNum1
		ui.callContact(name: "Automate 2136", type: "home")
		ui.hangup()
		
		// Verify dialer button bring up last dialed contact
		ui.gotoHome()
		ui.selectCallButton()
		ui.verifyDialerContact(name: "Automate 2136")
		
		// Clean up
		ui.deleteAllContacts()
		ui.gotoHome()
	}
	
	func test_2180_ReceiveCallOnSettingsScreen() {
		// Test Case: 2180 - Calling: Receive a call on settings screen
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Go to settings and answer incoming call that lasts for 30 seconds
		ui.gotoSettings()
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(30)
		ui.hangup()
		
		// Return Home
		ui.gotoHome()
	}
	
	func test_2186_CallBackgroundDecline() {
		// Test Case: 2186 - Calling: Backgrounding the app and declining the call will end the call on the DUT
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP and press home screen
		ui.call(phnum: oepNum)
		ui.pressHomeBtn()
		con.hangup()
		
		// Open Ntouch and verify SignMail
		ui.ntouchActivate()
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 2, response: "Exit")
		
		// Return Home
		ui.gotoHome()
	}
	
	func test_2204_RedialInternational() {
		// Test Case: 2204 - Calling: International dialing valid on redial
		
		// Call phone number with country exit code - Mexico (country code 52, and city code 55), you would dial 011 - 52 - 1 - 55 - XXXX-XXXX.
		ui.call(phnum: "01118013578460")
		ui.verifyVRSCall()
		ui.waitForCallOptions()
		ui.hangup()
		ui.redial()
		ui.waitForCallOptions()
		ui.verifyVRSCall()
		ui.hangup()
	}
	
	func test_2206_CallsLasting() {
		// Test Case: 2206 - Calling: Calls lasting
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		for _ in 1...3 {
			// Call 3 times for 3 minutes
			ui.call(phnum: oepNum)
			con.answer()
			ui.waitForCallOptions()
			sleep(180)
			ui.hangup()
		}
		
		// Return Home
		ui.gotoHome()
	}
	
	func test_2462_EndCallBackground() {
		// Test Case: 2462 - Calling: Backgrounding the app after a call will not cause the app to crash
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
		
		// Background and open ntouch
		ui.pressHomeBtn()
		ui.ntouchActivate()
		ui.gotoHome()
	}
	
	func test_2943_SendSignMailPlaceCall() {
		// Test Case: 2943 - Calling: Sending a signmail does not prevent the device from placing further outgoing calls
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP and OEP Declines
		ui.call(phnum: oepNum)
		con.rejectCall()
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")
		
		// Able to place further outgoing calls
		ui.call(phnum: oepNum)
		ui.hangup()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_2946_CallRinging() {
		// Test Case: 2946 - Calling: Calling does not take an unreasonably long time to display the dialer
		let endpoint = endpointsClient.checkOut()
		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP and receive in call progress quickly
		ui.call(phnum: oepNum)
		ui.verifyPhoneNumber(num: oepNum)
		ui.hangup()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_2967_OnHoldHangup() {
		// Test Case: 2967 - Calling: Hanging up on DUT while on hold
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// OEP Calls DUT and answers
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		
		// DUT backgrounds app
		ui.pressHomeBtn()
			
		// OEP Ends Call on DUT without DUT crashing
		con.hangup()
		ui.ntouchActivate()
		ui.hangup()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_2973_CoreCallerID() {
		// Test Case: 2973 - Calling: Call shows core caller ID when the call isn't a contact
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName  else { return }
		
		// Setup
		ui.deleteAllContacts()
		ui.gotoHome()
		
		// Call OEP that isn't contact
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		ui.verifyPhoneNumber(num: oepNum)
		ui.verifyCallerID(text: oepName)
		ui.hangup()
		
		// OEP Calls DUT thats not a contact
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		ui.waitForCallOptions()
		ui.verifyPhoneNumber(num: oepNum)
		ui.verifyCallerID(text: oepName)
		ui.hangup()
		
		// Clean up
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_3239_ProperUIElementsDisplayedOnCall() {
		// Test Case 3239 Calling: Ensure proper UI elements are displayed when making an outgoing call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call and connect to OEP
		ui.call(phnum: oepNum)
		con.answer()
		
		// Verify Dialer UI is not present in call
		ui.waitForCallOptions()
		ui.verifyNoDialerUI()
		ui.hangup()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_3572_CallCIRfromContactSupportPage() {
		// Test Case: 3572 - Calling - SIP: Calls to CIR from the Help tab in settings will use SIP

		// Navigate to the Help page
		ui.gotoCustomerCare()

		// Call Customer Care
		ui.selectCallCustomerService()
		ui.waitForCallOptions()
		ui.verifyCallerID(text: "Customer Care")
		ui.hangup()

		// Return to Home Screen
		ui.gotoHome()
	}
	
	func test_3739_ReceiveCallAndCallBack() {
		// Test Case: 3739 - Calling: User is able to call a remote endpoint back after the remote end hangs up
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		
		// OEP Hang up
		con.hangup()
		
		// Call back to OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		ui.hangup()
	}
	
	func test_3853_BackgroundReceiveCall() {
		// Test Case: 3853 - Calling: Receiving a SIP call from the background doesn't disconnect the call upon answering it
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Background app
		ui.pressHomeBtn()
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		ui.gotoHome()
	}
	
	func test_4488_CallContactWithLongName() {
		// Test Case: 4488 - Calling: Calling a contact with a very long name does not cause the hang up button to be inaccessible
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate4488ContactWithAReallyLongName12345678902234567890323456", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Call Contact and verify able to hang up
		testUtils.calleeNum = vrsNum1
		ui.callContact(name: "Automate4488ContactWithAReallyLongName12345678902234567890323456", type: "home")
		ui.hangup()
		
		// Cleanup
		ui.gotoHome()
		ui.deleteAllContacts()
	}
	
	func test_4492_CallDeclineBackground() {
		// Test Case: 4492 - Calling: Backgrounding the app after hanging up during the SignMail greeting allows app to sleep
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP and Decline
		ui.call(phnum: oepNum)
		con.rejectCall()
		
		// Background During Greeting
		ui.waitForGreeting()
		ui.pressHomeBtn()
		
		// Open ntouch and go to home
		ui.ntouchActivate()
		ui.gotoHome()
	}
	
	func test_4510_Redial411() {
		// Test Case: 4510 - Calling: hitting the call button when the field is empty after dialing a 3 digit number brings up that number in the dial field
		
		// Call 411 and Redial
		ui.call(phnum: "411")
		ui.waitForCallOptions()
		ui.verifyVRSCall()
		ui.hangup()
		ui.redial()
		ui.waitForCallOptions()
		ui.verifyVRSCall()
		ui.hangup()
	}
	
	func test_4532_CallBackgroundHangupCall() {
		// Test Case: 4532 - Calling: DUT receives calls in background if call is disconnected while in background
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Background and hangup OEP
		ui.pressHomeBtn()
		con.hangup()
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Cleanup
		ui.hangup()
		ui.gotoHome()
	}
	
	func test_5310_NoCallOptionsWhileCalling() {
		// Test Case: 5310 - Calling: Call Options are not available before call is answered
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP and verify no call options
		ui.call(phnum: oepNum)
		sleep(3)
		ui.verifyNoCallOptions()
		
		// Asnwer and verify call options
		con.answer()
		ui.waitForCallOptions()
		ui.hangup()
	}
	
	func test_5999_6000_6002_SelfView() {
		// Test Case: 5999 - Calling: Verify the user is able to hide their self view while in a call
		// Test Case: 6000 - Calling: Verify the user is able to bring their self view back while in a call
		// Test Case: 6002 - Calling: Verify that hide self view is available in portrait
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Test 6002
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		ui.verifySelfView(expected: true)
		ui.rotate(dir: "Left")
		sleep(3)
		ui.rotate(dir: "Portrait")
		
		// Test 5999
		ui.hideSelfView()
		ui.verifySelfView(expected: false)
		
		// Test 6000
		ui.hideSelfView(setting: false)
		ui.verifySelfView(expected: true)
		ui.hangup()
	}
	
	func test_7198_CallOptionsDismissedAfterCall() {
		// Test Case: 7198 - Calling: Call options pop up is dismissed after the call ends
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }

		// Verify call options are present and not after call ends.
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		ui.selectCallOptions()
		ui.verifyCallOptions()
		con.hangup()
		ui.verifyNoCallOptionsUI()
	
		// Return home
		ui.gotoHome()
	}
	
	func test_7221_7498_BackgroundForegroundActiveCall() {
		// Test Case: 7221 - Calling: User can background and foreground call and end up on ringing screen.
		// Test Case: 7498 - Calling: Call ends on DUT after backgrounding, foregrounding, and OEP ending the call.
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call and background
		ui.call(phnum: oepNum)
		ui.pressHomeBtn()

		// Test 7221
		// Resume and verify DUT has in call screen visible
		ui.ntouchActivate()
		ui.verifyCalling(num: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Test 7498
		// OEP Hangs up call and return home
		con.hangup()
		ui.gotoHome()
	}
	
	func test_7235_DialAfterEndingCall() {
		// Test Case: 7235 - Calling: UI updates correctly when entering a phone number after a call

		// Place Call
		ui.call(phnum: vrsNum1)
		ui.waitForCallOptions()
		ui.hangup()

		// Enter 411 in dialer and verify contact label
		ui.gotoHome()
		ui.dial(phnum: "411")
		ui.verifyDialerContact(name: "Information")
		ui.clearDialer()
		ui.gotoHome()
	}
	
	func test_7746_Background5MinIncomingCall() {
		// Test Case: 7746 - Calling: Incoming calls can still be received after application is backgrounded for 5+ minutes
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Background and sleep for 5+ minutes
		ui.gotoHome()
		ui.pressHomeBtn()
		sleep(301)
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_7795_DialerDUTNumberRotate() {
		// Test Case: 7795 - Calling - UI: The phone number of the logged in account of the DUT will be present on different screen orientations
		
		// Rotate and verify number
		ui.rotate(dir: "Left")
		ui.verifyDialerNumber(num: dutNum)
		sleep(2)
		ui.rotate(dir: "Right")
		ui.verifyDialerNumber(num: dutNum)
		sleep(2)
		ui.rotate(dir: "Portrait")
		ui.verifyDialerNumber(num: dutNum)
	}
	
	func test_8421_SwitchCameraVisibleDuringCall() {
		// Test Case: 8421 - Calling: Switch Camera UI button is located in the Self View
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call endpoint and answer
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Verify the switch camera button is visible
		ui.verifySwitchCameraButton()

		// Hang up
		ui.hangup()
	}
	
	func test_12171_ShareLocationNoBlackVideo() {
		// Test Case: 12171 - Calling: No black video with Share Location enabled in all orientations
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
			
		// Call OEP
		ui.call(phnum: oepNum)
		
		// Answer
		con.answer()
		ui.waitForCallOptions()
		sleep(10)
		
		// Share Location
		ui.shareLocation()
		ui.selectCall(option: "Show Call Stats")
		ui.verifyFpsSentGreaterThan(value: 0)
		
		// Set Landscape Left
		ui.rotate(dir: "Left")
		ui.verifyFpsSentGreaterThan(value: 0)
		
		// Set Landscape Right
		ui.rotate(dir: "Right")
		ui.verifyFpsSentGreaterThan(value: 0)
		
		// Set Portrait
		ui.rotate(dir: "Portrait")
		ui.verifyFpsSentGreaterThan(value: 0)
		
		// Hangup
		ui.hangup()
	}
	
	func test_12172_RotateNoBlackVideo() {
		// Test Case: 12172 - Calling: No black video in all orientations
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
			
		// Call OEP
		ui.call(phnum: oepNum)
		
		// Answer
		con.answer()
		ui.waitForCallOptions()
		sleep(10)
		ui.selectCall(option: "Show Call Stats")
		ui.verifyFpsSentGreaterThan(value: 0)
		
		// Set Landscape Left
		ui.rotate(dir: "Left")
		ui.verifyFpsSentGreaterThan(value: 0)
		
		// Set Landscape Right
		ui.rotate(dir: "Right")
		ui.verifyFpsSentGreaterThan(value: 0)
		
		// Set Portrait
		ui.rotate(dir: "Portrait")
		ui.verifyFpsSentGreaterThan(value: 0)
		
		// Hangup
		ui.hangup()
	}
}
