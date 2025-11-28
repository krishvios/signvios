//
//  HistoryDetailsRepo.swift
//  ntouch
//
//  Created by Cody Nelson on 2/20/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// provides the history details for a given call list item, phone number, or contact.
//class HistoryDetailsRepo : NSObject {
//
//	internal static var _historySummaryCount : Int = 8
//	fileprivate static let localeIdentifier = "en_US"
//	fileprivate static var dateFormatter : DateFormatter {
//		let formatter = DateFormatter()
//		formatter.dateStyle = .long
//		formatter.timeStyle = .none
//		formatter.locale = Locale(identifier: HistoryDetailsRepo.localeIdentifier)
//		return formatter
//	}
//
//	var callListManager: SCICallListManager
//	var number : String
//
//	 var dateDict : [Date : [CallHistoryDetail]] = [:] {
//		didSet{
//			debugPrint(#function, "didset")
//		}
//	}
//
//	var items = [CallHistoryDetail]() {
//		didSet{
//			debugPrint(#function, "updated history items for \(number)")
//
//		}
//	}
//
//	init(callListManager: SCICallListManager,
//				  number: String){
//
//		self.callListManager = callListManager
//		self.number = number
//		super.init()
//		self.loadItems()
//	}
//
//	func loadItems() {
//
//		let itemList = callListManager.items(forNumber: number)
//		guard
//			!itemList.isEmptyOrNil
//			else { return }
//
//		let itemsToMap = itemList as! [SCICallListItem]
//		self.items = itemsToMap.map { (callListItem) -> CallHistoryDetail in
//			let historyDetail = CallHistoryDetail(callListItem: callListItem)
//			let timelessDate = Date.getTimelessDate(originalDate: callListItem.date)
////			dateDict[timelessDate]?.append(historyDetail)
//			return historyDetail
//		}
//		self.dateDict = Dictionary(grouping: self.items, by: {$0.date})
//
//	}
//
//	var sortedHistory : [CallHistoryDetail] {
//		return items.sorted(by: {$0.date > $1.date} )
//	}
//
//	var callHistoryDateDetails : [CallHistoryDateDetail] {
//		var dates : [CallHistoryDateDetail] = []
//		let items = self.dateDict.reduce(into: [CallHistoryDateDetail]()) { (result, keyValueTuple) in
//
//			let (key, value) = keyValueTuple
//			guard
//				let result.filter{$0.first
//			for keyValueTuple.key
//
//
//		}
//
//		}
//		for (key, value) in self.dateDict {
//			dates.append(CallHistoryDateDetail(date: key, history: value))
//		}
//		return dates.sorted(by: { (dateDetail1, dateDetail2) -> Bool in
//			return dateDetail1.date > dateDetail2.date
//		})
//	}
//
//}
//
//extension String {
//
//	static let shortDateUS : DateFormatter = {
//		let formatter = DateFormatter()
//		formatter.calendar = Calendar(identifier: .iso8601)
//		formatter.locale = Locale(identifier: "en_US_POSIX")
//		formatter.dateStyle = .short
//		return formatter
//	}()
//	var shortDateUS: Date? {
//		return String.shortDateUS.date(from:self)
//	}
//}
////extension Date {
////
////	var timelessDate : Date {
////		let calendar = Calendar.current
////		let components = calendar.dateComponents([Calendar.Component.day, Calendar.Component.month, Calendar.Component.year],  from: self)
////		debugPrint(#function, "\(components)")
////		let newDate = calendar.date(from:components)
////		return newDate!
////	}
////
////	func makeTimeless( date : Date ) -> Date {
////
////		let calendar = Calendar.current
////		let dateComponents = DateComponents()
////		dateComponents.setValue(<#T##value: Int?##Int?#>, for: <#T##Calendar.Component#>)
////		let date = Calendar.current.date(bySettingHour: 0, minute: 0, second: 0, of: date)!
////		return date
////	}
////
////}
////	static func getTimelessDate(from originalDate : Date ) -> Date {
////
////		var components = Calendar.current.dateComponents([.year, .month, .day], from : originalDate)
//////		var components = DateComponents(calendar: Calendar.current, year: originalDate.date=)
////
////
////
////		return Calendar.current.date(from: components)!
////	}
////}
//extension Date {
//
//	static func getTimelessDate( originalDate : Date ) -> Date {
//
//		let cal = Calendar.current
//		let components = cal.dateComponents([.year, .month, .day ], from: originalDate )
//		let newDate = cal.date(from: components)
//		return newDate!
//
//
//	}
//}
