//
//  DetailsViewController.swift
//  ntouch
//
//  Created by Dan Shields on 8/14/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class DetailsViewController: UIViewController {
	
	private enum Section: Int, CaseIterable {
		case recents
		case numbers
		case constructiveActions
		case destructiveActions
	}
	
	private enum ConstructiveActions: CaseIterable {
		case createNewContact
		case addToContact
		case sendSignMail
		case favorite
		case unblock
		case saveToDevice
	}
	
	private enum DestructiveActions: CaseIterable {
		case unfavorite
		case block
	}
	
	@IBOutlet private var headerView: DetailsHeaderView!
	@IBOutlet private var tableView: UITableView!
	private var contentSizeObservation: NSKeyValueObservation?
	private lazy var editButton = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(edit(_:)))
	public var navbarCustomization = NavigationBarCustomization()
	
	public weak var delegate: DetailsViewControllerDelegate?
	
	/// Whether or not certain actions, specifically sending signmails or calling, should be allowed on the contact.
	public var allowsActions: Bool = true {
		didSet {
			guard isViewLoaded else { return }
			reloadHeaderView()
			reloadConstructiveActions()
			reloadDestructiveActions()
			tableView.reloadData()
		}
	}
	
	public var details: Details = CompositeDetails(compositeDetails: []) {
		didSet {
			if oldValue.delegate === self {
				oldValue.delegate = nil
			}
			details.delegate = self
		}
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle { return Theme.current.statusBarStyle }

	override func viewDidLoad() {
		super.viewDidLoad()
		tableView.register(UINib(nibName: "LabeledPhoneNumberTableViewCell", bundle: nil), forCellReuseIdentifier: "LabeledPhoneNumberTableViewCell")
		tableView.register(UINib(nibName: "RecentDetailsItemTableViewCell", bundle: nil), forCellReuseIdentifier: "RecentDetailsItemTableViewCell")
		
		// Remove empty row lines
		tableView.tableFooterView = UIView(frame: .zero)
		
		navbarCustomization.add(to: self)
		navbarCustomization.navigationBar.isTranslucent = true
		navbarCustomization.navigationBar.setBackgroundImage(UIImage(), for: .default)
		navbarCustomization.navigationBar.shadowImage = UIImage()
		if #available(iOS 13.0, *) {
			let appearance = UINavigationBarAppearance()
			appearance.configureWithTransparentBackground()
			appearance.titleTextAttributes = [.foregroundColor : Theme.current.navTextColor]
			appearance.largeTitleTextAttributes = [.foregroundColor : Theme.current.navTextColor]
			navbarCustomization.navigationBar.standardAppearance = appearance
			navbarCustomization.navigationBar.scrollEdgeAppearance = appearance
		}
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyBlockedListChanged), name: .SCINotificationBlockedsChanged, object: nil)
		
		definesPresentationContext = true
		
		// Adjust the content inset so that the user can always fully compress the header view by scrolling down.
		contentSizeObservation = tableView.observe(\.contentSize) { [weak self] (tableView, _) in
			guard let self = self else { return }
			let tabBarHeight = self.tabBarController?.tabBar.bounds.height ?? 0
			let scrollableWindow = tableView.bounds.height - tabBarHeight - (self.headerView.maxHeight - self.headerView.minHeight) + 8
			
			DispatchQueue.main.async {
				// Setting this directly in the observer can cause layout issues
				tableView.contentInset.bottom = max(tabBarHeight, (scrollableWindow - tableView.contentSize.height))
			}
		}
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		
		reloadHeaderView()
		reloadRecents()
		reloadConstructiveActions()
		reloadDestructiveActions()
		tableView.reloadData()
		
		if isMovingToParent || isBeingPresented || navigationController?.isBeingPresented == true || navigationController?.isMovingToParent == true
		{
			navbarCustomization.willAppear(animated: animated)
		}
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		
		// If the pop gesture wasn't disabled before going to the edit view controller, the user could partially pop while editing the contact
		// and the whole view becomes really buggy. Re-enable the pop gesture.
		navigationController?.interactivePopGestureRecognizer?.isEnabled = true
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
		if isMovingFromParent || isBeingDismissed || navigationController?.isBeingDismissed == true || navigationController?.isMovingFromParent == true
		{
			navbarCustomization.willDisappear(animated: animated)
		}
	}
	
	private var constructiveActions: [ConstructiveActions] = []
	private func reloadConstructiveActions() {
		if CallController.shared.isCallInProgress {
			navbarCustomization.navigationItem.rightBarButtonItem = nil
			return
		}
		
		navbarCustomization.navigationItem.rightBarButtonItem = details.isEditable ? editButton : nil
		let notFavorites = details.favoritableNumbers
			.filter { !$0.attributes.contains(.favorite) }
		
		constructiveActions = ConstructiveActions.allCases.filter {
			switch $0 {
			case .addToContact: return !details.isInPhonebook && !details.isMyPhoneMember && details.numberToAddToExistingContact != nil
			case .createNewContact: return !details.isInPhonebook && !details.isMyPhoneMember && !details.phoneNumbers.isEmpty
			case .sendSignMail: return allowsActions && details.canSendSignMail && !details.phoneNumbers.isEmpty
			case .favorite: return !notFavorites.isEmpty && SCIVideophoneEngine.shared.favoritesFeatureEnabled
			case .unblock: return details.canUnblock
			case .saveToDevice: return details.canSaveToDevice
			}
		}
	}
	
	private var destructiveActions: [DestructiveActions] = []
	private func reloadDestructiveActions() {
		if CallController.shared.isCallInProgress {
			return
		}
		
		let favorites = details.favoritableNumbers
			.filter { $0.attributes.contains(.favorite) }
		
		destructiveActions = DestructiveActions.allCases.filter {
			switch $0 {
			case .unfavorite: return !favorites.isEmpty && SCIVideophoneEngine.shared.favoritesFeatureEnabled
			case .block: return details.canBlock && !details.isMyPhoneMember
			}
		}
	}
	
	enum RecentItem {
		case header(Date)
		case recent(Recent)
	}
	
	private var groupedRecents: [RecentItem] = []
	private var recentTimeLabelWidth: CGFloat = 0
	private func reloadRecents() {
		groupedRecents = Dictionary(grouping: details.recents, by: { Calendar.current.startOfDay(for: $0.date) })
			.map { kv in (date: kv.key, recents: kv.value.sorted { $0.date > $1.date }) }
			.sorted { $0.date > $1.date }
			.flatMap { group in [RecentItem.header(group.date)] + group.recents.map { RecentItem.recent($0) } }
		
		// Determine a maximum time width so we can align the labels correctly
		let label = UILabel()
		label.font = UIFont(for: .caption1)
		recentTimeLabelWidth = details.recents
			.map {
				label.text = DateFormatter.localizedString(from: $0.date, dateStyle: .none, timeStyle: .short)
				return label.intrinsicContentSize.width
			}
			.max() ?? 0
	}
	
	private func chooseNumber(
		title: String? = nil, message: String? = nil,
		phoneNumbers: [LabeledPhoneNumber], disabledDialStrings: Set<String> = [], sourceView: UIView,
		completion: @escaping (LabeledPhoneNumber?) -> Void)
	{
		guard !phoneNumbers.isEmpty else { completion(nil); return }
		
		if phoneNumbers.count == 1,
			let phoneNumber = phoneNumbers.first,
			!disabledDialStrings.contains(phoneNumber.dialString)
		{
			completion(phoneNumber)
			return
		}
		
		let viewController = UIAlertController(title: title, message: message, preferredStyle: .actionSheet)
		for phoneNumber in phoneNumbers {
			let action = UIAlertAction(
				title: "\(phoneNumber.label?.localized ?? "phone".localized): \(FormatAsPhoneNumber(phoneNumber.dialString) ?? "")",
				style: .default,
				handler: { _ in completion(phoneNumber) })
			
			action.isEnabled = !disabledDialStrings.contains(phoneNumber.dialString)
			viewController.addAction(action)
		}
		
		viewController.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel) { _ in completion(nil) })
		
		if let popoverPresentationController = viewController.popoverPresentationController {
			popoverPresentationController.sourceView = sourceView
			popoverPresentationController.sourceRect = sourceView.bounds
		}
		
		present(viewController, animated: true)
	}
	
	@IBAction func edit(_ sender: Any?) {
		AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "details_edit" as NSObject])
		guard let viewController = details.edit() else { return }
		
		// If the pop gesture isn't disabled here, the user can partially pop while editing the contact and the whole view becomes really buggy.
		// This is because we need to present over the current context. It is re-enabled in viewDidAppear.
		navigationController?.interactivePopGestureRecognizer?.isEnabled = false
		
		viewController.modalPresentationStyle = .overCurrentContext
		viewController.modalTransitionStyle = .crossDissolve
		present(viewController, animated: true)
	}
	
	@objc private func notifyBlockedListChanged() {
		reloadConstructiveActions()
		reloadDestructiveActions()
		tableView.reloadData()
	}
}

