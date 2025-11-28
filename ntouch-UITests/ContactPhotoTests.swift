//
//  ContactPhotoTests.swift
//  ntouchUITests
//
//  Created by Mikael Leppanen on 1/24/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import XCTest

class ContactPhotoTests: XCTestCase {
	
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
		ui.enableContactPhotos(setting: true)
	}
	
	override func tearDown() {
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}

	func test_2908_2914_3567_4428_5193_AddPhoto() {
		// Test Case: 2908 - Contacts: Contacts can be assigned photos
		// Test Case: 2914 - Contacts: Creating a contact with a picture
		// Test Case: 3567 - Contacts: Test to make sure the camera function doesn't freeze
		// Test Case: 4428 - Contacts: Deleting contact with contact photo does not crash the app
		// Test Case: 5193 - Contact photos: Verify when touching take photo for a contact, device opens the camera application
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 2908
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2908", homeNumber: "18015757669", workNumber: "", mobileNumber: "")
		
		// Add photo
		ui.editContact(name: "Automate 2908")
		ui.selectGalleryPhoto()
		ui.saveContact()
		
		// Test 2914, 5193
		// Add contact and photo
		ui.gotoSorensonContacts()
		ui.addContactInfo(name: "Automate 2914", companyName: "", homeNumber: "18015757889", workNumber: "", mobileNumber: "")
		ui.takePhoto()
		ui.saveContact()
		
		// Verify Photos
		ui.gotoSorensonContacts()
		ui.verifyContactPhoto(name: "Automate 2908")
		ui.verifyContactPhoto(name: "Automate 2914")
		
		// Tests 3567
		ui.addContact(name: "Automate 3567", homeNumber: "18015757999", workNumber: "", mobileNumber: "")
		ui.editContact(name: "Automate 3567")
		ui.choosePhoto()
		ui.choosePhotoCancel()
		ui.takePhoto()
		ui.selectCancel()
		
		// Test 4428
		// Delete Contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 2908")
		ui.deleteContact(name: "Automate 2914")
		ui.deleteContact(name: "Automate 3567")
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_3120_3121_3525_5229_DisableContactPhotos() {
//		// Test Case: 3120 - Contact Photos: Disable Contact Photos
//		// Test Case: 3121 - Contact Photos: Toggle 'Disable Photos' option off
//		// Test Case: 3525 - Contact Photos: When the contact photos are disabled the default silhouette will be displayed
//		// Test Case: 5229 - Contacts: Creating contact from Call History with Contact Photos DISABLED, User is able to save Contact
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
////		core.addRemoteProfilePhoto(phoneNumber: oepNum)
//		core.setProfilePhotoPrivacy(phoneNumber: oepNum, value: "0")
//		
//		// Delete Contacts
//		ui.deleteAllContacts()
//		
//		// Delete Call History
//		ui.gotoCallHistory(list: "All")
//		ui.deleteAllCallHistory()
//		
//		// Disable contacts photos
//		ui.enableContactPhotos(setting: false)
//		
//		// Place Call
//		ui.call(phnum: oepNum)
//		con.answer()
//		sleep(3)
//		ui.hangup()
//		
//		// Test 5229
//		// Add Contact From History
//		ui.gotoCallHistory(list: "All")
//		ui.addContactFromHistory(num: oepNum, contactName: "Automate 3120")
//		
//		// Get disabled photo
//		ui.gotoSorensonContacts()
//		let contactPhotoDisabled = ui.getContactPhoto(name: "Automate 3120")
//		
//		// Test 3120, 3525
//		// verify photos disabled
//		ui.gotoCallHistory(list: "All")
//		ui.gotoSorensonContacts()
//		let contactPhoto = ui.getContactPhoto(name: "Automate 3120")
//		XCTAssertEqual(contactPhoto, contactPhotoDisabled)
//		
//		// Test 3121
//		// Enable Contact Photos and verify photos
//		ui.enableContactPhotos(setting: true)
//		ui.gotoSorensonContacts()
//		let contactPhoto2 = ui.getContactPhoto(name: "Automate 3120")
//		XCTAssertNotEqual(contactPhoto2, contactPhotoDisabled)
//		
//		// Cleanup
//		ui.deleteContact(name: "Automate 3120")
//		ui.gotoCallHistory(list: "All")
//		ui.deleteAllCallHistory()
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_3125_3127_3131_3457_4495_ContactProfilePhoto() {
//		// Test Case: 3125 - Contact - Phonebook: Import contact photo
//		// Test Case: 3127 - Contact - Phonebook: Import contact photo with only work number
//		// Test Case: 3131 - Contact - Phonebook: Import contact photo with only home number
//		// Test Case: 3457 - Contact Photos: Able to remove a contact photo
//		// Test Case: 4495 - Contact Photos: Device automatically imports a contact photo when contact is created
//		let endpoint = endpointsClient.checkOut()
//		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
////		core.addRemoteProfilePhoto(phoneNumber: oepNum)
//		core.setProfilePhotoPrivacy(phoneNumber: oepNum, value: "0")
//		
//		// Delete Contacts
//		ui.deleteAllContacts()
//		
//		// Test 3125
//		// Add Contact and get photo
//		ui.gotoSorensonContacts()
//		ui.addContact(name: "Automate 3125", homeNumber: oepNum, workNumber: "", mobileNumber: "")
//		let contactPhoto3125 = ui.getContactPhoto(name: "Automate 3125")
//		
//		// Remove Photo and verify (Test 3457)
//		ui.editContact(name: "Automate 3125")
//		ui.removePhoto()
//		ui.saveContact()
//		ui.gotoSorensonContacts()
//		let defaultPhoto3125 = ui.getContactPhoto(name: "Automate 3125")
//		XCTAssertNotEqual(defaultPhoto3125, contactPhoto3125)
//		
//		// Select Profile Photo and verify
//		ui.editContact(name: "Automate 3125")
//		ui.selectProfilePhoto()
//		ui.saveContact()
//		ui.gotoSorensonContacts()
//		let profilePhoto3125 = ui.getContactPhoto(name: "Automate 3125")
//		XCTAssertEqual(profilePhoto3125, contactPhoto3125)
//		ui.deleteContact(name: "Automate 3125")
//		
//		// Test 3127
//		// Add Contact and get photo
//		ui.gotoSorensonContacts()
//		ui.addContact(name: "Automate 3127", homeNumber: "", workNumber: oepNum, mobileNumber: "")
//		let contactPhoto3127 = ui.getContactPhoto(name: "Automate 3127")
//		
//		// Remove Photo and verify (Test 3457)
//		ui.editContact(name: "Automate 3127")
//		ui.removePhoto()
//		ui.saveContact()
//		ui.gotoSorensonContacts()
//		let defaultPhoto3127 = ui.getContactPhoto(name: "Automate 3127")
//		XCTAssertNotEqual(defaultPhoto3127, contactPhoto3127)
//		
//		// Select Profile Photo and verify
//		ui.editContact(name: "Automate 3127")
//		ui.selectProfilePhoto()
//		ui.saveContact()
//		ui.gotoSorensonContacts()
//		let profilePhoto3127 = ui.getContactPhoto(name: "Automate 3127")
//		XCTAssertEqual(profilePhoto3127, contactPhoto3127)
//		ui.deleteContact(name: "Automate 3127")
//		
//		// Test 3131
//		// Add Contact and get photo
//		ui.gotoSorensonContacts()
//		ui.addContact(name: "Automate 3131", homeNumber: "", workNumber: "", mobileNumber: oepNum)
//		let contactPhoto3131 = ui.getContactPhoto(name: "Automate 3131")
//
//		// Remove Photo and verify (Test 3457)
//		ui.editContact(name: "Automate 3131")
//		ui.removePhoto()
//		ui.saveContact()
//		ui.gotoSorensonContacts()
//		let defaultPhoto3131 = ui.getContactPhoto(name: "Automate 3131")
//		XCTAssertNotEqual(defaultPhoto3131, contactPhoto3131)
//
//		// Select Profile Photo and verify
//		ui.editContact(name: "Automate 3131")
//		ui.selectProfilePhoto()
//		ui.saveContact()
//		ui.gotoSorensonContacts()
//		let profilePhoto3131 = ui.getContactPhoto(name: "Automate 3131")
//		XCTAssertEqual(profilePhoto3131, contactPhoto3131)
//		
//		// Delete Contact
//		ui.deleteAllContacts()
//	}
	
	func test_3126_3132_3142_3462_4381_ImportContactsWithPhotos() {
		// Test Case: 3126 - Contact - Phonebook: Import contact photos for many contacts at once
		// Test Case: 3132 - Contact - Phonebook: Import contact photos for many contacts at once with only home numbers
		// Test Case: 3142 - Contact - Phonebook: Import photos when no photos are available
		// Test Case: 3462 - Contact Photos: Import contacts from another sorenson endpoint
		// Test Case: 4381 - Contacts: Imported contacts display without DUT being logged out then back
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 4381
		// Import VP Contacts
		ui.importVPContacts(phnum: "17323005678", pw: "1234", expectedNumber: "2")
		ui.importSorensonPhotos()
		
		// Verify Contacts
		ui.heartbeat() // refresh UI
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "Atest")
		ui.verifyContact(name: "Atest2")
		
		// Test 3132 & 3462
		// Verify Contact Photos
		ui.verifyContactPhoto(name: "Atest")
		ui.verifyContactDefaultPhoto(name: "Atest2")
		let contactPhoto = ui.getContactPhoto(name: "Atest")
		
		// Test 3126
		// Remove Photo and verify
		ui.editContact(name: "Atest")
		ui.removePhoto()
		ui.saveContact()
		ui.heartbeat() // refresh UI
		ui.gotoSorensonContacts()
		let defaultPhoto = ui.getContactPhoto(name: "Atest")
		XCTAssertNotEqual(defaultPhoto, contactPhoto)
		
		// Import Sorenson Photos
		ui.importSorensonPhotos()
		
		// Verify Photos
		ui.heartbeat() // refresh UI
		ui.gotoSorensonContacts()
		ui.verifyContactPhoto(name: "Atest")
		let contactPhoto2 = ui.getContactPhoto(name: "Atest")
		XCTAssertEqual(contactPhoto, contactPhoto2)
		
		// Test 3142
		// Import without photo
		ui.editContact(name: "Atest2")
		ui.selectProfilePhoto()
		ui.saveContact()
		
		// Delete Contacts
		ui.gotoHome()
		ui.deleteAllContacts()
	}
	
	func test_3254_3260_ProfilePhotoSelectionAndRemoval() {
		// Test Case: 3254 - Profile Photo: Remove Sorenson profile photo
		// Test Case: 3260 - Profile Photo: Add a Profile Photo from the Devices Album
		
		// 3260 - Go to Personal Info and add a photo
		ui.gotoPersonalInfo()
		ui.selectGalleryPhoto()
		ui.verifyProfileAvatar(expected: false)
		
		// 3254 - Remove photo
		ui.removePhoto()
		ui.verifyProfileAvatar(expected: true)
		ui.gotoHome()
	}
	
	func test_4089_EditContactPhotoCancel() {
		// Test Case: 4089 - Contact Photos: Verify when pressing the cancel or back button you are returned to the edit screen
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 4089", homeNumber: "18015757669", workNumber: "", mobileNumber: "")
		
		// edit contact and select the photo edit
		ui.editContact(name: "Automate 4089")
		
		// Select Take Photo and cancel
		ui.selectTakePhoto()
		ui.clickButton(name: "Cancel")

		// Verify Edit Contact Screen
		ui.verifyEditContact(name: "Automate 4089")
		
		// Select Choose photo and cancel
		ui.choosePhoto()
		ui.choosePhotoCancel()

		// Verify Edit Contact Screen
		ui.verifyEditContact(name: "Automate 4089")
		
		// Cleanup
		ui.deleteAllContacts()
	}
	
	func test_4092_EditContactPhotoAndReceiveCall() {
		// Test Case: 4092 - Contacts: Verify while editing a contact in Contact Photos you are able to answer a call
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
			
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 4092", homeNumber: "8015555555", workNumber: "", mobileNumber: "")
		
		// Edit contact and select the photo edit
		ui.editContact(name: "Automate 4092")
		ui.selectEditPhoto()
		
		// Receive call
		con.dial(number: dutNum)
		ui.incomingCall(response: "Answer")
		ui.hangup()
		ui.selectTop()
		ui.selectCancel()
		
		// Cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 4092")
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_5209_EditContactNumWithPhoto() {
//		// Test Case: 5209 - Contacts: Editing a contacts number with a photo then saving contact, photo is saved
//		let endpoint = endpointsClient.checkOut()
//		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
////		core.addRemoteProfilePhoto(phoneNumber: oepNum)
//		core.setProfilePhotoPrivacy(phoneNumber: oepNum, value: "0")
//		
//		// Delete Contacts
//		ui.deleteAllContacts()
//		
//		// Add Contact with 2 numbers
//		ui.gotoSorensonContacts()
//		ui.addContact(name: "Automate 5209", homeNumber: oepNum, workNumber: "18015757669", mobileNumber: "")
//		let contactPhoto = ui.getContactPhoto(name: "Automate 5209")
//		
//		// Edit work Number
//		ui.editContact(name: "Automate 5209")
//		ui.enterContact(number: "18015757889", type: "work")
//		ui.saveContact()
//		ui.gotoSorensonContacts()
//		let contactPhoto2 = ui.getContactPhoto(name: "Automate 5209")
//		
//		// Verify photo
//		XCTAssertEqual(contactPhoto, contactPhoto2)
//		
//		// Delete contact
//		ui.gotoSorensonContacts()
//		ui.deleteContact(name: "Automate 5209")
//		ui.gotoHome()
//	}
	
	func test_5325_TakeProfilePhoto() {
		// Test Case: 5325 - Contacts: Verify the App behaves as expected when touching 'take a photo' for the Users' personal contact photo
		// Go to Personal info
		ui.gotoPersonalInfo()
		
		// Take photo
		ui.takePhoto()
		
		// Remove photo
		ui.removePhoto()
		ui.gotoHome()
	}
}
