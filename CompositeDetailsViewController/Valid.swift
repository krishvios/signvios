//
//  Valid.swift
//  ntouch
//
//  Created by Cody Nelson on 3/13/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/**

A protocol that helps determine whether or not the objects properties are valid.

*/
protocol Valid {
	
	/**

	A boolean property that returns whether or not the object's properties are valid enough to be saved.
	
	*/
	var isValid: Bool { get }
	
}

extension Valid {

	var validMessage : String {
		return " ✅ Valid "
	}
	var invalidMessage: String {
		return " ❌ Invalid "
	}
}




