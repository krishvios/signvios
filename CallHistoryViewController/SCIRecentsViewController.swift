import UIKit

protocol RecentsViewControllerDelegate: NSObjectProtocol {
	func recentsViewController(_ viewController: SCIRecentsViewController, didPick number: String)
}

@objc class SCIRecentsViewController: SCITableViewController {
	
	weak var delegate: RecentsViewControllerDelegate?
	static let _toCallHistoryDetailsSegueIdentifier  = "CallHistoryToDetailsSegue"
	var selectedCallItem: SCICallListItem?
	@objc var isExplicitSearch: Bool = false
	@objc var stringToSearch: String = ""
	let searchController = UISearchController(searchResultsController: nil)
	@IBOutlet var sectionBar: UISegmentedControl!
	private var clearButton: UIBarButtonItem?
	private var deletingRecentsInProgress: Bool = false

    enum CallHistoryFilter: Int {
        case all, missed
	}

    fileprivate var callList: [SCICallListItem] = []

	override func awakeFromNib() {
		super .awakeFromNib()
		NotificationCenter.default.addObserver(self, selector: #selector(notifyAggregateCallListChanged), name: .SCINotificationAggregateCallListItemsChanged, object: SCICallListManager.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPropertyChanged), name: .SCINotificationPropertyChanged, object: SCIVideophoneEngine.shared)
	}
	
    override func viewDidLoad() {
        super.viewDidLoad()
		
		// Set up the edit and add buttons.
		// If are are in a call editing the call history should not be allowed.
		if presentingViewController == nil {
			self.navigationItem.rightBarButtonItem = editButtonItem
		}
		self.extendedLayoutIncludesOpaqueBars = true
		
        tableView.delegate = self
        tableView.dataSource = self
		sectionBar.selectedSegmentIndex = SCIDefaults.shared.recentsScope.intValue
		
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
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationContactsChanged, object: SCIContactManager.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyBlockedsChanged), name: .SCINotificationBlockedsChanged, object: SCIBlockedManager.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyDidEnterBackground), name: UIApplication.didEnterBackgroundNotification, object: nil)
		
		if !stringToSearch.isEmpty {
			searchController.searchBar.text = stringToSearch
			isExplicitSearch = true
		}
		
		didUpdate()
    }
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		searchController.searchBar.barStyle = Theme.current.searchBarStyle
		searchController.searchBar.keyboardAppearance = Theme.current.keyboardAppearance
		searchController.hidesNavigationBarDuringPresentation = true
		navigationController?.navigationBar.sizeToFit()
		navigationController?.navigationBar.isTranslucent = true
		self.title = presentingViewController == nil ? "Call History".localized : "History".localized
		navigationController?.title = "History".localized
		
		if !stringToSearch.isEmpty {
			searchController.searchBar.text = stringToSearch
			stringToSearch = String()
			isExplicitSearch = true
			didUpdate()
		}
		
		tableView.reloadData()
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		searchController.hidesNavigationBarDuringPresentation = false
		searchController.isActive = false
		setEditing(false, animated: false)
		super.viewWillDisappear(animated)
	}
	
	override func viewDidDisappear(_ animated: Bool) {
		super.viewDidDisappear(animated)
		
		clearBadge()
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
	
	// Invoked when the user touches Edit.
	override func setEditing(_ editing: Bool, animated: Bool) {
		// Updates the appearance of the Edit|Done button as necessary.
		super.setEditing(editing, animated: animated)
		navigationItem.setHidesBackButton(editing, animated: animated)
		tableView.setEditing(editing, animated: true)
		// replace the add button while editing
		if isEditing && clearButton == nil {
			clearButton = UIBarButtonItem(image: UIImage(named: "TrashCan"), style: .plain, target: self, action: #selector(clearList))
			var buttons = navigationItem.leftBarButtonItems ?? []
			buttons.append(clearButton!)
			self.navigationItem.setLeftBarButtonItems(buttons, animated: true)
			clearButton?.isEnabled = callList.count > 0
			clearButton?.accessibilityLabel = "Delete".localized
		}
		else {
			var buttons = navigationItem.leftBarButtonItems ?? []
			buttons.removeAll { $0 == clearButton }
			navigationItem.setLeftBarButtonItems(buttons, animated: true)
			clearButton = nil
		}
		AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : "edit_tapped" as NSObject])
	}
	
	func fetchCallListData() {
		if (SCICallListManager.shared.aggregatedItems != nil) {
			callList = SCICallListManager.shared.aggregatedItems.sorted {
				($0 as! SCICallListItem).date > ($1 as! SCICallListItem).date
				} as! [SCICallListItem]
		}
		if let searchString = searchController.searchBar.text, !searchString.isEmpty {
			if isExplicitSearch	{
				callList = callList.filter { $0.name.lowercased() == searchString.lowercased() || $0.phone.lowercased() == searchString.lowercased() }
			}
			else {
				callList = callList.filter { $0.name.lowercased().contains(searchString.lowercased()) || $0.phone.lowercased().contains(searchString.lowercased()) }
			}
		}
		
		if let filterScope = CallHistoryFilter(rawValue: sectionBar.selectedSegmentIndex) {
			switch filterScope {
			case .all: break
			case .missed:
				if let items = SCICallListManager.shared.items {
					callList = items.sorted {
						($0 as! SCICallListItem).date > ($1 as! SCICallListItem).date
						} as! [SCICallListItem]
					callList = callList.filter { $0.type == SCICallTypeMissed }
				}
			}
		}
	}
	
	func didUpdate()
	{
		if SCIVideophoneEngine.shared.isAuthorized {
			fetchCallListData()
			tableView.reloadData()
			SCINotificationManager.shared.didStopNetworking()
		}
	}
	
