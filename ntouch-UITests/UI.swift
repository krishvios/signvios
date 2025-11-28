//
//  UI.swift
//  ntouch
//
//  Created by Mikael Leppanen on 10/6/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

import XCTest
import UIKit


let app = XCUIApplication()
let xctest = XCTestCase()
let cfg = UserConfig()
let inCallServiceApp = XCUIApplication(bundleIdentifier: "com.apple.InCallService")
let springboard = XCUIApplication(bundleIdentifier: "com.apple.springboard")
let safari = XCUIApplication(bundleIdentifier: "com.apple.mobilesafari")

extension XCUIElement {
	func forceTapElement() {
		// Sends a tap event to a hittable/unhittable element
		if self.isHittable {
			self.tap()
		}
		else {
			let coordinate: XCUICoordinate = self.coordinate(withNormalizedOffset: CGVector(dx: 0.0, dy: 0.0))
			coordinate.tap()
		}
	}
	
	func tapAtPosition(X: Int, Y: Int) {
		// Tap an x,y coordinate
		let coordinate = self.coordinate(withNormalizedOffset: CGVector(dx: 0, dy:0)).withOffset(CGVector(dx: X, dy: Y))
		coordinate.tap()
	}
}

class UI {
	
	/*
	Name: acceptAgreements()
	Description: Accept Agreements a number of agreements
	Parameters: count
	Example: acceptAgreements(count: 3)
	*/
	func acceptAgreements(count: Int) {
		// Wait for Agreement
		self.waitForAgreement()
		
		for _ in 1...count {
			self.clickButton(name: "Accept", delay: 5)
		}
	}
	
	/*
	Name: addCall(num:)
	Description: Add call to a number
	Parameters: num
	Example: addCall(num: "12345678910")
	*/
	func addCall(num: String) {
		testUtils.calleeNum = num
		
		// Select Add Call
		self.selectAddCall()
		
		// Select Dial
		app.windows.element(boundBy: 1).tabBars.buttons["Dial"].tap()
		
		// Dial Number
		self.dialInCall(phnum: num)
		
		// Press the Call button
		app.windows.element(boundBy: 1).buttons["Call"].tap()
		sleep(3)
	}
	
	/*
	Name: addCallFromContacts(num:)
	Description: Add call from Contacts
	Parameters: name
	Example: addCallFromContacts(name: "Automate 5923")
	*/
	func addCallFromContacts(name: String) {
		// Select Add Call
		self.selectAddCall()
		
		// Select Contacts
		self.gotoCallTab(button: "Contacts")
		
		// Select Contact
		self.selectContact(name: name)
		sleep(3)
	}
	
	/*
	Name: addCallFromHistory(num:)
	Description: Add call from call history
	Parameters: num
	Example: addCallFromHistory(num: "12345678910")
	*/
	func addCallFromHistory(num: String) {
		testUtils.calleeNum = num
		
		// Select Add Call
		self.selectAddCall()
		
		// Select Call History
		self.gotoCallTab(button: "History")
		
		// Call item from Call History
		let phnum = self.formatPhoneNumber(num: num)
		app.windows.element(boundBy: 1).tables.cells.containing(.staticText, identifier: phnum).element(boundBy: 0).tap()
		sleep(3)
	}
	
	/*
	Name: addCompanyContact(name:homeNumber:workNumber:mobileNumber:)
	Description: Add a Company contact with Home, Work, and/or Mobile Number(s)
	Parameters: name
	homeNumber (optional)
	workNumber (optional)
	mobileNumber (optional)
	Example: addCompanyContact(name: "AutoCo ", homeNumber: "", workNumber: "15553334444", mobileNumber: "")
	*/
	func addCompanyContact(name: String, homeNumber: String, workNumber: String, mobileNumber: String) {
		// Select the Add button
		self.clickButton(name: "Add", query: app.navigationBars.buttons, delay: 3)
		
		// Enter the Contact Name
		self.enterCompany(name: name)
		
		// Enter Home Number
		if (!homeNumber.isEmpty) {
			self.enterContact(number: homeNumber, type: "home")
		}
		
		// Enter Work Number
		if (!workNumber.isEmpty) {
			self.enterContact(number: workNumber, type: "work")
		}
		
		// Enter Mobile Number
		if (!mobileNumber.isEmpty) {
			self.enterContact(number: mobileNumber, type: "mobile")
		}
		
		// Select the Done button
		self.saveContact()
	}
	
	/*
	Name: addContact(name:homeNumber:workNumber:mobileNumber:)
	Description: Add a contact with Home, Work, and/or Mobile Number(s)
	Parameters: name
				homeNumber (optional)
				workNumber (optional)
				mobileNumber (optional)
	Example: addContact(name:"Automate 5395", homeNumber:"15556663333", workNumber:"", mobileNumber:"")
	*/
	func addContact(name: String, homeNumber: String, workNumber: String, mobileNumber: String) {
		// Check if contact already exists and delete it
		if app.staticTexts[name].exists {
			self.deleteContact(name: name)
		}
		
		// Add Contact
		self.addContactInfo(name: name, companyName:"", homeNumber: homeNumber, workNumber: workNumber, mobileNumber: mobileNumber)
		
		// Select the Done button
		self.saveContact()
	}
	
	/*
	Name: addContact(name:companyName:homeNumber:workNumber:mobileNumber:)
	Description: Add a contact with company name, Home, Work, and/or Mobile Number(s)
	Parameters: name
	companyName (optional)
	homeNumber (optional)
	workNumber (optional)
	mobileNumber (optional)
	Example: addContact(name:"Automate 5395", companyName: "Sorenson", homeNumber:"15556663333", workNumber:"", mobileNumber:"")
	*/
	func addContact(name: String, companyName: String, homeNumber: String, workNumber: String, mobileNumber: String) {
		// Add Contact
		self.addContactInfo(name: name, companyName: companyName, homeNumber: homeNumber, workNumber: workNumber, mobileNumber: mobileNumber)
		
		// Select the Done button
		self.saveContact()
	}
	
	/*
	Name: addContactFromDialer(num:contactName:)
	Description: Add contact from Dialer
	Parameters: num
				contactName
	Example: addContactFromHistory(num: "1801575669", contactName: "Automate 0603")
	*/
	func addContactFromDialer(num: String, contactName: String) {
		// Go to dialer
		self.gotoHome()
		
		// Clear dialer
		self.clearDialer()
		
		// Dial number
		self.dial(phnum: num)
		
		// Select the Add button
		self.selectDialerAddBtn()
		
		// Select Create New Contact
		self.clickButton(name: "Create New Contact", query: app.sheets.buttons, delay: 3)
		
		// Enter the Contact Name
		self.enterContact(name: contactName)
		
		// Select the Done button
		self.saveContact()
	}
	
	/*
	Name: addContactFromHistory(num:contactName:)
	Description: Add contact from Call History
	Parameters: num
	contactName
	Example: addContactFromHistory(num: "18015550603", contactName: "Automate 0603")
	*/
	func addContactFromHistory(num: String, contactName: String) {
		self.selectContactFromHistory(num: num)
		self.clickStaticText(text: "Add Contact", query: app.tables.cells.staticTexts)
		
		// Clear the Contact Name field and Enter the Contact Name
		if contactName != "" {
			self.clickTextField(name: "Name", query: app.tables.textFields)
			if app.tables.cells.buttons["Clear text"].exists {
				app.tables.cells.buttons["Clear text"].tap()
			}
			self.enterContact(name: contactName)
		}
		
		// Select the Done button
		self.saveContact()
	}
	
	/*
	Name: addContactFromHistory(coreName:contactName:)
	Description: Add contact from Call History
	Parameters: coreName
				contactName
	Example: addContactFromHistory(coreName: "AutoNVP", contactName: "Automate 0603")
	*/
	func addContactFromHistory(coreName: String, contactName: String) {
		// Find Call History by Core Name and select Create New Contact in the Add Contact options
		app.tables.cells.containing(.staticText, identifier: coreName).element(boundBy: 0).buttons["More Info"].tap()
		self.clickStaticText(text: "Add Contact", query: app.tables.cells.staticTexts, delay: 5)
		
		// Clear the Contact Name field and Enter the Contact Name
		if contactName != "" {
			self.clickTextField(name: "Name", query: app.tables.textFields)
			self.clickButton(name: "Clear text", query: app.tables.textFields["Name"].buttons)
			self.enterContact(name: contactName)
		}
		
		// Select the Done button
		self.saveContact()
	}
	
	/*
	Name: addContactFromSignMail(phnum:contactName:)
	Description: Add contact from SignMail
	Parameters: phnum
				contactName
	Example: addContactFromSignMail(phnum: "18323883030", contactName: "Automate 1527")
	*/
	func addContactFromSignMail(phnum: String, contactName: String) {
		// Find SignMail by Phone number and select options button
		let formatNum = self.formatPhoneNumber(num: phnum)
		app.tables.cells.containing(.staticText, identifier: formatNum).element(boundBy: 0).buttons["More Info"].tap()
		sleep(1)
		
		// Select Add Contact
		self.clickStaticText(text: "Add Contact", query: app.tables.cells.staticTexts)
		
		// Clear the Contact Name field and Enter the Contact Name
		if contactName != "" {
			self.clickTextField(name: "Name", query: app.tables.textFields)
			self.clickButton(name: "Clear text", query: app.tables.textFields["Name"].buttons)
			self.enterContact(name: contactName)
		}
		
		// Enter num
		if !app.tables.cells["HomeNumberCell"].textFields[phnum].exists {
			self.enterContact(number: formatNum, type: "home")
		}
		
		// Select the Done button
		self.saveContact()
	}
	
	/*
	Name: addContactInfo(name:homeNumber:workNumber:mobileNumber:)
	Description: Add a contact with Home, Work, and/or Mobile Number(s)
	Parameters: name
	homeNumber (optional)
	workNumber (optional)
	mobileNumber (optional)
	Example: addContactInfo(name:"Automate 5395", homeNumber:"15556663333", workNumber:"", mobileNumber:"")
	*/
	func addContactInfo(name: String, companyName: String, homeNumber: String, workNumber: String, mobileNumber: String) {
		// Select the Add button
		self.clickButton(name: "Add", query: app.navigationBars.buttons)
		
		// Enter the Contact Name
		if (!name.isEmpty) {
			self.enterContact(name: name)
		}
		
		// Enter Company Name
		if (!companyName.isEmpty) {
			self.enterCompany(name: companyName)
		}
		
		// Enter Home Number
		if (!homeNumber.isEmpty) {
			self.enterContact(number: homeNumber, type: "home")
		}
		
		// Enter Work Number
		if (!workNumber.isEmpty) {
			self.enterContact(number: workNumber, type: "work")
		}
		
		// Enter Mobile Number
		if (!mobileNumber.isEmpty) {
			self.enterContact(number: mobileNumber, type: "mobile")
		}
	}
	
	/*
	Name: addContactWithFavorite(name:homeNumber:workNumber:mobileNumber:)
	Description: Add a contact and favorite with Home, Work, and/or Mobile Number(s)
	Parameters: name
	homeNumber (optional)
	workNumber (optional)
	mobileNumber (optional)
	Example: addContactWithFavorite(name:"Automate 5395", homeNumber:"15556663333", workNumber:"", mobileNumber:"")
	*/
	func addContactWithFavorite(name: String, homeNumber: String, workNumber: String, mobileNumber: String) {
		// Add Contact
		self.addContactInfo(name: name, companyName:"", homeNumber: homeNumber, workNumber: workNumber, mobileNumber: mobileNumber)
		
		if homeNumber != "" {
			let num = formatPhoneNumber(num: homeNumber)
			let typeAndNumber = NSString(format: "%@ %@", "home", num)
			self.addToFavorites()
			if app.sheets.buttons[typeAndNumber as String].exists {
				app.sheets.buttons[typeAndNumber as String].tap()
			}
		}
		
		if workNumber != "" {
			let num = formatPhoneNumber(num: workNumber)
			let typeAndNumber = NSString(format: "%@ %@", "work", num)
			self.addToFavorites()
			if app.sheets.buttons[typeAndNumber as String].exists {
				app.sheets.buttons[typeAndNumber as String].tap()
			}
		}
		
		if mobileNumber != "" {
			let num = formatPhoneNumber(num: mobileNumber)
			let typeAndNumber = NSString(format: "%@ %@", "mobile", num)
			self.addToFavorites()
			if app.sheets.buttons[typeAndNumber as String].exists {
				app.sheets.buttons[typeAndNumber as String].tap()
			}
		}
		
		// Select the Done button
		self.saveContact()
	}
	
	/*
	Name: addNumToContactFromDialer(phnum:contactName:)
	Description: Add a number to an existing contact from dialer
	Parameters: phnum
				name
				type
	Example: addNumToContactFromDialer(phnum: "18015757889", name: "Automate 1996", type: "Work")
	*/
	func addNumToContactFromDialer(phnum: String, name: String, type: String) {
		// Enter the phone number
		self.dial(phnum: phnum)
		
		// Select the add Button
		self.clickButton(name: "Add Contact")
		
		// Select Add to Existing Contact and select contact
		self.clickButton(name: "Add to Existing Contact", query: app.sheets.buttons)
		self.selectContact(name: name)
		
		// Add type
		self.clickButton(name: "\(type) ", query: app.sheets.buttons)
	}
	
	/*
	Name: addSavedText(text:)
	Description: Add a Saved Text item
	Parameters: text
	Example: addSavedText(text: "1234")
	*/
	func addSavedText(text: String) {
		// Select Add Saved Text
		self.selectAddSavedText()
		
		// Enter text
		let lastTextField = app.textFields.count - 1
		app.textFields.element(boundBy: lastTextField).tap()
		sleep(3)
		app.textFields.element(boundBy: lastTextField).typeText(text)
		
		// Select the Done button
		self.clickButton(name: "Done", query: app.navigationBars.buttons, delay: 3)
	}
	
	/*
	Name: addToFavorites()
	Description: Select the Add to Favorties button on the Contact screen
	Parameters:	 none
	Example: addToFavorites()
	*/
	func addToFavorites() {
		// Select Add to Favorites
		self.clickStaticText(text: "Add To Favorites", query: app.tables.cells.staticTexts, delay: 3)
	}
	
	/*
	Name: addToFavorites(type:phnum:)
	Description: Select the Add to Favorties button on the Contact screen
	Parameters: type ("home", "work", or "mobile")
				phnum
	Example: addToFavorites(type: "home", phnum: "18015757669")
	*/
	func addToFavorites(type: String, phnum: String) {
		// Select Add to Favorites
		self.addToFavorites()
		
		// Select type and number for multiple numbers
		let num = formatPhoneNumber(num: phnum)
		let typeAndNumber = NSString(format: "%@: %@", type, num)
		self.clickButton(name: typeAndNumber as String, query: app.sheets.buttons)
	}
	
	/*
	Name: blockContact(name:)
	Description: Block a contact by name
	Parameters: name
	Example: blockContact(contactName: "Automate 5395")
	*/
	func blockContact(name: String) {
		// Select Contact
		self.selectContact(name: name)
		
		// Block Contact if not already blocked
		if app.tables.staticTexts["Block Contact"].exists {
			app.tables.staticTexts["Block Contact"].tap()
			sleep(2)
		}
	}
	
	/*
	Name: blockContactFromHistory(name:)
	Description: block a contact through Call History
	Parameters:name
	Example: blockContactFromHistory(name: "Automate 0603")
	*/
	func blockContactFromHistory(name: String) {
		// Find Call History by Name and select the info button
		app.tables.cells.containing(.staticText, identifier:name).children(matching: .button).element(boundBy: 0).tap()
		sleep(1)
		
		// Block Contact
		self.clickStaticText(text: "Block Contact", query: app.tables.staticTexts, delay: 2)
	}
	
	/*
	Name: blockContactFromSignMail(name:)
	Description: block a contact through SignMail
	Parameters:phnum, contactName
	Example: blockContactFromSignMail("18015555555")
	*/
	func blockContactFromSignMail(phnum: String) {
		// Select the message by number
		self.selectSignMail(phnum: phnum)
		
		if (app.staticTexts["Block Caller"].exists) {
			// Select Block Caller if caller is not already blocked
			self.clickStaticText(text: "Block Caller")
		}

	}
	
	/*
	Name: blockNumberFromHistory(number:)
	Description: block a non-contact through Call History
	Parameters:number
	Example: blockNumberFromHistory(number: "18018083966")
	*/
	func blockNumberFromHistory(number: String) {
		let num = self.formatPhoneNumber(num: number)
		// Find Call History item by number and select the info button
		app.tables.cells.containing(.staticText, identifier:num).children(matching: .button).element(boundBy: 0).tap()
		sleep(1)
		
		// Block
		if app.staticTexts["Block Caller"].exists {
			self.clickStaticText(text: "Block Caller", delay: 2)
		}
		
	}
	
	/*
	Name: blockFavorite(name:num:)
	Description: Block Favorite by Name and Number
	Parameters: name
				num
	Example: blockFavorite(faveName: "Automate 5437", favNum: "18015757669")
	*/
	func blockFavorite(name: String, num: String) {
		// Block Number
		self.blockNumber(name: name, num: num)
		
		// Select Block
		self.clickButton(name: "Block", query: app.alerts.buttons)
	}
	
	/*
	Name: blockNumber(name:num:)
	Description: Block a number
	Parameters: name
				num
	Example: blockNumber(name: "Automate 5437", num: "18015757669")
	*/
	func blockNumber(name: String, num: String) {
		// Select Add Blocked Number
		self.clickButton(name: "Add", query: app.navigationBars.buttons)
		
		// Enter Name
		if (!name.isEmpty) {
			app.tables.cells.containing(.staticText, identifier:"Description").element.tap()
			sleep(3)
			app.tables.cells.containing(.staticText, identifier:"Description").element.typeText(name)
			sleep(1)
		}
		
		// Enter Number
		if (!num.isEmpty) {
			app.tables.cells.containing(.staticText, identifier:"Phone Number").element.tap()
			sleep(3)
			app.tables.cells.containing(.staticText, identifier:"Phone Number").element.typeText(num)
			sleep(1)
		}
		
		// Select the Done button
		self.clickButton(name: "Done", query: app.navigationBars.buttons, delay: 3)
	}
	
	/*
	Name: call(:phnum)
	Description: Place a call using the Dialer
	Parameters: phnum (must not be formatted)
	Example: call(phnum: "18015757669")
	*/
	func call(phnum: String) {
		testUtils.calleeNum = phnum
		
		// Go to dialer
		self.gotoHome()
		
		// Clear dialer
		self.clearDialer()
		
		// Dial number
		self.dial(phnum: phnum)
		self.selectCallButton()
	}
	
	/*
	Name: callCIRDismissible
	Description: Call CIR from Dismissible CIR
	Parameters: none
	Example: callCIRDismissible()
	*/
	func callCIRDismissible() {
		// Wait for Dismissible CIR
		self.waitForCIRDismissible()
		
		// Select Call CIR
		self.clickButton(name: "Call Customer Care", delay: 3)
	}
	
	/*
	Name: callCIRLoggedOut
	Description: Place a CIR call logged out
	Parameters: none
	Example: callCIRLoggedOut()
	*/
	func callCIRLoggedOut() {
		// iPad
		if UIDevice.current.userInterfaceIdiom == .pad {
			// Count Help instances
			let helpCount = app.tables.cells.staticTexts.matching(identifier: "Support").count
			let count : Int = Int(helpCount);
			
			// Select the hittable Help
			for index in 0 ..< count {
				let i = Int(index)
				let text = app.tables.cells.staticTexts.matching(identifier: "Support").element(boundBy: i)
				if text.isHittable == true {
					app.tables.cells.staticTexts.matching(identifier: "Support").element(boundBy: i).tap()
					break
				}
			}
		}
			
		// iPhone or iPod
		else {
			// Select Help
			self.clickStaticText(text: "Support", query: app.tables.staticTexts)
		}
		
		// Select Call Customer Care
		self.clickStaticText(text: "Call Customer Care", query: app.tables.staticTexts, delay: 3)
	}
	
	/*
	Name: callContact(name:type:)
	Description: Call a Contact by Name and Type
	Parameters: name
				type ("home", "work", or "mobile")
	Example: callContact(name: "Automate 7416", type:"home")
	*/
	func callContact(name: String, type: String) {
		// Select Contact
		self.selectContact(name: name)
		
		// Select the number type to call
		self.clickStaticText(text: type, delay: 3)
	}
	
	/*
	Name: callFavorite(name:type:)
	Description: Call a Favorite by Favorite Name and Type
	Parameters: name
				type ("home", "work", or "mobile")
	Example: callFavorite(name: "Automate 5395", type: "home")
	*/
	func callFavorite(name: String, type: String) {
		// Select the Favorite to place a call
		app.tables.cells.containing(.staticText, identifier:type).staticTexts[name].tap()
		sleep(3)
	}
	
	/*
	Name: callFromHistory(num:)
	Description: Place a call from the Call History
	Parameters: num
	Example: callFromHistory(num: cfg["nvpNum"]!)
	*/
	func callFromHistory(num: String) {
		testUtils.calleeNum = num
		
		// Call item from Call History
		let phnum = self.formatPhoneNumber(num: num)
		
		// P2P Call
		if (app.tables.cells.staticTexts[phnum].exists) {
			app.tables.cells.containing(.staticText, identifier:phnum).element(boundBy: 0).tap()
		}
		// VRS Call
		else {
			app.tables.cells.containing(.staticText, identifier: phnum + " / VRS").element.tap()
		}
		sleep(3)
	}
	
	/*
	Name: callFromHistory(name:num:)
	Description: Place a call from the Call History
	Parameters: name
				num
	Example: callFromHistory(name: "Automate 931", num: cfg["nvpNum"]!)
	*/
	func callFromHistory(name: String, num: String) {
		testUtils.calleeNum = num
		
		// Call item from Call History
		let phnum = self.formatPhoneNumber(num: num)
		app.tables.cells.containing(.staticText, identifier: phnum).staticTexts[name].tap()
		sleep(3)
	}
	
	/*
	Name: callFromYelp(location:call:)
	Description: Call from search result on yelp
	Parameters: location, number
	Example: callFromYelp(location:"262 N Central Ave, Farmington", call: "+1 (801) 451-1276")
	*/
	func callFromYelp(location: String, number: String) {
		let phnum = self.formatPhoneNumber(num: number)
		let locationText = self.waitFor(query: app.staticTexts, value: location)
		locationText.tap()
		self.clickButton(name: "Call +\(phnum)")
	}
	
	/*
	Name: callFromSignMail()
	Description: Return call during SignMail playback
	Parameters: none
	Example: callFromSignMail()
	*/
	func callFromSignMail() {
		// Select Call
		self.clickButton(name: "Call", delay: 3)
	}
	
	/*
	Name: callFromSignMail(phnum:)
	Description: Return call from SignMail by phone number
	Parameters: phnum
	Example: callFromSignMail(phnum: "19323883030")
	*/
	func callFromSignMail(phnum: String) {
		testUtils.calleeNum = phnum
		
		// Select SignMail
		self.selectSignMail(phnum: phnum)
		
		// Select phone icon
		self.clickButton(name: "icon phone", delay: 2)
	}
	
	/*
	Name: callWaiting(response:)
	Description: Respond to Call Waiting
	Parameters: response ("Decline", "Hold & Answer", "Hangup & Answer")
	Example: callWaiting(response: "Hold & Answer")
	*/
	func callWaiting(response: String) {
		// Wait for Incoming Call
		self.waitForCallWaiting()

		// Select Response
		self.clickButton(name: response)
		if response != "Decline" {
			self.waitForCallOptions()
		}
		sleep(5)
	}
	
	/*
	Name: cantSendSignMail(response:)
	Description: Verify Can't Send SignMail and select Cancel or Call Directly
	Parameters: response ("Cancel", "Call Directly")
	Example: cantSendSignMail("Cancel")
	*/
	func cantSendSignMail(response: String) {
		let cantSendSignMail = self.waitFor(query: app.alerts.staticTexts, value: "Cannot send SignMail")
		XCTAssertTrue(cantSendSignMail.exists, "Cannot send SignMail alert did not appear")
		self.clickButton(name: response, query: app.alerts.buttons)
	}
	
	/*
	Name: changePassword(old:new:confirm)
	Description: Change password
	Parameters: old, new, confirm
	Example: changePassword("Cancel")
	*/
	func changePassword(old: String, new: String, confirm: String) {
		// Go to Settings
		self.gotoPersonalInfo()
		
		// Select Change Password
		self.selectChangePassword()
		
		// Enter Old Password
		self.enterOldPassword(oldPassword: old)
		
		// Enter New Password
		self.enterNewPassword(newPassword: new)
		
		// Confirm Password
		self.enterNewPasswordConfirm(newPassword: confirm)
		
		// Select Change Password
		self.selectChangePassword()
		
		// Select OK and go to home
		self.clickButton(name: "OK", query: app.alerts["New Password Saved"].buttons)
		self.gotoHome()
	}
	
	/*
	Name: choosePhoto()
	Description: Select Choose Photo
	Parameters: none
	Example: choosePhoto()
	*/
	func choosePhoto() {
		// Select Edit photo
		self.selectEditPhoto()
		
		// Select Choose Photo
		self.clickButton(name: "Choose Photo", query: app.sheets.buttons, delay: 3)
	}
	
	/*
	Name: choosePhotoCancel()
	Description: select the Cancel button after selecting Choose Photo
	Parameters: none
	Example: choosePhotoCancel()
	*/
	func choosePhotoCancel() {
		app.navigationBars["Photos"].buttons["Cancel"].tap()
		sleep(3)
	}
	
	/*
	Name: cleanupAgreements()
	Description: Clear existing agreements
	Parameters: none
	Example: cleanupAgreements()
	*/
	func cleanupAgreements() {
		// Wait for Agreement
		if app.navigationBars["DynamicProviderAgreementView"].waitForExistence(timeout: 10) {
			while (app.buttons["Accept"].exists) {
				// Select Accept
				self.clickButton(name: "Accept", delay: 3)
			}
		}
	}
	
	/*
	Name: clearAlert()
	Description: Clear notification alerts from phone (3 variants included)
	Parameters:	none
	Example: clearAlert()
	*/
	func clearAlert() {
		// For OS 12 or earlier, this method will not work
		xctest.addUIInterruptionMonitor(withDescription: "System Dialog") {
		(alert) -> Bool in
		let okButton = alert.buttons["OK"]
		if okButton.exists {
			okButton.tap()
		}

		let allowButton = alert.buttons["Allow"]
		if allowButton.exists {
			allowButton.tap()
		  }
			
		let alwaydAllowButton = alert.buttons["Allow While Using App"]
		if alwaydAllowButton.exists {
			alwaydAllowButton.tap()
		}
		return true
		}
		self.selectTop()
	}
	
	/*
	Name: cleanupDismissibleCIR()
	Description: Clear Dismissible CIR
	Parameters: none
	Example: cleanupDismissibleCIR()
	*/
	func cleanupDismissibleCIR() {
		// Wait for Agreement
		if app.buttons["Call Customer Care"].waitForExistence(timeout: 10) {
			self.callCIRDismissible()
			self.hangup()
		}
	}
	
	/*
	Name: cleanupFccRegistration()
	Description: Clear FCC Registration
	Parameters: none
	Example: cleanupFccRegistration()
	*/
	func cleanupFccRegistration() {
		// Wait for Agreement
		if app.navigationBars["FCC Registration"].waitForExistence(timeout: 10) {
			self.selectNoSSN(setting: 1)
			self.selectBirthYear()
			self.selectSubmitRegistration()
		}
	}
	
	/*
	Name: clearAndEnterText(text:)
	Description: Entering text into the text field
	Parameters: text
	Example: clearAndEnterText(text: "greeting")
	*/
	func clearAndEnterText(text: String) {
		// Select text, and clear text if needed
		let textView = app.textViews.element(boundBy: 0)
		if app.textViews["Enter text here."].exists {
			app.textViews["Enter text here."].tap()
		}
		else {
			textView.tap()
			textView.press(forDuration: 1.2)
			self.clickMenuItem(name: "Select All")
			app.keys["delete"].tap()
		}
		
		// Enter text
		textView.typeText(text)
	}
	
	/*
	Name: clearDialer
	Description: Clear the dialer
	Parameters: none
	Example: clearDialer()
	*/
	func clearDialer() {
		// If backspace is present, long press to clear
		if app.buttons["Backspace"].exists {
			app.buttons["Backspace"].press(forDuration: 2)
		}
		
//		// If backspace is present, tap until cleared
//		if app.buttons["Backspace"].exists {
//			while app.buttons["Backspace"].exists {
//				app.buttons["Backspace"].tap()
//			}
//		}
	}
	
	/*
	Name: clearYelpSearch()
	Description: Clear Yelp Search
	Parameters:	field, timeout, delay
	Example: clearYelpSearch()
	*/
	func clearSearch(field: String = "Search", timeout: Double = 30, delay: UInt32 = 1) {
		var searchField: XCUIElement
		searchField = self.waitFor(query: app.searchFields, value: field, timeout: timeout)
		searchField.tap()
		self.clickButton(name: "Cancel", delay: delay)
	}
	
	/*
	Name: clearSharedText()
	Description: Clear Shared Text
	Parameters:	none
	Example: clearSharedText()
	*/
	func clearSharedText() {
		if app.buttons["Clear All"].exists {
			app.buttons["Clear All"].tap()
		}
		else {
			self.share()
			app.buttons["Clear All"].tap()
		}
		sleep(1)
		
		// close keyboard
		self.closeShareTextKeyboard()
	}
	
	/*
	Name: clearYelpSearch()
	Description: Clear Yelp Search
	Parameters:	none
	Example: clearYelpSearch()
	*/
	func clearYelpSearch() {
		self.clearSearch(field: "Search Contacts & Yelp")
	}
	
