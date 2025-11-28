//
//  GetNumberDetailsForContact.swift
//  ntouch
//
//  Created by Cody Nelson on 4/3/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
protocol GetNumberDetailsForContact {}

struct GetContactEditNumberDetails: GetNumberDetailsForContact {
	var contact : SCIContact?
	
	func execute() -> EditNumbersListDetailsViewModelItem {
		let numbersList = EditNumbersListDetailsViewModelItem(editNumbers: [])
		if let homePhone = contact?.homePhone,
			!homePhone.isBlank {
			let homeDetail = EditHomeNumberDetail(number: homePhone)
			if contact?.isResolved ?? false
			{
				homeDetail.isFavorite = contact?.isFavorite(forPhoneType: .home) ?? false
			}
			numbersList.numbers.append(homeDetail)
		} else {
			numbersList.numbers.append(EditHomeNumberDetail(number:""))
		}
		if let workPhone = contact?.workPhone,
			!workPhone.isBlank {
			let workDetail = EditWorkNumberDetail(number: workPhone)
			if contact?.isResolved ?? false
			{
				workDetail.isFavorite = contact?.isFavorite(forPhoneType: .work) ?? false
			}
			numbersList.numbers.append(workDetail)
		} else {
			numbersList.numbers.append(EditWorkNumberDetail(number: ""))
		}
		if let cellPhone = contact?.cellPhone,
			!cellPhone.isBlank {
			let cellDetail = EditMobileNumberDetail(number: cellPhone)
			if contact?.isResolved ?? false
			{
				cellDetail.isFavorite = contact?.isFavorite(forPhoneType: .cell) ?? false
			}
			numbersList.numbers.append(cellDetail)
		} else {
			numbersList.numbers.append(EditMobileNumberDetail(number: ""))
		}
		return numbersList
	}
}
