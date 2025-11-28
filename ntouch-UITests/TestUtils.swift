//
//  TestUtils.swift
//  ntouch-UITests
//
//  Created by Mikael Leppanen on 4/2/21.
//  Copyright © 2021 Sorenson Communications. All rights reserved.
//

import Foundation

let testUtils = TestUtils()

class TestUtils {
	var callerNum = UserConfig()["Phone"]!
	var calleeNum = ""
	
	func getTimeStamp() -> String {
		let format = DateFormatter()
		format.dateFormat = "yyyy-MM-dd HH:mm:ss"
		let timestamp = format.string(from: Date())
		return timestamp
	}
}
