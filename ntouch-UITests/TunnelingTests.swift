//
//  TunnelingTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 7/30/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import XCTest

class TunnelingTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
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

	func test_3070_TunneledCall() {
		// Test Case: 3070 - Tunneling: Call with tunneling on
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Enable Tunneling
		ui.enableTunneling(setting: 1)
		
		// Call VP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		ui.showCallStats()
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		ui.hangup()
		
		// Disable Tunneling
		ui.enableTunneling(setting: 0)
	}
	
	func test_3071_NonTunneledCall() {
		// Test Case: 3071 - Tunneling: Call with tunneling disabled
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Disable Tunneling
		ui.enableTunneling(setting: 0)
		
		// Call VP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		ui.showCallStats()
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		ui.hangup()
	}
	
	func test_3094_3976_TunneledIncomingCall() {
		// Test Case: 3094 - Tunneling: Incoming call with tunneling on
		// Test Case: 3976 - Tunneling: App is stable when tunneling is turned off and incoming call is answered
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Enable Tunneling
		ui.enableTunneling(setting: 1)
		
		// Call DUT
		con.dial(number: cfg["Phone"]!)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.showCallStats()
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		ui.hangup()
		
		// Disable Tunneling
		ui.enableTunneling(setting: 0)
		
		// Call DUT
		con.dial(number: cfg["Phone"]!)
		ui.incomingCall(response: "Answer")
		ui.showCallStats()
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		sleep(3)
		ui.hangup()
	}
	
	func test_3098_TunneledCallWaiting() {
		// Test Case: 3098 - Tunneling: Able to use Call waiting while tunneling
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Enable Tunneling
		ui.enableTunneling(setting: 1)
		
		// Add Contacts
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 3098-1", homeNumber: oepNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 3098-2", homeNumber: oepNum2, workNumber: "", mobileNumber: "")
		
		// Call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(3)
		ui.showCallStats()
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		
		// Call DUT
		con2.dial(number: cfg["Phone"]!)
		
		// Call Waiting "Hold + Answer"
		ui.callWaiting(response: "Hold & Answer")
		ui.showCallStats()
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		
		// Switch Calls and verify
		ui.switchCall()
		ui.verifyCallerID(text: "Automate 3098-1")
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		ui.switchCall()
		ui.verifyCallerID(text: "Automate 3098-2")
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		
		// Hangup both calls
		ui.hangup()
		ui.hangup()
		
		// Disable Tunneling
		ui.enableTunneling(setting: 0)
		
		// Delete Contacts
		ui.deleteAllContacts()
		ui.gotoHome()
	}
	
	func test_4009_ToggleTunneling5x() {
		// Test Case: 4009 - Tunneling: Calling function is stable after switching from enable to disable several times
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
			
		// Toggle Tuenneling on and off
		ui.gotoSettings()
		for _ in 1...5 {
			ui.toggleTunneling()
		}
		ui.enableTunneling(setting: 0)
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		ui.showCallStats()
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		ui.hangup()
	}
	
	func test_7462_TunneledCallx5x() {
		// Test Case: 7462 - Tunneling: User is able to place multiple calls while tunneling
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Enable Tunneling
		ui.enableTunneling(setting: 1)
		
		// Call VP
		for _ in 1...5 {
			ui.call(phnum: oepNum)
			con.answer()
			sleep(3)
			ui.showCallStats()
			ui.verifyFpsSentGreaterThan(value: 0)
			ui.verifyFpsReceivedGreaterThan(value: 0)
			ui.hangup()
		}
		
		// Disable Tunneling
		ui.enableTunneling(setting: 0)
	}
}
