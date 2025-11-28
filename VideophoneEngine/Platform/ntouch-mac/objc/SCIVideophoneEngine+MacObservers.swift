//
//  SCIVideophoneEngine+MacObservers.swift
//  ntouch Mac
//
//  Created by Andrew Emrazian on 1/30/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

extension SCIVideophoneEngine {
	
	private static var _doNotDisturbEnabledWas: Bool = false
	private static var _uiDidSetDoNotDisturbEnabled = false
	@objc var uiDidSetDoNotDisturbEnabled: Bool {
		get { return SCIVideophoneEngine._uiDidSetDoNotDisturbEnabled }
		set(newValue) { SCIVideophoneEngine._uiDidSetDoNotDisturbEnabled = newValue }
	}
	
	@objc func addMacObservers() {
		SCIVideophoneEngine._doNotDisturbEnabledWas = self.doNotDisturbEnabled
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPropertyChanged(note:)), name: .SCINotificationPropertyChanged, object: self)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPropertyReceived(note:)), name: .SCINotificationPropertyReceived, object: self)
	}
	
	@objc func removeMacObservers() {
		NotificationCenter.default.removeObserver(self)
	}
	
	@objc func notifyPropertyChanged(note: Notification) {  // SCINotificationPropertyChanged
		guard let propertyName = note.userInfo?[SCINotificationKeyName] as? String else { return }
		if SCIPropertyNameDoNotDisturbEnabled == propertyName {
			if uiDidSetDoNotDisturbEnabled {
				SCIVideophoneEngine._doNotDisturbEnabledWas = doNotDisturbEnabled
			} else if !SCIVideophoneEngine._doNotDisturbEnabledWas && doNotDisturbEnabled {
				doNotDisturbEnabled = false
			}
			uiDidSetDoNotDisturbEnabled = false
			
		}
	}
	
	@objc func notifyPropertyReceived(note: Notification) {  // SCINotificationPropertyReceived
		guard let propertyName = note.userInfo?[SCINotificationKeyName] as? String else { return }
		if SCIPropertyNameDoNotDisturbEnabled == propertyName {
			guard let sciSettingItem = note.userInfo?["SCISettingItem"] as? SCISettingItem else { return }
			guard let dndEnabled = sciSettingItem.value as? Bool else { return }
			
			// Call dnd setter to make sure that rejectIncomingCalls also gets updated
			if !doNotDisturbEnabled || SCIVideophoneEngine._doNotDisturbEnabledWas {
				doNotDisturbEnabled = dndEnabled
			}
			SCIVideophoneEngine._doNotDisturbEnabledWas = doNotDisturbEnabled
		}
	}
}
