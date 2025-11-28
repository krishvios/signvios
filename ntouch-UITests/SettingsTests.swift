//
//  SettingsTests.swift
//  ntouch
//
//  Created by Mikael Leppanen on 3/2/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import XCTest

class SettingsTests: XCTestCase {
	
	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["settingsAltNum"]!
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
		// TODO: Server for CoreAccess currently not available
//		core.setDoNotDisturb(value: "0")
//		core.setHideMyCallerID(value: "0")
		ui.ntouchTerminate()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_2081_AutoSpeed() {
//		// Test Case: 2081 - Settings: Auto is the default bitrate
//		// Enable Auto Speed
//		core.setAutoSpeed(value: "1")
//		ui.heartbeat()
//		
//		// Verify Auto Speed (no setting)
//		ui.gotoSettings()
//		ui.verifyNoNetworkSpeedSetting()
//		
//		// Go to home
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_2082_Bitrates() {
//		// Test Case: 2082 - Settings: All bitrate values appear
//		// Disable Auto Speed
//		core.setAutoSpeed(value: "0")
//		ui.heartbeat()
//		
//		// Verify NetworkSpeedBitrates
//		ui.gotoSettings()
//		ui.verifyNetworkSpeedBitrates()
//		
//		// Enable Auto Speed
//		core.setAutoSpeed(value: "1")
//		ui.heartbeat()
//		
//		// Go to home
//		ui.gotoHome()
//	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_2083_SetBitrate() {
//		// Test Case: 2083 - Settings: Network Speed changes when new bitrate selected
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint), let oepNum = endpoint.number else { return }
//		
//		// Disable Auto Speed
//		core.setAutoSpeed(value: "0")
//		ui.heartbeat()
//		
//		// Set Speed
//		ui.gotoSettings()
//		ui.selectNetworkSpeed(setting: "512 Kbps")
//		
//		// Enable Stats
//		ui.toggleStats()
//		
//		// check bitrate
//		ui.call(phnum: oepNum)
//		con.answer()
//		ui.verifyTargetBitrateSentLessThan(value: 513)
//		sleep(30)
//		ui.verifyTargetBitrateSentLessThan(value: 513)
//		
//		// Hangup
//		ui.hangup()
//		
//		// Enable Auto Speed
//		core.setAutoSpeed(value: "1")
//		ui.heartbeat()
//		ui.gotoHome()
//	}
	
	func test_2214_RingsBeforeSignMailVerify() {
		// Navigate to Settings
		ui.gotoSettings()
		
		// Open Play SignMail Greeting After Menu
		ui.gotoPlaySignMailGreetingAfter()
		
		// Verify 5-15 settins are available
		ui.verifyRingsBeforeGreetingList()
		
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_2265_MissedCallDND() {
//		// Test Case: 2265 - Settings: Missed calls recorded with Do Not Disturb enabled
//		let endpoint = endpointsClient.checkOut()
//		guard let con = endpointsClient.tryToConnectToEndpoint(endpoint) else { return }
//		
//		// Delete Call History
//		ui.gotoCallHistory(list: "All")
//		ui.deleteAllCallHistory()
//		
//		// Enable DND
//		ui.gotoSettings()
//		ui.doNotDisturb(setting: 1)
//		
//		// Call DUT
//		con.dial(number: dutNum)
//		sleep(3)
//		con.hangup()
//		
//		// wait for missed call
//		ui.waitForMissedCallBadge(count: "1")
//		
//		// Disable DND
//		core.setDoNotDisturb(value: "0")
//		ui.heartbeat()
//		ui.gotoHome()
//	}
	
	func test_2300_TurnOnVoice() {
		// Test Case: 2300 - VCO: Setting Use Voice to ON, Microphone toggle will be displayed
		// Enable Use Voice and Verify
		ui.gotoSettings()
		ui.useVoice(setting: 1)
		ui.gotoHome()
		ui.verifyMicrophoneButtonEnabled()
		
		// Disable Use Voice and Verify
		ui.gotoSettings()
		ui.useVoice(setting: 0)
		ui.gotoHome()
		ui.verifyMicrophoneButtonDisabled()
	}
	
	func test_2550_2552_2711_InvalidEmail() {
		// Test Case: 2550 - SignMail: Only valid email address accepted in SignMail notifications
		// Test Case: 2552 - SignMail: Fields can be left blank in SignMail notification
		// Test Case: 2711 - Settings: Text for invalid  Email Address
		
		// Test 2711
		// Enter invalid email and verify error message
		ui.gotoPersonalInfo()
		ui.enterEmail1(email: "a")
		ui.selectSave()
		ui.verifyInvalidEmail()
		
		// Test 2550
		// Enter email addresses with 256 char and verify invalid
		let email255 = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz12345@test.co";
		let email256 = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz12345@test.com";
		
		
		// Enter email addresses with 256 char verify 255 char
		ui.gotoPersonalInfo()
		ui.enterEmail1(email: email256)
		ui.enterEmail2(email: email256)
		ui.selectSave()
		ui.gotoPersonalInfo()
		ui.verifyEmail1(email: email255)
		ui.verifyEmail2(email: email255)
		
		// Test 2552
		// Enter blank email
		ui.gotoPersonalInfo()
		ui.enterEmail1(email: "")
		ui.enterEmail2(email: "")
		ui.selectSave()
		ui.gotoHome()
	}
	
	func test_2598_RingCount5to15() {
		// Test Case: 2598 Settings: Ring count can go from 5 to 15
		ui.gotoSettings()
		
		// Select Ring Count 15 and Verify 15 Rings is displayed in settings
		ui.gotoPlaySignMailGreetingAfter()
		ui.selectRingBeforeGreeting(setRing: "15")
		ui.verifyRingsBeforeGreeting(checkRings: "15")
		
		// Set back to 5 Rings
		ui.gotoPlaySignMailGreetingAfter()
		ui.selectRingBeforeGreeting(setRing: "5")
		ui.gotoHome()
	}
	
	func test_2603_3818_defaultRingCount() {
		// Test Case: 2603 Settings: Default Ring Count value is 5 Rings
		// Select Ring Count 8 and Verify 8 Rings is displayed in settings
		ui.gotoSettings()
		ui.gotoPlaySignMailGreetingAfter()
		ui.selectRingBeforeGreeting(setRing: "8")
		ui.verifyRingsBeforeGreeting(checkRings: "8")
		
		// Log into Default Account
		ui.switchAccounts(number: altNum, password: defaultPassword)
		ui.gotoHome()
		
		// Test 2603
		// Verify 5 Rings is displayed in settings
		ui.gotoSettings()
		ui.verifyRingsBeforeGreeting(checkRings: "5")
		ui.gotoHome()
		
		// Test 3818
		// Log back in and Verify Ring Count is 8
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoSettings()
		ui.verifyRingsBeforeGreeting(checkRings: "8")
		
		// Set back to 5 Rings
		ui.gotoPlaySignMailGreetingAfter()
		ui.selectRingBeforeGreeting(setRing: "5")
		ui.gotoHome()
	}
	
	func test_2676_EmailAddress40PlusChar() {
		// Test Case: 2676 - Settings: Long email in contact information
		let email40PlusChar1 = "40pluscharacterslong12345678901234@email1.com"
		let email40PlusChar2 = "40pluscharacterslong12345678901234@email2.com"

		// Enter email address
		ui.gotoPersonalInfo()
		ui.enterEmail1(email: email40PlusChar1)
		ui.enterEmail2(email: email40PlusChar2)
		ui.selectSave()

		// Verify email addresses
		ui.gotoPersonalInfo()
		ui.verifyEmail1(email: email40PlusChar1)
		ui.verifyEmail2(email: email40PlusChar2)

		// Cleanup
		ui.deleteEmail1()
		ui.deleteEmail2()
		ui.selectSave()
		ui.gotoHome()
	}

	func test_2677_6101_EmailAddress() {
		// Test Case: 2677 - Settings: SignMail  Email Contact Information allows long Top Level Domain for email
		// Test Case: 6101 - Settings: The Signmail Settings page will have two email text fields both labeled Email Address
		
		// Test 6101
		// Verify email text fields
		ui.gotoPersonalInfo()
		ui.verifyEmailTextfields()
		
		// Test 2677
		// Enter email addresses
		ui.enterEmail1(email: "test@test.travel")
		ui.enterEmail2(email: "test@test.museum")
		ui.selectSave()
		
		// Verify email addresses
		ui.gotoPersonalInfo()
		ui.verifyEmail1(email: "test@test.travel")
		ui.verifyEmail2(email: "test@test.museum")
		
		// Delete email addresses
		ui.deleteEmail1()
		ui.deleteEmail2()
		ui.selectSave()
		ui.gotoHome()
	}
	
	func test_2839_EmailAddress255Char() {
		// Test Case: 2839 - Settings: Email in personal information allows up to 255 characters
		let email255 = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz1234@test.com";
		
		// Enter email addresses with 255 char
		ui.gotoPersonalInfo()
		ui.enterEmail1(email: email255)
		ui.enterEmail2(email: email255)
		ui.selectSave()
		
		// Delete email addresses
		ui.gotoPersonalInfo()
		ui.verifyEmail1(email: email255)
		ui.verifyEmail2(email: email255)
		ui.deleteEmail1()
		ui.deleteEmail2()
		ui.selectSave()
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_2922_SetBitrate128k() {
//		// Test Case: 2922 - Settings: Setting speed to 128k should disable Audio in settings
//		
//		// Disable Auto Speed
//		core.setAutoSpeed(value: "0")
//		ui.heartbeat()
//		
//		// Set Speed to 512k
//		ui.gotoSettings()
//		ui.selectNetworkSpeed(setting: "512 Kbps")
//		
//		// Enable Use Voice
//		ui.useVoice(setting: 1)
//		
//		// Set Speed to 128k
//		ui.selectNetworkSpeed(setting: "128 Kbps")
//		
//		// Verify Audio Disabled
//		ui.verifyAudioDisabledMessage()
//		ui.gotoSettings()
//		ui.verifyUseVoiceDisabled()
//		
//		// Set Speed back to 512k
//		ui.selectNetworkSpeed(setting: "512 Kbps")
//		
//		// Enable Auto Speed
//		core.setAutoSpeed(value: "1")
//		ui.heartbeat()
//		ui.gotoHome()
//	}
	
	func test_4304_UseVoiceDisplaysMicrophone() {
		// Test Case: 4304 - Settings: Turning on Use Voice displays the microphone
		// Enable Use Voice and Verify
		ui.gotoSettings()
		ui.useVoice(setting: 1)
		ui.gotoHome()
		ui.verifyMicrophoneButtonEnabled()
		
		// Disable Use Voice and Verify
		ui.gotoSettings()
		ui.useVoice(setting: 0)
		ui.gotoHome()
		ui.verifyMicrophoneButtonDisabled()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_4496_ContactPhotosDisabled() {
//		// Test Case: 4496 - Settings: Changing accounts from photos enabled to an account with photos disabled does not display the wrong setting
//		// Disable Contact Photos
//		core.setContactPhotos(value: "0")
//		ui.heartbeat()
//		
//		// Add Contact
//		ui.gotoSorensonContacts()
//		ui.addContact(name: "Automate 4496", homeNumber: cfg["nvpNum"]!, workNumber: "", mobileNumber: "")
//		
//		// Verify Default photo instead of profile photo
//		ui.gotoSorensonContacts()
//		ui.verifyContactDefaultPhoto(name: "Automate 4496")
//		
//		// Delete Contact
//		ui.deleteContact(name: "Automate 4496")
//		
//		// Enable Contact Photos
//		core.setContactPhotos(value: "1")
//		ui.heartbeat()
//		ui.gotoHome()
//	}
	
	func test_4524_MicrophoneToggle() {
		// Test Case: 4524 - Settings: On/Off button for Microphone does not switch to on after turning it off and going into the settings
		// Enable Use Voice
		ui.gotoSettings()
		ui.useVoice(setting: 1)
		
		// toggle microphone on
		ui.gotoHome()
		ui.toggleMicrophone(setting: 1)
		ui.verifyMicrophoneToggleOn()
		
		// toggle microphone off
		ui.gotoHome()
		ui.toggleMicrophone(setting: 0)
		ui.verifyMicrophoneToggleOff()
		
		// Verify Use Voice enabled
		ui.gotoSettings()
		ui.verifyUseVoiceEnabled()
		
		// verify microphone is still off
		ui.gotoHome()
		ui.verifyMicrophoneToggleOff()
		
		// cleanup
		ui.gotoSettings()
		ui.useVoice(setting: 0)
		ui.gotoHome()
	}
	
	func test_4626_4627_LoginKeyboard() {
		// Test Case: 4626 - Settings: User can enter a phone number "phone dialer will display"
		// Test Case: 4627 - Settings: User can enter a password "key pad will display"
		
		// Logout
		ui.logout()
		
		// Test 4626
		// Verify keyboard for phone number
		ui.selectPhoneNumberTextbox()
		ui.verifyNumKeyboard()
		
		// Test 4627
		// Verify keyboard for password
		ui.selectPasswordTextbox()
		ui.verifyKeyboard()
		
		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	// TODO: Server for CoreAccess currently not available
//	func test_5259_C36593_EnableDND() {
//		// Test Case: 5259 - Settings: Changing Do Not Disturb Settings in VDM display on DUT
//		// Test Case: C36593 - Settings: DND setting is persistent across app restarts
//		
//		// Enable DND
//		core.setDoNotDisturb(value: "1")
//		ui.heartbeat()
//		
//		// Verify DND Enabled
//		ui.gotoHome()
//		ui.verifyDoNotDisturbStatus(enabled: true)
//		
//		// Disable DND
//		core.setDoNotDisturb(value: "0")
//		ui.heartbeat()
//		ui.gotoHome()
//		ui.verifyDoNotDisturbStatus(enabled: false)
//	}
	
	func test_5285_PersonalInfoNameAndNumber() {
		// Test Case: 5285 - Settings: Name and Phone Number appear on the Personal Information screen
		ui.gotoPersonalInfo()
		ui.verifyUserNameAndNumber()
		ui.gotoHome()
	}
	
	func test_7082_PortraitSettings() {
		// Test Case: 7082 - Settings: User is able to access Settings or help in portrait mode
		// Set Portrait and go to settings
		ui.rotate(dir: "Portrait")
		ui.gotoSettings()
		
		// Verify Settings Page and go to home
		ui.verifySettingsPage()
		ui.rotate(dir: cfg["Orientation"]!)
		ui.gotoHome()
	}
	
	func test_8192_HideCallerID() {
		// Test Case: 8192 - Settings: Hide My Caller ID option is present on the Settings page
		// Enable Hide My Caller ID
		ui.gotoSettings()
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: true)
		
		// Disable Hide My Caller ID
		ui.setSettingsSwitch(setting: "Hide My Caller ID", value: false)
		ui.gotoHome()
	}
	
	func test_9540_SelectTheme() {
		// Test Case: 9540 - Theme:  User will be able to select themes
		// Select Light theme
		ui.gotoSettings()
		ui.clickStaticText(text: "Color Theme")
		ui.clickStaticText(text: "Light")
		
		// Select High Contrast theme
		ui.clickStaticText(text: "Color Theme")
		ui.clickStaticText(text: "High Contrast")
		
		// Select Dark (default) theme
		ui.clickStaticText(text: "Color Theme")
		ui.clickStaticText(text: "Dark (default)")
		ui.gotoHome()
	}
	
	func test_9541_AvailableThemes() {
		// Test Case: 9541 - Settings: There are 3 selectable themes
		ui.gotoSettings()
		ui.clickStaticText(text: "Color Theme")
		ui.verifyAvailableThemes()
		ui.gotoHome()
	}
	
	func test_9590_UserGuides() {
		// Test Case: 9590 - Settings: User Guides title is accurate
		// Verify User Guides Title
		ui.gotoCustomerCare()
		ui.verifyUserGuidesTitle()
		ui.gotoHome()
	}
	
	func test_9639_SettingsSearchResult() {
		// Test Case: 9639 - Settings: Search in settings will show accurate Search Result
		// Test using literal words
		ui.gotoSettings()
		ui.searchSettings(word: "Cell")
		ui.verifySearchFieldResult(word: "Cellular Video Quality")
		
		// Clear the search in Settings
		ui.searchSettingsClear()
		
		// Test using key words like 'video' and Cellular will show up
		ui.searchSettings(word: "video")
		ui.verifySearchFieldResult(word: "Cellular Video Quality")
		ui.verifySearchFieldResult(word: "SignMail Greeting")
		
		// Clear the search to perform more tests
		ui.searchSettingsClear()
		
		// Test using other word
		ui.searchSettings(word: "Bl")
		ui.verifySearchFieldResult(word: "Blocked Numbers")
		ui.verifySearchFieldResult(word: "Hide My Caller ID")
		ui.verifySearchFieldResult(word: "Block Anonymous Callers")
		
		// Clean up
		ui.searchSettingsClear()
		ui.gotoHome()
	}
	
	func test_11251_AppVersion() {
		// Test Case: 11251 - App version: The version won't be 9.0.0 if theme is changed
		let current = "9.4"
		
		// Setup
		ui.gotoSettings()
		
		// Light theme verify version
		ui.clickStaticText(text: "Color Theme")
		ui.clickStaticText(text: "Light")
		ui.verifyVersion(version: current)
		
		// High Contrast theme verify version
		ui.clickStaticText(text: "Color Theme")
		ui.clickStaticText(text: "High Contrast")
		ui.verifyVersion(version: current)
		
		// Dark (default) theme verify version
		ui.clickStaticText(text: "Color Theme")
		ui.clickStaticText(text: "Dark (default)")
		ui.verifyVersion(version: current)
		
		// Cleanup
		ui.gotoHome()
	}
}
