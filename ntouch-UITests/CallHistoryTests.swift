//
//  CallHistoryTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 3/8/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import XCTest

class CallHistoryTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let dutPassword = UserConfig()["Password"]!
	let endpointsClient = TestEndpointsClient.shared
	let svrsNum = "18663278877"
	let csNum = "611"
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: cfg["Orientation"]!)
		continueAfterFailure = false
		ui.ntouchActivate()
		ui.clearAlert()
		ui.waitForMyNumber()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.deleteAllContacts()
	}
	
	override func tearDown() {
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}

	func test_0899_EditContactFromHistory() {
		// Test Case: 899 - Call History: Ability to edit a contact from the call history
		// Setup and call history entry
		ui.gotoHome()
		ui.call(phnum: "18015550899")
		sleep(3)
		ui.hangup()
		
		// Add Contact from history
		ui.gotoCallHistory(list: "All")
		ui.addContactFromHistory(num: "18015550899", contactName: "Automate 0899")
		
		// select an entry from the contact and edit contact
		ui.selectEdit()
		ui.removeContactName(name: "Automate 0899")
		ui.enterContact(name: "Automate 0899-A")
		ui.saveContact()
		ui.gotoHome()
		
		// verify contact is edited
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Automate 0899-A")
		
		// cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 0899-A")
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_0900_2352_2529_2944_4557_5224_AddContactFromHistory() {
		// Test Case: 900 - Call History: Create new nTouch contacts from call history lists
		// Test Case: 2352 - Call History: Adding a contact properly updates call history
		// Test Case: 2529 - Call History: In app call history updates in a reasonable time frame
		// Test Case: 2944 - Call History: Pressing the 'view all' button does not prevent the user from seeing additional call history items the next time he enters call history
		// Test Case: 4557 - Call History: In app call history displays only a single entry per call
		// Test Case: 5224 - Call History: List Filter remains after receiving a Call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }

		// Incoming Call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()

		// Test 2352, 2529
		// Add from history - Incoming
		ui.gotoCallHistory(list: "All")
		ui.addContactFromHistory(num: oepNum, contactName: "Automate 900")

		// Verify call history item
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "Automate 900", num: oepNum)

		// Test 900
		// Verify contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Automate 900")
		ui.deleteContact(name: "Automate 900")

		// Missed Call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")

		// Test 2352, 2529
		// Add from history - Missed
		ui.heartbeat()
		ui.gotoCallHistory(list: "Missed")
		ui.addContactFromHistory(num: oepNum, contactName: "Automate 900")

		// Verify call history item
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistory(name: "Automate 900", num: oepNum)

		// Test 900
		// Verify contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Automate 900")
		
		// Call contact
		testUtils.calleeNum = oepNum
		ui.callContact(name: "Automate 900", type: "home")
		sleep(3)
		ui.hangup()

		// Outgoing Call
		ui.call(phnum: "18015757669")
		sleep(3)
		ui.hangup()

		// Test 2352, 2529
		// Add from history - Outgoing
		ui.gotoCallHistory(list: "All")
		ui.addContactFromHistory(num: "18015757669", contactName: "Automate 2352")

		// Verify call history item
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "Automate 2352", num: "18015757669")
		
		// Test 900
		// Verify contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Automate 2352")
		
		// Call contact
		testUtils.calleeNum = "18015757669"
		ui.callContact(name: "Automate 2352", type: "home")
		sleep(3)
		ui.hangup()
		
		// Test 2944
		// Go to Call History and Search Contact
		ui.gotoCallHistory(list: "All")
		ui.searchCallHistory(item: "Automate 2352")
		ui.verifyCallHistory(count: 1)
		ui.clearSearch()
		
		// Test 4557
		// Go to All Call History and Verify
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(count: 2)
		
		// Test 5224
		// Miss Call and Verify
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		sleep(3)
		ui.heartbeat()
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(count: 3)
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.deleteAllContacts()
	}

	func test_0905_callNonContactNumer() {
		// Test Case: 905 - Call History: Make a call to an unknown caller - (a number that has not been previously dialed)
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Place call
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		ui.hangup()
		
		// Verify Call History Item
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: oepName, num: oepNum)
		ui.deleteAllCallHistory()
	}
	
	func test_0916_ReturnCallContactNumber() {
		// Test Case: 916 - Call History: Return a call from the call history to an "nTouch Mobile" contact type
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 916", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call from history
		ui.gotoCallHistory(list: "All")
		ui.callFromHistory(name: "Automate 916", num: oepNum)
		con.answer()
		sleep(3)
		ui.hangup()
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 916")
		ui.gotoHome()
	}
	
	func test_0917_ReturnCallNonContactNumber() {
		// Test Case: 917 - Call History: Return a call to an caller who is not a contact
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Call from history
		ui.gotoCallHistory(list: "All")
		ui.callFromHistory(name: oepName, num: oepNum)
		con.answer()
		sleep(3)
		ui.hangup()
		
		// Delete call history
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_0929_MissedCallsDifferentNum() {
		// Test Case: 929 - Call History: Multiple missed calls from different numbers
		let endpoint1 = endpointsClient.checkOut()
		let endpoint2 = endpointsClient.checkOut()
		guard let con1 = endpointsClient.tryToConnectToEndpoint(endpoint1),
			let con2 = endpointsClient.tryToConnectToEndpoint(endpoint2) else { return }
		
		// Call DUT
		con1.dial(number: dutNum)
		ui.incomingCall(response: "Decline")

		// Call DUT
		con2.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
	
		// Verify Missed Call Badge
		ui.waitForMissedCallBadge(count: "2")
	
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_0930_4186_MissedCallsSameNum() {
		// Test Case: 930 - Call History: Multiple missed calls from the same number
		// Test Case: 4186 - Call History: missed call badge is cleared when call history is deleted
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		
		// Call DUT
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		
		// Test 930
		// Verify Missed Call Badge
		ui.waitForMissedCallBadge(count: "2")
		
		// Test 4186
		// Delete All and verify Badge cleared
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.verifyNoMissedCallBadge()
		ui.gotoHome()
	}
	
	func test_0931_ReturnMissedCall() {
		// Test Case: 931 - Call History: Return missed call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }

		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 931", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		
		// Call DUT
		con.dial(number: dutNum)
		
		// Decline and wait for missed call
		ui.incomingCall(response: "Decline")
		ui.waitForMissedCallBadge(count: "1")
		
		// Return Missed Call
		ui.gotoCallHistory(list: "Missed")
		ui.callFromHistory(name: "Automate 931", num: oepNum)

		// Answer
		con.answer()
		sleep(3)
		
		// Hangup
		ui.hangup()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 931")
		ui.gotoHome()
	}
	
	func test_1537_CallVRSFromHistory() {
		// Test Case: 1537 - Call History: Handle calls to "Non-Core" numbers initiated from the call history list
		
		// Call Hearing number
		ui.call(phnum: "18015757669")
		sleep(3)
		ui.hangup()
		
		// Test 1537
		// Place Call from history
		ui.gotoCallHistory(list: "All")
		ui.callFromHistory(num: "18015757669")
		sleep(3)
		ui.hangup()
		
		// Delete call history
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.rotate(dir: "Portrait")
		ui.gotoHome()
	}
	
	func test_1610_CallRedirectedNumber() {
		// Test Case: 1610 - Call History: New Call History Entries created for redirected calls
		
		// Place call to Redirected number
		ui.call(phnum: "19638010007")

		// Select Continue for redirected number and wait for SignMail recording
		ui.redirectedMessage(response: "Continue")
		ui.leaveSignMailRecording(time: 2, response: "Exit", doSkip: false)
		
		// Verify Call History
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(num: "19638010008")
		ui.verifyNoCallHistory(num: "19638010007")
		
		// Cleanup
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_2115_GroupMissedCalls() {
		// Test Case: 2115 - Call History: Missed calls from same number will be grouped in recent summary list
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }

		// Place Calls to DUT
		for _ in 1...3 {
			con.dial(number: dutNum)
			ui.waitForIncomingCall()
			con.hangup()
		}

		// Verify Call History
		ui.waitForMissedCallBadge(count: "3")
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: oepName, num: oepNum)
		ui.verifyCallHistory(count: 1)

		// Cleanup
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_2116_GroupPlacedCalls() {
		// Test Case: 2116 - Call History: Placed calls from same number will be grouped in recent summary list
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }

		// Place Calls
		for _ in 1...3 {
			ui.call(phnum: oepNum)
			con.waitForIncomingCall()
			ui.hangup()
		}

		// Verify Call History
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(num: oepNum)
		ui.verifyCallHistory(count: 1)

		// Cleanup
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_2117_GroupReceivedCalls() {
		// Test Case: 2117 - Call History: Received calls from same number will be grouped in recent summary list
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }

		// Place Calls to DUT
		for _ in 1...3 {
			con.dial(number: dutNum)
			ui.incomingCall(response: "Answer")
			sleep(3)
			ui.hangup()
		}

		// Verify Call History
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: oepName, num: oepNum)
		ui.verifyCallHistory(count: 1)

		// Cleanup
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_2126_5969_9352_CallHistoryEmptyString() {
		// Test Case: 2126 - Call History: Call History List will show "You don't have any calls" when no calls have been placed, received or missed
		// Test Case: 5969 - Call History: Empty call history shows text for no calls
		// Test Case: 9352 - Call History: text when recents list is clear
		
		// Verify No Recent Calls
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistoryEmpty()
		
		// Verify No Missed Calls
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistoryEmpty()
		ui.gotoHome()
	}
	
	func test_2347_2348_2349_CallHistoryLog() {
		// Test Case: 2347 - Call History: Missed calls appear in Call History
		// Test Case: 2348 - Call History: placed calls appear in Call History
		// Test Case: 2349 - Call History: Received calls appear in Call History
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2347", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		
		// Test 2347
		// Call DUT
		con.dial(number: dutNum)
		
		// Decline and wait for missed call
		ui.incomingCall(response: "Decline")
		ui.waitForMissedCallBadge(count: "1")
		
		// Verify Call History
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistory(name: "Automate 2347", num: oepNum)
		
		// Test 2348
		// Call OEP
		ui.call(phnum: oepNum)
		
		// Answer
		con.answer()
		sleep(3)
		
		// Hangup
		ui.hangup()
		
		// Verify Call History
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "Automate 2347", num: oepNum)
		
		// Test 2349
		// Call DUT
		con.dial(number: dutNum)
		
		// Answer
		ui.incomingCall(response: "Answer")
		sleep(3)
		
		// Hangup
		ui.hangup()
		
		// Verify Call History
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "Automate 2347", num: oepNum)
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2347")
		ui.gotoHome()
	}
	
	func test_2895_2896_CallHistoryAfterLogin() {
		// Test Case: 2895 - Call History: Logging out and logging back in to the same device does not create a duplicate for missed calls
		// Test Case: 2896 - Call History: Received calls are properly displayed when a user logs in
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Test 2896
		// Incoming Call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Test 2895
		// Missed Call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		
		// Logout
		ui.logout()
		
		// Login and Verify Missed Call
		ui.login(phnum: dutNum, password: dutPassword)
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistory(name: oepName, num: oepNum)
		
		// Verify Incoming Call
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: oepName, num: oepNum)
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}

	func test_2998_SelectCallHistoryItem() {
		// Test Case: 2998 - Call History: Select a call history line in portrait orientation does not crash the app
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2998", homeNumber: "18015757669", workNumber: "", mobileNumber: "")
		
		// Place Call
		ui.call(phnum: "18015757669")
		sleep(3)
		ui.hangup()
		
		// Select call history item
		ui.gotoCallHistory(list: "All")
		ui.selectContactFromHistory(contactName: "Automate 2998")
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.deleteAllContacts()
		ui.gotoHome()
	}

	func test_3006_5223_BlockFromHistory() {
		// Test Case: 3006 - Call History: User is not able to create multiple identical blocked number entries using the call history
		// Test Case: 5223 - Call History: Selecting view all from a blocked contact displays the call history
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Delete Blocked List
		ui.gotoBlockedList()
		ui.deleteBlockedList()

		// Outgouting Call
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		ui.hangup()

		// Incoming Call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		sleep(3)

		// Block Number from history - Missed
		ui.gotoCallHistory(list: "Missed")
		ui.blockNumberFromHistory(number: oepNum)

		// Get Blocked List
		ui.gotoBlockedList()
		let blockedList = ui.getBlockList()

		// Block Number from history - All
		ui.gotoCallHistory(list: "All")
		ui.blockNumberFromHistory(number: oepNum)

		// Test 3006
		// Verify Blocked List
		ui.gotoBlockedList()
		ui.verifyBlockedOrder(list: blockedList)
		
		// Test 5223
		ui.selectContact(name: oepName)
		ui.selectViewCallHistory()
		ui.verifyCallHistory(name: oepName, num: oepNum)
		
		// Cleanup
		ui.clearSearch()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoHome()
	}

	func test_3065_DeleteCallHistoryItem() {
		// Test Case: 3065 - Call History: A user can delete an entry in the call history
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }

		// Outgouting Call
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		ui.hangup()

		// Delete Call History Item
		ui.gotoCallHistory(list: "All")
		ui.deleteCallHistory(name: oepName, num: oepNum)
		ui.verifyNoCallHistory(num: oepNum)
		ui.gotoHome()
	}

	func test_3067_C35979_DeleteAllCallHistory() {
		// Test Case: 3067 - Call History: A user can delete all call history entries in the call history
		// Test Case: C35969 - Call History: DUT is able to launch application when there is no call history present.
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Call 1
		ui.call(phnum: oepNum)
		con.answer()
		sleep(3)
		ui.hangup()
		
		// Call 2
		ui.call(phnum: "18015553067")
		sleep(3)
		ui.hangup()
		
		// 3067 - Delete All Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.verifyCallHistory(count: 0)
		
		// C35969 - Verify DUT is able to background and launch with an empty call history
		ui.ntouchTerminate()
		ui.ntouchActivate()
		ui.gotoSettings()
		ui.gotoHome()
	}

	func test_3172_3903_7137_7729_MissedCallBadge() {
		// Test Case: 3172 - Call History: A badge appears on the missed call tab in the Call History
		// Test Case: 3903 - Call History: Only a single missed call is added
		// Test Case: 7137 - Call History: Missed calls from contacts with only a company name appear with a name on the missed call screen
		// Test Case: 7729 - Call History: Clearing Call history while the numerical notification badge is present, clears the badge
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Add Contact with Company Name
		ui.gotoSorensonContacts()
		ui.addContact(name: "", companyName: "AutoCO 7137", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		
		// Call DUT and hangup
		con.dial(number: dutNum)
		ui.waitForIncomingCall()
		con.hangup()
		
		// Test 3172, 3909
		// Verify Missed Call Badge
		ui.waitForMissedCallBadge(count: "1")
		
		// Test 7137
		// Verify Company Name
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "AutoCO 7137", num: oepNum)
		
		// Test 7729
		ui.deleteAllCallHistory()
		ui.verifyNoMissedCallBadge()
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "AutoCO 7137")
		ui.gotoHome()
	}
	
	func test_3964_SearchPunctuation() {
		// Test Case: 3964 - Call History: App searches for the call history of person's name that begins with a certain punctuation
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "???", homeNumber: "18015757669", workNumber: "", mobileNumber: "")

		// Select Contact and View History
		ui.gotoCallHistory(list: "All")
		ui.searchCallHistory(item: "???")
		ui.clearSearch()
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "???")
		ui.gotoHome()
	}
	
	func test_4512_CallHistorySorensonContacts() {
		// Test Case: 4512 - Call History: Placing a call to SVRS or CIR records the contact name in the call history
		
		// Call SVRS
		ui.gotoSorensonContacts()
		testUtils.calleeNum = svrsNum
		ui.callContact(name: "SVRS", type: "work")
		sleep(3)
		ui.hangup()
		
		// Call CIR
		ui.gotoSorensonContacts()
		testUtils.calleeNum = csNum
		ui.callContact(name: "Customer Care", type: "work")
		sleep(3)
		ui.hangup()
		
		// Verify Call History
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "SVRS", num: svrsNum)
		ui.verifyCallHistory(name: "Customer Care", num: csNum)
		
		// Cleanup
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_7404_CallHistorySearch() {
		// Test Case: 7404 - Call History: Able to search Call History
		
		// Call SVRS
		ui.call(phnum: svrsNum)
		sleep(3)
		ui.verifyCallerID(text: "SVRS")
		ui.hangup()
		
		// Call CIR
		ui.call(phnum: csNum)
		//ui.call(phnum: "18013868500")
		sleep(3)
		ui.verifyCallerID(text: "Customer Care")
		ui.hangup()
		
		// Search Call History for Customer
		ui.gotoCallHistory(list: "All")
		ui.searchCallHistory(item: "Customer")
		
		// Verify Call History
		ui.verifyCallHistory(name: "Customer Care", num: csNum)
		
		// Cleanup
		ui.clearSearch()
		ui.gotoHome()
	}

	// TODO: Server for CoreAccess currently not available
