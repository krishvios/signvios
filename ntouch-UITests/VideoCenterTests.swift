//
//  VideoCenterTests.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 3/30/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import XCTest

class VideoCenterTests: XCTestCase {

	let ui = UI()
	let core = CoreAccess()
	let dutNum = UserConfig()["Phone"]!
	let defaultPassword = UserConfig()["Password"]!
	let endpointsClient = TestEndpointsClient.shared
	let vcNum = "19323388677"
	
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
	
	func test_2556_2557_2639_VCPlayVideo() {
		// Test Case: 2556 - Video Center: Play a Video in portrait
		// Test Case: 2557 - Video Center: Play a Video in Landscape
		// Test Case: 2639 - Video Center: Videos are displayed on the Sorenson Channel

		// Test 2639
		// Login Account with VC Video
		ui.switchAccounts(number: vcNum, password: defaultPassword)
		ui.gotoVideoCenter()
		ui.selectVideoCenter(channel: "Sorenson")

		// Test 2557
		// Play VC Video Landscape
		ui.rotate(dir: "Left")
		ui.playVideo(title: "Turbine")
		sleep(20)
		ui.selectVideoDoneBtn()

		//Test 2556
		// Play VC Video Portrait
		ui.rotate(dir: "Portrait")
		ui.playVideo(title: "Turbine")
		sleep(20)
		ui.selectVideoDoneBtn()

		// Cleanup
		ui.rotate(dir: "Portrait")
		ui.switchAccounts(number: dutNum, password: defaultPassword)
		ui.gotoHome()
	}
}