// MARK: - Table view delegate / data source

	override func numberOfSections(in tableView: UITableView) -> Int { return 1 }

	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int { return callList.count }
	
	override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
		
		var height: CGFloat! = self.tableView(tableView, viewForHeaderInSection: section)?.frame.size.height ?? 0
		height = height + 10
		return height
	}

	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		
        let cell = tableView.dequeue(SCIRecentsTableViewCell.self, for: indexPath)
		cell.configure(with: RecentDetails(callListItem: callList[indexPath.row]))
        return cell
    }
	
	override func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat
	{
		return viewController(self, noRecordsViewForFooterInSection: section)?.frame.size.height ?? 0.01
	}
	
	override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		
		if callList.count == 0 	{
			switch searchController.searchBar.selectedScopeButtonIndex {
			case CallHistoryFilter.missed.rawValue: return "You don't have any missed calls.".localized
			default: return "You don't have any calls.".localized
			}
		}
		else {
			return nil
		}
	}
	
	override func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? {
		return viewController(self, noRecordsViewForFooterInSection: section)
	}
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		
		selectedCallItem = callList[indexPath.row]
		tableView.deselectRow(at: indexPath, animated: true)
		guard let selectedCallItem = selectedCallItem else {
			return
		}
		
		if let delegate = delegate {
			delegate.recentsViewController(self, didPick: selectedCallItem.phone)
		}
		else {
			let dialSource: SCIDialSource = (selectedCallItem.dialMethod == .vrsDisconnected ? .callHistoryDisconnected : .callHistory)
			CallController.shared.makeOutgoingCall(to: selectedCallItem.phone, dialSource: dialSource, name: selectedCallItem.name)
			AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : "item_dialed_from_list" as NSObject])
		}
	}
	
	override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
		
		if let controller = segue.destination as? DetailsViewController,
			let selectedItem = self.selectedCallItem
		{
			self.title = "Call History".localized.wrapNavButtonString
			navigationController?.title = "History".localized
			controller.delegate = self
			controller.details = ContactSupplementedDetails(details: RecentDetails(callListItem: selectedItem))
		}
	}
	
	override func tableView(_ tableView: UITableView, accessoryButtonTappedForRowWith indexPath: IndexPath) {
		guard !indexPath.isEmpty else { return }
		selectedCallItem = callList[indexPath.row]
		performSegue(withIdentifier: SCIRecentsViewController._toCallHistoryDetailsSegueIdentifier,
					 sender: self)
	}
	
	@available(iOS 11.0, *)
	override func tableView(_ tableView: UITableView, leadingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
		return nil
	}
	
	@available(iOS 11.0, *)
	override func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration?
	{
		switch indexPath.section {
		case 0:
			let callListItem: SCICallListItem = callList[indexPath.row]
			
			var actions: [UIContextualAction] = []
			// Don't allow deleting if the user is in a call.
			if delegate == nil {
				// Not in a call. So create the delete swipe action.
				let deleteButton = UIContextualAction(style: .destructive, title: "Delete".localized) { (action, view, handler) in
					// iOS 12 allows user to click delete twice
					guard self.callList.indices.contains(indexPath.row) && self.callList[indexPath.row] == callListItem else { return }
					
					SCICallListManager.shared.removeItemWithoutNotification(callListItem)
					self.callList.remove(at: indexPath.row)
					tableView.deleteRows(at: [indexPath], with: UITableView.RowAnimation.fade)
					AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : "item_deleted_from_list" as NSObject])
				}
				deleteButton.backgroundColor = Theme.current.tableRowDestructiveActionBackgroundColor
				actions.append(deleteButton)
			}
			if SCIVideophoneEngine.shared.directSignMailEnabled && !callListItem.isVRSCall && !isEditing && delegate == nil
			{
				// Create the reply button.
				let replyButton = UIContextualAction(style: .normal, title: "Send\nSignMail".localized) { (action, view, handler) in
					tableView.setEditing(false, animated: true)
					self.sendSignMail(callListItem)
					AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : "send_signmail_from_list" as NSObject])
				}
				replyButton.backgroundColor = Theme.current.tableRowSecondaryActionBackgroundColor
				actions.append(replyButton)
			}
			let actionsConfig = UISwipeActionsConfiguration(actions: actions)
			actionsConfig.performsFirstActionWithFullSwipe = false
			
			return actionsConfig
		default:
			return nil
		}
	}
	
	@available(iOS, deprecated: 13.0)
	override func tableView(_ tableView: UITableView, editActionsForRowAt indexPath: IndexPath) -> [UITableViewRowAction]?
	{
		switch indexPath.section {
		case 0:
			let callListItem: SCICallListItem = callList[indexPath.row]
			
			let deleteButton = UITableViewRowAction(style: .default, title: "Delete".localized) { (action, indexPath) in
				SCICallListManager.shared.removeItemWithoutNotification(callListItem)
				self.callList.remove(at: indexPath.row)
				tableView.deleteRows(at: [indexPath], with: UITableView.RowAnimation.fade)
				AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : "item_deleted_from_list" as NSObject])
			}
			deleteButton.backgroundColor = Theme.current.tableRowDestructiveActionBackgroundColor
			
			let replyButton = UITableViewRowAction(style: .normal, title: "Send\nSignMail".localized) { (action, indexPath) in
				tableView.setEditing(false, animated: true)
				self.sendSignMail(callListItem)
				AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : "send_signmail_from_list" as NSObject])
			}
			replyButton.backgroundColor = Theme.current.tableRowSecondaryActionBackgroundColor
			
			var actions = [deleteButton]
			if SCIVideophoneEngine.shared.directSignMailEnabled && !callListItem.isVRSCall && !isEditing && delegate == nil
			{
				actions.insert(replyButton, at: 1)
			}
			
			return actions
		default:
			return nil
		}
	}
	
	override func tableView(_ tableView: UITableView, willBeginEditingRowAt indexPath: IndexPath)
	{
		
	}
	
	override func tableView(_ tableView: UITableView, didEndEditingRowAt indexPath: IndexPath?)
	{
		
	}
	
	override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
		if editingStyle == UITableViewCell.EditingStyle.delete {
			let callListItem: SCICallListItem = callList[indexPath.row]
			SCICallListManager.shared.removeItemWithoutNotification(callListItem)
			callList.remove(at: indexPath.row)
			tableView.deleteRows(at: [indexPath], with: UITableView.RowAnimation.fade)
		}
	}
	
	func sendSignMail(_ callListItem: SCICallListItem)
	{
		AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : "send_signmail" as NSObject])
		
		let dialSource: SCIDialSource = (callListItem.dialMethod == .vrsDisconnected ? .callHistoryDisconnected : .callHistory)
		
		AnalyticsManager.shared.trackUsage(.signMailInitiationSource, properties: ["description" : AnalyticsManager.pendoStringFromDialSource(dialSource: dialSource)])
		
		CallController.shared.makeOutgoingCall(to: callListItem.phone, dialSource: dialSource, skipToSignMail: true)
	}
	
	@objc func clearList() {
		var action = ""
		var description = ""
		switch sectionBar.selectedSegmentIndex {
		case CallHistoryFilter.all.rawValue:
			action = "Clear All Call History".localized
			description = "clear_all"
		case CallHistoryFilter.missed.rawValue:
			action = "Clear All Missed Calls".localized
			description = "clear_missed"
		default:
			action = "Clear Listed Calls".localized
			description = "clear_listed"
		}
		let clearActionSheet: UIAlertController = UIAlertController.init(title: nil, message: nil, preferredStyle: UIAlertController.Style.actionSheet)
		clearActionSheet.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
		clearActionSheet.addAction(UIAlertAction(title: action, style: .destructive, handler: {(alert: UIAlertAction!) in
			SCINotificationManager.shared.didStartNetworking()
			self.deletingRecentsInProgress = true
			if let searchString = self.searchController.searchBar.text, !searchString.isEmpty {
				let items = self.callList
				SCICallListManager.shared.removeItems(in: items)
			}
			else if self.searchController.searchBar.selectedScopeButtonIndex == CallHistoryFilter.missed.rawValue {
				SCICallListManager.shared.clearCalls(of: SCICallTypeMissed)
			}
			else {
				SCICallListManager.shared.clearCalls(of: SCICallTypeRecent)
			}
			self.deletingRecentsInProgress = false
			AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : description as NSObject])
			return
		}))
		
		let popover: UIPopoverPresentationController? = clearActionSheet.popoverPresentationController
		if(popover != nil)
		{
			popover?.barButtonItem = clearButton
			popover?.permittedArrowDirections = UIPopoverArrowDirection.any
		}
		
		present(clearActionSheet, animated: true, completion: nil)
	}
	
	func updateDataSharing() {
		var names: [String] = []
		var numbers: [String] = []
		
		let items = (SCICallListManager.shared.aggregatedItems as? [SCICallListItem]) ?? []
		for item in items.sorted(by: { $0.date > $1.date }) {
			names.append(ContactUtilities.displayName(for: item))
			numbers.append(item.phone)
		}
		
		let defaults = UserDefaults(suiteName: "group.sorenson.ntouch.recent-calls-extension-data-sharing")
		defaults?.set(["name": names, "unformatedNumber": numbers], forKey: "recentSummary")
	}
	
