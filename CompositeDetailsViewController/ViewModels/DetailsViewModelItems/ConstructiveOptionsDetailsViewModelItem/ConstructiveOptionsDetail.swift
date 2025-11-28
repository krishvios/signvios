//
//  ConstructiveOptionsDetail.swift
//  ntouch
//
//  Created by Cody Nelson on 3/20/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

protocol ConstructiveOptionsDetail : HasRowCount {
	var type: ConstructiveOptionsDetailType {get}
	var title: String { get }
	func action(sender : Any?)
}

extension ConstructiveOptionsDetail {
	func action(sender: Any?) {
		fatalError( " The object has not implemeented this method.")
	}
}
