//
//  DestructiveOptionsDetail.swift
//  ntouch
//
//  Created by Cody Nelson on 3/27/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
/**
	Options that the allow the user to interact with calls and contacts in negative ways.
*/
protocol DestructiveOptionDetail {
	
	var type : DestructiveOptionDetailType { get }
	
	var title : String {get set}
	
	func action(sender: Any?)
}
extension DestructiveOptionDetail {
	func action(sender: Any?){ fatalError("This method is not implemented for this object.") }
}
