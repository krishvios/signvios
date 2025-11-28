//
//  NeedsUpdate.swift
//  ntouch
//
//  Created by Cody Nelson on 3/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
/**

A protocol that helps determine whether or not the objects properties have changed.

*/
protocol NeedsUpdate {
	/**
	
	A boolean property that returns whether or not the object's properties have changed.
	
	*/
	var needsUpdate : Bool { get }
}

extension NeedsUpdate {
	
	var needsUpdateMessage : String {
		return " ❌💾 Needs update "
	}
	var upToDateMessage: String {
		return " ✅ Up to date "
	}
}
