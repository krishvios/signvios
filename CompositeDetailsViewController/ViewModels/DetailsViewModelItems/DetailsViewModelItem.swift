//
//  DetailsViewModelItem.swift
//  ntouch
//
//  Created by Cody Nelson on 2/11/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/**
	Types of Details View Model sub-components.

	Contains types of 'DetailViewModel' subcomponents; These components help convey details about
	their model in different ways.
*/
enum DetailsViewModelItemType {
	
	/// displays name and photo details.
	case nameAndPhoto
	
	/// displays phone numbers of different types.
	case numbers
	
	/// provides constructive interaction.
	case constructiveOptions
	
	/// provides destructive interaction.
	case destructiveOptions
	
	case relayLanguage
}

// MARK: - Caller Details View Model Item

/// A single component of a 'DetailsViewModel'
protocol DetailsViewModelItem : HasRowCount, HasSectionHeader {
	
	var type : DetailsViewModelItemType {get}

}

