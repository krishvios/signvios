//
//  RecentSummaryCollectionViewHeader.swift
//  ntouch
//
//  Created by Kevin Selman on 4/11/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

@IBDesignable
class RecentSummaryCollectionViewHeader : UICollectionReusableView {
	
	@IBOutlet var clearButton:UIButton!
	static let identifier : String = "RecentSummaryCollectionViewHeader"
	
	@IBOutlet var visualEffectView: UIVisualEffectView!
	
	
	override func layoutSubviews() {
		super.layoutSubviews()
		visualEffectView.layer.cornerRadius = min(visualEffectView.frame.width, visualEffectView.frame.height) / 2
	}
}
