//
//  EditSpanishVRSDetailsViewModelItem.swift
//  ntouch
//
//  Created by Cody Nelson on 4/3/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
class EditRelayLanguageDetailsViewModelItem : NSObject, DetailsViewModelItem {
	var type: DetailsViewModelItemType { return .relayLanguage }
	var title : String
	let relayLanguage : String
	var editRelayLanguage : String
	
	init( title: String, relayLanguage : String ) {
		self.title = title
		self.relayLanguage = relayLanguage
		self.editRelayLanguage = relayLanguage
	}
}

extension EditRelayLanguageDetailsViewModelItem : Valid, NeedsUpdate {
	
	var isValid: Bool {
		return true
	}
	
	var needsUpdate: Bool {
		// 1. Edit relay language must not be the same as the current relay language
		guard editRelayLanguage != relayLanguage else { return false }
		return true
	}
}

extension EditRelayLanguageDetailsViewModelItem : CanShowView {
	var canShow: Bool {
		return (SCIVideophoneEngine.shared.spanishFeatures && SCIVideophoneEngine.shared.interfaceMode == .standard)
	}
}
