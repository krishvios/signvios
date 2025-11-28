//
//  CompositeDetailsViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 2/10/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit
/**
	A view controller and selection tuple alias for passing selection properties to the view model.
*/
typealias ViewControllerAndSelectionView = (vc: UIViewController & UIPopoverPresentationControllerDelegate, selectionView: UIView?)

@objc
class CompositeDetailsViewController: UIViewController, UITableViewDelegate  {
	
	fileprivate let callButtonImage = UIImage(named: "SolidPhoneIcon" )
	fileprivate let mailButtonImage = UIImage(named: "SolidSignMailNavBar" )
	fileprivate let viewModelIsNilMessage = "The view model is nil."
	fileprivate let contactIsNilMessage = "The contact is nil."
	fileprivate let viewModelIsNotEditable = "The view model is not editable."
	
	// MARK : - UITableView Methods and Properties

	static var _DefaultCellHeight : CGFloat = 44
	static var _DefaultSectionHeaderHeight : CGFloat = 44.0
	
	@IBOutlet var tableView : UITableView!
	
	@IBOutlet weak var headerView: UIView!
	@IBOutlet weak var titleLabel: UILabel!
	@IBOutlet weak var subtitleLabel: UILabel!
	@IBOutlet weak var imageView: UIImageView!
	@IBOutlet weak var separatorView: UIView!
	@IBOutlet var headerButtons: [RoundedButton] = []
	@IBOutlet var headerButtonLabels: [UILabel] = []
	@IBOutlet var callHeaderButtons: [RoundedButton] = []
	@IBOutlet var callHeaderButtonLabels: [UILabel] = []
	
	var navbarCustomization = NavigationBarCustomization()
	
	// Programmatic constraints
	@IBOutlet var separatorHeight: NSLayoutConstraint!
	@IBOutlet var headerHeight: NSLayoutConstraint!
	@IBOutlet var imageMinSize: NSLayoutConstraint!
	@IBOutlet var imageMaxSize: NSLayoutConstraint!
	@IBOutlet var topInsetScalesWithImage: NSLayoutConstraint!
	let minTitleFontSize: CGFloat = UIFont(for: .subheadline).pointSize
	let maxTitleFontSize: CGFloat = UIFont(for: .title1).pointSize
	let maxSubtitleFontSize: CGFloat = UIFont(for: .subheadline).pointSize
	
	var minHeaderHeight: CGFloat = 0
	var maxHeaderHeight: CGFloat = 0
	
	var observeContentSize: NSKeyValueObservation?
	
	fileprivate var registerCellTypes : RegisterCellsForCompositeDatailsTableViewController?
	
	var loadDetailsData : LoadCompositeDetails?
	
	weak var delegate : CompositeDetailsViewControllerDelegate?
	
