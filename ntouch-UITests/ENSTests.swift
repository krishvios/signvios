//
//  ENSTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 10/4/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import XCTest

class ENSTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
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
		// TODO: Server for CoreAccess currently not available
//		core.setBlockAnonymousCalls(value: "0")
//		core.setDoNotDisturb(value: "0")
		ui.ntouchTerminate()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_7485_8244_8255_DNDCall() {
//		// Test Case: 7485 - ENS: DND is respected by ENS server
//		// Test Case: 8244 - ENS: App rejects incoming call when DND enabled
//		// Test Case: 8255 - ENS: Missed call record generation, Do Not Disturb
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
//		
//		// Delete Call History
//		ui.gotoCallHistory(list: "All")
//		ui.deleteAllCallHistory()
//		
//		// Enable DND
//		ui.gotoSettings()
//		ui.doNotDisturb(setting: 1)
//		ui.heartbeat()
//		
//		// Kill ntouch
//		ui.ntouchTerminate()
//		
//		// Call DUT
//		con.dial(number: dutNum)
//		sleep(30)
//		
//		// Verify call record
//		ui.ntouchActivate()
//		ui.gotoCallHistory(list: "Missed")
//		ui.verifyCallHistory(name: oepName, num: oepNum)
//		
//		// Cleanup
//		ui.gotoSettings()
//		ui.doNotDisturb(setting: 0)
//		ui.gotoHome()
//	}
	
	func test_8144_EnsAdditionalCalls() {
		// Test Case: 8144 - ENS: Receiving a call while app closed doesn't cause app to stop receiving calls
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Kill ntouch
		ui.ntouchTerminate()
		
		// Call DUT
		con.dial(number: dutNum)
		sleep(10)
		
		// Answer
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call DUT again
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		ui.gotoHome()
	}

	func test_8239_8261_8267_9204_ForceKillReceiveCall() {
		// Test Case: 8239 - ENS: Receive incoming call push notification after force kill
		// Test Case: 8261 - ENS: Received call record generation, answer from background
		// Test Case: 8267 - ENS: Call transferred from ENS to device (VP call)
		// Test Case: 9204 - ENS: Respond to push notification: foreground app
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Kill ntouch
		ui.ntouchTerminate()
		
		// Call DUT
		con.dial(number: dutNum)
		sleep(10)
		
		// Answer
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Verify call record
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: oepName, num: oepNum)
		ui.gotoHome()
	}
	
	func test_8245_RejectAnonymousCall() {
		// Test Case: 8245 - ENS: App accepts incoming anonymized call when block anon call enabled
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		con.settingSet(type: "String", name: "BlockCallerId", value: "1")
		
		// Reject Anonymous Calls
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: true)
		ui.heartbeat()
		
		// Kill ntouch
		ui.ntouchTerminate()
		
		// Call DUT
		con.dial(number: dutNum, assert: false)
		sleep(15)
		
		// Verify no incoming call
		ui.ntouchActivate()
		ui.gotoHome()
		
		// Cleanup
		con.settingSet(type: "String", name: "BlockCallerId", value: "0")
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: false)
		ui.heartbeat()
		ui.gotoHome()
	}
	
	func test_8246_AnonymousCall() {
		// Test Case: 8246 - ENS: App accepts incoming anonymized call when block anon call disabled
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		con.settingSet(type: "String", name: "BlockCallerId", value: "1")
		
		// Allow Anonymous Calls
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Block Anonymous Callers", value: false)
		ui.gotoHome()
		ui.heartbeat()

		// Kill ntouch
		ui.ntouchTerminate()

		// Call DUT
		con.dial(number: dutNum)
		sleep(15)

		// Answer
		ui.incomingCall(response: "Answer")
		sleep(3)

		// Cleanup
		ui.hangup()
		con.settingSet(type: "String", name: "BlockCallerId", value: "0")
	}
	
	func test_8247_8254_BlockedCall() {
		// Test Case: 8247 - ENS: App rejects incoming call from blocked caller
		// Test Case: 8254 - ENS: Missed call record generation, blocked call (no record)
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Delete contacts
		ui.deleteAllContacts()
		
		// Block Number
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8247", num: oepNum)
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Kill ntouch
		ui.ntouchTerminate()
		
		// Call DUT
		con.dial(number: dutNum, assert: false)
		sleep(5)
		
		// Verify no call record
		ui.ntouchActivate()
		ui.verifyNoMissedCallBadge()
		ui.gotoCallHistory(list: "Missed")
		ui.verifyNoCallHistory(name: oepName, num: oepNum)
		
		// Cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoHome()
	}
	
	func test_8252_9203_ForceKillIgnoreCall() {
		// Test Case: 8252 - ENS: Missed call record generation, do not respond to call
		// Test Case: 9203 - ENS: Respond to push notification: Do nothing
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Kill ntouch
		ui.ntouchTerminate()
		
		// Call DUT and ignore
		con.dial(number: dutNum)
		sleep(60)
		
		// Verify call record
		ui.ntouchActivate()
		ui.waitForTabBar()
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistory(name: oepName, num: oepNum)
		ui.gotoHome()
	}
	
	func test_8253_9205_ForceKillRejectCall() {
		// Test Case: 8253 - ENS: Missed call record generation, call rejected
		// Test Case: 9205 - ENS: Respond to push notification: reject call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Kill ntouch
		ui.ntouchTerminate()
		
		// Call DUT
		con.dial(number: dutNum)
		sleep(10)
		
		// Decline
		ui.incomingCall(response: "Decline")
		
		// Verify call record
		ui.ntouchActivate()
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistory(name: oepName, num: oepNum)
		ui.gotoHome()
	}
	
	func test_9658_OtherAppReceiveCall() {
		// Test Case: 9658 - ENS: With another app opened, after push notification, when app is opened it will show incoming call on the UI
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Background ntouch
		ui.pressHomeBtn()
		
		// Open Safari
		ui.safariOpen()
		
		// Call DUT
		con.dial(number: dutNum)
		sleep(5)
		
		// Answer
		ui.incomingCall(response: "Answer")
		sleep(3)
		
		// Hangup
		ui.hangup()
	}
	
	func test_9660_ForceQuitOtherAppReceiveCall() {
		// Test Case: 9660 - ENS: With another app opened and ntouch force-quit, after push notification, when app is opened it will show incoming call on the UI
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Force-quit ntouch
		ui.ntouchTerminate()
		
		// Open Safari
		ui.safariOpen()
		
		// Call DUT
		con.dial(number: dutNum)
		sleep(5)
		
		// Answer
		ui.incomingCall(response: "Answer")
		sleep(3)
		
		// Hangup
		ui.hangup()
	}
}
