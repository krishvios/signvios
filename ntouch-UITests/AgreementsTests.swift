//
//  AgreementsTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 8/3/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import XCTest

class AgreementsTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let endpointsClient = TestEndpointsClient.shared
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: cfg["Orientation"]!)
		continueAfterFailure = false
		ui.ntouchActivate()
		ui.clearAlert()
		ui.cleanupDismissibleCIR()
		ui.cleanupAgreements()
		ui.cleanupFccRegistration()
		ui.waitForMyNumber()
	}
	
	override func tearDown() {
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_2896_5207_LogoutSendEULA() {
//		// Test Case: 2869 - Agreements: not displayed while on login screen
//		// Test Case: 5207 - Agreements: Logout, send agreements, login
//		
//		// Logout
//		ui.logout()
//		
//		// Send Agreements
//		core.sendAgreements()
//		
//		// Login
//		ui.selectLogin()
//		
//		// Accept Agreements
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_3598_AgreementsIncomingCall() {
//		// Test Case: 3598 - Agreements: Call will connect when the EULA page is showing
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//		
//		// Wait for Agreement
//		ui.waitForAgreement()
//		
//		// Call DUT
//		con.dial(number: cfg["Phone"]!)
//		ui.incomingCall(response: "Answer")
//		sleep(3)
//		ui.hangup()
//		
//		// Accept Agreements
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_3604_3605_EULA() {
//		// Test Case: 3604 - EULA: Send FCC Registration page to user
//		// Test Case: 3605 - EULA: Submit FCC Registration Page
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//		
//		// Accept Agreements
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_3606_3607_3608_3609_RegistrationRequirements() {
//		// Test Case: 3606 - Agreements: FCC registration does not allow submission if there is no DOB
//		// Test Case: 3607 - Agreements: FCC registration does not allow submission if there is no SSN
//		// Test Case: 3608 - Agreements: submit FCC registration with No SSN box checked
//		// Test Case: 3609 - Agreements: FCC registration does not allow submission with no DOB and with No SSN box checked
//
//		// Send FCC Registration
//		core.sendTrsRegistration()
//		ui.heartbeat()
//		ui.waitForUserRegistration()
//
//		// Test 3606
//		// Enter Last 4 SSN
//		ui.enterLast4ssn(num: "1234")
//		ui.selectSubmitRegistration()
//		ui.verifyInvalidBirthday()
//
//		// Test 3609
//		// Remove SSN and select No SSN
//		ui.deleteLast4ssn()
//		ui.selectNoSSN(setting: 1)
//		ui.selectSubmitRegistration()
//		ui.verifyInvalidBirthday()
//
//		// Test 3607
//		// Remove No SSN and enter birth date
//		ui.selectNoSSN(setting: 0)
//		ui.selectBirthYear()
//		ui.selectSubmitRegistration()
//		ui.verifyInvalidSSN()
//
//		// Test 3608
//		// Select No SSN and Submit
//		ui.selectNoSSN(setting: 1)
//		ui.selectSubmitRegistration()
//		ui.gotoHome()
//	}

	// TODO: Server for CoreAccess currently not available
//	func test_3611_3612_3613_3617_CancelFCCRegistration() {
//		// Test Case: 3611 - Agreements: Press Cancel on FCC registration page
//		// Test Case: 3612 - Agreements: FCC registration cancel warning allows user to go back to the FCC registration
//		// Test Case: 3613 - Agreements: After canceling the FCC user registration, upon log in the user is presented with registration again
//		// Test Case: 3617 - Agreements: FCC registration is displayed after the app has been Backgrounded
//
//		// Test 3611
//		core.sendTrsRegistration()
//		ui.heartbeat()
//		ui.waitForUserRegistration()
//		ui.selectCancelRegistration(response: "Cancel")
//
//		// Test 3612
//		ui.selectCancelRegistration(response: "Logout")
//
//		// Test 3613
//		ui.selectLogin()
//		ui.waitForUserRegistration()
//
//		// Test 3617
//		// User is presented with registration after backgrounding application
//		ui.selectCancelRegistration(response: "Logout")
//		ui.selectLogin()
//		ui.pressHomeBtn()
//		ui.ntouchActivate()
//		ui.waitForUserRegistration()
//		ui.enterLast4ssn(num: "1234")
//		ui.selectBirthYear()
//		ui.selectSubmitRegistration()
//		ui.gotoHome()
//	}

	// TODO: Server for CoreAccess currently not available
//	func test_3614_4315_SubmitRegistration() {
//		// Test Case: 3614 - Agreements: SSN is obscured on FCC registration page
//		// Test Case: 4315 - Agreements: FCC User Registration Page does dismisses itself after submitting automatically
//
//		// Send FCC Registration
//		core.sendTrsRegistration()
//		ui.heartbeat()
//		ui.waitForUserRegistration()
//
//		// Test 3614
//		// Enter Last 4 SSN Verify Obscured
//		ui.enterLast4ssn(num: "1234")
//		ui.verifyObscuredSSN()
//
//		// Test 4315
//		// Select birth and submit
//		ui.selectBirthYear()
//		ui.selectSubmitRegistration()
//		ui.gotoHome()
//	}

	// TODO: Server for CoreAccess currently not available
//	func test_3619_RegistrationIncomingCall() {
//		// Test Case: 3619 - Agreements: Place call to device with the FCC registration page up
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
//
//		// Send FCC Registration
//		core.sendTrsRegistration()
//		ui.heartbeat()
//		ui.waitForUserRegistration()
//
//		// Place Call to DUT
//		con.dial(number: cfg["Phone"]!)
//		ui.incomingCall(response: "Answer")
//		sleep(60)
//		ui.hangup()
//
//		// Verify and Submit FCC Registration
//		ui.waitForUserRegistration()
//		ui.selectNoSSN(setting: 1)
//		ui.selectBirthYear()
//		ui.selectSubmitRegistration()
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_3787_3877_4032_7446_EULA() {
//		// Test Case: 3787 - Agreements: Ensure EULA acceptance agreements when triggered to appear are showing after heartbeat
//		// Test Case: 3877 - Agreements: App does not lock up after accepting two EULAs
//		// Test Case: 4032 - Agreements: All Agreements must be accepted in any order sent
//		// Test Case: 7446 - Agreements: User only has to accept the agreements once
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//		
//		// Accept Agreements
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_3790_RejectAgreementSelectNo() {
//		// Test Case: 3790 - Agreements: Verify when reject button is pressed, the application stays on selected EULA agreement page
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//		
//		// Accept 2 Agreements
//		ui.acceptAgreements(count: 2)
//		
//		// Reject 3rd agreement and select No
//		ui.waitForAgreement()
//		ui.rejectAgreement(response: "No")
//		
//		// Verify Agreement still present and accept
//		ui.waitForAgreement()
//		ui.acceptAgreements(count: 1)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_4038_AgreementAfterLoggingOut() {
//		// Test Case: 4038 - Agreements: receive agreement after log out and in
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.logout()
//		ui.selectLogin()
//		
//		// Accept Agreements
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_4039_AgreementPressHomeBtn() {
//		// Test Case: 4039 - Agreements: Agreement are displayed until user accepts agreement or rejects and is not able to bypass by hitting the home button
//
//		// Send Agreements
//		ui.gotoHome()
//		core.sendAgreements()
//		ui.heartbeat()
//
//		// Accept 1st Agreement
//		ui.waitForAgreement()
//		ui.acceptAgreements(count: 1)
//
//		// Press Home Button on Agreement
//		ui.waitForAgreement()
//		ui.pressHomeBtn()
//
//		// Accept Remaining Agreements
//		ui.ntouchActivate()
//		ui.waitForAgreement()
//		ui.acceptAgreements(count: 2)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_4216_7225_RejectAgreementSelectYes() {
//		// Test Case: 4216 - Agreements: Verify the behavior after declining the provider agreement
//		// Test Case: 7225 - Agreements: After rejecting EULA user can log in and is presented with EULA again
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//		
//		// Accept 1st Agreement
//		ui.acceptAgreements(count: 1)
//		
//		// Reject 2nd agreement and select Yes
//		ui.waitForAgreement()
//		ui.rejectAgreement(response: "Yes")
//		
//		// Login
//		ui.selectLogin()
//		
//		// Accept Agreements
//		ui.waitForAgreement()
//		ui.acceptAgreements(count: 2)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_7006_SwipeKillWithAgreement() {
//		// Test Case: 7006 - Agreements: Force killing app with agreement up
//
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//		ui.waitForAgreement()
//
//		// kill ntouch and relaunch
//		ui.ntouchTerminate()
//		ui.ntouchActivate()
//		ui.waitForAgreement()
//
//		// Accept Agreements
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_7307_ReceiveCallAfterRejectingAgreement() {
//		// Test Case: 7307 - Agreements: receive call after rejecting agreement
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//
//		// Reject Agreements
//		ui.rejectAgreement(response: "Yes")
//
//		// Log back into account and heartbeat
//		ui.enterPhoneNumber(phnum: dutNum)
//		ui.enterPassword(password: defaultPassword)
//		ui.selectLogin()
//
//		// Accept agreements
//		ui.acceptAgreements(count: 3)
//
//		// Place call
//		con.dial(number: dutNum)
//		ui.waitForIncomingCall()
//		ui.incomingCall(response: "Answer")
//
//		// Hangup call and heartbeat
//		ui.hangup()
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_7762_ReceiveAgreementsInCall() {
//		// Test Case: 7762 - Agreements don't display till call ended
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
//		
//		// Call DUT
//		con.dial(number: cfg["Phone"]!)
//		ui.incomingCall(response: "Answer")
//		
//		// Send Agreements
//		core.sendAgreements()
//		
//		// Wait for a bit
//		sleep(20)
//		ui.hangup()
//		ui.pressHomeBtn()
//		ui.ntouchActivate()
//		
//		// Accept Agreements
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_8139_DismissibleCIR() {
//		// Test Case: 8139 - Agreements: Agreement is displayed to the user and a Call CIR dismissible is sent to the device
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//		ui.acceptAgreements(count: 3)
//		
//		// Send Dismissible CIR
//		core.sendDismissibleCIR()
//		ui.heartbeat()
//		
//		// Call CIR Dismissible
//		ui.callCIRDismissible()
//		sleep(3)
//		ui.verifyCallerID(text: "Customer Care")
//		ui.hangup()
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_8434_AgreementDisplayedAfterRejection() {
//		// Test Case: 8434 - Agreements: EULA is not displayed over call window after rejection
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//		ui.waitForAgreement()
//		
//		// Reject Agreements
//		ui.rejectAgreement(response: "Yes")
//		
//		// Login and accept agreements
//		ui.selectLogin()
//		ui.acceptAgreements(count: 3)
//		
//		// Call DUT
//		con.dial(number: cfg["Phone"]!)
//		ui.incomingCall(response: "Answer")
//		sleep(3)
//		
//		// Resend Agreements and hangup
//		core.sendAgreements()
//		ui.hangup()
//		
//		// Accept Agreements
//		ui.heartbeat()
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_9345_swipeAroundOnAgreements() {
//		// Test Case: 9345 - Agreements: swipe around
//		
//		// Send Agreements
//		core.sendAgreements()
//		ui.heartbeat()
//
//		// swipe around
//		app.swipeUp()
//		app.swipeDown()
//		app.swipeLeft()
//		app.swipeRight()
//
//		// hit home button
//		ui.pressHomeBtn()
//		
//		// Accept and go home
//		ui.ntouchActivate()
//		ui.acceptAgreements(count: 3)
//		ui.gotoHome()
//	}
}
