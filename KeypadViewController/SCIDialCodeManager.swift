//
//  SCIDialCodeManager.swift
//  ntouch
//
//  Created by Kevin Selman on 12/2/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import Foundation

@objc class SCIDialCodeManager : NSObject {
	
	static let sharedInstance = SCIDialCodeManager()
	let swiftAppDelegate = UIApplication.shared.delegate as! AppDelegate

	
	override init()
	{
		super.init()

	}
	
	/// Returns a replacement string if the dial code was recognized, or nil otherwise.
	func processDialCode(code: String) -> String? {
		
		var replacementString = ""
		switch (code) {
			
		case "150":
			setTargetBitrate(bitRate: 1536) // enable 1.5 Mbps
		
		case "256":
			setTargetBitrate(bitRate: 256) // enable 256 kbps
		
		case "512":
			setTargetBitrate(bitRate: 512) // enable 512 kbps
		
		case "768":
			setTargetBitrate(bitRate: 768) // enable 768 kbps
		
		case "354": // "EKG"
			Alert("Heartbeat".localized, "Sending heartbeat request".localized)
			SCIVideophoneEngine.shared.heartbeat()
		
		case "2673": // "CORE" - show debug flags for logging
			SCIAccountManager.shared.displayDebugLogging()
		
		case "66564": // "NOLOG" - Removes the trace.conf file to prevent logging on startup
			var documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
			documentsPath.appendPathComponent("trace.conf")
			do {
				try FileManager.default.removeItem(atPath: documentsPath.absoluteString)
				Alert("Debug Logging".localized,
					  "trace.file.removed.log.disabled".localized)
				
			}
			catch let error as NSError {
				Alert("Debug Logging".localized,
					  "unable.remove.trace.conf".localizeWithFormat(arguments: error.description))
			}
		
		case "842": // toggle VRCL
			let vrclEnabled = SCIVideophoneEngine.shared.vrclEnabled
			SCIVideophoneEngine.shared.setVrclEnabledPrimitive(!vrclEnabled)

			Alert("VRCL", !vrclEnabled ? "VRCL Enabled".localized : "VRCL Disabled".localized);
		
		case "349": // enable/disable HUD
			swiftAppDelegate.showHUD = !swiftAppDelegate.showHUD;
			Alert("Toggling HUD".localized, swiftAppDelegate.showHUD ? "HUD Enabled".localized : "HUD Disabled".localized)
			
		
		case "4766": // "IPON" - enable dialing by IP address
			swiftAppDelegate.dialByIPEnabled = !swiftAppDelegate.dialByIPEnabled;
			Alert("Dialing By IP".localized, swiftAppDelegate.dialByIPEnabled ? "Enabled".localized : "Disabled".localized);
		
		case "4323": // H323 prefix
			swiftAppDelegate.dialByIPEnabled = true;
			replacementString = "H323:"
		
		case "4133": // toggle auto-hide for call window toolbar
			swiftAppDelegate.disableToolBarAutoHide = !swiftAppDelegate.disableToolBarAutoHide;
			Alert(swiftAppDelegate.disableToolBarAutoHide ?
					"Disabled toolbar auto hide".localized :
					"Enabled toolbar auto hide".localized,
				  nil)
		
		case "7474": // SIP prefix
			swiftAppDelegate.dialByIPEnabled = true;
			replacementString = "sip:10."
			
		case "7475": // SIP prefix
			swiftAppDelegate.dialByIPEnabled = true;
			replacementString = "sip:h264@209.169.244.210"

		case "7476": // SIP prefix
			swiftAppDelegate.dialByIPEnabled = true;
			replacementString = "sip:meet999@209.169.243.50"

		case "335266": // Delete all contacts and favorites
			SCIContactManager.shared.clearContacts()
			SCIContactManager.shared.clearFavorites()
			Alert("Deleted all contacts".localized, nil)
	
		case "866":
			let tunnelingEnabled = SCIVideophoneEngine.shared.tunnelingEnabled
			SCIVideophoneEngine.shared.tunnelingEnabled = !tunnelingEnabled
			Alert("Tunnelling Toggle".localized,
				  tunnelingEnabled ? "Tunneling OFF".localized : "Tunneling ON".localized)
		
		case "622":
			SCIVideophoneEngine.shared.resetNetworkMACAddress()
		
		case "666":
			// Crash the app - for testing crash logs
			// https://firebase.google.com/docs/crashlytics/test-implementation?platform=ios#force-test-crash
			let numbers = [0]
			let _ = numbers[1]
		
		case "837":
			let version = Bundle.main.infoDictionary?["CFBundleVersion"] as? String
			Alert("Build Version".localized, version);
		
		case "774":
			SCIVideophoneEngine.shared.setInternetSearchAllowedPrimitive(true)
			Alert("Enabled internet searches".localized, nil)
			
		case "263":
			CaptureController.shared.forcedCodec = kCMVideoCodecType_H263;
			DisplayController.shared.forcedCodec = kCMVideoCodecType_H263;
			Alert("Setting.Encode.Decode.Codec".localizeWithFormat(arguments: "H263"), "")
	
		case "1263":
			CaptureController.shared.forcedCodec = kCMVideoCodecType_H263;
			Alert("Setting.Encode.Codec".localizeWithFormat(arguments: "H263"), "")

		case "0263":
			DisplayController.shared.forcedCodec = kCMVideoCodecType_H263;
			Alert("Setting.Decode.Codec".localizeWithFormat(arguments: "H263"), "")
			
		case "264":
			CaptureController.shared.forcedCodec = kCMVideoCodecType_H264;
			DisplayController.shared.forcedCodec = kCMVideoCodecType_H264;
			Alert("Setting.Encode.Decode.Codec".localizeWithFormat(arguments: "H264"), "")
		
		case "1264":
			CaptureController.shared.forcedCodec = kCMVideoCodecType_H264;
			Alert("Setting.Encode.Codec".localizeWithFormat(arguments: "H264"), "")

		case "0264":
			DisplayController.shared.forcedCodec = kCMVideoCodecType_H264;
			Alert("Setting.Decode.Codec".localizeWithFormat(arguments: "H264"), "")

		case "265":
			CaptureController.shared.forcedCodec = kCMVideoCodecType_HEVC;
			DisplayController.shared.forcedCodec = kCMVideoCodecType_HEVC;
			Alert("Setting.Encode.Decode.Codec".localizeWithFormat(arguments: "H265"), "")

		case "1265":
			CaptureController.shared.forcedCodec = kCMVideoCodecType_HEVC;
			Alert("Setting.Encode.Codec".localizeWithFormat(arguments: "H265"), "")

		case "0265":
			DisplayController.shared.forcedCodec = kCMVideoCodecType_HEVC;
			Alert("Setting.Decode.Codec".localizeWithFormat(arguments: "H265"), "")
		
		case "255": // 255 == keypad ALL
			CaptureController.shared.forcedCodec = 0;
			DisplayController.shared.forcedCodec = 0;
			Alert("Setting Codecs to Defaults.".localized, "")
			
		case "423": // Read ICE Log and copy to clipboard.
			if let dir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
				var iceLog = ""
				let fileURL = dir.appendingPathComponent("RvIce.log")
				
				do {
					iceLog = try String(contentsOf: fileURL, encoding: .utf8)
					let pasteboard = UIPasteboard.general
					pasteboard.string = iceLog
					Alert("RvIce.log copied to clipboard.","")
				}
				catch {
					Alert("Error reading ICE Log","")
				}
			}
			
		case "785": // Read RV Log and copy to clipboard.
			if let dir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
				var sipLog = ""
				let fileURL = dir.appendingPathComponent("SipLog.txt")
				
				do {
					sipLog = try String(contentsOf: fileURL, encoding: .utf8)
					let pasteboard = UIPasteboard.general
					pasteboard.string = sipLog
					Alert("SipLog.txt copied to clipboard.","")
				}
				catch {
					Alert("Error reading Radvision Sip Log","")
				}
			}
		
