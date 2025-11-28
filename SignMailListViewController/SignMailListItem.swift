//
//  SignMailListItem.swift
//  ntouch
//
//  Created by Joseph Laser on 4/17/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class SignMailListItem: NSObject
{
	var parentTable: UITableView
	var cellIdentifier: String
	
	var cellMessage: SCIMessageInfo?
	var cellCategory: SCIMessageCategory?
	var categoryCount = 0
	
	public init(tableView: UITableView, identifier: String, message: SCIMessageInfo)
	{
		parentTable = tableView
		cellIdentifier = identifier
		cellMessage = message
		super.init()
	}
	
	public init(tableView: UITableView, identifier: String, category: SCIMessageCategory, count: Int)
	{
		parentTable = tableView
		cellIdentifier = identifier
		cellCategory = category
		super.init()
	}
	
	func configure() -> UITableViewCell?
	{
		var cell: UITableViewCell?
		if let message = cellMessage
		{
			cell = parentTable.dequeueReusableCell(withIdentifier: cellIdentifier) as? SignMailCell ?? SignMailCell(style: .default, reuseIdentifier: cellIdentifier)
			(cell as! SignMailCell).details = ContactSupplementedDetails(details: MessageDetails(message: message))
		}
		else if let category = cellCategory
		{
			cell = parentTable.dequeueReusableCell(withIdentifier: cellIdentifier) as? VideoCenterChannelCell ?? VideoCenterChannelCell(style: .default, reuseIdentifier: cellIdentifier)
			(cell as! VideoCenterChannelCell).configure(WithCategory: category, count: categoryCount)
		}
		return cell
	}
	
	func selected(cell: UITableViewCell, in signMailListVC: SignMailListViewController)
	{
		if cellMessage != nil
		{
			(cell as! SignMailCell).selected(in: signMailListVC)
		}
		else if cellCategory != nil
		{
			(cell as! VideoCenterChannelCell).selected(in: signMailListVC)
		}
		else
		{
			signMailListVC.tableView.allowsSelection = true
		}
	}
	
	@available(iOS 11.0, *)
	func trailingSwipeActions(for cell: UITableViewCell, in signMailListVC: SignMailListViewController, indexPath: IndexPath) -> UISwipeActionsConfiguration?
	{
		if cellMessage != nil
		{
			return (cell as! SignMailCell).trailingSwipeActions(for: indexPath, in: signMailListVC)
		}
		else if cellCategory != nil
		{
			return (cell as! VideoCenterChannelCell).trailingSwipeActions(for: indexPath)
		}
		return nil
	}
	
	@available(iOS, deprecated: 13.0)
	func editActions(for cell: UITableViewCell, in signMailListVC: SignMailListViewController, indexPath: IndexPath) -> [UITableViewRowAction]?
	{
		if cellMessage != nil
		{
			return (cell as! SignMailCell).editActions(for: indexPath, in: signMailListVC)
		}
		else if cellCategory != nil
		{
			return (cell as! VideoCenterChannelCell).editActions(for: indexPath)
		}
		return nil
	}
}