extension DetailsViewController: DetailsDelegate {
	func detailsDidChange(_ contactDetails: Details) {
		guard isViewLoaded else { return }
		
		reloadHeaderView()
		reloadConstructiveActions()
		reloadDestructiveActions()
		reloadRecents()
		tableView.reloadData()
	}
	
	func detailsWasRemoved(_ contactDetails: Details) {
		presentingViewController?.dismiss(animated: true)
		if let viewControllers = navigationController?.viewControllers, let index = viewControllers.firstIndex(of: self) {
			let count = viewControllers.endIndex - index
			for _ in 0..<count {
				navigationController?.popViewController(animated: true)
			}
		}
	}
}

// MARK: Handling the header
extension DetailsViewController: ContactDetailsHeaderViewDelegate, UIScrollViewDelegate {

	private func reloadHeaderView() {
		headerView.image = details.photo ?? ColorPlaceholderImage.getColorPlaceholderFor(
			name: details.name ?? details.companyName,
			dialString: details.phoneNumbers.first?.dialString ?? "1",
			size: headerView.maxImageSize)
		
		if var name = details.name {
			let validPhoneNumbers = details.phoneNumbers.filter({!$0.dialString.isEmpty})
			if validPhoneNumbers.count == 0 || details.isFixed {
				// localize e.g. "No Caller ID", "Customer Care"
				name = name.localized
			}
			if name.contains("No Caller ID") {
				name = name.localized
			}
			headerView.title = name
			headerView.subtitle = details.companyName
		}
		else {
			headerView.title = details.companyName ?? "No Name".localized
		}
		
		// don't show these buttons if we're in a call, if this view is shown while in a call it's part of selecting a contact/number for other reasons and these options don't make sense in that moment
		headerView.isCallHidden = !allowsActions || !(details.canCall && !details.phoneNumbers.isEmpty) || CallController.shared.isCallInProgress
		headerView.isSignMailHidden = !allowsActions || !(details.canSendSignMail && !details.phoneNumbers.isEmpty) || CallController.shared.isCallInProgress
		headerView.isFavoriteHidden = details.favoritableNumbers.isEmpty || CallController.shared.isCallInProgress
		
		if !tableView.isTracking {
			var contentOffset = tableView.contentOffset
			snapHeader(&contentOffset)
			tableView.setContentOffset(contentOffset, animated: true)
			updateHeaderHeight()
		}
	}
	
