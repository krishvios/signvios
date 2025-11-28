//
//  AccountTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 4/20/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class AccountTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let altNum = UserConfig()["accountAltNum"]!
	let vrsNum = "18015757669"
	let num500Contacts = "18018679000"
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

	func test_0617_7156_InvalidEndpointType() {
		// Test Case: 0617 - Account - Sign In:  MVRS does not allow a user to login to an account associated to a different device type
		// Test Case: 7156 - Account - Sign In: When using a Desktop or VP number to login to an Mobile device the "Endpoint Mismatch" error will display
		
		// Logout
		ui.logout()

		// Login with VP account
		ui.enterPhoneNumber(phnum: UserConfig()["nvpNum"]!)
		ui.enterPassword(password: defaultPassword)
		ui.selectDone()

		// Verify Invalid Endpoint Type
		ui.verifyInvalidEndpointType()

		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_0618_C36946_InvalidLogin() {
		// Test Case: 0618 - Account - Sign In: Failed attempts time out
		// Test Case: C35946 - Account - Sign In: A password mistake should not cause Cannot Log In with correct credentials
		
		// Logout
		ui.logout()
		
		// Login with invalid pw
		ui.enterPassword(password: "1111")
		ui.selectDone()
		
		// Verify Invalid Login
		ui.verifyInvalidLogin()
		
		// 0618
		// Verify Invalid Login again
		ui.selectLogin()
		ui.verifyInvalidLogin()
		
		// C35946
		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_1940_AccountInfo() {
		// Test Case: 1940 - Account - Password: Account Information page is available in settings page

		ui.gotoPersonalInfo()
		ui.gotoHome()
	}
	
	func test_1942_InvalidOldPassword() {
		// Test Case: 1942 - Account - Password: Old Password Field Information incorrect

		// Go to Change Password
		ui.gotoPersonalInfo()
		ui.selectChangePassword()

		// Enter invalid old password
		ui.enterOldPassword(oldPassword: "1111")
		ui.selectChangePassword()

		// Verify old password invalid
		ui.verifyInvalidOldPassword()
		ui.gotoHome()
	}
	
	func test_1944_1950_PasswordLength() {
		// Test Case: 1944 - Account - Password- Confirm Password Field: Verify correct number of characters(1-15)
		// Test Case: 1950 - Account - Password- Confirm Password Field: matches New Password field
		
		// Test 1944
		// Change password to 1 character
		ui.changePassword(old: defaultPassword, new: "1", confirm: "1")
		
		// Change password to 8 characters
		ui.changePassword(old: "1", new: "12345678", confirm: "12345678")
		
		// Change password to 15 characters
		ui.changePassword(old: "12345678", new: "123456789012345", confirm: "123456789012345")
		
		// Test 1950
		// Restore password
		ui.changePassword(old: "123456789012345", new: defaultPassword, confirm: defaultPassword)
	}
	
	func test_1952_ChangePasswordScreenBackground() {
		// Test Case: 1952 - Account - Password: Background after entering all information
		
		// Go to Personal Info and select change password
		ui.gotoPersonalInfo()
		ui.selectChangePassword()
		
		// Enter old password
		ui.enterOldPassword(oldPassword: defaultPassword)
		
		// Enter new password
		ui.enterNewPassword(newPassword: "1111")
		
		// Confirm new password
		ui.enterNewPasswordConfirm(newPassword: "1111")
		
		// Background the app and re-open ntouch
		ui.pressHomeBtn()
		ui.ntouchActivate()
		
		// Verify Password text fields cleared
		ui.verifyPasswordFieldsEmpty()
		
		// Go to home
		ui.gotoHome()
	}
	
	func test_1955_1958_2740_2748_4474_C35947_LogoutLogin() {
		// Test Case: 1955 - Account - Sign In: App provides option to logout in settings
		// Test Case: 1958 - Account - Sign In: App will log out if user chooses to log out from settings.
		// Test Case: 2740 - Account - Sign In:  When logging out of DUT the app will go to the Login Screen
		// Test Case: 2748 - Account - Sign In: User is able to Sign In
		// Test Case: 4474 - Account - Sign In: Verify DUT doesn't log itself out after loggin in
		// Test Case: C35947 - Account - Sign In: ntouch DUT "Attempting to Login..."

		// Test C35947 - Logout and log back in as expected
		// Test 1955, 1958, 4474
		// Logout
		ui.logout()
		
		// Test 2740, 2748, 4474
		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		
		// Go to home
		ui.gotoHome()
	}
	
	func test_1960_1961_1962_4622_RememberPassword() {
		// Test Case: 1960 - Account - Sign In: Remember password enabled
		// Test Case: 1961 - Account - Sign In: Remember password disabled
		// Test Case: 1962 - Account - Sign In: Toggling Remember password on if it is off will not fill in the password field
		// Test Case: 4462 - Account - Sign In: Password will be hidden

		// Test 1960
		// Remember Password Switch
		ui.logout()
		ui.rememberPassword(setting: true)
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.logout()

		// Test 4622
		// Verify Password Remember
		ui.verifyPassword()

		// Test 1961
		ui.rememberPassword(setting: false)
		ui.verifyPasswordBlank()
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.logout()

		// Test 1962
		// Verify Password Is Blank
		ui.ntouchTerminate()
		ui.ntouchActivate()
		ui.verifyPasswordBlank()
		ui.rememberPassword(setting: true)
		ui.verifyPasswordBlank()
		ui.enterPassword(password: defaultPassword)
		ui.login()
		ui.gotoHome()
	}
	
	func test_2103_LogoutForceClose() {
		// Test Case: 2103 - Account - Sign In: App will not crash if sitting on Login Screen after being force closed
		
		// Logout and Force Close ntouch
		ui.logout()
		ui.ntouchTerminate()
		
		// Open ntouch and login
		ui.ntouchActivate()
		ui.login()
		ui.gotoHome()
	}
	
	func test_2246_LoginScreenNoCallTS() {
		// Test Case: 2264 - Account - Sign In: Option to call TS is not available on Sign In screen

		// Logout
		ui.logout()

		// Verify no call Tech Support
		ui.verifyNoCallTS()

		// Login
		ui.login()
		ui.gotoHome()
	}
	
	func test_2716_ForgotPassword() {
		// Test Case: 2716 - Account - Sign In: The text for password reads "Forgot Password"

		// Logout
		ui.logout()

		// Verify Forgot Password
		ui.verifyForgotPassword()

		// Login
		ui.login()
		ui.gotoHome()
	}
	
	func test_2837_LoggedOutNoCalls() {
		// Test Case: 2837 Account - Sign In: DUT can not receiving calls while logged out

		// Logout of 1st account and login 2nd account
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Verify call to offline account goes to SignMail
		ui.call(phnum: dutNum)
		ui.waitForGreeting()
		ui.hangup()

		// Clean up
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_2870_InvalidLoginBackground() {
		// Test Case: 2870 - Account - Sign In: Multiple "Cannot login"  error  does not repeatedly display the error message
		
		// Logout
		ui.logout()
		
		// Login with invalid pw
		ui.enterPassword(password: "1111")
		ui.selectDone()
		
		// Verify Invalid Login and background the app
		ui.verifyInvalidLogin()
		ui.pressHomeBtn()
		
		// Verify Invalid Login again and background
		ui.ntouchActivate()
		ui.selectLogin()
		ui.pressHomeBtn()
		ui.ntouchActivate()
		ui.verifyInvalidLogin()
		
		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_2941_SwitchingAccountsLargeDifferenceInContacts() {
		// Test Case 2941 Account Sign in: Switching accounts quickly does not crash the app even if there is a large disparity between the numbers of contacts on the two accounts.
		
		// Alt Num has 500 Contacts
		// Setup DUT Account 1 contact
		ui.deleteAllContacts()
		ui.gotoSorensonContacts()
		ui.addContact(name: "Automate 2941", homeNumber: vrsNum, workNumber: "", mobileNumber: "")
		
		// Switch between 2 accounts 5 times
		for _ in 1...5 {
			ui.switchAccounts(number: altNum, password: defaultPassword)
			ui.switchAccounts(number: dutNum, password: defaultPassword)
		}
		
		// Clean up DUT Account
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.deleteAllContacts()
		ui.gotoHome()
	}
	
	func test_2956_ChangePasswordSpecialCharacters() {
		// Test Case: Account - Password- Confirm Password Field: All special characters
		
		// Go to Change Password
		ui.gotoPersonalInfo()
		ui.selectChangePassword()

		// Change password to special characters
		ui.enterOldPassword(oldPassword: defaultPassword)
		ui.enterNewPassword(newPassword: "!@#$%^&*()")
		ui.enterNewPasswordConfirm(newPassword: "!@#$%^&*()")
		ui.selectChangePassword()
		
		// Verify invalid password characters
		ui.verifyInvalidPasswordCharacters()
		ui.gotoHome()
	}
	
	func test_3011_LoginWithManyContactsDoesNotCrash() {
		// Test Case: Account - Log Out: Logging into and out of a device with many contacts does not crash the app.

		// Empty contact list to prepare for import
		ui.deleteAllContacts()

		// Import contacts
		ui.importVPContacts(phnum: num500Contacts, pw: defaultPassword, expectedNumber: "500")

		// Log in and out of account and verify app does not crash
		for _ in 1...5 {
			ui.switchAccounts(number: dutNum, password: defaultPassword)
			sleep(3)
		}

		// Cleanup
		ui.deleteAllContacts()
	}
	
	func test_3188_PhoneNumberLength() {
		// Test Case: Account - Sign In: User is able to sign in with 10 or 11 digit number

		// Logout and Login with 10 digit num
		let num = String(dutNum.dropFirst())
		ui.switchAccounts(number: num, password: defaultPassword)

		// Logout and Login with 11 digit num
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_3702_InvalidLoginValidLoginCall() {
		// Test Case: 3702 - Account - Sign In: A user, while logging in, enters the wrong password, upon entering the correct password and logging in and DUT will allow user to place the call
		
		// Logout
		ui.logout()
		
		// Login with invalid pw
		ui.enterPassword(password: "1111")
		ui.selectDone()
		
		// Verify Invalid Login
		ui.verifyInvalidLogin()
		
		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
		
		// Place Call
		ui.call(phnum: vrsNum)
		ui.hangup()
	}
	
	func test_4566_BlankPassword() {
		// Test Case: 4566 - Account - Password: Attempting to login with blank password or phone number does not cause app to continue to attempt to log in

		// Logout
		ui.logout()

		// Login with blank password
		ui.enterPassword(password: "")
		ui.selectLogin()

		// Verify Invalid Login
		ui.verifyBlankPassword()

		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_4628_LoginScreenBackground() {
		// Test Case: 4628 - Account - Sign In: User can background and foreground and login will display

		// Logout and background login screen
		ui.logout()
		ui.pressHomeBtn()

		// open ntouch and login
		ui.ntouchActivate()
		ui.login()
		ui.gotoHome()
	}
	
	func test_6025_7204_LoginYelpSearch() {
		// Test Case: 6025 - YNS: Verify the user can search using yelp by typing in the directory
		// Test Case: 7204 - Account - Yelp - Sign In: Signing in and quickly using the yelp search works as expected
		
		// Logout
		ui.logout()
		
		// Login and Search for pazzo
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.yelp(search: "university of utah")
		
		// Verify Yelp Result
		ui.verifyYelp(result: "The University of Utah")
		
		// Cleanup
		ui.clearYelpSearch()
		ui.gotoHome()
	}
	
	func test_7002_ChangePasswordSame4x() {
		// Test Case: 7002 - Account - Password: Changing password 3 times concurrently  doesn't produce an incorrect password notification on 4th attempt
		
		// Change Password to the same password 4 times
		for _ in 1...4 {
			ui.changePassword(old: defaultPassword, new: defaultPassword, confirm: defaultPassword)
		}
		
		// Go to home
		ui.gotoHome()
	}
	
	func test_7111_InvalidLogin12DigitNumber() {
		// Test Case: 7111 - Account: Correct number stored in  accounts after failed login attempt

		// Logout
		ui.logout()

		// Login with 12 digit number
		ui.enterPhoneNumber(phnum: "112345678900")
		ui.enterPassword(password: defaultPassword)
		ui.selectLogin()

		// Verify Invalid Phone Number
		ui.verifyInvalidPhoneNumber()

		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_7113_LoginSwitchAccounts() {
		// Test Case: 7113 Account - Sign In: DUT can not receiving calls while logged out

		// Logout of 1st account and login 2nd account
		ui.switchAccounts(number: altNum, password: defaultPassword)

		// Clean up
		ui.switchAccounts(number: dutNum, password: defaultPassword)
	}
	
	func test_7422_PopulateCorrectLoggedNumber() {
		// Test Case 7422 Account Sign In: Selecting numbers from drop down list on log in screen will choose correct number.
			
		// Logout
		ui.logout()
		
		// Select Logged Number
		ui.selectMultipleUsers()
		
		// Select DUT Number
		ui.selectDutNumber(phnum: dutNum)
		
		// Verify Phone Number
		ui.verifyPhoneNumberTextField(phnum: dutNum)
		
		// Login
		ui.login()
		ui.gotoHome()
	}
	
	func test_8271_ForceCloseRememberPassword() {
		// Test Case: 8271 - Account - Sign In: User is taken to the Home screen after force close

		// Verify remember password is on
		ui.logout()
		ui.rememberPassword(setting: true)
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()

		// Force close, launch app, and go to home
		ui.ntouchTerminate()
		ui.ntouchActivate()
		ui.verifyTabSelected(tab: "Dial")
	}
	
	func test_8402_CaseInsensitiveCharactersPassword() {
		// Test Case 8402 Account - Sign In: Case insensitive characters for a password will allow device to log in.
		
		// Change password to Insensitive characters
		ui.changePassword(old: defaultPassword, new: "QwErTys", confirm: "QwErTys")

		// Logout
		ui.logout()
		
		//Login with Insensitive Password
		ui.login(phnum: dutNum, password: "qwertys")
		ui.gotoHome()
		
		// Change Password Default
		ui.changePassword(old: "qwertys", new: defaultPassword, confirm: defaultPassword)
	}
	
	func test_9342_MultipleUsersDeleteAccountHistory() {
		// Test Case 9342 Account: Multiple previous accounts can be deleted from Account History(iOS)
		
		// Login with 2nd account
		ui.switchAccounts(number: altNum, password: defaultPassword)
		
		// Logout of 2nd account
		ui.logout()
			
		// Select Multiple Users
		ui.selectMultipleUsers()
		
		// Delete Account History
		ui.deleteMultipleUsers()
		
		// Login
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
	
	func test_11171_RememberPasswordForceClose() {
		// Test Case 11171 Account - Sign in: Disabled remember password signs in upon force quit of app

		// Setup
		ui.logout()
		ui.rememberPassword(setting: false)
		ui.login(phnum: dutNum, password: defaultPassword)

		// Force close app, user remains logged in when app is reopened
		ui.ntouchTerminate()
		ui.ntouchActivate()
		ui.gotoHome()

		// Cleanup
		ui.logout()
		ui.rememberPassword(setting: true)
		ui.login(phnum: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
}