//	func test_7500_CallHistoryPhoto() {
//		// Test Case: 7500 - Call History: Filtering call history will display contact photo
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
////		core.addRemoteProfilePhoto(phoneNumber: oepNum)
//		core.setProfilePhotoPrivacy(phoneNumber: oepNum, value: "0")
//		ui.enableContactPhotos(setting: true)
//		
//		// Add Contact
//		ui.gotoSorensonContacts()
//		ui.addContact(name: "Automate 7500", homeNumber: oepNum, workNumber: "", mobileNumber: "")
//		
//		// Call 1
//		ui.call(phnum: oepNum)
//		con.answer()
//		sleep(3)
//		ui.hangup()
//		
//		// Call 2
//		ui.call(phnum: "18015757669")
//		sleep(3)
//		ui.hangup()
//		
//		// Filter Call History and Verify Photo
//		ui.gotoCallHistory(list: "All")
//		ui.searchCallHistory(item: "Automate 7500")
//		ui.verifyContactPhoto(name: "Automate 7500")
//		ui.clearSearch()
//		
//		// Cleanup
//		ui.gotoCallHistory(list: "All")
//		ui.deleteAllCallHistory()
//		ui.deleteAllContacts()
//		ui.gotoHome()
//	}

	func test_8395_ViewAllDeleteContactCallHistory() {
		// Test Case: 8395 - Call History: Select View All on a Contact and Deleting Call History Items
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 8395", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		
		// Call Contact
		testUtils.calleeNum = cfg["nvpNum"]!
		ui.callContact(name: "Automate 8395", type: "home")
		sleep(3)
		ui.hangup()
		
		// Select View all from Contact Edit screen
		ui.gotoCallHistory(list: "All")
		ui.searchCallHistory(item: "Automate 8395")
		
		// Delete Call History Item and Verify
		ui.deleteCallHistory(name: "Automate 8395", num: cfg["nvpNum"]!)
		ui.clearSearch()
		ui.gotoCallHistory(list: "All")
		ui.verifyNoCallHistory(num: cfg["nvpNum"]!)
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 8395")
		ui.gotoHome()
	}

	func test_8423_DuplicateEntriesDelete1() {
		// Test Case: 8423 - Call History: Deleting call history entries deletes all the entries in a row for the same phone number

		// Place 2 VRS Calls
		ui.call(phnum: "411")
		sleep(3)
		ui.hangup()

		// Call 2
		ui.redial()
		sleep(3)
		ui.hangup()
		
		// Delete 1 Call History item and verify
		ui.gotoCallHistory(list: "All")
		ui.deleteCallHistory(name: "Information", num: "411")
		ui.verifyNoCallHistory(name: "Information", num: "411")
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}

	func test_8502_VRSIndicator() {
		// Test Case: 8502 - Call History: Calls to VRS have "/VRS" in the call record
		
		// Add VRS Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 8502", homeNumber: "18015757669", workNumber: "", mobileNumber: "")
		
		// Place call to VRS Contact
		ui.call(phnum: "18015757669")
		sleep(3)
		ui.hangup()
		
		// Place call to VRS
		ui.call(phnum: "18015757889")
		sleep(3)
		ui.hangup()
		
		// Verify VRS Indicators
		ui.gotoCallHistory(list: "All")
		ui.verifyVRSIndicator(num: "18015757669")
		ui.verifyVRSIndicator(num: "18015757889")
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 8502")
		ui.gotoHome()
	}

	func test_8614_CallHistoryTabsInCall() {
		// Test Case: 8614 - Call History: Tabs for call history can be changed in call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Place call
		ui.call(phnum: oepNum)
		con.answer()
		
		// Select Add Call and verify Call History tabs
		ui.selectAddCall()
		ui.gotoCallHistoryInCall(list: "Missed")
		ui.gotoCallHistoryInCall(list: "All")
		ui.selectCancel()

		// Hangup
		ui.hangup()
		ui.gotoHome()
	}
	
	func test_C35970_CallHistoryAfterBackgrounding() {
		// Test Case: C35970 - Call History: After killing ntouch Missed/Outgoing/Incoming won't merge into the All category
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number, let oepName = endpoint.userName else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// OEP calls DUT for a missed call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Decline")
		
		// DUT calls out for an outbound call
		ui.call(phnum: svrsNum)
		sleep(3)
		ui.hangup()
		
		// Verify call history items prior to force kill app
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(count: 2)
		ui.verifyCallHistory(name: oepName, num: oepNum)
		ui.verifyCallHistory(name: "SVRS", num: svrsNum)
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistory(count: 1)
		ui.verifyCallHistory(name: oepName, num: oepNum)
		
		// Force kill and restart nTouch
		ui.ntouchTerminate()
		ui.ntouchActivate()
		
		// Verify Call history items after force kill app
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(count: 2)
		ui.verifyCallHistory(name: oepName, num: oepNum)
		ui.verifyCallHistory(name: "SVRS", num: svrsNum)
		ui.gotoCallHistory(list: "Missed")
		ui.verifyCallHistory(count: 1)
		ui.verifyCallHistory(num: oepNum)
		
		// Cleanup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
	}
}
