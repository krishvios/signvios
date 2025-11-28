//
//  VideoCenterChannelCell.swift
//  ntouch
//
//  Created by Joseph Laser on 4/17/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@objc class VideoCenterChannelCell: ThemedTableViewCell
{
	
	@IBOutlet var channelImageView: UIImageView!
	@IBOutlet var channelNameLabel: UILabel!
	@IBOutlet var unviewedCountLabel: UITextView!
	@IBOutlet var viewedDot: UIView!
	
	let channelImages = ["ntouch" : "ntouch_icon_color_lg",
						 "Events" : "events_icon_color_lg",
						 "FCC News" : "fcc_icon_color_lg",
						 "News" : "news_icon_color_lg",
						 "Support" : "support_icon_color_lg",
						 "General Video" : "generic_icon_color_lg",
						 "Favorites" : "fav_icon_color_lg",
						 "Community" : "community_icon_color_lg",
						 "DKN" : "dkn_icon_color_lg",
						 "PSA" : "psa_icon_color_lg",
						 "SN" : "sn_icon_color_lg",
						 "Sorenson" : "SorensonVRS",
						 "School News" : "schoolnews_icon_color_lg",
						 "Gallaudet" : "gallaudet_icon_color_lg",
						 "FSDB" : "fsdbicon_color_lg",
						 "NTID" : "ntidicon_color_lg"]

	var category: SCIMessageCategory?
	var badgeString: String?
	var badgeCount = 0
	
	override func layoutSubviews() {
		super.layoutSubviews()
		separatorInset.left = channelImageView.frame.origin.x + channelImageView.frame.size.width + 16
	}
	
	override func applyTheme()
	{
		super.applyTheme()
		backgroundColor = Theme.current.tableCellBackgroundColor
		tintColor = Theme.current.tableCellTintColor
		channelNameLabel.textColor = Theme.current.tableCellTextColor
		channelNameLabel.font = Theme.current.tableCellFont
		unviewedCountLabel.font = Theme.current.tableCellSecondaryFont
	}
	
	func update()
	{
		if let category = category
		{
			let imageName = channelImages[category.shortName] ?? "generic_icon_color_lg"
			
			channelImageView.image = UIImage(named: imageName)?.resize(CGSize(width: 60, height: 60))
			channelNameLabel.text = category.longName
			
			badgeString = nil
			if badgeCount > 0
			{
				unviewedCountLabel.isHidden = false
				unviewedCountLabel.text = String(badgeCount)
				unviewedCountLabel.backgroundColor = .lightGray
				unviewedCountLabel.layer.masksToBounds = true
				unviewedCountLabel.layer.cornerRadius = 5
				unviewedCountLabel.textContainerInset = UIEdgeInsets(top: 0, left: 5, bottom: 0, right: 5)
			
				viewedDot.isHidden = false
			}
			else
			{
				unviewedCountLabel.isHidden = true
				viewedDot.isHidden = true
			}
		}
	}
	
	@objc func configure(WithCategory theCategory: SCIMessageCategory, count: Int)
	{
		category = theCategory
		badgeCount = count
		update()
	}
	
	func selected(in signMailListVC: SignMailListViewController)
	{
		signMailListVC.tableView.allowsSelection = true
		if let category = category
		{
			let episodeViewController = VideoCenterEpisodeViewController(style: .grouped)
			episodeViewController.selectedCategory = category
			episodeViewController.channelImage = channelImages[category.shortName] ?? "generic_icon_color_lg"
			signMailListVC.navigationController?.pushViewController(episodeViewController, animated: true)
		}
	}
	
	@available(iOS 11.0, *)
	func trailingSwipeActions(for indexPath: IndexPath) -> UISwipeActionsConfiguration?
	{
		return nil
	}
	
	@available(iOS, deprecated: 13.0)
	func editActions(for indexPath: IndexPath) -> [UITableViewRowAction]?
	{
		return nil
	}
}
