//
//  CoreAccess.swift
//  ntouch
//
//  Created by Mikael Leppanen on 10/28/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

import Foundation
import XCTest

class CoreAccess {
	
	let cfg = UserConfig()
	
	/*
	Name: addRemoteProfilePhoto()
	Description: Add profile photo by phone number
	Parameters: phoneNumber
	Example: addRemoteProfilePhoto()
	*/
	func addRemoteProfilePhoto(phoneNumber: String) {
		let imgUrls = self.imageUploadURLGet(phoneNumber: phoneNumber)
		self.remoteProfilePhotoAdd(imgUrls["ImageId"]!, imgUrls["ImageUploadUrl"]!)
	}
	
	/*
	Name: imageUploadURLGet()
	Description: Get the ImageID and ImageUploadUrl by phone number
	Parameters: phoneNumber
	Example: imageUploadURLGet()
	*/
	func imageUploadURLGet(phoneNumber: String) -> [String: String] {
		let url = URL(string: "http://\(cfg["CoreAPIHost"]!)/core/image_upload_url_get/\(cfg["CoreSystem"]!)/\(phoneNumber)/")
		var urlDict = [String: String]()
		var urlData = NSArray()
		var seperateArray = [String]()
		var convertArray = ""
		do {
			let imageContent = try NSString(contentsOf: url!, encoding: String.Encoding.utf8.rawValue)
			let data = imageContent.data(using: String.Encoding.utf8.rawValue)
			let xmlParser = Foundation.XMLParser(data: data!)
			let xmlReader = XMLParser()
			xmlParser.delegate = xmlReader
			xmlParser.parse()
			urlData = xmlReader.allElements
		}
		catch {
			XCTFail("Unable to open the unencrypted call history list.")
			return urlDict
		}
		convertArray = String(describing: urlData)
		seperateArray = convertArray.components(separatedBy: "\n")
		for (i, str) in seperateArray.enumerated().reversed() {
			if str.count < 20 {
				seperateArray.remove(at: i)
			}
		}
		let stringOne = seperateArray[0]
		let stringTwo = seperateArray[1]
		var stringSplit = stringOne.components(separatedBy: "\"")
		let IdValue = stringSplit[1]
		urlDict["ImageId"] = IdValue
		stringSplit = stringTwo.components(separatedBy: "/")
		let imgValue = stringSplit[2]
		urlDict["ImageUploadUrl"] = imgValue
		return urlDict
	}
	
	/*
	Name: remoteProfilePhotoAdd()
	Description: Add profile photo to remote endpoint
	Parameters: pubId, uploadHost
	Example: remoteProfilePhotoAdd()
	*/
	func remoteProfilePhotoAdd(_ pubId: String, _ uploadHost: String) {
		// upload profile photo to the remote endpoint AND you don't get a choice of photo to upload
		let url = URL(string: "http://\(cfg["CoreAPIHost"]!)/core/profile_photo_upload/\(pubId)/\(uploadHost)/")
		_ = try? Data(contentsOf: url!)
	}
	
	/*
	Name: remoteProfilePhotoRemove()
	Description: Remove profile photo from remote endpoint
	Parameters: phoneNumber
	Example: remoteProfilePhotoRemove()
	*/
	func remoteProfilePhotoRemove(phoneNumber: String) {
		// remove a profile photo from the remote endpoint
		let url = URL(string: "http://\(cfg["CoreAPIHost"]!)/core/profile_photo_remove/\(cfg["CoreSystem"]!)/\(cfg["Phone"]!)/")
		_ = try? Data(contentsOf: url!)
	}
	
	/*
	Name: sendAgreements()
	Description: Send Agreements
	Parameters: none
	Example: sendAgreements()
	*/
	func sendAgreements() {
		let url = URL(string: "http://" + cfg["CoreAPIHost"]! + "/core/send_agreements/" + cfg["CoreSystem"]! + "/" + cfg["Phone"]! + "/")
		var coreResp: String?
		let resp = try? Data(contentsOf: url!)
		if resp != nil {
			coreResp = "Ok"
		} else {
			coreResp = "Send Agreements Error"
		}
		print("Core Response: " + coreResp!)
	}
	