	/// Make sure that at the top of scrolling the header can be expanded completely.
	private func updateContentInsets() {
		tableView.contentInset.top = headerView.maxHeight
		tableView.scrollIndicatorInsets.top = headerView.maxHeight
		tableView.scrollIndicatorInsets.bottom = tabBarController?.tabBar.frame.height ?? 0
	}
	
	
	/// Snap the header fully open or closed, depending on the content offset
	private func snapHeader(_ contentOffset: inout CGPoint) {
		let percentOpen = (-contentOffset.y - headerView.minHeight) / (headerView.maxHeight - headerView.minHeight)
		if percentOpen > 0.5 {
			contentOffset.y = -headerView.maxHeight
		}
		else if percentOpen > 0.0 {
			contentOffset.y = -headerView.minHeight
		}
	}
	
	/// Adjust the header height while the user scrolls
	private func updateHeaderHeight() {
		
		headerView.percentOpen = max(0, min(1, (-tableView.contentOffset.y - headerView.minHeight) / (headerView.maxHeight - headerView.minHeight)))
	}
	
	func scrollViewDidScroll(_ scrollView: UIScrollView) {
		updateHeaderHeight()
	}
	
	func scrollViewWillEndDragging(
		_ scrollView: UIScrollView,
		withVelocity velocity: CGPoint,
		targetContentOffset: UnsafeMutablePointer<CGPoint>)
	{
		snapHeader(&targetContentOffset.pointee)
	}
	
