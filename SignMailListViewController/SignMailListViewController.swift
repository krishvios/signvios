//
//  SignMailListViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 1/8/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@objc class SignMailListViewController: SCITableViewController, UISearchBarDelegate, UISearchResultsUpdating
{
	enum VideoListType : Int
	{
		case signMail
		case videoCenter
	}
	let swiftAppDelegate = UIApplication.shared.delegate as! AppDelegate
	static let _toSignMailDetailsSegueIdentifier  = "SignMailToDetailsSegue"
	private var clearButton: UIBarButtonItem?
	@IBOutlet var composeButton: UIBarButtonItem!
	var selectedMessage: SCIMessageInfo?
	@IBOutlet var sectionBar: UISegmentedControl!
	let searchController = UISearchController(searchResultsController: nil)
	var itemArray: [SignMailListItem] = []
	
	static let lastViewedVideoCenterKey: String = "VideoCenterLastViewed"
	var videoCenterBadgeCount = 0
	
	// MARK: View lifecycle
	
	override func awakeFromNib()
	{
		super.awakeFromNib()
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCategoryChanged), name: .SCINotificationMessageCategoryChanged, object: SCIVideophoneEngine.shared)
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
	
	override func viewDidLoad()
	{
		super.viewDidLoad()
		
		self.extendedLayoutIncludesOpaqueBars = true
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyMessageChanged), name: .SCINotificationMessageChanged, object: SCIVideophoneEngine.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationContactsChanged, object: SCIContactManager.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyBlockedsChanged), name: .SCINotificationBlockedsChanged, object: SCIBlockedManager.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPropertyChanged), name: .SCINotificationPropertyChanged, object: SCIVideophoneEngine.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyLoggedIn), name: .SCINotificationLoggedIn, object: SCIVideophoneEngine.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(reachabilityChanged), name: .networkReachabilityDidChange, object: nil)
		
		layoutNavigationBar()
		
		searchController.searchResultsUpdater = self
		searchController.obscuresBackgroundDuringPresentation = false
		searchController.searchBar.placeholder = "Search".localized
		
		sectionBar.sizeToFit()
		navigationItem.titleView = sectionBar
		if #available(iOS 11.0, *) {
			navigationItem.searchController = searchController
			navigationItem.hidesSearchBarWhenScrolling = false
		} else {
			tableView.tableHeaderView = searchController.searchBar
		}
		definesPresentationContext = true
		
		tableView.rowHeight = UITableView.automaticDimension
	}
	
	override func viewWillAppear(_ animated: Bool)
	{
		super.viewWillAppear(animated)
		searchController.searchBar.barStyle = Theme.current.searchBarStyle
		searchController.searchBar.keyboardAppearance = Theme.current.keyboardAppearance
		searchController.hidesNavigationBarDuringPresentation = true
		
		navigationController?.navigationBar.sizeToFit()
		navigationController?.navigationBar.isTranslucent = true
		
		didUpdate()
	}
	
	override func loadView() {
		super.loadView()
	}
	
	override func viewDidAppear(_ animated: Bool)
	{
		super.viewDidAppear(animated)
		if (itemArray.count > 0) {
			SCIVideophoneEngine.shared.updateLastMessageViewTime()
		}
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		// If we don't end editing here, the searchBarEndEditing will re-show the navigation bar that was suppose to be
		// hidden when the contact details view controller viewWillAppear hid it.
		searchController.hidesNavigationBarDuringPresentation = false
		searchController.isActive = false
		setEditing(false, animated: false)
		super.viewWillDisappear(animated)
	}
	
	override func viewDidDisappear(_ animated: Bool)
	{
		super.viewDidDisappear(animated)
	}
	
	func updateFooter()
	{
		tableView.tableFooterView = nil
		
		// Display a custom footer when fetched results section count is 0.
		// tableView will not render footer without sections.  This occurs only when sectionNameKeyPath is used in fetchedResultsController
		if itemArray.count == 0
		{
			tableView.tableFooterView = viewController(self, viewForFooterInSection: 0)
		}
	}
	
	func layoutNavigationBar()
	{
		switch sectionBar.selectedSegmentIndex {
		case VideoListType.signMail.rawValue:
			navigationItem.rightBarButtonItem = editButtonItem
			if SCIVideophoneEngine.shared.directSignMailEnabled
			{
				composeButton.accessibilityLabel = "Compose"
				navigationItem.leftBarButtonItem = composeButton
			}
			else
			{
				navigationItem.leftBarButtonItem = nil
			}
		default:
			navigationItem.rightBarButtonItem = nil
			navigationItem.leftBarButtonItem = nil
		}
		
		navigationController?.navigationBar.sizeToFit()
	}
	
	// MARK: Table view methods
	
	override func numberOfSections(in tableView: UITableView) -> Int
	{
		return 1
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int
	{
		switch section {
		case 0:
			return itemArray.count
		default:
			return 0
		}
	}
	
	override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat
	{
		return (self.tableView(tableView, viewForHeaderInSection: section)?.frame.size.height ?? 0) + 10
	}
	
	override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView?
	{
		switch sectionBar.selectedSegmentIndex {
		case VideoListType.videoCenter.rawValue:
			// Center label
			let label = UILabel()
			label.frame = CGRect(x: 0, y: 0, width: view.frame.size.width, height: 15)
			label.backgroundColor = .clear
			label.textColor = Theme.current.textColor
			label.font = UIFont.boldSystemFont(ofSize: 17)
			label.text = "Channels".localized
			label.center = CGPoint(x: view.frame.size.width / 2, y: 17.5)
			label.textAlignment = .center
			
			let view = UIView(frame: CGRect(x: 0, y: 0, width: tableView.frame.size.width, height: 25))
			view.addSubview(label)
			return view
		default:
			return viewController(self, viewForHeaderInSection: section)
		}
	}
	
	override func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat
	{
		if let tempView = viewController(self, noRecordsViewForFooterInSection: section)
		{
			return tempView.frame.size.height
		}
		return 0
	}
	
	override func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView?
	{
		switch sectionBar.selectedSegmentIndex {
		case VideoListType.signMail.rawValue:
			if section == 0
			{
				return viewController(self, noRecordsViewForFooterInSection: section)
			}
			return nil
		default:
			return nil
		}
	}
	
	override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String?
	{
		return nil
	}
	
	override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String?
	{
		switch sectionBar.selectedSegmentIndex {
		case VideoListType.signMail.rawValue:
			if section == 0 && itemArray.count == 0
			{
				return "You don't have any SignMails.".localized
			}
		default:
			break
		}
		return nil
	}
	
	override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool
	{
		switch sectionBar.selectedSegmentIndex {
		case VideoListType.signMail.rawValue:
			if indexPath.section == 0
			{
				return true
			}
		default:
			break
		}
		return false
	}
	
	@available(iOS 11.0, *)
	override func tableView(_ tableView: UITableView, leadingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration?
	{
		return nil
	}
	
	@available(iOS 11.0, *)
	override func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration?
	{
		switch indexPath.section {
		case 0:
			if let cell = tableView.cellForRow(at: indexPath)
			{
				let item = itemArray[indexPath.row]
				return item.trailingSwipeActions(for: cell, in: self, indexPath: indexPath)
			}
		default:
			break
		}
		return nil
	}
	
	override func tableView(_ tableView: UITableView, editActionsForRowAt indexPath: IndexPath) -> [UITableViewRowAction]?
	{
		switch indexPath.section {
		case 0:
			if let cell = tableView.cellForRow(at: indexPath)
			{
				let item = itemArray[indexPath.row]
				return item.editActions(for: cell, in: self, indexPath: indexPath)
			}
		default:
			break
		}
		return nil
	}
	
	override func tableView(_ tableView: UITableView, willBeginEditingRowAt indexPath: IndexPath)
	{
		
	}
	
	override func tableView(_ tableView: UITableView, didEndEditingRowAt indexPath: IndexPath?)
	{
		
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell
	{
		switch indexPath.section {
		case 0:
			let listItem = itemArray[indexPath.row]
			if let cell = listItem.configure()
			{
				return cell
			}
		default:
			break
		}
		let emptyId = "EmptyCell"
		let cell = tableView.dequeueReusableCell(withIdentifier: emptyId) as? ThemedTableViewCell ?? ThemedTableViewCell(style: .default, reuseIdentifier: emptyId)
		return cell
	}
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath)
	{
		tableView.deselectRow(at: indexPath, animated: true)
		switch indexPath.section {
		case 0:
			if let cell = tableView.cellForRow(at: indexPath)
			{
				tableView.allowsSelection = false
				let item = itemArray[indexPath.row]
				item.selected(cell: cell, in: self)
			}
			break
		default:
			break
		}
	}
	
	override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
		if let controller = segue.destination as? DetailsViewController,
			let selectedItem = self.selectedMessage
		{
			controller.details = ContactSupplementedDetails(details: MessageDetails(message: selectedItem))
		}
	}
	
	override func tableView(_ tableView: UITableView, accessoryButtonTappedForRowWith indexPath: IndexPath)
	{
		guard !indexPath.isEmpty else { return }
		selectedMessage = itemArray[indexPath.row].cellMessage
		performSegue(withIdentifier: SignMailListViewController._toSignMailDetailsSegueIdentifier,
					 sender: self)
		AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "item_details_viewed" as NSObject])
	}
	
	override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool
	{
		return false
	}
	
	// Invoked when the user touches Edit.
	override func setEditing(_ editing: Bool, animated: Bool) {
		AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "edit" as NSObject])
		// Updates the appearance of the Edit|Done button as necessary.
		super.setEditing(editing, animated: animated)
		tableView.setEditing(editing, animated: true)
		// replace the add button while editing
		if isEditing {
			clearButton = UIBarButtonItem(image: UIImage(named: "TrashCan"), style: .plain, target: self, action: #selector(clearList))
			self.navigationItem.setLeftBarButton(clearButton, animated: true)
			clearButton?.isEnabled = itemArray.count > 0
			clearButton?.accessibilityLabel = "Delete".localized
		}
		else {
			clearButton = nil
			self.navigationItem.leftBarButtonItem = composeButton
		}
	}
	
	// MARK: IBActions
	
	@IBAction func clearList(_ sender: Any)
	{
		AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "delete_all_signmail" as NSObject])
		let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
		actionSheet.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
		actionSheet.addAction(UIAlertAction(title: "Delete All SignMails".localized, style: .destructive, handler: { (action) in
			SCINotificationManager.shared.didStartNetworking()
			SCIVideophoneEngine.shared.deleteMessages(inCategory: SCIVideophoneEngine.shared.signMailMessageCategoryId)
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "signmail_cleared" as NSObject])
			self.setEditing(false, animated: true)
		}))
		
		if let popover = actionSheet.popoverPresentationController
		{
			popover.barButtonItem = clearButton
			popover.permittedArrowDirections = .any
		}
		
		present(actionSheet, animated: true, completion: nil)
	}
	
	@IBAction func composeSignMail()
	{
		AnalyticsManager.shared.trackUsage(.signMail, properties: ["description": "send_new_signmail" as NSObject])
		
		if swiftAppDelegate.networkMonitor.bestInterfaceType == .none
		{
			Alert("Network Error".localized, "signmail.compose.nonetwork".localized)
			return
		}
	
		let viewController = PickPhoneViewController()
		viewController.pickPhoneDelegate = self
		if UIDevice.current.userInterfaceIdiom == .pad
		{
			viewController.modalPresentationStyle = .formSheet
		}

		present(viewController, animated: true)
	}
	
	// MARK: Fetched results
	
	func fetchChannelData()
	{
		let channelsArray = SCIVideophoneEngine.shared.messageCategories.filter() { $0.type != 1 }
		itemArray.removeAll()
		videoCenterBadgeCount = 0
		
		for parentCategory in channelsArray
		{
			// Get episodes
			var programsArray = SCIVideophoneEngine.shared.messageSubcategories(forCategoryId: parentCategory.categoryId) ?? []
			
			// if this channel doesnt have any subcategories, use parent category.
			if programsArray.count == 0 {
				programsArray.append(parentCategory)
			}
			
			let lastViewedDate = UserDefaults.standard.object(forKey: SignMailListViewController.lastViewedVideoCenterKey) as? Date
			let messages = programsArray.flatMap { SCIVideophoneEngine.shared.messages(forCategoryId: $0.categoryId) ?? [] }
			let unreadCount = messages.lazy
				.filter { message in !message.viewed && message.date > lastViewedDate ?? Date(timeIntervalSince1970: 0) }
				.count
			
			videoCenterBadgeCount += unreadCount
			itemArray.append(SignMailListItem(tableView: tableView, identifier: "VideoCenterCell", category: parentCategory, count: messages.count))
		}
		
		updateBadge()
	}
	
	func fetchMessageData()
	{
		let signMailCategory = SCIVideophoneEngine.shared.signMailMessageCategoryId
		var messagesArray = SCIVideophoneEngine.shared.messages(forCategoryId: signMailCategory) ?? []
		
		// Apply Sorting
		messagesArray = messagesArray.sorted(by: { obj1, obj2 in
			return obj1.date.compare(obj2.date) == .orderedDescending
		})
		
		// Apply Search criteria
		if let searchText = searchController.searchBar.text
		{
			if searchText.count > 0
			{
				let filteredSignMailArray = messagesArray.filter() { ($0.name != nil && $0.name.localizedCaseInsensitiveContains(searchText)) }
				messagesArray = filteredSignMailArray
			}
		}
		
		itemArray.removeAll()
		for message in messagesArray
		{
			itemArray.append(SignMailListItem(tableView: tableView, identifier: "SignMailCell", message: message))
		}
	}
	
	func updateBadge()
	{
		let count = SCINotificationManager.shared.signMailBadgeCount
		navigationController?.tabBarItem.badgeValue = (count > 0) ? String(format: "%ld", Int(count)) : nil
		
		searchController.searchBar.scopeButtonTitles?[VideoListType.videoCenter.rawValue] = videoCenterBadgeCount > 0 ? "Video Center ●".localized : "Video Center".localized
		SCIVideophoneEngine.shared.pulseSignMail(count > 0);
	}
	
	func didUpdate()
	{
		if SCIVideophoneEngine.shared.isAuthorized
		{
			switch sectionBar.selectedSegmentIndex {
			case VideoListType.videoCenter.rawValue:
				fetchMessageData()
				fetchChannelData()
			default:
				fetchChannelData()
				fetchMessageData()
				if self.viewIfLoaded?.window != nil && itemArray.count > 0 {
					SCIVideophoneEngine.shared.updateLastMessageViewTime()
				}
				
			}
			tableView.reloadData()
			SCINotificationManager.shared.didStopNetworking()
		}
		updateBadge()
	}
	
	// MARK: Notifications
	
	@objc func reachabilityChanged(note: NSNotification)
	{
		if let networkTypeInt = note.userInfo?[networkReachabilityKey] as? NSNumber {
			let networkType = NetworkType(rawValue: networkTypeInt.intValue)
			if networkType != NetworkType.none {
				tableView.reloadData()
			}
		}
	}

	@objc func notifyCategoryChanged(note: NSNotification) // SCINotificationMessageCategoryChanged
	{
		didUpdate()
	}
	
	@objc func notifyMessageChanged(note: NSNotification) // SCINotificationMessageChanged
	{
		if let item = note.userInfo?[SCINotificationKeyItemId] as? SCIItemId {
			var index = 0
			self.itemArray.forEach { message in
				if let cellMsg = message.cellMessage {
					if cellMsg.messageId.isEqual(to: item) {
						self.tableView.beginUpdates()
						self.tableView.reloadRows(at: [IndexPath(row: index, section: 0)], with: .automatic)
						self.tableView.endUpdates()
					}
				}
				index = index + 1
			}
		}
	}

	@objc func notifyContactsChanged(note: NSNotification) // SCINotificationContactsChanged
	{
		didUpdate()
	}
	
	@objc func notifyBlockedsChanged(note: NSNotification) // SCINotificationBlockedsChanged
	{
		didUpdate()
	}
	
	@objc func notifyPropertyChanged(note: NSNotification) // SCINotificationPropertyChanged
	{
		let propertyName: String = note.userInfo?[SCINotificationKeyName] as! String
		if propertyName == SCIPropertyNameDirectSignMailEnabled
		{
			layoutNavigationBar()
		}
	}
	
	@objc func notifyLoggedIn(note: NSNotification) // SCINotificationLoggedIn
	{
		layoutNavigationBar()
		didUpdate()
	}
	
	// MARK: UISearchBarDelegate
	
	func updateSearchResults(for searchController: UISearchController) {
		search()
	}
	
	func search()
	{
		fetchMessageData()
		if #available(iOS 11.0, *) {
			tableView.setContentOffset(CGPoint(x: 0, y: -tableView.adjustedContentInset.top), animated: true)
		} else {
			tableView.setContentOffset(CGPoint(x: 0, y: -tableView.contentInset.top), animated: true)
		}
		tableView.reloadData()
	}
	
	@IBAction func sectionDidChange() {
		switch sectionBar.selectedSegmentIndex {
		case VideoListType.signMail.rawValue:
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "signmail_scope_bar" as NSObject])
			showSignMail()
		case VideoListType.videoCenter.rawValue:
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "videocenter_scope_bar" as NSObject])
			self.setEditing(false, animated: true)
			showVideoCenter()
		default:
			break
		}
	}
	
	public func showSignMail() {
		guard isViewLoaded else { return }
		
		sectionBar.selectedSegmentIndex = VideoListType.signMail.rawValue
		navigationItem.title = "SignMail"
		if let textField = searchController.searchBar.value(forKey: "searchField") as? UITextField
		{
			textField.isEnabled = true
		}
		if #available(iOS 11.0, *) {
			navigationItem.searchController = searchController
		} else {
			tableView.tableHeaderView = searchController.searchBar
		}
		fetchMessageData()
		tableView.reloadData()
		layoutNavigationBar()
	}
	
	public func showVideoCenter() {
		guard isViewLoaded else { return }
		
		sectionBar.selectedSegmentIndex = VideoListType.videoCenter.rawValue
		navigationItem.title = "Video Center".localized
		UserDefaults.standard.set(Date(), forKey: SignMailListViewController.lastViewedVideoCenterKey)
		if #available(iOS 11.0, *) {
			navigationItem.searchController = nil
		} else {
			tableView.tableHeaderView = nil
		}
		fetchChannelData()
		tableView.reloadData()
		layoutNavigationBar()
	}
	
	// MARK: Memory management
	
	override func didReceiveMemoryWarning() {
		super.didReceiveMemoryWarning()
	}
}

extension SignMailListViewController: PickPhoneViewControllerDelegate {
	func pickPhoneViewControllerDidCancel(_ viewController: PickPhoneViewController) {
		viewController.dismiss(animated: true)
	}
	
	
	func pickPhoneViewController(_ viewController: PickPhoneViewController, didPick number: String, source: SCIDialSource) {
		AnalyticsManager.shared.trackUsage(.signMailInitiationSource, properties: ["description" : "signmail_compose"])
		CallController.shared.makeOutgoingCall(to: number, dialSource: .signMail, skipToSignMail: true) { error in
			// The call window controller will handle the error
			if error == nil {
				viewController.dismiss(animated: true)
			}
		}
	}
}
