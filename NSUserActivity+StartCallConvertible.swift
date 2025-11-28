/*
	Copyright (C) 2016 Apple Inc. All Rights Reserved.
	See LICENSE.txt for this sample’s licensing information
	
	Abstract:
	Extension to allow creating a CallKit CXStartCallAction from an NSUserActivity which the app was launched with
*/

import Foundation
import Intents

extension NSUserActivity: StartCallConvertible {

    fileprivate struct Constants {
		static let ActivityType: String =
		{
			if #available(iOS 13.0, *)
			{
				return String(describing: INStartCallIntent.self)
			}
			else
			{
				return String(describing: INStartVideoCallIntent.self)
			}
		}()
    }

    var startCallHandle: String? {
		if #available(iOS 10.0, *) {
			guard activityType == Constants.ActivityType else {
				return nil
			}
		} else {
			return nil
		}

		if #available(iOS 13.0, *) {
			guard let
				interaction = interaction,
				let startVideoCallInternt = interaction.intent as? INStartCallIntent,
				let contact = startVideoCallInternt.contacts?.first
				
				else {
					return nil
				}
			return contact.personHandle?.value
		}
		else {
			guard let
				interaction = interaction,
				let startVideoCallInternt = interaction.intent as? INStartVideoCallIntent,
				let contact = startVideoCallInternt.contacts?.first
				
				else {
					return nil
			}
				return contact.personHandle?.value
		}
    }
    
}
