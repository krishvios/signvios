import Foundation

extension UserDefaults {
	
	private enum Key: String {
		case usingModalKeypad
		case selectedThemeIdentifier
		case contactOrderingIsFirstLast
		case phonebookTabSelection
		case shownInternetSearchAllowed
		case videoPrivacyEnabled
	}
	
	var usingModalKeypad: Bool {
		get { return bool(forKey: Key.usingModalKeypad.rawValue) }
		set { set(newValue, forKey: Key.usingModalKeypad.rawValue) }
	}
	
	@objc var selectedThemeIdentifier: String {
		// returns 0 if no key is present (which is the default theme)
		get { return string(forKey: Key.selectedThemeIdentifier.rawValue) ?? Theme.themes.first!.identifier }
		set { set(newValue, forKey: Key.selectedThemeIdentifier.rawValue) }
	}
	
	var contactOrderingFirstLast: Bool {
		get { return bool(forKey: Key.contactOrderingIsFirstLast.rawValue) }
		set { set(newValue, forKey: Key.contactOrderingIsFirstLast.rawValue) }
	}
	
	var phonebookTabSelection: Int? {
		get { return object(forKey: Key.phonebookTabSelection.rawValue) as? Int }
		set {
			if let newValue = newValue {
				set(newValue, forKey: Key.phonebookTabSelection.rawValue)
			} else {
				removeObject(forKey: Key.phonebookTabSelection.rawValue)
			}
			synchronize()
		}
	}
	
	var shownInternetSearchAllowed: Bool {
		get { return bool(forKey: Key.shownInternetSearchAllowed.rawValue) }
		set { set(newValue, forKey: Key.shownInternetSearchAllowed.rawValue) }
	}
}

