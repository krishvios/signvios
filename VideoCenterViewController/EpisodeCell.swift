//
//  EpisodeCell.swift
//  ntouch
//
//  Created by Dan Shields on 5/21/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class EpisodeCell: ThemedTableViewCell {
	@IBOutlet var episodeTitle: UILabel!
	@IBOutlet var episodeSubTitle: UILabel!
	@IBOutlet var statusImage: UIImageView!
	@IBOutlet var progressView: UIProgressView!
	@IBOutlet var progressLabel: UILabel!
	@objc var isViewed: Bool = true {
		didSet {
			applyTheme()
		}
	}
	
	override func applyTheme() {
		super.applyTheme()
		
		if isViewed {
			episodeTitle.font = Theme.current.tableCellFont
		}
		else {
			episodeTitle.font = Theme.current.tableCellFont.font(byAdding: .traitBold)
		}
		
		episodeTitle.textColor = Theme.current.tableCellTextColor
		episodeSubTitle.font = Theme.current.tableCellSecondaryFont
		episodeSubTitle.textColor = Theme.current.tableCellSecondaryTextColor
		progressLabel.font = Theme.current.tableCellSecondaryFont
		progressLabel.textColor = Theme.current.tableCellSecondaryTextColor
	}
}
