//
//  UIDevice+Machine.swift
//  ntouch
//
//  Created by Daniel Shields on 1/3/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

/// Represents a parsed hardware version (i.e. "iPhone10,1")
@objc(SCIDeviceVersion)
class DeviceVersion: NSObject {

	@objc let machine: String
	@objc let product: String
	@objc let majorVersion: Int
	@objc let minorVersion: Int
	@objc var isSimulator: Bool {
		// In this case the major and minor versions will be 0.
		return machine == "x86_64" || machine == "i386"
	}

	@objc init(utsname: utsname) {
		// Convert the variable "machine" from a tuple of Int8 representing the fixed-length array, into a String.
		let machineMirror = Mirror(reflecting: utsname.machine)
		var machine = ""
		machine.reserveCapacity(Int(machineMirror.children.count))
		for child in machineMirror.children {
			guard let char = child.value as? Int8, char != 0 else { break }
			machine.append(Character.init(Unicode.Scalar(UInt8(char))))
		}
		self.machine = machine

		let firstNumerical = machine.firstIndex(where: { $0 >= "0" && $0 <= "9" })
		let firstComma = machine.suffix(from: firstNumerical ?? machine.endIndex).firstIndex(where: { $0 == "," })

		let productRange = machine.startIndex ..< (firstNumerical ?? machine.endIndex)
		product = String(machine[productRange])

		let majorVersionRange = (firstNumerical ?? machine.endIndex) ..< (firstComma ?? machine.endIndex)
		majorVersion = Int(machine[majorVersionRange]) ?? 0

		if let firstComma = firstComma {
			let minorVersionRange = machine.index(after: firstComma) ..< machine.endIndex
			minorVersion = Int(machine[minorVersionRange]) ?? 0
		}
		else {
			minorVersion = 0
		}

		super.init()
	}

	// Old obj-c functions
	@objc var isVerizonPhone: Bool {
		return machine == "iPhone3,3"
	}

	@objc var isiPhone4: Bool {
		return product == "iPhone" && majorVersion == 3
	}

	@objc var isiPhone4S: Bool {
		return machine == "iPhone4,1"
	}

	@objc var isiPod5: Bool {
		return machine == "iPod5,1"
	}

	@objc var isiPod: Bool {
		return product == "iPod"
	}

	@objc var isiPad: Bool {
		return product == "iPad"
	}

	@objc var isiPhone: Bool {
		return product == "iPhone";
	}

	@objc var isiPhone5: Bool {
		return product == "iPhone" && majorVersion == 5
	}

	@objc var isiPhone5S: Bool {
		return product == "iPhone" && majorVersion == 6
	}

	@objc var isiPhone6: Bool {
		return product == "iPhone" && majorVersion == 7
	}

	@objc var isiPhone720p: Bool {
		return product == "iPhone" && majorVersion >= 7;
	}

	@objc var isiPad720p: Bool {
		return product == "iPad" && (majorVersion > 3 || (majorVersion == 3 && minorVersion >= 4))
	}
	
