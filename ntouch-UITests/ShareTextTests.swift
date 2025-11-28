//
//  ShareTextTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 3/30/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class ShareTextTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["shareTextAltNum"]!
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
	
	func test_2933_ShareClearText() {
		// Test Case: 2933 - Share Text: A user can share text and clear text
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		
		// Answer
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Share Text
		ui.shareText(text: "2933")
		ui.verifySelfViewText(text: "2933")
		
		// Clear Text
		ui.clearSharedText()
		ui.verifyNoSharedText(text: "2933")
		
		// Hangup
		ui.hangup()
	}
	
	func test_3272_ShareTextHangup() {
		// Test Case: 3272 - Share Text: Placing a call after using share text will not display text from previous call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Share Text
		ui.shareText(text: "3272")
		ui.verifySelfViewText(text: "3272")
		
		// Hangup
		ui.hangup()
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Verify no shared text
		ui.verifyNoSharedText(text: "3272")
		
		// Hangup
		ui.hangup()
	}
	
	func test_3276_ClearRemoteText() {
		// Test Case: 3276 - Share Text: DUT is able to clear the text from a remote endpoint
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// OEP Share Text
		con.shareText(text: "3276")
		ui.verifyRemoteViewText(text: "3276")
		
		// Clear Text
		ui.clearSharedText()
		ui.verifyNoSharedText(text: "3276")
		
		// Hangup
		ui.hangup()
	}
	
	func test_3280_ClearAllText() {
		// Test Case: 3280 - Share Text: Clear All Text button on the DUT clears text shared from both endpoints
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// DUT Share Text
		ui.shareText(text: "DUT 3280")
		
		// OEP Share Text
		con.shareText(text: "OEP 3280")
		
		// Verify Text
		ui.verifySelfViewText(text: "DUT 3280")
		ui.verifyRemoteViewText(text: "OEP 3280")
		
		// Clear Text
		ui.clearSharedText()
		ui.verifyNoSharedText(text: "DUT 3280")
		ui.verifyNoSharedText(text: "OEP 3280")
		
		// Hangup
		ui.hangup()
	}
	
	func test_3304_3312_3314_AddEditDeleteSavedText() {
		// Test Case: 3304 - Share Text: Save Text
		// Test Case: 3312 - Share Text: Edit a saved text
		// Test Case: 3314 - Share Text: Delete a saved text
		
		// Test 3304
		// Add Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "3304")
		ui.verifySavedText(text: "3304")
		
		// Test 3312
		// Edit Saved Text
		ui.editSavedText(text: "3304", newText: "_3312")
		
		// Test 3314
		// Delete Saved Text
		ui.deleteSavedText(text: "3304_3312")
		ui.gotoHome()
	}

	func test_3306_3310_ShareTextSavedText() {
		// Test Case: 3306 - Share Text: Entered text with saved text
		// Test Case: 3310 - Share Text: Add saved text box
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Add Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "3306")
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// DUT Share Text
		ui.shareText(text: "DUT")
		ui.verifySelfViewText(text: "DUT")
		
		// Share Saved Text
		ui.shareSavedText(text: "3306")
		ui.verifySelfViewText(text: "DUT 3306")
		
		// Add New Saved Text in call
		ui.share()
		ui.selectSavedText()
		ui.addSavedText(text: "3310")
		ui.selectClose()
		
		// Hangup
		con.hangup()
		
		// Delete Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.gotoHome()
	}
	
	func test_3308_3309_SavedTextLimit() {
		// Test Case: 3308 - Share Text: DUT can save strings of up to 88 characters
		// Test Case: 3309 - Share Text: DUT can send saved strings of up to 88 characters
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		let savedText88Char = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678"
		
		// Add Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: savedText88Char)
		ui.verifySavedText(text: savedText88Char)
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Share Saved Text
		ui.shareSavedText(text: savedText88Char)
		ui.verifySelfViewText(text: savedText88Char)
		
		// Hangup
		ui.hangup()
		
		// Delete Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.gotoHome()
	}
	
	func test_3311_5384_5393_SavedTextLimitMaximum() {
		// Test Case: 3311 - Share Text: Add no more than 10 saved text boxes
		// Test Case: 5384 - Saved Text: User is not able to add more saved text after logging back in when there are already 10 entries
		// Test Case: 5393 - Saved Text: Adding then deleting saved text will not allow user to add more than 10 saved text entries
		
		// Create 10 SavedTexts
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "Test1")
		ui.addSavedText(text: "Test2")
		ui.addSavedText(text: "Test3")
		ui.addSavedText(text: "Test4")
		ui.addSavedText(text: "Test5")
		ui.addSavedText(text: "Test6")
		ui.addSavedText(text: "Test7")
		ui.addSavedText(text: "Test8")
		ui.addSavedText(text: "Test9")
		ui.addSavedText(text: "Test10")
		
		// Test 3311
		// Verify Max Saved Text
		ui.selectAddSavedText()
		ui.verifyMaxSavedTexts()
		
		// Test 5393
		// Delete a few saved texts
		// Add more saved texts back to maximum
		ui.deleteSavedText(text: "Test5")
		ui.deleteSavedText(text: "Test4")
		ui.addSavedText(text: "Test5_2")
		ui.addSavedText(text: "Test4_2")
		
		// Attempt to add over maximum and verify warning message preventing you from adding more
		ui.selectAddSavedText()
		ui.verifyMaxSavedTexts()
		
		// Test 5384
		// Logout
		ui.logout()
		ui.login()
		
		// Attempt to create more than 10
		ui.gotoSavedText()
		ui.selectAddSavedText()
		ui.verifyMaxSavedTexts()
	
		// Clean up
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.gotoHome()
	}
	
	func test_3313_DeleteTextInSavedTextbox() {
		// Test Case: 3313 - Share text: Delete all text within a text box
		
		// Add Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "3313")
		ui.verifySavedText(text: "3313")
		
		// Delete Saved Text in textbox
		ui.deleteSavedTextInBox(text: "3313")
		
		// Delete Saved Text
		ui.verifyNoSavedText(text: "3313")
		ui.deleteAllSavedText()
		ui.gotoHome()
	}
	
	func test_3316_ShareTextSavedTextRotate() {
		// Test Case: 3316 - Share Text: Rotate device while on the saved text screen
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Add Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "3316")

		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Open Saved Text and verify
		ui.share()
		ui.selectSavedText()
		ui.verifySavedText(text: "3316")

		// Rotate and Verify Saved Text
		ui.rotate(dir: "Left")
		ui.verifySavedText(text: "3316")
		ui.rotate(dir: "Portrait")
		ui.verifySavedText(text: "3316")

		// Hangup
		con.hangup()

		// Delete Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.gotoHome()
	}
	
	func test_3317_EditSavedTextInCallBackground() {
		// Test Case: 3317 - Share Text: Background and Foreground app while editing a saved text
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Add Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "3317")
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Share Saved Text
		ui.shareSavedText(text: "3317")
		ui.verifySelfViewText(text: "3317")
		
		// Edit Saved Text in call
		ui.share()
		ui.selectSavedText()
		ui.selectEdit()
		ui.editSavedText(text: "3317", newText: "_1")
		
		// Background, Foreground, verify text
		ui.pressHomeBtn()
		ui.ntouchActivate()
		ui.verifySavedText(text: "3317_1")
		ui.selectClose()
		
		// Hangup
		con.hangup()
		
		// Delete Saved Text
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.gotoHome()
	}
	
	func test_3466_ShareTextByChar() {
		// Test Case: 3466 - Share Text: Ability to share text character by character
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)

		// Share Text
		ui.shareText(text: "3")
		ui.verifySelfViewText(text: "3")
		ui.shareText(text: "4")
		ui.verifySelfViewText(text: "34")
		ui.shareText(text: "6")
		ui.verifySelfViewText(text: "346")
		ui.shareText(text: "6")
		ui.verifySelfViewText(text: "3466")

		// Hangup
		ui.hangup()
	}
	
	func test_3468_ReceiveTextByChar() {
		// Test Case: 3468 - Share Text: Ability to receive text character by character
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)

		// OEP Share Text
		con.shareText(text: "3")
		ui.verifyRemoteViewText(text: "3")
		con.shareText(text: "4")
		ui.verifyRemoteViewText(text: "34")
		con.shareText(text: "6")
		ui.verifyRemoteViewText(text: "346")
		con.shareText(text: "8")
		ui.verifyRemoteViewText(text: "3468")

		// Hangup
		ui.hangup()
	}
	
	func test_3472_3473_3474_RemoteTextCallWaitingSwitchCalls() {
		// Test Case: 3472 - Share Text: Call Waiting - Switching to a call where OEP is not sharing text
		// Test Case: 3473 - Share Text: Call Waiting - Switching to a call where OEP is sharing text
		// Test Case: 3474 - Share Text: Call Waiting - Switching to calls where both OEPs are sharing text
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1),
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// OEP1 Call DUT
		con1.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		ui.waitForCallOptions()

		// OEP1 Share Text to DUT
		con1.shareText(text: "3472")
		ui.verifyRemoteViewText(text: "3472")

		// Test 3472
		// OEP2 Call DUT and verify no text
		con2.dial(number: dutNum)
		ui.callWaiting(response: "Hold & Answer")
		ui.verifyNoSharedText(text: "3472")

		// Test 3473
		// Switch Call and verify text
		ui.switchCall()
		ui.verifyRemoteViewText(text: "3472")

		// Test 3474
		// OEP2 Share Text to DUT and verify text
		ui.switchCall()
		con2.shareText(text: "3474")
		ui.verifyRemoteViewText(text: "3474")

		// Switch Call and verify text
		ui.switchCall()
		ui.verifyRemoteViewText(text: "3472")
		con2.hangup()
		con1.hangup()
	}

	func test_3475_ShareTextCallWaitingHangup() {
		// Test Case: 3475 - Share Text: Call Waiting - Active call where text was shared hangs up
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// DUT call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		ui.waitForCallOptions()
		
		// Share Text between both DUT and OEP1
		ui.shareText(text: "DUT to OEP1")
		con1.shareText(text: "OEP1 to DUT")
		
		// Verify text
		ui.verifySelfViewText(text: "DUT to OEP1")
		ui.verifyRemoteViewText(text: "OEP1 to DUT")

		// OEP2 Call DUT
		con2.dial(number: dutNum)
		ui.callWaiting(response: "Hold & Answer")

		// Share Text between bot DUT and OEP2
		ui.shareText(text: "DUT to OEP2")
		con2.shareText(text: "OEP2 to DUT")
		
		// Verify text
		ui.verifySelfViewText(text: "DUT to OEP2")
		ui.verifyRemoteViewText(text: "OEP2 to DUT")
		
		// OEP2 hangup
		con2.hangup()
		
		// Verify Text
		ui.verifySelfViewText(text: "DUT to OEP1")
		ui.verifyRemoteViewText(text: "OEP1 to DUT")
		
		// Cleanup
		con1.hangup()
	}
	
	func test_3476_ShareTextCallWaitingClearText() {
		// Test Case: 3476 - Share Text: Call Waiting - Switching to a call where DUT is sharing text after deleting all text in current call
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// DUT call OEP1
		ui.call(phnum: oepNum1)
		con1.answer()
		ui.waitForCallOptions()
		
		// DUT Share Text to OEP1
		ui.shareText(text: "DUT to OEP1")
		ui.verifySelfViewText(text: "DUT to OEP1")

		// OEP2 Call DUT
		con2.dial(number: dutNum)
		ui.callWaiting(response: "Hold & Answer")

		// DUT Share Text to OEP2
		ui.shareText(text: "DUT to OEP2")
		ui.verifySelfViewText(text: "DUT to OEP2")
		
		// Switch to OEP1 and clear text
		ui.switchCall()
		ui.verifySelfViewText(text: "DUT to OEP1")
		ui.clearSharedText()
		ui.verifyNoSharedText(text: "DUT to OEP1")
		
		// Switch to OEP2 and verify text
		ui.switchCall()
		ui.verifySelfViewText(text: "DUT to OEP2")
		
		// Cleanup
		con2.hangup()
		con1.hangup()
	}
	
	func test_3560_ShareSavedTextCancel() {
		// Test Case: 3560 - Share Text: Saved Text - User has option to Cancel
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Select share saved text
		ui.share()
		ui.selectSavedText()
		ui.selectClose()
		ui.closeShareTextKeyboard()
		
		// Hangup
		ui.hangup()
	}
	
	func test_3622_ShareTextRemoteClearText() {
		// Test Case: 3622 - Share Text: Clear very large text in response to remote command
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		let shareText88Char = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678"
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Share text 88 characters
		ui.shareText(text: shareText88Char)
		ui.verifySelfViewText(text: shareText88Char)
		
		// OEP Clear Text
		con.clearSharedText()
		ui.verifyNoSharedText(text: shareText88Char)
		
		// Hangup
		ui.hangup()
	}
	
	func test_3626_NoSharedTextRemoteClearText() {
		// Test Case: 3626 - Share Text: Handle remote clear command with no text shared
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)

		// Clear Text on the OEP
		con.clearSharedText()

		// Nothing breaks
		sleep(3)
		ui.hangup()
	}
	
	func test_3786_ShareTextDeleteMultipleRows() {
		// Test Case: 3786 - Share Text: Verify that the app can delete several rows in pre-populated share text
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Share text
		let text = "Test Case: 3786 - Share Text: Verify that the app can delete several rows in pre-populated share text"
		let text2 = text + text
		let textCount = text.count
		ui.shareText(text: text2, closeKeyboard: false)

		// Backspace multiple times
		ui.deleteSharedTextCharacters(count: textCount)
		ui.verifySelfViewText(text: text)
		ui.closeShareTextKeyboard()
		ui.hangup()
	}
	
	func test_3841_ShareTextBackspaceMultipleTimes() {
		// Test Case: 3841 - Share Text: backspacing text several times
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Share text
		let text = "3841_share_text_backspace_multiple_times"
		let textCount = text.count - 4
		ui.shareText(text: text, closeKeyboard: false)
		ui.verifySelfViewText(text: text)

		// Backspace multiple times
		ui.deleteSharedTextCharacters(count: textCount)
		ui.verifySelfViewText(text: "3841")
		ui.closeShareTextKeyboard()
		ui.hangup()
	}
	
	func test_3941_ShareLocationBackground() {
		// Test Case: 3941 - Share Location: Location sharing remains functioning after backgrounding, then returning
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Share location
		ui.shareLocation()
		ui.verifyLocationContains(text: "UT")

		// Background and Resume
		ui.pressHomeBtn()
		ui.ntouchActivate()

		// Verify Shared location still visible after returning from Background
		ui.verifyLocationContains(text: "UT")
		ui.hangup()
	}
	
	func test_4012_ShareTextCallTransfer() {
		// Test Case: 4012 - Share Text: Share text clears after call is transferred to the endpoint
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1), let oepNum1 = endpoint1.number,
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2), let oepNum2 = endpoint2.number else { return }
		
		// Call OEP1
		ui.call(phnum: oepNum1)

		// Call answered and share text
		con1.answer()
		ui.waitForCallOptions()
		ui.shareText(text: "4012")
		ui.verifySelfViewText(text: "4012")

		// Transfer call to OEP2
		con1.transferCall(number: oepNum2)
		con2.answer()
		sleep(3)

		// Verify share text cleared after transfer
		ui.verifyNoSharedText(text: "4012")
		con2.hangup()
		con1.hangup()
	}
	
	func test_4027_ShareTextHoldingBackspaceButton() {
		// Test Case: 4027 - Share Text: Holding backspace button deletes all remaining text
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else {return}

		// Call Endpoint and Answer
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Share Text
		ui.shareText(text: "4027 - Share Text: Holding backspace button deletes all remaining text", closeKeyboard: false)
		ui.verifySelfViewText(text: "4027 - Share Text: Holding backspace button deletes all remaining text")

		// Hold down backspace button on the keyboard to delete faster
		ui.pressBackspace(duration: 6)
		ui.verifySelfViewText(text: "")
		ui.closeShareTextKeyboard()

		// Hang up
		ui.hangup()
	}

	func test_4101_ShareTextPersistAfterReopeningShareTextBox() {
		// Test Case: 4101 - Share Text: Text shared on the DUT reappears in the share text dialog box
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else {return}

		// Call Endpoint and Answer
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()

		// Share Text
		ui.shareText(text: "4101")
		ui.verifySelfViewText(text: "4101")

		// Verify Share Text persists
		ui.share()
		ui.verifySelfViewText(text: "4101")
		con.hangup()
	}

	func test_4298_SavedTextAfterDeleteWontRepopulate() {
		// Test Case: 4298 - Share Text: Deleting all of the saved texts and then pressing add will not repopulate one of the deleted items
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else {return}
		
		// Delete saved texts, and add to five saved texts
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		for i in 1...5 {
			ui.addSavedText(text: "4298_\(i)")
		}
		
		// Call Endpoint and Answer
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Delete all Saved Texts while in call
		ui.share()
		ui.selectSavedText()
		ui.deleteAllSavedText()
		
		// Click the add button
		app.staticTexts["Add Saved Text"].tap()
		
		// Verify the text field does not reproduce a deleted item
		XCTAssertTrue(app.textFields["required"].exists)
		for i in 1...5 {
			XCTAssertFalse(app.textFields["4298_\(i)"].exists)
		}
		app.buttons["Close"].tap()
		
		// Hang up
		con.hangup()
	}

	func test_4326_EmptySavedTextCannotBeShared() {
		// Test Case: 4326 - Saved Text: Empty fields in Saved Text  cannot be shared
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else {return}
		
		// Call Endpoint and Answer
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Verify share text label is "Share"
		ui.share()
		sleep(3)
		
		// Select Saved Text
		ui.selectSavedText()
		app.textFields["required"].tap()
		sleep(3)
		ui.selectClose()
		ui.verifyNoSavedText(text: "required")
		
		// Hang up
		con.hangup()
	}

	func test_4362_ShareTextOptionLabel() {
		// Test Case: 4362 - Share Text: When sharing text the option is displayed as "share"
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else {return}
		
		// Call Endpoint and Answer
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Verify share text label is "Share"
		ui.verifyShareTextButtonLabel(label: "Share")
		sleep(5)
		
		// Hang up
		ui.hangup()
	}

	func test_4367_ShareTextMultipleSavedText() {
		// Test Case: 4367 - Saved Text: Selecting multiple saved text in a row displays them with one space between each saved text
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else {return}
		
		// Delete all existing saved texts
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		
		// Add multiple saved texts
		ui.addSavedText(text: "4367_text_1")
		ui.addSavedText(text: "4367_text_2")
		ui.gotoHome()
		
		// Call endpoint
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Share multiple saved texts and verify space in between inserted texts
		ui.shareSavedText(text: "4367_text_1")
		ui.shareSavedText(text: "4367_text_2")
		ui.verifySelfViewText(text: "4367_text_1 4367_text_2")
		
		//Hang up
		ui.hangup()
		
		//Clean Up
		ui.gotoSavedText()
		ui.deleteSavedText(text: "4367_text_1")
		ui.deleteSavedText(text: "4367_text_2")
		ui.gotoHome()
	}
	
	func test_5172_ShareTextCallWaiting() {
		// Test Case: 5172 - Share Text: Verify text can be shared and edited to both endpoints.
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1),
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Call DUT with first device, answer with DUT
		con1.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
			
		// Share text
		ui.shareText(text: "5172_1_1")
		
		// Call DUT with second device, answer second call on DUT
		con2.dial(number: dutNum)
		ui.callWaiting(response: "Hold & Answer")
		
		// Share text between DUT and second device
		ui.shareText(text: "5172_2_1")
		
		// Switch back and forth and share text four more times
		// Verifying each time the shared text persisted
		for i in 2 ..< 5 {
			// Switch to call with first device
			ui.switchCall()
			
			// Clear text and share new text with first call
			ui.verifySelfViewText(text: "5172_1_\(i-1)")
			ui.clearSharedText()
			ui.verifyNoSharedText(text: "5172_1_\(i)")
			ui.shareText(text: "5172_1_\(i)")
			ui.verifySelfViewText(text: "5172_1_\(i)")
			
			// Switch to second device
			ui.switchCall()
				
			// Clear text and share new text with second call
			ui.verifySelfViewText(text: "5172_2_\(i-1)")
			ui.clearSharedText()
			ui.verifyNoSharedText(text: "5172_2_\(i)")
			ui.shareText(text: "5172_2_\(i)")
			ui.verifySelfViewText(text: "5172_2_\(i)")
		}
		
		//Hang up both active calls
		ui.hangup()
		ui.hangup()
	}
	
	func test_5173_ShareTextCallWaitingBackground() {
		// Test Case: 5173 - Share Text: Verify sharing text during call waiting is not affected by backgrounding the app.
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1),
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Call DUT with first device, answer with DUT
		con1.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		
		// Share text
		ui.shareText(text: "5173_1")
		
		// Call DUT with second device, answer second call on DUT
		con2.dial(number: dutNum)
		ui.callWaiting(response: "Hold & Answer")
		
		// Share text between DUT and second device
		ui.shareText(text: "5173_2")
		
		// Switch to call with first device
		ui.switchCall()
		
		// Background call and verify backgrounding and call switching does not affect share text function
		ui.pressHomeBtn()
		ui.ntouchActivate()
		ui.clearSharedText()
		ui.verifyNoSharedText(text: "5173_1_2")
		ui.shareText(text: "5173_1_2")
		ui.verifySelfViewText(text: "5173_1_2")
		
		// Switch to second device
		ui.switchCall()
		
		//Hang up both active calls
		ui.hangup()
		ui.hangup()
	}
	
	func test_5174_ShareTextCallWaitingOrientation() {
		// Test Case: 5174 - Share Text: Verify sharing text during call waiting is not affected by changing orientation.
		
		let endpoint1 = endpointsClient.checkOut(.vp2)
		let endpoint2 = endpointsClient.checkOut(.vp2)
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1),
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Call DUT with first device, answer with DUT
		con1.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		
		// Share text
		ui.shareText(text: "5174_1")
		
		// Call DUT with second device, answer second call on DUT
		con2.dial(number: dutNum)
		ui.callWaiting(response: "Hold & Answer")
		
		// Share text between DUT and second device
		ui.shareText(text: "5174_2")
		
		// Change orientation to landscape
		ui.rotate(dir: "Left")

		// Switch to call with first device
		ui.switchCall()
		
		// Verify changed orientation "Landscape" does not affect share text functionality
		ui.shareText(text: "_2")
		ui.verifySelfViewText(text: "5174_1_2")
		ui.clearSharedText()
		ui.verifyNoSharedText(text: "5174_1_2")
		
		// Change orientation back to portrait
		ui.rotate(dir: "Portrait")
		
		// Switch to call with second device
		ui.switchCall()
		
		// Verify changing back to orientation "Portrait" does not affect share text functionality
		ui.shareText(text: "_2")
		ui.verifySelfViewText(text: "5174_2_2")
		ui.clearSharedText()
		ui.verifyNoSharedText(text: "5174_2_2")
		
		//Hang up both active calls
		ui.hangup()
		ui.hangup()
	}

	func test_5268_9560_ShareTextBackground() {
		// Test Case: 5268 - Share Text: When app is brought back to the foreground the text that was entered is not duplicated.
		// Test Case: 9560 - ShareText: User can background app and foreground and see text
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		let sharedText = "Test 5268 & 9560"
		
		// Call OEP
		ui.call(phnum: oepNum)
		
		// Answer
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Share Text
		ui.shareText(text: sharedText)
		ui.verifySelfViewText(text: sharedText)
		
		// Background, Resume and verify text
		ui.pressHomeBtn()
		ui.ntouchActivate()
		ui.verifySelfViewText(text: sharedText)
		ui.hangup()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_5311_DisableShareText() {
//		// Test Case: 5311 - Share Text: All options for ShareText can be disabled in VDM
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
//		
//		// Disable ShareText refresh
//		core.setShareText(value: "0")
//		ui.heartbeat()
//		
//		// Call OEP verify ShareText
//		ui.call(phnum: oepNum)
//		con.answer()
//		ui.waitForCallOptions()
//		sleep(3)
//		ui.verifyNoShareOptions()
//		ui.hangup()
//		
//		// Clean up
//		core.setShareText(value: "1")
//		
//		// Return home
//		ui.gotoHome()
//	}
	
	func test_5380_SavedTextCall10Spots() {
		// Test Case: 5380 - Saved Text: Saved text save spots increased to 10 in a call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Setup
		ui.gotoSavedText()
		ui.deleteAllSavedText()

		// Enable and setup 10 SaveTexts
		ui.addSavedText(text: "Test1")
		ui.addSavedText(text: "Test2")
		ui.addSavedText(text: "Test3")
		ui.addSavedText(text: "Test4")
		ui.addSavedText(text: "Test5")
		ui.addSavedText(text: "Test6")
		ui.addSavedText(text: "Test7")
		ui.addSavedText(text: "Test8")
		ui.addSavedText(text: "Test9")
		ui.addSavedText(text: "Test10")
		
		// Call options and verify ShareText SavedText
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		
		// Verify SavedTexts
		ui.share()
		ui.selectSavedText()
		ui.verifySavedText(text: "Test1")
		ui.verifySavedText(text: "Test2")
		ui.verifySavedText(text: "Test3")
		ui.verifySavedText(text: "Test4")
		ui.verifySavedText(text: "Test5")
		ui.verifySavedText(text: "Test6")
		ui.verifySavedText(text: "Test7")
		ui.verifySavedText(text: "Test8")
		ui.verifySavedText(text: "Test9")
		ui.verifySavedText(text: "Test10")

		// Clean up
		ui.selectClose()
		con.hangup()
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_5382_SavedTextSwitchAccounts() {
		// Test Case: 5382 - Saved Text: Switching between accounts doesn't copy previous saved text into saved text for new account logging in
		
		// Setup SavedTexts on Account1
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "Test 5382")
		ui.verifySavedText(text: "Test 5382")
		ui.logout()
		
		// Account 2
		ui.login(phnum: altNum, password: defaultPassword)
		ui.gotoSavedText()
		ui.verifyNoSavedText(text: "Test 5382")
		ui.selectDone()
		ui.logout()
		
		// Clean up
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_5383_5385_5386_SavedTextEditAll() {
		// Test Case: 5383 - Saved Text: Verified that saved text is not listed alphabetically
		// Test Case: 5385 - Saved Text: User is able to delete all 10 saved text entries
		// Test Case: 5386 - Saved Text: User is able to edit all 10 saved text entries
		
		// Setup SavedTexts
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "B Test5386 1")
		ui.addSavedText(text: "E Test5386 2")
		ui.addSavedText(text: "D Test5386 3")
		ui.addSavedText(text: "G Test5386 4")
		ui.addSavedText(text: "F Test5386 5")
		ui.addSavedText(text: "A Test5386 6")
		ui.addSavedText(text: "H Test5386 7")
		ui.addSavedText(text: "I Test5386 8")
		ui.addSavedText(text: "K Test5386 9")
		ui.addSavedText(text: "J Test5386 10")
		
		// Test 5383
		// Verify Non Alpabetical on background
		ui.gotoHome()
		ui.ntouchActivate()
		ui.gotoSavedText()
		ui.verifySavedTextPositions()
		
		// Test 5386
		// Edit all 10 SavedTexts
		ui.editAllSavedTextsRandomly()
		
		// Test 5385
		// Clean up
		ui.deleteAllSavedTextRandomly()
		
		// Return home
		ui.gotoHome()
	}
	
	func test_7158_ShareText88Characters() {
		// Test Case: 7158 - Share Text: User can send 88 characters without app crashing
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Setup
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Share a large 88 character text
		ui.shareText(text: "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678")
		ui.verifySelfViewText(text: "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678")
	
		// Cleanup
		ui.hangup()
	}
	
	func test_8424_ShareTextVisableWithOritentation() {
		// Test Case: 8424 - Share Text: rotating from portrait to landscape never causes text to disappear from self view
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// setup
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Rotate and verify text
		ui.shareText(text: "Test Orientation View")
		ui.rotate(dir: "Left")
		ui.verifySelfViewText(text: "Test Orientation View")
		ui.rotate(dir: "Right")
		ui.verifySelfViewText(text: "Test Orientation View")
		ui.rotate(dir: "Portrait")
		ui.verifySelfViewText(text: "Test Orientation View")
		
		// Cleanup
		con.hangup()
	}
	
	func test_9284_SavedTextAddTextEditMode() {
		// Test Case: 9284 - Share Text - Saved Text: Clicking Add text button multiple times should place app in edit mode if it's not
		
		// Setup
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.verifyEditButton()
		
		// Select Add Saved Text multiple times
		for _ in 1...3 {
			ui.selectAddSavedText()
		}
		
		// Verify edit mode
		ui.verifyDoneButton()
		ui.selectSettingsBackBtn()
		
		// Return home
		ui.gotoHome()
	}

	func test_9285_SavedTextSelectInSetting() {
		// Test Case: 9285 - Share Text - Saved Text: selecting a saved text in settings should not crash app
		
		// Setup
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "9285")
		
		// Select one of the Saved Texts
		ui.selectSavedTextItem(text: "9285")
		sleep(2)
		
		// Clean up
		ui.deleteAllSavedText()
		ui.gotoHome()
	}

	func test_9331_SavedTextDeleteThenCreateAndShareNewText() {
		// Test Case: 9331 - Saved Text: Able to delete saved text and re-make new saved text
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Setup
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.addSavedText(text: "9331_1")
		ui.addSavedText(text: "9331_2")
		
		// Call Endpoint and Answer
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		
		// Go to Saved Text, edit, and then delete
		ui.share()
		ui.selectSavedText()
		ui.selectEdit()
		ui.editSavedText(text: "9331_1", newText: "_edited")
		ui.verifySavedText(text: "9331_1_edited")
		ui.deleteSavedText(text: "9331_2")
		ui.verifyNoSavedText(text: "9331_2")
		
		// Add new Saved Text, then share newly created Saved Text
		ui.addSavedText(text: "9331_3")
		ui.selectSavedTextItem(text: "9331_3")
		ui.verifySelfViewText(text: "9331_3")
		sleep(3)
		
		// Hang up and clean up
		con.hangup()
		ui.waitForTabBar()
		ui.gotoSavedText()
		ui.deleteAllSavedText()
		ui.gotoHome()
	}
}