	/*
	Name: sendDismissibleCIR()
	Description: Send Dismissible CIR
	Parameters: none
	Example: sendDismissibleCIR()
	*/
	func sendDismissibleCIR() {
		let url = URL(string: "http://" + cfg["CoreAPIHost"]! + "/core/send_dismissiblecir/" + cfg["CoreSystem"]! + "/" + cfg["Phone"]! + "/")
		var coreResp: String?
		let resp = try? Data(contentsOf: url!)
		if resp != nil {
			coreResp = "Ok"
		} else {
			coreResp = "Send Dismissible CIR Error"
		}
		print("Core Response: " + coreResp!)
	}
	
	/*
	Name: sendTrsRegistration()
	Description: Send TRS Registration
	Parameters: none
	Example: sendTrsRegistration()
	*/
	func sendTrsRegistration() {
		let url = URL(string: "http://" + cfg["CoreAPIHost"]! + "/core/send_trsRegistration/" + cfg["CoreSystem"]! + "/" + cfg["Phone"]! + "/")
		var coreResp: String?
		let resp = try? Data(contentsOf: url!)
		if resp != nil {
			coreResp = "Ok"
		} else {
			coreResp = "Send TRS Registration Error"
		}
		print("Core Response: " + coreResp!)
	}
	
	/*
	Name: setAutoSpeed(value:)
	Description: Enable or Disable Auto Speed in core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setAutoSpeed(value: "0")
	*/
	func setAutoSpeed(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"AutoSpeedEnabled", settingValue: value)
	}
	
	/*
	Name: setBlockAnonymousCalls(value:)
	Description: Enable or Disable Block Anonymous Calls in core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setBlockAnonymousCalls(value: "0")
	*/
	func setBlockAnonymousCalls(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"BlockAnonymousCalls", settingValue: value)
	}
	
	/*
	Name: setContactPhotos(value:)
	Description: Enable or Disable Contact Photos in core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setContactPhotos(value: "0")
	*/
	func setContactPhotos(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"ContactImagesEnabled", settingValue: value)
	}
	
	/*
	Name: setDoNotDisturb(value:)
	Description: Enable or Disable Do Not Disturb in core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setDoNotDisturb(value: "0")
	*/
	func setDoNotDisturb(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"cmCallAutoReject", settingValue: value)
	}
	
	/*
	Name: setEnhancedSignMail(value:)
	Description: Enable or Disable Enhanced SignMail in core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setEnhancedSignMail(value: "0")
	*/
	func setEnhancedSignMail(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"DirectSignMailEnabled", settingValue: value)
	}
	