	func deviceProductName() -> String {
		
		//iPhone
		if machine == "iPhone1,1"        { return "iPhone (1st generation)" }
		else if machine == "iPhone1,2"   { return "iPhone 3G" }
		else if machine == "iPhone2,1"   { return "iPhone 3GS" }
		else if machine == "iPhone3,1"   { return "iPhone 4 (GSM)" }
		else if machine == "iPhone3,2"   { return "iPhone 4 (GSM, 2nd revision)" }
		else if machine == "iPhone3,3"   { return "iPhone 4 (CDMA)" }
		else if machine == "iPhone4,1"   { return "iPhone 4S" }
		else if machine == "iPhone5,1"   { return "iPhone 5 (GSM)" }
		else if machine == "iPhone5,2"   { return "iPhone 5 (GSM+CDMA)" }
		else if machine == "iPhone5,3"   { return "iPhone 5c (GSM)" }
		else if machine == "iPhone5,4"   { return "iPhone 5c (GSM+CDMA)" }
		else if machine == "iPhone6,1"   { return "iPhone 5s (GSM)" }
		else if machine == "iPhone6,2"   { return "iPhone 5s (GSM+CDMA)" }
		else if machine == "iPhone7,2"   { return "iPhone 6" }
		else if machine == "iPhone7,1"   { return "iPhone 6 Plus" }
		else if machine == "iPhone8,1"   { return "iPhone 6s" }
		else if machine == "iPhone8,2"   { return "iPhone 6s Plus" }
		else if machine == "iPhone8,4"   { return "iPhone SE" }
		else if machine == "iPhone9,1"   { return "iPhone 7 (GSM+CDMA)" }
		else if machine == "iPhone9,3"   { return "iPhone 7 (GSM)" }
		else if machine == "iPhone9,2"   { return "iPhone 7 Plus (GSM+CDMA)" }
		else if machine == "iPhone9,4"   { return "iPhone 7 Plus (GSM)" }
		else if machine == "iPhone10,1"   { return "iPhone 8 (GSM+CDMA)" }
		else if machine == "iPhone10,4"   { return "iPhone 8 (GSM)" }
		else if machine == "iPhone10,2"   { return "iPhone 8 Plus (GSM+CDMA)" }
		else if machine == "iPhone10,5"   { return "iPhone 8 Plus (GSM)" }
		else if machine == "iPhone10,3"   { return "iPhone X (GSM+CDMA)" }
		else if machine == "iPhone10,6"   { return "iPhone X (GSM)" }
			
		//iPod Touch
		else if machine == "iPod1,1"     { return "iPod Touch 1G" }
		else if machine == "iPod2,1"     { return "iPod Touch 2G" }
		else if machine == "iPod3,1"     { return "iPod Touch 3G" }
		else if machine == "iPod4,1"     { return "iPod Touch 4G" }
		else if machine == "iPod5,1"     { return "iPod Touch 5G" }
		else if machine == "iPod7,1"     { return "iPod Touch 6G" }
			
		//iPad
		else if machine == "iPad1,1"     { return "iPad (1st generation)" }
		else if machine == "iPad2,1"     { return "iPad 2 (Wi-Fi)" }
		else if machine == "iPad2,2"     { return "iPad 2 (GSM)" }
		else if machine == "iPad2,3"     { return "iPad 2 (CDMA)" }
		else if machine == "iPad2,4"     { return "iPad 2 (Wi-Fi, Mid 2012)" }
		else if machine == "iPad3,1"     { return "iPad (3rd generation) (Wi-Fi)" }
		else if machine == "iPad3,2"     { return "iPad (3rd generation) (GSM+CDMA)" }
		else if machine == "iPad3,3"     { return "iPad (3rd generation) (GSM)" }
		else if machine == "iPad3,4"     { return "iPad (4th generation) (Wi-Fi)"}
		else if machine == "iPad3,5"     { return "iPad (4th generation) (GSM)" }
		else if machine == "iPad3,6"     { return "iPad (4th generation) (GSM+CDMA)" }
		else if machine == "iPad6,11"    { return "iPad (5th generation) (Wi-Fi)" }
		else if machine == "iPad6,12"    { return "iPad (5th generation) (Cellular)" }
			
		//iPad Mini
		else if machine == "iPad2,5"     { return "iPad Mini (Wi-Fi)" }
		else if machine == "iPad2,6"     { return "iPad Mini (GSM)" }
		else if machine == "iPad2,7"     { return "iPad Mini (GSM+CDMA)" }
		else if machine == "iPad4,4"     { return "iPad Mini 2 (Wi-Fi)" }
		else if machine == "iPad4,5"     { return "iPad Mini 2 (Cellular)" }
		else if machine == "iPad4,6"     { return "iPad Mini 2 (China)" }
		else if machine == "iPad4,7"     { return "iPad Mini 3 (Wi-Fi)" }
		else if machine == "iPad4,8"     { return "iPad Mini 3 (Cellular)" }
		else if machine == "iPad4,9"     { return "iPad Mini 3 (China)" }
		else if machine == "iPad5,1"     { return "iPad Mini 4 (Wi-Fi)" }
		else if machine == "iPad5,2"     { return "iPad Mini 4 (Cellular)" }
			
		//iPad Air
		else if machine == "iPad4,1"     { return "iPad Air (Wi-Fi)" }
		else if machine == "iPad4,2"     { return "iPad Air (Cellular)" }
		else if machine == "iPad4,3"     { return "iPad Air (China)" }
		else if machine == "iPad5,3"     { return "iPad Air 2 (Wi-Fi)" }
		else if machine == "iPad5,4"     { return "iPad Air 2 (Cellular)" }
			
		//iPad Pro
		else if machine == "iPad6,3"     { return "iPad Pro 9.7\" (Wi-Fi)" }
		else if machine == "iPad6,4"     { return "iPad Pro 9.7\" (Cellular)" }
		else if machine == "iPad6,7"     { return "iPad Pro 12.9\" (Wi-Fi)" }
		else if machine == "iPad6,8"     { return "iPad Pro 12.9\" (Cellular)" }
		else if machine == "iPad7,1"     { return "iPad Pro 12.9\" (2nd generation) (Wi-Fi)" }
		else if machine == "iPad7,2"     { return "iPad Pro 12.9\" (2nd generation) (Cellular)" }
		else if machine == "iPad7,3"     { return "iPad Pro 10.5\" (Wi-Fi)" }
		else if machine == "iPad7,4"     { return "iPad Pro 10.5\" (Cellular)" }
			
		//Apple TV
		else if machine == "AppleTV2,1"  { return "Apple TV 2G" }
		else if machine == "AppleTV3,1"  { return "Apple TV 3" }
		else if machine == "AppleTV3,2"  { return "Apple TV 3 (2013)" }
		else if machine == "AppleTV5,3"  { return "Apple TV 4" }
		else if machine == "AppleTV6,2"  { return "Apple TV 4K" }
			
		//Apple Watch
		else if machine == "Watch1,1"    { return "Apple Watch (1st generation) (38mm)" }
		else if machine == "Watch1,2"    { return "Apple Watch (1st generation) (42mm)" }
		else if machine == "Watch2,6"    { return "Apple Watch Series 1 (38mm)" }
		else if machine == "Watch2,7"    { return "Apple Watch Series 1 (42mm)" }
		else if machine == "Watch2,3"    { return "Apple Watch Series 2 (38mm)" }
		else if machine == "Watch2,4"    { return "Apple Watch Series 2 (42mm)" }
		else if machine == "Watch3,1"    { return "Apple Watch Series 3 (38mm Cellular)" }
		else if machine == "Watch3,2"    { return "Apple Watch Series 3 (42mm Cellular)" }
		else if machine == "Watch3,3"    { return "Apple Watch Series 3 (38mm)" }
		else if machine == "Watch3,4"    { return "Apple Watch Series 3 (42mm)" }
			
		//Simulator
		else if machine == "i386"        { return "Simulator" }
		else if machine == "x86_64"      { return "Simulator"}
		
		return machine
	}
}

extension UIDevice {
	@objc var version: DeviceVersion {
		var systemInfo = utsname()
		uname(&systemInfo)
		return DeviceVersion(utsname: systemInfo)
	}
}
