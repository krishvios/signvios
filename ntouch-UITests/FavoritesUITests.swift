//
//  FavoritesUITests.swift
//  ntouch
//
//  Created by Mikael Leppanen on 10/6/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

import XCTest

class FavoritesUITests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["favoritesAltNum"]!
	let vrsNum1 = "18015757669"
	let vrsNum2 = "18015757559"
	let vrsNum3 = "18015757889"
	let vrsNum4 = "15552223333"
	let vrsNum5 = "15553334444"
	let endpointsClient = TestEndpointsClient.shared
	
	override func setUp() {
		super.setUp()
		ui.rotate(dir: cfg["Orientation"]!)
		continueAfterFailure = false
		ui.ntouchActivate()
		ui.clearAlert()
		// TODO: Server for CoreAccess currently not available
//		verifySetup()
		ui.waitForMyNumber()
	}
	
	override func tearDown() {
		super.tearDown()
		endpointsClient.closeVrclConnections()
		endpointsClient.checkInEndpoints()
		ui.ntouchTerminate()
	}
	
	var userDefaults: [String:String] = [
		"ContactFavoriteEnabled": "1"
	]
	
	// TODO: Server for CoreAccess currently not available
//	func verifySetup() {
//		// user settings
//		let allSettings = core.userSettingsGet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: dutNum)
//		for (key, val) in userDefaults {
//			if ( allSettings[key] != val ) {
//				print("User setting not correct setting:" )
//				core.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: dutNum, userSetting: key, settingValue: val)
//				ui.heartbeat()
//			}
//		}
//	}
	
	func test_5395_RemoveFavNum() {
		// Test Case: 5395 - Removing a number from a contact removes the associated favorite entry
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact and Favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5395", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: "")
		ui.selectContact(name: "Automate 5395")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		
		// Verify Favorite
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5395", type: "home")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5395")
	}
	
	func test_5446_Scrolling() {
		// Test Case: 5446 - Favorites are scrollable
		
		// Logout
		ui.logout()
		
		// Login to account with 30 favorites
		ui.login(phnum: altNum, password: defaultPassword)
		ui.gotoFavorites()
		
		// Scroll Down and Up
		ui.scrollFavorites(dir: "Down")
		ui.scrollFavorites(dir: "Up")
		
		// Login to original account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_5400_5940_7654_AddMultipleContactsFav() {
		// Test Case: 5400 - User is able to add favorites from multiple contacts
		// Test Case: 5940 - Favorite: For each contact, there is a way to tag the contact as a favorite
		// Test Case: 7654 - User can add all numbers from a contact to favorites
		
		// Delete Contacts
		ui.deleteAllContacts()

		// Test 5400
		// Add Contact and Favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5400", homeNumber: vrsNum4, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5400")
		ui.addToFavorites()

		// Test 5940
		// Add 2nd Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5940", homeNumber: vrsNum5, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5940")
		ui.addToFavorites()

		// Test 7654
		// Add third contact
		// then favorite all it's numbers
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7654", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		ui.selectContact(name: "Automate 7654")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		ui.addToFavorites(type: "mobile", phnum: vrsNum3)

		// Verify Favorites
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5400", type: "home")
		ui.verifyFavorite(name: "Automate 5940", type: "home")
		ui.verifyFavorite(name: "Automate 7654", type: "home")
		ui.verifyFavorite(name: "Automate 7654", type: "work")
		ui.verifyFavorite(name: "Automate 7654", type: "mobile")
		
		// Delete Contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5400")
		ui.deleteContact(name: "Automate 5940")
		ui.deleteContact(name: "Automate 7654")
	}
	
	func test_5401_5404_NoFavorites() {
		// Test Case: 5401 - 0 Favorites will be displayed when all favorites are removed
		// Test Case: 5404 - If there are no favorites an instructional message will display
		
		// Test 5401
		// Verify 0 Favorites
		ui.deleteAllContacts()
		ui.deleteAllFavorites()
		ui.verifyFavorites(count: 0)
		
		// Test 5404
		// Verify Favorites Message
		ui.verifyFavoritesMessage()
	}
	
	func test_5418_5419_VerifyFavoriteStar() {
		// Test Case: 5418 - Empty star displayed
		// Test Case: 5419 - Sold star displays
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5418
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5418", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Verify No Star
		ui.selectContact(name: "Automate 5418")
		ui.verifyNoFavoriteStar(type: "home")
		
		// Test 5419
		// Add to favroties and verify star
		ui.addToFavorites()
		ui.verifyFavoriteStar(type: "home")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5418")
	}
	
	func test_5420_5430_DeleteFavorite() {
		// Test Case: 5420 - Delete favorite and check that contact has an empty star
		// Test Case: 5430 - Deleting favorites does not delete the number in the contact
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5420
		// Create a contact and favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5420", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5420")
		ui.addToFavorites()
		
		// Delete Favorite
		ui.gotoFavorites()
		ui.deleteFavorite(name: "Automate 5420", type: "home")
		
		// Verify No Star
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 5420")
		ui.verifyNoFavoriteStar(type: "home")

		// Test 5430
		// Verify Contact Number
		ui.verifyContact(phnum: vrsNum1, type: "home")

		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5420")
	}
	
	func test_5421_5422_5423_AddContactNumNoStar() {
		// Test Case: 5421 - Add work number to contact - favorite star should be appear empty
		// Test Case: 5422 - Add mobile number to contact - favorite star should be appear empty
		// Test Case: 5423 - Add home number to contact - favorite star should be appear empty
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5421
		// Add work number
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5421", homeNumber: "", workNumber: vrsNum1, mobileNumber: "")
		
		// Verify no work star
		ui.selectContact(name: "Automate 5421")
		ui.verifyNoFavoriteStar(type: "work")
		
		// Test 5422
		// Add mobile number
		ui.selectEdit()
		ui.enterContact(number: vrsNum2, type: "mobile")
		ui.saveContact()
		
		// Verify no mobile star
		ui.verifyNoFavoriteStar(type: "mobile")
		
		// Test 5423
		// Add home number
		ui.selectEdit()
		ui.enterContact(number: vrsNum3, type: "home")
		ui.saveContact()
		
		// Verify no home star
		ui.verifyNoFavoriteStar(type: "home")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5421")
	}
	
	func test_5424_5425_5426_FavUnFavHomeWorkMobile() {
		// Test Case: 5424 - Add 3 numbers verify empty star appears
		// Test Case: 5425 - Contact with home, work, mobile numbers are all able to be favorited
		// Test Case: 5426 - Contact with home, work, mobile favorites are all able to be unfavorited
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5424
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5424", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		
		// Verify empty stars
		ui.selectContact(name: "Automate 5424")
		ui.verifyNoFavoriteStar(type: "home")
		ui.verifyNoFavoriteStar(type: "work")
		ui.verifyNoFavoriteStar(type: "mobile")
		
		// Test 5424
		// Favorite home, work, mobile
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		ui.addToFavorites(type: "mobile", phnum: vrsNum3)
		
		// Verify Favorites
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5424", type: "home")
		ui.verifyFavorite(name: "Automate 5424", type: "work")
		ui.verifyFavorite(name: "Automate 5424", type: "mobile")
		
		// Test 5426
		// Unfavorite home, work, mobile
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 5424")
		ui.removeFavorite(type: "home", phnum: vrsNum1)
		ui.removeFavorite(type: "work", phnum: vrsNum2)
		ui.removeFavorite(type: "mobile", phnum: vrsNum3)
		
		// Verify Favorites
		ui.gotoFavorites()
		ui.verifyNoFavorite(name: "Automate 5424", type: "home")
		ui.verifyNoFavorite(name: "Automate 5424", type: "work")
		ui.verifyNoFavorite(name: "Automate 5424", type: "mobile")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5424")
	}
	
	func test_5427_6182_EditFavNumVerifyStar() {
		// Test Case: 5427 - Changing favorited number does not change favorited state - solid star
		// Test Case: 6182 - Editing favorited contact does change favorited name
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5427", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.addContact(name: "Automate 6182", homeNumber: vrsNum2, workNumber: "", mobileNumber: "")
		
		// Add favorite
		ui.selectContact(name: "Automate 5427")
		ui.addToFavorites()
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 6182")
		ui.addToFavorites()
		
		// Edit Home number
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 5427")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: vrsNum3, type: "home")
		ui.saveContact()
		
		// Edit Contact Name
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 6182")
		ui.removeContactName(name: "Automate 6182")
		ui.enterContact(name: "Automate 6182 edited")
		ui.saveContact()
		
		// Verify Star
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 5427")
		ui.verifyFavoriteStar(type: "home")
		
		// Verify Edited Name
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 6182 edited", type: "home")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5427")
		ui.deleteContact(name: "Automate 6182 edited")
		
	}
	
	func test_5428_5487_5489_5490_FavoritesOrder() {
		// Test Case: 5428 - Favorites in the order they were added
		// Test Case: 5487 - Favorites order is not change when favorites are removed
		// Test Case: 5489 - Favorites are not rearranged after logging back in
		// Test Case: 5490 - Favorites are not rearranged after backgrounding
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5428
		// Add and favorite 3 contacts
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5428A", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5428A")
		ui.addToFavorites()
		
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5428B", homeNumber: vrsNum2, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5428B")
		ui.addToFavorites()
		
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5428C", homeNumber: vrsNum3, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5428C")
		ui.addToFavorites()
		
		// Verify favorites in order added
		ui.gotoFavorites()
		ui.verifyFavoritePosition(favorite: "Automate 5428A home", position: 0)
		ui.verifyFavoritePosition(favorite: "Automate 5428B home", position: 1)
		ui.verifyFavoritePosition(favorite: "Automate 5428C home", position: 2)
		
		// Test 5487
		// Delete favorite and verify order
		ui.deleteFavorite(name: "Automate 5428B", type: "home")
		ui.verifyFavoritePosition(favorite: "Automate 5428A home", position: 0)
		ui.verifyFavoritePosition(favorite: "Automate 5428C home", position: 1)
		
		// Test 5489
		// Logout, Login, and verify order
		ui.logout()
		ui.selectLogin()
		ui.gotoFavorites()
		ui.verifyFavoritePosition(favorite: "Automate 5428A home", position: 0)
		ui.verifyFavoritePosition(favorite: "Automate 5428C home", position: 1)
		
		// Test 5490
		// Background and verify order
		ui.pressHomeBtn()
		ui.ntouchActivate()
		ui.verifyFavoritePosition(favorite: "Automate 5428A home", position: 0)
		ui.verifyFavoritePosition(favorite: "Automate 5428C home", position: 1)
		
		// Delete contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5428A")
		ui.deleteContact(name: "Automate 5428B")
		ui.deleteContact(name: "Automate 5428C")
	}
	
	func test_5433_5434_5435_CallFavorite() {
		// Test Case: 5433 - Selecting home favorite places call
		// Test Case: 5434 - Selecting work favorite places call
		// Test Case: 5435 - Selecting mobile favorite places call
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5433
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5433", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		
		// Favorite home and call
		ui.selectContact(name: "Automate 5433")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.gotoFavorites()
		testUtils.calleeNum = vrsNum1
		ui.callFavorite(name: "Automate 5433", type: "home")
		sleep(3)
		ui.hangup()
		
		// Test 5434
		// Favorite work and call
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 5433")
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		ui.gotoFavorites()
		testUtils.calleeNum = vrsNum2
		ui.callFavorite(name: "Automate 5433", type: "work")
		sleep(3)
		ui.hangup()
		
		// Test 5435
		// Favorite mobile and call
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 5433")
		ui.addToFavorites(type: "mobile", phnum: vrsNum3)
		ui.gotoFavorites()
		testUtils.calleeNum = vrsNum3
		ui.callFavorite(name: "Automate 5433", type: "mobile")
		sleep(3)
		ui.hangup()
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5433")
	}
	
	func test_5436_AddNewNumNoFavorite() {
		// Test Case: 5436 - Adding new number to contact does not automatically add it as a favorite
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5436", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Add favorite
		ui.selectContact(name: "Automate 5436")
		ui.addToFavorites()
		
		// Add Work number
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 5436")
		ui.enterContact(number: vrsNum2, type: "work")
		ui.saveContact()
		
		// Verify Favorites
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5436", type: "home")
		ui.verifyNoFavorite(name: "Automate 5436", type: "work")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5436")
	}
	
	func test_5437_BlockFavorite() {
		// Test Case: 5437 - Blocking a contact with a favorite does not block calls to the favorite

		// Delete Contacts and Blocked list
		ui.deleteAllContacts()
		ui.gotoBlockedList()
		ui.deleteBlockedList()

		// Add contact with favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5437", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5437")
		ui.addToFavorites()

		// Block Favorite
		ui.gotoBlockedList()
		ui.blockFavorite(name: "Automate 5437", num: vrsNum1)

		// Call Favorite
		ui.gotoFavorites()
		testUtils.calleeNum = vrsNum1
		ui.callFavorite(name: "Automate 5437", type: "home")
		sleep(3)
		ui.hangup()

		// Delete Blocked Number
		ui.gotoBlockedList()
		ui.deleteBlockedNumber(name: "Automate 5437")

		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5437")
	}
	
	func test_5438_6481_FavoritesCount() {
		// Test Case: 5438 - Favorites counter displays the correct number of favorites
		// Test Case: 6481 - Favorites count tag is updated after a favorite is deleted BUG# 20439
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5438
		// Add contact 3 favorites
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5438", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		ui.selectContact(name: "Automate 5438")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		ui.addToFavorites(type: "mobile", phnum: vrsNum3)
		
		// Verify Favorites Count
		ui.gotoFavorites()
		ui.verifyFavorites(count: 3)
		
		// Add contact 2 favorites
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6481", homeNumber: vrsNum4, workNumber: vrsNum5, mobileNumber: "")
		ui.selectContact(name: "Automate 6481")
		ui.addToFavorites(type: "home", phnum: vrsNum4)
		ui.addToFavorites(type: "work", phnum: vrsNum5)
		
		// Verify Favorites Count
		ui.gotoFavorites()
		ui.verifyFavorites(count: 5)
		
		// Test 6481
		// Delete 1 Favorite and Verify Count
		ui.deleteFavorite(name: "Automate 6481", type: "home")
		ui.verifyFavorites(count: 4)
		
		// Delete contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5438")
		ui.deleteContact(name: "Automate 6481")
	}
	
	func test_5440_DeleteMultipleContactFav() {
		// Test Case: 5440 - Delete favorites from multiple contact at one time
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add and favorite contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5440A", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5440A")
		ui.addToFavorites()
		
		// Add and favorite 2nd contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5440B", homeNumber: "", workNumber: vrsNum2, mobileNumber: "")
		ui.selectContact(name: "Automate 5440B")
		ui.addToFavorites()
		
		// Delete both favorites
		ui.deleteAllFavorites()
		
		// Verify No Favorites
		ui.verifyNoFavorite(name: "Automate 5440A", type: "home")
		ui.verifyNoFavorite(name: "Automate 5440B", type: "work")
		
		// Delete contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5440A")
		ui.deleteContact(name: "Automate 5440B")
	}
	
	func test_5441_FavoriteCallHistory() {
		// Test Case: 5441 - Placing call from favorites creates a call history
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Delete All Call History
		ui.gotoCallHistory(list: "All")
		ui.deleteAllCallHistory()
		
		// Add and favorite contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5441", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5441")
		ui.addToFavorites()
		
		// Call favorite
		ui.gotoFavorites()
		testUtils.calleeNum = vrsNum1
		ui.callFavorite(name: "Automate 5441", type: "home")
		sleep(3)
		ui.hangup()
		
		// Verify Call History
		ui.gotoCallHistory(list: "All")
		ui.verifyCallHistory(name: "Automate 5441", num: vrsNum1)
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5441")
	}
	
	func test_5442_AddContactFavNum() {
		// Test Case: 5442 - Number can be favorited at time of contact creation
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5442", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Add favorite
		ui.selectContact(name: "Automate 5442")
		ui.addToFavorites()
		
		// Verify Favorite
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5442", type: "home")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5442")
	}
	
	func test_5443_5447_5432_AddFavoriteEditDelete() {
		// Test Case: 5443 - Number can be favorited after contact is created
		// Test Caes: 5447 - Changing name and number for a favorite
		// Test Case: 5432 - Deleting contact deletes favorite
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5443
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5443", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Add Favorite
		ui.selectContact(name: "Automate 5443")
		ui.addToFavorites()
		
		// Verify Favorite
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5443", type: "home")
		
		// Test 5447
		// Edit Contact namd and number
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 5443")
		ui.removeContactName(name: "Automate 5443")
		ui.enterContact(name: "Automate 5447")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: vrsNum2, type: "home")
		ui.saveContact()
		
		// Verify Favorite
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5447", type: "home")
		ui.verifyNoFavorite(name: "Automate 5443", type: "home")
		
		// Test 5432
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5447")
		
		// Verify no Favorite
		ui.gotoFavorites()
		ui.verifyNoFavorite(name: "Automate 5447", type: "home")
	}
	
	func test_5444_AddContactFav2Num() {
		// Test Case: 5444 - 2 numbers can be favorited at time of contact creation
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5444", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: "")
		
		// Favorite home and work numbers
		ui.selectContact(name: "Automate 5444")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		
		// Verify Favorites
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5444", type: "home")
		ui.verifyFavorite(name: "Automate 5444", type: "work")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5444")
	}
	
	func test_5445_AddContactFav3Num() {
		// Test Case: 5445 - 3 numbers can be favorited at time of contact creation
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5445", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		
		// Favorite home, work, mobile
		ui.selectContact(name: "Automate 5445")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		ui.addToFavorites(type: "mobile", phnum: vrsNum3)
		
		// Verify Favorites
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 5445", type: "home")
		ui.verifyFavorite(name: "Automate 5445", type: "work")
		ui.verifyFavorite(name: "Automate 5445", type: "mobile")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5445")
	}
	
	func test_5450_5451_5452_AddPhotos() {
		// Test Case: 5450 - Adding contact photo through take photo diplays on favorites
		// Test Case: 5451 - Adding contact photo through gallery diplays on favorites
		// Test Case: 5452 - Favorites photos remain after logging out then in

		// Delete Contacts
		ui.deleteAllContacts()

		// Test 5450
		// Add contact and favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5450", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5450")
		ui.addToFavorites()

		// Take Photo
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 5450")
		ui.takePhoto()
		ui.saveContact()

		// Test 5451
		// Add contact and favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5451", homeNumber: vrsNum2, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5451")
		ui.addToFavorites()

		// Select Gallery Photo
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 5451")
		ui.selectGalleryPhoto()
		ui.saveContact()

		// Verify Photos
		ui.gotoFavorites()
		ui.verifyFavoritePhoto(name: "Automate 5450", type: "home")
		ui.verifyFavoritePhoto(name: "Automate 5451", type: "home")

		//Test 5452
		// Logout and Log back in
		ui.logout()
		ui.login()

		// Verify Photo
		ui.gotoFavorites()
		ui.verifyFavoritePhoto(name: "Automate 5450", type: "home")
		ui.verifyFavoritePhoto(name: "Automate 5451", type: "home")

		// Delete contact
		ui.deleteAllContacts()
	}
	
	func test_5457_RemoveFavoriteStar() {
		// Test Case: 5457 - User can remove a favorite from the Contact Edit screen by deselecting a star
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact and favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5457", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5457")
		ui.addToFavorites()
		
		// Remove Favorite
		ui.removeFavorite()
		
		// Verify No Favorite
		ui.gotoFavorites()
		ui.verifyNoFavorite(name: "Automate 5457", type: "home")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5457")
	}
	
	func test_5454_5458_5396_FavoritesAfterLogin() {
		// Test Case: 5454 - Logging into different device with same number displays the favorites for that account
		// Test Case: 5458 - Favorites will be displayed upon login
		// Login to account with 30 favorites
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Verify Favorites Count
		ui.gotoFavorites()
		ui.verifyFavorites(count: 30)
		
		// Test 5396
		// Background the app
		ui.pressHomeBtn()

		// Open and Verify Favorites Count
		ui.ntouchActivate()
		ui.verifyFavorites(count: 30)

		// Login to original account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_5465_FavoritesNotImported() {
		// Test Case: 5465 - Favorites will not be imported with Contact Import
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Import VP Contacts
		ui.importVPContacts(phnum: "17323005678", pw: "1234")
		
		// Verify No Favorite was added 
		ui.gotoFavorites()
		ui.verifyFavorites(count: 0)
		
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Atest")
		ui.deleteContact(name: "Atest2")
	}
	
	func test_5484_5486_5488_5491_SortFavorites() {
		// Test Case: 5484 - Order the Favorites list
		// Test Case: 5486 - User is not able to move more than one favorite at a time
		// Test Case: 5488 - New favorites are added at the bottom of a rearranged list
		// Test Case: 5491 - Contact with 3 favorites can be rearranged

		// Delete Contacts
		ui.deleteAllContacts()

		// Test 5484, 5486, 5491
		// Add Contact with 3 favorites
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5484", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		ui.selectContact(name: "Automate 5484")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		ui.addToFavorites(type: "mobile", phnum: vrsNum3)

		// Verify order
		ui.gotoFavorites()
		ui.verifyFavoritePosition(favorite: "Automate 5484 home", position: 0)
		ui.verifyFavoritePosition(favorite: "Automate 5484 work", position: 1)
		ui.verifyFavoritePosition(favorite: "Automate 5484 mobile", position: 2)

		// Sort the Favorites list
		ui.moveFavorite(favorite: "Automate 5484, work", toFavorite: "Automate 5484, home")
		ui.moveFavorite(favorite: "Automate 5484, home", toFavorite: "Automate 5484, mobile")

		// Verify order
		ui.verifyFavoritePosition(favorite: "Automate 5484 work", position: 0)
		ui.verifyFavoritePosition(favorite: "Automate 5484 mobile", position: 1)
		ui.verifyFavoritePosition(favorite: "Automate 5484 home", position: 2)

		// Test 5488
		// Add Contact with 1 favorite
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5488", homeNumber: vrsNum4, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 5488")
		ui.addToFavorites()

		// Verify order
		ui.gotoFavorites()
		ui.verifyFavoritePosition(favorite: "Automate 5484 work", position: 0)
		ui.verifyFavoritePosition(favorite: "Automate 5484 mobile", position: 1)
		ui.verifyFavoritePosition(favorite: "Automate 5484 home", position: 2)
		ui.verifyFavoritePosition(favorite: "Automate 5488 home", position: 3)

		// Delete contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5484")
		ui.deleteContact(name: "Automate 5488")
	}
	
	func test_5494_MaxFavoritesAdd1() {
		// Test Case: 5494 - The maximum number of favorites a user can have is 30
		// Login to account with 30 favorites
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Add Contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5494", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")

		// Add favorite verify error
		ui.selectContact(name: "Automate 5494")
		ui.addToFavorites()
		ui.verifyFavoritesListFull()

		// Delete Favorite
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Larry Page1001")
		ui.removeFavorite()

		// Add Favorite
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Larry Page1001")
		ui.addToFavorites()

		// Add favorite verify error
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 5494")
		ui.addToFavorites()
		ui.verifyFavoritesListFull()

		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5494")

		// Login to original account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_5942_AddToFavoritesBtn1Num() {
		// Test Case: 5942 - Favorite: Contact with one number, select Add to Favorites
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5942", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")

		// Select Add to Favorites, home
		ui.selectContact(name: "Automate 5942")
		ui.addToFavorites()
		
		// Verify Star
		ui.verifyFavoriteStar(type: "home")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5942")
	}
	
	func test_5945_AddToFavoritesBtn3Num() {
		// Test Case: 5945 - Favorite: Contact with one number, select Add to Favorites
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5945", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		
		// Select Add to Favorites, work
		ui.selectContact(name: "Automate 5945")
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		
		// Verify Star
		ui.verifyFavoriteStar(type: "work")
		ui.verifyNoFavoriteStar(type: "home")
		ui.verifyNoFavoriteStar(type: "mobile")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5945")
	}
	
	func test_5954_5955_5956_C36006_FavEnabled() {
		// Test Case: 5954 - Favorites Distribution: Favorites enabled, Favorites button will appear in phonebook
		// Test Case: 5955 - Favorites Distribution: Favorites enabled, stars will appear when creating a contact
		// Test Case: 5956 - Favorites Distribution: Favorites enabled, stars will appear when editing a contact
		// Test Case: C36006 - Contacts: User can add all contacts numbers to favorites
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Test 5954
		// Verify Favorites Page
		ui.gotoFavorites()
		
		// Test 5955
		// Verify Favorite Star Buttons
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 5954", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		ui.selectContact(name: "Automate 5954")
		ui.verifyAddToFavoritesBtn()
		
		// Test 5956/C36006
		// Add numbers to favorites and verify stars
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		ui.addToFavorites(type: "mobile", phnum: vrsNum3)
		ui.verifyFavoriteStar(type: "home")
		ui.verifyFavoriteStar(type: "work")
		ui.verifyFavoriteStar(type: "mobile")
		
		// Delete Contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 5954")
	}

	// TODO: Server for CoreAccess currently not available