	func headerViewReceivedCallAction(_ headerView: DetailsHeaderView, from sender: UIView?) {
		AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "details_call_shortcut" as NSObject])
		guard let sourceView = sender else { return }
		chooseNumber(title: "contact.choose.num.call".localized, phoneNumbers: details.phoneNumbers, sourceView: sourceView) { phoneNumber in
			guard let phoneNumber = phoneNumber else { return }
			self.details.call(phoneNumber)
		}
	}
	
	func headerViewReceivedSignmailAction(_ headerView: DetailsHeaderView, from sender: UIView?) {
		AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "details_signmail_shortcut" as NSObject])
		guard let sourceView = sender else { return }
		chooseNumber(title: "contact.choose.num.signmail".localized, phoneNumbers: details.phoneNumbers, sourceView: sourceView) { phoneNumber in
			guard let phoneNumber = phoneNumber else { return }
			self.details.sendSignMail(phoneNumber)
		}
	}
	
	func headerViewReceivedFavoriteAction(_ headerView: DetailsHeaderView, from sender: UIView?) {
		AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "details_favorite_shortcut" as NSObject])
		guard let sourceView = sender else { return }
		let favorites = details.favoritableNumbers
			.filter { $0.attributes.contains(.favorite) }
			.map { $0.dialString }
		chooseNumber(
			title: "contact.choose.num.fav".localized,
			phoneNumbers: details.favoritableNumbers,
			disabledDialStrings: Set(favorites),
			sourceView: sourceView)
		{ phoneNumber in
			guard let phoneNumber = phoneNumber else { return }
			self.details.favorite(phoneNumber)
		}
	}
	
	func headerViewDidChangeHeightCharacteristics(_ headerView: DetailsHeaderView) {
		updateContentInsets()
	}
}