		default:
			
			if code.hasPrefix("737") {
				let index = code.index(code.startIndex, offsetBy: 3)
				let numberToDial = String(code[index...])
				CallController.shared.repeatCallTest(to: numberToDial)
			}
			else if code.hasPrefix("899") {
				if (code.count == 4) // 899x - Auto-answer, x rings (0 to disable)
				{
					let index = code.index(code.startIndex, offsetBy: 3)
					let rings = String(code[index...])
					SCIDefaults.shared.autoAnswerRings = UInt(rings)!
					
					if (UInt(rings) == 0)
					{
						Alert("Auto-answer".localized, "Auto-answer Disabled.".localized);
					}
					else
					{
						Alert("Auto-answer".localized,
							  "Auto-answer after %@ rings.".localizeWithFormat(arguments: rings));
					}
				}
			}
			else {
				return nil
			}
		}
		
		// Valid Dial Code
		return replacementString
	}
	
	func setTargetBitrate(bitRate: UInt) {
		let rate = bitRate * 1000
		SCIVideophoneEngine.shared.setMaxAutoSpeeds(rate, sendSpeed: rate)
	
		let defaults = UserDefaults.standard
		defaults.set( bitRate * 10, forKey: "targetBitRate")
		defaults.synchronize()
		let message = String(format: "%dkb", "Bitrate: ".localized, bitRate)
		Alert("Set Bitrate".localized, message)
	}

}