	var viewModel : DetailsViewModel? {
		didSet{
			debugPrint(#function, "didSet")
			// NOTE: reload on set.
		}
	}
	

	
	// MARK : - Controller Life Cycle
	
    override func viewDidLoad() {
        super.viewDidLoad()
		titleLabel.text = nil
		subtitleLabel.text = nil
		separatorHeight.constant = 1.0 / UIScreen.main.scale
		headerHeight.priority = .fittingSizeLevel + 2
		updateImageConstraints()
		
		viewModel?.delegate = self
		viewModel?.updateData()
		tableView.delegate = self
		tableView.dataSource = viewModel
		
		observeContentSize = tableView!.observe(\.contentSize) { [weak self] _, _ in
			self?.updateBottomInset()
		}
		
		// Create navigation bar instance
		navbarCustomization.add(to: self)
		navbarCustomization.navigationBar.isTranslucent = true
		navbarCustomization.navigationBar.setBackgroundImage(UIImage(), for: .default)
		navbarCustomization.navigationBar.shadowImage = UIImage()
		
		configureTableView()
		
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme), name: Theme.themeDidChangeNotification, object: nil)
    }
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		tableView.reloadData()
	}
	
	override func viewWillLayoutSubviews() {
		super.viewWillLayoutSubviews()
		displayEditButton(navigationItem: navbarCustomization.navigationItem,
						  viewModel: self.viewModel)
		titleLabel.preferredMaxLayoutWidth = view.bounds.width - 16
		subtitleLabel.preferredMaxLayoutWidth = view.bounds.width - 16
	}
	
	private func displayEditButton(navigationItem: UINavigationItem, viewModel : DetailsViewModel?) {
		guard let viewModel = self.viewModel else { return }
		if viewModel.isEditable {
			navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(editNavButtonTapped(_:)))
		
		} else {
			navigationItem.rightBarButtonItem = nil
		}
	}
	
	@available(iOS 11.0, *)
	override func viewSafeAreaInsetsDidChange() {
		super.viewSafeAreaInsetsDidChange()
		updateTopInset()
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		NotificationCenter.default.addObserver(self, selector: #selector(authenticatedDidChange(_:)), name: .SCINotificationAuthenticatedDidChange, object: nil)
	
		viewModel?.updateData()
		viewModel?.addObservers()
		
		navbarCustomization.willAppear(animated: animated)
		
		tableView?.reloadData()
		
		if let event = AnalyticsEvent.getEventForDetailsViewModel(viewModel) {
			AnalyticsManager.shared.trackUsage(event, properties: ["description" : self.viewModel?.contact?.isFixed == true ? "default_contact_details_viewed" : "item_details_viewed" as NSObject])
		}
		
		self.applyTheme()
		configureShortcutButtonsOnAppear()
	}

	override func viewLayoutMarginsDidChange() {
		debugPrint(#function, self.view.layoutMargins)
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		
		super.viewWillDisappear(animated)
		NotificationCenter.default.removeObserver(self, name: .SCINotificationAuthenticatedDidChange, object: nil)
		self.viewModel?.removeObservers()
		navbarCustomization.willDisappear(animated: animated)
	}

	func configureTableView() {

		// Cannot register cell types before the view model is loaded.
		self.registerCellTypes = RegisterCellsForCompositeDatailsTableViewController(tableView: tableView)
		registerCellTypes?.register()
		
		if #available(iOS 11.0, *) {
			tableView.contentInsetAdjustmentBehavior = .never
		} else {
			// TODO: Find fallback options
		}
		
	}
	
	// MARK: - Navigation Augmentation & Safe Area Layout

	@objc
	func editNavButtonTapped(_:Any?) {
		if let event = AnalyticsEvent.getEventForDetailsViewModel(viewModel) {
			AnalyticsManager.shared.trackUsage(event, properties: ["description" : "details_edit" as NSObject])
		}
		debugPrint(#function)
		guard let thisViewModel = self.viewModel else {
			debugPrint(viewModelIsNilMessage)
			return
		}
		guard let contact = thisViewModel.contact else {
			debugPrint(contactIsNilMessage)
			return
		}
		guard thisViewModel.isEditable else {
			debugPrint(viewModelIsNotEditable)
			return
		}
		
		let editDetails = CompositeEditDetailsViewController()
        let loadEditContact = LoadCompositeEditContactDetails(compositeController: editDetails,
                                                                                   contact: contact,
                                                                                   contactManager: SCIContactManager.shared,
                                                                                   blockedManager: SCIBlockedManager.shared)

		editDetails.loadDetails = loadEditContact
		loadEditContact.execute()
		editDetails.modalPresentationStyle = .overCurrentContext
        self.definesPresentationContext = true
        editDetails.modalTransitionStyle = .crossDissolve
        show(editDetails, sender: self)
	}
    
    override func show(_ vc: UIViewController, sender: Any?) {
        /**
            Instead of presenting, we want to push the edit view onto the navigation stack.  Otherwise, the tabbar is hidden or the context view will be dismissed.
        **/
        if let editDetailsVC = vc as? CompositeEditDetailsViewController {
            navigationController?.pushViewController(editDetailsVC, animated: true)
        } else {
            super.show(vc, sender: sender)
        }
    }
	
	
	//MARK: - Shortcut Buttons
	// TODO: - UPDATE Comment*
	/**
		Currently this won't work.  We will eventually need to have the buttons added to the Header only when they exist
		within the Shortcut Buttons view model item.  At the time of load/configuration we can call the delegate methods to
		customize them.
	*/
	func configureShortcutButtonsOnAppear() {
		guard let viewModel = self.viewModel else { return }
		for button in viewModel.shortcutsList.shortcuts {
			delegate?.compositeDetailsViewController(self, willDisplay: button, for: viewModel.contact)
		}
		
		// Disable shortcut buttons if we can't make calls right now
		for button in callHeaderButtons {
			button.isEnabled = CallController.shared.canMakeOutgoingCalls
			button.alpha = CallController.shared.canMakeOutgoingCalls ? 1 : 0.3
		}
		for label in callHeaderButtonLabels {
			label.alpha = CallController.shared.canMakeOutgoingCalls ? 1 : 0.3
		}
	}
	
	@IBAction
	func callShortcutTapped(sender: UIButton? ){
		debugPrint(#function, "📞")
		if let event = AnalyticsEvent.getEventForDetailsViewModel(viewModel) {
			AnalyticsManager.shared.trackUsage(event, properties: ["description" : "details_call_shortcut" as NSObject])
		}
		
		guard let viewModel = self.viewModel else {
			debugPrint("View model is nil.")
			return
		}
		
		guard let callButton = viewModel.shortcutsList.shortcuts.filter({$0 is CallShortcutButton}).first as? CallShortcutButton else { return }
		delegate?.compositeDetailsViewController(self, willSelect: callButton, for: viewModel.contact)
		callButton.action(sender: ViewControllerAndSelectionView(vc: self, selectionView: sender))
		delegate?.compositeDetailsViewController(self, didSelect: callButton, for: viewModel.contact)
	}
	
	@IBAction
	func signMailShortcutTapped(sender: UIButton?){
		debugPrint(#function, "✉️" )
		if let event = AnalyticsEvent.getEventForDetailsViewModel(viewModel) {
			AnalyticsManager.shared.trackUsage(event, properties: ["description" : "details_signmail_shortcut" as NSObject])
		}
		
		guard let viewModel = self.viewModel else {
			debugPrint("View model is nil.")
			return
		}

		guard let signMailShortcutButton = viewModel.shortcutsList.shortcuts.filter({$0 is SignMailShortcutButton }).first as? SignMailShortcutButton else { return }
		delegate?.compositeDetailsViewController(self, willSelect: signMailShortcutButton, for: viewModel.contact)
		signMailShortcutButton.action(sender: ViewControllerAndSelectionView(vc: self, selectionView: sender))
		delegate?.compositeDetailsViewController(self, didSelect: signMailShortcutButton, for: viewModel.contact)
	}

	var popOverLocation: CGPoint?
	
	
	func scrollViewDidScroll(_ scrollView: UIScrollView) {
		updateHeaderHeight()
	}
	
	func scrollViewWillEndDragging(
		_ scrollView: UIScrollView,
		withVelocity velocity: CGPoint,
		targetContentOffset: UnsafeMutablePointer<CGPoint>)
	{
		// Snap to the header being open or closed
		let percentClosed = (targetContentOffset.pointee.y + scrollView.contentInset.top) / (maxHeaderHeight - minHeaderHeight)
		if percentClosed < 0.5 {
			targetContentOffset.pointee.y = -scrollView.contentInset.top
		}
		else if percentClosed > 0.5 && percentClosed < 1.0 {
			targetContentOffset.pointee.y = -scrollView.contentInset.top + (maxHeaderHeight - minHeaderHeight)
		}
	}

	func call(numberDetail : NumberDetail, vpe: SCIVideophoneEngine, callController: CallController) {
		guard let viewModel = self.viewModel else { return }
		guard let contact = viewModel.contact else {
			callController.makeOutgoingCall(to: numberDetail.number, dialSource: viewModel.type.dialSource, name: titleLabel.text)
			return
		}
		callController.makeOutgoingCall(to: numberDetail.number, dialSource: viewModel.type.dialSource)
	}
	
	func signMail(numberDetail : NumberDetail, vpe: SCIVideophoneEngine, callController: CallController) {
		guard let viewModel = self.viewModel else { return }
		callController.makeOutgoingCall(to: numberDetail.number, dialSource: viewModel.type.dialSource, skipToSignMail: true)
	}
	
	// MARK: - Selection Helpers
	
	fileprivate func selectNumberDetail( numberDetail: NumberDetail ) {
		guard let viewModel = self.viewModel else { return }
		
		if let event = AnalyticsEvent.getEventForDetailsViewModel(viewModel) {
			let numberType = self.viewModel?.contact?.isFixed == true ? "default" : numberDetail.typeString
			AnalyticsManager.shared.trackUsage(event, properties: ["description" : "\(numberType)_dialed_from_details" as NSObject])
		}
		/**
            Delegate decides if selection is permitted in order to provide other controllers an opportunity to intercrept actions.
		*/
		guard delegate?.compositeDetailsViewController(self, shouldSelect: numberDetail, for: viewModel.contact) ?? true else { return }
		
		delegate?.compositeDetailsViewController(self, willSelect: numberDetail, for: viewModel.contact)
		
		// Default action.
		numberDetail.call.execute(name: self.titleLabel?.text ?? "Unknown", source: viewModel.type.dialSource)
		
		delegate?.compositeDetailsViewController(self, didSelect: numberDetail, for: viewModel.contact)
	}
	
	// MARK : - UITableVIewDelegate Methods
	
	var selectedIndexPath: IndexPath?

	
	
	func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		
		guard let viewModel = self.viewModel else { return }
		// Keeps track of the most recently selected indexPath
		selectedIndexPath = indexPath
		let item = viewModel.items[indexPath.section]
		var _ : ViewControllerAndSelectionView? = ViewControllerAndSelectionView(vc: self, selectionView: nil) // Set this view controller as the sender by default.
		switch item.type {
		case .numbers:
			if let numbersItem = item as? NumbersListDetailsViewModelItem {
				let numberDetail = numbersItem.numbers[indexPath.row]
				selectNumberDetail(numberDetail: numberDetail)
			}
		case .constructiveOptions:
			debugPrint(self.debugDescription, #function, "🎚", "constructive option pressed")
			if let constructiveItem = item as? ConstructiveOptionsDetailsViewModelItem {
				let subItem = constructiveItem.options[indexPath.row]
				subItem.action(sender: ViewControllerAndSelectionView(vc: self, selectionView: tableView.cellForRow(at: indexPath)))
		
				var description: String
				switch subItem.type {
				case .exportContact:
					description = "export_contact"
				case .sendSignMail:
					description = "send_signmail"
				case .addNewContact:
					description = "add_contact"
				case .addToExistingContacts:
					description = "add_to_existing"
				case .addToFavorites:
					description = "add_to_favorites"
				}
				if let event = AnalyticsEvent.getEventForDetailsViewModel(viewModel) {
					AnalyticsManager.shared.trackUsage(event, properties: ["description" : "\(description)_from_details" as NSObject])
				}
			}
		case .destructiveOptions:
			debugPrint(#function, "🎚", "destructive option pressed")
			if let destructiveitem = item as? DestructiveOptionsDetailsViewModelItem {
				let subItem = destructiveitem.options[indexPath.row]
				subItem.action(sender: ViewControllerAndSelectionView(vc: self, selectionView: tableView.cellForRow(at: indexPath)))
				
				var description: String
				switch subItem.type {
				case .blockContact:
					description = "block_contact"
				case .deleteContact:
					description = "delete_contact"
				case .blockCaller:
					description = "block_caller"
				case .removeFavorite:
					description = "remove_favorite"
				case .unblockContact:
					description = "unblock_contact"
				case .unblockCaller:
					description = "unblock_caller"
				}
				if let event = AnalyticsEvent.getEventForDetailsViewModel(viewModel) {
					AnalyticsManager.shared.trackUsage(event, properties: ["description" : "\(description)_from_details" as NSObject])
				}
			}
		default:
			debugPrint(#function, "🎚", " ⚠️ unknown pressed")
		}
		tableView.deselectRow(at: indexPath, animated: true)
	}
	
	//MARK: - Theme
	
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}

	
	@objc
	func applyTheme() {		
		self.tableView.backgroundColor = Theme.current.backgroundColor
		if #available(iOS 11.0, *) {
			self.navigationItem.largeTitleDisplayMode = .never
		}
		
		headerView.backgroundColor = Theme.current.tableCellBackgroundColor
		titleLabel.textColor = Theme.current.tableCellTextColor
		subtitleLabel.textColor = Theme.current.tableCellSecondaryTextColor
		separatorView.backgroundColor = Theme.current.tableSeparatorColor
		headerButtons.forEach {
			$0.tintColor = Theme.current.tableCellBackgroundColor
			$0.backgroundColor = Theme.current.tableCellTintColor
			$0.highlightedColor = Theme.current.tableCellTintColor.withAlphaComponent(0.33)
		}
		headerButtonLabels.forEach {
			$0.textColor = Theme.current.tableCellTintColor
		}
		
		setNeedsStatusBarAppearanceUpdate()
	}
	
	@objc
	func authenticatedDidChange(_ note: Notification?) {
		if !SCIVideophoneEngine.shared.isAuthenticated {
			presentingViewController?.dismiss(animated: true)
			navigationController?.popToRootViewController(animated: true)
		}
	}
}

// MARK: Header view implementation

extension CompositeDetailsViewController {
	/// Make sure that at the top of scrolling the header is expanded completely.
	func updateTopInset() {
		var topInsets: CGFloat = 0
		if #available(iOS 11.0, *) {
			topInsets = view.safeAreaInsets.top
		}
		
		tableView.contentInset.top = maxHeaderHeight + topInsets
		if #available(iOS 13.0, *)
		{
			tableView.verticalScrollIndicatorInsets.top = maxHeaderHeight + topInsets
		}
		else
		{
			tableView.scrollIndicatorInsets.top = maxHeaderHeight + topInsets
		}
		
		// After adjusting the top inset we need to either expand or compact the header view
		var targetContentOffset = tableView.contentOffset
		let percentClosed = (targetContentOffset.y + tableView.contentInset.top) / (maxHeaderHeight - minHeaderHeight)
		if percentClosed < 0.5 {
			targetContentOffset.y = -tableView.contentInset.top
		}
		else if percentClosed > 0.5 && percentClosed < 1.0 {
			targetContentOffset.y = -tableView.contentInset.top + (maxHeaderHeight - minHeaderHeight)
		}
		
		tableView.setContentOffset(targetContentOffset, animated: true)
	}
	
	/// Make sure we can scroll down far enough that the header is compressed completely.
	func updateBottomInset() {
		var topInsets: CGFloat = 0
		var bottomInsets: CGFloat = 0
		if #available(iOS 11.0, *) {
			topInsets = view.safeAreaInsets.top
			bottomInsets = view.safeAreaInsets.bottom
		}
		
		tableView.contentInset.bottom = max(topInsets + bottomInsets, view.frame.height - (tableView.contentSize.height + minHeaderHeight + topInsets))
	}
	
	func updateHeaderHeight() {
		headerHeight.constant = -tableView.contentOffset.y
		
		let percentOpenUnclamped = 1 - (tableView.contentOffset.y + tableView.contentInset.top) / (maxHeaderHeight - minHeaderHeight)
		let percentOpen = max(0, min(1, percentOpenUnclamped))
		
		let faded: CGFloat = 0.5 // The percent open at which the subtitle becomes hidden
		subtitleLabel.alpha = (percentOpen - faded) / (1 - faded)
		
		titleLabel.font = titleLabel.font.withSize(percentOpen * (maxTitleFontSize - minTitleFontSize) + minTitleFontSize)
		subtitleLabel.font = subtitleLabel.font.withSize(percentOpen * (maxSubtitleFontSize - 1) + 1)
	}
	
	func updateImageConstraints() {
		NSLayoutConstraint.deactivate([topInsetScalesWithImage].compactMap { $0 })
		
		let sizeDelta = imageMaxSize.constant - imageMinSize.constant
		
		// A constraint such that the distance between the image and the top of the screen is the navigation bar height
		// when the image is its max height and 0 when the image is its min height.
		let navigationBarHeight: CGFloat = 44
		var topAnchor: NSLayoutYAxisAnchor
		var topOffset: CGFloat
		if #available(iOS 11.0, *) {
			topAnchor = headerView.safeAreaLayoutGuide.topAnchor
			topOffset = 0
		}
		else {
			topAnchor = headerView.topAnchor
			topOffset = UIApplication.shared.statusBarFrame.height
		}
		topInsetScalesWithImage = topAnchor.anchorWithOffset(to: imageView.topAnchor).constraint(
				equalTo: imageView.heightAnchor,
				multiplier: navigationBarHeight / sizeDelta,
				constant: -navigationBarHeight / sizeDelta * imageMinSize.constant + topOffset)
		
		NSLayoutConstraint.activate([topInsetScalesWithImage].compactMap { $0 })
		
		// The min/max header heights need to be updated as they depend on the above constraints
		let currentTitleFont = titleLabel.font!
		let currentSubtitleFont = subtitleLabel.font!
		headerHeight.isActive = false

		titleLabel.font = titleLabel.font.withSize(maxTitleFontSize)
		subtitleLabel.font = subtitleLabel.font.withSize(maxSubtitleFontSize)
		maxHeaderHeight = headerView.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize).height
		
		titleLabel.font = titleLabel.font.withSize(minTitleFontSize)
		subtitleLabel.font = subtitleLabel.font.withSize(1)
		minHeaderHeight = headerView.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize).height
		
		titleLabel.font = currentTitleFont
		subtitleLabel.font = currentSubtitleFont
		headerHeight.isActive = true
		
		updateTopInset()
		updateBottomInset()
	}
}

