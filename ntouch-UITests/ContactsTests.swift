//
//  ContactsTests.swift
//  ntouch
//
//  Created by Mikael Leppanen on 8/10/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import XCTest

class ContactsTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let dutName = UserConfig()["Name"]!
	let altNum = UserConfig()["contactsAltNum"]!
	let endpointsClient = TestEndpointsClient.shared
	let vrsNum1 = "18015757669"
	let vrsNum2 = "18015757889"
	let vrsNum3 = "18015757999"
	let vrsNum4 = "18015757559"
	let vrsNum5 = "18015757779"
	let vrsNum6 = "18015757449"
	let contacts500  = "18018679000"
	let redirectedNum = "19638010007"
	let cirNum = "611"
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: UserConfig()["Orientation"]!)
		continueAfterFailure = false
		ui.ntouchActivate()
		ui.clearAlert()
		ui.waitForMyNumber()
		ui.deleteAllContacts()
	}
	
	override func tearDown() {
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}
	
	func test_0477_CallVRSContact() {
		// Test Case: 477 - Contacts: Dial a Contact in ntouch that is not in Core

		// Add VRS Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 477", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")

		// Call VRS Contact
		testUtils.calleeNum = vrsNum1
		ui.callContact(name: "Automate 477", type: "home")
		sleep(3)
		ui.hangup()

		// Cleanup
		ui.deleteAllContacts()
	}
	
	func test_0560_ReceiveSignMailAddContact() {
		// Test Case: 560 - Contacts: Adding a contact updates the Video Inbox
		
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Call DUT, skip greeting and send signmail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")
		
		// Login to DUT account to verify message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Wait for SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 560", homeNumber: altNum, workNumber: "", mobileNumber: "")
		
		// Go to SignMail and verify contact name
		ui.gotoSignMail()
		ui.verifyContact(name: "Automate 560")
		
		// Cleanup
		ui.deleteAllSignMail()
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 560")
		ui.gotoHome()
	}
	
	func test_0600_IncomingCallerID() {
		// Test Case: 600 - Contacts: Contact name appears on incoming call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 600", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		
		// Call DUT and verify Caller ID
		con.dial(number: dutNum)
		ui.waitForIncomingCall(callerID: "Automate 600")
		
		// Answer
		ui.incomingCall(response: "Answer")
		sleep(3)
		ui.hangup()
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 600")
		ui.gotoHome()
	}
	
	func test_0603_IncomingContactInfoDisplayed() {
		// Test Case: 0603 - Contacts: Add contact to list after they call, correct info displays when they call back
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
		
		// Place Call from nVP and Answer Incoming Call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		sleep(5)
		ui.hangup()
		
		// Navigate to Call History and Add Contact
		ui.gotoCallHistory(list: "All")
		ui.addContactFromHistory(num: oepNum, contactName: "Automate 0603")
		
		// Place Call from nVP and verify Caller ID
		con.dial(number: dutNum)
		ui.waitForIncomingCall(callerID: "Automate 0603")
		con.hangup()
		
		// Clear Call History and Delete Contact
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 0603")
		
		// Close connection to OEP
		ui.gotoHome()
	}
	
	func test_0604_contactOneNumber() {
		// Test Case: 0604 - Contacts: Contact with 1 Number
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 0604", homeNumber: oepNum, workNumber: "", mobileNumber: "")
		
		// Call Contact and Verify Caller ID
		testUtils.calleeNum = oepNum
		ui.callContact(name: "Automate 0604", type: "home")
		con.answer()
		ui.verifyCallerID(text: "Automate 0604")
		ui.hangup()
		
		// Call from Contact and Verify Caller ID
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		ui.verifyCallerID(text: "Automate 0604")
		ui.hangup()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 0604")
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_0846_2934_ContactFromHistory() {
//		// Test Case: 846 - Contacts: Add contact from call history number auto-populated
//		// Test Case: 2934 - Contacts: Adding a contact from the call history is reflected in the UI
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
////		core.addRemoteProfilePhoto(phoneNumber: oepNum)
//		core.setProfilePhotoPrivacy(phoneNumber: oepNum, value: "0")
//
//		// Clean up call history
//		ui.gotoCallHistory(list: "All")
//		ui.deleteAllCallHistory()
//
//		// Produce a call history entry in DUT
//		con.dial(number: dutNum)
//		ui.incomingCall(response: "Answer")
//		sleep(3)
//		ui.hangup()
//
//		// Test 846
//		// Select the call history entry and add the contact
//		ui.gotoCallHistory(list: "All")
//		ui.addContactFromHistory(num: oepNum, contactName: "Automate 0846")
//		
//		// Verify number auto-population
//		ui.gotoSorensonContacts()
//		ui.selectContact(name: "Automate 0846")
//		ui.verifyContact(phnum: oepNum, type: "home")
//
//		// Test 2934
//		ui.gotoCallHistory(list: "All")
//		ui.verifyCallHistory(name: "Automate 0846", num: oepNum)
//		ui.verifyContactPhoto(name: "Automate 0846")
//		
//		// Cleanup
//		ui.gotoCallHistory(list: "All")
//		ui.deleteAllCallHistory()
//		ui.deleteAllContacts()
//		ui.gotoHome()
//	}
	
	func test_0857_ContactAreaCodes() {
		// Test Case: 857 Contacts: Contact list recognizes area codes
		// Verify that the DUT will correctly differentiate contacts with the same number except the area code
		
		// Create multiple contacts to a number of remote endpoints with different area codes but same number: 15323330000 16323330000		17323330000		18323330000		19323330000
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 0857-0", homeNumber: "15323330000", workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 0857-1", homeNumber: "16323330000", workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 0857-2", homeNumber: "17323330000", workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 0857-3", homeNumber: "18323330000", workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 0857-4", homeNumber: "19323330000", workNumber: "", mobileNumber: "")
		
		// Call a new contact and verify the user is connected to the hold server
		ui.gotoSorensonContacts()
		testUtils.calleeNum = "17323330000"
		ui.callContact(name: "Automate 0857-2", type: "home")
		sleep(2)

		// The contacts are correctly resolved when only the area code differs
		ui.verifyCallerID(text: "Automate 0857-2")
		ui.hangup()
		
		// Call a number
		ui.gotoHome()
		ui.call(phnum: "16323330000")
		sleep(2)
		
		// The contacts are correctly resolved when only the area code differs
		ui.verifyCallerID(text: "Automate 0857-1")
		
		// Cleanup
		ui.hangup()
		ui.deleteAllContacts()
	}
	
	func test_0880_OutgoingContactInfoDisplayed() {
		// Test Case: 0880 - Contacts: Add contact to list after they call, correct info displays when they call back
		
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
		
		// Place Call
		ui.call(phnum: vrsNum1)
		ui.waitForCallOptions()
		sleep(5)
		ui.hangup()
		
		// Navigate to Call History and Add Contact
		ui.gotoCallHistory(list: "All")
		ui.addContactFromHistory(num: vrsNum1, contactName: "Automate 0880")
		
		// Call contact and verify Caller ID
		ui.call(phnum: vrsNum1)
		ui.waitForCallOptions()
		ui.verifyCallerID(text: "Automate 0880")
		ui.hangup()
		
		// Clear Call History and Delete Contact
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoHome()
		ui.deleteAllContacts()
	}
	
	func test_1325_1522_1525_ImportContacts() {
		// Test Case: 1325 - Contacts: Able to import contacts using VP 10 digit number
		// Test Case: 1522 - Contacts: Import Contacts from Existing VP Account
		// Test Case: 1525 - Contacts: Importing contacts imports the correct number of contacts
		
		// Import VP Contacts
		ui.importVPContacts(phnum: "7323005678", pw: "1234", expectedNumber: "2")
		
		// Verify Contacts
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Atest")
		ui.verifyContact(name: "Atest2")
		
		// Delete Contacts
		ui.gotoHome()
		ui.deleteAllContacts()
	}
	
	func test_1527_4478_7420_AddContactFromSignMail() {
		// Test Case: 1527 - Contacts: Adding new contacts from SignMail
		// Test Case: 4478 - Contacts: Verify the phone number is populated when adding a contact from the signmail page
		// Test Case: 7420 - Contacts: Saving a new Sorenson Contact from a SignMail to DUT label should read "Add Contact"
		
		// Delete all SignMail
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		
		// Login to alt account to send message
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Call DUT, skip greeting and send signmail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.leaveSignMailRecording(time: 10, response: "Send")
		
		// Login to DUT account to verify message
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		
		// Wait for SignMail
		ui.gotoSignMail()
		ui.waitForSignMail(phnum: altNum)

		// Add Contact From SignMail
		ui.gotoSignMail()
		ui.addContactFromSignMail(phnum: altNum, contactName: "Automate 1527")
		
		// verify Contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Automate 1527")
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 1527")
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSignMail()
		ui.deleteAllSignMail()
		ui.gotoHome()
	}

	func test_1609_ContactRedirectNotUpdated() {
		// Test Case: 1609 - Contacts: Not updated with redirected number
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 1609", homeNumber: redirectedNum, workNumber: "", mobileNumber: "")
		
		// Call Contact and verify Number redirected
		testUtils.calleeNum = redirectedNum
		ui.callContact(name: "Automate 1609", type: "home")
		ui.redirectedMessage(response: "Cancel")
		
		// Verify Contact Number
		ui.verifyContact(phnum: redirectedNum, type: "home")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 1609")
		ui.gotoHome()
	}
	
	func test_1996_2512_AddNumToContactFromDialer() {
		// Test Case: 1996 - Contacts: Add to existing contact from dialer will save multiple phone numbers
		// Test Case: 2512 - Contacts: Add to Existing Contact dialog box should have the contact name

		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 1996", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")

		// Add work number to contact from dialer
		ui.gotoHome()
		ui.addNumToContactFromDialer(phnum: vrsNum2, name: "Automate 1996", type: "work")

		// Add mobile number to contact
		ui.enterContact(number: vrsNum3, type: "mobile")
		ui.saveContact()

		// Verify Contact Numbers
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 1996")
		ui.verifyContact(phnum: vrsNum1, type: "home")
		ui.verifyContact(phnum: vrsNum2, type: "work")
		ui.verifyContact(phnum: vrsNum3, type: "mobile")

		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 1996")
		ui.gotoHome()
		ui.clearDialer()
	}

	func test_2119_2120_DefaultContactsAdded() {
		// Test 2119 - Contacts: Sorenson VRS added on clean install
		// Test 2120 - Contacts: Customer Care Contact added on clean install
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "SVRS")
		ui.verifyContact(name: "Customer Care")
	}
	
	func test_2487_AddInvalidNumberToContact() {
		// Test Case: 2487 - Contacts: Attempting to add a phone number of greater than allowed length will not create a contact without a phone number

		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2487", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")

		// Add Invalid Number from dialer (33 char)
		ui.gotoHome()
		ui.replaceContactNumFromDialer(name: "Automate 2487", type: "home", oldNum: vrsNum1, newNum: "132456789012345678901234567890123")

		// Verify Done button disabled
		ui.verifyDoneButtonDisabled()
		ui.selectCancel()

		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2487")
		ui.gotoHome()
		ui.clearDialer()
	}
	
	func test_2503_2893_2904_4227_7252_ImportMaxContactNumberDisplay() {
		// Test Case: 2503 - Contacts: Import contacts will display the number of contacts imported
		// Test Case: 2893 - Contacts: Adding additional contacts while at the contact limit displays appropriate error
		// Test Case: 2904 - Contacts: Importing contacts that would bring phone above limit stops at limit
		// Test Case: 4227 - Contacts: DUT with 500 contacts hangs up before call is answered and returns to home screen
		// Test Case: 7252 - Contacts: Adding a contact from the home screen when at cap will inform the user
		
		// create a couple dummy contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2503 a", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 2503 b", homeNumber: vrsNum2, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 2503 c", homeNumber: vrsNum3, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 2503 d", homeNumber: vrsNum4, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 2503 e", homeNumber: vrsNum5, workNumber: "", mobileNumber: "")
		
		// Test 2503
		// import contacts from 500 account, get 495
		ui.importVPContacts(phnum: contacts500, pw: "1234", expectedNumber: "495")
		
		// attempt to add additional contact manually
		ui.gotoSorensonContacts()
		
		// Test 2893 2904
		// verify add contact fails
		ui.selectAddContact()
		ui.verifyCannotAddContact()
		
		//verify number of contacts
		ui.verifyContactCount(count: 502)
		
		// Test 7252
		ui.gotoHome()
		ui.clearDialer()
		ui.dial(phnum: vrsNum6)
		ui.selectAddNewContactFromDialer()
		ui.verifyCannotAddContact()
		
		// Test 4227
		// Call and hangup
		ui.gotoHome()
		ui.call(phnum: vrsNum1)
		sleep(3)
		ui.hangup()
		
		// cleanup all contacts
		ui.deleteAllContacts()
	}
	
	func test_2504_ContactNoNameAlert() {
		// Test 2504 - Contacts: Create Sorenson only contact without a name
		
		// Add a Contact with no Name and Select ok on Alert
		ui.gotoSorensonContacts()
		ui.addContact(name: "", homeNumber: "", workNumber: cfg["nvpNum"]!, mobileNumber: "")
		ui.selectCancel()
		ui.gotoHome()
	}
	
	func test_2505_NameDisplaysOnDialer() {
		// Test 2505 - Contacts: Sorneson Only contacts display name on dialer screen
		
		//Create a contact to dial
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2505", homeNumber: "", workNumber: "", mobileNumber: vrsNum1)
		
		//Go to home screen and dial contact number
		ui.gotoHome()
		ui.clearDialer()
		ui.dial(phnum: vrsNum1)
		ui.verifyDialerContact(name: "Automate 2505")
		
		//cleanup
		ui.clearDialer()
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2505")
	}

	func test_2891_ContactHeaders() {
		//Test 2891 - Contacts: The home, work, and mobile number field for each contact will have an appropriate headers associated with those fields
		ui.gotoPhonebook()
		ui.gotoSorensonContacts()
		ui.verifyContactHeaders()
		ui.gotoHome()
	}
	
	func test_2894_ContactsOrder() {
		// Test 2894 - Contacts: Phonebook is sorted the same as nVP
		
		// Add Contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "T1ster", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "TEster", homeNumber: vrsNum2, workNumber: "", mobileNumber: "")
		ui.addContact(name: "T ster", homeNumber: vrsNum3, workNumber: "", mobileNumber: "")
		ui.addContact(name: "T&ster", homeNumber: vrsNum4, workNumber: "", mobileNumber: "")
		ui.addContact(name: "T@ster", homeNumber: vrsNum5, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Tester", homeNumber: vrsNum6, workNumber: "", mobileNumber: "")
		
		// Verify Contact Order
		ui.verifyContactPosition(name: "T ster", position: 2)
		ui.verifyContactPosition(name: "T@ster", position: 3)
		ui.verifyContactPosition(name: "T&ster", position: 4)
		ui.verifyContactPosition(name: "T1ster", position: 5)
		ui.verifyContactPosition(name: "Tester", position: 6)
		ui.verifyContactPosition(name: "TEster", position: 7)
		
		// Delete Contacts
		ui.gotoHome()
		ui.deleteAllContacts()
	}
	
	func test_2898_SorensonContactDelete() {
		// Test 2898 - Contacts: Sorenson contacts (CIR and VRS) are not able to be deleted
		ui.gotoSorensonContacts()
		
		// Select Customer Care and select alert after attempting to delete
		ui.deleteSorensonContact(name: "Customer Care")
		ui.verifyContact(name: "Customer Care")
		
		// Select SVRS and select ok on alert after attempting to delete
		ui.deleteSorensonContact(name: "SVRS")
		ui.verifyContact(name: "SVRS")
		ui.gotoHome()
	}

	func test_2900_DeletingSameNumberContacts() {
		// Test case 2900 - Contacts: Deleting a contact with the same phone number as another contact always deletes the one selected

		// add both contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "Hello World", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Goodbye World", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")

		// delete only one contact to and confirm that the other is still in the list
		ui.deleteContact(name: "Hello World")
		ui.verifyContact(name: "Goodbye World")

		// clean up and delete contacts
		ui.gotoHome()
		ui.deleteAllContacts()
	}

	func test_2901_4437_ContactAddNumber() {
		// Test case 2901 - Contacts: Additional phone numbers can be added to an existing contact
		// Test case 4437 - Contacts: "Contacts counter" displays the number of Contacts not the number of phone numbers
		
		// add contact to phonebook
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2901", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Test 2901
		// add number to existing contact
		ui.editContact(name: "Automate 2901")
		ui.enterContact(number: vrsNum2, type: "work")
		ui.saveContact()
		
		// Test 4437
		// Verify Contact Count
		ui.gotoSorensonContacts()
		ui.verifyContactCount(count: 3)
		
		// verify new number is added to contact
		ui.selectContact(name: "Automate 2901")
		ui.verifyContact(phnum: vrsNum2, type: "work")
		
		// delete contacts from contact list to clean up
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2901")
		ui.gotoHome()
	}

	func test_2966_EditContactSameNumber() {
		
		// Add Contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2966", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Name Change", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		
		// Edit Contact and Verify Correct Name
		ui.editContact(name: "Automate 2966")
		ui.verifyEditContact(name: "Automate 2966")
		ui.selectCancel()
		ui.gotoSorensonContacts()
		
		// Edit Contact and Verify Correct Name
		ui.editContact(name: "Name Change")
		ui.verifyEditContact(name: "Name Change")
		ui.selectCancel()
		ui.gotoSorensonContacts()
		
		// Delete Both Contacts
		ui.deleteContact(name: "Automate 2966")
		ui.deleteContact(name: "Name Change")
		ui.gotoHome()
	}

	func test_2974_ContactNameEntry() {
		// Test Case: 2974 - Contacts: Contact name may not contain returns
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContactInfo(name: "Automate 2974", companyName: "", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Enter more into the Contact name and save
		ui.enterContact(name: "Test")
		ui.saveContact()
		
		// Verify Contact and Delete
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Automate 2974Test")
		ui.deleteContact(name: "Automate 2974Test")
		ui.gotoHome()
	}
	
	func test_3906_RemoveHomeNum() {
		// Test Case: 3906 - Contacts: Removing home number from contact gives error
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 3906", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Remove Home number
		ui.editContact(name: "Automate 3906")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: vrsNum2, type: "mobile")
		ui.saveContact()
		
		// Verify Changes
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 3906")
		ui.verifyContact(phnum: vrsNum2, type: "mobile")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 3906")
		ui.gotoHome()
	}

	func test_4047_DeletedNotShown() {
		// test 4047: Contacts: Deleted names are not shown in contact list
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 4047", homeNumber: "", workNumber: "", mobileNumber: vrsNum1)
		ui.gotoHome()
		
		// Go to contacts
		ui.gotoSorensonContacts()
		
		// delete contact
		ui.deleteContact(name: "Automate 4047")
		
		// verify name removed from searched list
		ui.verifyContactCount(name: "Automate 4047", count: 0)
	}

	func test_4074_CallSorensonContact() {
		// test 4074: Contacts: selecting a sorenson contact on the contacts tab places a call to the selected contact
		// go to contacts
		ui.gotoSorensonContacts()
		
		// place call to CIR
		testUtils.calleeNum = cirNum
		ui.callContact(name: "Customer Care", type: "work")
		
		// verify call
		ui.verifyCallerID(text: "Customer Care")
		// cleanup
		ui.hangup()
		ui.gotoHome()
	}

	func test_4076_4347_EditDoesNotDuplicate() {
		// Test Case: 4076 - Contacts: contacts are not diplicated after editing
		// Test Case: 4347 - Contacts: When the HOME field is left blank when creating a new contact the app will function normal
		
		// Test 4347
		// Create a contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 4076", homeNumber: "", workNumber: vrsNum1, mobileNumber: vrsNum2)
		ui.gotoHome()
		
		// go to contacts
		ui.gotoSorensonContacts()
		
		// edit contact
		ui.editContact(name: "Automate 4076")
		ui.removeContactNumber(type: "mobile")
		ui.saveContact()
		
		// Test 4076
		// verify no duplication
		ui.gotoSorensonContacts()
		ui.verifyContactCount(name: "Automate 4076", count: 1)
		
		// cleanup
		ui.deleteContact(name: "Automate 4076")
	}
	
	func test_4343_7524_SearchContacts() {
		// Test Case: 4343 - Contacts: When you select the search function in contacts the keyboard will appear
		// Test Case: 7524 - Contacts: The search button locates contacts in the contact list
		
		// Add Contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 4343", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 7524", homeNumber: vrsNum2, workNumber: "", mobileNumber: "")
		ui.verifyContactCount(count: 4)
		
		// Test 4343 7524
		// Search Contact
		ui.searchContactList(name: "Automate 7524")
		ui.verifyContact(name: "Automate 7524")
		
		// Delete Contacts
		ui.selectCancel()
		ui.deleteContact(name: "Automate 4343")
		ui.deleteContact(name: "Automate 7524")
		ui.gotoHome()
	}
	
	func test_4360_AddContactTitle() {
		// Test Case: 4360 - Contacts: Selecting add contact displays the title "add contact"
		
		// Select Add Contact Button
		ui.gotoSorensonContacts()
		ui.selectAddContact()

		// Verify title
		ui.verifyContactView()

		// go to home
		ui.selectCancel()
		ui.gotoHome()
	}
	
	func test_4386_DisplayAllContactHistory() {
		// Test Case 4386 - Contacts: "View all" displays all phone numbers in the contact not just the primary number
		// Delete Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()

		// Test 4386
		// Add Contact with 3 numbers
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 4386", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)

		// Place call to each number
		testUtils.calleeNum = vrsNum1
		ui.callContact(name: "Automate 4386", type: "home")
		sleep(3)
		ui.hangup()
		ui.gotoSorensonContacts()
		testUtils.calleeNum = vrsNum2
		ui.callContact(name: "Automate 4386", type: "work")
		sleep(3)
		ui.hangup()
		ui.gotoSorensonContacts()
		testUtils.calleeNum = vrsNum3
		ui.callContact(name: "Automate 4386", type: "mobile")
		sleep(3)
		ui.hangup()

		// Search Contact and Verify correct number of calls
		ui.gotoCallHistory(list: "All")
		ui.searchCallHistory(item: "Automate 4386")
		ui.verifyCallHistory(count: 3)
		ui.verifyCallHistory(name: "Automate 4386", num: vrsNum1)
		ui.verifyCallHistory(name: "Automate 4386", num: vrsNum2)
		ui.verifyCallHistory(name: "Automate 4386", num: vrsNum3)
		ui.clearSearch()

		// Delete call history and contact
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 4386")
		ui.gotoHome()
	}
	
	func test_4565_AddContactSame3Num() {
		// Test Case: 4565 - Contacts: Adding a contact with the same number for home, work, and mobile won't display the number in one field
		
		// Add Contact with 3 matching numbers
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 4565", homeNumber: vrsNum1, workNumber: vrsNum1, mobileNumber: vrsNum1)
		
		// Verify numbers
		ui.selectContact(name: "Automate 4565")
		ui.verifyContact(phnum: vrsNum1, type: "home")
		ui.verifyContact(phnum: vrsNum1, type: "work")
		ui.verifyContact(phnum: vrsNum1, type: "mobile")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 4565")
		ui.gotoHome()
	}
	
	func test_5247_EditContactRemoveName() {
		// Test Case: 5247 - Contacts: Editing a contact and deleting the name then selecting cancel does not save the contact without a name
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5247", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Edit Contact
		ui.editContact(name: "Automate 5247")
		ui.removeContactName(name: "Automate 5247")
		ui.selectCancel()
		
		// Verify Contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Automate 5247")
		
		// Delete Contact
		ui.deleteContact(name: "Automate 5247")
		ui.gotoHome()
	}
	
	func test_5986_7227_7324_7472_AddContactFromDialer() {
		// Test Case: 5986 - Contacts: Phone number auto populates from the Dialer
		// Test Case: 7227 - Contacts: Add contact button on the dialer adds new contact
		// Test Case: 7324 - Contacts: Add contact with new button on home screen
		// Test Case: 7472 - Phonebook: add contact button is displayed when generating a number with call button
		
		// Add contact from dialer
		ui.addContactFromDialer(num: vrsNum1, contactName: "Automate 5986")

		// Verify and Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5986")
		
		// Cleanup
		ui.gotoHome()
		ui.clearDialer()
	}

	func test_6243_SpanishFeatureTitle() {
		// Test Case: 6243 - Contacts: Verify that the feature title is "Enable Spanish Features" for SVRS Espanol contact
		// Navigate to Settings and Verify Spanish Feature Title is displayed
		ui.gotoSettings()
		ui.verifySpanishFeatureTitle()
		
		// Cleanup
		ui.gotoHome()

	}
	
	func test_6461_CompanyNameField() {
		// Test Case: 6461 - Company Name: Contacts: Verify that the Company name field is present
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6461", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Edit Contact
		ui.editContact(name: "Automate 6461")
		ui.enterCompany(name: "AutoCO 6461")
		ui.saveContact()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 6461")
		ui.gotoHome()
	}
	
	func test_6463_AddContactWithNameAndCompany() {
		// Test Case: 6463 - Contacts - Company Name: Verify creating a contact with both a personal name and a company name the personal name will be what is displayed in the phonebook list.
		
		// Navigate to Contacts
		ui.gotoSorensonContacts()
		
		// Test 6463
		// Add Contact with Company Name
		ui.addContact(name: "Automate 6463", companyName:"Do Not Display", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
		
		// Verify Company Name is not displayed
		ui.verifyContact(name: "Automate 6463")
		
		// Cleanup
		ui.deleteContact(name: "Automate 6463")
		ui.gotoHome()
	}

	func test_6467_EditCompanyName() {
		// Test Case: 6467 - Contacts - Company Name: Verify that the Company name field can be edited
		
		// Add Company Contact
		ui.gotoSorensonContacts()
		ui.addCompanyContact(name: "AutoCO 6467", homeNumber: "", workNumber: vrsNum1, mobileNumber: "")
		
		// Test 6467
		// Edit Company name
		ui.editContact(name: "AutoCO 6467")
		ui.removeCompanyName(name: "AutoCO 6467")
		ui.enterCompany(name: "AutoCO 6467A")
		ui.saveContact()
		
		// Verify Contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "AutoCO 6467A")
		
		// Delete Contact
		ui.deleteContact(name: "AutoCO 6467A")
		ui.gotoHome()
	}

	func test_6469_CompanyNameRestriction() {
		// Test Case: 6469 - Contacts - Company Name: Verify that the Company name field has a 64 character limit
		
		// Navigate to Contacts
		ui.gotoSorensonContacts()
		
		// Add Contact with 66 Characters
		ui.addCompanyContact(name: "Appleisthegiantitthestoreerrorthrewtytheyeartheamountthegrapearewe", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Verify Company Name only saved 64 characters
		ui.verifyContact(name: "Appleisthegiantitthestoreerrorthrewtytheyeartheamountthegrapeare")
		
		// Delete Contact
		ui.deleteContact(name: "Appleisthegiantitthestoreerrorthrewtytheyeartheamountthegrapeare")
		ui.gotoHome()
	}

	func test_7108_ContactFromCallHistoryIncludesName() {
		// Test Case: 7108 - Contacs: Adding new contact from call history does include the name in the contact
		// Setup
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepName = endpoint.userName else { return }
	
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		ui.hangup()
		
		// Add contact from history
		ui.gotoCallHistory(list: "All")
		ui.addContactFromHistory(coreName: oepName, contactName: "")
		
		// Verify contact had name
		ui.gotoSorensonContacts()
		ui.verifyContact(name: oepName)
		
		// cleanup
		ui.deleteContact(name: oepName)
	}
	
	func test_7123_ContactsWithSameNameAndCompanyName() {
		// Test Case: 7123 - Contacts: Creating 2 contacts with the same name for a company name, both contacts will display.
		
		// Navigate to Contacts and Grab Contacts list
		ui.gotoSorensonContacts()
		let testName = "Automate 7123"
		var list = [""]
		var contacts = ui.getContactsList()
		contacts.append(testName)
		contacts.append(testName)
		
		// Create Contacts with different Company Names and the same phone numbers
		ui.addContact(name: testName, companyName: "", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "", companyName: testName, homeNumber: vrsNum1, workNumber: "", mobileNumber: "")

		// Verify Contacts in phonebook
		list = ui.getContactsList()
		XCTAssertTrue(list == contacts)
		
		// Delete Contacts
		ui.deleteAllContacts()
		ui.gotoHome()
	}

	func test_7141_RemoveNameAddCompany() {
		// Test Case: 7141 - Contacts: UI: Editing and existing contact, removing the name and adding a company name causes the Contact to display the company name
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7141", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Remove Contact Name and add Company Name
		ui.editContact(name: "Automate 7141")
		ui.removeContactName(name: "Automate 7141")
		ui.enterCompany(name: "AutoCO 7141")
		ui.saveContact()
		
		// Verify Company Name
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "AutoCO 7141")
		ui.deleteContact(name: "AutoCO 7141")
		ui.gotoHome()
	}
	
	func test_7327_AddNumberToFullContact() {
		// Test Case: 7327 - Contacts: Add number to full contact from home screen button
		// setup
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7327", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)

		// replace contact home number
		ui.gotoHome()
		ui.replaceContactNumFromDialer(name: "Automate 7327", type: "home", oldNum: vrsNum1, newNum: vrsNum4)
		ui.saveContact()

		// verify home number
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 7327")
		ui.verifyContact(phnum: vrsNum4, type: "home")

		// cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7327")
	}

	func test_7332_AddContactPageFromDialer() {
		// Test Case: 7332 - Contacts: Pressing the "+" Add contact in Contacts, bring up new contact view
		
		// Enter Number and select add contact from Dialer
		ui.dial(phnum: vrsNum1)
		ui.selectAddNewContactFromDialer()
		
		// Verify Title of Page and select Cancel
		ui.verifyContactView()
		ui.selectCancel()
		
		// Clean Up Dialer and go to Home
		ui.clearDialer()
		ui.gotoHome()
	}

	func test_7410_DeleteNumberFromContact() {
		// Test Case: 7410 - Deleting a number from a contact leaves number field blank
		// setup
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7410", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		
		// Edit Contact
		ui.editContact(name: "Automate 7410")
		ui.removeContactNumber(type: "work")
		ui.saveContact()
		
		// log out and back in
		ui.logout()
		ui.login()
		
		// verify number removed
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 7410")
		ui.verifyNoContactNumber(type: "work")

		// cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7410")
	}
	
	func test_7416_CallContact() {
		// Test Case: 7416 - Contacts: Call a contact
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7416", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Call Contact
		ui.gotoSorensonContacts()
		testUtils.calleeNum = vrsNum1
		ui.callContact(name: "Automate 7416", type: "home")
		sleep(3)
		ui.verifyCallerID(text: "Automate 7416")
		ui.hangup()
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7416")
		ui.gotoHome()
	}

	func test_7424_ChangeFavNumType() {
		// Test Case: 7424 - Contacts: When changing a phone number from home to mobile that is on a favorited contact the app will not crash

		// Add Contact and Favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7424", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 7424")
		ui.addToFavorites()

		// Edit Contact, change num from home to mobile
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 7424")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: vrsNum1, type: "mobile")
		ui.saveContact()
		ui.addToFavorites()
		
		// Verify Favorite
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 7424", type: "mobile")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7424")
		ui.gotoHome()
	}
	
	func test_7470_AddOneToContactNumber() {
		// Test Case: 7470 - a number entered for a contact without a leading one receives a one
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7470", homeNumber: "", workNumber: "", mobileNumber: "8018087777")
		
		// verify number
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 7470")
		ui.verifyContact(phnum: "18018087777", type: "mobile")
		
		// cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7470")
	}
	
	func test_7765_BlockedCallSuccessCall() {
		// Test Case: 7765 - User is able to call to an endpoint that has the DUT blocked then place a call to another endpoint
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		
		// Setup block
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoBlockedList()
		ui.deleteBlockedList()
		ui.deleteAllContacts()
		
		// Add contact and block
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7765", homeNumber: dutNum, workNumber: "", mobileNumber: "")
		ui.gotoSorensonContacts()
		ui.blockContact(name: "Automate 7765")
		ui.logout()
		
		// Call Blocking OEP
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.call(phnum: altNum)
		sleep(3)
		ui.verifyCallFailed()
		
		// Call Additional OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		ui.hangup()
		
		// Clean up
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 7765")
		ui.gotoSorensonContacts()
		ui.deleteAllContacts()
		ui.gotoHome()
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_9760_AnyCombinationNumberField() {
		// Test Case: 9760 - Home, Work, and Mobile number field accepts any combination of numbers
		
		// Setup
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 9760", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Test (1)111-111-1111
		ui.editContact(name: "Automate 9760")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: "11111111111", type: "home")
		ui.verifyButtonEnabled(name: "Done", expected: false)
		ui.removeContactNumber(type: "home")
		
		// Test (1)000-000-0000
		ui.enterContact(number: "10000000000", type: "work")
		ui.verifyButtonEnabled(name: "Done", expected: false)
		ui.removeContactNumber(type: "work")
		
		// Test (1)801-292-3066
		ui.enterContact(number: "18012923066", type: "mobile")
		ui.verifyButtonEnabled(name: "Done", expected: true)
		ui.removeContactNumber(type: "mobile")
		ui.clickButton(name: "Cancel")
		
		// Clean Up
		ui.gotoHome()
		ui.deleteAllContacts()
	}
	
	func test_11165_InvalidPhoneNumbersInContacts() {
		// Test Case: 11165 - Contacts: Invalid phone numbers will be marked as invalid on contact page

		// Go to contacts
		ui.gotoSorensonContacts()

		// Click Add Contact btn
		ui.selectAddContact()

		// Enter Contact Name
		ui.enterContact(name: "Automate 11165")

		// Loop through 8 randomly generated invalid phone number combinations with 11 digits that start with 1-
		for _ in 0...8 {
			let randomNum = ui.generateRandomInvalidAreaCodeNumber()

			// Attempt to save invalid number to contact
			ui.enterContact(number: randomNum, type: "home")

			// Verify invalid number assignment to contact is not allowed
			ui.verifyDoneButtonDisabled()
		}

		// Cleanup
		ui.selectCancel()
		ui.gotoHome()
	}
	
	func test_11167_ContactWithEmoji() {
		// Test Case: 11167 - Emoji aren't supported in Contacts Name, Company Field
		
		// Copy Emojis
		ui.gotoSorensonContacts()
		ui.enterTextAndCopy(text: "😃😄")
		ui.clickButton(name: "Cancel")
		
		// Create contact with Emojis
		ui.addContactInfo(name: "Automate😃😄 11167", companyName: "", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.saveContact()
		ui.verifyContact(name: "Automate 11167")
		
		// Edit Contact and paste emojis in
		ui.editContactWithPaste(name: "Automate 11167")
		ui.verifyContact(name: "No Name")
		
		// Clean up
		ui.deleteAllContacts()
		ui.gotoHome()
	}
	
	func test_11183_ImportContactsWithEmoji() {
		// Test Case: 11183 - import Sorenson Contacts Number Password allows Emojis
		
		if #available(iOS 14.0, *) {
			// Setup
			ui.gotoSorensonContacts()
			ui.enterTextAndCopy(text: "😃😄")
			ui.clickButton(name: "Cancel")
			ui.gotoImportContacts()
			
			// Enter Emoji or Paste in Emoji
			ui.enterImportSorensonPassword(password: "😃😄")
			ui.enterImportSorensonPasswordPaste()
			ui.verifyPasswordImportContacts()
			ui.clickButton(name: "done")
			
			// Clean up
			ui.gotoHome()
		}
	}
	
	func test_11250_ImportContactFailure() {
		// Test Case: 11250 - Import VP Contacts: Import contacts failure
		
		// Setup
		ui.gotoSettings()
		ui.gotoImportContacts()
		
		// Import Contacts with wrong password
		ui.enterImportSorensonPhoneNumber(phnum: "17323005678")
		ui.enterImportSorensonPassword(password: "4321")
		ui.clickStaticText(text: "Import From Videophone Account")
		
		// Verify Error message
		ui.verifyImportAlert(alert: "Can't import contacts.")
		ui.importSorensonButtonOK()
		
		// Import contacts correct password
		ui.importVPContacts(phnum: "17323005678", pw: "1234")
		
		// Clean Up
		ui.gotoHome()
		ui.deleteAllContacts()
	}
	
	func test_11256_DeleteContactNoFreeze() {
		// Test Case: 11256 - Deleting contact does not freeze contacts/favorites/device tabs
		
		// Setup
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 11256-1", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 11256-2", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 11256-3", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 11256-4", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Delete contacts
		ui.deleteContact(name: "Automate 11256-1")
		ui.deleteContact(name: "Automate 11256-2")
		ui.deleteContact(name: "Automate 11256-3")
		
		// Verify Contact Are Available And Contacts Function
		ui.verifyContact(name: "Automate 11256-4")
		ui.editContact(name: "Automate 11256-4")
		ui.removeContactName(name: "Automate 11256-4")
		ui.enterContact(name: "Automate 11256-4-1")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: vrsNum2, type: "home")
		ui.saveContact()
		
		// Verify edit contact and scrolling up and down works
		ui.verifyContact(phnum: vrsNum2, type: "home")
		ui.scrollFavorites(dir: "Up")
		ui.scrollFavorites(dir: "Down")
		
		// Clean Up
		ui.gotoSorensonContacts()
		ui.deleteAllContacts()
		ui.gotoHome()
	}
	
	func test_12167_EditContactBeforeDelete(){
		// Test Case: 12167 - Contacts: User can scroll down on a contact and edit Name, Home, Work, and Mobile fields before a delete is done on the contact info screen
		
		// Navigate to Contacts and create a contact
		ui.gotoSorensonContacts()
		ui.addContact(name:"Test 12167", homeNumber: "18015551234", workNumber: "18015551235",mobileNumber: "18015551236")
		
		// Edit contact fields
		ui.editContact(name:"Test 12167")
		ui.removeContactName(name: "Test 12167")
		ui.enterContact(name:"Test 12167 Edited")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: "18015552234", type: "home")
		ui.removeContactNumber(type: "work")
		ui.enterContact(number:"18015552235", type: "work")
		ui.removeContactNumber(type:"mobile")
		ui.enterContact(number:"18015552236", type: "mobile")
		
		// Delete contact
		ui.clickStaticText(text: "Delete Contact")
		
		// Select Delete action confirm
		ui.clickButton(name: "Delete Contact")
		
		// Clean up
		ui.gotoHome()
	}
	
	func test_C36007_BlankNameBlankCompany() {
		// Test Case - C36007 - Contacts: blank name and company name should not crash app
		
		// Verify that contact without name or company name cannot be created
		ui.gotoSorensonContacts()
		ui.addContactInfo(name: "", companyName: "", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.verifyButtonEnabled(name: "Done", expected: false)
		
		// Verify app will not crash afterwards during cleanup steps
		ui.enterCompany(name: "sorenson")
		ui.saveContact()
		ui.deleteAllContacts()
		ui.gotoHome()
	}
	
	func test_C46682_AddContactFullList() {
		// Test Case - C46682 - Contacts: Adding a contact from History to a full Contacts list throws an appropriate error
		
		// Select account with full contact list
		ui.switchAccounts(number: contacts500, password: defaultPassword)
		
		// Create a call history with DUT
		ui.call(phnum: dutNum)
		ui.hangup()
		
		// Attempt to add contact and verify error message
		ui.gotoCallHistory(list: "All")
		ui.selectContactFromHistory(num: dutNum)
		ui.clickStaticText(text: "Add Contact")
		ui.verifyText(text: "The maximum number of allowable contacts has been reached.", expected: true)
		ui.clickButton(name: "OK")
		
		// Cleanup
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
}
