//
//  RelayLanguage.swift
//  ntouch
//
//  Created by Izzy Mansurov on 12/6/22.
//  Copyright © 2022 Sorenson Communications. All rights reserved.
//

import Foundation
enum RelayLanguage: String, CaseIterable {
	case english = "English"
	case spanish = "Spanish"
}

let RelaySpanish = RelayLanguage.spanish.rawValue
let RelayEnglish = RelayLanguage.english.rawValue