//pragma mark - Notifications
	
	@objc func notifyAggregateCallListChanged(note: NSNotification) // SCINotificationAggregateCallListItemsChanged
	{
		// Update the recent calls widget
		if SCIVideophoneEngine.shared.interfaceMode != .public {
			updateDataSharing()
		}
		
		// Because this tab is lazily loaded, notifications in viewDidLoad will not be wired up unless
		// this tab is selected. This particular one is wired up on awakeFromNib, which occurs when the
		// storyboard is loaded.  So we will check for isViewLoaded before doing updates to the view.
		
		if isViewLoaded {
			didUpdate()
		}
		updateBadge()
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
		if propertyName == SCIPropertyNameLastMissedTime
		{
			updateBadge()
		}
	}
	
	@objc func notifyDidEnterBackground(note: NSNotification)
	{
		if let tabBar = self.tabBarController as? MainTabBarController,
			tabBar.selectedIndex == tabBar.getIndexForTab(.recents)
		{
			clearBadge()
		}
	}
	
	func updateBadge()
	{
		// Update Badge on TabBar
		let count = SCINotificationManager.shared.recentCallsBadgeCount
		self.navigationController?.tabBarItem.badgeValue = count > 0 ? String(count) : nil
		SCIVideophoneEngine.shared.pulseMissedCall(count > 0)
	}
	
	func clearBadge()
	{
		// Missed calls viewed, clear badge
		let latestMissedCallTime = SCICallListManager.shared.latestMissedCallTime()
		if (latestMissedCallTime != Date.init(timeIntervalSince1970: 0)) {
			SCIVideophoneEngine.shared.setLastMissedViewTimePrimitive(latestMissedCallTime)
		}
		self.navigationController?.tabBarItem.badgeValue = nil;
		SCINotificationManager.shared.updateAppBadge()
	}
}

