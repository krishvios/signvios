//
//  BuildVersion.swift
//  ntouch
//
//  Created by Isaac Roach on 1/6/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import Foundation

@objc class BuildVersion: NSObject {
	
	var major = 0
	var minor = 0
	var revision = 0
	var buildNumber = 0
	var versionString: String
	
	//convenience override
	init(string versionString: String) {
		self.major = -1
		self.minor = -1
		self.revision = -1
		self.buildNumber = -1
		self.versionString = versionString
		let versionNumbers: [String] = versionString.components(separatedBy: ".")
		if 4 == versionNumbers.count {
			self.major = Int(versionNumbers[0])!
			self.minor = Int(versionNumbers[1])!
			self.revision = Int(versionNumbers[2])!
			self.buildNumber = Int(versionNumbers[3])!
		}
		if 3 == versionNumbers.count {
			self.major = Int(versionNumbers[0])!
			self.minor = Int(versionNumbers[1])!
			self.buildNumber = Int(versionNumbers[2])!
		}
		else if 2 == versionNumbers.count {
			self.major = Int(versionNumbers[0])!
			self.minor = Int(versionNumbers[1])!
			self.buildNumber = 0
		}
		else if 0 < versionNumbers.count {
			self.buildNumber = Int(versionNumbers[0])!
		}
	}

	func isGreaterThanBuildVersion(_ buildVersion: BuildVersion) -> Bool {
		//if major or minor versions are not specified, assume they are the same
		if self.major > -1 && buildVersion.major > -1 && self.major != buildVersion.major {
			return self.major > buildVersion.major
		}
		else if self.minor > -1 && buildVersion.minor > -1 && self.minor != buildVersion.minor {
			return self.minor > buildVersion.minor
		}
		else if self.revision > -1 && buildVersion.revision > -1 && self.revision != buildVersion.revision {
			return self.revision > buildVersion.revision
		}
		
		return self.buildNumber > buildVersion.buildNumber
	}
	
	@objc class func isVersionNewerCurrentVersion(_ currentVersion: String, updateVersion: String) -> Bool {
		let currentBuildVersion: BuildVersion = BuildVersion(string: currentVersion)
		let updateBuildVersion: BuildVersion = BuildVersion(string: updateVersion)
		return updateBuildVersion.isGreaterThanBuildVersion(currentBuildVersion)
	}

}
