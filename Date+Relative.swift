//
//  Date+Relative.swift
//  ntouch
//
//  Created by Kevin Selman on 10/16/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

extension Date {

	/*
	Creates a formatted string from the current instance with
	a similar format as in Apple's Phone and Mail apps.
	
		3:45 PM
		Yesterday
		Tuesday
		Monday
		8/23/2020
	
	- Returns: A new Date formatted string.
	*/
	func formatRelativeString() -> String {
		let dateFormatter = DateFormatter()
		let calendar = Calendar(identifier: .gregorian)
		dateFormatter.doesRelativeDateFormatting = true
		
		if calendar.isDateInToday(self) {
			dateFormatter.timeStyle = .short
			dateFormatter.dateStyle = .none
		} else if calendar.isDateInYesterday(self){
			dateFormatter.timeStyle = .none
			dateFormatter.dateStyle = .medium
		} else if calendar.compare(Date(), to: self, toGranularity: .weekOfYear) == .orderedSame {
			let weekday = calendar.dateComponents([.weekday], from: self).weekday ?? 0
			return dateFormatter.weekdaySymbols[weekday-1]
		} else {
			dateFormatter.timeStyle = .none
			dateFormatter.dateStyle = .short
		}
		
		return dateFormatter.string(from: self)
	}
}

