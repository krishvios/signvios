//
//  EndpointSelectionController.swift
//  ntouch
//
//  Created by Joseph Laser on 2/10/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

class EndpointSelectionController: NSObject {
	@objc var ringGroupInfos: [SCIRingGroupInfo] = []
	public var endpointSelectionActionSheet: UIAlertController?
	let appDelegate = UIApplication.shared.delegate as? AppDelegate
	
	let defaults = SCIDefaults.shared
	
	@objc func showEndpointSelectionMenu() {
		endpointSelectionActionSheet = UIAlertController(title: "Select which account you want to use.", message: nil, preferredStyle: .actionSheet)
		
		endpointSelectionActionSheet?.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel) { _ in
			SCIAccountManager.shared.cancelLogin()
		})
		
		if ringGroupInfos.count > 0 {
			for info in ringGroupInfos {
				endpointSelectionActionSheet?.addAction(UIAlertAction(title: info.name.count > 0 ? info.name : "No Name", style: .default) { (action: UIAlertAction!) in
					let endpointName = action.title
					var selectedInfo = SCIRingGroupInfo()
					
					// Find SCIRingGroupInfo for selection
					for info in self.ringGroupInfos {
						if info.name == endpointName {
							selectedInfo = info
							break
						}
					}
					
					SCIVideophoneEngine.shared.loginName = selectedInfo.phone
					self.appDelegate?.login(usingCache: false, withImmediateCallback: {
					}, errorHandler: { error in
						if error != nil {
//							SCILog("Failed End Point Selection login. Error: %@", error)
							Alert("Error logging in.", "\("")")
						}
					}, successHandler: {
						SCIAccountManager.shared.state = NSNumber(value: CSSignedIn.rawValue)
						// Turn on autoSignIn.  User has signed in.
						self.defaults?.isAutoSignIn = true
					})
				})
			}
            
            if let popover = endpointSelectionActionSheet?.popoverPresentationController {
                popover.sourceView = appDelegate?.topViewController()?.view
                popover.sourceRect = appDelegate?.topViewController()?.view.bounds ?? CGRect(x: 0, y: 0, width: 0, height: 0)
                popover.permittedArrowDirections = .any
            }
            
            appDelegate?.topViewController()?.present(endpointSelectionActionSheet!, animated: true, completion: nil)
		}
	}
}
