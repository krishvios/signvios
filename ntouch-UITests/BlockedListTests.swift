//
//  BlockedListTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 1/24/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import XCTest

class BlockedListTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let endpointsClient = TestEndpointsClient.shared
	let dutNum = UserConfig()["Phone"]!
	let dutName = UserConfig()["Name"]!
	let altNum = UserConfig()["blockedListAltNum"]!
	let altNum2 = UserConfig()["blockedListAltNum2"]!
	let defaultPassword = UserConfig()["Password"]!
	let vrsNum = "18015757669"
	
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
		// TODO: Server for CoreAccess currently not available
//		core.setHideMyCallerID(value: "0")
		ui.ntouchTerminate()
	}

	func test_2902_BlockedListInfo() {
		// Test Case 2902 - Contacts: Navigating away from the block list and back to it displays the block list rather than the contacts
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Add Contacts to Phonebook
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2902", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 2902-01", homeNumber: cfg["npcNum"]!, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 2902-02", homeNumber: cfg["macNum"]!, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 2902-03", homeNumber: cfg["andNum"]!, workNumber: "", mobileNumber: "")
		
		// Block Contacts in Phonebook
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 2902")
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 2902-01")
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 2902-02")
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 2902-03")
		ui.gotoSorensonContacts()
		
		// Verify Block List
		ui.gotoBlockedList()
		let blockContactsList = ui.getBlockList()
		ui.verifyBlockContactsOrder(list: blockContactsList)
		
		// Navigate away and back to the block list and verify block list
		ui.gotoSignMail()
		ui.gotoBlockedList()
		ui.verifyBlockContactsOrder(list: blockContactsList)
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoHome()
	}
	
	func test_2903_3001_ViewAllContactHistory() {
		// Test Case: 2903 - Contacts: The view all button takes the user to the history and displays all the call history entries for the contact in question
		// Test Case: 3001 - Contacts: Selecting view all, in a block list entry, displays all call history entries associated with that phone number
		
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// delete call history
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// add contact to phonebook
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2903", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		
		// make calls from oep to device
		for _ in 1...3
		{
			con.dial(number: dutNum)
			sleep(10)
			con.hangup()
		}
		ui.waitForMissedCallBadge(count: "3")
		
		// Test 2903
		// check contact call history
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 2903")
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "Automate 2903", num: oepNum)
		
		// Test 3001
		// Block contact and check history
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 2903")
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 2903")
		ui.selectContactCallHistory()
		ui.verifyCallHistory(name: "Automate 2903", num: oepNum)
		
		// delete contacts
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 2903")
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2903")
		
		// delete call history
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
	}
	
	func test_2955_BlockPartiallyBlockedContact() {
		// Test Case: 2955 - Blocking a contact with a number already blocked blocks the remaining unblocked numbers
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2955", homeNumber: "18016662955", workNumber: "", mobileNumber: "")
		
		// Block contact
		ui.blockContact(name: "Automate 2955")
		
		// Add numbers to contact
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 2955")
		ui.enterContact(number: "18016662966", type: "work")
		ui.enterContact(number: "18016662977", type: "mobile")
		ui.saveContact()
		
		// Block all numbers
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 2955")
		
		// Verify blocks
		ui.gotoBlockedList()
		ui.verifyBlockedNumbers(name: "Automate 2955", count: 3)
		
		// Delete Blocked list
		ui.deleteBlockedList()
		
		// Delete Contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2955")
	}
	
	func test_3000_EditBlockedNumber() {
		// Test Case: 3000 - Contacts: User is not able to edit a blocked number so as to produce multiple blocked number entries with the same phone number
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Add Contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 3000", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 3000-1", homeNumber: cfg["andNum"]!, workNumber: "", mobileNumber: "")
		
		// Block Contacts
		ui.blockContact(name: "Automate 3000")
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 3000-1")
		
		// Edit Blocked Entry to new Phone Number
		ui.gotoBlockedList()
		ui.editBlockedNumber(name: "Automate 3000-1", changeNum: cfg["nvpNum"]!)
		ui.verifyNumberAlreadyBlocked()
		ui.selectCancel()
		
		// Delete Blocked list
		ui.deleteBlockedList()
		
		// Delete Contacts
		ui.deleteAllContacts()
	}
	
	func test_3002_BlockedListOrder() {
		// Test Case 3002 - Contacts: Blocked list follows the same sorting algorithm as the phone book
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Create Contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate-A", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate-B", homeNumber: cfg["andNum"]!, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate-C", homeNumber: cfg["iosNum"]!, workNumber: "", mobileNumber: "")
		
		// Block Contacts
		ui.blockContact(name: "Automate-A")
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate-B")
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate-C")
		
		// Create blocked list Array and Verify Blocked List Order
		let blockedList = ["Automate-A", "Automate-B", "Automate-C"]
		ui.gotoBlockedList()
		ui.verifyBlockedOrder(list: blockedList)
		
		// Delete Blocked Numbers and Contacts
		ui.deleteBlockedList()
		ui.deleteAllContacts()
	}
	
	func test_3003_BlockedListAmount() {
		// Test Case 3003 - Contacts: Device allows more than 10 blocked phone numbers
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Create Array with Contact Names
		var contactList = [String]()
		contactList.append("Automate-A")
		contactList.append("Automate-B")
		contactList.append("Automate-C")
		contactList.append("Automate-D")
		contactList.append("Automate-E")
		contactList.append("Automate-F")
		contactList.append("Automate-G")
		contactList.append("Automate-H")
		contactList.append("Automate-I")
		contactList.append("Automate-J")
		contactList.append("Automate-K")
		
		// Set count to the array count
		let count = contactList.count
		
		// Add Contacts
		ui.gotoSorensonContacts()
		for i in 0 ..< count {
			ui.addContact(name: contactList[i], homeNumber: "1801555\(i)000", workNumber: "", mobileNumber: "")
		}
		
		// Block Contacts
		for i in 0 ..< count {
			ui.blockContact(name: contactList[i])
			ui.gotoSorensonContacts()
		}
		
		// Navigate to Blocked List and Verify Order
		ui.gotoBlockedList()
		ui.verifyBlockContactsOrder(list: contactList)
		
		// Delete Blocked Contacts and Contacts
		ui.deleteBlockedList()
		ui.deleteAllContacts()
	}
	
	func test_3004_3005_5346_blockFromHistory() {
		// Test Case: 3004 - Contacts: Block contact from call history
		// Test Case: 3005 - Contacts: blocking number from call history without a name does not create a nameless blocked item
		// Test Case: 5346 - Contacts: Verify blocking a contact from call history is confirming if the user would like to block the contact
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// delete call history
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// setup contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 3004", homeNumber: "", workNumber: "18015553004", mobileNumber: "")
		
		// generate call history entry
		testUtils.calleeNum = "18015553004"
		ui.callContact(name: "Automate 3004", type: "work")
		sleep(5)
		ui.hangup()
		ui.call(phnum: "18018093966")
		sleep(5)
		ui.hangup()
		
		// block contact
		ui.gotoCallHistory(list: "All")
		ui.blockContactFromHistory(name:"Automate 3004")
		ui.gotoCallHistory(list: "All")
		ui.blockNumberFromHistory(number: "18018093966")
		ui.gotoBlockedList()
		ui.verifyBlockedNumber(name: "Automate 3004")
		ui.verifyBlockedNumber(name: "Unknown Caller")
		
		// cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.deleteAllContacts()
	}
	
	func test_5334_EditBlockedDescription() {
		// Test Case: 5334 - Contacts: Verify the user is able to re-edit a blocked number immediately, without backing up, then returning to the same blocked number
		// Go to blocked list
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Auto 5334", num: "12223334444")
		
		// Edit Blocked Description
		ui.gotoBlockedList()
		ui.editBlockedDescription(name: "Auto 5334", newName: "Automation 5334")
		
		// Edit Blocked Description
		ui.editBlockedDescription(name: "Automation 5334", newName: "Auto 5334")
		
		// Delete Blocked Number
		ui.deleteBlockedNumber(name: "Auto 5334")
		ui.gotoHome()
	}
	
	func test_6567_6578_BlockCallerID() {
		// Test Case: 6567 - BCI: Callers info is replaced with No Caller ID
		// Test Case: 6578 - BCI: The User is Notified when they have BCI enabled
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
		
		// Hide My Caller ID
		con.settingSet(type: "Number", name: "BlockCallerId", value: "1")
		
		// Test 6567
		// Call DUT
		con.dial(number: dutNum)
		
		// Verify Call from "No Caller ID" and Answer
		ui.waitForIncomingCall(callerID: "No Caller ID")
		ui.incomingCall(response: "Answer")
		
		// Test 6578
		// Verify Call Connected to "No Caller ID"
		ui.verifyCallerID(text: "No Caller ID")
		ui.hangup()
		
		// Cleanup
		con.settingSet(type: "Number", name: "BlockCallerId", value: "0")
		ui.gotoHome()
	}
	
	func test_7095_BlockNumbersMax() {
		// Test Case: 7095 - Contacts: Users can have only max numbers blocked
		//Log into account with max blocked numbers
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		//go to blocked and try to add another
		ui.gotoBlockedList()
		ui.blockNumber(name: "Test 7095", num: vrsNum)
		
		//verify block list full message triggers
		ui.verifyUnableToBlock()
		
		//cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_7275_DeleteBlockedNumber() {
		// Test Case: 7275 - Contacts: Remove blocked number from blocked list
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7275", homeNumber: "18015757669", workNumber: "", mobileNumber: "")
		
		// Block Contact
		ui.blockContact(name: "Automate 7275")
		
		// Delete Blocked Number
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 7275")
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7275")
		ui.gotoHome()
	}
	
	func test_7414_BlockContact() {
		// Test Case: 7414 - Phonebook: Block a contact
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7414", homeNumber: "18015757669", workNumber: "", mobileNumber: "")
		
		// Block Contact
		ui.blockContact(name: "Automate 7414")
		
		// Verify Blocked
		ui.gotoBlockedList()
		ui.verifyBlockedNumber(name: "Automate 7414")
		
		// Delete Blocked
		ui.deleteBlockedNumber(name: "Automate 7414")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7414")
		ui.gotoHome()
	}
	
	func test_7415_BlockNumber() {
		// Test Case: 7415 - Phonebook: Block a number
		// Block Number
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 7415", num: "15553334444")
		
		// Verify Blocked
		ui.gotoBlockedList()
		ui.verifyBlockedNumber(name: "Automate 7415")
		
		// Delete Blocked
		ui.deleteBlockedNumber(name: "Automate 7415")
		ui.gotoHome()
	}
	
	func test_7436_BlockViewAllEditContact() {
		// Test Case: 7436 - Contacts: Selecting View all on a blocked contact, then editing a contact and selecting back will not cause the app to navigate away from Phonebook
		
		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// Add Contact and block
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7436", homeNumber: "18015757669", workNumber: "", mobileNumber: "")
		ui.blockContact(name: "Automate 7436")
		
		// Select Blocked Contact and View All
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 7436")
		ui.selectViewCallHistory()
		
		// Edit Contact and Cancel
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 7436")
		ui.selectCancel()
		
		// Verify Contacts Page
		ui.verifyTabSelected(tab: "Contacts")
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7436")
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 7436")
		ui.gotoCallHistory(list: "All")
		ui.clearSearch()
		ui.gotoHome()
	}
	
	func test_7765_CallAfterCallingBlockingEndpoint() {
		// Test Case: 7765 - Contacts - BCI: User is able to call to an endpoint that has the DUT blocked then place a call to another endpoint
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Logout and login to account that will block DUTs number
		ui.switchAccounts(number: altNum2, password: defaultPassword)

		// Block DUT's number
		ui.gotoBlockedList()
		ui.blockNumber(name: dutName, num: dutNum)

		// Logout and back into DUTs account
		ui.switchAccounts(number: dutNum, password: defaultPassword)

		// Place call to blocking endpoint
		ui.call(phnum: altNum2)
		sleep(2)
		ui.clickButton(name: "Cancel")

		// Assure DUT can call another endpoint
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		sleep(3)
		con.hangup()

		// Clean up
		ui.switchAccounts(number: altNum2, password: defaultPassword)
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_8475_Block10DigitNum() {
		// Test Case: 8475 - Contacts - BCI: Completing a 10 digit number
		// Block 10 digit number
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8475", num: "8015558475")
		
		// Verify Blocked Number
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 8475")
		ui.verifyBlockedNumber(num: "18015558475")
		
		// Cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 8475")
		ui.gotoHome()
	}
	
	func test_8476_Block11DigitNum() {
		// Test Case: 8476 - Contacts - BCI: Not completing 11 digit number
		// Block 11 digit number
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8476", num: "18015558476")
		
		// Verify Blocked Number
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 8476")
		ui.verifyBlockedNumber(num: "18015558476")
		
		// Cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 8476")
		ui.gotoHome()
	}
	
	func test_8477_BlockWithoutAreaCode() {
		// Test Case: 8477 - Contacts - BCI: Completing with an area code
		// Get Area Code
		let phnum = dutNum
		let index1 = phnum.index(phnum.startIndex, offsetBy: 1)
		let index3 = phnum.index(phnum.startIndex, offsetBy: 3)
		let areaCode = phnum[index1...index3]
		
		// Block number without area code
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8477", num: "5558477")
		
		// Add Area Code to Blocked Num
		let blockedNum = String("1" + areaCode + "5558477")
		
		// Verify Area Code added
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 8477")
		ui.verifyBlockedNumber(num:blockedNum)
		
		// Cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 8477")
		ui.gotoHome()
	}
	
	func test_8479_8480_CantBlockEmptyNumberOrDescription() {
		// Test Case: 8479 - Contacts - BCI: Adding from block add/edit screen blank phone number
		// Test Case: 8480 - Contacts - BCI: Adding from block add/edit screen blank description
		//go to block list
		ui.gotoBlockedList()
		
		// attempt to block empty string
		ui.blockNumber(name: "Automate 8479", num: "")
		ui.verifyUnableToBlockNoNumber()
		
		// attempt to block empty description
		ui.gotoBlockedList()
		ui.blockNumber(name: "", num: "18015558480")
		ui.verifyUnableToBlockNoNumber()
		ui.selectCancel()
		
		// Cleanup
		ui.gotoHome()
	}
	
	func test_8481_8630_BlockDescriptionNumCap() {
		// Test Case: 8481 - Contacts - BCI: Adding from block add/edit screen description length is capped at 64 characters
		// Test Case: 8630 - Contacts - BCI: Adding a Phone Number from block add/edit screen length
		// go to block list
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		
		// attempt to block large description and num
		ui.blockNumber(name: "Automate 8481 123456789012345678901234567890123456789012345678901234567890", num: "1234567890123456789012345678901234567890123456789018015558481")
		
		// Test 8481
		// verify blocked number (with fewer trailing digits)
		ui.gotoBlockedList()
		ui.verifyBlockedNumber(name: "Automate 8481 12345678901234567890123456789012345678901234567890")
		
		// Test 8360
		// verify blocked string
		ui.selectContact(name: "Automate 8481 12345678901234567890123456789012345678901234567890")
		ui.verifyBlockedString(string: "12345678901234567890123456789012345678901234567890")
		
		// cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedList()
	}
	
	func test_8482_BlockedNumCharacters() {
		// Test Case: 8482 - Contacts - BCI: Adding from block add/edit screen digits only
		// Navigate to Blocked List and Add Blocked Item
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8482", num: "+1 801.456.4345")
		
		// Select Blocked Item and Verify Number
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 8482")
		ui.verifyBlockedString(string: "1 (801) 456-4345")
		
		// Delete Blocked Item
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 8482")
		ui.gotoHome()
	}
	
	func test_8483_BlockSipUri() {
		// Test Case: 8483 - Contacts - BCI: Adding from block add/edit screen alpha numeric (sip URI)
		// Block SIP URI
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8483", num: "sip:18015558483@sip.com")
		
		// Verify Blocked Number
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 8483")
		ui.verifyBlockedString(string: "sip:18015558483@sip.com")
		
		// Cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 8483")
		ui.gotoHome()
	}
	
	func test_8484_BlockDomain() {
		// Test Case: 8484 - Contacts - BCI: Adding from block add/edit screen alpha numeric (domain)
		// Block Domain
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8484", num: "sip.com")
		
		// Verify Blocked Number
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 8484")
		ui.verifyBlockedString(string: "sip.com")
		
		// Cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 8484")
		ui.gotoHome()
	}
	
	func test_8485_BlockText() {
		// Test Case: 8485 - Contacts - BCI: Adding from block add/edit screen alpha numeric (text)
		// Block Text
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8485", num: "test")
		
		// Verify Blocked Number
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 8485")
		ui.verifyBlockedString(string: "test")
		
		// Cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 8485")
		ui.gotoHome()
	}
	
	func test_8486_BlockIpAddress() {
		// Test Case: 8486 - Contacts - BCI: Adding from block add/edit screen ip address
		// Block IP Address
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.blockNumber(name: "Automate 8485", num: "192.168.254.254")
		
		// Verify Blocked Number
		ui.gotoBlockedList()
		ui.selectContact(name: "Automate 8485")
		ui.verifyBlockedString(string: "192.168.254.254")
		
		// Cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 8485")
		ui.gotoHome()
	}
	
	func test_8489_BlockFromHistoryPreservesInfo() {
		// Test Case: 8489 - Contacts - BCI: Adding from Call History no changes
		// setup
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
		ui.call(phnum: "18015128489")
		sleep(5)
		ui.hangup()
		
		// block number from history
		ui.gotoCallHistory(list: "All")
		ui.blockNumberFromHistory(number: "18015128489")
		
		// verify block info
		ui.gotoBlockedList()
		ui.verifyBlockedNumber(name: "Unknown Caller")
		ui.selectContact(name: "Unknown Caller")
		ui.verifyBlockedNumber(num: "18015128489")
		
		// cleanup
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
	}
}