//	/*
//	Name: setFavorites(value:)
//	Description: Enable or Disable Favorites in core by the user phonenumber
//	Parameters: value ("0" or "1")
//	Example: setFavorites(value: "0")
//	*/
//	func setFavorites(value: String) {
//		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"ContactFavoriteEnabled", settingValue: value)
//	}
	
	/*
	* Name: setGreetingType
	* Description: Set Media Encryption
	* Parameters:  value (0 - No Greeting, 1 - Sorenson, 2 - Personal 3 - Personal with Text, 4 - Text Only)
	* Example setGreetingType(phoneNumber: "17323005678", value: "1");
	*/
	func setGreetingType(phoneNumber: String, value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: phoneNumber, userSetting:"GreetingPreference", settingValue: value)
	}
	
	/*
	Name: setHideMyCallerID(value:)
	Description: Enable or Disable Hide My Caller ID
	Parameters: value ("0" or "1")
	Example: setHideMyCallerID(value: "0")
	*/
	func setHideMyCallerID(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"BlockCallerId", settingValue: value)
	}
	
	/*
	Name: setHideMyCallerIDFeature(value:)
	Description: Enable or Disable Hide My Caller ID feature in core
	Parameters: value ("0" or "1")
	Example: setHideMyCallerIDFeature(value: "0")
	*/
	func setHideMyCallerIDFeature(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"BlockCallerIdEnabled", settingValue: value)
	}
	
	/*
	* Name: setMediaEncryption
	* Description: Set Media Encryption
	* Parameters:  value (0 - Not Preferred, 1 - Preferred, 2 - Required)
	* Example setMediaEncryption(value: "1");
	*/
	func setMediaEncryption(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"SecureCallMode", settingValue: value)
	}
	
	/*
	Name: setGroupVideo(value:)
	Description: Enable or Disable Group Video in core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setGroupVideo(value: "0")
	*/
	func setGroupVideo(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"GroupVideoChatEnabled", settingValue: value)
	}
	
	/*
	Name: setProfilePhotoPrivacy(value:)
	Description: Set Profile Photo Privacy
	Parameters: phoneNumber, value (0 - Everyone, 1 - Contacts)
	Example: setProfilePhotoPrivacy(phoneNumber: "17323005678", value: "0")
	*/
	func setProfilePhotoPrivacy(phoneNumber: String, value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: phoneNumber, userSetting:"ProfileImagePrivacyLevel", settingValue: value)
	}
	
	/*
	Name: setShareText(value:)
	Description: Enable or Disable Share Text in core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setShareText(value: "0")
	*/
	func setShareText(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"RealTimeTextEnabled", settingValue: value)
	}
	
	/*
	Name: setSpanishFeatures(value:)
	Description: Enable or Disable Spanish Features core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setSpanishFeatures(value: "0")
	*/
	func setSpanishFeatures(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"SpanishFeatures", settingValue: value)
	}
	
	/*
	Name: setVideoCenter(value:)
	Description: Enable or Disable Video Center (SignMail) in core by the user phonenumber
	Parameters: value ("0" or "1")
	Example: setVideoCenter(value: "0")
	*/
	func setVideoCenter(value: String) {
		self.userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"VideoMessageEnabled", settingValue: value)
	}
	
	/*
	Name: userSet(host:coreSystem:phoneNumber:userSetting:settingValue:)
	Description: Set a user setting in core by the user phonenumber
	Parameters: host
				coreSystem
				phoneNumber
				userSetting
				settingValue
	Example: userSet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!, userSetting:"ContactFavoriteEnabled", settingValue: favVal)
	*/
	func userSet(host: String, coreSystem: String, phoneNumber: String, userSetting: String, settingValue: String) {
		let url = URL(string: "http://" + host + "/core/user_set/" + coreSystem + "/" + phoneNumber  + "/" + userSetting + "/" + settingValue )
		do {
			let data = try? Data(contentsOf: url!)
			let res: Any? = try JSONSerialization.jsonObject(with: data! , options: [])
			if let daytah = res as? NSDictionary {
				let coreResp = daytah["data"] as! String?
				print(coreResp!)
			}
		} catch let error as NSError {
			print("error: \(error)")
		}
	}
	
	/*
	Name: userSettingsGet(host:coreSystem:phoneNumber:)
	Description: Set a user setting in core by the user phonenumber
	Parameters: host
				coreSystem
				phoneNumber
	Example: userSettingsGet(host: cfg["CoreAPIHost"]!, coreSystem: cfg["CoreSystem"]!, phoneNumber: cfg["Phone"]!)
	*/
	func userSettingsGet(host: String, coreSystem: String, phoneNumber: String) ->(Dictionary<String,String>) {
		let url = URL(string: "http://" + host + "/core/user_settings/" + coreSystem + "/" + phoneNumber + "/all")
		var coreResp: Dictionary<String,String>?
		do {
			let data = try? Data(contentsOf: url!)
			let res: Any? = try JSONSerialization.jsonObject(with: data! , options: [])
			if let daytah = res as? NSDictionary {
				coreResp = daytah["data"] as? Dictionary<String,String>
			}
		} catch let error as NSError {
			print("error: \(error)")
		}
		return coreResp!
	}
	
}
