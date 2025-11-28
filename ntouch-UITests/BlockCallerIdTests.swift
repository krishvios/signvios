//
//  BlockingCallerIdTests.swift
//  ntouch-UITests
//
//  Created by Chenfan Li on 10/8/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class BlockCallerIdTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let endpointsClient = TestEndpointsClient.shared
	let dutNum = UserConfig()["Phone"]!
	let altNum = UserConfig()["bciAltNum"]!
	let defaultPassword = UserConfig()["Password"]!
	
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
		// TODO: Server for CoreAccess currently not available
//		core.setHideMyCallerID(value: "0")
		ui.ntouchTerminate()
	}

	func test_3943_BlockedNumberCantReceiveSignMailOrCall() {
		// Test Case 3943 - Block: Verify that a user can no longer receive calls or SignMails from the blocked number
		
		// Remove all existing SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoBlockedList()
		ui.deleteBlockedList()

		// Log in as OEP to leave a SignMail with DUT
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)
		ui.call(phnum: dutNum)
		ui.leaveSignMailRecording(time: 10, response: "Send")

		// Switch back to DUT and block OEP from SignMail
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		ui.blockContactFromSignMail(phnum: altNum)

		// Delete all existing SignMail and Call History
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Switch over to OEP and attempt to call DUT
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.call(phnum: dutNum)
		sleep(3)
		ui.verifyCallFailed()

		// Switch back to DUT and verify no call or SignMail came through from blocked number
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoCallHistory(list: "All")
		ui.verifyNoCallHistory(name: "iOS Automation", num: altNum)
		ui.gotoSignMail()
		ui.verifyNoSignMail()

		// Clean up
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoHome()
	}
	
	func test_6193_BlockAnonymousCallsSettingLabel() {
		// Test Case 6193 - Block Anonymous: Verify UI labeled as "Block Anonymous callers" on block anonymous feature.

		// Go to Advanced Settings and verify Block Anonymous Callers label
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: true)
		ui.verifyText(text: "Block Anonymous Callers", expected: true)

		// Toggle option and verify label does not change
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: false)
		ui.verifyText(text: "Block Anonymous Callers", expected: true)
	}
	
	func test_6194_6216_BlockAnonymousCallsEnabledAndHideCallerID() {
		// Test Case: 6194 - Block Anonymous: Verify that "Block Anonymous Callers" enabled blocks all Videophone calls with block CID enabled.
		// Test Case 6216 - Block Anonymous: Verify that callers blocked by "Don't Allow Anonymous Videophone Calls" cannot leave a signmail.

		// Clear out recent calls
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()

		// Enable "Block Anonymous Callers" option
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: true)
		ui.heartbeat()

		//Switch to alt account to turn on Block CID
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: true)
		ui.heartbeat()

		// Attempt to call DUT from endpoint 3 times
		// 6194/6126: Caller cannot complete call and/or leave SignMail
		for _ in 1...3 {
			ui.call(phnum: dutNum)
			ui.verifyCallFailed(message: "Call Disconnected")
		}

		// Clean up Alt Account
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)
		ui.heartbeat()

		// Switch to DUT account and check call history for calls
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoCallHistory(list: "All")
		ui.verifyNoCallHistory(name: "iOS Automation", num: altNum)

		// Clean up DUT account
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: false)
		ui.heartbeat()
		ui.gotoHome()
	}
	
	func test_6195_6567_6571_BlockAnonymousCallsDisabledAndHideCallerID() {
		// Test Case: 6195 - Block Anonymous: Verify "Don't allow anonymous Videophone calls" disabled allows calls from devices with block CID enabled
		// Test Case: 6567 - BCI: Callers info is replaced with "No Caller ID"
		// Test Case: 6571 - BCI: When leaving a SignMail with BCI enabled the number and name is not displayed

		// Clear out recent calls and sign mail
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()

		// Disable "Block Anonymous Callers" option
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: false)

		//Switch to alt account to turn on Block CID
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: true)
		
		// 6195 - Calls from devices with BCI are allowed when "Block Anonymous Callers" is disabled
		// Call DUT and leave a Sign Mail
		ui.call(phnum: dutNum)
		ui.leaveSignMailRecording(time: 10, response: "Send")

		// Clean up Alt Account
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)

		// Switch to DUT account and check call history for calls
		// 6567 - Verify caller info replaced with "No Caller ID"
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(count: 1)
		ui.verifyText(text: "No Caller ID", expected: true)
		ui.deleteAllCallHistory()

		// 6571 - Verify User Info is not available on SignMail
		ui.gotoSignMail()
		ui.verifySignMail(count: 1)
		ui.verifyText(text: "No Caller ID", expected: true)
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_6554_BlockCallerIDFeatureCanBeDisabled() {
//		// Test Case: 6554 - BCI: VDM has ability to disable Block Caller ID
//
//		// Set Caller ID Block Feature to disabled
//		core.setHideMyCallerIDFeature(value: "0")
//		ui.heartbeat()
//
//		// Verify feature is not visible, scroll by toggling block anonymous callers option
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: true)
//		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: false)
//		ui.verifyText(text: "Hide My Caller ID", expected: false)
//
//		// Clean up
//		core.setHideMyCallerIDFeature(value: "1")
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_6555_BlockCallerIDFeatureCanBeEnabled() {
//		// Test Case: 6555 - BCI: VDM has ability to enable Block Caller ID
//
//		// Set Caller ID Block Feature to enabled
//		core.setHideMyCallerIDFeature(value: "1")
//		ui.heartbeat()
//
//		// Verify option is visible, and toggleable
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: true)
//		ui.verifyText(text: "Hide My Caller ID", expected: true)
//		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)
//		ui.gotoHome()
//	}
	
	func test_6568_7193_BlockCallerIDFeatureCannotBypassBlockedList() {
		// Test Case: 6568 - BCI: User has a caller Blocked, the user cannot Block Caller ID and call the User
		// Test Case: 7129 - BCI: When opening the block list, the application will not crash.

		// 7193 - Access Blocked List without crashing
		// Block Number
		ui.gotoBlockedList()
		ui.blockNumber(name: "iOS Automation", num: altNum)

		// Log in as the blocked account and turn on Hide My Caller ID
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: true)

		// 6568 - Attempt to call DUT number and fail to connect due to being blocked.
		ui.call(phnum: dutNum)
		ui.verifyCallFailed(message: "Call Disconnected")

		// Clean up
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoHome()
	}
	
	func test_6572_BlockCallerIDEnabledCallTransfer() {
		// Test Case: 6572 - BCI: Block Caller ID is enabled when the Call is Transferred
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let ep1Name = endpoint1.userName,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let ep2Num = endpoint2.number else { return }

		// Turn on Block Caller ID for endpoint 2
		con2.settingSet(type: "Number", name: "BlockCallerId", value: "1")

		// OEP calls DUT
		con1.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)

		// OEP1 transfers DUT to OEP2 with No Caller ID option enabled
		con1.transferCall(number: ep2Num)
		con2.answer()
		sleep(3)

		// Verify that, due to No Caller ID enabled, DUT only sees info from OEP1
		ui.verifyText(text: ep1Name, expected: true)
		con2.hangup()
		con1.hangup()

		// Clean up
		con2.settingSet(type: "Number", name: "BlockCallerId", value: "0")
	}
	
	func test_6573_BlockCallerIDEnabledBlockedCallTransfer() {
		// Test Case: 6573 - BCI: The User with BCI enabled cannot be transferred to another User who has both caller endpoints on the Block Contact List
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let epNum = endpoint.number, let epName = endpoint.userName else { return }

		// Turn on Block Caller ID for endpoint
		con.settingSet(type: "Number", name: "BlockCallerId", value: "1")

		// Switch to alternate device to block both DUT, and OEP with BCI
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: epName, num: epNum)
		ui.blockNumber(name: "6573_DUT", num: dutNum)

		// Switch back to DUT then DUT calls OEP with BCI
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.call(phnum: epNum)
		con.answer()
		sleep(3)

		// DUT attempts to transfer OEP to alternate device that has both accounts blocked
		ui.transferCall(toNum: altNum)
		ui.verifyTransferError()

		// Clean up
		con.hangup()
		con.settingSet(type: "Number", name: "BlockCallerId", value: "0")
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_6578_BlockCallerIDEnabledBlocked() {
		// Test Case: 6578 - BCI: Blocked Caller with BCI will not be able to call.

		// Enable Blocked Caller ID
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: true)

		// Switch to alternate device to block DUT
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "6573_DUT", num: dutNum)

		// Switch back to DUT then DUT calls OEP, verify call fails
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.call(phnum: altNum)
		ui.verifyCallFailed()

		// Clean up
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_7118_BCISettingChangesWithAccounts() {
		// Test Case: 7118 - BCI: Changing numbers changes BCI settings.

		// Enable Hide My Caller ID
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: true)
		ui.heartbeat()
		ui.verifyHideMyCallerIDState(state: true)

		// Switch to alternate device and go to setting
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoSettings()
		
		// Change and make sure Hide My Caller ID is false on the alternate device
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)
		ui.heartbeat()

		// Switch back to DUT to verify BCI settings
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSettings()
		
		// Scroll next to Caller ID by setting Block Anonymouse Calls setting to expected default
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: false)
		ui.heartbeat()
		
		// Verify that the Hide My Caller ID is enabled on DUT
		ui.verifyHideMyCallerIDState(state: true)
		
		// Clean up
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)
		ui.gotoHome()
	}
	
	func test_7127_BCICallHistory() {
		// Test Case: 7127 - BCI: Endpoints do not receive more than one call history if an incoming call has BCI on.
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }

		// Turn on Block Caller ID for endpoint
		con.settingSet(type: "Number", name: "BlockCallerId", value: "1")

		// Delete recent call history
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()

		// Endpoint calls DUT, then verify that only 1 call history item exists per call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		ui.waitForMissedCallBadge(count: "1")
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(count: 1)
		ui.verifyText(text: "No Caller ID", expected: true)
		sleep(3)

		// Clean up
		con.settingSet(type: "Number", name: "BlockCallerId", value: "0")
		ui.gotoHome()
	}
	
	func test_7632_CannotAddBlockedCallerIDFromCallHistory() {
		// Test Case: 7632 - BCI: User is not able to add a contact that shows in call history as a Blocked Caller ID
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }

		// Turn on Block Caller ID for endpoint
		con.settingSet(type: "Number", name: "BlockCallerId", value: "1")

		// Delete call history
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()

		// Have endpoint with Blocked Caller ID enabled call the DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()

		// Navigate to call history and attempt to add caller as contact
		ui.gotoCallHistory(list: "All")
		ui.selectContactFromHistory(contactName: "No Caller ID")
		ui.verifyText(text: "Add Contact", expected: false)

		// Clean up
		con.settingSet(type: "Number", name: "BlockCallerId", value: "0")
		ui.gotoHome()
	}
}