extension DetailsViewController: UITableViewDataSource {
	func numberOfSections(in tableView: UITableView) -> Int {
		return Section.allCases.count
	}
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		switch Section(rawValue: section)! {
		case .recents:
			return groupedRecents.count
			
		case .numbers:
			return details.phoneNumbers.count
			
		case .constructiveActions:
			return constructiveActions.count
			
		case .destructiveActions:
			reloadDestructiveActions()
			return destructiveActions.count
		}
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		switch Section(rawValue: indexPath.section)! {
		case .recents:
			switch groupedRecents[indexPath.row] {
			case .header(let date):
				let cell = tableView.dequeueReusableCell(withIdentifier: "RecentHeaderTableViewCell") as? RecentHeaderTableViewCell
					?? RecentHeaderTableViewCell(reuseIdentifier: "RecentHeaderTableViewCell")
				cell.configure(with: date)
				cell.selectionStyle = .none
				cell.separatorInset = UIEdgeInsets(top: 0, left: UIScreen.main.bounds.width, bottom: 0, right: 0)
				return cell
				
			case .recent(let recent):
				let cell = tableView.dequeueReusableCell(withIdentifier: "RecentDetailsItemTableViewCell", for: indexPath) as! RecentDetailsItemTableViewCell
				cell.configure(with: recent, timeLabelWidth: recentTimeLabelWidth)
				cell.selectionStyle = .none
				cell.separatorInset = UIEdgeInsets(top: 0, left: UIScreen.main.bounds.width, bottom: 0, right: 0)
				return cell
			}

		case .numbers:
			let cell = tableView.dequeueReusableCell(withIdentifier: "LabeledPhoneNumberTableViewCell", for: indexPath) as! LabeledPhoneNumberTableViewCell
			cell.configure(with: details.phoneNumbers[indexPath.row])
			return cell
			
		case .constructiveActions:
			let cell = tableView.dequeueReusableCell(withIdentifier: "ConstructiveTableViewCell")
				?? ThemedTableViewCell(style: .value1, reuseIdentifier: "ConstructiveTableViewCell")

			switch constructiveActions[indexPath.row] {
			case .createNewContact:
				cell.textLabel?.text = details.isContact ? "Import Contact".localized : "Add Contact".localized
			case .addToContact:
				cell.textLabel?.text = "Add To Existing Contact".localized
			case .sendSignMail:
				cell.textLabel?.text = "Send SignMail".localized
			case .favorite:
				cell.textLabel?.text = "Add To Favorites".localized
			case .unblock:
				cell.textLabel?.text = details.isContact ? "Unblock Contact".localized : "Unblock Caller".localized
			case .saveToDevice:
				cell.textLabel?.text = "Save Contact To My Device".localized
			}
			
			return cell
			
		case .destructiveActions:
			let cell = tableView.dequeueReusableCell(withIdentifier: "DestructiveTableViewCell")
				?? DestructiveTableViewCell(style: .value1, reuseIdentifier: "DestructiveTableViewCell")
			
			switch destructiveActions[indexPath.row] {
			case .unfavorite:
				cell.textLabel?.text = "Remove Favorite".localized
			case .block:
				cell.textLabel?.text = details.isContact ? "Block Contact".localized : "Block Caller".localized
			}
			
			return cell
		}
	}
}


