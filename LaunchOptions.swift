//
//  LaunchOptions.swift
//  ntouch
//
//  Created by Kevin Selman on 6/5/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

@objc (SCILaunchOptions)
class LaunchOptions : NSObject {
	
	@objc public class func processProperties(launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> NSDictionary? {
		guard let launchUrl = launchOptions?[UIApplication.LaunchOptionsKey.url]
		else {
			return nil
		}
		
		var coreOptions = Dictionary<String, Any>()
		if let components = URLComponents(url: launchUrl as! URL, resolvingAgainstBaseURL: false), components.host == "urlgen" {
			components.queryItems?.forEach {
				
				if $0.name == "ens" {
					UserDefaults.standard.set($0.value, forKey: "SCIDefaultENSServer")
				}
				else if $0.name == "deleteAll" {
					LaunchOptions.deletePersistentData()
				}
				else {
					coreOptions[$0.name] = $0.value
				}
			}
			
			// Turn off AutoSignIn and wizardCompleted
			LaunchOptions.clearLoginInfo()

			UserDefaults.standard.set(false, forKey: "wizardCompleted")
			UserDefaults.standard.synchronize()
		}
			
		return coreOptions as NSDictionary
	}
	
	@objc class func clearLoginInfo() {
		SCIDefaults.shared.isAutoSignIn = false
		SCIDefaults.shared.phoneNumber = ""
		SCIDefaults.shared.pin = ""
	}
	
	class func deletePersistentData() {
			// Delete Keychain Items
			let secItemClasses =  [kSecClassGenericPassword, kSecClassInternetPassword, kSecClassCertificate, kSecClassKey, kSecClassIdentity]
			for itemClass in secItemClasses {
				let spec: NSDictionary = [kSecClass: itemClass]
				SecItemDelete(spec)
			}
			
			// Delete UserDefaults
			if let bundleID = Bundle.main.bundleIdentifier {
				UserDefaults.standard.removePersistentDomain(forName: bundleID)
			}
			
			// Documents Folder
			let fileManager = FileManager.default
			let docFolder = fileManager.urls(for: .documentDirectory, in: .userDomainMask).first!
			do {
				try fileManager.removeItem(at: docFolder)
			}
			catch {
				debugPrint("Error deleting Documents directory")
			}
		}
	}
