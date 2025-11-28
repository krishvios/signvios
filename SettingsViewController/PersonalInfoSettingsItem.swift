//
//  PersonalInfoSettingsItem.swift  ntouch
//
//  Created by Cody Nelson on 3/22/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation


enum PersonalInfoSettingsItemType {
	case nameAndPhoto
	case numbers
	case accountInfo
}
protocol PersonalInfoSettingsItem: HasRowCount {
	var type : PersonalInfoSettingsItemType { get }
}

struct NameAndPhotoPersonalInfoSettingsItem : PersonalInfoSettingsItem {
	var type: PersonalInfoSettingsItemType { return .nameAndPhoto }
	var image : UIImage
	var name : String
}

struct NumberPersonalInfoSettingsItem : PersonalInfoSettingsItem {
	var type: PersonalInfoSettingsItemType { return .numbers }
	var image : UIImage
	var title : String
	var number: String
	
}

struct AccountInfoPersonalInfoSettingsItem : PersonalInfoSettingsItem {
	var type: PersonalInfoSettingsItemType { return .accountInfo }
	var title : String
	var detail : String
}

