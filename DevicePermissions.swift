//
//  DevicePermissions.swift
//  ntouch
//
//  Created by Kevin Selman on 5/8/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import Foundation
import AVKit


public class DevicePermissions: NSObject {

	@objc static func checkAndAlertVideoPermissions(fromView view:UIViewController, withCompletion completion: @escaping (_ granted: Bool) -> ()) throws {
		AVCaptureDevice.requestAccess(for: AVMediaType.video) { (granted) in
		
			DispatchQueue.main.async {
				if (granted) {
					completion(granted)
				}
				else {
					let alert = UIAlertController.init(title: "Enable Camera", message: "To use ntouch, you'll need to enable Camera access in Settings.", preferredStyle: .alert)
					alert.addAction(UIAlertAction(title: "Cancel", style: .cancel) { action in
						completion(granted)
					})
			
					alert.addAction(UIAlertAction(title: "Settings", style: .default) { action in
			
						if let settingsUrl = URL(string: UIApplication.openSettingsURLString) {
							
							if #available(iOS 10.0, *) {
								// iOS 10.0.
								UIApplication.shared.open(settingsUrl, options: [:], completionHandler: nil)
								
							} else {
								// Fallback on earlier versions.
								UIApplication.shared.openURL(settingsUrl)
							}
						}
						completion(granted)
					})
					
					view.present(alert, animated: true, completion: nil)
					
				}
			}
		}
	}
}