	/*
	Name: clickEndEnterSearch(field:text:timeout:delay:)
	Description: Click and enter text into a search field
	Parameters: field, text, timeout, delay,
	Example: clickEndEnterSearch(field: "Search", text: "gibberish")
	*/
	func clickAndEnterSearch(field: String, text: String, timeout: Double = 30, delay: UInt32 = 5) {
		let searchField = self.waitFor(query: app.searchFields, value: field, timeout: timeout)
		searchField.tap()
		searchField.typeText(text)
		if #available(iOS 14.0, *) {
			self.clickButton(name: "search")
		}
		else {
			self.clickButton(name: "Search")
		}
		sleep(delay)
	}
	
	/*
	Name: clickAndEnterText(field:text:clear:)
	Description: Click and enter text into a text field
	Parameters: field, text, clear
	Example: clickAndEnterText(field: "First Name", text: "first")
	*/
	func clickAndEnterText(field: String, text: String, clear: Bool = false) {
		// Click text field and enter text
		self.clickTextField(name: field)
		if clear {
			self.clickButton(name: "Clear text")
		}
		self.enterText(field: field, text: text)
	}
	
	/*
	Name: clickButton(name:query:timeout:delay:doubleClick:)
	Description: Click a button
	Parameters: name, query, timeout, delay, doubleClick
	Example: clickButton(name: "Dialpad")
	*/
	func clickButton(name: String, query: XCUIElementQuery = app.buttons, timeout: Double = 30, delay: UInt32 = 1, doubleClick: Bool = false) {
		let button = self.waitFor(query: query, value: name, timeout: timeout)
		if doubleClick {
			button.doubleTap()
		}
		else {
			button.tap()
		}
		sleep(delay)
	}
	
	/*
	Name: clickMenuItem(name:query:timeout:delay:doubleClick:)
	Description: Click a menu item
	Parameters: name, query, timeout, delay, doubleClick
	Example: clickMenuItem(name: "Dialpad")
	*/
	func clickMenuItem(name: String, query: XCUIElementQuery = app.menuItems, timeout: Double = 30, delay: UInt32 = 1, doubleClick: Bool = false) {
		let menuItem = self.waitFor(query: query, value: name, timeout: timeout)
		if doubleClick {
			menuItem.doubleTap()
		}
		else {
			menuItem.tap()
		}
		sleep(delay)
	}
	
	/*
	Name: clickSegmentedControlButton(name:query:timeout:delay:)
	Description: Click a segmented control button
	Parameters: name, query, timeout, delay
	Example: clickSegmentedControlButton(name: "Contacts")
	*/
	func clickSegmentedControlButton(name: String, query: XCUIElementQuery = app.segmentedControls.buttons, timeout: Double = 30, delay: UInt32 = 1) {
		let controlBtn = self.waitFor(query: query, value: name, timeout: timeout)
		controlBtn.tap()
		sleep(delay)
		XCTAssert(controlBtn.isSelected, "Failed to select segmented control button: \(name)")
	}
	
	/*
	Name: clickStaticText(text:query:timeout:delay:longClick:)
	Description: Click static text
	Parameters: text, query, timeout, delay, longClick
	Example: clickStaticText(text: "Select All")
	*/
	func clickStaticText(text: String, query: XCUIElementQuery = app.staticTexts, timeout: Double = 30, delay: UInt32 = 1, longClick: Bool = false) {
		let staticText = self.waitFor(query: query, value: text, timeout: timeout)
		if longClick {
			staticText.press(forDuration: 2)
		}
		else {
			staticText.tap()
		}
		sleep(delay)
	}
	
	/*
	Name: clickTabBarButton(name:query:timeout:delay:doubleClick:)
	Description: Click a tab bar button
	Parameters: name, query, timeout, delay, doubleClick
	Example: clickTabBarButton(name: "Dialpad")
	*/
	func clickTabBarButton(name: String, query: XCUIElementQuery = app.tabBars.buttons, timeout: Double = 30, delay: UInt32 = 1, doubleClick: Bool = false) {
		let tabBarBtn = self.waitFor(query: query, value: name, timeout: timeout)
		if doubleClick {
			tabBarBtn.doubleTap()
		}
		else {
			tabBarBtn.tap()
		}
		sleep(delay)
		XCTAssert(tabBarBtn.isSelected, "Failed to select tab bar button: \(name)")
	}
	
	/*
	Name: clickTextField(name:query:timeout:delay:doubleClick:longClick)
	Description: Click a text field
	Parameters: name, query, timeout, delay, doubleClick, longClick
	Example: clickTextField(name: "Username")
	*/
	func clickTextField(name: String, query: XCUIElementQuery = app.textFields, timeout: Double = 30, delay: UInt32 = 1, doubleClick: Bool = false, longClick: Bool = false) {
		let textField = self.waitFor(query: query, value: name, timeout: timeout)
		if doubleClick {
			textField.doubleTap()
		}
		else if longClick {
			textField.press(forDuration: 2)
		}
		else {
			textField.tap()
		}
		sleep(delay)
	}
	
	/*
	Name: closeShareTextKeyboard()
	Description: Close Share Text keyboard
	Parameters: none
	Example: closeShareTextKeyboard()
	*/
	func closeShareTextKeyboard() {
		if #available(iOS 15.0, *) {
			// Select the top 3rd to dismiss keyboard instead to avoid landscape issue on OS 15
			self.selectTop3rd()
			sleep(1)
		} else {
			// Swipe down to close keyboard
			app.swipeDown()
			sleep(1)
		}
	}
	
	/*
	Name: confirmSignMailSend(response:)
	Description: Send, Record Again, or Exit SignMail recording
	Parameters: response ("Send", "Rerecord", "Exit")
	Example: confirmSignMailSend(response: "Exit")
	*/
	func confirmSignMailSend(response: String) {
		// Wait for Confirm
		waitForConfirmSignMailSend()
		
		// Select Response
		switch response {
		case "Send":
			self.clickButton(name: "Send", query: app.sheets["Confirm SignMail Send"].buttons, delay: 5)
		case "Rerecord":
			self.clickButton(name: "Record Again", query: app.sheets["Confirm SignMail Send"].buttons, delay: 5)
		case "Exit":
			self.clickButton(name: "Exit", query: app.sheets["Confirm SignMail Send"].buttons, delay: 5)
		default:
			XCTFail("Incorrect response string")
		}
	}
	
	/*
	Name: confirmSignMailSendDoubleTap(response:)
	Description: Double tap Send, Record Again, or Exit SignMail recording
	Parameters: response ("Send", "Rerecord", "Exit")
	Example: confirmSignMailSendDoubleTap(response: "Send")
	*/
	func confirmSignMailSendDoubleTap(response: String) {
		// Wait for Confirm
		waitForConfirmSignMailSend()
		
		// Select Response
		switch response {
		case "Send":
			app.sheets["Confirm SignMail Send"].buttons["Send"].doubleTap()
			
		case "Rerecord":
			app.sheets["Confirm SignMail Send"].buttons["Record Again"].doubleTap()
		
		case "Exit":
			app.sheets["Confirm SignMail Send"].buttons["Exit"].doubleTap()
		default:
			XCTFail("Incorrect response string")
		}
		sleep(5)
	}
	
	/*
	Name: createGreeting(time:)
	Description: Record and Save a Greeting
	Parameters: time
	Example: createGreeting(time: 30)
	*/
	func createGreeting(time: UInt32) {
		// Record Greeting
		self.recordGreeting(time: time)
		
		// Save Greeting
		self.greetingRecorded(response: "Save")
		
	}
	
	/*
	Name: createGreeting(time:text:)
	Description: Record and Save a Greeting with text
	Parameters: time
				text
	Example: createGreeting(time: 30, text: "greeting")
	*/
	func createGreeting(time: UInt32, text: String) {
		// Record Greeting
		self.recordGreeting(time: time, text: text)
		
		// Save Greeting
		self.greetingRecorded(response: "Save")
		
		// Verify text
		self.verifyGreeting(text: text)
	}
	
	/*
	Name: createTextGreeting(text:)
	Description: Create a text only greeting
	Parameters: text, alternateSaveButton
	Example: createTextGreeting(text: "Test greeting")
	*/
	func createTextGreeting(text: String, alternateSaveButton: Bool = false) {
		// Select edit and personal signmail type
		editAndSelectPersonalRecordType(type: "Text Only")
		
		clearAndEnterText(text: text)
		app.swipeDown() // refresh ui
		self.clickButton(name: "Done")
		if alternateSaveButton {
			// Select the top right Save button
			app.buttons.matching(identifier: "Save").element(boundBy: 1).tap()
			sleep(3)
		} else {
			// Select the Save button
			self.clickButton(name: "Save", query: app.navigationBars.buttons, delay: 3)
		}

		// Verify text
		self.verifyGreeting(text: text)
	}
	
	/*
	Name: deleteAllCallHistory()
	Description: Delete all Call History
	Parameters: none
	Example: deleteAllCallHistory()
	*/
	func deleteAllCallHistory() {
		// Check for No Recent Calls
		if (!app.tables.staticTexts["You don't have any calls."].exists) {
			// Select Edit
			self.clickButton(name: "Edit", query: app.navigationBars.buttons, delay: 3)
			
			// Select Delete All
			self.clickButton(name: "Delete", query: app.navigationBars.buttons)
			let clearAllCallHistory = self.waitFor(query: app.buttons, value: "Clear All Call History")
			clearAllCallHistory.tap()
			
			// Wait for No Calls
			let noCalls = self.waitFor(query: app.tables.staticTexts, value: "You don't have any calls.", timeout: 60)
			XCTAssertTrue(noCalls.exists, "Failed to delete all Call History")
			
			// Select Done
			self.clickButton(name: "Done", query: app.navigationBars.buttons)
		}
	}
	
	/*
	Name: deleteAllContacts()
	Description: Delete all Contacts (enableKeycodes must have been called to funciton)
	Parameters: none
	Example: deleteAllContacts()
	*/
	func deleteAllContacts() {
		// dialcode 335266 (delcon)
		self.enableKeyCodes()
		app.buttons["3"].doubleTap()
		app.buttons["5"].tap()
		app.buttons["2"].tap()
		app.buttons["6"].doubleTap()
		app.buttons["Call"].tap()
		self.clickButton(name: "OK")
	}
	
	/*
	Name: deleteAllFavorites()
	Description: Delete all Favorites
	Parameters: none
	Example: deleteAllFavorites()
	*/
	func deleteAllFavorites() {
		// Go to Favorites
		self.gotoFavorites()
		
		// Check for 0 Favorites
		if app.tables.cells.count != 0 {
			// Delete each Favorite
			while (app.tables.cells.count != 0) {
				app.tables.children(matching: .cell).element(boundBy: 0).swipeLeft()
				self.clickButton(name: "Delete", delay: 3)
			}
		}
	}
	
	/*
	Name: deleteAllSavedText()
	Description: Delete all SavedTexts
	Parameters: none
	Example: deleteAllSavedText()
	*/
	func deleteAllSavedText() {
		// Exit Edit mode
		if app.buttons["Done"].exists {
			self.clickButton(name: "Done")
		}
		let count = app.textFields.count
		if count > 0 {
			for _ in 1...count {
				app.textFields.element(boundBy: 0).swipeLeft()
				self.clickButton(name: "Delete")
			}
		}
	}
	
	/*
	Name: deleteAllSavedTextRandomly()
	Description: Delete all SavedTexts Randomly
	Parameters: none
	Example: deleteAllSavedTextRandomly()
	*/
	func deleteAllSavedTextRandomly() {
		// Exit Edit mode
		if app.buttons["Done"].exists {
			self.clickButton(name: "Done")
		}
		let count = app.textFields.count
		for _ in 1...count {
			let count = app.textFields.count
			let random = Int(arc4random_uniform(UInt32(count)))
			app.textFields.element(boundBy: random).swipeLeft()
			self.clickButton(name: "Delete")
		}
	}
	
	/*
	Name: deleteAllSignMail()
	Description: Delete all SignMail
	Parameters: none
	Example: deleteAllSignMail()
	*/
	func deleteAllSignMail() {
		// Check for No SignMail
		if (!app.tables.staticTexts["You don't have any SignMails."].exists) {
			// Select the Edit Button
			self.clickButton(name: "Edit", query: app.navigationBars.buttons)
			
			// Select Delete Button
			self.clickButton(name: "Delete", query: app.navigationBars.buttons)
			
			// Select Delete All Videos
			self.clickButton(name: "Delete All SignMails", query: app.sheets.buttons, delay: 3)
			
			// Wait for No SignMail
			let noSignMail = self.waitFor(query: app.tables.staticTexts, value: "You don't have any SignMails.", timeout: 60)
			XCTAssertTrue(noSignMail.exists, "Failed to delete all SignMail")
		}
	}
	
	/*
	Name: deleteAllSignMailInList()
	Description: Delete each SignMail in the list
	Parameters: none
	Example: deleteAllSignMailInList()
	*/
	func deleteAllSignMailInList() {
		while (!app.tables.staticTexts["You don't have any SignMails."].exists) {
			// Swipe top SignMail
			app.tables.cells.element(boundBy: 0).swipeLeft()
			sleep(1)
			
			// Select Delete
			self.clickButton(name: "Delete", delay: 3)
		}
	}
	
	/*
	Name: deleteApp()
	Description: Delete nTouch app
	Parameters: none
	Example: deleteApp()
	*/
	func deleteApp() {
		XCUIApplication().terminate()
		
		// Force delete the app from the springboard
		let icon = springboard.icons["ntouch"]
		if icon.exists {
			let iconFrame = icon.frame
			let springboardFrame = springboard.frame
			
			if #available(iOS 13.0, *) {
				icon.press(forDuration: 1.4)
				let buttonRemoveApp = springboard.buttons["Remove App"]
				if buttonRemoveApp.waitForExistence(timeout: 5) {
					buttonRemoveApp.tap()
				} else {
					XCTFail("Button \"Remove App\" not found")
				}
			} else {
				// Tap the little "X" button at approximately where it is. The X is not exposed directly
				icon.press(forDuration: 5)
				let xPosition = CGVector(dx: (iconFrame.minX + 3) / springboardFrame.maxX,
									 dy: (iconFrame.minY + 3) / springboardFrame.maxY)
				sleep(3)
				springboard.coordinate(withNormalizedOffset: xPosition).tap()
			}
			let buttonDeleteApp = springboard.alerts.buttons["Delete App"]
			if buttonDeleteApp.waitForExistence(timeout: 5) {
				buttonDeleteApp.tap()
			}
			else {
				deleteAppCancel()
				XCTFail("Button \"Delete App\" not found")
				return
			}
			let buttonDelete = springboard.alerts.buttons["Delete"]
			if buttonDelete.waitForExistence(timeout: 5) {
				buttonDelete.tap()
			}
			else {
				deleteAppCancel()
				XCTFail("Button \"Delete\" not found")
			}
		}
	}
	
	/*
	Name: deleteAppCancel()
	Description: Cancel the delete nTouch app springboard popup
	Parameters: none
	Example: deleteAppCancel()
	*/
	func deleteAppCancel() {
		let cancelAlertPopup = springboard.alerts.buttons["Cancel"]
		if cancelAlertPopup.waitForExistence(timeout: 5) {
			cancelAlertPopup.tap()
		}
	}
	
	/*
	Name: deleteBlockedList()
	Description: Delete blocked list
	Parameters: none
	Example: deleteBlockedList()
	*/
	func deleteBlockedList() {
		// Check for 0 Favorites
		if app.tables.cells.count != 0 {
			while (app.tables.cells.count != 0) {
				app.tables.cells.element(boundBy: 0).tap()
				sleep(3)
				
				// Delete Blocked Number
				self.clickStaticText(text: "Delete Blocked", query: app.tables.staticTexts)
				XCTAssertTrue(app.staticTexts["Are you sure you want to remove this blocked number?"].exists)
				self.clickButton(name: "Delete Blocked", delay: 3)
			}
		}
	}
	
	/*
	Name: deleteBlockedNumber(name:)
	Description: Delete a blocked number by name
	Parameters: name
	Example: deleteBlockedNumber(name: "Automate 5395")
	*/
	func deleteBlockedNumber(name: String) {
		// Select first instance of blocked number
		let buttonQuery = app.tables.staticTexts.matching(identifier: name)
		let firstBlocked = buttonQuery.element(boundBy: 0)
		firstBlocked.tap()
		
		// Delete Blocked Number
		self.clickStaticText(text: "Delete Blocked", query: app.tables.staticTexts)
		XCTAssertTrue(app.staticTexts["Are you sure you want to remove this blocked number?"].exists)
		self.clickButton(name: "Delete Blocked", delay: 3)
	}
	
	/*
	Name: deleteCallHistory(name:num:)
	Description: Delete Call History item by Name and Type
	Parameters: name
				num
	Example: deleteCallHistory(name: "Automate 5441", num: "15553334444")
	*/
	func deleteCallHistory(name: String, num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		
		// P2P Call
		if (app.tables.cells.staticTexts[phnum].exists) {
			app.tables.cells.containing(.staticText, identifier:phnum).element(boundBy: 0).swipeLeft()
		}
		// VRS No Number
		else if phnum == "" {
			app.tables.cells.containing(.staticText, identifier: phnum + "VRS").element.swipeLeft()
		}
		// VRS Call
		else {
			app.tables.cells.containing(.staticText, identifier: phnum + " / VRS").element.swipeLeft()
		}
		sleep(3)
		
		// Delete Item
		self.clickButton(name: "Delete", query: app.tables.buttons, delay: 5)
	}
	
	/*
	Name: deleteContact(name:)
	Description: Delete a contact by name
	Parameters: name
	Example: deleteContact(name: "Automate 5395")
	*/
	func deleteContact(name: String) {
		// Check for Contact, swipe up if not hittable
		if !app.tables.cells.containing(.staticText, identifier: name).element(boundBy: 0).isHittable {
			app.swipeUp()
			sleep(1)
		}
		
		while app.tables.cells.containing(.staticText, identifier: name).element(boundBy: 0).exists {
			// Swipe Contact
			app.tables.cells.containing(.staticText, identifier: name).element(boundBy: 0).swipeLeft()
			sleep(1)
			
			// Delete Contact
			self.clickButton(name: "Delete", query: app.tables.buttons, delay: 3)
		}
	}
	
	/*
	Name: deleteEmail1()
	Description: Delete email1 address
	Parameters: none
	Example: deleteEmail1()
	*/
	func deleteEmail1() {
		// Delete email 1
		self.clickTextField(name: "Email1TextField")
		if (app.tables.textFields["Email1TextField"].buttons["Clear text"].exists) {
			app.tables.textFields["Email1TextField"].buttons["Clear text"].tap()
		}
		self.clickButton(name: "Done")
	}
	
	/*
	Name: deleteEmail2()
	Description: Delete email2 address
	Parameters: none
	Example: deleteEmail2()
	*/
	func deleteEmail2() {
		// Delete email 2
		self.clickTextField(name: "Email2TextField")
		if (app.tables.textFields["Email2TextField"].buttons["Clear text"].exists) {
			app.tables.textFields["Email2TextField"].buttons["Clear text"].tap()
		}
		self.clickButton(name: "Done")
	}
	
	/*
	Name: deleteFavorite(name:type:)
	Description: Delete a Favorite by name and number
	Parameters: favoriteName
				type ("home", "work", or "mobile")
	Example: deleteFavorite(name: "Automate 5420", type:"home")
	**/
	func deleteFavorite(name: String, type: String) {
		let cellCount = app.tables.cells.count
		let count : Int = Int(cellCount);
		let favoriteCellName = String(format: "%@ %@", name, type)
		var cellList = [String]()
		var cellIndex : Int = 0
		
		// Get Favorite Cells
		cellList = self.getFavoritesList()
		
		// Get Favorite Index by name and type
		for index in 0 ..< count {
			if cellList[index].contains(favoriteCellName) {
				cellIndex = index
				break
			}
		}
		
		// Delete Favorite by index
		app.tables.children(matching: .cell).element(boundBy: cellIndex).swipeLeft()
		self.clickButton(name: "Delete", delay: 3)
	}
	
	/*
	Name: deleteLast4ssn()
	Description: Delete Last 4 SSN
	Parameters: none
	Example: deleteLast4ssn()
	*/
	func deleteLast4ssn() {
		self.clickTextField(name: "••••", query: app.tables.secureTextFields)
		// iPad
		if UIDevice.current.userInterfaceIdiom == .pad {
			app.keys["delete"].tap()
		}
		// iPhone
		else {
			app.keys["Delete"].tap()
		}
	}
	
	/*
	Name: deleteMultipleUsers()
	Description: Deletes All Users in Account Histroy
	Parameters:	none
	Example: ui.deleteMultipleUsers()
	*/
	func deleteMultipleUsers() {
		let accountHistoryNavigationBar = app.navigationBars["Account History"]
		accountHistoryNavigationBar.buttons["Edit"].tap()
		accountHistoryNavigationBar.buttons["Delete"].tap()
		self.clickButton(name: "Clear All", query: app.sheets.buttons)
		accountHistoryNavigationBar.buttons["Done"].tap()
		accountHistoryNavigationBar.buttons["Back"].tap()
	}
	
	/*
	Name: deleteSavedText(:text)
	Description: Delete a Saved Text item
	Parameters: text
	Example: deleteSavedText(text: "1234")
	**/
	func deleteSavedText(text: String) {
		// Swipe text to delete
		app.textFields[text].swipeLeft()
		
		// Delete the Saved Text item
		self.clickButton(name: "Delete", delay: 3)
	}
	
	/*
	Name: deleteSavedTextInBox(:text)
	Description: Delete text from the saved text box
	Parameters: text
	Example: deleteSavedTextInBox(text: "1234")
	*/
	func deleteSavedTextInBox(text: String) {
		// Select clear text
		self.clickTextField(name: text)
		self.clickButton(name: "Clear text")
		
		// Select the Done button
		self.clickButton(name: "Done", query: app.navigationBars.buttons, delay: 3)
	}
	
	/*
	Name: deleteSharedTextCharacters(:count)
	Description: Press the delete key <count> number of times
	Parameters: count
	Example: deleteSharedTextCharacters(count: 5)
	*/
	func deleteSharedTextCharacters(count: Int) {		
		// Press delete <count> number times
		for _ in 1 ... count {
			app.keyboards.keys["delete"].tap()
		}
	}
	
	/*
	Name: deleteSignMail(phnum:)
	Description: Delete SignMail by phone number
	Parameters: phnum
	Example: deleteSignMail(phnum: "19323883030")
	*/
	func deleteSignMail(phnum: String) {
		// Swipe SignMail
		let phnum = String (self.formatPhoneNumber(num: phnum))
		app.tables.cells.containing(.staticText, identifier: phnum).element(boundBy: 0).swipeLeft()
		sleep(1)
		
		// Delete Contact
		self.clickButton(name: "Delete", query: app.tables.buttons, delay: 3)
	}
	
	/*
	Name: deleteSorensonContact(name:)
	Description: Attemp to delete Sorenson contact and verify no delete button
	Parameters: name
	Example: deleteSorensonContact(name: "SVRS")
	*/
	func deleteSorensonContact(name: String) {
		// Swipe left to Delete Contact, Verify no delete button, swipe down
		app.tables.cells.staticTexts[name].swipeLeft()
		sleep(2)
		XCTAssertTrue(!app.tables.buttons["Delete"].exists)
		app.swipeDown()
		sleep(2)
	}
	
	/*
	Name: dial
	Description: Enter a number in the dialer
	Parameters: phnum (must not be formatted)
	Example: dial(phnum: "18015757669")
	*/
	func dial(phnum: String) {
		// dial phnum
		for character in phnum {
			switch character {
			case "0":
				app.buttons["0"].tap()
				break
			case "1":
				app.buttons["1"].tap()
				break
			case "2":
				app.buttons["2"].tap()
				break
			case "3":
				app.buttons["3"].tap()
				break
			case "4":
				app.buttons["4"].tap()
				break
			case "5":
				app.buttons["5"].tap()
				break
			case "6":
				app.buttons["6"].tap()
				break
			case "7":
				app.buttons["7"].tap()
				break
			case "8":
				app.buttons["8"].tap()
				break
			case "9":
				app.buttons["9"].tap()
				break
			default:
				XCTFail("dial failed: only use 0-9")
			}
		}
	}
	
	/*
	Name: dialInCall
	Description: Enter a number in the dialer while in call
	Parameters: phnum (must not be formatted)
	Example: dialInCall(phnum: "18015757669")
	*/
	func dialInCall(phnum: String) {
		// dial phnum
		for character in phnum {
			switch character {
			case "0":
				app.windows.element(boundBy: 1).buttons["0"].tap()
				break
			case "1":
				app.windows.element(boundBy: 1).buttons["1"].tap()
				break
			case "2":
				app.windows.element(boundBy: 1).buttons["2"].tap()
				break
			case "3":
				app.windows.element(boundBy: 1).buttons["3"].tap()
				break
			case "4":
				app.windows.element(boundBy: 1).buttons["4"].tap()
				break
			case "5":
				app.windows.element(boundBy: 1).buttons["5"].tap()
				break
			case "6":
				app.windows.element(boundBy: 1).buttons["6"].tap()
				break
			case "7":
				app.windows.element(boundBy: 1).buttons["7"].tap()
				break
			case "8":
				app.windows.element(boundBy: 1).buttons["8"].tap()
				break
			case "9":
				app.windows.element(boundBy: 1).buttons["9"].tap()
				break
			default:
				XCTFail("dial failed: only use 0-9")
			}
		}
	}
	
	/*
	Name: doNotDisturb(setting:)
	Description: Set Do Not Disturb on or off
	Parameters: setting (0 or 1)
	Example: doNotDisturb(setting: 0)
	*/
	func doNotDisturb(setting: Int) {
		// Tap Do Not Disturb to scroll
		app.tables.staticTexts.matching(identifier: "Do Not Disturb (Reject Incoming Calls)").element.tap()
		
		if #available(iOS 13.0, *) {
			// Get Do Not Disturb Switch State
			let switchState = app.tables.switches.matching(identifier: "Do Not Disturb (Reject Incoming Calls)").element.value.debugDescription
			
			// DND on
			if (setting == 1) {
				if switchState.contains("0") {
					app.tables.switches.matching(identifier: "Do Not Disturb (Reject Incoming Calls)").element.tap()
					sleep(1)
					self.clickButton(name: "OK", query: app.alerts.buttons)
				}
			}
			// DND off
			else {
				if switchState.contains("1") {
					app.tables.switches.matching(identifier: "Do Not Disturb (Reject Incoming Calls)").element.tap()
					sleep(1)
				}
			}
		}
		else {
			// Get Do Not Disturb Switch State
			let switchState = app.tables.switches.matching(identifier: "Do Not Disturb (Reject Incoming Calls)").element.debugDescription
			
			// DND on
			if (setting == 1) {
				if switchState.contains("value: 0") {
					app.tables.switches.matching(identifier: "Do Not Disturb (Reject Incoming Calls)").element.tap()
					sleep(1)
					self.clickButton(name: "OK", query: app.alerts.buttons)
				}
			}
			// DND off
			else {
				if switchState.contains("value: 1") {
					app.tables.switches.matching(identifier: "Do Not Disturb (Reject Incoming Calls)").element.tap()
					sleep(1)
				}
			}
		}
	}

	/*
	Name: editAndSelectPersonalRecordType()
	Description: Click Edit and Select a Record Type ("Video Only", "Text Only", "Video With Text")
	Parameters: type
	Example: editAndSelectPersonalRecordType(type: "Video Only")
	*/
	func editAndSelectPersonalRecordType(type: String) {
		// Select the Edit button (icon pencil)
		if app.buttons["icon pencil"].exists {
			app.buttons["icon pencil"].tap()
		}
		sleep(3)
		
		// Select and tap Personal Record Type
		self.clickButton(name: type)
	}

	/*
	Name: editAllSavedTextsRandomly()
	Description: Edit all saved texts Randomly
	Parameters: none
	Example: editAllSavedTextsRandomly()
	*/
	func editAllSavedTextsRandomly() {
		for _ in 1...30 {
			let count = app.textFields.count
			let random = Int(arc4random_uniform(UInt32(count)))
			let textField = app.tables.children(matching: .cell).element(boundBy: random).children(matching: .textField).element
			textField.tap()
			app.keyboards.keys["delete"].tap()
		}
	}
	
	/*
	Name: editBlockedNumber(name:,changeNum:)
	Description: Select a contact to edit by name
	Parameters: name
				changeNum
	Example: editBlockedContactNumber(name: "Automate 3000", changeNum: "18015556666")
	*/
	func editBlockedNumber(name: String, changeNum: String) {
		// Select Contact
		self.selectContact(name: name)
		
		// Select Edit
		self.selectEdit()
		
		// Select Phone Number and Edit
		app.tables.cells.containing(.staticText, identifier:"Phone Number").children(matching: .textField).element.tap()
		self.clickButton(name: "Clear text", query: app.tables.buttons)
		app.tables.cells.containing(.staticText, identifier:"Phone Number").children(matching: .textField).element.typeText(changeNum)
		
		// Select Done
		self.selectDone()
	}
	
	/*
	Name: editBlockedDescription(name:,newName:)
	Description: Edit Blocked Description
	Parameters: name
				newName
	Example: editBlockedDescription(name: "Auto 5334", newName: "Automate 5334")
	*/
	func editBlockedDescription(name: String, newName: String) {
		// Select Contact
		self.selectContact(name: name)
		
		// Select Edit
		self.selectEdit()
		
		// Select Name and Edit
		app.tables.cells.containing(.staticText, identifier:"Description").children(matching: .textField).element.tap()
		self.clickButton(name: "Clear text", query: app.tables.buttons)
		app.tables.cells.containing(.staticText, identifier:"Description").children(matching: .textField).element.typeText(newName)
		sleep(1)
		
		// Select Done
		self.clickButton(name: "Done", query: app.navigationBars["Blocked Number"].buttons)
		
		// Go to blocked list
		self.gotoBlockedList()
	}
	
	/*
	Name: editContact(name:)
	Description: Select a contact to edit by name
	Parameters: name
	Example: editContact(name: "Automate 5395")
	*/
	func editContact(name: String) {
		// Select Contact
		self.selectContact(name: name)
		
		// Select Edit
		self.selectEdit()
	}
	/*
	Name: editContactWithPaste(name:)
	Description: Select a contact to edit by name and add new text
	Parameters: name
	Example: editContactWithPaste(name: "Automate 5395")
	*/
	func editContactWithPaste(name: String) {
		// Select Contact
		self.selectContact(name: name)
		
		// Select Edit
		self.selectEdit()
		
		// Clear and paste new text
		self.clickTextField(name: name, query: app.tables.textFields)
		self.clickButton(name: "Clear text", query: app.tables.cells.buttons)
		app.tables.textFields["Name"].press(forDuration: 1.2)
		self.clickMenuItem(name: "Paste")
		self.clickButton(name: "Done")
		
		// Error handle on No name
		if app.buttons["OK"].exists {
			app.buttons["OK"].tap()
		}
		else {
			if app.tables.textFields[name].exists {
				self.clickButton(name: "Contacts", query: app.navigationBars.buttons)
			}
		}
	}
	
	/*
	Name: editSavedText(:text, :newText)
	Description: Edit a Saved Text item (append)
	Parameters: text,
				newText
	Example: editSavedText(text: "1234")
	*/
	func editSavedText(text: String, newText: String) {
		// Enter text
		self.clickAndEnterText(field: text, text: newText)
		
		// Select the Done button
		self.clickButton(name: "Done", query: app.navigationBars.buttons, delay: 3)
	}
	
	/*
	Name: editTextGreeting(text:)
	Description: Edit only the text for the greeting
	Parameters: text
	Example: editTextGreeting(text: "Test greeting")
	*/
	func editTextGreeting(text: String) {
		// Edit current greeting
		self.clickButton(name: "icon pencil")
		
		clearAndEnterText(text: text)
		app.swipeDown() // refresh ui
		self.clickButton(name: "Done")
		
		// Select the Save button
		self.clickButton(name: "Save")
		sleep(3)
		
		// Verify text
		let textValue = app.textViews.element(boundBy: 0).value(forKey: "value")
		XCTAssertTrue((textValue! as AnyObject).contains(text))
	}
	
	/*
	Name: enableAudio(setting:)
	Description: Set Audio Enabled on or off
	Parameters: setting
	Example: enableAudio(setting: 0)
	*/
	func enableAudio(setting: Int) {
		// Tap Audio Enabled to scroll
		app.tables.staticTexts.matching(identifier: "Audio Enabled").element(boundBy: 0).tap()
		
		// Get Enable Audio Switch State
		let switchState = app.tables.switches.matching(identifier: "Audio Enabled").element(boundBy: 0).value.debugDescription
		
		// Enable Audio
		if (setting == 1) {
			if switchState.contains("0") {
				app.tables.switches.matching(identifier: "Audio Enabled").element(boundBy: 0).tap()
				sleep(1)
			}
		}
		// Disable Audio
		else {
			if switchState.contains("1") {
				app.tables.switches.matching(identifier: "Audio Enabled").element(boundBy: 0).tap()
				sleep(1)
			}
		}
	}
	
	/*
	Name: enable1LineVCO(setting:)
	Description: Set 1-Line VCO on or off
	Parameters: setting
	Example: enable1LineVCO(setting: 1)
	*/
	func enable1LineVCO(setting: Int) {
		// iPad
		if UIDevice.current.userInterfaceIdiom == .pad {
			// Tap 1-Line VCO
			app.tables.staticTexts.matching(identifier: "1-Line Voice Carry Over (VCO)").element(boundBy: 0).tap()
			
			// Get 1 Line VCO Switch State
			let switchState = app.tables.switches.matching(identifier: "1-Line Voice Carry Over (VCO)").element(boundBy: 0).value.debugDescription
			
			// Enable VCO
			if setting == 1 {
				if switchState.contains("0") {
						app.tables.switches.matching(identifier: "1-Line Voice Carry Over (VCO)").element(boundBy: 0).tap()
						sleep(1)
				}
			}
			// Disable VCO
			else {
				if switchState.contains("1") {
						app.tables.switches.matching(identifier: "1-Line Voice Carry Over (VCO)").element(boundBy: 0).tap()
						sleep(1)
				}
			}
		}
		// iPhone or iPod
		else {
			// Tap 1-Line VCO
			app.tables.staticTexts.matching(identifier: "1-Line VCO").element(boundBy: 0).tap()
			
			// Get 1 Line VCO Switch State
			let switchState = app.tables.switches.matching(identifier: "1-Line VCO").element(boundBy: 0).value.debugDescription
			
			// Enable VCO
			if setting == 1 {
				if switchState.contains("0") {
						app.tables.switches.matching(identifier: "1-Line VCO").element(boundBy: 0).tap()
						sleep(1)
				}
			}
			// Disable VCO
			else {
				if switchState.contains("1") {
						app.tables.switches.matching(identifier: "1-Line VCO").element(boundBy: 0).tap()
						sleep(1)
				}
			}
		}
	}
	
	/*
	Name: enableContactPhotos(setting:)
	Description: Show Contact Photos on or off
	Parameters: setting
	Example: enableContactPhotos(setting: true)
	*/
	func enableContactPhotos(setting: Bool) {
		// Go to Personal Info
		self.gotoPersonalInfo()
		
		// Set the switch
		self.setSettingsSwitch(setting: "Show Contact Photos", value: setting)
		sleep(3)
	}

	/*
	Name: enableKeyCodes()
	Description: Enable Key Codes using the dialer
	Parameters: none
	Example: enableKeyCodes()
	*/
	func enableKeyCodes() {
		self.gotoHome()
		self.clearDialer()
		app.buttons["4"].tap()
		app.buttons["1"].tap()
		app.buttons["9"].tap()
		app.buttons["2"].tap()
		app.buttons["Call"].tap()
		self.clickButton(name: "OK")
	}
	
	/*
	Name: enablePhoneSettingPermissions()
	Description: enable app permissions in phone setting
	Parameters: none
	Example: enablePhoneSettingPermissions()
	*/
	func enablePhoneSettingPermissions() {
		self.ntouchTerminate()
		let settingsApp = XCUIApplication(bundleIdentifier: "com.apple.Preferences")
		settingsApp.launch()
		settingsApp.tables.cells.staticTexts["ntouch"].tap()
		let switchMicrophone = settingsApp.tables.switches.matching(identifier: "Microphone").element(boundBy: 0)
		let switchCamera = settingsApp.tables.switches.matching(identifier: "Camera").element(boundBy: 0)
		let switchContacts = settingsApp.tables.switches.matching(identifier: "Contacts").element(boundBy: 0)
		let switchLocalNetwork = settingsApp.tables.switches.matching(identifier: "Local Network").element(boundBy: 0)
		let location = settingsApp.tables.cells.staticTexts["Location"]
		let notifications = settingsApp.tables.cells.staticTexts["Notifications"]
		
		if (switchMicrophone.value as? String == "0") {
			switchMicrophone.tap()
		}
		if (switchCamera.value as? String == "0") {
			switchCamera.tap()
		}
		if (switchContacts.exists && switchContacts.value as? String == "0") {
			switchContacts.tap()
		}
		
		if (switchLocalNetwork.exists && switchLocalNetwork.value as? String == "0") {
			switchLocalNetwork.tap()
		}
		
		if (notifications.exists) {
			notifications.tap()
			let switchNotifications = settingsApp.tables.switches.matching(identifier: "Allow Notifications").element(boundBy: 0)
			if (switchNotifications.value as? String == "0") {
				switchNotifications.tap()
			}
			settingsApp.buttons["ntouch"].tap()
			settingsApp.buttons["Settings"].tap()
			settingsApp.tables.cells.staticTexts["ntouch"].tap()
		}
		
		if (location.exists) {
			sleep(2)
			location.tap()
			settingsApp.tables.cells.staticTexts["While Using the App"].tap()
		}

		settingsApp.terminate()
		self.ntouchActivate()
	}
	
	/*
	Name: enableSpanishFeatures(setting:)
	Description: Set Enable Spanish Features on or off
	Parameters: setting
	Example: enableSpanishFeatures(setting: false)
	*/
	func enableSpanishFeatures(setting: Bool) {
		self.setSettingsSwitch(setting: "Enable Spanish Features", value: setting)
		self.heartbeat()
	}
	
	/*
	Name: enableTunneling(setting:)
	Description: Set Tunneling on or off
	Parameters: setting
	Example: enableTunneling(setting: 0)
	*/
	func enableTunneling(setting: Int) {
		// Go to Settings
		self.gotoSettings()
		
		// Tap Tunneling Enabled to scroll
		app.tables.staticTexts.matching(identifier: "Tunneling Enabled").element(boundBy: 0).tap()
		sleep(1)
		
		// Get Tunneling Enabled Switch State
		let switchState = app.tables.switches.matching(identifier: "Tunneling Enabled").element(boundBy: 0).value.debugDescription
		
		switch setting {
		// Enable Tunneling
		case 1:
			if switchState.contains("0") {
				app.tables.switches.matching(identifier: "Tunneling Enabled").element(boundBy: 0).tap()
				sleep(1)
				self.heartbeat()
			}
			break
		// Disable Tunneling
		case 0:
			if switchState.contains("1") {
				app.tables.switches.matching(identifier: "Tunneling Enabled").element(boundBy: 0).tap()
				sleep(1)
				self.heartbeat()
			}
			break
		default:
			break
		}
	}
	
	/*
	Name: enterCompany(name:)
	Description: Enter a company name
	Parameters: name
	Example: enterCompany(name: "Auto Company")
	*/
	func enterCompany(name: String) {
		// Enter the Company Name
		self.clickTextField(name: "Company Name", query: app.tables.textFields, delay: 3)
		self.enterText(field: "Company Name", text: name)
	}
	
	/*
	Name: enterContact(name:)
	Description: Enter a contact name
	Parameters: name
	Example: enterContact(name: "Automate 5395")
	*/
	func enterContact(name: String) {
		// Enter the Contact Name
		self.clickTextField(name: "Name", query: app.tables.textFields, delay: 3)
		self.enterText(field: "Name", text: name)
	}
	
	/*
	Name: enterContact(number:type:)
	Description: Enter a contact number by type
	Parameters: phnum
				type ("home", "work", or "mobile")
	Example: enterContact(number: "18015757669", type:"home")
	**/
	func enterContact(number: String, type: String) {
		// Select textfield
		app.tables.cells.containing(.staticText, identifier: type).children(matching: .textField).element.tap()
		sleep(3)
		
		// Tap Clear text button
		if app.tables.cells.containing(.staticText, identifier: type).children(matching: .textField).buttons["Clear text"].exists {
			app.tables.cells.containing(.staticText, identifier: type).children(matching: .textField).buttons["Clear text"].tap()
		}
		
		// Enter Contact number
		app.tables.cells.containing(.staticText, identifier: type).children(matching: .textField).element.typeText(number)
		sleep(1)
	}
	
	/*
	Name: enterEmail1(email)
	Description: Enter email1 address
	Parameters: email
	Example: enterEmail(email: "test@test.com")
	*/
	func enterEmail1(email: String) {
		// Enter email 1
		self.clickTextField(name: "Email1TextField", delay: 3)
		if (app.tables.textFields["Email1TextField"].buttons["Clear text"].exists) {
			app.tables.textFields["Email1TextField"].buttons["Clear text"].tap()
		}
		self.enterText(field: "Email1TextField", text: email)
		self.clickButton(name: "Done")
	}
	
	/*
	Name: enterEmail2(email)
	Description: Enter email2 address
	Parameters: email
	Example: enterEmail2(email: "test@test.com")
	*/
	func enterEmail2(email: String) {
		// Enter email 2
		self.clickTextField(name: "Email2TextField", delay: 3)
		if (app.tables.textFields["Email2TextField"].buttons["Clear text"].exists) {
			app.tables.textFields["Email2TextField"].buttons["Clear text"].tap()
		}
		self.enterText(field: "Email2TextField", text: email)
		self.clickButton(name: "Done")
	}
	
	/*
	Name: enterText(field:text:query:timeout:delay)
	Description: Enter text into a text field
	Parameters: field, text, query, timeout, delay
	Example: enterText(field: "Username", text: "testuser5")
	*/
	func enterText(field: String, text: String, query: XCUIElementQuery = app.textFields,timeout: Double = 30, delay: UInt32 = 1) {
		let textField = self.waitFor(query: query, value: field, timeout: timeout)
		textField.typeText(text)
		sleep(delay)
	}
	
	/*
	enterTextAndCopy(text)
	Description: enter text and then copy it.
	Parameters: none
	Example: enterTextAndCopy(Text: "1234")
	*/
	func enterTextAndCopy(text: String) {
		// Input Emojis and copy them
		app.searchFields["Search Contacts & Yelp"].tap()
		app.searchFields["Search Contacts & Yelp"].typeText(text)
		app.searchFields["Search Contacts & Yelp"].press(forDuration: 1.2)
		self.clickMenuItem(name: "Select All")
		self.clickMenuItem(name: "Copy")
	}
	
	/*
	Name: enterLast4ssn(num)
	Description: Enter Last 4 SSN
	Parameters: ssn
	Example: enterLast4ssn(num: "1234")
	*/
	func enterLast4ssn(num: String) {
		// Tap Secure Text Field and Enter Last 4 SSN
		self.clickTextField(name: "XXXX", query: app.tables.secureTextFields)
		self.enterText(field: "XXXX", text: num, query: app.secureTextFields)
	}
	
	/*
	Name: enterNewPassword(newPassword)
	Description: Enter New Password
	Parameters: newPassword
	Example: enterNewPassword(newPassword: "1234")
	*/
	func enterNewPassword(newPassword: String) {
		self.clickTextField(name: "New Password", query: app.tables.secureTextFields, delay: 3)
		self.enterText(field: "New Password", text: newPassword, query: app.secureTextFields)
		self.clickButton(name: "Return")
	}
	
	/*
	Name: enterNewPasswordConfirm(newPassword)
	Description: Enter New Password Confirm
	Parameters: newPassword
	Example: enterNewPasswordConfirm(newPassword: "1234")
	*/
	func enterNewPasswordConfirm(newPassword: String) {
		self.clickTextField(name: "Confirm Password", query: app.tables.secureTextFields, delay: 3)
		self.enterText(field: "Confirm Password", text: newPassword, query: app.secureTextFields)
		self.clickButton(name: "Return")
	}
	
	/*
	Name: enterOldPassword(oldPassword)
	Description: Enter Old Password
	Parameters: oldPassword
	Example: enterOldPassword(oldPassword: "1234")
	*/
	func enterOldPassword(oldPassword: String) {
		self.clickTextField(name: "Old Password", query: app.tables.secureTextFields, delay: 3)
		self.enterText(field: "Old Password", text: oldPassword, query: app.secureTextFields)
		self.clickButton(name: "Return")
	}
	
	/*
	Name: enterPassword(num)
	Description: Enter Password
	Parameters: password
	Example: enterPassword(password: "1234")
	*/
	func enterPassword(password: String) {
		self.clickTextField(name: "Password", query: app.secureTextFields, delay: 3)
		self.enterText(field: "Password", text: password, query: app.secureTextFields)
	}
	
	/*
	Name: enterPhoneNumber(num)
	Description: Enter Phone Number
	Parameters: phnum
	Example: enterPhoneNumber(phnum: "19323887000")
	*/
	func enterPhoneNumber(phnum: String) {
		let phoneNumberTextField = app.textFields["PhoneNumber"]
		
		if #available(iOS 14.0, *) {
			phoneNumberTextField.tap() // this is needed for iPhone 8 and 8 Plus
			sleep(1)
			phoneNumberTextField.doubleTap()
			phoneNumberTextField.press(forDuration: 2) // this is needed for iPhone 8 and 8 Plus
			sleep(1)
			app.keys["Delete"].tap()
		}
		else if #available(iOS 13.4, *) {
			phoneNumberTextField.press(forDuration: 2)
			sleep(1)
			app.keys["Delete"].tap()
		}
		else {
			phoneNumberTextField.tap()
			sleep(1)
			app.keys["Delete"].press(forDuration: 6)
		}
		sleep(3)
		self.enterText(field: "PhoneNumber", text: phnum)
	}
	
	/*
	Name: enterImportSorensonPassword(password)
	Description: Enter Secure Static Text for Import Sorenson Contact
	Parameters: password
	Example: enterImportSorensonPassword(password: "1234")
	*/
	func enterImportSorensonPassword(password: String) {
		// Enter Import Sorenson Contacts password
		let pwTextbox = app.tables.cells.containing(.staticText, identifier:"Password").secureTextFields.element(boundBy: 0)
		pwTextbox.tap()
		sleep(1)
		pwTextbox.typeText(password)
	}
	
	/*
	Name: enterImportSorensonPasswordPaste()
	Description: Enter pasted text into Secure Static Text Import Sorenson Contact
	Parameters: text
	Example: enterImportSorensonPasswordPaste()
	*/
	func enterImportSorensonPasswordPaste() {
		// Paste password into Import Sorenson Password field required
		let pwTextbox = app.tables.cells.containing(.staticText, identifier:"Password").secureTextFields.element(boundBy: 0)
		pwTextbox.tap()
		pwTextbox.press(forDuration: 1.2)
		self.clickMenuItem(name: "Paste")
		
	}
	/*
	Name: enterImportSorensonPhoneNumber(phnum)
	Description: Enter Static Text Import Sorenson Contact
	Parameters: phum
	Example: enterImportSorensonPhoneNumber(phnum)
	*/
	func enterImportSorensonPhoneNumber(phnum: String) {
		// Enter Import Sorenson Contacts phone number
		let phnumTextbox = app.tables.cells.containing(.staticText, identifier:"Phone Number").textFields.element(boundBy: 0)
		phnumTextbox.tap()
		sleep(1)
		phnumTextbox.typeText(phnum)
	}
	
	/*
	Name: formatPhoneNumber(num:)
	Description: Format a phone number
	Parameters: phnum
	Example: formatPhoneNumber(num: "18015757669")
	**/
	func formatPhoneNumber(num: String) -> String {
		var formattedPhoneNumber = ""
		if num.count == 11 {
			var start = num.index(num.startIndex, offsetBy: 1)
			var end = num.index(num.endIndex, offsetBy: -7)
			let areaCode = start ..< end
			start = num.index(num.startIndex, offsetBy: 4)
			end = num.index(num.endIndex, offsetBy: -4)
			let prefix = start ..< end
			start = num.index(num.startIndex, offsetBy: 7)
			end = num.index(num.endIndex, offsetBy: 0)
			let exten = start ..< end
			formattedPhoneNumber = String(format: "%@ (%@) %@-%@", String(num[num.index(num.startIndex, offsetBy: 0)]),
											  String(num[areaCode]),
											  String(num[prefix]),
											  String(num[exten]))
		}
		else {
			formattedPhoneNumber = num
		}
		return formattedPhoneNumber
	}
	
	/*
	 * Name: generateRandomInvalidAreaCodeNumber
	 * Description: Generate a phone number with random invalid area code
	 * Parameters: none
	 * Example: generateRandomInvalidAreaCodeNumber()
	 */
	func generateRandomInvalidAreaCodeNumber() -> String {
		let randNum = Int.random(in: 10000000000...12002000000)
		return String(randNum)
	}
	
	/*
	Name: getCallHistoryList()
	Description: Get the Call History list of names
	Parameters: none
	Example: getCallHistoryList()
	*/
	func getCallHistoryList() -> [String]{
		// Get cell count
		let cellCount = app.tables.cells.count
		let count : Int = Int(cellCount);
		
		// Get each item and add it to the list
		let callHistoryTable = app.tables.cells
		var callHistoryList = [String]()
		
		for index in 0 ..< count {
			let i = Int(index)
			let callHistoryName = callHistoryTable.element(boundBy: i).staticTexts.element(boundBy: 0).label + " " + callHistoryTable.element(boundBy: i).staticTexts.element(boundBy: 2).label
			callHistoryList.append(callHistoryName)
		}
		return callHistoryList
	}
	
	/*
	Name: getContactPhoto(name:)
	Description: Get Contact Photo by Name
	Parameters: name
	Example: getContactPhoto(name: "Automate 5395")
	*/
	func getContactPhoto(name: String) -> Data {
		let cellCount = app.tables.cells.count
		let count : Int = Int(cellCount);
		var cellList = [String]()
		var imageIndex : Int = 0
		
		// Get Contact Photo Strings
		for index in 0 ..< count {
			let i : Int = index
			let cellValue = app.tables.children(matching: .cell).element(boundBy: i).identifier
			cellList.append(cellValue)
		}
		
		// Get Contact Photo Index by name
		for index in 0 ..< count {
			if cellList[index].contains(name) {
				imageIndex = index
				break
			}
		}
		
		// Get Contact Photo Data
		let contactPhotoData = app.tables.cells.children(matching: .image).element(boundBy: imageIndex).screenshot().pngRepresentation
		return contactPhotoData
	}
	
	/*
	Name: getContactCount(name:)
	Description: Get the number of instances of a contact
	Parameters: name
	Example: getContactCount()
	*/
	func getContactCount(name: String) -> Int{
		// get the contacts list, then count the instances of name
		var trueCount = 0
		let contactsList = self.getContactsList()
		for i in contactsList {
			if i == name {
				trueCount += 1
			}
		}
		return trueCount
	}
	
	/*
	Name: getContactsList()
	Description: Get the Contacts list
	Parameters: none
	Example: getContactsList()
	*/
	func getContactsList() -> [String]{
		// Get cell count
		let cellCount = app.tables.cells.count
		let count : Int = Int(cellCount);
		
		// Get the name of each contact and add it to the list
		let contactsTable = app.tables.cells
		var contactsList = [String]()
		
		for index in 0 ..< count {
			let i = Int(index)
			let contactName =  contactsTable.element(boundBy: i).staticTexts.element.label
			contactsList.append(contactName)
		}
		return contactsList
	}
	
	/*
	Name: getBlockList()
	Description: Get the Blocked Contact list
	Parameters: none
	Example: getBlockList()
	*/
	func getBlockList() -> [String]{
		// Get cell count
		let cellCount = app.tables.cells.count
		let count : Int = Int(cellCount);
		
		// Get the name of each contact and add it to the list
		let blockTable = app.tables.cells
		var blockList = [String]()
		//for var index = 0; index < count; index+=1 {
		for index in 0 ..< count {
			let i = Int(index)
			let blockContactName = blockTable.element(boundBy: i).staticTexts.element(boundBy: 0).label
			blockList.append(blockContactName)
		}
		return blockList
	}
	
	/*
	Name: getFavoritesList()
	Description: Get the Favorites list with Name and Type
	Parameters: none
	Example: getFavoritesList()
	*/
	func getFavoritesList() -> [String]{
		// Get cell count
		let cellCount = app.tables.cells.count
		let count : Int = Int(cellCount);
		
		// Get the name and type of each favorite and add it to the list
		let favoritesTable = app.tables.cells
		var favoritesList = [String]()
		//for var index = 0; index < count; index+=1 {
		for index in 0 ..< count {
			let i = Int(index)
			let favoriteName = favoritesTable.element(boundBy: i).staticTexts.element(boundBy: 0).label
			let favoriteType = favoritesTable.element(boundBy: i).staticTexts.element(boundBy: 1).label
			favoritesList.append(favoriteName + " " + favoriteType)
		}
		return favoritesList
	}
	
	/*
	Name: gotoAccountInfo()
	Description: go to Account Info
	Parameters: none
	Example: gotoAccountInfo()
	*/
	func gotoAccountInfo() {
		// go to Settings
		self.gotoSettings()
		
		// Select Account Info
		self.clickStaticText(text: "Account Information", query: app.tables.staticTexts)
	}
	
	/*
	Name: gotoBlockedList()
	Description: go to the Blocked List
	Parameters: none
	Example: gotoBlockedList()
	*/
	func gotoBlockedList() {
		// go to Settings
		self.gotoSettings()
		
		// Select Blocked Numbers
		self.clickStaticText(text: "Blocked Numbers", query: app.tables.staticTexts)
	}
	
	/*
	Name: gotoCallHistory(list:)
	Description: go to Call History
	Parameters: list (All, Missed)
	Example: gotoCallHistory(list: "All")
	*/
	func gotoCallHistory(list: String) {
		// Tap the Call History Button
		self.clickTabBarButton(name: "History", doubleClick: true)
		
		// Select list
		self.clickSegmentedControlButton(name: list)
	}
	
	/*
	Name: gotoCallHistoryInCall(list:)
	Description: go to Call History in call window
	Parameters: list (All, Missed)
	Example: gotoCallHistoryInCall(list: "All")
	*/
	func gotoCallHistoryInCall(list: String) {
		// Tap the Call History Button
		self.gotoCallTab(button: "History")
		sleep(1)
		
		// Select list
		self.clickSegmentedControlButton(name: list)
	}
	
	/*
	Name: gotoCallTab(button:)
	Description: Select a Call Tab Button
	Parameters: button (Dial, Contacts, History)
	Example: gotoCallTab(button: "Dial")
	*/
	func gotoCallTab(button: String) {
		// Tap the Call Tab Button
		app.windows.element(boundBy: 1).tabBars.buttons[button].tap()
		sleep(1)
	}
	
	/*
	Name: gotoDeviceContacts()
	Description: go to Device Contacts
	Parameters: none
	Example: gotoDeviceContacts()
	*/
	func gotoDeviceContacts() {
		// go to Phonebook
		self.gotoPhonebook()
		
		// Select Sorenson Contacts
		self.clickSegmentedControlButton(name: "Device")
	}
	
	/*
	Name: gotoEnhancedSignMail()
	Description: go to Enhanced SignMail
	Parameters: none
	Example: gotoEnhancedSignMail()
	*/
	func gotoEnhancedSignMail() {
		// Go to SignMail
		self.gotoSignMail()
		
		// Select New Direct SignMail button
		self.clickButton(name: "Compose", query: app.navigationBars.buttons, delay: 3)
	}
	
	/*
	Name: gotoEnhancedSignMailContacts()
	Description: Go to Contacts tab on Enhanced SignMail
	Parameters: name
	Example: gotoEnhancedSignMailContacts()
	*/
	func gotoEnhancedSignMailContacts() {
		// Go to Enhanced SignMail
		self.gotoEnhancedSignMail()
		
		// Select Contacts
		if #available(iOS 13.0, *) {
			app.tabBars.element(boundBy: 1).buttons["Contacts"].tap()
		}
		else {
			app.tabBars.element(boundBy: 0).buttons["Contacts"].tap()
		}
		sleep(1)
	}
	
	/*
	Name: gotoFavorites()
	Description: go to Favorites
	Parameters: none
	Example: gotoFavorites()
	*/
	func gotoFavorites() {
		// go to Phonebook
		self.gotoPhonebook()
		
		// Select Favorites
		self.clickSegmentedControlButton(name: "Favorites", delay: 3)
	}
	
	/*
	Name: gotoCustomerCare()
	Description: go to Customer Care
	Parameters: none
	Example: gotoCustomerCare()
	*/
	func gotoCustomerCare() {
		// Go to Settings
		self.gotoSettings()
		
		// Select Customer Care
		self.clickStaticText(text: "Call Customer Care")
	}
	
	/*
	Name: gotoHome()
	Description: go to the dialer, home
	Parameters: none
	Example: gotoHome()
	*/
	func gotoHome() {
		// Tap the Dial Button
		self.clickTabBarButton(name: "Dial")
	}
	
	/*
	Name: gotoImportContacts()
	Description: Go to import contacts in settings
	Parameters: none
	Example: gotoImportContacts()()
	*/
	func gotoImportContacts() {
		// Go to Settings
		self.gotoSettings()
		
		// Select Contacts
		self.clickStaticText(text: "Contacts", query: app.tables.staticTexts)
		
	}
	
	/*
	 Name: gotoPlaySignMailGreetingAfter()
	 Description: go to the Play SignMail Greeting After
	 Parameters: none
	 Example: gotoPlaySignMailGreetingAfter()
	*/
	func gotoPlaySignMailGreetingAfter() {
		//Select Play SignMail greeting after from settings
		self.clickStaticText(text: "Play SignMail greeting after", query: app.tables.staticTexts, delay: 3)
	}
	
	/*
	Name: gotoPersonalInfo()
	Description: go to Personal Info
	Parameters: none
	Example: gotoPersonalInfo()
	*/
	func gotoPersonalInfo() {
		// go to Settings
		self.gotoSettings()
		
		// Select Personal Info
		self.clickStaticText(text: "Personal Information", query: app.tables.staticTexts)
	}
	
	/*
	Name: gotoPhonebook()
	Description: go to the phonebook
	Parameters: none
	Example: gotoPhonebook()
	*/
	func gotoPhonebook() {
		// Double tap the Phonebook Button
		self.clickTabBarButton(name: "Contacts", doubleClick: true)
	}
	
	/*
	Name: gotoPSMG()
	Description: go to PSMG
	Parameters: none
	Example: gotoPSMG()
	*/
	func gotoPSMG() {
		// go to Settings
		self.gotoSettings()
		
		// Select SignMail Greeting
		self.clickStaticText(text: "SignMail Greeting", query: app.tables.staticTexts)
	}
	
	/*
	Name: gotoSavedText()
	Description: go to Saved Text
	Parameters: none
	Example: gotoSavedText()
	*/
	func gotoSavedText() {
		// Go to Settings
		self.gotoSettings()
		
		// Select Saved Text
		self.clickStaticText(text: "Saved Text", query: app.tables.staticTexts)
	}
	
	/*
	Name: gotoSettings()
	Description: go to Settings
	Parameters: none
	Example: gotoSettings()
	*/
	func gotoSettings() {
		self.clickTabBarButton(name: "Settings", doubleClick: true)
	}
	
	/*
	Name: gotoSignMail()
	Description: go to the SignMail
	Parameters: none
	Example: gotoSignMail()
	*/
	func gotoSignMail() {
		// Tap the SignMail Tab
		self.clickTabBarButton(name: "SignMail", doubleClick: true)
		
		// Tab the SignMail Button
		self.clickSegmentedControlButton(name: "SignMail")
	}
	
	/*
	Name: gotoSorensonContacts()
	Description: go to Sorenson Contacts
	Parameters: none
	Example: gotoSorensonContacts()
	*/
	func gotoSorensonContacts() {
		// go to Phonebook
		self.gotoPhonebook()
		
		// Select Sorenson Contacts
		self.clickSegmentedControlButton(name: "Contacts")
	}
	
	/*
	Name: gotoVideoCenter()
	Description: go to Video Center
	Parameters: none
	Example: gotoVideoCenter()
	*/
	func gotoVideoCenter() {
		// Tap the SignMail Tab
		self.clickTabBarButton(name: "SignMail", doubleClick: true)
		
		// Tap the Video Center Button
		self.clickSegmentedControlButton(name: "Video Center")
	}
	
	/*
	Name: gotoYelp()
	Description: go to Yelp
	Parameters: none
	Example: gotoYelp()
	*/
	func gotoYelp() {
		// Go to home and select the search button
		self.gotoHome()
		self.clickButton(name: "Search", delay: 3)
	}
	
	/*
	Name: greetingRecorded(response:)
	Description: Save, Record Again, Preview, or Cancel greeting
	Parameters: response ("Save", "Record Again", "Preview", "Cancel")
	Example: greetingRecorded(response: "Save")
	*/
	func greetingRecorded(response: String) {
		// Select Response
		app.sheets["Greeting Recorded"].buttons[response].tap()
		if response == "Save" {
			// Wait for upload to complete (Play button appear)
			let playBtn = self.waitFor(query: app.buttons, value: "icon play", timeout: 60)
			XCTAssertTrue(playBtn.exists, "Failed to upload greeting")
		}
		sleep(3)
	}
	
	/*
	Name: hangup()
	Description: Select the End Call button
	Parameters: none
	Example: hangup()
	*/
	func hangup() {
		// Select the End Call button
		if (app.buttons["Hangup"].exists) {
			app.buttons["Hangup"].tap()
		}
		else {
			self.selectTop()
			if(app.buttons["Hangup"].exists) {
				app.buttons["Hangup"].tap()
			}
			else {
				XCTFail("Hangup button not found - Caller: \(testUtils.callerNum), Callee: \(testUtils.calleeNum) - \(testUtils.getTimeStamp())")
			}
		}
		sleep(5)
	}
	
	/*
	Name: heartbeat()
	Description: Heartbeat the iOS device
	Parameters: none
	Example: heartbeat()
	*/
	func heartbeat() {
		// Press home button
		self.pressHomeBtn()
		
		// Open ntouch
		self.ntouchActivate()
	}
	
	/*
	Name: hideSelfView(setting:)
	Description: Hide or Show self view during a call
	Parameters: setting
	Example: hideSelfView(setting: true)
	*/
	func hideSelfView(setting: Bool = true) {
		if setting {
			// Swipe Left until x cooridnate is negative
			while app.otherElements["LocalViewVideo"].frame.debugDescription.contains("(-") == false {
				app.otherElements["LocalViewVideo"].swipeLeft()
			}
		}
		else {
			// Swipe Right with 10,000 velocity to restore self view
			app.otherElements["LocalViewVideo"].swipeRight(velocity: 10000)
		}
		sleep(1)
	}
	
	/*
	Name: importSorensonButtonOK()
	Description: Import Sorenson button OK
	Parameters: none
	Example: importSorensonButtonOK()
	*/
	func importSorensonButtonOK() {
		// Successful import
		if (app.alerts["Import completed."].buttons["OK"].exists) {
			app.alerts["Import completed."].buttons["OK"].tap()
		}
		// Invalid phone number
		else
			if (app.alerts["Invalid Phone Number"].buttons["OK"].exists) {
				app.alerts["Invalid Phone Number"].buttons["OK"].tap()
			}
			// Invalid password
			else
				if (app.alerts["Can't import contacts."].buttons["OK"].exists) {
					app.alerts["Can't import contacts."].buttons["OK"].tap()
				}
	}
	
	/*
	Name: importSorensonPhotos(setting:)
	Description: Import Sorenson Photos
	Parameters: none
	Example: importSorensonPhotos(setting: 0)
	*/
	func importSorensonPhotos() {
		// Go Settings
		self.gotoSettings()
		
		// Select Contacts
		self.clickStaticText(text: "Contacts", query: app.tables.staticTexts, delay: 2)
		
		// Select Import Photos From Sorenson
		self.clickStaticText(text: "Import Photos From Sorenson", query: app.tables.staticTexts, delay: 5)
		
		// Select OK
		let okButton = self.waitFor(query: app.alerts["Import completed."].buttons, value: "OK")
		okButton.tap()
	}
	
	/*
	Name: importVPContacts(phnum:pw:)
	Description: import contacts with phnum and pw
	Parameters: phnum
				pw
	Example: importVPContacts(phnum: "17323005678", pw:"1234")
	*/
	func importVPContacts(phnum: String, pw: String) {
		// go to settings
		self.gotoSettings()
		
		// select Contacts
		self.clickStaticText(text: "Contacts", query: app.tables.staticTexts)
		
		// Enter phonenumber
		enterImportSorensonPhoneNumber(phnum: phnum)
		
		// Enter password
		enterImportSorensonPassword(password: pw)
		
		// Select the Done button
		self.clickButton(name: "Done")
		self.clickStaticText(text: "Import From Videophone Account", query: app.tables.staticTexts, delay: 5)
		
		// Wait for Contacts Imported and select OK
		let contactsImported = self.waitFor(query: app.alerts, value: "Contacts imported.")
		XCTAssertTrue(contactsImported.exists, "Import contacts failed")
		contactsImported.buttons["OK"].tap()
	}
	
	/*
	Name: importVPContacts(phnum:pw:expectedNumber)
	Description: import contacts with phnum, pw, and expected number of contacts
	Parameters: phnum
	pw
	expectedNumber
	Example: importVPContacts(phnum: "17323005678", pw:"1234", expectedNumber: "2")
	*/
	func importVPContacts(phnum: String, pw: String, expectedNumber: String) {
		// go to settings
		self.gotoSettings()
		
		// select Contacts
		self.clickStaticText(text: "Contacts", query: app.tables.staticTexts)
		
		// Enter phonenumber
		self.enterImportSorensonPhoneNumber(phnum: phnum)
		
		// Enter password
		self.enterImportSorensonPassword(password: pw)
		
		// Select the Done button
		self.clickButton(name: "Done")
		self.clickStaticText(text: "Import From Videophone Account", query: app.tables.staticTexts)
		
		// Wait for Contacts Imported
		let contactsImported = self.waitFor(query: app.alerts, value: "Contacts imported.")
		XCTAssertTrue(contactsImported.exists, "Import contacts failed")
		
		// Verify number of imported contacts
		self.verifyImportedNumber(expectedNumber: expectedNumber)
		
		// Select OK
		contactsImported.buttons["OK"].tap()
	}
	
	/*
	Name: incomingCall(response:)
	Description: Answer or Decline incoming CallKit window
	Parameters: response ("Answer", "Decline")
	Example: incomingCall(response: "Answer")
	*/
	func incomingCall(response: String) {
		// Wait for Incoming Call
		self.waitForIncomingCall()
		
		// CallKit
		switch response {
		case "Answer":
			if #available(iOS 18.0, *) {
				inCallServiceApp.buttons["acceptCall"].firstMatch.tap()
				springboard.buttons["Accept"].tap()
				sleep(3)
				// workaround for double CallKit window
				if inCallServiceApp.buttons["acceptCall"].waitForExistence(timeout: 3) {
					inCallServiceApp.buttons["acceptCall"].firstMatch.tap()
				}
			}
			else {
				springboard.buttons["Accept"].tap()
				sleep(3)
				if springboard.buttons["Accept"].waitForExistence(timeout: 3) {
					springboard.buttons["Accept"].tap()
				}
			}
			self.waitForCallOptions()
		case "Decline":
			if #available(iOS 18.0, *) {
				inCallServiceApp.buttons["rejectCall"].firstMatch.tap()
				sleep(3)
				// workaround for double CallKit window
				if inCallServiceApp.buttons["rejectCall"].waitForExistence(timeout: 3) {
					inCallServiceApp.buttons["rejectCall"].firstMatch.tap()
				}
			}
			else {
				springboard.buttons["Decline"].tap()
				sleep(3)
				// workaround for double CallKit window
				if springboard.buttons["Decline"].waitForExistence(timeout: 3) {
					springboard.buttons["Decline"].tap()
				}
			}
		default:
			XCTFail("Incoming call failed: use Answer, or Decline")
		}
		sleep(3)
	}
	
	/*
	Name: incomingNtouchCall(response:)
	Description: Answer or Decline incoming ntouch call window
	Parameters: response ("Answer", "Decline")
	Example: incomingNtouchCall(response: "Answer")
	*/
	func incomingNtouchCall(response: String) {
		// Wait for Incoming Call
		self.waitForIncomingNtouchCall()
		
		// ntouch
		switch response {
		case "Answer":
			self.clickButton(name: "Answer", delay: 3)
			self.waitForCallOptions()
		case "Decline":
			self.clickButton(name: "Hang up", delay: 3)
		default:
			XCTFail("Incoming call failed: use Answer, or Decline")
		}
	}
	
	/*
	Name: leaveSignMailRecording(time:response:doSkip)
	Description: Leave signmail recording after call rejection or enhanced signmail button is pressed
	Parameters:
	time,
	response ("Send", "Rerecord", "Exit"),
	doSkip
	Example: leaveSignMailRecording()
	*/
	func leaveSignMailRecording(time: UInt32, response: String, doSkip: Bool = true) {
		if(doSkip) {
			self.skipGreeting()
		}
		self.waitForSignMailRecording()
		sleep(time)
		if time < 120 {
			self.hangup()
		}
		self.confirmSignMailSend(response: response)
	}
	
	/*
	Name: login()
	Description: Login without Phnum and Password
	Parameters: none
	Example: login()
	*/
	func login() {
		// Select the Log In button
		self.selectLogin()
		
		// Wait for tabBar
		self.waitForTabBar()
	}
	
	/*
	Name: login(phnum:password:)
	Description: Login with Phnum and Password
	Parameters: phnum (optional)
				password (optional)
	Example: login(phnum: "19323883030", password: "1234")
			 login(phnum: "", password: "")
	*/
	func login(phnum: String, password: String) {
		testUtils.callerNum = phnum
		
		// Enter Phnum
		self.enterPhoneNumber(phnum: phnum)
		
		// Enter Password and select Done to login
		self.enterPassword(password: password)
		self.clickButton(name: "Done")
		
		// Wait for tabBar
		self.waitForTabBar()
	}
	
	/*
	Name: logout()
	Description: logout of ntouch
	Parameters: none
	Example: logout()
	*/
	func logout() {
		// go to Account Info
		self.gotoSettings()
		
		// Select the Log Out button
		self.clickButton(name: "Log Out", query: app.navigationBars.buttons, delay: 10)
	}
	
	/*
	Name: moveFavorite(favorite:toFavorite:)
	Description: Move a favorite to another favorite's position
	Parameters: favorite
				toFavorite
	Example: moveFavorite(favorite: "Automate 5484, home", toFavorite: "Automate 5484, work")
	*/
	func moveFavorite(favorite: String, toFavorite: String) {
		// Select the Edit button
		self.clickButton(name: "Edit", query: app.navigationBars.buttons)
		
		// Move Favorite
		app.buttons["Reorder " + favorite].press(forDuration: 0.2, thenDragTo: app.buttons["Reorder " + toFavorite])
		
		// Select the Done button
		self.clickButton(name: "Done", query: app.navigationBars.buttons)
	}
	
	/*
	Name: ntouchActivate()
	Description: Re-open the ntouch app
	Parameters: none
	Example: ntouchActivate()
	*/
	func ntouchActivate() {
		XCUIApplication().activate()
		sleep(5)
	}
	
	/*
	Name: ntouchRestart()
	Description: Restart the ntouch app
	Parameters: none
	Example: ntouchRestart()
	*/
	func ntouchRestart() {
		self.ntouchTerminate()
		self.ntouchActivate()
	}
	
	/*
	Name: ntouchTerminate()
	Description: Terminate the ntouch app
	Parameters: none
	Example: ntouchTerminate()
	*/
	func ntouchTerminate() {
		XCUIApplication().terminate()
		sleep(10)
	}
	
	/*
	Name: pauseMessage()
	Description: Select the SignMail pause button
	Parameters: none
	Example: pauseMessage()
	*/
	func pauseMessage() {
		if !app.buttons["Pause"].exists {
			self.selectTop3rd()
		}
		app.buttons["Pause"].tap()
	}
	
	/*
	Name: openFullScreen()
	Description: Open Full Screen
	Parameters: none
	Example: openFullScreen()
	*/
	func openFullScreen() {
		if (!app.buttons["Full screen"].exists) {
			self.selectTop()
			sleep(1)
		}
		app.buttons["Full screen"].tap()
	}
	
	/*
	Name: optionSwitchStatus(text:)
	Description: Returns whether switch element is in the on or off state
	Parameters: text
	Example: optionSwitchStatus(text: "Hide My Caller ID")
	*/
	func optionSwitchStatus(text: String) -> Bool {
		app.tables.staticTexts.matching(identifier: text).element.tap()
		
		// Get Switch State
		let switchState = app.tables.switches.matching(identifier: text).element.value.debugDescription
		
		if switchState.contains("1") {
			return true
		}
		return false
	}
	
	/*
	Name: pausePSMG()
	Description: Pause PSMG
	Parameters: none
	Example: pausePSMG()
	*/
	func pausePSMG() {
		self.clickButton(name: "icon pause", delay: 3)
	}
	
	/*
	Name: playMessage()
	Description: Select the SignMail play button
	Parameters: none
	Example: playMessage()
	*/
	func playMessage() {
		if !app.buttons["Play"].exists {
			app.windows.element(boundBy: 0).tap()
		}
		app.buttons["Play"].tap()
	}
	
	/*
	Name: playMessage(phnum:)
	Description: Play SignMail by phone number
	Parameters: phnum
	Example: playMessage(phnum: "19323883030")
	*/
	func playMessage(phnum: String) {
		// Format the phone number
		let phoneNumber = String (self.formatPhoneNumber(num: phnum))
		//Select and Play SignMail message
		self.clickStaticText(text: phoneNumber, delay: 5)
	}
	
	/*
	Name: playPSMG()
	Description: Play PSMG
	Parameters: none
	Example: playPSMG()
	*/
	func playPSMG() {
		self.clickButton(name: "icon play", delay: 3)
	}
	
	/*
	Name: playVideo(title:delay:)
	Description: Play Video Center Video by name
	Parameters: video, delay
	Example: playVideo(title: "Default Greetings")
	*/
	func playVideo(title: String, delay: UInt32 = 3) {
		app.staticTexts.matching(identifier: title).element(boundBy: 0).tap()
		sleep(delay)
	}

	/*
	Name: pressBackSpace(duration:)
	Description: Press and hold keyboard Backspace button for specified duration
	Parameters: duration
	Example: pressBackSpace(duration: 5)
	*/
	func pressBackspace(duration: Double) {
		app.keyboards.keys["delete"].press(forDuration: duration)
	}
	
	/*
	Name: pressBackSpace(times:)
	Description: Press the  keyboard Backspace button a specified number of times
	Parameters: times
	Example: pressBackSpace(times: 5)
	*/
	func pressBackspace(times: Int) {
		for _ in 1...times {
			app.keyboards.keys["delete"].tap()
		}
	}

	/*
	Name: pressHomeBtn()
	Description: Press the iOS device Home Button
	Parameters: none
	Example: pressHomeBtn()
	*/
	func pressHomeBtn() {
		// Press the device Home Button
		XCUIDevice.shared.press(XCUIDevice.Button.home)
		sleep(10)
	}
	
	/*
	Name: record(time:)
	Description: Record for length (sec)
	Parameters: time
	Example: record(time: 30)
	*/
	func record(time: UInt32, doubleTapRecord: Bool = false) {
		// Select the Record button
		if !doubleTapRecord {
			self.clickButton(name: "Record")
		} else {
			self.clickButton(name: "Record", doubleClick: true)
		}
		sleep(4)
		
		// Wait for time
		sleep(time)
		
		// Select the Stop button
		if time < 30 {
			app.buttons["Stop"].tap()
		}
		sleep(3)
	}

	/*
	Name: recordGreeting()
	Description: Start to Record Greeting
	Parameters: none
	Example: recordGreeting()
	*/
	func recordGreeting() {
		// Select edit and personal signmail type
		editAndSelectPersonalRecordType(type: "Video Only")
		
		// Select the Record button
		self.clickButton(name: "Record", delay: 4)
	}
	
	/*
	Name: recordGreeting(time:)
	Description: Record Greeting for length (sec)
	Parameters: time
	Example: recordGreeting(time: 30)
	*/
	func recordGreeting(time: UInt32) {
		// Select the Edit button (icon pencil)
		// Select edit and personal signmail type
		editAndSelectPersonalRecordType(type: "Video Only")
		
		// Record
		self.record(time: time)
	}
	
	/*
	Name: recordGreeting(time:text:)
	Description: Record Greeting for time (sec) with text
	Parameters: time
				text
	Example: recordGreeting(time: 30, text: "greeting")
	*/
	func recordGreeting(time: UInt32, text: String, alternateTextSave: Bool = false) {
		// Select edit and personal signmail type
		editAndSelectPersonalRecordType(type: "Video With Text")
		
		clearAndEnterText(text: text)
		app.swipeDown() // refresh ui
		if alternateTextSave {
			self.selectTop3rd()
			self.clickButton(name: "Save")
			self.clickButton(name: "icon pencil")
			// Record
			self.record(time: time)
		} else {
			self.clickButton(name: "Done")
			// Record
			self.record(time: time)
		}
	}
	
	/*
	Name: redial
	Description: redial the last number
	Parameters: none
	Example: redial()
	*/
	func redial() {
		// Double tap the Call button
		self.clickButton(name: "Call", delay: 3, doubleClick: true)
	}
	
	/*
	Name: redirectedMessage(response:)
	Description: Verify Redirected message and select Cancel or Continue
	Parameters: response
	Example: redirectedMessage(response: "Cancel")
	*/
	func redirectedMessage(response: String) {
		// Verify Redirected Number
		XCTAssertTrue(app.alerts["Attention"].exists)
		let textViewQuery = app.alerts.staticTexts.debugDescription
		XCTAssertTrue(textViewQuery.contains("The phone number you have dialed has been changed to"), "Redirect message not found.")
		
		// Select response
		self.clickButton(name: response, query: app.alerts.buttons, delay: 3)
	}
	
	/*
	Name: rejectAgreement()
	Description: Reject Agreement and select Yes or No
	Parameters: response
	Example: rejectAgreement(repsonse: "Yes")
	*/
	func rejectAgreement(response: String) {
		// Select Reject Button
		self.clickButton(name: "Reject", delay: 3)
		
		// Select response
		self.clickButton(name: response, query: app.alerts["Are you sure?"].buttons, delay: 5)
	}

	/*
	Name: rememberPassword()
	Description: Toggles Remember Password Switch
	Parameters: Setting: true or false
	Example:  rememberPassword(setting: true)
	*/
	func rememberPassword(setting: Bool) {
		// Get Switch State
		let switchState = app.switches["Remember Password"].value.debugDescription

		// Switch on
		if (setting) {
			if switchState.contains("0") {
				app.switches["Remember Password"].tap()
				sleep(1)
			}
		}
		// Switch off
		 else {
			if switchState.contains("1") {
				app.switches["Remember Password"].tap()
				sleep(1)
			}
		}
	}
	
	/*
	Name: removeCompanyName()
	Description: Remove a Company Name from the Edit Contact screen
	Parameters: none
	Example: removeCompanyName()
	*/
	func removeCompanyName(name: String) {
		// Remove Company Name
		self.clickTextField(name: "Company Name", query: app.tables.textFields)
		self.clickButton(name: "Clear text", query: app.tables.cells.buttons)
	}
	
	/*
	Name: removeContactName()
	Description: Remove a Contact Name from the Edit Contact screen
	Parameters: none
	Example: removeContactName()
	*/
	func removeContactName(name: String) {
		// Remove Contact Name
		self.clickTextField(name: name, query: app.tables.textFields)
		self.clickButton(name: "Clear text", query: app.tables.cells.buttons)
	}
	
	/*
	Name: removeContactNumber(type:)
	Description: Remove the Contact Number from the Edit Contact screen by type
	Parameters: type ("home", "work", or "mobile")
	Example: removeContactNumber(type: "home")
	*/
	func removeContactNumber(type: String) {
		// Remove Number by type
		app.tables.cells.containing(.staticText, identifier: type).children(matching: .textField).element.tap()
		self.clickButton(name: "Clear text", query: app.tables.cells.buttons)
	}
	
	/*
	Name: removeFavorite()
	Description: Remove Favorite on the Contact Details screen
	Parameters: none
	Example: removeFavorite()
	*/
	func removeFavorite() {
		// Select Remove Favorite
		self.clickStaticText(text: "Remove Favorite", query: app.tables.cells.staticTexts)
	}
	
	/*
	Name: removeFavorite(type:phnum:)
	Description: Remove Favorite on the Contact Details screen by type
	Parameters: type (home, work, mobile)
				num
	Example: removeFavorite(type: "home", phnum: "18015757669")
	*/
	func removeFavorite(type: String, phnum: String) {
		// Select Remove Favorite
		self.removeFavorite()
		
		// Select type and number for multiple numbers
		let num = formatPhoneNumber(num: phnum)
		let typeAndNumber = NSString(format: "%@: %@", type, num)
		self.clickButton(name: typeAndNumber as String, query: app.sheets.buttons)
	}
	
	/*
	Name: removePhoto()
	Description: Remove the Contact Photo
	Parameters: none
	Example: removePhoto()
	*/
	func removePhoto() {
		// Select Edit photo
		self.selectEditPhoto()
		
		// Select Remove Photo
		self.clickButton(name: "Remove Photo", query: app.sheets.buttons)
	}
	
	/*
	Name: replaceContactNumFromDialer(name:oldNum:newNum)
	Description: Replace a number for an existing contact from dialer
	Parameters: name
				type
				oldNum
				newNum
	Example: replaceContactNumFromDialer(name: "Automate 1996", type: "home", oldNum: "18015757889", newNum: "18015757889")
	*/
	func replaceContactNumFromDialer(name: String, type: String, oldNum: String, newNum: String) {
		// Enter the phone number
		self.clearDialer()
		self.dial(phnum: newNum)
		
		// Select the add Button
		self.clickButton(name: "Add Contact")
		
		// Select Add to Existing Contact and select contact
		self.clickButton(name: "Add to Existing Contact", query: app.sheets.buttons, delay: 3)
		self.selectContact(name: name)
		
		// Replace Number
		// Format oldNum
		let oldFormatNum = self.formatPhoneNumber(num: oldNum)
		self.clickButton(name: "\(type) \(oldFormatNum)", query: app.sheets.buttons)
	}
	
	/*
	Name: rotate(dir:)
	Description: Rotate the screen
	Parameters: dir ("Left", "Right", or "Portrait")
	Example: rotate(dir: "Left")
	*/
	func rotate(dir: String) {
		switch dir {
		case "Left":
			XCUIDevice.shared.orientation = .landscapeLeft
			break
		case "Right":
			XCUIDevice.shared.orientation = .landscapeRight
			break
		case "Portrait":
			XCUIDevice.shared.orientation = .portrait
			break
		default:
			XCTFail("Rotate failed: use Left, Right, or Portrait")
		}
		sleep(3)
	}

	/*
	Name: safariOpen()
	Description: Open the Safari app
	Parameters: none
	Example: safariOpen()
	*/
	func safariOpen() {
		safari.launch()
		sleep(5)
	}
	
	/*
	Name: saveContact()
	Description: Save the contact by selecting the done button
	Parameters: none
	Example: saveContact()
	*/
	func saveContact() {
		// Select the Done button
		self.clickButton(name: "Done")
		sleep(5)
	}
	
	/*
	Name: searchCallHistory(item:)
	Description: Search Call History
	Parameters: item (name or number)
	Example: searchCallHistory(item: "Tech")
	*/
	func searchCallHistory(item: String) {
		self.clickAndEnterSearch(field: "Search", text: item)
	}
	
	/*
	Name: searchContactList(name:)
	Description: Searches Contact List
	Parameters: name
	Example: searchContactList(name: "Automate")
	*/
	func searchContactList(name: String) {
		self.clickAndEnterSearch(field: "Search Contacts & Yelp", text: name)
	}

	/*
	Name: searchSettings(word:)
	Description: Search Settings
	Parameters: word (name or number)
	Example: searchSettings(word: "Cellular")
	*/
	func searchSettings(word: String) {
		self.clickAndEnterSearch(field: "Search Settings", text: word)
	}

	/*
	Name: searchSettingsClear()
	Description: Clears the Search Settings
	Parameters: none
	Example: searchSettingsClear()
	*/
	func searchSettingsClear() {
		self.clickButton(name: "Cancel", delay: 3)
	}
	
	/*
	Name: searchSignMail(item:)
	Description: Search SignMail
	Parameters: item (name or number)
	Example: searchSignMail(item: "Tech")
	*/
	func searchSignMail(item: String) {
		self.clickAndEnterSearch(field: "Search", text: item)
	}
	
	/*
	Name: selectAddCall()
	Description: Select the Add Call button
	Parameters: none
	Example: selectAddCall()
	*/
	func selectAddCall() {
		// Select Add Call
		if app.buttons["Call Options"].waitForExistence(timeout: 10) {
			app.buttons["Call Options"].tap()
		}
		else {
			selectTop()
			app.buttons["Call Options"].tap()
		}
		sleep(3)
		
		// Select Make Another Call
		self.clickButton(name: "Make Another Call", query: app.sheets.buttons, delay: 3)
	}
	
	/*
	Name: selectAddContact()
	Description: Select the Add Contact button
	Parameters: none
	Example: selectAddContact()
	*/
	func selectAddContact() {
		// Select Add
		self.clickButton(name: "Add", query: app.navigationBars.buttons)
	}
	
	/*
	Name: selectAddNewContactFromDialer()
	Description: Select Add New Contact from dialer
	Parameters: none
	Example: selectAddNewContactFromDialer()
	*/
	func selectAddNewContactFromDialer() {
		// Select Add button
		self.selectDialerAddBtn()
		
		// Select Create New Contact
		self.clickButton(name: "Create New Contact", query: app.sheets.buttons)
	}
	
	/*
	Name: selectAddSavedText()
	Description: Select Add SavedText
	Parameters: none
	Example: selectAddSavedText()
	*/
	func selectAddSavedText() {
		self.clickStaticText(text: "Add Saved Text", delay: 3)
	}
	
	/*
	Name: selectBirthYear()
	Description: Swipe down to select birth year
	Parameters: none
	Example: selectBirthYear()
	*/
	func selectBirthYear() {
		// Select Birthday
		self.clickStaticText(text: "Birthday", query: app.tables.staticTexts)
		
		// Get Current Year
		let date = Date()
		let calendar = Calendar.current
		let currentYear = String(calendar.component(.year, from: date))
		
		// Swipe down to select birth year
		if #available(iOS 14.0, *) {
			app.datePickers.otherElements["Date Picker"].tap()
			app.datePickers.buttons["Show year picker"].tap()
			sleep(1)
		}
		let datePickerWheels = app.datePickers.pickerWheels
		datePickerWheels[currentYear].swipeDown()
		sleep(1)
	}
	
	/*
	Name: selectCall(option:)
	Description: Select a call option
	Parameters: option (Make Another Call, Switch Calls, Dialpad/Tones, Transfer Call, Join Calls, Show/Hide Call Stats)
	Example: selectCall(option: "Transfer")
	*/
	func selectCall(option: String) {
		// Select the Call Options button
		if (app.buttons["Call Options"].exists) {
			app.buttons["Call Options"].tap()
		}
		else {
			self.selectTop3rd()
			app.buttons["Call Options"].tap()
		}
		sleep(3)
		
		// Select Option
		self.clickButton(name: option, query: app.sheets.buttons, delay: 3)
	}
	
	/*
	Name: selectCallButton()
	Description: Select Call Button
	Parameters: none
	Example: selectCallButton()
	*/
	func selectCallButton() {
		// Press the Call button
		self.clickButton(name: "Call", delay: 3)
	}
	
	/*
	Name: selectCallOptions()
	Description: Select Call Options Button
	Parameters: none
	Example: selectCallOptions()
	*/
	func selectCallOptions() {
		// Press the Call button
		self.clickButton(name: "Call Options", delay: 3)
	}
	
	/*
	Name: selectCallCustomerService()
	Description: Select Call Customer Care
	Parameters: none
	Example: selectCallCustomerService()
	*/
	func selectCallCustomerService() {
		self.clickStaticText(text: "Call Customer Care", delay: 3)
	}
	
	/*
	Name: selectCancel()
	Description: select the Cancel button
	Parameters: none
	Example: selectCancel()
	*/
	func selectCancel() {
		self.clickButton(name: "Cancel", delay: 3)
	}
	
	/*
	Name: selectCancelRegistration()
	Description: select the Cancel button and select Cancel or Logout
	Parameters: response
	Example: selectCancelRegistration(response: "Cancel")
	*/
	func selectCancelRegistration(response: String) {
		self.selectCancel()
		self.clickButton(name: response, query: app.alerts.buttons, delay: 2)
	}
	
	/*
	Name: selectChangePassword
	Description: Select Change Password
	Parameters: none
	Example: selectChangePassword()
	*/
	func selectChangePassword() {
		self.clickStaticText(text: "Change Password", query: app.tables.staticTexts, delay: 3)
	}
	
	/*
	Name: selectClose()
	Description: select the Close button
	Parameters: none
	Example: selectClose()
	*/
	func selectClose() {
		// Select Close
		self.clickButton(name: "Close", delay: 3)
	}
	
	/*
	Name: selectContact(name:)
	Description: Select a contact by name
	Parameters: contactName
	Example: selectContact(name: "Automate 5395")
	*/
	func selectContact(name: String) {
		// Select Contact Name
		app.tables.staticTexts.matching(identifier: name).element(boundBy: 0).tap()
		sleep(3)
	}
	
	/*
	Name: selectContactAlert()
	Description: Select OK on Alert from Contact Error message
	Parameters: none
	Example: selectContact()
	*/
	func selectContactAlert() {
		// Select OK on Alert
		self.clickButton(name: "OK", query: app.alerts["Unable to Save"].buttons)
	}
	
	/*
	Name: selectSettingsBackBtn
	Description: Select the Back Button for Settings
	Parameters: none
	Example: selectSettingsBackBtn()
	*/
	func selectContactsBackBtn() {
		// Select Settings Back button
		self.clickButton(name: "Contacts", query: app.navigationBars.buttons)
	}
	
	/*
	Name: selectContactCallHistory()
	Description: Select "View Call History" button from contact entry screen
	Parameters: none
	Example: selectGreeting()
	*/
	func selectContactCallHistory() {
		// Select view call history button
		self.clickStaticText(text: "View Call History")
	}
	
	/*
	Name: selectContactFromHistory(contactName:)
	Description: select contact from Call History
	Parameters: contactName
	Example: selectContactFromHistory(contactName: "Automate 0603")
	*/
	func selectContactFromHistory(contactName: String) {
		// Find Call History by Contact Name and select Info
		app.tables.cells.containing(.staticText, identifier:contactName).children(matching: .button).element(boundBy: 0).tap()
		sleep(1)
	}
	
	/*
	Name: selectContactFromHistory(num:)
	Description: select contact from Call History
	Parameters: num
	Example: selectContactFromHistory(num: "18017760003")
	*/
	func selectContactFromHistory(num: String) {
		// Format phone number
		let phnum = self.formatPhoneNumber(num: num)
		
		// Find Call History by phnum
		app.tables.cells.containing(.staticText, identifier: phnum).element(boundBy: 0).buttons["More Info"].tap()
	}
	
	/*
	Name: selectDialerAddBtn()
	Description: Select the Add Contact button on the dialer
	Parameters: none
	Example: selectDialerAddBtn()
	*/
	func selectDialerAddBtn() {
		// Select the Add button
		self.clickButton(name: "Add Contact", delay: 3)
	}
	
	/*
	Name: selectDone()
	Description: select the Done button
	Parameters: none
	Example: selectDone()
	*/
	func selectDone() {
		
		let hittable = NSPredicate(format: "hittable == true")
		let expectation = XCTNSPredicateExpectation(predicate: hittable, object: app.buttons["Done"])
		let result = XCTWaiter().wait(for: [expectation], timeout: 20)
		XCTAssertEqual(result, .completed, "Failed to select Done Button")
		
		// select the Done button
		self.clickButton(name: "Done", delay: 3)
	}
	
	/*
	Name: selectDutNumber()
	Description: Selects DUT Number
	Parameters:	phnum
	Example: ui.selectDutNumber(phnum: dutNum)
	*/
	func selectDutNumber(phnum: String) {
		// Format the phone number
		let phoneNumber = String (self.formatPhoneNumber(num: phnum))
		// Selects DUT Number in Multie-Users table list
		self.clickStaticText(text: phoneNumber, query: app.tables.staticTexts)
	}

	/*
	Name: selectEdit()
	Description: Select the Edit button
	Parameters: none
	Example: selectEdit()
	*/
	func selectEdit() {
		// Select Edit
		self.clickButton(name: "Edit")
	}
	
	/*
	Name: selectEditPhoto()
	Description: Select edit photo for a contact
	Parameters: none
	Example: selectEditPhoto()
	*/
	func selectEditPhoto() {
		// Select edit photo
		if app.tables.staticTexts["edit"].exists {
			app.tables.staticTexts["edit"].tap()
		}
		else {
			if #available(iOS 14.0, *) {
				app.tables.staticTexts.matching(identifier: "Edit").element(boundBy: 1).tap()
			}
			else {
				self.clickStaticText(text: "Edit", query: app.tables.staticTexts)
			}
		}
		sleep(1)
	}
	
	/*
	Name: selectGalleryPhoto()
	Description: Select the first Gallery photo for a contact
	Parameters: none
	Example: selectGalleryPhoto()
	*/
	func selectGalleryPhoto() {
		// Select Choose Photo
		self.choosePhoto()
		
		if #available(iOS 14.0, *) {
			// Select Photos
			self.clickSegmentedControlButton(name: "Photos", delay: 3)
			
			// Select the first photo in the gallery
			app.scrollViews.images.element(boundBy: 0).forceTapElement()
		}
		else {
			// Select All Photos or Camera Roll
			if app.tables.cells["All Photos"].exists {
				app.tables.cells["All Photos"].tap()
			}
			else if app.tables.cells["Camera Roll"].exists{
				app.tables.cells["Camera Roll"].tap()
			}
			sleep(3)
			
			// Select the first photo in the gallery
			app.collectionViews.children(matching: .cell).element(boundBy: 0).tap()
		}
		sleep(3)
		
		// iPad
		if UIDevice.current.userInterfaceIdiom == .pad {
			app.buttons["Use"].forceTapElement()
			sleep(3)
		}
		// iPhone or iPod
		else {
			// Select Choose Button
			app.buttons["Choose"].forceTapElement()
			sleep(3)
		}
	}
	
	/*
	Name: selectGreeting(type:)
	Description: Select Greeting type
	Parameters: type ("Sorenson", "Personal", "No Greeting")
	Example: selectGreeting(type: "Personal")
	*/
	func selectGreeting(type: String) {
		// Select Greeting Type
		self.clickButton(name: type)
		heartbeat()
	}
	
	/*
	Name: selectLogin()
	Description: select the Log In button
	Parameters: none
	Example: selectLogin()
	*/
	func selectLogin() {
		// select the Log In button (if Log In is not hittable select Done)
		if app.buttons["Log In"].isHittable {
			self.clickButton(name: "Log In")
		}
		else {
			self.clickButton(name: "Done")
		}
		sleep(10)
	}
	
	/*
	Name: selectmultipleUsers()
	Description: MultipleUser button
	Parameters:	none
	Example: ui.selectMultipleUsers()
	*/
	func selectMultipleUsers() {
		// Selects the MultipleUsers button
		self.clickButton(name: "MultipleUsers")
	}
	
	/*
	Name: selectNavDone()
	Description: select the Navigation Bar Done button
	Parameters: none
	Example: selectNavDone()
	*/
	func selectNavDone() {
		self.clickButton(name: "Done", query: app.navigationBars.buttons, delay: 3)
	}
	
	/*
	Name: selectNetworkSpeed()
	Description: Select Network Speed
	Parameters: none
	Example: selectNetworkSpeed()
	*/
	func selectNetworkSpeed() {
		self.clickStaticText(text: "Network Speed", query: app.tables.staticTexts, delay: 3)
	}
	
	/*
	Name: selectNetworkSpeed()
	Description: Select Network Speed
	Parameters: setting
	Example: selectNetworkSpeed(setting: "512 Kbps")
	*/
	func selectNetworkSpeed(setting: String) {
		self.selectNetworkSpeed()
		self.clickStaticText(text: setting, query: app.tables.staticTexts, delay: 3)
	}
	
	/*
	Name: selectNoSSN(setting:)
	Description: Select No SSN
	Parameters: setting (0 or 1)
	Example: selectNoSSN(setting: 0)
	*/
	func selectNoSSN(setting: Int) {
		if #available(iOS 13.0, *) {
			let ssnSelected = app.buttons["checkmark"].exists
			if setting == 1 {
				if !ssnSelected {
					self.clickStaticText(text: "I do not have a SSN")
				}
			}
			else {
				if ssnSelected {
					self.clickStaticText(text: "I do not have a SSN")
				}
			}
		}
		else {
			let ssnSelected = app.buttons["More Info"].exists
			if setting == 1 {
				if !ssnSelected {
					self.clickStaticText(text: "I do not have a SSN")
				}
			}
			else {
				if ssnSelected {
					self.clickStaticText(text: "I do not have a SSN")
				}
			}
		}
	}
	
	/*
	Name: selectPasswordTextbox()
	Description: select the Password textbox
	Parameters: none
	Example: selectPasswordTextbox()
	*/
	func selectPasswordTextbox() {
		self.clickTextField(name: "Password", query: app.secureTextFields)
	}
	
	/*
	Name: selectPhoneNumberTextbox()
	Description: select the Phone Number textbox
	Parameters: none
	Example: selectPhoneNumberTextbox()
	*/
	func selectPhoneNumberTextbox() {
		self.clickTextField(name: "PhoneNumber")
	}
	
	/*
	Name: selectProfilePhoto()
	Description: Select the Profile photo for a contact
	Parameters: none
	Example: selectProfilePhoto()
	*/
	func selectProfilePhoto() {
		// Select Edit photo
		self.selectEditPhoto()
		
		// Select Profile Photo
		self.clickButton(name: "Use Sorenson Photo", query: app.sheets.buttons, delay: 3)
		
		if (app.alerts["No Profile Photo Found"].exists) {
			self.clickButton(name: "OK", query: app.alerts.buttons, delay: 2)
		}
	}
	
	/*
	Name: selectRingBeforeGreeting
	Description: Select Rings Before Greeting Count
	Parameters: setRing
	Example: selectRingBeforeGreeting(setRing: 5)
	**/
	func selectRingBeforeGreeting(setRing: String) {
		// Select Ring count based on value of setRing (min & default is 5, range 5-15)
		self.clickStaticText(text: "\(setRing) Rings", delay: 5)
	}
	
	/*
	Name: selectSave()
	Description: select the Save button
	Parameters: none
	Example: selectSave()
	*/
	func selectSave() {
		// select the Save button
		self.clickButton(name: "Save", delay: 3)
	}
	
	/*
	Name: selectSavedText()
	Description: select the SavedText button
	Parameters: none
	Example: selectSavedText()
	*/
	func selectSavedText() {
		// select the SavedText button
		self.clickButton(name: "SavedText", delay: 5)
	}
	
	/*
	Name: selectSavedTextItem(text:)
	Description: Select the Saved Text item
	Parameters: text
	Example: selectSavedTextItem("1234")
	*/
	func selectSavedTextItem(text: String) {
		// Tap specified Saved Text item
		self.clickTextField(name: text)
		if (app.buttons["Done"].exists) {
			self.clickButton(name: "Done")
		}
	}
	
	/*
	Name: selectSearchField()
	Description: Select search field
	Parameters: none
	Example: selectSearchField()
	*/
	func selectSearchField() {
		// Tap the search field
		app.searchFields["Search Contacts & Yelp"].tap()
	}
	
	/*
	Name: selectSetupPersonalSignMailGreeting()
	Description: Select Setup Personal SignMail Greeting
	Parameters: none
	Example: selectSetupPersonalSignMailGreeting()
	*/
	func selectSetupPersonalSignMailGreeting() {
		self.clickStaticText(text: "Setup Personal SignMail Greeting", delay: 2)
		XCTAssertTrue(app.staticTexts["SignMail Greeting"].exists)
	}
	
	/*
	Name: selectSettingsBackBtn
	Description: Select the Back Button for Settings
	Parameters: none
	Example: selectSettingsBackBtn()
	*/
	func selectSettingsBackBtn() {
		// Select Settings Back button
		self.clickButton(name: "Settings", query: app.navigationBars.buttons, delay: 3)
	}
	
	/*
	Name: selectSignMail(phnum:)
	Description: Select SignMail by phone number
	Parameters: phnum
	Example: selectSignMail(phnum: "19323883030")
	*/
	func selectSignMail(phnum: String) {
		// Format the phone number
		let phoneNumber = String (self.formatPhoneNumber(num: phnum))
		
		// Find cell by phnum
		let messageCell = app.tables.cells.containing(.staticText, identifier:phoneNumber)
		
		// Select Options button
		messageCell.buttons["More Info"].tap()
		sleep(2)
	}
	
	/*
	Name: selectSubmitRegistration()
	Description: select the Submit Registration button
	Parameters: none
	Example: selectSubmitRegistration()
	*/
	func selectSubmitRegistration() {
		app.navigationBars["FCC Registration"].forceTapElement()
		self.clickButton(name: "Submit")
	}
	
	/*
	Name: selectTakePhoto()
	Description: Select Take Photo
	Parameters: none
	Example: selectTakePhoto()
	*/
	func selectTakePhoto() {
		// Select Edit photo
		self.selectEditPhoto()
		
		// Select Take Photo
		self.clickButton(name: "Take Photo", query: app.sheets.buttons, delay: 3)
	}
	
	/*
	Name: selectTop()
	Description: Select the top center of the screen
	Parameters: none
	Example: selectTop()
	*/
	func selectTop() {
		let x: Int = Int(app.frame.width)/2		// center
		let y: Int = Int(app.frame.height)/10	// top 10th
		app.tapAtPosition(X: x, Y: y)
	}
	
	/*
	Name: selectTop3rd()
	Description: Select the top 3rd of the screen
	Parameters: none
	Example: selectTop3rd()
	*/
	func selectTop3rd() {
		let x: Int = Int(app.frame.width)/2		// center
		let y: Int = Int(app.frame.height)/3	// top 3rd
		app.tapAtPosition(X: x, Y: y)
	}
	
	/*
	Name: selectVideoCenter(channel:)
	Description: Select Video Center Channel
	Parameters: channel
	Example: selectVideoCenter(channel: "Sorenson")
	*/
	func selectVideoCenter(channel: String) {
		self.clickStaticText(text: channel, query: app.tables.staticTexts, delay: 3)
	}
	
	/*
	Name: selectVideoDoneBtn()
	Description: Select Done button after playing Video Center Video
	Parameters: none
	Example: selectVideoDoneBtn()
	*/
	func selectVideoDoneBtn() {
		// Select the Done button
		if (app.buttons["Done"].isHittable) {
			self.clickButton(name: "Done", delay: 5)
		}
		else {
			app.buttons["Done"].forceTapElement()
			self.clickButton(name: "Done", delay: 5)
		}
	}
	
	/*
	Name: selectViewCallHistory()
	Description: Select View Call History for a contact
	Parameters: none
	Example: selectViewCallHistory()
	*/
	func selectViewCallHistory() {
		// Select the View Call History
		self.clickStaticText(text: "View Call History", query: app.tables.staticTexts, delay: 5)
	}
	
	/*
	Name: sendSignMailToCallHistoryItem(:name:num)
	Description: Send a SignMail to a call history item
	Parameters: name
				num
	Example: sendSignMailToCallHistoryItem(name: "test", num: "18015757669")
	*/
	func sendSignMailToCallHistoryItem(name: String, num: String) {
		// Go to Enhanced SignMail
		self.gotoEnhancedSignMail()
		
		// Select Call History
		if #available(iOS 13.0, *) {
			app.tabBars.element(boundBy: 1).buttons["History"].tap()
		}
		else {
			app.tabBars.element(boundBy: 0).buttons["History"].tap()
		}
		
		// Select Call History Item
		let phnum = self.formatPhoneNumber(num: num)
		
		// P2P Call
		if (app.tables.cells.staticTexts[phnum].exists) {
			app.tables.cells.containing(.staticText, identifier: phnum).staticTexts[name].tap()
		}
		// VRS Call
		else {
			app.tables.cells.containing(.staticText, identifier: phnum + " / VRS").staticTexts[name].tap()
		}
		sleep(3)
	}
	
	/*
	Name: sendSignMailToContact(:name)
	Description: Send a SignMail to a contact
	Parameters: name
	Example: sendSignMailToContact(name: )
	*/
	func sendSignMailToContact(name: String) {
		// Go to Enhanced SignMail and select Contacts
		self.gotoEnhancedSignMailContacts()
		self.clickSegmentedControlButton(name: "Contacts")
		
		// Select Contact
		self.clickStaticText(text: name, delay: 3)
	}
	
	/*
	Name: sendSignMailToFavorite(:name)
	Description: Send a SignMail to a favorite
	Parameters: name
	Example: sendSignMailToFavorite(name: )
	*/
	func sendSignMailToFavorite(name: String) {
		// Go to Enhanced SignMail and select Favorites
		self.gotoEnhancedSignMailContacts()
		self.clickSegmentedControlButton(name: "Favorites")
		
		// Select Favorite
		self.clickStaticText(text: name, delay: 3)
	}
	
	/*
	Name: sendSignMailToNum(:phnum)
	Description: Send a SignMail using the Dialer
	Parameters: phnum (must not be formatted)
	Example: sendSignMailToNum(phnum: )
	*/
	func sendSignMailToNum(phnum: String) {
		// Go to Enhanced SignMail
		self.gotoEnhancedSignMail()
		
		// Select Dial
		if #available(iOS 13.0, *) {
			app.tabBars.element(boundBy: 1).buttons["Dial"].tap()
		}
		else {
			app.tabBars.element(boundBy: 0).buttons["Dial"].tap()
		}

		// Dial number
		self.dial(phnum: phnum)
		
		// Press the Record button
		self.clickButton(name: "Call")
	}
	
	/*
	Name: setHomeScreenVCO(setting:)
	Description: Set ad-hoc 1-Line VCO on or off on home page
	Parameters: setting
	Example: setHomeScreenVCOVCO(setting: 1)
	*/
	func setHomeScreenVCO(setting: Int) {
		//enable VCO
		if setting == 1 {
			if !app.switches["1"].exists {
				app.switches["0"].tap()
				sleep(1)
			}
		}
		//Disable VCO
		else {
			if app.switches["1"].exists {
				app.switches["1"].tap()
				sleep(1)
			}
		}
	}
	
	/*
	Name: setSettingsSwitch(setting:value:)
	Description: Set the settings switch by setting and value
	Parameters: setting, value
	Example: setSettingsSwitch(setting: "Encrypt My Calls", value: true)
	*/
	func setSettingsSwitch(setting: String, value: Bool, save: Bool = false) {
		// Tap to scroll to setting
		self.clickStaticText(text: setting, query: app.tables.staticTexts)
		
		// Get the switch state
		let settingSwitch = app.tables.switches[setting]
		let switchState = settingSwitch.value as? String
		var toggled = false
		
		// Set the value
		switch value {
		case true:
			if switchState == "0" {
				settingSwitch.tap()
				toggled = true
			}
			break
		case false:
			if switchState == "1" {
				settingSwitch.tap()
				toggled = true
			}
			break
		}
		if toggled && save {
			selectSave()
		}
	}
	
	/*
	Name: scrollFavorites(dir:)
	Description: Scroll the favorites list
	Parameters: dir ("Up", "Down")
	Example: scrollFavorites(dir: "Up")
	*/
	func scrollFavorites(dir: String) {
		switch dir {
			case "Up":
				app.tables.element(boundBy: 0).swipeDown()
				break
			case "Down":
				app.tables.element(boundBy: 0).swipeUp()
				break
			default:
				XCTFail("Scroll failed: use Up, or Down")
		}
		sleep(3)
	}
	
	/*
	Name: share(:delay)
	Description: Select the Share button
	Parameters: delay
	Example: share()
	*/
	func share(delay: UInt32 = 3) {
		// Select the Share button
		if (app.buttons["Share"].exists) {
			app.buttons["Share"].tap()
		}
		else {
			app.windows.element(boundBy: 0).tap()
			app.buttons["Share"].tap()
		}
		sleep(delay)
	}
	
	/*
	Name: shareContact(name:)
	Description: Share Contact
	Parameters: name
	Example: shareContact(name:"Automate 1234")
	*/
	func shareContact(name: String) {
		// Select the Share Contact
		self.share()
		self.clickButton(name: "ShareContact", delay: 3)
		
		// Select Contact Name
		self.selectContact(name: name)
		sleep(3)
		
		// Tap window to close keyboard
		if #available(iOS 13, *) {
			self.selectTop3rd()
			sleep(1)
		}
	}
	
	/*
	Name: shareContact(name:type:)
	Description: Share Contact
	Parameters: name
				type ("home", "work", or "mobile")
	Example: shareContact(name:"Automate 1234" , type:"home")
	*/
	func shareContact(name: String, type: String) {
		// Select the Share Contact
		self.share()
		self.clickButton(name: "ShareContact", delay: 3)
		
		// Select Contact Name
		self.selectContact(name: name)
		self.clickStaticText(text: type, delay: 3)
		
		// Tap window to close keyboard
		if #available(iOS 13, *) {
			self.selectTop3rd()
			sleep(1)
		}
	}
	
	/*
	Name: shareLocation()
	Description: Share Location
	Parameters: none
	Example: shareLocation()
	*/
	func shareLocation() {
		// Select Share Location
		self.share()
		self.clickButton(name: "ShareLocation", delay: 10)
		
		// Wait for Location
		let confirmBtn = self.waitFor(query: app.buttons, value: "Confirm")
		XCTAssertTrue(confirmBtn.exists, "Failed to get location")
		
		// Share Location
		self.clickButton(name: "Confirm", delay: 2)
		
		// Tap window to close keyboard
		self.selectTop3rd()
		sleep(1)
	}
	
	/*
	Name: shareSavedText(text:)
	Description: Share Saved Text
	Parameters: text
	Example: shareSavedText(text:"1234")
	*/
	func shareSavedText(text: String) {
		// Select the Share Text
		self.share()
		sleep(3)
		
		// Select Saved Text
		self.selectSavedText()
		selectSavedTextItem(text: text)
		
		if #available(iOS 13.0, *) {
			// close keyboard
			self.closeShareTextKeyboard()
		}
	}
	
	/*
	Name: shareText(text:closeKeyboard:)
	Description: Share Text and close keyboard
	Parameters: text, closeKeyboard
	Example: shareText(text:"1234")
	*/
	func shareText(text: String, closeKeyboard: Bool = true) {
		// Select the Share Text
		self.share()
		sleep(3)
		
		// Enter Text
		app.typeText(text)
		sleep(1)
		
		// close keyboard
		if closeKeyboard {
			self.closeShareTextKeyboard()
		}
	}
	
	/*
	Name: showCallStats()
	Description: Show Call Stats
	Parameters: none
	Example: showCallStats()
	*/
	func showCallStats() {
		// Select the More button
		if (app.buttons["Call Options"].exists) {
			app.buttons["Call Options"].tap()
		}
		else {
			app.windows.element(boundBy: 0).tap()
			app.buttons["Call Options"].tap()
		}
		sleep(3)
		
		// Select Show Call Stats
		self.clickButton(name: "Show Call Stats", query: app.sheets.buttons, delay: 3)
	}
	
	/*
	Name: skipGreeting()
	Description: Skip the SignMail Greeting
	Parameters: none
	Example: skipGreeting()
	*/
	func skipGreeting() {
		// Select the Skip Greeting button
		if (app.buttons["Skip"].exists) {
			app.buttons["Skip"].tap()
		}
		else {
			self.selectTop()
			if(app.buttons["Skip"].exists) {
				app.buttons["Skip"].tap()
			}
		}
	}
	
	/*
	Name: switchAccounts()
	Description: Log out and log in as parameter-defined account
	Parameters: number, password
	Example: switchAccounts("18765432222", "1234")
	*/
	func switchAccounts(number: String, password: String) {
		self.logout()
		self.login(phnum: number, password: password)
	}
	
	/*
	Name: switchCall()
	Description: Select the Switch Call Button
	Parameters: none
	Example: switchCall()
	*/
	func switchCall() {
		// Select the call options button and Switch Calls
		self.selectCall(option: "Switch Calls")
		sleep(5)
	}
	
	/*
	Name: takePhoto()
	Description: Take a photo for the contact photo
	Parameters: none
	Example: takePhoto()
	*/
	func takePhoto() {
		// Select Take Photo
		self.selectTakePhoto()
		
		// Take Photo
		self.clickButton(name: "PhotoCapture", delay: 3)
		
		// Select Use Photo
		self.clickButton(name: "Use Photo", delay: 3)
	}
	
	/*
	Name: toggleTabs(setting:)
	Description: Toggle the to Settings and Home tab
	Parameters: none
	Example: toggleTabs()
	*/
	func toggleTabs() {
		self.gotoSettings()
		self.gotoHome()
	}
	
	/*
	Name: toggleMicrophone(setting:)
	Description: Toggle the microphone on the home screen
	Parameters: setting
	Example: toggleMicrophone(setting: 1)
	*/
	func toggleMicrophone(setting: Int) {
		// Enable Microphone
		if setting == 1 {
			if app.buttons.matching(identifier: "Microphone Off").element(boundBy: 1).exists {
				app.buttons.matching(identifier: "Microphone Off").element(boundBy: 1).tap()
				sleep(1)
				self.toggleTabs()
			}
		}
		// Disable Microphone
		else {
			if app.buttons.matching(identifier: "Microphone On").element(boundBy: 1).exists {
				app.buttons.matching(identifier: "Microphone On").element(boundBy: 1).tap()
				sleep(1)
				self.toggleTabs()
			}
		}
	}
	
	/*
	Name: toggleStats()
	Description: Toggle Stats
	Parameters: none
	Example: toggleStats()
	*/
	func toggleStats() {
		self.enableKeyCodes()
		app.buttons["3"].tap()
		app.buttons["4"].tap()
		app.buttons["9"].tap()
		app.buttons["Call"].tap()
		self.clickButton(name: "OK")
	}
	
	/*
	Name: toggleSpanishContact(setting:)
	Description: Set Enable Spanish Features on or off in Contact
	Parameters: setting
	Example: toggleSpanishContact(setting: 0)
	*/
	func toggleSpanishContact(setting: Int) {
		if #available(iOS 13.0, *) {
			// Get Spanish VRS Switch State
			let switchState = app.tables.switches.matching(identifier: "Spanish VRS").element.debugDescription
			
			// Turn on Spanish VRS
			if (setting == 1) {
				if switchState.contains("value: 0") {
					app.tables.switches.matching(identifier: "Spanish VRS").element.tap()
					sleep(1)
				}
			}
			// Turn off Spanish VRS
			else {
				if switchState.contains("value: 1") {
					app.tables.switches.matching(identifier: "Spanish VRS").element.tap()
					sleep(1)
				}
			}
		}
		else {
			// Get Spanish VRS Switch State
			let switchState = app.tables.switches.matching(identifier: "Spanish VRS").element.debugDescription
			
			// Turn on Spanish VRS
			if (setting == 1) {
				if switchState.contains("value: 0") {
					app.tables.switches.matching(identifier: "Spanish VRS").element.tap()
					sleep(1)
				}
			}
			// Turn off Spanish VRS
			else {
				if switchState.contains("value: 1") {
					app.tables.switches.matching(identifier: "Spanish VRS").element.tap()
					sleep(1)
				}
			}
		}
		self.clickButton(name: "Done", delay: 3)
	}
	
	/*
	Name: toggleTunneling()
	Description: Toggle Tunneling
	Parameters: setting
	Example: toggleTunneling()
	*/
	func toggleTunneling() {
		app.tables.staticTexts.matching(identifier: "Tunneling Enabled").element(boundBy: 0).tap()
		app.tables.switches.matching(identifier: "Tunneling Enabled").element(boundBy: 0).tap()
	}
	
	/*
	Name: transferCall(toContact:type:)
	Description: Transfer call a contact
	Parameters: toContact, type ("home", "work", "mobile")
	Example: transferCall(toContact: "Automate 3694", type: "home")
	*/
	func transferCall(toContact: String, type: String) {
		// Select Call Options - Transfer
		self.selectCall(option: "Transfer Call")
		
		// Select Contacts
		self.gotoCallTab(button: "Contacts")
		
		// Select Contact and type
		self.callContact(name: toContact, type: type)
		sleep(3)
	}
	
	/*
	Name: transferCall(toNum:)
	Description: Transfer call to a number
	Parameters: toNum
	Example: transferCall(toNum: "18015757669")
	*/
	func transferCall(toNum: String) {
		testUtils.calleeNum = toNum
		
		// Transfer Call
		self.selectCall(option: "Transfer Call")
		
		// Select Dial
		self.gotoCallTab(button: "Dial")
		
		// Dial Number
		self.dialInCall(phnum: toNum)
		
		// Press the Transfer button
		app.windows.element(boundBy: 1).buttons["Call"].tap()
		sleep(3)
	}
	
	/*
	Name: useVoice(setting:)
	Description: Set Use Voice on or off
	Parameters: setting
	Example: useVoice(setting: 0)
	*/
	func useVoice(setting: Int) {
		// Tap Use Voice to scroll
		app.tables.staticTexts.matching(identifier: "Use voice").element.tap()
		
		if #available(iOS 13.0, *) {
			// Get Use Voice Switch State
			let switchState = app.tables.switches.matching(identifier: "Use voice").element.debugDescription
			
			// Turn on Voice
			if (setting == 1) {
				if switchState.contains("value: 0") {
					app.tables.switches.matching(identifier: "Use voice").element.tap()
					sleep(3)
				}
			}
			// Turn off Voice
			else {
				if switchState.contains("value: 1") {
					app.tables.switches.matching(identifier: "Use voice").element.tap()
					sleep(3)
				}
			}
		}
		else {
			// Get Use Voice Switch State
			let switchState = app.tables.switches.matching(identifier: "Use voice").element.debugDescription
			
			// Turn on Voice
			if (setting == 1) {
				if switchState.contains("value: 0") {
					app.tables.switches.matching(identifier: "Use voice").element.tap()
					sleep(3)
				}
			}
			// Turn off Voice
			else {
				if switchState.contains("value: 1") {
					app.tables.switches.matching(identifier: "Use voice").element.tap()
					sleep(3)
				}
			}
		}
		self.heartbeat()
		self.gotoSettings()
	}
	
	/*
	Name: verifyAddToFavoritesBtn(visible:)
	Description: Verify if Add To Favorites button is visible or not
	Parameters:	visible
	Example: verifyAddToFavoritesBtn(visible: false)
	*/
	func verifyAddToFavoritesBtn(visible: Bool = true) {
		if (visible) {
				XCTAssertTrue(app.tables.cells.staticTexts["Add To Favorites"].exists)
		} else {
			XCTAssertTrue(!app.tables.cells.staticTexts["Add To Favorites"].exists)
		}
	}

	/*
	Name: verifyAddCalltoSameCalleeFails()
	Description: Verify Call Failed message when adding another call to
	the same callee
	Parameters: none
	Example: verifyAddCalltoSameCalleeFails()
	*/
	func verifyAddCalltoSameCalleeFails() {
		XCTAssertTrue(app.alerts.staticTexts["Failed to make call"].exists)
		XCTAssertTrue(app.alerts.staticTexts["A call to this number is already in progress."].exists)
		app.alerts.buttons["OK"].tap()
		sleep(2)
	}
	
	/*
	Name: verifyAudioEnabled()
	Description: Verify Audio is Enabled in Settings
	Parameters:	none
	Example: verifyAudioEnabled()
	*/
	func verifyAudioEnabled() {
		let switchState = app.tables.switches.matching(identifier: "Audio Enabled").element(boundBy: 0).value.debugDescription
		XCTAssertTrue(switchState.contains("1"))
	}
	
	/*
	Name: verifyAudioDisabled()
	Description: Verify Audio is Disabled in Settings
	Parameters:	none
	Example: verifyAudioDisabled()
	*/
	func verifyAudioDisabled() {
		let switchState = app.tables.switches.matching(identifier: "Audio Enabled").element(boundBy: 0).value.debugDescription
		XCTAssertTrue(switchState.contains("0"))
	}
	
	
	/*
	Name: verifyAudioDisabledMessage()
	Description: Verify Audio Disabled message
	Parameters:	none
	Example: verifyAudioDisabledMessage()
	*/
	func verifyAudioDisabledMessage() {
		XCTAssertTrue(app.alerts.staticTexts["Network Speed too low for Audio"].exists)
		app.alerts.buttons["OK"].tap()
	}
	
	/*
	Name: verifyAvailableThemes()
	Description: Verify available themes
	Parameters:	none
	Example: verifyAvailableThemes()
	*/
	func verifyAvailableThemes() {
		self.verifyText(text: "Dark (default)", expected: true)
		self.verifyText(text: "Light", expected: true)
		self.verifyText(text: "High Contrast", expected: true)
	}
	
	/*
	Name: verifyBlankPassword()
	Description: Verify Password cannot be blank
	Parameters:	none
	Example: verifyBlankPassword()
	*/
	func verifyBlankPassword() {
		XCTAssertTrue(app.alerts.staticTexts["Your password cannot be blank."].exists)
		app.alerts.buttons["OK"].tap()
	}
	
	/*
	Name: verifyBlockContactsOrder(list:)
	Description: Verify the Blocked contacts List order
	Parameters: list
	Example: verifyBlockContactsOrder(list: contactsList)
	*/
	func verifyBlockContactsOrder(list: [String]) {
		// get Blocked contact Lists
		let list1 = list
		let list2 = getBlockList()
		
		// Verify Blocked Lists match
		XCTAssertEqual(list1.count, list2.count)
		let count = list1.count
		//for var i = 0; i < count; i+=1 {
		for i in 0 ..< count {
			XCTAssertEqual(list1[i], list2[i])
		}
	}
	
	/*
	Name: verifyBlockListNotDisplaying()
	Description: Verify the Block List is not displaying in the Add Call Interface
	Parameters: none
	Example: verifyBlockListNotDisplaying()
	*/
	func verifyBlockListNotDisplaying() {
		XCTAssertFalse(app.staticTexts["Blocked Numbers"].exists)
	}
	
	/*
	Name: verifyBlockedNumber(name:)
	Description: Verify blocked number by name
	Parameters: name
	Example: verifyBlockedNumber(name: "Automate 558")
	*/
	func verifyBlockedNumber(name: String) {
		XCTAssertTrue(app.staticTexts[name].exists)
	}
	
	/*
	Name: verifyBlockedNumber(num:)
	Description: Verify blocked number
	Parameters: num
	Example: verifyBlockedNumber(num: "15558018475")
	*/
	func verifyBlockedNumber(num: String) {
		// Format Number
		let phnum = self.formatPhoneNumber(num: num)

		// Verify Number
		XCTAssertTrue(app.textFields[phnum].exists)
	}
	
	/*
	Name: verifyBlockedNumbers(name:count:)
	Description: Verify blocked numbers by name
	Parameters: name
				count
	Example: verifyBlockedNumbers(name: "Automate 558", count: 3)
	*/
	func verifyBlockedNumbers(name: String, count: Int) {
		let blocklist = self.getBlockList()
		let blocklistCount = blocklist.count
		var tally = count
		for i in 0..<blocklistCount {
			if blocklist[i] == name {
				tally -= 1
			}
		}
		XCTAssertTrue(tally == 0)
	}
	
	/*
	Name: verifyBlockedOrder(list:)
	Description: Verify the Blocked List order
	Parameters: list
	Example: verifyBlockedOrder(list: contactsList)
	*/
	func verifyBlockedOrder(list: [String]) {
		// Get Blocked Lists
		let list1 = list
		let list2 = getBlockList()
		
		// Verify Blocked Lists match
		XCTAssertEqual(list1.count, list2.count)
		let count = list1.count
		//for var i = 0; i < count; i+=1 {
		for i in 0 ..< count {
			XCTAssertEqual(list1[i], list2[i])
		}
	}
	
	/*
	Name: verifyBlockedSignMail(name:)
	Description: Verify Blocked SignMail by name
	Parameters: name
	Example: verifyBlockedSignMail(name: "Automate 4199")
	*/
	func verifyBlockedSignMail(name: String) {
		self.verifyContact(name: name)
		XCTAssertTrue(app.images["BlockedSignMail"].exists)
	}
	
	/*
	Name: verifyBlockedString(string:)
	Description: Verify blocked string or description
	Parameters: string
	Example: verifyBlockedString(string: "test")
	*/
	func verifyBlockedString(string: String) {
		// Verify description
		XCTAssertTrue(app.textFields[string].exists)
	}
	
	/*
	Name: verifyButtonEnabled(string:bool:)
	Description: Verify if button is enabled or disabled as expected
	Parameters: name, expected
	Example: verifyButtonEnabled(name: "Video With Text", expected: true)
	*/
	func verifyButtonEnabled(name: String, expected: Bool) {
		if expected {
			XCTAssert(app.buttons[name].isEnabled)
		} else {
			XCTAssert(!app.buttons[name].isEnabled)
		}
	}
	
	/*
	Name: verifyButtonExists(string:bool:)
	Description: Verify if button is visible or not as expected
	Parameters: name, expected
	Example: verifyButtonExists(name: "Record", expected: true)
	*/
	func verifyButtonExists(name: String, expected: Bool = true) {
		if expected {
			XCTAssert(app.buttons[name].exists)
		} else {
			XCTAssert(!app.buttons[name].exists)
		}
	}
	
	/*
	Name: verifyButtonSelected(string:bool:)
	Description: Verify if button is selected or not as expected
	Parameters: name, expected
	Example: verifyButtonSelected(name: "Video With Text", expected: true)
	*/
	func verifyButtonSelected(name: String, expected: Bool) {
		if expected {
			XCTAssert(app.buttons[name].isSelected)
		} else {
			XCTAssert(!app.buttons[name].isSelected)
		}
	}
	
	/*
	Name: verifyCallEncryptionFailed()
	Description: Verify Call Not Encrypted message
	Parameters:	none
	Example: verifyCallEncryptionFailed()
	*/
	func verifyCallEncryptionFailed() {
		let message = "The videophone you are calling can’t support encrypted calls. Change your encryption settings or contact your IT administrator."
		self.waitForCallIsNotEncrypted()
		XCTAssertTrue(app.alerts.staticTexts[message].exists)
		app.alerts.buttons["Cancel"].tap()
	}
	
	/*
	Name: verifyCallFailed()
	Description: Verify Call Failed message
	Parameters:	none
	Example: verifyCallFailed()
	*/
	func verifyCallFailed(message: String = "Your call was not answered.") {
		XCTAssertTrue(app.alerts.staticTexts[message].exists)
		app.alerts.buttons["Cancel"].tap()
	}
	
	/*
	Name: verifyCallHistory(count:)
	Description: Verify the call history count
	Parameters: count
	Example: verifyCallHistory(count: 0)
	**/
	func verifyCallHistory(count: UInt) {
		let rows = app.tables.cells.count
		XCTAssertEqual(rows, Int(count))
	}
	
	/*
	Name: verifyCallHistory(num:)
	Description: Verify Call History item by Number
	Parameters: num
	Example: verifyCallHistory(num: "15553334444")
	*/
	func verifyCallHistory(num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		let callHistoryList = self.getCallHistoryList()
		
		// verify phnum
		if app.tables.cells.staticTexts[phnum].exists {
			for i in 0 ..< callHistoryList.count {
				if callHistoryList[i].contains(phnum) {
					XCTAssertTrue(callHistoryList[i].contains(phnum))
					break
				}
			}
		}
		else if app.tables.cells.staticTexts[phnum + " / VRS"].exists {
			for i in 0 ..< callHistoryList.count {
				if callHistoryList[i].contains(phnum + " / VRS") {
					XCTAssertTrue(callHistoryList[i].contains(phnum + " / VRS "))
					break
				}
			}
		}
		// phnum not found
		else {
			XCTFail(phnum + " not found")
		}
	}
	
	/*
	Name: verifyCallHistory(name:num:)
	Description: Verify Call History item by Name and Number
	Parameters: name
				num
	Example: verifyCallHistory(name: "Automate 5441", num: "15553334444")
	*/
	func verifyCallHistory(name: String, num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		let callHistoryList = self.getCallHistoryList()
		
		// P2P Call
		if app.tables.cells.staticTexts[phnum].exists {
			for i in 0 ..< callHistoryList.count {
				if callHistoryList[i].contains(phnum) {
					XCTAssertTrue(callHistoryList[i].contains(phnum + " " + name))
					break
				}
			}
		}
		// VRS Call
		else if app.tables.cells.staticTexts[phnum + " / VRS"].exists {
			for i in 0 ..< callHistoryList.count {
				if callHistoryList[i].contains(phnum + " / VRS") {
					XCTAssertTrue(callHistoryList[i].contains(phnum + " / VRS " + name))
					break
				}
			}
		} // VRS No Number
		else if phnum == "" {
			for i in 0 ..< callHistoryList.count {
				XCTAssertTrue(callHistoryList[i].contains("VRS " + name))
				break
			}
		}
		// phnum not found
		else {
			XCTFail(phnum + " not found")
		}
	}
	
	/*
	Name: verifyCallHistoryEmpty()
	Description: Verify the call history empty string
	Parameters: none
	Example: verifyCallHistory()
	*/
	func verifyCallHistoryEmpty() {
		XCTAssertTrue(app.tables.staticTexts["You don't have any calls."].exists)
	}
	
	/*
	Name: verifyCallHistoryFilter(list:)
	Description: Verify the selected call history filter
	Parameters: list
	Example: verifyCallHistoryFilter(list: "All")
	*/
	func verifyCallHistoryFilter(list: String) {
		XCTAssertTrue(app.tables.buttons[list].isSelected)
	}
	
	/*
	Name: verifyCallHistoryOrder(list:)
	Description: Verify the Call History order
	Parameters: list
	Example: verifyCallHistoryOrder(list: callHistoryList)
	*/
	func verifyCallHistoryOrder(list: [String]) {
		// Get Blocked Lists
		let list1 = list
		let list2 = self.getCallHistoryList()
		
		// Verify Blocked Lists match
		XCTAssertEqual(list1.count, list2.count)
		let count = list1.count
		
		for i in 0 ..< count {
			XCTAssertEqual(list1[i], list2[i])
		}
	}
	
	/*
	Name: verifyCallerID(text:)
	Description: verify Caller ID
	Parameters: text
	Example: verifyCallerID(text: "Customer Care")
	*/
	func verifyCallerID(text: String) {
		if (!app.staticTexts[text].exists) {
			self.selectTop3rd()
		}
		XCTAssertTrue(app.staticTexts[text].exists)
	}
	
	/*
	Name: verifyCalling(num:)
	Description: verify calling by phone num
	Parameters: num
	Example: verifyCalling(num: oepNum)
	*/
	func verifyCalling(num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		XCTAssertTrue(app.staticTexts["Calling..."].exists)
		XCTAssertTrue(app.staticTexts[phnum].exists)
	}
	
	/*
	Name: verifyCallOptions()
	Description: Verify Call Options
	Parameters: none
	Example: verifyCallOptions()
	*/
	func verifyCallOptions() {
		// Verify Call Options
		XCTAssertTrue(app.buttons["Make Another Call"].exists)
		XCTAssertTrue(app.buttons["Dialpad"].exists)
		XCTAssertTrue(app.buttons["Transfer Call"].exists)
	}
	
	/*
	Name: verifyCannotAddContact()
	Description: Verify additional contacts cannot be added
	Parameters: none
	Example: verifyCannotAddContact()
	*/
	func verifyCannotAddContact() {
		XCTAssertTrue(app.alerts.staticTexts["Unable to Add Contact"].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyCannotDialOwnNumber()
	Description: Verify cannot dial own phone number
	Parameters: none
	Example: verifyCannotDialOwnNumber()
	*/
	func verifyCannotDialOwnNumber() {
		XCTAssertTrue(app.alerts.staticTexts["Failed to make call"].exists)
		XCTAssertTrue(app.alerts.staticTexts["You cannot call your own Sorenson phone number from this device."].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyCannotSendSignMailToSelf()
	Description: Verify cannot send SignMail to self
	Parameters: none
	Example: verifyCannotSendSignMailToSelf()
	*/
	func verifyCannotSendSignMailToSelf() {
		XCTAssertTrue(app.alerts.staticTexts["Cannot send SignMail"].exists)
		XCTAssertTrue(app.alerts.staticTexts["Sorry, you can’t send a SignMail to yourself."].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
		self.selectCancel()
	}
	
	/*
	Name: verifyContactDefaultPhoto(name:)
	Description: Verify the Contact default photo
	Parameters: contactName
	Example: verifyContactDefaultPhoto(name: "Automate 5395")
	**/
	func verifyContactDefaultPhoto(name: String) {
		// Verify Contact Default Photo
		// default photo ID: 00000000-0000-0000-0000-000000000001
		let defaultPhoto = String(format: "%@ 00000000-0000-0000-0000-000000000001", name)
		XCTAssertTrue(app.tables.cells[defaultPhoto].exists)
	}
	
	/*
	Name: verifyContact(company:)
	Description: Verify a contact company name
	Parameters: company
	Example: verifyContact(company: "AutoCO 6998")
	*/
	func verifyContact(company: String) {
		XCTAssertTrue(app.staticTexts[company].exists)
	}
	
	/*
	Name: verifyContact(name:)
	Description: Verify a contact by name
	Parameters: name
	Example: verifyContact(name: "PizzaRev")
	*/
	func verifyContact(name: String) {
		XCTAssertTrue(app.staticTexts[name].exists)
	}
	
	/*
	Name: verifyContact(phnum:type:)
	Description: Verify that the phone number specified exists in the specified type field
	Parameters: phnum
				type ("home", "work", or "mobile")
	Example: verifyContact(phnum: "1 (555) 444-2200", type: "home")
	*/
	func verifyContact(phnum: String, type: String) {
		let formattedNum = formatPhoneNumber(num: phnum)
		XCTAssertTrue(app.tables.cells.containing(.staticText, identifier: type).staticTexts[formattedNum].exists)
	}
	
	/*
	Name: verifyContactCount(count:)
	Description: Verify the contact count
	Parameters: count
	Example: verifyContactCount(count: 0)
	*/
	func verifyContactCount(count: UInt) {
		let rows = app.tables.cells.count
		XCTAssertEqual(rows, Int(count))
	}
	
	/*
	Name: verifyContactCount(name:count:)
	Description: Verify the number of contacts of a given name
	Parameters: name, count
	Example: verifyContactCount(count: 0)
	*/
	func verifyContactCount(name: String, count: Int) {
		// get actual count, compare to count
		let trueCount = self.getContactCount(name: name)
		XCTAssertEqual(count, trueCount)
	}
	
	/*
	 Name: verifyContactHeaders()
	 Description: Verify that all headers in contact pager are present
	 Parameters: none
	 Example: verifyContactHeaders()
	 **/
	func verifyContactHeaders() {
		// Select the Add button
		self.clickButton(name: "Add", query: app.navigationBars.buttons)
		
		XCTAssertTrue(app.staticTexts["home"].exists)
		XCTAssertTrue(app.staticTexts["work"].exists)
		XCTAssertTrue(app.staticTexts["mobile"].exists)
		self.clickButton(name: "Cancel")
	}
	
	/*
	Name: verifyContactPhoto(name:)
	Description: Verify a Contact Photo by Name
	Parameters: contactName
	Example: verifyContactPhoto(name: "Automate 5395")
	*/
	func verifyContactPhoto(name: String) {
		// Verify Contact
		self.verifyContact(name: name)
		
		// Verify No Default Photo
		// default photo ID: 00000000-0000-0000-0000-000000000001
		let defaultPhoto = String(format: "%@ 00000000-0000-0000-0000-000000000001", name)
		XCTAssertFalse(app.tables.cells[defaultPhoto].exists)
		
		// Verify Photo not removed
		// Removed photo ID: 00000000-0000-0000-0000-000000000000
		let removedPhoto = String(format: "%@ 00000000-0000-0000-0000-000000000000", name)
		XCTAssertFalse(app.tables.cells[removedPhoto].exists)
	}
	
	/*
	Name: verifyContactPosition(name:position:)
	Description: Verify Contact Position
	Parameters: name
	position
	Example: verifyContactPosition(name: "Automate 5395", position: 0)
	*/
	func verifyContactPosition(name: String, position: Int) {
		let contactsList = self.getContactsList()
		XCTAssertEqual(contactsList[position], name)
	}
	
	/*
	Name: verifyContactsOrder(list:)
	Description: Verify the Contacts List order
	Parameters: list
	Example: verifyContactsOrder(list: contactsList)
	*/
	func verifyContactsOrder(list: [String]) {
		// get Contact Lists
		let list1 = list
		let list2 = getContactsList()
		
		// Verify Contact Lists match
		XCTAssertEqual(list1.count, list2.count)
		let count = list1.count
		//for var i = 0; i < count; i+=1 {
		for i in 0 ..< count {
			XCTAssertEqual(list1[i], list2[i])
		}
	}
	
	/*
	Name: verifyContactView()
	Description: Verify Contact view
	Parameters:	none
	Example: verifyContactView()
	*/
	func verifyContactView() {
		XCTAssertTrue(app.textFields["Name"].exists)
		XCTAssertTrue(app.textFields["Company Name"].exists)
		XCTAssertTrue(app.staticTexts["home"].exists)
		XCTAssertTrue(app.staticTexts["work"].exists)
		XCTAssertTrue(app.staticTexts["mobile"].exists)
	}
	
	/*
	Name: verifyDialerContact(name:)
	Description: Verify a contact by name on the dialer
	Parameters: name
	Example: verifyDialerContact(name: "PizzaRev")
	*/
	func verifyDialerContact(name: String) {
		XCTAssertTrue(app.buttons[name].exists)
	}
	
	/*
	Name: verifyDialerNumber()
	Description: Verify Phone number below the dialer
	Parameters:	none
	Example: verifyDialerNumber(num: String)
	*/
	func verifyDialerNumber(num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		XCTAssertTrue(app.textViews[phnum].exists)
	}
	
	/*
	Name: verifyDialerSelfView()
	Description: Verify the Dialer Self View
	Parameters: none
	Example: verifyDialerSelfView()
	*/
	func verifyDialerSelfView() {
		XCTAssertTrue(app.windows.otherElements["DialerSelfView"].exists)
	}
	
	/*
	Name: verifyDoneButton()
	Description: Verify the done button
	Parameters: none
	Example: verifyEditButton()
	*/
	func verifyDoneButton() {
		XCTAssertTrue(app.buttons["Done"].exists)
	}
	
	/*
	Name: verifyDoneButtonDisabled()
	Description: Verify Done button disabled
	Parameters: none
	Example: verifyDoneButtonDisabled()
	**/
	func verifyDoneButtonDisabled() {
		XCTAssertTrue(!app.buttons["Done"].isEnabled)
	}
	
	/*
	Name: verifyDoNotDisturbStatus()
	Description: Verify Do Not Disturb enabled or not
	Parameters: none
	Example: verifyDoNotDisturbStatus(enabled: false)
	**/
	func verifyDoNotDisturbStatus(enabled: Bool = true) {
		if enabled {
			XCTAssertTrue(app.textViews["Do Not Disturb. You are not receiving calls."].exists)
		} else {
			XCTAssertFalse(app.textViews["Do Not Disturb. You are not receiving calls."].exists)
		}
	}
	
	/*
	Name: verifyEditButton()
	Description: Verify the Edit button
	Parameters: none
	Example: verifyEditButton()
	*/
	func verifyEditButton() {
		XCTAssertTrue(app.buttons["Edit"].exists)
	}
	
	/*
	Name: verifyEditContact(name:)
	Description: Verify a contact by name on the Edit Screen
	Parameters: name
	Example: verifyEditContact(name: "Automate")
	*/
	func verifyEditContact(name: String) {
		XCTAssertTrue(app.tables.textFields[name].exists)
	}
	
	/*
	Name: verifyEmail1(email:)
	Description: Verify email1 address
	Parameters: email
	Example: verifyEmail1(email: "test@test.com")
	**/
	func verifyEmail1(email: String) {
		XCTAssertTrue(app.textFields["Email1TextField"].value.debugDescription.contains(email))
	}
	
	/*
	Name: verifyEmail2(email:)
	Description: Verify email2 address
	Parameters: email
	Example: verifyEmail2(email: "test@test.com")
	**/
	func verifyEmail2(email: String) {
		XCTAssertTrue(app.textFields["Email2TextField"].value.debugDescription.contains(email))
	}
	
	/*
	Name: verifyEmailTextfields()
	Description: Verify email address text fields
	Parameters: none
	Example: verifyEmailTextfields()
	**/
	func verifyEmailTextfields() {
		XCTAssertTrue(app.tables.staticTexts.matching(identifier: "Email address").element(boundBy: 0).exists)
		XCTAssertTrue(app.tables.textFields["Email1TextField"].exists)
		XCTAssertTrue(app.tables.staticTexts.matching(identifier: "Email address").element(boundBy: 1).exists)
		XCTAssertTrue(app.tables.textFields["Email2TextField"].exists)
	}
	
	/*
	Name: verifyEncryptionIcon()
	Description: Verify Encryption Icon is visible
	Parameters:	none
	Example: verifyEncryptionIcon()
	*/
	func verifyEncryptionIcon() {
		XCTAssertTrue(app.buttons["EncryptionEnabled"].exists)
	}
	
	/*
	Name: verifyEnhancedSignMailDialerBtn()
	Description: Verify Enhanced SignMail dialer button
	Parameters: none
	Example: verifyEnhancedSignMailDialerBtn()
	*/
	func verifyEnhancedSignMailDialerBtn() {
		// Select Dial
		if #available(iOS 13.0, *) {
			app.tabBars.element(boundBy: 1).buttons["Dial"].tap()
		}
		else {
			app.tabBars.element(boundBy: 0).buttons["Dial"].tap()
		}
		
		// Verify Call Button
		XCTAssertTrue(app.buttons["Call"].exists)
	}
	
	/*
	Name: verifyEnhancedSignMailDisabled()
	Description: Verify Enhanced SignMail disabled
	Parameters: none
	Example: verifyEnhancedSignMailDisabled()
	*/
	func verifyEnhancedSignMailDisabled() {
		XCTAssertTrue(!app.navigationBars["SignMail"].buttons["Compose"].exists)
	}
	
	/*
	Name: verifyEnhancedSignMailEnabled()
	Description: Verify Enhanced SignMail enabled
	Parameters: none
	Example: verifyEnhancedSignMailEnabled()
	*/
	func verifyEnhancedSignMailEnabled() {
		XCTAssertTrue(app.navigationBars["SignMail"].buttons["Compose"].exists)
	}
	
	/*
	Name: verifyEnhancedSignMailIcon()
	Description: Verify Enhanced SignMail enabled
	Parameters: none
	Example: verifyEnhancedSignMailIcon()
	*/
	func verifyEnhancedSignMailIcon() {
		XCTAssertTrue(app.images["DirectSignMailIndicator"].exists)
	}
	
	/*
	Name: verifyFavorite(name:type:)
	Description: Verify a Favorite by Favorite Name and Type
	Parameters: name
				type ("home", "work", or "mobile")
	Example: verifyFavorite(name: "Automate 5395", type:"home")
	*/
	func verifyFavorite(name: String, type: String) {
		let favoriteCellName = String(format: "%@ %@", name, type)
		let favoritesList = self.getFavoritesList()
		XCTAssertTrue(favoritesList.contains(favoriteCellName))
	}
	
	/*
	Name: verifyFavoriteDefaultPhoto(name:type:)
	Description: Verify the Favorite default photo
	Parameters: favoriteName
	type ("home", "work", or "mobile")
	Example: verifyFavoriteDefaultPhoto(name: "Automate 5395", type: "home")
	**/
	func verifyFavoriteDefaultPhoto(name: String, type: String) {
		// Verify Favorite
		verifyFavorite(name: name, type: type)
		
		// Verify Favorite Default Photo
		// default photo ID: 00000000-0000-0000-0000-000000000001
		let defaultPhoto = String(format: "%@ 00000000-0000-0000-0000-000000000001", name)
		XCTAssertTrue(app.tables.cells[defaultPhoto].exists)
	}
	
	/*
	Name: verifyFavoritePhoto(name:type:)
	Description: Verify a Photo by Favorite Name and Type
	Parameters: favoriteName
				type ("home", "work", or "mobile")
	Example: verifyFavoritePhoto(name: "Automate 5395", type: "home")
	*/
	func verifyFavoritePhoto(name: String, type: String) {
		// Verify Favorite
		verifyFavorite(name: name, type: type)
		
		// Verify No Favorite Default Photo
		// default photo ID: 00000000-0000-0000-0000-000000000001
		let defaultPhoto = String(format: "%@ 00000000-0000-0000-0000-000000000001", name)
		XCTAssertFalse(app.tables.cells[defaultPhoto].exists)
		
		// Verify Photo not removed
		// Removed photo ID: 00000000-0000-0000-0000-000000000000
		let removedPhoto = String(format: "%@ 00000000-0000-0000-0000-000000000000", name)
		XCTAssertFalse(app.tables.cells[removedPhoto].exists)
	}
	
	/*
	Name: verifyFavoritePosition(favorite:position:)
	Description: Verify Favorite Position
	Parameters: favorite
				position
	Example: verifyFavoritePosition(favorite: "Automate 5395 home", position: 0)
	*/
	func verifyFavoritePosition(favorite: String, position: Int) {
		let favoritesList = self.getFavoritesList()
		XCTAssertEqual(favoritesList[position], favorite)
	}
	
	/*
	Name: verifyFavorites(count:)
	Description: Verify the Favorites count
	Parameters: count
	Example: verifyFavorites(count: 0)
	**/
	func verifyFavorites(count: UInt) {
		let rows = app.tables.cells.count
		XCTAssertEqual(rows, Int(count))
	}
	
	/*
	Name: verifyFavoritesListFull()
	Description: Verify the Favorites list is full message and select OK
	Parameters: none
	Example: verifyFavoritesListFull()
	*/
	func verifyFavoritesListFull() {
		let favoritesFull = self.waitFor(query: app.alerts, value: "Favorites Full")
		XCTAssertTrue(favoritesFull.exists, "Favorites List not full")
		XCTAssertTrue(favoritesFull.staticTexts["The maximum number of allowed favorites has been reached."].exists)
		favoritesFull.buttons["OK"].tap()
	}
	
	/*
	Name: verifyFavoritesMessage()
	Description: Verify the Favorites message when there are 0 favorites
	Parameters: none
	Example: verifyFavoritesMessage()
	*/
	func verifyFavoritesMessage() {
		let favoritesMessage = "Want to create a new Favorite?\n Click \"Add to Favorites\" while looking at a contact."
		XCTAssertTrue(app.tables.staticTexts[favoritesMessage].exists)
	}
	
	/*
	Name: verifyFavoritesPage()
	Description: Verify the Favorites page in the Phonebook
	Parameters:	none
	Example: verifyFavoritesPage()
	*/
	func verifyFavoritesPage() {
		app.swipeDown() // refresh ui
		if #available(iOS 14.0, *) {
			XCTAssertTrue(app.staticTexts["Favorites"].exists)
		}
		else {
			XCTAssertTrue(app.navigationBars["Favorites"].exists)
		}
	}
	
	/*
	Name: verifyFavoriteStar(type:)
	Description: Verify a Favorite Star by Type
	Parameters:	type ("home", "work", or "mobile")
	Example: verifyFavoriteStar(type: "home")
	*/
	func verifyFavoriteStar(type: String) {
		let cell = app.tables.cells.containing(.staticText, identifier:type)
		XCTAssertTrue(cell.staticTexts["★"].exists)
	}
	
	/*
	Name: verifyForgotPassword()
	Description: Verify Forgot Password
	Parameters:	none
	Example: verifyForgotPassword()
	*/
	func verifyForgotPassword() {
		XCTAssertTrue(app.buttons["Forgot Password?"].exists)
	}
	
	/*
	Name: verifyFpsReceivedGreaterThan(value:)
	Description: Verify Frame rate in call stats greater than value passed
	Parameters:	value
	Example: verifyFpsReceivedGreaterThan(value: 0)
	*/
	func verifyFpsReceivedGreaterThan(value: Float) {
		let inLabel = app.staticTexts["RemoteFPS"].label
		let inArray = inLabel.components(separatedBy: " ")
		let inValue = inArray[0]
		let inFPS: Float = Float(inValue)!
		print("Frame rate received: \(inFPS) fps")
		XCTAssertTrue(inFPS > value, "FPS Received: " + inArray[0] + "not greater than " + String(format: "%.2f", value))
	}
	
	/*
	Name: verifyFpsReceivedGreaterThan0()
	Description: Verify Frame rate in call stats greater than 0
	Parameters:	none
	Example: verifyFpsReceivedGreaterThan0()
	*/
	func verifyFpsReceivedGreaterThan0() -> Bool {
			let inLabel = app.staticTexts["RemoteFPS"].label
			let inArray = inLabel.components(separatedBy: " ")
			let inValue = inArray[0]
			let inFPS: Float = Float(inValue) ?? 0.0
			print("Frame rate received: \(inFPS) fps")
			return inFPS > 0
	}
	
	/*
	Name: verifyFpsSentGreaterThan(value:)
	Description: Verify Frame rate sent in call stats greater than value passed
	Parameters:	value
	Example: verifyFpsSentGreaterThan(value: 0)
	*/
	func verifyFpsSentGreaterThan(value: Float) {
		let outLabel = app.staticTexts["LocalFPS"].label
		let outArray = outLabel.components(separatedBy: " ")
		let outValue = outArray[0]
		let outFPS: Float = Float(outValue)!
		print("Frame rate sent: \(outFPS) fps")
		XCTAssertTrue(outFPS > value, "FPS Sent: " + outArray[0] + "not greater than " + String(format: "%.2f", value))
	}
	
	/*
	Name: verifyFpsSentGreaterThan0()
	Description: Verify Frame rate sent in call stats greater than 0
	Parameters:	none
	Example: verifyFpsSentGreaterThan0()
	*/
	func verifyFpsSentGreaterThan0() -> Bool {
		let outLabel = app.staticTexts["LocalFPS"].label
		let outArray = outLabel.components(separatedBy: " ")
		let outValue = outArray[0]
		let outFPS: Float = Float(outValue) ?? 0.0
		print("Frame rate sent: \(outFPS) fps")
		return outFPS > 0
	}
	
	/*
	 * Name: verifyFrameRateGreaterThan
	 * Description: Verify Frame rate greater than value passed
	 * Parameters: value
	 * Example: verifyFrameRateGreaterThan(0)
	 */
	func verifyFrameRateGreaterThan(value: Float) {
		self.verifyFpsSentGreaterThan(value: value)
		self.verifyFpsReceivedGreaterThan(value: value)
	}
	
	/*
	Name: verifyGreeting(text:)
	Description: Verify PSMG Text
	Parameters: text
	Example: verifyGreeting(text: "Remote text greeting")
	**/
	func verifyGreeting(text: String) {
		var textValue = ""
		if #available(iOS 14.0, *) {
			textValue = app.textViews.element(boundBy: 0).value as? String ?? ""
		}
		else if #available(iOS 13.0, *) {
			textValue = app.textViews.staticTexts.element(boundBy: 0).label
		}
		else {
			textValue = app.textViews.element(boundBy: 0).value as? String ?? ""
		}
		XCTAssertTrue(textValue == text)
	}
	
	/*
	Name: verifyGreetingBtn(type:)
	Description: Verify Greeting button Type
	Parameters: type ("Sorenson", "Personal", "No Greeting")
	Example: verifyGreetingBtn(type: "Personal")
	*/
	func verifyGreetingBtn(type: String) {
		// Verify Greeting Button Type
		XCTAssertTrue(app.segmentedControls.buttons[type].exists)
	}
	
	/*
	Name: verifyGreetingTextLength(count:)
	Description: Verify PSMG Text length
	Parameters: count
	Example: verifyGreetingTextLength(count: 176)
	**/
	func verifyGreetingTextLength(count: Int) {
		var textValue = ""
		if #available(iOS 14.0, *) {
			textValue = app.textViews.element(boundBy: 0).value as? String ?? ""
		}
		else if #available(iOS 13.0, *) {
			textValue = app.textViews.staticTexts.element(boundBy: 0).label
		}
		else {
			textValue = app.textViews.element(boundBy: 0).value as? String ?? ""
		}
		XCTAssertTrue(textValue.count == count)
	}
	
	/*
	Name: verifyHideMyCallerIDState
	Description: Verify Hide My Caller ID switch is in expected state
	Parameters: state
	Example: verifyHideMyCallerIDState(count: 176)
	**/
	func verifyHideMyCallerIDState(state: Bool) {
		if(state) {
			XCTAssertTrue(optionSwitchStatus(text: "Hide My Caller ID"))
		} else {
			XCTAssertTrue(!optionSwitchStatus(text: "Hide My Caller ID"))
		}
	}
	
	/*
	Name: verifyHold()
	Description: Verify call is on Hold
	Parameters:	none
	Example: verifyHold()
	*/
	func verifyHold() {
		let onHold = self.waitFor(query: app.otherElements, value: "RemoteViewHold")
		XCTAssertTrue(onHold.exists, "Call not on Hold")
	}
	
	/*
	Name: verifyVRSCall()
	Description: Verify VRS text in call
	Parameters: none
	Example: verifyVRSCall()
	*/
	func verifyVRSCall() {
		let _ = self.waitFor(query: app.buttons, value: "Caller Info")
		let staticTextQuery = app.staticTexts.debugDescription
		XCTAssertTrue(staticTextQuery.contains("VRS"), "VRS not found")
	}
	
	/*
	Name: verifyHomeVCOSwitchState(settingValue:)
	Description: Verify VCO is Enabled/Disabled on dialpad
	Parameters:	settingValue
	Example: verifyHomeVCOSwitchState(settingValue: "1")
	*/
	func verifyHomeVCOSwitchState(settingValue: String) {
		XCTAssertTrue(app.switches[settingValue].exists)
	}
	
	/*
	Name: verifyImportedAlert(alert)
	Description: Verify the alert message from importing contacts
	Parameters:	alert ("Import completed.", "Invalid Phone Number", "Can't import contacts.")
	Example: verifyImportedNumber(alert: "Import completed.")
	*/
	func verifyImportAlert(alert: String) {
		// Verify import alert message
		XCTAssertTrue(app.alerts[alert].exists)
	}
	
	/*
	Name: verifyIconPencilDisabled()
	Description: Verify the Icon Pencil button is disabled
	Parameters: none
	Example: verifyIconPencilDisabled()
	*/
	func verifyIconPencilDisabled() {
		XCTAssertFalse(app.buttons["icon pencil"].isEnabled)
	}
	
	/*
	Name: verifyImportedNumber(expectedNumber:)
	Description: Verify the number of imported contacts
	Parameters:	expectedNumber
	Example: verifyImportedNumber(expectedNumber: "2")
	*/
	func verifyImportedNumber(expectedNumber: String) {
		XCTAssertTrue(app.alerts["Contacts imported."].staticTexts[expectedNumber + " contacts were imported."].exists)
	}
	
	/*
	Name: verifyIncomingCall()
	Description: Verify Answer button is available
	Parameters:	none
	Example: verifyIncomingCall()
	*/
	func verifyIncomingCall() {
		XCTAssertTrue(app.buttons["Answer"].exists)
	}
	
	/*
	Name: verifyInvalidBirthday()
	Description: Verify Birthday Invalid message
	Parameters:	none
	Example: verifyInvalidBirthday()
	*/
	func verifyInvalidBirthday() {
		self.clickButton(name: "OK", query: app.alerts["Birthday Invalid"].buttons)
	}
	
	/*
	Name: verifyInvalidEmail()
	Description: Verify Invalid email address
	Parameters:	none
	Example: verifyInvalidEmail()
	*/
	func verifyInvalidEmail() {
		XCTAssertTrue(app.alerts.staticTexts["The email address you entered is invalid.  Please enter a valid email address."].exists)
		self.clickButton(name: "OK", query: app.alerts["Invalid Email Address"].buttons)
	}
	
	/*
	Name: verifyInvalidEndpointType()
	Description: Verify Invalid Endpoint Type
	Parameters:	none
	Example: verifyInvalidEndpointType()
	*/
	func verifyInvalidEndpointType() {
		XCTAssertTrue(app.alerts.staticTexts["Endpoint types do not match"].exists)
		self.clickButton(name: "OK", query: app.alerts["Cannot Log In"].buttons)
	}
	
	/*
	Name: verifyInvalidLogin()
	Description: Verify Invalid Login
	Parameters:	none
	Example: verifyInvalidLogin()
	*/
	func verifyInvalidLogin() {
		XCTAssertTrue(app.alerts.staticTexts["Phone number and/or password is incorrect."].exists)
		self.clickButton(name: "OK", query: app.alerts["Login Failed"].buttons)
	}
	
	/*
	Name: verifyInvalidOldPassword()
	Description: Verify Invalid Old Password
	Parameters:	none
	Example: verifyInvalidOldPassword()
	*/
	func verifyInvalidOldPassword() {
		XCTAssertTrue(app.alerts.staticTexts["Old Password was incorrect."].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyInvalidPasswordCharacters()
	Description: Verify Invalid Password characters
	Parameters:	none
	Example: verifyInvalidPasswordCharacters()
	*/
	func verifyInvalidPasswordCharacters() {
		XCTAssertTrue(app.alerts.staticTexts["Only standard letters, numbers, and punctuation characters can be used."].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyInvalidPhoneNumber()
	Description: Verify Invalid Phone Number
	Parameters:	none
	Example: verifyInvalidPhoneNumber()
	*/
	func verifyInvalidPhoneNumber() {
		XCTAssertTrue(app.alerts.staticTexts["Your assigned phone number must be 10 or 11 digits."].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyInvalidSSN()
	Description: Verify SSN Invalid message
	Parameters:	none
	Example: verifyInvalidSSN()
	*/
	func verifyInvalidSSN() {
		self.clickButton(name: "OK", query: app.alerts["Invalid SSN"].buttons)
	}
	
	/*
	Name: verifyKeyboard()
	Description: Verify the Keyboard
	Parameters: none
	Example: verifyKeyboard()
	**/
	func verifyKeyboard() {
		XCTAssertTrue(app.keyboards.keys["q"].exists || app.keyboards.keys["Q"].exists)
		XCTAssertTrue(app.keyboards.keys["w"].exists || app.keyboards.keys["W"].exists)
		XCTAssertTrue(app.keyboards.keys["e"].exists || app.keyboards.keys["E"].exists)
		XCTAssertTrue(app.keyboards.keys["r"].exists || app.keyboards.keys["R"].exists)
		XCTAssertTrue(app.keyboards.keys["t"].exists || app.keyboards.keys["T"].exists)
		XCTAssertTrue(app.keyboards.keys["y"].exists || app.keyboards.keys["Y"].exists)
		XCTAssertTrue(app.keyboards.keys["u"].exists || app.keyboards.keys["U"].exists)
		XCTAssertTrue(app.keyboards.keys["i"].exists || app.keyboards.keys["I"].exists)
		XCTAssertTrue(app.keyboards.keys["o"].exists || app.keyboards.keys["O"].exists)
		XCTAssertTrue(app.keyboards.keys["p"].exists || app.keyboards.keys["P"].exists)
		XCTAssertTrue(app.keyboards.keys["a"].exists || app.keyboards.keys["A"].exists)
		XCTAssertTrue(app.keyboards.keys["s"].exists || app.keyboards.keys["S"].exists)
		XCTAssertTrue(app.keyboards.keys["d"].exists || app.keyboards.keys["D"].exists)
		XCTAssertTrue(app.keyboards.keys["f"].exists || app.keyboards.keys["F"].exists)
		XCTAssertTrue(app.keyboards.keys["g"].exists || app.keyboards.keys["G"].exists)
		XCTAssertTrue(app.keyboards.keys["h"].exists || app.keyboards.keys["H"].exists)
		XCTAssertTrue(app.keyboards.keys["j"].exists || app.keyboards.keys["J"].exists)
		XCTAssertTrue(app.keyboards.keys["k"].exists || app.keyboards.keys["K"].exists)
		XCTAssertTrue(app.keyboards.keys["l"].exists || app.keyboards.keys["L"].exists)
		XCTAssertTrue(app.keyboards.keys["z"].exists || app.keyboards.keys["Z"].exists)
		XCTAssertTrue(app.keyboards.keys["x"].exists || app.keyboards.keys["X"].exists)
		XCTAssertTrue(app.keyboards.keys["c"].exists || app.keyboards.keys["C"].exists)
		XCTAssertTrue(app.keyboards.keys["v"].exists || app.keyboards.keys["V"].exists)
		XCTAssertTrue(app.keyboards.keys["b"].exists || app.keyboards.keys["B"].exists)
		XCTAssertTrue(app.keyboards.keys["n"].exists || app.keyboards.keys["N"].exists)
		XCTAssertTrue(app.keyboards.keys["m"].exists || app.keyboards.keys["M"].exists)
	}
	
	/*
	Name: verifyLocationContains(text:)
	Description: Verify location contains
	Parameters: text
	Example: verifyLocationContains(text: "UT")
	*/
	func verifyLocationContains(text: String) {
		let textViewQuery = app.textViews.element(boundBy: 0).value as! String
		XCTAssertTrue(textViewQuery.contains(text))
	}
	
	/*
	Name: verifyMailboxFull()
	Description: Verify Mailbox Full message and select Cancel or Call Directly
	Parameters: response ("Cancel", "Call Directly")
	Example: verifyMailboxFull("Cancel")
	*/
	func verifyMailboxFull(response: String) {
		let mailboxFull = self.waitFor(query: app.alerts.staticTexts, value: "Sorry, that mailbox is full.")
		XCTAssertTrue(mailboxFull.exists, "Mailbox Full alert did not appear")
		self.clickButton(name: response, query: app.alerts.buttons)
	}
	
	/*
	Name: verifyMaxSavedTexts()
	Description: Verify Max Saved Texts Alert Message
	Parameters: none
	Example: verifyMaxSavedTexts()
	*/
	func verifyMaxSavedTexts() {
		let maxSharedTexts = self.waitFor(query: app.alerts, value: "Max custom shared texts")
		XCTAssertTrue(maxSharedTexts.exists, "Max custom shared texts message did not appear")
		maxSharedTexts.buttons["OK"].tap()
	}
	
	/*
	Name: verifyMicrophoneButtonDisabled()
	Description: Verify the microphone button is disabled on the home screen
	Parameters: none
	Example: verifyMicrophoneButtonDisabled()
	*/
	func verifyMicrophoneButtonDisabled() {
		XCTAssertTrue(!app.buttons["Microphone Off"].isEnabled)
	}
	
	/*
	Name: verifyMicrophoneButtonEnabled()
	Description: Verify the microphone button is enabled on the home screen
	Parameters: none
	Example: verifyMicrophoneButtonEnabled()
	*/
	func verifyMicrophoneButtonEnabled() {
		if app.buttons["Microphone On"].exists {
			XCTAssertTrue(app.buttons.matching(identifier: "Microphone On").element(boundBy: 1).isEnabled)
		}
		else {
			XCTAssertTrue(app.buttons.matching(identifier: "Microphone Off").element(boundBy: 1).isEnabled)
		}
	}
	
	/*
	Name: verifyMicrophoneToggleOff()
	Description: Verify the Microphone toggle is off
	Parameters: none
	Example: verifyMicrophoneToggleOff()
	*/
	func verifyMicrophoneToggleOff() {
		XCTAssertTrue(app.buttons["Microphone Off"].exists)
	}
	
	/*
	Name: verifyMicrophoneToggleOn()
	Description: Verify the Microphone toggle is on
	Parameters: none
	Example: verifyMicrophoneToggleOn()
	*/
	func verifyMicrophoneToggleOn() {
		XCTAssertTrue(app.buttons["Microphone On"].exists)
	}
	
	/*
	Name: verifyNetworkSpeedBitrates()
	Description: Verify Network Speed Bitrates
	Parameters: none
	Example: verifyNetworkSpeedBitrates()
	*/
	func verifyNetworkSpeedBitrates() {
		self.selectNetworkSpeed()
		XCTAssertTrue(app.staticTexts["128 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["192 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["256 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["320 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["384 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["448 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["512 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["576 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["640 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["704 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["768 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["832 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["896 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["960 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["1024 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["1152 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["1280 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["1408 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["1536 Kbps"].exists)
		XCTAssertTrue(app.staticTexts["2048 Kbps"].exists)
		self.selectSettingsBackBtn()
	}
	
	/*
	Name: verifyNoAddCallOption()
	Description: Verify no Add Call Option
	Parameters: none
	Example: verifyNoAddCallOption()
	*/
	func verifyNoAddCallOption() {
		// Check for Hangup button and verify Add Call is not available
		if (app.buttons["Hangup"].exists) {
			XCTAssertFalse(app.buttons["Add Call"].exists)
		}
		else {
			app.windows.element(boundBy: 0).tap()
			XCTAssertFalse(app.buttons["Add Call"].exists)
		}
	}
	
	/*
	Name: verifyNoBlockedSignMail(name:)
	Description: Verify No Blocked SignMail by name
	Parameters: name
	Example: verifyNoBlockedSignMail(name: "Automate 4199")
	*/
	func verifyNoBlockedSignMail(name: String) {
		self.verifyContact(name: name)
		XCTAssertTrue(!app.images["BlockedSignMail"].exists)
	}
	
	/*
	Name: verifyNoCallHistory(num:)
	Description: Verify Phone Number not in call history
	Parameters: num
	Example: verifyNoCallHistory(num: "19323387000")
	*/
	func verifyNoCallHistory(num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		XCTAssertFalse(app.tables.cells.staticTexts[phnum].exists)
	}
	
	/*
	Name: verifyNoCallHistory(name:num:)
	Description: Verify No Call History item by Name and Type
	Parameters: name
				num
	Example: verifyNoCallHistory(name: "Automate 5441", num: "15553334444")
	*/
	func verifyNoCallHistory(name: String, num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		
		// P2P Call
		if (app.tables.cells.staticTexts[phnum].exists) {
			XCTAssertTrue(!app.tables.cells.containing(.staticText, identifier: phnum).staticTexts[name].exists)
		}
		// VRS Call
		else {
			XCTAssertTrue(!app.tables.cells.containing(.staticText, identifier: phnum + " / VRS").staticTexts[name].exists)
		}
	}
	
	/*
	Name: verifyNoCallOptions()
	Description: Verify no Call Options
	Parameters: none
	Example: verifyNoCallOptions()
	*/
	func verifyNoCallOptions() {
		// Verify No Call Options
		XCTAssertFalse(app.buttons["Add Call"].exists)
		XCTAssertFalse(app.buttons["Keypad"].exists)
		XCTAssertFalse(app.buttons["Text"].exists)
	}
	
	/*
	Name: verifyNoCallOptionsUI()
	Description: Verify no Call Options UI
	Parameters: none
	Example: verifyNoCallOptionsUI()
	*/
	func verifyNoCallOptionsUI() {
		// Verify No Call Options
		XCTAssertFalse(app.buttons["Make Another Call"].exists)
		XCTAssertFalse(app.buttons["Dialpad"].exists)
		XCTAssertFalse(app.buttons["Transfer Call"].exists)
	}
	
	/*
	Name: verifyNoContact(name:)
	Description: Verify a Contact does not exist by Name
	Parameters: name
	Example: verifyNoContact(name: "Automate 5395")
	*/
	func verifyNoContact(name: String) {
		XCTAssertFalse(app.tables.staticTexts[name].exists)
	}
	
	/*
	Name: verifyNoContactNumber(type:)
	Description: From within a contact, verify a number type is empty
	Parameters: type ("home", "work", or "mobile")
	Example: verifyNoContactNumber(type: "home")
	*/
	func verifyNoContactNumber(type: String) {
		XCTAssertFalse(app.tables.cells.containing(.staticText, identifier: type).element.exists)
	}
	
	/*
	Name: verifyNoDialerUI()
	Description: Verify UI for call and UI elements that should not be present for call
	Parameters:	none
	Example: verifyNoDialerUI()
	*/
	func verifyNoDialerUI() {
		// Dialer UI that doesn't transition
		XCTAssertFalse(app.staticTexts.debugDescription.contains("Add Contact"), "Add Contact found")
		XCTAssertFalse(app.buttons.debugDescription.contains("Back"), "Back found")
		XCTAssertFalse(app.webViews.debugDescription.contains("HandleBarViews"), "HandleBarViews found")
		
		// Dial pad transition view
		if (app.buttons["1"].isHittable) == true {XCTFail("1 hittable")}
		if (app.buttons["2"].isHittable) == true {XCTFail("2 hittable")}
		if (app.buttons["3"].isHittable) == true {XCTFail("3 hittable")}
		if (app.buttons["4"].isHittable) == true {XCTFail("4 hittable")}
		if (app.buttons["5"].isHittable) == true {XCTFail("5 hittable")}
		if (app.buttons["6"].isHittable) == true {XCTFail("6 hittable")}
		if (app.buttons["7"].isHittable) == true {XCTFail("7 hittable")}
		if (app.buttons["8"].isHittable) == true {XCTFail("8 hittable")}
		if (app.buttons["9"].isHittable) == true {XCTFail("9 hittable")}
		if (app.buttons["Microphone Off"].isHittable) == true {XCTFail("Microphone Off hittable")}
		if (app.buttons["0"].isHittable) == true {XCTFail("0 hittable")}
		if (app.buttons["Search"].isHittable) == true {XCTFail("Search hittable")}
		
		// Tab bar transition view
		if (app.buttons["Dial"].isHittable) == true {XCTFail("Dial hittable")}
		if (app.buttons["History"].isHittable) == true {XCTFail("History hittable")}
		if (app.buttons["Contacts"].isHittable) == true {XCTFail("Contacts hittable")}
		if (app.buttons["SignMail"].isHittable) == true {XCTFail("SignMail hittable")}
		if (app.buttons["Settings"].isHittable) == true {XCTFail("Settings hittable")}
		
		// Call UI
		XCTAssertTrue(app.buttons.debugDescription.contains("Call Options"), "Call Options not found")
		XCTAssertTrue(app.buttons.debugDescription.contains("Share"), "Share not found")
		XCTAssertTrue(app.buttons.debugDescription.contains("Caller Info"), "Caller Info not found")
		XCTAssertTrue(app.buttons.debugDescription.contains("Switch Camera"), "Switch Camera not found")
	}
	
	/*
	Name: verifyNoEncryptionIcon()
	Description: Verify Encryption Icon is not visible
	Parameters:	none
	Example: verifyNoEncryptionIcon()
	*/
	func verifyNoEncryptionIcon() {
		XCTAssertTrue(!app.buttons["EncryptionEnabled"].exists)
	}

	/*
	Name: verifyNoFavorite(name:type:)
	Description: Verify a Favorite does not exist by Favorite Name and Type
	Parameters: favoriteName
				type ("home", "work", or "mobile")
	Example: verifyNoFavorite(name: "Automate 5395", type: "home")
	*/
	func verifyNoFavorite(name: String, type: String) {
		let favoriteBtnName = NSString(format: "More Info, %@, %@", name, type)
		XCTAssertFalse(app.tables.buttons[favoriteBtnName as String].exists)
	}
	
	/*
	Name: verifyNoFavoritesPage()
	Description: Verify no Favorites page in the Phonebook
	Parameters:	none
	Example: verifyNoFavoritesPage()
	*/
	func verifyNoFavoritesPage() {
		if #available(iOS 14.0, *) {
			XCTAssertFalse(app.staticTexts["Favorites"].exists)
		}
		else {
			XCTAssertFalse(app.navigationBars["Favorites"].exists)
		}
	}
	
	/*
	Name: verifyNoFavoritePhoto(name:type:)
	Description: Verify there is No Photo by Favorite Name and Type
	Parameters: favoriteName
				type ("home", "work", or "mobile")
	Example: verifyNoFavoritePhoto(name: "Automate 5395", type: "home")
	*/
	func verifyNoFavoritePhoto(name: String, type: String) {
		// Verify Favorite
		verifyFavorite(name: name, type: type)
		
		// Verify Photo removed
		// Removed photo ID: 00000000-0000-0000-0000-000000000000
		let removedPhoto = String(format: "%@ 00000000-0000-0000-0000-000000000000", name)
		XCTAssertTrue(app.tables.cells[removedPhoto].exists)
	}
	
	/*
	Name: verifyNoFavoriteStar(type:)
	Description: Verify No Favorite Star by Type
	Parameters:	type ("home", "work", or "mobile")
	Example: verifyNoFavoriteStar(type: "home")
	*/
	func verifyNoFavoriteStar(type: String) {
		let cell = app.tables.cells.containing(.staticText, identifier:type)
		XCTAssertFalse(cell.staticTexts["★"].exists)
	}
	
	/*
	Name: verifyNoFavoriteStarBtn(type:)
	Description: Verify no Favorite Star Button by Type
	Parameters:	type ("home", "work", or "mobile")
	Example: verifyNoFavoriteStarBtn(type: "home")
	*/
	func verifyNoFavoriteStarBtn(type: String) {
		if (type == "home") {
			XCTAssertFalse(app.tables.cells["HomeNumberCell"].buttons["HomeFavoriteButton"].exists)
		}
		else if(type == "work") {
			XCTAssertFalse(app.tables.cells["WorkNumberCell"].buttons["WorkFavoriteButton"].exists)
		}
		else if(type == "mobile") {
			XCTAssertFalse(app.tables.cells["MobileNumberCell"].buttons["MobileFavoriteButton"].exists)
		}
	}
	
	/*
	Name: verifyNoGreeting(text:)
	Description: Verify No PSMG Text
	Parameters: text
	Example: verifyNoGreeting(text: "Remote text greeting")
	**/
	func verifyNoGreeting(text: String) {
		var textValue = ""
		if app.textViews.count != 0 {
			if #available(iOS 14.0, *) {
				textValue = app.textViews.element(boundBy: 0).value as? String ?? ""
			}
			else if #available(iOS 13.0, *) {
				textValue = app.textViews.staticTexts.element(boundBy: 0).label
			}
			else {
				textValue = app.textViews.element(boundBy: 0).value as? String ?? ""
			}
		}
		XCTAssertTrue(textValue != text)
	}
	
	/*
	Name: verifyNoMissedCallBadge(count:)
	Description: Verify no missed call badge
	Parameters:	none
	Example: verifyNoMissedCallBadge()
	*/
	func verifyNoMissedCallBadge() {
		let missedCallCount = app.tabBars.buttons["History"].value as? String
		XCTAssertTrue(missedCallCount == "")
	}
	
	/*
	Name: verifyNoNetworkSpeedSetting()
	Description: Verify no Network Speed Setting
	Parameters:	none
	Example: verifyNoNetworkSpeedSetting()
	*/
	func verifyNoNetworkSpeedSetting() {
		XCTAssertFalse(app.staticTexts["Network Speed"].exists)
	}
	
	/*
	Name: verifyNoSavedText(text: String)
	Description: Verify no saved text present
	Parameters: none
	Example: verifyNoSavedText(text: String)
	*/
	func verifyNoSavedText(text: String) {
		XCTAssertFalse(app.textFields[text].exists)
	}

	/*
	Name: verifyNoSharedText(text:)
	Description: Verify no Share Text
	Parameters: text
	Example: verifyNoSharedText(text: "1234")
	*/
	func verifyNoSharedText(text: String) {
		XCTAssertFalse(app.textViews[text].exists)
	}
	
	/*
	Name: verifyNoSignMailOption()
	Description: Verify no option to send SignMail
	Parameters: none
	Example: verifyNoSignMailOption()
	*/
	func verifyNoSignMailOption() {
		XCTAssertFalse(app.staticTexts["Send SignMail"].exists)
	}
	
	/*
	Name: verifyNoShareOptions()
	Description: Verify no Share Options
	Parameters: none
	Example: verifyNoShareOptions()
	*/
	func verifyNoShareOptions() {
		XCTAssertTrue(!app.buttons["More"].exists)
	}
	
	/*
	Name: verifyNoShareText()
	Description: Verify no Share Text
	Parameters: none
	Example: verifyNoShareText()
	*/
	func verifyNoShareText() {
		XCTAssertTrue(!app.buttons["Share"].exists)
	}
	
	/*
	Name: verifyNoSignMail()
	Description: Verify no SignMail
	Parameters: none
	Example: verifyNoSignMail()
	*/
	func verifyNoSignMail() {
		XCTAssertTrue(app.tables.staticTexts["You don't have any SignMails."].exists)
	}
	
	/*
	Name: verifyNoSpanishFeatures()
	Description: Verify no Spanish Features
	Parameters: none
	Example: verifyNoSpanishFeatures()
	*/
	func verifyNoSpanishFeatures() {
		XCTAssertTrue(!app.tables.staticTexts["Enable Spanish Features"].exists)
	}
	
	/*
	Name: verifyNoCallTS()
	Description: Verify no Call Technical Support on Login Screen
	Parameters: none
	Example: verifyNoCallTS()
	*/
	func verifyNoCallTS() {
		XCTAssertTrue(!app.staticTexts["Technical Support"].exists)
	}
	
	/*
	Name: verifyNoRecordButton()
	Description: Verify no Record Button
	Parameters: none
	Example: verifyNoRecordButton()
	*/
	func verifyNoRecordButton() {
		XCTAssertFalse(app.buttons["Record"].exists)
	}
	
	/*
	Name: verifyNoReturnOnKeyboard()
	Description: Verify no Return on keyboard
	Parameters: none
	Example: verifyNoReturnOnKeyboard()
	*/
	func verifyNoReturnOnKeyboard() {
		let textView = app.textViews.element(boundBy: 0)
		if app.textViews["Enter text here."].exists {
			app.textViews["Enter text here."].tap()
		}
		else {
			textView.tap()
		}
		
		// Verify no return key
		XCTAssertFalse(app.keyboards.keys["return"].exists)
		
		// close keyboard
		app.swipeDown() // refresh ui
		app.buttons["Done"].tap()
	}
	
	/*
	Name: verifyNumberAlreadyBlocked()
	Description: Select Ok on Number already blocked alert
	Parameters: none
	Example: verifyNumberAlreadyBlocked()
	*/
	func verifyNumberAlreadyBlocked() {
		XCTAssert(app.alerts["Number already blocked."].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyNumKeyboard()
	Description: Verify the Numeric Keyboard
	Parameters: none
	Example: verifyNumKeyboard()
	**/
	func verifyNumKeyboard() {
		XCTAssertTrue(app.keyboards.keys["1"].exists)
		XCTAssertTrue(app.keyboards.keys["2"].exists)
		XCTAssertTrue(app.keyboards.keys["3"].exists)
		XCTAssertTrue(app.keyboards.keys["4"].exists)
		XCTAssertTrue(app.keyboards.keys["5"].exists)
		XCTAssertTrue(app.keyboards.keys["6"].exists)
		XCTAssertTrue(app.keyboards.keys["7"].exists)
		XCTAssertTrue(app.keyboards.keys["8"].exists)
		XCTAssertTrue(app.keyboards.keys["9"].exists)
		XCTAssertTrue(app.keyboards.keys["0"].exists)
	}
	
	/*
	Name: verifyObscuredSSN()
	Description: Verify the SSN is obscured
	Parameters: none
	Example: verifyObscuredSSN()
	*/
	func verifyObscuredSSN() {
		XCTAssertTrue(app.tables.secureTextFields["••••"].exists)
	}
	
	/*
	Name: verifyPassword()
	Description: Verify Password Secure Field String "••••"
	Parameters: none
	Example: verifyPassword()
	*/
	func verifyPassword() {
		// Verify Password
		sleep(3)
		XCTAssertTrue(app.secureTextFields["Password"].value as? String == "••••")
		sleep(1)
	}

	/*
	Name: verifyPasswordBlank()
	Description: Verify Password Field String "Password"
	Parameters: none
	Example: verifyPasswordBlank()
	*/
	func verifyPasswordBlank() {
		// Verify Password is Blank
		sleep(3)
		XCTAssertTrue(app.secureTextFields["Password"].value as? String == "Password")
		sleep(1)
	}

	/*
	Name: verifyPasswordFieldsEmpty()
	Description: Verify Password Fields empty ("required")
	Parameters: none
	Example: verifyPasswordFieldsEmpty()
	*/
	func verifyPasswordFieldsEmpty() {
		XCTAssertTrue(app.tables.secureTextFields["Old Password"].value as? String == "required")
		XCTAssertTrue(app.tables.secureTextFields["New Password"].value as? String == "required")
		XCTAssertTrue(app.tables.secureTextFields["Confirm Password"].value as? String == "required")
	}
	/*
	Name: verifyPasswordImportContacts()
	Description: Verify Password Secure Field String "••••"
	Parameters: none
	Example: verifyPasswordImportContacts()
	*/
	func verifyPasswordImportContacts() {
		// Verify Password
		sleep(3)
		XCTAssertTrue(app.tables.cells.containing(.staticText, identifier:"Password").secureTextFields.element(boundBy: 0).value as? String == "••••")
	}
	
	
	/*
	Name: verifyPhoneNumber()
	Description: Verify Phone number
	Parameters:	num
	Example: verifyPhoneNumber()
	*/
	func verifyPhoneNumber(num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		XCTAssertTrue(app.staticTexts[phnum].exists)
	}
	
	/*
	Name: verifyPhoneNumberTextField()
	Description: Verify Phone number TextField
	Parameters:	phnum
	Example: ui.verifyPhoneNumberTextField(phnum: dutNum)
	*/
	func verifyPhoneNumberTextField(phnum: String) {
		// Format the phone number
		let phoneNumber = String (self.formatPhoneNumber(num: phnum))
		//
		XCTAssertTrue(app.textFields["PhoneNumber"].value as? String == phoneNumber)
	}
	
	/*
	Name: verifyPlaybackControls()
	Description: Verify Playback Controls
	Parameters: none
	Example: verifyPlaybackControls()
	*/
	func verifyPlaybackControls() {
		if (!app.buttons["Play/Pause"].exists) {
			self.selectTop3rd()
			sleep(1)
		}
		XCTAssertTrue(app.buttons["Play/Pause"].exists)
	}
	
	/*
	Name: verifyProfileAvatar(expected:)
	Description: Verify Profile Avatar (no profile photo)
	Parameters: expected
	Example: verifyProfileAvatar(expected: true)
	*/
	func verifyProfileAvatar(expected: Bool) {
		if expected {
			XCTAssertTrue(app.buttons["avatar default"].exists)
		}
		else {
			XCTAssertFalse(app.buttons["avatar default"].exists)
		}
	}
	
	/*
	Name: verifyProgressBar()
	Description: Verify Progress bar exists
	Parameters: none
	Example: verifyProgressBar()
	*/
	func verifyProgressBar() {
		XCTAssert(app.progressIndicators["Progress"].exists)
	}
	
	/*
	Name: verifyRingsBeforeGreeting(checkRings:)
	Description: Verify Rings before Greetings
	Parameters: checkRings
	Example: verifyRingsBeforeGreeting(checkRings: 5)
	*/
	func verifyRingsBeforeGreeting(checkRings: String){
		// Verify ring + " Rings" is displayed in Incoming Calls Settings (min & default is 5, range 5-15)
		XCTAssertTrue(app.staticTexts[checkRings + " Rings"].exists)
	}
	
	/*
	Name: verifyRingsBeforeGreetingList
	Description: Verify Rings before Greetings
	Parameters: None
	Example: verifyRingsBeforeGreetingList
	*/
	func verifyRingsBeforeGreetingList() {
		// Verify that 5-15 Ring is available
		XCTAssertTrue(app.staticTexts["5 Rings"].exists)
		XCTAssertTrue(app.staticTexts["6 Rings"].exists)
		XCTAssertTrue(app.staticTexts["7 Rings"].exists)
		XCTAssertTrue(app.staticTexts["8 Rings"].exists)
		XCTAssertTrue(app.staticTexts["9 Rings"].exists)
		XCTAssertTrue(app.staticTexts["10 Rings"].exists)
		XCTAssertTrue(app.staticTexts["11 Rings"].exists)
		XCTAssertTrue(app.staticTexts["12 Rings"].exists)
		XCTAssertTrue(app.staticTexts["13 Rings"].exists)
		XCTAssertTrue(app.staticTexts["14 Rings"].exists)
		XCTAssertTrue(app.staticTexts["15 Rings"].exists)
	}
	
	/*
	Name: verifySavedText(text:)
	Description: Verify Saved Text item
	Parameters: text
	Example: verifySavedText(text: "1234")
	*/
	func verifySavedText(text: String) {
		XCTAssertTrue(app.textFields[text].exists)
	}
	
	/*
	Name: verifySavedTextPositions()
	Description: Verify Saved Text Position
	Parameters: none
	Example: verifySavedTextPositions()
	*/
	func verifySavedTextPositions() {
		XCTAssertTrue(app.textFields.element(boundBy: 0).value as? String == "B Test5386 1")
		XCTAssertTrue(app.textFields.element(boundBy: 1).value as? String == "E Test5386 2")
		XCTAssertTrue(app.textFields.element(boundBy: 2).value as? String == "D Test5386 3")
		XCTAssertTrue(app.textFields.element(boundBy: 3).value as? String == "G Test5386 4")
		XCTAssertTrue(app.textFields.element(boundBy: 4).value as? String == "F Test5386 5")
		XCTAssertTrue(app.textFields.element(boundBy: 5).value as? String == "A Test5386 6")
		XCTAssertTrue(app.textFields.element(boundBy: 6).value as? String == "H Test5386 7")
		XCTAssertTrue(app.textFields.element(boundBy: 7).value as? String == "I Test5386 8")
		XCTAssertTrue(app.textFields.element(boundBy: 8).value as? String == "K Test5386 9")
		XCTAssertTrue(app.textFields.element(boundBy: 9).value as? String == "J Test5386 10")
	}
	
	/*
	Name: verifySearchField()
	Description: Verify search field
	Parameters: none
	Example: verifySearchField()
	*/
	func verifySearchField() {
		// Verify search field for contacts
		XCTAssertTrue(app.searchFields["Search Contacts & Yelp"].exists)
	}
	
	/*
	Name: verifySearchField(value:)
	Description: Verify search field value
	Parameters: value
	Example: verifySearchField(value: "pazz")
	*/
	func verifySearchField(value: String) {
		// Verify search field for contacts
		let searchValue = app.searchFields["Search Contacts & Yelp"].value as! String
		XCTAssertEqual(searchValue, value)
	}
	
	/*
	Name: verifySearchFieldBar()
	Description: Verify search field for Call History and SignMail
	Parameters: none
	Example: verifySearchFieldBar()
	*/
	func verifySearchFieldBar() {
		// VerifysearchFieldBar for History and SignMail
		XCTAssertTrue(app.searchFields["Search"].exists)
	}

	/*
	Name: verifySearchFieldResult(word:)
	Description: Verify Search field result matches listed on the screen
	Parameters: none
	Example: verifySearchFieldResult(word: "Cellular")
	*/
	func verifySearchFieldResult(word: String) {
		XCTAssertTrue(app.staticTexts[word].exists)
	}
	
	/*
	Name: verifySettingsPage()
	Description: Verify the settings page
	Parameters: none
	Example: verifySettingsPage()
	*/
	func verifySettingsPage() {
		XCTAssertTrue(app.navigationBars["Settings"].exists)
	}
	
	/*
	Name: verifySettingsSwitch(setting:value:)
	Description: Verify Settings switch by setting and value
	Parameters:	setting, value
	Example: verifyEncryptMyCallsRequired(setting: "Encrypt My Calls", true)
	*/
	func verifySettingsSwitch(setting: String, value: Bool) {
		// Tap to scroll to setting
		self.clickStaticText(text: setting, query: app.tables.staticTexts)
		
		// Get the switch state
		let settingSwitch = app.tables.switches[setting]
		let switchState = settingSwitch.value as? String
		
		// Verify the value
		switch value {
		case true:
			XCTAssertTrue(switchState == "1")
			break
		case false:
			XCTAssertTrue(switchState == "0")
			break
		}
	}
	
	/*
	Name: verifyRemoteViewText(text:)
	Description: Verify Remote View Shared Text
	Parameters: text
	Example: verifyRemoteViewText(text: "1234")
	*/
	func verifyRemoteViewText(text: String) {
		XCTAssertTrue(app.textViews["RemoteViewText"].value as! String == text)
	}
	
	/*
	Name: verifyOrientation(bool:)
	Description: Verify if orientation is landscape
	Parameters: bool
	Example: verifyOrientation(isLandscape: true)
	*/
	func verifyOrientation(isLandscape: Bool) {
		if isLandscape {
			XCTAssertTrue(XCUIDevice.shared.orientation.isLandscape)
		} else {
			XCTAssertFalse(XCUIDevice.shared.orientation.isLandscape)
		}
	}
	
	/*
	Name: verifySelfView(expected:)
	Description: Verify Self View
	Parameters: expected
	Example: verifySelfView(expected: true)
	*/
	func verifySelfView(expected: Bool) {
		if (expected) {
			// Verify x coordinate is not a negative value
			XCTAssertFalse(app.otherElements["LocalViewVideo"].frame.debugDescription.contains("(-"))
		} else {
			// Verify x coordinate is negative value
			XCTAssertTrue(app.otherElements["LocalViewVideo"].frame.debugDescription.contains("(-"))
		}
	}
	
	/*
	Name: verifySelfViewText(text:)
	Description: Verify Self View Shared Text
	Parameters: text
	Example: verifySelfViewText(text: "1234")
	*/
	func verifySelfViewText(text: String) {
		XCTAssertTrue(app.textViews["SelfViewText"].value as! String == text)
	}
	
	/*
	Name: verifyShareTextButtonLabel(label:)
	Description: Verify the label name for share text
	Parameters: label
	Example: verifyShareTextButtonLabel(label: "Share")
	*/
	func verifyShareTextButtonLabel(label: String) {
		XCTAssertTrue(app.buttons[label].exists)
	}

	/*
	Name: verifySignMail(count:)
	Description: Verify the SignMail count
	Parameters: count
	Example: verifySignMail(count: 0)
	**/
	func verifySignMail(count: UInt) {
		let maxWait: Int = 300
		for i in 1 ... maxWait {
			let rows = app.tables.cells.count
			if rows == count {
				XCTAssertEqual(rows, Int(count))
				break
			}
			else {
				self.heartbeat()
			}
			if (i == maxWait) {
				XCTAssertTrue(false, "Failed to receive SignMail count:" + String(count))
			}
		}
	}
	
	/*
	Name: verifySignMail(phnum:name:)
	Description: Verify SignMail by phone number and name
	Parameters: phnum, name
	Example: verifySignMail(phnum: "19323883030", "Automate 563")
	*/
	func verifySignMail(phnum: String, name: String) {
		// Format the phone number
		let phoneNumber = String (self.formatPhoneNumber(num: phnum))
		
		// Find cell by phnum
		let messageCell = app.tables.cells.containing(.staticText, identifier:phoneNumber)
		
		// Verify name
		XCTAssertTrue(messageCell.staticTexts[name].exists)
	}
	
	/*
	Name: verifySignMailBadge(count:)
	Description: Verify SignMail badge count
	Parameters:	count
	Example: verifySignMailBadge(count: "1")
	*/
	func verifySignMailBadge(count: String) {
		let signMailCount = app.tabBars.buttons["SignMail"].value.debugDescription
		XCTAssertTrue(signMailCount.contains(count), "")
	}
	
	/*
	Name: verifySignMailDisabled()
	Description: Verify SignMail disabled
	Parameters: none
	Example: verifySignMailDisabled()
	*/
	func verifySignMailDisabled() {
		XCTAssertTrue(!app.tabBars.buttons["SignMail"].exists)
	}
	
	/*
	Name: verifySignMailGreetingTypes
	Description: Verify SignMail Greeting Types
	Parameters: count
	Example: verifySignMailGreetingTypes(count: 3)
	*/
	func verifySignMailGreetingTypeCount(count: Int) {
		let typeCount = app.segmentedControls.children(matching: .button).count
		if typeCount > count {
			XCTFail("Too Many Greeting Types")
		}
		if typeCount < count {
			XCTFail("Too Few Greeting Types")
		}
	}
	
	
	/*
	Name: verifySignMailOptions()
	Description: Verify SignMail options
	Parameters: none
	Example: verifySignMailOptions()
	*/
	func verifySignMailOptions() {
		XCTAssertTrue(app.tables.cells.staticTexts["Add Contact"].exists)
		XCTAssertTrue(app.tables.cells.staticTexts["Add To Existing Contact"].exists)
		XCTAssertTrue(app.tables.cells.staticTexts["Send SignMail"].exists)
		XCTAssertTrue(app.tables.cells.staticTexts["Block Caller"].exists)
	}
	
	/*
	Name: verifySignMailRecording()
	Description: Verify SignMail Recording status
	Parameters: none
	Example: verifySignMailRecording()
	*/
	func verifySignMailRecording() {
		let recording = waitFor(query: app.staticTexts, value: "REC")
		XCTAssertTrue(recording.exists)
	}
	
	/*
	Name: verifySignMailSendOptions()
	Description: Verify SignMail Send options
	Parameters: none
	Example: verifySignMailSendOptions()
	*/
	func verifySignMailSendOptions() {
		XCTAssertTrue(app.sheets["Confirm SignMail Send"].buttons["Send"].exists)
		XCTAssertTrue(app.sheets["Confirm SignMail Send"].buttons["Record Again"].exists)
		XCTAssertTrue(app.sheets["Confirm SignMail Send"].buttons["Exit"].exists)
	}
	
	/*
	Name: verifySignMailUnwatched(phnum:)
	Description: Verify SignMail not watched by phone number
	Parameters: phnum
	Example: verifySignMailUnwatched(phnum: "19323883030")
	*/
	func verifySignMailUnwatched(phnum: String) {
		// Format the phone number
		let phoneNumber = String (self.formatPhoneNumber(num: phnum))
		XCTAssertTrue(app.tables.cells.containing(.staticText, identifier: phoneNumber).otherElements["viewedDot"].exists)
	}
	
	/*
	Name: verifySignMailWatched(phnum:)
	Description: Verify SignMail watched by phone number
	Parameters: phnum
	Example: verifySignMailWatched(phnum: "19323883030")
	*/
	func verifySignMailWatched(phnum: String) {
		// Format the phone number
		let phoneNumber = String (self.formatPhoneNumber(num: phnum))
		XCTAssertFalse(app.tables.cells.containing(.staticText, identifier: phoneNumber).otherElements["viewedDot"].exists)
	}
	
	/*
	Name: verifySkipGreetingBtn()
	Description: Verify the Skip Greeting button
	Parameters: none
	Example: verifySkipGreetingBtn()
	*/
	func verifySkipGreetingBtn() {
		// Select the Skip Greeting button
		if (!app.buttons["Skip"].exists) {
			self.selectTop()
		}
		XCTAssertTrue(app.buttons["Skip"].exists, "Skip greeting button not found")
	}
	
	/*
	Name: verifySpanishFeatureTitle()
	Description: Verify Spanish Feature Title
	Parameters: none
	Example: verifySpanishFeatureTitle()
	*/
	func verifySpanishFeatureTitle() {
		XCTAssertTrue(app.staticTexts["Enable Spanish Features"].exists)
	}
	
	/*
	Name: verifySpanishSwitchState()
	Description: Verify the state of the Spanish switch in a contact
	Parameters: state
	Example: verifySpanishSwitchState(state: "1")
	*/
	func verifySpanishSwitchState(state: String) {
		if #available(iOS 13.0, *) {
			let switchState = app.tables.switches.matching(identifier: "Spanish VRS").element.value.debugDescription
			XCTAssertTrue(switchState.contains(state))
		}
		else {
			let switchState = app.tables.switches.matching(identifier: "Spanish VRS").element.debugDescription
			XCTAssertTrue(switchState.contains("value: " + state))
		}
	}
	
	/*
	Name: verifySwitchCallNotEnabled()
	Description: Verify the disabled state of Switch Call button while in a ringing call
	Parameters: none
	Example: verifySwitchCallNotEnabled()
	*/
	func verifySwitchCallNotEnabled() {
		XCTAssertFalse(app.staticTexts["icon swap"].exists)
	}
	
	/*
	Name: verifySwitchCameraButton()
	Description: Verify Switch Camera button is visible
	Parameters: none
	Example: verifySwitchCameraButton()
	*/
	func verifySwitchCameraButton() {
		XCTAssertTrue(app.buttons["SwitchCamera"].exists)
	}
	
	/*
	Name: verifyTabSelected(tab:)
	Description: Verify the selected tab
	Parameters:	tab
	Example: verifyTabSelected(tab: "Contacts")
	*/
	func verifyTabSelected(tab: String) {
		XCTAssertTrue(app.tabBars.buttons[tab].isSelected)
	}
	
	/*
	Name: verifyTargetBitrateSentLessThan(value:)
	Description: Verify Target bitrate in call stats less than value passed
	Parameters:	value
	Example: verifyTargetBitrateSentLessThan(value: 513)
	*/
	func verifyTargetBitrateSentLessThan(value: Int) {
		let textCount = app.staticTexts.count
		let count : Int = Int(textCount);
		
		// Get Strings
		var textList = [String]()
		//for var index = 0; index < count; index+=1 {
		for index in 0 ..< count {
			let i = Int(index)
			let textValue = app.staticTexts.element(boundBy: i).label
			textList.append(textValue)
		}
		
		// Verify Bitrate Sent
		//for var index = 0; index < count; index+=1 {
		for index in 0 ..< count {
			if (textList[index].hasPrefix("Out")) {
				let outArray = textList[index].components(separatedBy: " ")
				let outValue = outArray[3]
				let outString: NSString = outValue as NSString
				let outBR: Int = Int(outString.intValue)
				XCTAssertTrue(outBR < value, "Bitrate sent: " + outArray[3] + " not less than " + String(value))
				break
			}
		}
	}
	
	/*
	Name: verifyTransferError()
	Description: Verify Call Transfer Error
	Parameters:	none
	Example: verifyTransferError()
	*/
	func verifyTransferError() {
		XCTAssertTrue(app.alerts.staticTexts["Your call could not be transferred to the specified phone number."].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyText()
	Description: Verify if text exists or not based on expectation
	Parameters:	text, expected
	Example: verifyText(text: "Unblocker Caller", true)
	*/
	func verifyText(text: String, expected: Bool) {
		if (expected) {
			XCTAssertTrue(app.staticTexts[text].exists)
		} else {
			XCTAssertFalse(app.staticTexts[text].exists)
		}
	}
	
	/*
	Name: verifyUnableToPlaceVRSCalls()
	Description: Verify unable to place VRS calls
	Parameters:	none
	Example: verifyUnableToPlaceVRSCalls()
	*/
	func verifyUnableToPlaceVRSCalls() {
		XCTAssertTrue(app.alerts.staticTexts["This account is not allowed to place VRS calls."].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyUnableToBlock()
	Description: Verify unable to block additional numbers
	Parameters:	none
	Example: verifyUnableToBlock()
	*/
	func verifyUnableToBlock() {
		let blockFullAlert = app.alerts["No more numbers can be blocked."]
		XCTAssertTrue(blockFullAlert.exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyUnableToBlockNoNumber()
	Description: Verify unable to block a description with no numbers
	Parameters:	none
	Example: verifyUnableToBlockNoNumber()
	*/
	func verifyUnableToBlockNoNumber() {
		XCTAssertTrue(app.alerts["Unable to Save"].exists)
		self.clickButton(name: "OK", query: app.alerts.buttons)
	}
	
	/*
	Name: verifyUserGuidesTitle()
	Description: Verify User Guides Title
	Parameters:	none
	Example: verifyUserGuidesTitle()
	*/
	func verifyUserGuidesTitle() {
		XCTAssertTrue(app.staticTexts["User Guides"].exists)
	}
	
	/*
	Name: verifyUserNameAndNumber()
	Description: Verify user name and number on personal info screen
	Parameters: none
	Example: verifyUserNameAndNumber()
	**/
	func verifyUserNameAndNumber() {
		// Verify user name
		XCTAssertTrue(app.staticTexts[cfg["Name"]!].exists)
		
		// Format and verify phone number
		let phnum = formatPhoneNumber(num: cfg["Phone"]!)
		XCTAssertTrue(app.staticTexts[phnum].exists)
	}
	
	/*
	Name: verifyUseVoiceDisabled()
	Description: Verify Use Voice disabled
	Parameters: none
	Example: verifyUseVoiceDisabled()
	*/
	func verifyUseVoiceDisabled() {
		if #available(iOS 13.0, *) {
			// Get Use Voice Switch State
			let switchState = app.tables.switches.matching(identifier: "Use voice").element.value.debugDescription
			
			// Verify Disabled
			XCTAssertTrue(switchState.contains("0"))
		}
		else {
			// Get Use Voice Switch State
			let switchState = app.tables.switches.matching(identifier: "Use voice").element.debugDescription
			
			// Verify Disabled
			XCTAssertTrue(switchState.contains("value: 0"))
		}
	}
	
	/*
	Name: verifyUseVoiceEnabled()
	Description: Verify Use Voice enabled
	Parameters: none
	Example: verifyUseVoiceEnabled()
	*/
	func verifyUseVoiceEnabled() {
		if #available(iOS 13.0, *) {
			// Get Use Voice Switch State
			let switchState = app.tables.switches.matching(identifier: "Use voice").element.value.debugDescription
			
			// Verify Disabled
			XCTAssertTrue(switchState.contains("1"))
		}
		else {
			// Get Use Voice Switch State
			let switchState = app.tables.switches.matching(identifier: "Use voice").element.debugDescription
			
			// Verify Enabled
			XCTAssertTrue(switchState.contains("value: 1"))
		}
	}
	
	/*
	Name: verifyVCOSwitchState(SettingValue:)
	Description: Verify VCO is Enabled/Disabled in Settings
	Parameters:	setting
	Example: verifyVCOSwitchState(settingValue: "1")
	*/
	func verifyVCOSwitchState(settingValue: String) {
		// iPad
		if UIDevice.current.userInterfaceIdiom == .pad {
			let switchState = app.tables.switches.matching(identifier: "1-Line Voice Carry Over (VCO)").element(boundBy: 0).value.debugDescription
			XCTAssertTrue(switchState.contains(settingValue))
		}
		// iPhone and iPod
		else {
			let switchState = app.tables.switches.matching(identifier: "1-Line VCO").element(boundBy: 0).value.debugDescription
			XCTAssertTrue(switchState.contains(settingValue))
		}
	}
	
	/*
	Name: verifyVersion(version: String)
	Description: Verify Current Major Version
	Parameters:	version
	Example: ui.verifyVersion(version: "9.3")
	*/
	func verifyVersion(version: String) {
		let iOSVersion = "iOS Version " + version
		XCTAssertTrue(app.staticTexts.debugDescription.contains(iOSVersion), "Version Not Found")
	}
	
	/*
	Name: verifyVideoPrivacyDisabled()
	Description: Verify Video Privacy is disabled
	Parameters:	none
	Example: verifyVideoPrivacyDisabled()
	*/
	func verifyVideoPrivacyDisabled() {
		XCTAssertFalse(app.otherElements["RemoteViewPrivacy"].exists)
	}
	
	/*
	Name: verifyVideoPrivacyEnabled()
	Description: Verify Video Privacy is enabled
	Parameters:	none
	Example: verifyVideoPrivacyEnabled()
	*/
	func verifyVideoPrivacyEnabled() {
		XCTAssertTrue(app.otherElements["RemoteViewPrivacy"].exists)
	}
	
	/*
	Name: verifyVRSIndicator(name:num:)
	Description: Verify Call History item by Name and Number
	Parameters: num
	Example: verifyVRSIndicator(num: "15553334444")
	*/
	func verifyVRSIndicator(num: String) {
		let phnum = self.formatPhoneNumber(num: num)
		let vrsIndicator = phnum + " / VRS"
		XCTAssertTrue(app.tables.cells.staticTexts[vrsIndicator].exists)
	}
	
	/*
	Name: verifyYelp(result:)
	Description: Verify Yelp Result
	Parameters:	result
	Example: verifyYelp(result: "PizzaRev")
	*/
	func verifyYelp(result: String) {
		XCTAssertTrue(app.staticTexts[result].exists)
	}
	
	/*
	Name: waitFor(query:value:timeout:)
	Description: Wait for value in UI Element Query
	Parameters: query, value, timeout
	Example: waitFor(query: app.tabBars.buttons, value: "Dial")
	*/
	func waitFor(query: XCUIElementQuery, value: String, timeout: Double = 30) -> XCUIElement {
		if !query[value].waitForExistence(timeout: timeout) {
			XCTFail("UI Element: \(value) not found in query.")
		}
		return query[value]
	}
	
	/*
	Name: waitForAgreement()
	Description: Wait for Agreement
	Parameters: none
	Example: waitForAgreements()
	*/
	func waitForAgreement() {
		// Wait for Agreement
		let aggreement = self.waitFor(query: app.navigationBars, value: "DynamicProviderAgreementView", timeout: 60)
		XCTAssertTrue(aggreement.exists, "Agreeemnt not found")
	}
	
	/*
	Name: waitForCallIsNotEncrypted()
	Description: Wait for Call Not Encrypted alert
	Parameters:	none
	Example: waitForCallIsNotEncrypted()
	*/
	func waitForCallIsNotEncrypted() {
		let callNotEncrypted = self.waitFor(query: app.alerts, value: "Call Is Not Encrypted")
		XCTAssertTrue(callNotEncrypted.exists, "Call Not Encrypted alert did not appear")
	}
	
	/*
	Name: waitForCallOptions()
	Description: Wait for Call Options
	Parameters:	none
	Example: waitForCallOptions()
	*/
	func waitForCallOptions() {
		if !app.buttons["Call Options"].waitForExistence(timeout: 30) {
			XCTFail("Call failed to connect - Caller: \(testUtils.callerNum), Callee: \(testUtils.calleeNum) - \(testUtils.getTimeStamp())")
		}
	}
	
	/*
	Name: waitForCallWaiting()
	Description: Wait for Call Waiting
	Parameters:	none
	Example: waitForCallWaiting()
	*/
	func waitForCallWaiting() {
		// Wait for Call Waiting
		let callWaiting = self.waitFor(query: app.buttons, value: "Hold & Answer")
		XCTAssertTrue(callWaiting.exists, "Call Waiting failed")
	}
	
	/*
	Name: waitForCIRDismissible()
	Description: Wait for CIR Dismissible
	Parameters: none
	Example: waitForCIRDismissible()
	*/
	func waitForCIRDismissible() {
		// Wait for CIR Dismissible
		let cirDismissible = self.waitFor(query: app.buttons, value: "Call Customer Care", timeout: 60)
		XCTAssertTrue(cirDismissible.exists, "Send Dismissible CIR failed")
	}
	
	/*
	Name: waitForConfirmSignMailSend()
	Description: Wait for Confirm SignMail Send
	Parameters:	none
	Example: waitForConfirmSignMailSend()
	*/
	func waitForConfirmSignMailSend() {
		// Wait for Confirm SignMail Send
		let confirmSignMailSend = self.waitFor(query: app.sheets, value: "Confirm SignMail Send")
		XCTAssertTrue(confirmSignMailSend.exists, "Failed to recieve Confirm SignMail Send")
	}
	
	/*
	Name: waitForGreeting()
	Description: Wait for Greeting video screen to appear
	Parameters:	textOnly, text
	Example: waitForGreeting(textOnly: true, text: "7474")
	*/
	func waitForGreeting(textOnly: Bool = false, text: String = "") {
		if textOnly {
			verifyGreeting(text: text)
		} else {
			XCTAssertTrue(app.otherElements["RemoteViewVideo"].waitForExistence(timeout: 40))
		}
	}
	
	/*
	Name: waitForIncomingCall()
	Description: Wait for Incoming Call
	Parameters:	none
	Example: waitForIncomingCall()
	*/
	func waitForIncomingCall() {
		// CallKit
		if #available(iOS 18.0, *) {
			if !inCallServiceApp.buttons["acceptCall"].waitForExistence(timeout: 30) {
				XCTFail("Incoming call failed - Caller: \(testUtils.callerNum), Callee: \(testUtils.calleeNum) - \(testUtils.getTimeStamp())")
			}
		}
		else {
			if !springboard.buttons["Accept"].waitForExistence(timeout: 30) {
				XCTFail("Incoming call failed - Caller: \(testUtils.callerNum), Callee: \(testUtils.calleeNum) - \(testUtils.getTimeStamp())")
			}
		}
	}
	
	/*
	Name: waitForIncomingCall()
	Description: Wait for Incoming Call
	Parameters:	none
	Example: waitForIncomingCall(callerID:)
	*/
	func waitForIncomingCall(callerID: String) {
		self.waitForIncomingCall()
		self.verifyCallerID(text: callerID)
	}
	
	/*
	Name: waitForIncomingNtouchCall()
	Description: Wait for Incoming ntouch call window
	Parameters:	none
	Example: waitForIncomingNtouchCall()
	*/
	func waitForIncomingNtouchCall() {
		// ntouch
		if !app.buttons["Answer"].waitForExistence(timeout: 30) {
			XCTFail("Incoming call failed - Caller: \(testUtils.callerNum), Callee: \(testUtils.calleeNum) - \(testUtils.getTimeStamp())")
		}
	}
	
	/*
	Name: waitForMissedCallBadge(count:)
	Description: Wait for Missed Call badge with calls count
	Parameters:	count
	Example: waitForMissedCallBadge(count: "1")
	*/
	func waitForMissedCallBadge(count: String) {
		self.heartbeat()
		
		// Wait for Missed Call Badge
		let predicate = NSPredicate(format: "value.debugDescription CONTAINS %@", count)
		let expectation = XCTNSPredicateExpectation(predicate: predicate, object: app.tabBars.buttons["History"])
		let result = XCTWaiter().wait(for: [expectation], timeout: 60)
		XCTAssertEqual(result, .completed, "Failed to receive Missed Call Notification")
	}
	
	/*
	Name: waitForMyNumber()
	Description: Wait for the My Number label
	Parameters:	none
	Example: waitForMyNumber()
	*/
	func waitForMyNumber() {
		// check if logged out
		if app.buttons["Log In"].exists {
			self.rememberPassword(setting: true)
			self.login(phnum: UserConfig()["Phone"]!, password: UserConfig()["Password"]!)
		}
		
		// go to Home
		self.gotoHome()
		
		// Wait for My Number
		let myNumber = self.formatPhoneNumber(num: cfg["Phone"]!)
		let maxWait: TimeInterval = 10
		if !app.textViews[myNumber].waitForExistence(timeout: maxWait) {
			self.logout()
			self.rememberPassword(setting: true)
			self.login(phnum: UserConfig()["Phone"]!, password: UserConfig()["Password"]!)
			self.gotoHome()
			
			// Wait for My Number
			if !app.textViews[myNumber].waitForExistence(timeout: maxWait) {
				XCTFail("Wrong number, or login failed?")
			}
		}
	}
	
	/*
	Name: waitForSignMail(phnum:)
	Description: Wait for SignMail by phone number
	Parameters: phnum
	Example: waitForSignMail(phnum: "19323883030")
	*/
	func waitForSignMail(phnum: String) {
		// Format the phone number
		let phoneNumber = String (self.formatPhoneNumber(num: phnum))
		
		// Wait for SignMail message
		for _ in 1...100 {
			// Refresh the SignMail List
			if app.staticTexts[phoneNumber].waitForExistence(timeout: 3) {
				break
			}
			else {
				self.heartbeat()
			}
		}
		XCTAssertTrue(app.staticTexts[phoneNumber].exists)
	}
	
	/*
	Name: waitForSignMailRecording()
	Description: Verify SignMail has started recording
	Parameters: none
	Example: waitForSignMailRecording()
	*/
	func waitForSignMailRecording() {
		self.selectTop()
		let signMailRecording = self.waitFor(query: app.staticTexts, value: "02:00", timeout: 120)
		XCTAssertTrue(signMailRecording.exists, "SignMail failed to record")
		sleep(1)
	}

	/*
	Name: waitForStopBtn()
	Description: Wait for Stop Recording Button
	Parameters:	none
	Example: waitForStopBtn()
	*/
	func waitForStopBtn() {
		let stopButton = self.waitFor(query: app.buttons, value: "Stop")
		XCTAssertTrue(stopButton.exists, "No Stop Recording Button")
	}

	/*
	Name: waitForTabBar()
	Description: Wait for the tabBar
	Parameters:	none
	Example: waitForTabBar()
	*/
	func waitForTabBar() {
		let hittable = NSPredicate(format: "hittable == true")
		let expectation = XCTNSPredicateExpectation(predicate: hittable, object: app.tabBars.buttons["Dial"])
		let result = XCTWaiter().wait(for: [expectation], timeout: 60)
		XCTAssertEqual(result, .completed, "TabBar Home button not hittable, login failed?")
		sleep(10)
	}
	
	/*
	Name: waitForUserRegistration()
	Description: Wait for User Registration
	Parameters: none
	Example: waitForAgreements()
	*/
	func waitForUserRegistration() {
		// Wait for User Registration
		let registration = self.waitFor(query: app.navigationBars, value: "FCC Registration", timeout: 60)
		XCTAssertTrue(registration.exists, "Send TRS Registration failed")
	}
	
	/*
	Name: yelpAddContact(result:)
	Description: Add a contact from Yelp Results
	Parameters:	result
	Example: yelpAddContact(result: "Pizza")
	*/
	func yelpAddContact(result: String) {
		self.clickStaticText(text: result)
		self.clickButton(name: "Add Contact", query: app.sheets.buttons, delay: 3)
		self.clickButton(name: "Done", query: app.navigationBars.buttons, delay: 3)
	}
	
	/*
	Name: yelpSearch(search:)
	Description: Perform a Yelp Search
	Parameters:	search
	Example: yelp(search: "Pizza")
	*/
	func yelp(search: String) {
		self.gotoYelp()
		self.clickAndEnterSearch(field: "Search Contacts & Yelp", text: search)
	}
}