//	func test_5961_5962_5964_5965_6175_6176_FavPhotos() {
//		// Test Case: 5961 - The user can add a favorite with a contact photo to the favorites list
//		// Test Case: 5962 - The user can add a favorite without a contact photo to the favorites list
//		// Test Case: 5964 - The user can remove a favorite with a contact photo to the favorites list
//		// Test Case: 5965 - The user can remove a favorite without a contact photo to the favorites list
//		// Test Case: 6175 - Favorites: A favorite number has a photo associated with it if underlying contact does
//		// Test Case: 6176 - Favorites: A favorite number has no photo associated with it if underlying contact has no photo
//		let endpoint = endpointsClient.checkOut()
//		guard let _ = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
////		core.addRemoteProfilePhoto(phoneNumber: oepNum)
//		core.setProfilePhotoPrivacy(phoneNumber: oepNum, value: "0")
//		ui.enableContactPhotos(setting: true)
//
//		// Delete Contacts
//		ui.deleteAllContacts()
//
//		// Add Contact with Profile Photo and Favorite Home number
//		ui.gotoSorensonContacts()
//		ui.addContact(name: "Automate 5961", homeNumber: oepNum, workNumber: "", mobileNumber: "")
//		ui.selectContact(name: "Automate 5961")
//		ui.addToFavorites()
//
//		// Add Contact without Photo and Favorite Home number
//		ui.gotoSorensonContacts()
//		ui.addContact(name: "Automate 5962", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
//		ui.selectContact(name: "Automate 5962")
//		ui.addToFavorites()
//
//		// Go to Favorites
//		ui.gotoFavorites()
//
//		// Test 5961 & 6175
//		// Verify Favorite Photo
//		ui.verifyFavoritePhoto(name: "Automate 5961", type: "home")
//
//		// Test 5962 & 6176
//		// Verify Default Favorite Photo
//		ui.verifyFavoriteDefaultPhoto(name: "Automate 5962", type: "home")
//
//		// Test 5964
//		// Delete Favorite with photo and verify
//		ui.deleteFavorite(name: "Automate 5961", type: "home")
//		ui.verifyNoFavorite(name: "Automate 5961", type: "home")
//
//		// Test 5965
//		// Delete Favorite with Default photo and verify
//		ui.deleteFavorite(name: "Automate 5962", type: "home")
//		ui.verifyNoFavorite(name: "Automate 5962", type: "home")
//
//		// Delete Contacts
//		ui.gotoSorensonContacts()
//		ui.deleteContact(name: "Automate 5961")
//		ui.deleteContact(name: "Automate 5962")
//	}
	
	func test_6178_RemoveFavPhotoViaAssociatedContact() {
		// Test Case: 6178 - Favorites: Removing a photo from a contact removes the photo from any favorites associated.

		// Delete Contacts
		ui.deleteAllContacts()

		// Add Contact with Profile Photo and Favorite Home number
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6178", homeNumber: "17323005678", workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 6178")
		ui.addToFavorites()

		// Go to Favorites
		ui.gotoFavorites()

		// Verify Favorite Photo
		ui.verifyFavoritePhoto(name: "Automate 6178", type: "home")
		
		// Delete photo from contact and verify photo is gone from corresponding favorite
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 6178")
		ui.removePhoto()
		ui.saveContact()
		ui.gotoFavorites()
		ui.verifyNoFavoritePhoto(name: "Automate 6178", type: "home")
		
		// Delete Contacts
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 6178")
	}
	
	func test_6479_FavContactOrder() {
		// Test Case:  6479 - Favorites: Only the favorite moves, the contact stays in place
		// Delete Contacts
		ui.deleteAllContacts()

		// Add 5 Contacts with favorites
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6479A", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 6479A")
		ui.addToFavorites()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6479B", homeNumber: vrsNum2, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 6479B")
		ui.addToFavorites()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6479C", homeNumber: vrsNum3, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 6479C")
		ui.addToFavorites()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6479D", homeNumber: vrsNum4, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 6479D")
		ui.addToFavorites()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6479E", homeNumber: vrsNum5, workNumber: "", mobileNumber: "")
		ui.selectContact(name: "Automate 6479E")
		ui.addToFavorites()

		// Get Contacts List
		ui.gotoSorensonContacts()
		let contactsList = ui.getContactsList()

		// Sort the Favorites List
		ui.gotoFavorites()
		ui.moveFavorite(favorite: "Automate 6479B, home", toFavorite: "Automate 6479A, home")

		// Verify Favorites order
		ui.verifyFavoritePosition(favorite: "Automate 6479B home", position:0)
		ui.verifyFavoritePosition(favorite: "Automate 6479A home", position:1)
		ui.verifyFavoritePosition(favorite: "Automate 6479C home", position:2)
		ui.verifyFavoritePosition(favorite: "Automate 6479D home", position:3)
		ui.verifyFavoritePosition(favorite: "Automate 6479E home", position:4)

		// Verify Contacts order
		ui.gotoSorensonContacts()
		ui.verifyContactsOrder(list: contactsList)

		// Delete Contacts
		ui.deleteAllContacts()
	}
	
	func test_6483_30FavoritesDelete1() {
		// Test Case: 6483 - Favorites: When favorites are full users are not able to remove a favorite from a contact. BUG# 20437
		
		// Login to account with 30 favorites
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Go to Contacts and Remove Favorite
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Larry Page1001")
		ui.removeFavorite()
		
		// Verify No Favorite
		ui.gotoFavorites()
		ui.verifyNoFavorite(name: "Larry Page1001", type: "home")
		
		// Restore Favorite
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Larry Page1001")
		ui.addToFavorites()
		
		// Login to original account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_6485_Contact3FavDelete1() {
		// Test Case: 6485 - 3 Favorites from the same contact, remove on from favorites list BUG# 20498
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 6485", homeNumber: vrsNum1, workNumber: vrsNum2, mobileNumber: vrsNum3)
		
		// Favorite home, work, mobile
		ui.selectContact(name: "Automate 6485")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		ui.addToFavorites(type: "work", phnum: vrsNum2)
		ui.addToFavorites(type: "mobile", phnum: vrsNum3)
		
		// Delete home favorite
		ui.gotoFavorites()
		ui.deleteFavorite(name: "Automate 6485", type: "home")
		
		// Verify Favorite Stars
		ui.gotoSorensonContacts()
		ui.selectContact(name: "Automate 6485")
		ui.verifyNoFavoriteStar(type: "home")
		ui.verifyFavoriteStar(type: "work")
		ui.verifyFavoriteStar(type: "mobile")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 6485")
	}
	
	func test_7392_AddEditDeleteFavorite() {
		// Test Case: 7392 - Add/Edit/Delete Favorite
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Add contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 7392", homeNumber: vrsNum1, workNumber: "", mobileNumber: "")
		
		// Add Favorite
		ui.selectContact(name: "Automate 7392")
		ui.addToFavorites()
		
		// Verify Favorite
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 7392", type: "home")
		
		// Edit Contact
		ui.gotoSorensonContacts()
		ui.editContact(name: "Automate 7392")
		ui.removeContactNumber(type: "home")
		ui.enterContact(number: vrsNum2, type: "home")
		ui.saveContact()
		
		// Verify Favorite
		ui.gotoFavorites()
		ui.verifyFavorite(name: "Automate 7392", type: "home")
		
		// Delete Favorite and verify
		ui.deleteFavorite(name: "Automate 7392", type: "home")
		ui.verifyNoFavorite(name: "Automate 7392", type: "home")
		
		// Delete contact
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "Automate 7392")
	}
	
	func test_7753_FavoriteNumNameContact() {
		// Test Case: 7753 - Application will not crash when favoriting a contact with a number for the name
		
		// Delete Contacts
		ui.deleteAllContacts()
		
		// Setup
		ui.gotoSorensonContacts()
		ui.addContact(name: "7753 Automate", homeNumber: vrsNum1, workNumber: "", mobileNumber: vrsNum2)
		
		// favorite contact
		ui.selectContact(name: "7753 Automate")
		ui.addToFavorites(type: "home", phnum: vrsNum1)
		
		// Verify favorite
		ui.gotoFavorites()
		ui.verifyFavorite(name: "7753 Automate", type: "home")
		
		// cleanup
		ui.gotoSorensonContacts()
		ui.deleteContact(name: "7753 Automate")
	}
}