extension SCIRecentsViewController: UISearchResultsUpdating {
	
	func search()
	{
		fetchCallListData()
		if #available(iOS 11.0, *) {
			tableView.setContentOffset(CGPoint(x: 0, y: -tableView.adjustedContentInset.top), animated: true)
		} else {
			tableView.setContentOffset(CGPoint(x: 0, y: -tableView.contentInset.top), animated: true)
		}
		tableView.reloadData()
	}
	
	func updateSearchResults(for searchController: UISearchController) {
		search()
		
		var description: String?
		switch sectionBar.selectedSegmentIndex {
		case CallHistoryFilter.all.rawValue:
			description = "all_scope_bar"
		case CallHistoryFilter.missed.rawValue:
			description = "missed_scope_bar"
		default:
			break
		}
		
		if let description = description {
			AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : description as NSObject])
		}
	}
	
	@IBAction func sectionDidChange() {
		SCIDefaults.shared.recentsScope = sectionBar.selectedSegmentIndex as NSNumber
		search()
		
		var description: String?
		switch sectionBar.selectedSegmentIndex {
		case CallHistoryFilter.all.rawValue:
			description = "all_scope_bar"
		case CallHistoryFilter.missed.rawValue:
			description = "missed_scope_bar"
		default:
			break
		}
		
		if let description = description {
			AnalyticsManager.shared.trackUsage(.callHistory, properties: ["description" : description as NSObject])
		}
	}
}

extension SCIRecentsViewController: DetailsViewControllerDelegate {
	func detailsViewController(_ viewController: DetailsViewController, shouldPerformDefaultActionFor phoneNumber: LabeledPhoneNumber) -> Bool {
		if delegate != nil {
			delegate?.recentsViewController(self, didPick: phoneNumber.dialString)
			return false
		}
		return true
	}
}
