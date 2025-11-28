//
//  SettingsConstants.swift
//  ntouch
//
//  Created by Cody Nelson on 1/10/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
let kMainStoryboardName = "Main-Universal"
let kNewSettingsStoryboardName = "SettingsNew"

// Images
let kSettingsNavImageKey = "navbar_settings.png"

// MARK: - Settings Titles
let kPersonalInfoTitle = "Personal Info"
let kDeviceOptionsTitle = "Device Options"
let kCallOptionsTitle = "Call Options"
let kSaveTextTitle = "Save Text"
let kControlsTitle = "Controls"
let kSupportTitle = "Support"
let kNetworkAdminTitle = "Network / Admin"
let kAboutTitle = "About"

let kDoNotDisturbTitle = "Do Not Disturb"

// MARK: - Settings Icon Images
let kPersonalInfoIcon = UIImage(named: "Envelope")
let kDeviceOptionsIcon = UIImage(named: "Envelope")
let kCallOptionsIcon = UIImage(named: "Envelope")
let kSaveTextIcon = UIImage(named: "Envelope")
let kControlsIcon = UIImage(named: "Envelope")
let kSupportIcon = UIImage(named: "Envelope")
let kNetworkAdminIcon = UIImage(named: "Envelope")
let kAboutIcon = UIImage(named: "Envelope")

let kDoNotDisturbIcon = UIImage(named: "Envelope")
let kDefaultPersonalInfoButtonIcon = UIImage(named: "SorensonUser")

// MARK: - Settings Segues
let kSettingsToPersonalInfoSegueIdentifier = "SettingsToPersonalInfoSegue"
let kSettingsToDeviceOptionsSegueIdentifier = "SettingsToDeviceOptionsSegue"
let kSettingsToCallOptionsSegueIdentifier = "SettingsToCallOptionsSegue"
let kSettingsToControlsSegueIdentifier = "SettingsToControlsSegue"
let kSettingsToNetworkAdminSegueIdentifier = "SettingsToNetworkAdminSegue"
let kSettingsToSupportSegueIdentifier = "SettingsToSupportSegue"
let kSettingsToAboutSegueIdentifier = "SettingsToAboutSegue"

// MARK: - Settings Cell Identifiers
let kPersonalInfoContactCellIdentifier = "PersonalInfoContactCell"
let kPersonalInfoContactCellName = "PersonalInfoContactCell"
let kPersonalInfoButtonTableViewCellIdentifier = "PersonalInfoButtonTableViewCell"
let kPersonalInfoButtonTableViewCellName = "PersonalInfoButtonTableViewCell"

let kDefaultCellIdentifier : String = "Cell"

let kPersonalInfoButtonMenuItemHeight : CGFloat = 150

// MARK: - Settings List Items
let kSettingsViewControllerItems : [MenuItem] = [
	PersonalInfoButtonMenuItem(title: kPersonalInfoTitle, image: kPersonalInfoIcon),
	ToggleMenuItem(title: kDoNotDisturbTitle, image: kDoNotDisturbIcon, toggleSwitch: UISwitch() ),
	SegueToControllerMenuItem(title: kDeviceOptionsTitle, image: kDeviceOptionsIcon, segueIdentifier: kSettingsToDeviceOptionsSegueIdentifier),
	SegueToControllerMenuItem(title: kCallOptionsTitle, image: kCallOptionsIcon, segueIdentifier: kSettingsToCallOptionsSegueIdentifier),
	SegueToControllerMenuItem(title: kControlsTitle, image: kControlsIcon, segueIdentifier: kSettingsToControlsSegueIdentifier),
	SegueToControllerMenuItem(title: kNetworkAdminTitle , image: kNetworkAdminIcon, segueIdentifier: kSettingsToCallOptionsSegueIdentifier),
	SegueToControllerMenuItem(title: kSupportTitle, image: kSupportIcon, segueIdentifier: kSettingsToSupportSegueIdentifier),
	SegueToControllerMenuItem(title: kAboutTitle, image: kAboutIcon, segueIdentifier: kSettingsToAboutSegueIdentifier)
]
