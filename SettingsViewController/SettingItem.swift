//
//  SettingItem.swift
//  ntouch
//
//  Created by Kevin Selman on 4/8/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

@objcMembers
class SettingItem : NSObject {
	
	var rowName:String
	var searchWords:String
	
	@objc public init(rowName: String, searchWords: String) {
		self.rowName = rowName
		self.searchWords = searchWords
		super.init()
	}
}

