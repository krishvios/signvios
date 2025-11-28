//
//  EnduranceTests.swift
//  ntouch
//
//  Created by Mikael Leppanen on 6/30/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import XCTest

// Errors for repeated calls test to throw.
enum CallError: Error {
	case incomingError(String)
	case outgoingError(String)
	case signMailError(String)
}

class EnduranceTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let dutPassword = UserConfig()["Password"]!
	let endpointsClient = TestEndpointsClient.shared
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: cfg["Orientation"]!)
		continueAfterFailure = true
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
	
	func answerCall() throws {
		let anwerBtn = springboard.buttons["Accept"]
		if anwerBtn.exists {
			sleep(1)
			anwerBtn.tap()
			sleep(3)
			
			// Double CallKit window workaround
			if springboard.buttons["Accept"].waitForExistence(timeout: 3) {
				let anwerBtn2 = springboard.buttons["Accept"]
				if anwerBtn2.exists {
					anwerBtn2.tap()
				}
			}
		}
		else {
			throw(CallError.incomingError("Timeout waiting for incoming call."))
		}
	}
	
	func test_RepeatCalls() {
		// Test Setup
		let maxCalls = 500
		let requiredCalls = 400
		var successfulCalls  = 0
		var failedCalls = 0
		var fpsSent = false
		var fpsReceived = false
		
		// Toggle Stats
		ui.toggleStats()
		
		for totalCalls in 1...maxCalls {
			// Repeat Call Test
			let endpoint = endpointsClient.checkOut(.vp2)
			guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
			
			do {
				// Call
				print("Start Call: \(totalCalls)")
				print("Calling: \(oepNum)")
				ui.call(phnum: oepNum)
				
				// Click Cancel if call failed
				if (app.buttons["Cancel"]).exists {
					sleep(1)
					ui.selectCancel()
					throw(CallError.outgoingError("Outgoing call failed - \(oepNum)"))
				}
				
				// Hangup if sent to SignMail
				ui.selectTop()
				if (app.buttons["Skip"].exists) {
					ui.hangup()
					throw(CallError.signMailError("Call went to SignMail - \(oepNum)"))
				}
				
				// Answer Call
				try _ = con.waitFor(text: "<CallIncoming", assert: false)
				try _ = con.send(text: "<CallAnswer />")
				let connected = try con.waitFor(text: "<CallConnectedEx", assert: false)
				sleep(5)
				if ((connected?.contains("<CallConnectedEx")) != nil) {
					fpsSent = ui.verifyFpsSentGreaterThan0()
					fpsReceived = ui.verifyFpsReceivedGreaterThan0()
					if fpsSent && fpsReceived {
						successfulCalls += 1
					}
					else {
						ui.hangup()
						failedCalls += 1
						print("Call failed - \(oepNum)")
					}
				}
				else {
					ui.hangup()
					failedCalls += 1
					print("Call failed - \(oepNum)")
				}
				con.hangup()
			}
			catch {
				failedCalls += 1
				print("Call failed - \(oepNum)")
			}
			endpointsClient.closeVrclConnections()
			endpointsClient.checkInEndpoints()
			
			print("Total calls: \(totalCalls)")
			print("Successful calls: \(successfulCalls)")
			print("Failed calls: \(failedCalls)")
			
			if successfulCalls == requiredCalls {
				break
			}
		}
		XCTAssertTrue(successfulCalls == requiredCalls, "Successful calls: \(successfulCalls) does not equal Required calls: \(requiredCalls)")
	}
	
	func test_RepeatIncomingCalls() {
		// Test Setup
		let maxCalls = 250
		let requiredCalls = 150
		var successfulCalls  = 0
		var failedCalls = 0
		var fpsSent = false
		var fpsReceived = false
		
		// Toggle Stats
		ui.toggleStats()
		
		// Repeat Incoming Call Test
		for totalCalls in 1...maxCalls {
			let endpoint = endpointsClient.checkOut(.vp2)
			guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
			
			do {
				print("Start Call: \(totalCalls)")
				print("Call from: \(oepNum)")
				con.dial(number: dutNum, assert: false)
				
				// Wait for Incoming Call
				if !springboard.buttons["Accept"].waitForExistence(timeout: 30) {
					throw(CallError.incomingError("Timeout waiting for incoming call."))
				}
				
				// Answer
				try answerCall()
				let connected = try con.waitFor(text: "<CallConnectedEx", assert: false)
				sleep(5)
				if ((connected?.contains("<CallConnectedEx")) != nil) {
					fpsSent = ui.verifyFpsSentGreaterThan0()
					fpsReceived = ui.verifyFpsReceivedGreaterThan0()
					if fpsSent && fpsReceived {
						successfulCalls += 1
						ui.hangup()
					}
					else {
						try _ = con.send(text: "<CallHangUp />")
						failedCalls += 1
						print("Call failed - \(oepNum)")
					}
				}
			}
			catch {
				con.hangup()
				failedCalls += 1
				print("Call failed  - \(oepNum)")
			}
			endpointsClient.closeVrclConnections()
			endpointsClient.checkInEndpoints()
			
			print("Total calls: \(totalCalls)")
			print("Successful calls: \(successfulCalls)")
			print("Failed calls: \(failedCalls)")
			
			if successfulCalls == requiredCalls {
				break
			}
		}
		XCTAssertTrue(successfulCalls == requiredCalls, "Successful calls: \(successfulCalls) does not equal Required calls: \(requiredCalls)")
	}
	
	func test_RepeatIncomingCallsBackground() {
		// Test Setup
		let maxCalls = 250
		let requiredCalls = 150
		var successfulCalls  = 0
		var failedCalls = 0
		var fpsSent = false
		var fpsReceived = false
		
		// Toggle Stats
		ui.toggleStats()
		
		// Repeat Incoming Call Test
		for totalCalls in 1...maxCalls {
			let endpoint = endpointsClient.checkOut(.vp2)
			guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
			
			do {
				// Background ntouch
				print("Background before call: \(totalCalls)")
				ui.pressHomeBtn()
				
				// Call
				print("Start Call: \(totalCalls)")
				print("Call from: \(oepNum)")
				con.dial(number: dutNum, assert: false)
				
				// Wait for Incoming Call
				if !springboard.buttons["Accept"].waitForExistence(timeout: 30) {
					throw(CallError.incomingError("Timeout waiting for incoming call."))
				}
				
				// Answer
				try answerCall()
				let connected = try con.waitFor(text: "<CallConnectedEx", assert: false)
				sleep(5)
				if ((connected?.contains("<CallConnectedEx")) != nil) {
					fpsSent = ui.verifyFpsSentGreaterThan0()
					fpsReceived = ui.verifyFpsReceivedGreaterThan0()
					if fpsSent && fpsReceived {
						successfulCalls += 1
						ui.hangup()
					}
					else {
						try _ = con.send(text: "<CallHangUp />")
						failedCalls += 1
						print("Call failed - \(oepNum)")
					}
				}
			}
			catch {
				con.hangup()
				failedCalls += 1
				print("Call failed  - \(oepNum)")
			}
			endpointsClient.closeVrclConnections()
			endpointsClient.checkInEndpoints()
			
			print("Total calls: \(totalCalls)")
			print("Successful calls: \(successfulCalls)")
			print("Failed calls: \(failedCalls)")
			
			if successfulCalls == requiredCalls {
				break
			}
		}
		XCTAssertTrue(successfulCalls == requiredCalls, "Successful calls: \(successfulCalls) does not equal Required calls: \(requiredCalls)")
	}
	
	func test_RepeatIncomingCallsENS(){
		// Test Setup
		let maxCalls = 250
		let requiredCalls = 150
		var successfulCalls  = 0
		var failedCalls = 0
		var fpsSent = false
		var fpsReceived = false
		
		// Close ntouch
		print("Close ntouch before 1st call")
		ui.pressHomeBtn()
		ui.ntouchTerminate()
		
		// Repeat Incoming Call Test
		for totalCalls in 1...maxCalls {
			let endpoint = endpointsClient.checkOut(.vp2)
			guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
			
			do {
				// Call
				print("Start Call: \(totalCalls)")
				print("Call from: \(oepNum)")
				con.dial(number: dutNum, assert: false)
				
				// Wait for Incoming Call
				if !springboard.buttons["Accept"].waitForExistence(timeout: 30) {
					throw(CallError.incomingError("Timeout waiting for incoming call."))
				}
				
				// Answer
				try answerCall()
				let connected = try con.waitFor(text: "<CallConnectedEx", assert: false)
				sleep(5)
				if ((connected?.contains("<CallConnectedEx")) != nil) {
					ui.showCallStats()
					fpsSent = ui.verifyFpsSentGreaterThan0()
					fpsReceived = ui.verifyFpsReceivedGreaterThan0()
					if fpsSent && fpsReceived {
						successfulCalls += 1
						ui.hangup()
					}
					else {
						try _ = con.send(text: "<CallHangUp />")
						failedCalls += 1
						print("Call failed - \(oepNum)")
					}
				}
			}
			catch {
				con.hangup()
				failedCalls += 1
				print("Call failed  - \(oepNum)")
			}
			endpointsClient.closeVrclConnections()
			endpointsClient.checkInEndpoints()
			
			print("Total calls: \(totalCalls)")
			print("Successful calls: \(successfulCalls)")
			print("Failed calls: \(failedCalls)")
			
			// Close ntouch
			print("Close ntouch after call: \(totalCalls)")
			ui.pressHomeBtn()
			ui.ntouchTerminate()
			
			if successfulCalls == requiredCalls {
				break
			}
		}
		XCTAssertTrue(successfulCalls == requiredCalls, "Successful calls: \(successfulCalls) does not equal Required calls: \(requiredCalls)")
	}
	
	func test_IdleENS() {
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Place call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		
		// Close ntouch
		print("Close ntouch")
		ui.pressHomeBtn()
		ui.ntouchTerminate()
		
		// Wait for idle time
		sleep(28800) // 8 hrs
		
		let endpoint2 = endpointsClient.checkOut()
		guard let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// ENS Call
		con2.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.showCallStats()
		ui.verifyFpsSentGreaterThan(value: 0)
		ui.verifyFpsReceivedGreaterThan(value: 0)
		ui.hangup()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}
	
	func test_SwitchCalls() {
		// Test Setup
		let maxSwitchCalls = 150
		
		// Toggle Stats
		ui.toggleStats()
		
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		print("Dial Call: 1")
		ui.call(phnum: oepNum1)
		con1.answer()
		sleep(10)
		
		print("Add Call: 2")
		ui.addCall(num: oepNum2)
		con2.answer()
		sleep(10)
		
		for i in 1...maxSwitchCalls {
			ui.verifyFpsSentGreaterThan(value: 0)
			ui.verifyFpsReceivedGreaterThan(value: 0)
			print("Switch Call: " +  String(i))
			ui.switchCall()
			sleep(5)
		}
		
		print("Hangup Call: 1")
		ui.hangup()
		print("Hangup Call: 2")
		ui.hangup()
	}
	
	func test_TransferCalls() {
		// Test Setup
		let maxCalls = 200
		let requiredCalls = 150
		var successfulCalls  = 0
		var failedCalls = 0
		var fpsSent = false
		var fpsReceived = false
		
		// Toggle Stats
		ui.toggleStats()
		
		for totalCalls in 1...maxCalls {
			let endpoint1 = endpointsClient.checkOut(.vp2)
			let endpoint2 = endpointsClient.checkOut(.vp2)
			guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
			
			do {
				print("Dial Call: " + String(totalCalls))
				ui.call(phnum: oepNum1)
				
				// Click Cancel if call failed
				if (app.buttons["Cancel"]).exists {
					sleep(1)
					ui.selectCancel()
					throw(CallError.outgoingError("Outgoing call failed - \(oepNum1)"))
				}
				
				// Hangup if sent to SignMail
				ui.selectTop()
				if (app.buttons["Skip"].exists) {
					ui.hangup()
					throw(CallError.signMailError("Call went to SignMail - \(oepNum1)"))
				}
				
				try _ = con1.waitFor(text: "<CallIncoming")
				try _ = con1.send(text: "<CallAnswer />")
				let connected = try con1.waitFor(text: "<CallConnectedEx", assert: false)
				sleep(5)
				if ((connected?.contains("<CallConnectedEx")) != nil) {
					fpsSent = ui.verifyFpsSentGreaterThan0()
					fpsReceived = ui.verifyFpsReceivedGreaterThan0()
					if fpsSent && fpsReceived {
						successfulCalls += 1
						print("Transfer Call: " + String(totalCalls))
						ui.transferCall(toNum: oepNum2)
						con2.answer()
						sleep(5)
					}
					else {
						ui.hangup()
						failedCalls += 1
						print("Call failed - \(oepNum1)")
					}
				}
				else {
					ui.hangup()
					failedCalls += 1
					print("Call failed - \(oepNum1)")
				}
				
				print("Hangup Call: " + String(totalCalls))
				con1.hangup()
				con2.hangup()
			}
			catch {
				failedCalls += 1
				print("Call failed - \(oepNum1)")
			}
			
			endpointsClient.closeVrclConnections()
			endpointsClient.checkInEndpoints()
			
			print("Total calls: \(totalCalls)")
			print("Successful calls: \(successfulCalls)")
			print("Failed calls: \(failedCalls)")
			
			if successfulCalls == requiredCalls {
				break
			}
		}
		XCTAssertTrue(successfulCalls == requiredCalls, "Successful calls: \(successfulCalls) does not equal Required calls: \(requiredCalls)")
	}
	
	func test_TabNavigation() {
		print("Delete Call History")
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
		
		for i in 1...150 {
			print("Iteration: " + String(i))
			ui.gotoCallHistory(list: "All")
			ui.gotoCallHistory(list: "Missed")
			ui.gotoSorensonContacts()
			ui.gotoDeviceContacts()
			ui.gotoFavorites()
			ui.gotoSignMail()
			ui.gotoVideoCenter()
			ui.gotoSettings()
			ui.gotoHome()
		}
	}
	
	func test_CallDuration() {
		let endpoint = endpointsClient.checkOut(.vp2)
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		var fpsSent = false
		var fpsReceived = false
		
		// Toggle Stats
		ui.toggleStats()
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// 12 hr call check video every hour
		for hours in 1...12 {
			print("Hour: \(hours)")
			sleep(3600) // 1 hr
			fpsSent = ui.verifyFpsSentGreaterThan0()
			fpsReceived = ui.verifyFpsReceivedGreaterThan0()
			if !fpsSent || !fpsReceived {
				XCTFail("Call failed - Frame rate sent: \(fpsSent), Frame rate received: \(fpsReceived)")
				break
			}
		}
		ui.hangup()
	}
	
	func test_SignMailSender() {
		//Reminder: before running this test, the remote endpoint should be set to do not disturb or have their ringcount lowered to reduce the time per signmail to send.
		//With current waiting configuration, test runs at 1m 25s per signmail.
		
		//edit to define how many signmails to send
		let numToSend = 20
		
		// Place call and record, edit number to endpoint to which you want to send
		ui.call(phnum: "18088080004")
		ui.leaveSignMailRecording(time: 15, response: "Send", doSkip: false)
		
		//repeat (with faster dialing method) numToSend-1 more times
		for _ in 1...numToSend {
			
			// place call and record
			ui.redial()
			ui.leaveSignMailRecording(time: 15, response: "Send",  doSkip: false)
			
		}
	}
	
	func test_maxSignMails() {
		// Test Case: 3952 - SignMail: Verify that deleting SignMails does not cause any problems
		// Delete All SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login to alt account to send message
		ui.switchAccounts(number: "19323887363", password: "1234")
		
		// Send 200 SignMails
		for i in 1...200 {
			ui.sendSignMailToNum(phnum: dutNum)
			ui.leaveSignMailRecording(time: 5, response: "Send", doSkip: false)
			print("Send SignMail: \(i)")
		}
		
		// Verify Mailbox full
		ui.sendSignMailToNum(phnum: dutNum)
		ui.verifyMailboxFull(response: "Cancel")
		
		// Login to DUT account
		ui.switchAccounts(number: dutNum, password: dutPassword)
		
		// Delete SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}
	
	func test_maxFavorites() {
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add 30 Favorites
		ui.gotoSorensonContacts()
		for i in 101...130 {
			ui.addContactWithFavorite(name: "Auto \(i)", homeNumber: "18015550\(i)", workNumber: "", mobileNumber: "")
			print("Added Favorite: \(i)")
		}
		
		// Add Contact
		ui.addContact(name: "Auto", homeNumber: "18015550001", workNumber: "", mobileNumber: "")
		
		// Add Favorite Verify error
		ui.selectContact(name: "Auto")
		ui.addToFavorites()
		ui.verifyFavoritesListFull()
		ui.saveContact()
		
		// Delete Contacts
		ui.deleteAllContacts()
	}
	
	func test_maxBlocked() {
		ui.switchAccounts(number: "19323883030", password: "1234")
		
		// Delete Blocked List
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Add 200 Blocked numbers
		for i in 1074...1200 {
			ui.blockNumber(name: "Auto \(i)", num: "1801555\(i)")
			ui.gotoBlockedList()
			print("Added Blocked Number: \(i)")
		}
		
		// Verify Add Blocked error
		ui.blockNumber(name: "Auto", num: "18015550001")
		ui.verifyUnableToBlock()
		
		// Delete Blocked List
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoHome()
	}
	
	func test_maxContacts() {
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Import 500 Contacts
		ui.importVPContacts(phnum: "18018679000", pw: "1234", expectedNumber: "500")
		
		// Verify Contacts Count
		ui.gotoSorensonContacts()
		ui.verifyContactCount(count: 503)
		
		// Verify Add Contact error
		ui.selectAddContact()
		ui.verifyCannotAddContact()
		
		// Delete Contacts
		ui.deleteAllContacts()
	}
}
