//
//  EncryptionTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 9/25/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class EncryptionTests: XCTestCase {
	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let endpointsClient = TestEndpointsClient.shared
	let dutNum = UserConfig()["Phone"]!
	let vrsNum = "18015757669"
	
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
//		core.setMediaEncryption(value: "0")
		ui.heartbeat()
		ui.ntouchTerminate()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_11199_RequiredCallHoldServer() {
//		// Test Case: 11199 - Encryption: Required encryption VRS calls can connect to hold server
//
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//
//		// Call Hold Server
//		ui.call(phnum: vrsNum)
//		sleep(3)
//		ui.verifyVRSCall()
//		ui.hangup()
//	}
	
	func test_11216_EncryptionIconPreferredCallPreferred() {
		// Test Case: 11216 - Encryption Icon: Preferred call Preferred
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }

		// Set OEP to Preferred
		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")

		// Set Encryption to Preferred
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Encrypt My Calls", value: true)

		// Call OEP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Verify Encryption Icon
		ui.verifyEncryptionIcon()
		ui.hangup()
	}
	
	func test_11217_EncryptionIconPreferredCallNotPreferred() {
		// Test Case: 11217 - Encryption Icon: Preferred call Preferred
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }

		// Set OEP to Preferred
		con.settingSet(type: "Number", name: "SecureCallMode", value: "0")

		// Set Encryption to Preferred
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Encrypt My Calls", value: true)

		// Call OEP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Verify No Encryption Icon
		ui.verifyNoEncryptionIcon()
		ui.hangup()
		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12094_EncryptionIconRequiredCallRequired() {
//		// Test Case: 12094 - Encryption Icon: Required call Required
//		let endpoint = endpointsClient.checkOut(.vp2)
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
//
//		// Set OEP to Required
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "2")
//
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//
//		// Call OEP
//		ui.call(phnum: vpNum)
//		con.answer()
//		ui.waitForCallOptions()
//		sleep(3)
//
//		// Verify Encryption Icon
//		ui.verifyEncryptionIcon()
//		ui.hangup()
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12095_EncryptionIconRequiredCallPreferred() {
//		// Test Case: 12095 - Encryption Icon: Required call Preferred
//		let endpoint = endpointsClient.checkOut(.vp2)
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
//
//		// Set OEP to Preferred
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
//
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//
//		// Call OEP
//		ui.call(phnum: vpNum)
//		con.answer()
//		ui.waitForCallOptions()
//		sleep(3)
//
//		// Verify Encryption Icon
//		ui.verifyEncryptionIcon()
//		ui.hangup()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12096_EncryptionIconRequiredCallNotPreferred() {
//		// Test Case: 12096 - Encryption Icon: Required call Not Preferred
//		let endpoint = endpointsClient.checkOut(.vp2)
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
//
//		// Set OEP to Not Preferred
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "0")
//
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//
//		// Call OEP
//		ui.call(phnum: vpNum)
//		con.answer()
//		ui.waitForCallOptions()
//		sleep(3)
//
//		// Verify Encryption Icon
//		ui.verifyEncryptionIcon()
//		ui.hangup()
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
//	}
	
	func test_12099_EncryptionIconNotPreferredCallNotPreferred() {
		// Test Case: 12099 - Encryption Icon: Not Preferred call Not Preferred
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }

		// Set OEP to Not Preferred
		con.settingSet(type: "Number", name: "SecureCallMode", value: "0")

		// Set Encryption to Not Preferred
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Encrypt My Calls", value: false)

		// Call OEP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Verify No Encryption Icon
		ui.verifyNoEncryptionIcon()
		ui.hangup()
		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12100_PreferredCallHoldServer() {
//		// Test Case: 12100 - Encryption: Preferred encryption VRS calls can connect to hold server
//		
//		// Set Encryption to Preferred
//		core.setMediaEncryption(value: "1")
//		ui.heartbeat()
//
//		// Call Hold Server
//		ui.call(phnum: vrsNum)
//		sleep(3)
//		ui.verifyVRSCall()
//		ui.hangup()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12101_NotPreferredCallHoldServer() {
//		// Test Case: 12101 - Encryption: Not Preferred encryption VRS calls can connect to hold server
//		
//		// Set Encryption to Not Preferred
//		core.setMediaEncryption(value: "0")
//		ui.heartbeat()
//
//		// Call Hold Server
//		ui.call(phnum: vrsNum)
//		sleep(3)
//		ui.verifyVRSCall()
//		ui.hangup()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12102_TunneledEncryptionIconRequiredCallRequired() {
//		// Test Case: 12102 - Encryption Icon: Tunneled - Required call Required
//		let endpoint = endpointsClient.checkOut(.vp2)
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
//
//		// Set OEP to Required
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "2")
//
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//		
//		// Enable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)
//
//		// Call OEP
//		ui.call(phnum: vpNum)
//		con.answer()
//		ui.waitForCallOptions()
//		sleep(3)
//
//		// Verify Encryption Icon
//		ui.verifyEncryptionIcon()
//		ui.hangup()
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
//		
//		// Disable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: false)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12103_TunneledEncryptionIconRequiredCallPreferred() {
//		// Test Case: 12103 - Encryption Icon: Tunneled - Required call Preferred
//		let endpoint = endpointsClient.checkOut(.vp2)
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
//
//		// Set OEP to Preferred
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
//
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//		
//		// Enable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)
//
//		// Call OEP
//		ui.call(phnum: vpNum)
//		con.answer()
//		ui.waitForCallOptions()
//		sleep(3)
//
//		// Verify Encryption Icon
//		ui.verifyEncryptionIcon()
//		ui.hangup()
//		
//		// Disable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: false)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12104_TunneledEncryptionIconRequiredCallNotPreferred() {
//		// Test Case: 12104 - Encryption Icon: Tunneled - Required call Not Preferred
//		let endpoint = endpointsClient.checkOut(.vp2)
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }
//
//		// Set OEP to Not Preferred
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "0")
//
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//		
//		// Enable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)
//
//		// Call OEP
//		ui.call(phnum: vpNum)
//		con.answer()
//		ui.waitForCallOptions()
//		sleep(3)
//
//		// Verify Encryption Icon
//		ui.verifyEncryptionIcon()
//		ui.hangup()
//		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
//		
//		// Disable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: false)
//		ui.gotoHome()
//	}
	
	func test_12106_TunneledEncryptionIconPreferredCallPreferred() {
		// Test Case: 12106 - Encryption Icon: Tunneled - Preferred call Preferred
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }

		// Set OEP to Preferred
		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")

		// Set Encryption to Preferred
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Encrypt My Calls", value: true)
		
		// Enable Tunneling
		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)

		// Call OEP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Verify Encryption Icon
		ui.verifyEncryptionIcon()
		ui.hangup()
		
		// Disable Tunneling
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: false)
		ui.gotoHome()
	}
	
	func test_12107_TunneledEncryptionIconPreferredCallNotPreferred() {
		// Test Case: 12107 - Encryption Icon: Tunneled - Preferred call Not Preferred
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }

		// Set OEP to Preferred
		con.settingSet(type: "Number", name: "SecureCallMode", value: "0")

		// Set Encryption to Preferred
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Encrypt My Calls", value: true)
		
		// Enable Tunneling
		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)

		// Call OEP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Verify No Encryption Icon
		ui.verifyNoEncryptionIcon()
		ui.hangup()
		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
		
		// Disable Tunneling
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: false)
		ui.gotoHome()
	}
	
	func test_12109_TunneledEncryptionIconNotPreferredCallNotPreferred() {
		// Test Case: 12109 - Encryption Icon: Tunneled - Not Preferred call Not Preferred
		// Test Case: 12099 - Encryption Icon: Not Preferred call Not Preferred
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let vpNum = endpoint.number else { return }

		// Set OEP to Not Preferred
		con.settingSet(type: "Number", name: "SecureCallMode", value: "0")

		// Set Encryption to Not Preferred
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Encrypt My Calls", value: false)
		
		// Enable Tunneling
		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)

		// Call OEP
		ui.call(phnum: vpNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Verify No Encryption Icon
		ui.verifyNoEncryptionIcon()
		ui.hangup()
		con.settingSet(type: "Number", name: "SecureCallMode", value: "1")
		
		// Disable Tunneling
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12110_TunneledRequiredCallHoldServer() {
//		// Test Case: 12110 - Encryption: Tunneled - Required encryption VRS calls can connect to hold server
//		
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//		
//		// Enable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)
//
//		// Call Hold Server
//		ui.call(phnum: vrsNum)
//		sleep(3)
//		ui.verifyVRSCall()
//		ui.hangup()
//		
//		// Disable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: false)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12111_TunneledPreferredCallHoldServer() {
//		// Test Case: 12111 - Encryption: Tunneled - Preferred encryption VRS calls can connect to hold server
//		
//		// Set Encryption to Preferred
//		core.setMediaEncryption(value: "1")
//		ui.heartbeat()
//		
//		// Enable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)
//
//		// Call Hold Server
//		ui.call(phnum: vrsNum)
//		sleep(3)
//		ui.verifyVRSCall()
//		ui.hangup()
//		
//		// Disable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: false)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12112_TunneledNotPreferredCallHoldServer() {
//		// Test Case: 12112 - Encryption: Tunneled - Not Preferred encryption VRS calls can connect to hold server
//		
//		// Set Encryption to Not Preferred
//		core.setMediaEncryption(value: "0")
//		ui.heartbeat()
//		
//		// Enable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: true)
//
//		// Call Hold Server
//		ui.call(phnum: vrsNum)
//		sleep(3)
//		ui.verifyVRSCall()
//		ui.hangup()
//		
//		// Disable Tunneling
//		ui.gotoSettings()
//		ui.setSettingsSwitch(setting: "Tunneling Enabled", value: false)
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_12113_VerifyRequiredSetting() {
//		// Test Case: 12113 - Encryption: Verify Required Setting
//		
//		// Set Encryption to Required
//		core.setMediaEncryption(value: "2")
//		ui.heartbeat()
//		
//		// Verify Encrypt My Calls is Required
//		ui.gotoSettings()
//		ui.verifySettingsSwitch(setting: "Encrypt My Calls", value: true)
//		ui.gotoHome()
//	}
}