// MARK: - DetailsViewModelDelegate Methods

extension CompositeDetailsViewController {
	
	func detailsViewModel(_: DetailsViewModel, hasUpdated name: String) {
		debugPrint(#file, #function)
		titleLabel.text = name
		updateImageConstraints()
	}
	
	func detailsViewModel(_: DetailsViewModel, hasUpdated company: String?) {
		debugPrint(#file, #function)
		subtitleLabel.text = company
		updateImageConstraints()
	}
	
	func detailsViewModel(_: DetailsViewModel, hasUpdated photo: UIImage?) {
		debugPrint(#file, #function)
		imageView.image = photo
	}

	func detailsViewModel(_: DetailsViewModel, missing data: Any?) {
		if (data as? SCIContact) != nil {
			// Contact may have been deleted.  Dismiss view controller.
			self.navigationController?.popToRootViewController(animated: true)
			self.dismiss(animated: true) {
				debugPrint("Dismissing CompositeDetailsViewController due to the view model missing its contact.")
			}
		}
	}
}

enum Action {
	case call
	case signMail
}

extension CompositeDetailsViewController: ShortcutButtonDelegate {
	func shortcutButton(_ shortcutButton: ShortcutButton, shouldSelect numberDetail: NumberDetail) -> Bool {
		return delegate?.compositeDetailsViewController(self, shouldSelectShortcut: shortcutButton.type, numberDetail: numberDetail, for: viewModel?.contact) ?? true
	}
}

@available(iOS 11.0, *)
struct TableViewCustomAppearanceProperties {
	
	var contentInsetAdjustmentBehavior : UIScrollView.ContentInsetAdjustmentBehavior?
	
	func applyProperties(to tableView: UITableView){
		if let contentInsetAdjustmentBehavior = self.contentInsetAdjustmentBehavior {
			tableView.contentInsetAdjustmentBehavior = contentInsetAdjustmentBehavior
		}
	}
}

extension CompositeDetailsViewController: CompositeEditDetailsViewControllerDelegate {
	func compositeEditDetailsViewController(_ compositeEditDetailsViewController: CompositeEditDetailsViewController, willAdd modelItem: DetailsViewModelItem) -> DetailsViewModelItem? {
		debugPrint(#function)
		return nil
	}
	

	func compositeEditDetailsViewController(_ compositeEditDetailsViewController: CompositeEditDetailsViewController, contactAddedToViewModel: DetailsViewModel) {
		
	}
}

extension CompositeDetailsViewController: UIPopoverPresentationControllerDelegate {
	func prepareForPopoverPresentation(_ popoverPresentationController: UIPopoverPresentationController) {
		debugPrint(#function)
		
		popoverPresentationController.permittedArrowDirections = [.any]
		
		guard let selectedIndexPath = self.selectedIndexPath,
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
