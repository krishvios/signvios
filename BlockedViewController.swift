//
//  BlockedViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 10/11/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import Foundation

class BlockedViewController: UITableViewController, UISearchResultsUpdating
{
	var searchController = UISearchController(searchResultsController: nil)
	var blockedsArray: Array<SCIBlocked> = Array.init()
	let cellReuseIdentifier = "BlockedCell"
	
	func reloadBlockeds()
	{
		if var tempBlockeds: Array<SCIBlocked> = SCIBlockedManager.shared.blockeds as? Array<SCIBlocked>
		{
			// Apply Search criteria
			let searchText = searchController.searchBar.text!
			if(searchText.count > 0 && tempBlockeds.count > 0)
			{
				let filteredContactsArray = tempBlockeds.filter({ (blocked: SCIBlocked) -> Bool in
					return blocked.title.lowercased().contains(searchText.lowercased()) || blocked.number.lowercased().contains(searchText.lowercased())
				})
				tempBlockeds = filteredContactsArray
			}
			
			self.blockedsArray = tempBlockeds
			self.navigationItem.title = "blocked.count".localizeWithFormat(arguments: tempBlockeds.count)
		}
	}
	
	override func numberOfSections(in tableView: UITableView) -> Int
	{
		return 1
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int
	{
		return blockedsArray.count
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell
	{
		let cell = tableView.dequeueReusableCell(withIdentifier: cellReuseIdentifier) as? ThemedTableViewCell ?? ThemedTableViewCell(style: .subtitle, reuseIdentifier: cellReuseIdentifier)
		
		let blockedNumber: SCIBlocked = blockedsArray[indexPath.row]
		cell.imageView?.image = UIImage.init(named: "Blocked_Grey20")
		cell.textLabel?.text = blockedNumber.title
		cell.detailTextLabel?.text = FormatAsPhoneNumber(blockedNumber.number)
		return cell
	}
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath)
	{
		let blockedNumber = blockedsArray[indexPath.row]
		let blockedDetailViewController: BlockedDetailViewController = BlockedDetailViewController.init(style: .grouped)
		blockedDetailViewController.selectedBlocked = blockedNumber
		self.navigationController?.pushViewController(blockedDetailViewController, animated: true)
	}
	
	func search()
	{
		self.reloadBlockeds()
		self.tableView.reloadData()
	}
	
	func updateSearchResults(for searchController: UISearchController) {
		search()
	}
	
	@objc func addBlockedItem()
	{
		let blockedDetailViewController: BlockedDetailViewController = BlockedDetailViewController.init(style: .grouped)
		blockedDetailViewController.selectedBlocked = SCIBlocked.init()
		blockedDetailViewController.isNewBlocked = true
		self.navigationController?.pushViewController(blockedDetailViewController, animated: true)
		AnalyticsManager.shared.trackUsage(.blockedList, properties: ["description" : "add_item" as NSObject])
	}
	
	@objc func notifyBlockedsChanged(note: NSNotification)
	{
		self.didUpdate()
	}
	
	func didUpdate()
	{
		if SCIVideophoneEngine.shared.isAuthorized {
			self.reloadBlockeds()
			self.tableView.reloadData()
		}
	}
	
	override func viewWillAppear(_ animated: Bool)
	{
		super.viewWillAppear(animated)
		searchController.searchBar.barStyle = Theme.current.searchBarStyle
		searchController.searchBar.keyboardAppearance = Theme.current.keyboardAppearance
		searchController.hidesNavigationBarDuringPresentation = true
		
		// Log to analytics
		AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "block_list_view" as NSObject])
		
		self.didUpdate()
	}
	
	override func viewDidLoad()
	{
		super.viewDidLoad()
		
		NotificationCenter.default.addObserver(self, selector: #selector(self.notifyBlockedsChanged(note:)), name: .SCINotificationBlockedsChanged, object: SCIBlockedManager.shared)
		
		let addButton: UIBarButtonItem = UIBarButtonItem.init(barButtonSystemItem: .add, target: self, action: #selector(self.addBlockedItem))
		self.navigationItem.setRightBarButton(addButton, animated: true)
		
		// Add Search bar
		searchController.searchResultsUpdater = self
		searchController.obscuresBackgroundDuringPresentation = false
		searchController.searchBar.placeholder = "Search".localized
		
		if #available(iOS 11.0, *) {
			navigationItem.searchController = searchController
			navigationItem.hidesSearchBarWhenScrolling = false
		} else {
			tableView.tableHeaderView = searchController.searchBar
		}
		// Register the table view cell class and its reuse id
		self.tableView.register(UITableViewCell.self, forCellReuseIdentifier: cellReuseIdentifier)

		
		if #available(iOS 9.0, *) {
			self.tableView.cellLayoutMarginsFollowReadableWidth = false
		} else {
			// Fallback on earlier versions
		}
		
		self.edgesForExtendedLayout = []
		AnalyticsManager.shared.trackUsage(.blockedList, properties: ["description" : "block_list_viewed" as NSObject])
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		searchController.isActive = false
		super.viewWillDisappear(animated)
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
	
	override func didReceiveMemoryWarning()
	{
		super.didReceiveMemoryWarning()
	}
}
