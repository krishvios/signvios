//
//  SearchTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 3/30/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class SearchTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let cfg = UserConfig()
	let dutNum = UserConfig()["Phone"]!
	let dutPassword = UserConfig()["Password"]!
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
	
	func test_6026_6027_ClearingSearch() {
		// Test Case: 6026 - YNS: Verify the user can manually clear search in yelp
		// Test Case: 6027 - YNS: Verify the user can clear search in yelp
		
		// Search
		ui.yelp(search: "university of utah")
		ui.verifyYelp(result: "The University of Utah")

		// 6026 - Manual backspace to remove result string
		ui.selectSearchField()
		ui.pressBackspace(times: 6)
		ui.verifySearchField(value: "university o")

		// 6027 - Clear search  by clicking x
		ui.clickButton(name: "Clear text")
		ui.verifySearchField(value: "Search Contacts & Yelp")
		ui.selectCancel()
		ui.gotoHome()
	}
	
	func test_6203_YelpSearch() {
		// Test Case: 6203 - YNS: Yelp search uses GPS to determine location
		// Search
		ui.yelp(search: "university of utah")
		ui.verifyYelp(result: "The University of Utah")
		
		// Cleanup
		ui.clearYelpSearch()
		ui.gotoHome()
	}
	
	func test_6211_YelpAddContact() {
		// Test Case: 6211 - YNS: Add contact to phonebook from yelp search
		// Delete All Contacts
		ui.deleteAllContacts()
		
		// Search for pazzo
		// Search
		ui.yelp(search: "university of utah")
		
		// Add Contact
		ui.yelpAddContact(result: "The University of Utah")
		ui.clearYelpSearch()
		
		// Verify Contact
		ui.gotoSorensonContacts()
		ui.verifyContact(name: "The University of Utah")
		
		// Cleanup
		ui.deleteContact(name: "The University of Utah")
		ui.gotoHome()
	}
	
	func test_9507_ContactsSearch() {
		// Test Case: 9507 - Click search a few times doesn't crash

		// Setup
		ui.gotoSorensonContacts()
		
		// First Search Test
		ui.searchContactList(name: "Pizza")
		ui.clickButton(name: "Cancel")
		
		// Second Search Test
		ui.gotoFavorites()
		ui.searchContactList(name: "Pizza")
		ui.clickButton(name: "Cancel")
		
		// Third Search Test
		ui.gotoDeviceContacts()
		ui.searchContactList(name: "Pizza")
		ui.selectSearchField()
		ui.clickButton(name: "Search")
		ui.selectSearchField()
		ui.clickButton(name: "Search")
		ui.clickButton(name: "Cancel")
		
		// Clean Up
		ui.gotoSorensonContacts()
		ui.verifySearchField()
		ui.gotoHome()
	}
	
	func test_11163_CallYelpBusinessOnCall() {
		// Test Case: 11163 - While on a call, trying to call a business from yelp search
		let endpoint = endpointsClient.checkOut()
		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
		let yelpSearch = "university of utah"
		let yelpNum = "18015817200"
		let yelpLocation = "201 Presidents Cir, Salt Lake City"
		
		// Setup call with OEP
		ui.call(phnum: oepNum)
		con.answer()
		ui.waitForCallOptions()
		ui.selectCallOptions()
		ui.clickButton(name: "Make Another Call")
		ui.gotoCallTab(button: "Contacts")
		
		// Call from yelp search results
		ui.searchContactList(name: yelpSearch)
		ui.callFromYelp(location: yelpLocation, number: yelpNum)
		ui.verifyVRSCall()
		sleep(3)
		
		// Clean up
		ui.hangup()
		ui.hangup()
	}
}