extension DetailsViewController: UITableViewDelegate {
	func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: true)
		switch Section(rawValue: indexPath.section)! {
		case .recents:
			break
			
		case .numbers:
			let number = details.phoneNumbers[indexPath.row]
			AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: [
				"description": "number_dialed_from_details" as NSObject,
				"name": (number.label ?? "phone") as NSObject
			])
			if delegate?.detailsViewController(self, shouldPerformDefaultActionFor: number) != false && details.canCall {
				details.call(number)
			}
			
		case .constructiveActions:
			switch constructiveActions[indexPath.row] {
			case .createNewContact:
				AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "add_contact_from_details" as NSObject])
				guard let viewController = details.createNewContact() else { return }
				viewController.modalPresentationStyle = .formSheet
				present(viewController, animated: true)
				
			case .addToContact:
				AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "add_to_existing_from_details" as NSObject])
				details.addToExistingContact(presentingViewController: self)
				
			case .sendSignMail:
				AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "send_signmail_from_details" as NSObject])
				guard let sourceView = tableView.cellForRow(at: indexPath) else { return }
				chooseNumber(title: "contact.choose.num.signmail".localized, phoneNumbers: details.phoneNumbers, sourceView: sourceView) { phoneNumber in
					guard let phoneNumber = phoneNumber else { return }
					self.details.sendSignMail(phoneNumber)
				}
				
			case .favorite:
				AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "add_to_favorites_from_details" as NSObject])
				guard let sourceView = tableView.cellForRow(at: indexPath) else { return }
				let favorites = details.favoritableNumbers
					.filter { $0.attributes.contains(.favorite) }
					.map { $0.dialString }
				chooseNumber(
					title: "contact.choose.num.fav".localized,
					phoneNumbers: details.favoritableNumbers,
					disabledDialStrings: Set(favorites),
					sourceView: sourceView)
				{ phoneNumber in
					guard let phoneNumber = phoneNumber else { return }
					self.details.favorite(phoneNumber)
				}
				
			case .unblock:
				AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "unblock_from_details" as NSObject])
				details.unblockAll()
				
			case .saveToDevice:
				AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "export_contact_from_details" as NSObject])
				details.saveToDevice { error in
					if let error = error {
						AlertWithError(error)
					} else {
						AlertWithMessage("Export Successful".localized)
					}
				}
			}
			
		case .destructiveActions:
			switch destructiveActions[indexPath.row] {
			case .block:
				AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "block_from_details" as NSObject])
				details.blockAll()
				
			case .unfavorite:
				AnalyticsManager.shared.trackUsage(details.analyticsEvent, properties: ["description": "remove_favorite_from_details" as NSObject])
				guard let sourceView = tableView.cellForRow(at: indexPath) else { return }
				let notFavorites = details.favoritableNumbers
					.filter { !$0.attributes.contains(.favorite) }
					.map { $0.dialString }
				chooseNumber(
					title: "contact.choose.num.rem.fav".localized,
					phoneNumbers: details.favoritableNumbers,
					disabledDialStrings: Set(notFavorites),
					sourceView: sourceView)
				{ phoneNumber in
					guard let phoneNumber = phoneNumber else { return }
					self.details.unfavorite(phoneNumber)
				}
			}
		}
	}

	func tableView(_ tableView: UITableView, shouldShowMenuForRowAt indexPath: IndexPath) -> Bool {
		return Section(rawValue: indexPath.section) == .numbers
	}
	
	func tableView(_ tableView: UITableView, canPerformAction action: Selector, forRowAt indexPath: IndexPath, withSender sender: Any?) -> Bool {
		return Section(rawValue: indexPath.section) == .numbers && action == #selector(copy(_:))
	}
	
	func tableView(_ tableView: UITableView, performAction action: Selector, forRowAt indexPath: IndexPath, withSender sender: Any?) {
		tableView.cellForRow(at: indexPath)?.perform(action, with: sender)
	}
	
	func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
		switch Section(rawValue: section)! {
		case .recents:
			return details.recents.isEmpty ? 0 : 16
		case .numbers:
			return details.phoneNumbers.isEmpty ? 0 : 16
		case .constructiveActions:
			return constructiveActions.isEmpty ? 0 : 16
		case .destructiveActions:
			return destructiveActions.isEmpty ? 0 : 16
		}
	}
	
	func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
		// Headers are empty and simply for spacing. There's a bug where they appear 44 pixels below where they should be so we need to make sure they don't obstruct buttons
		let header = UIView(frame: .zero)
		header.isUserInteractionEnabled = false
		return header
	}
}


// Needed for add to existing contact
extension DetailsViewController: UIPopoverPresentationControllerDelegate {
	func prepareForPopoverPresentation(_ popoverPresentationController: UIPopoverPresentationController) {
		guard let selectedIndexPath = tableView.indexPathForSelectedRow,
			let cell = tableView.cellForRow(at: selectedIndexPath) else {
				// No selected index was found.
				popoverPresentationController.sourceView = view
				popoverPresentationController.sourceRect = view.bounds
				return
		}
		
		popoverPresentationController.sourceView = cell
		popoverPresentationController.sourceRect = cell.bounds
	}
}

protocol DetailsViewControllerDelegate: class {
	func detailsViewController(_ viewController: DetailsViewController, shouldPerformDefaultActionFor phoneNumber: LabeledPhoneNumber) -> Bool
}

