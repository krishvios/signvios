//
//  CompositeEditDetailsViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 2/14/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit


class CompositeEditDetailsViewController: UIViewController  {
	static let defaultSectionHeaderHeight : CGFloat = 44
	static let kCompositeDetailsToEditSegueIdentifier = "compositeDetailsToEditSegueIdentifier"
	static let kPhonebookToEditSegueIdentifier = "phonebookToEditSegueIdentifier"
	
	fileprivate let viewModelIsNilMessage = "The view model is nil."
	fileprivate let contactIsNilMessage = "The contact is nil."
	fileprivate let doneButtonIsNilMessage = "The Done Button is nil."
	fileprivate let cancelButtonIsNilMessage = "The Cancel Button is nil."
	
	let defaultSectionHeaderSize : CGSize = CGSize(width: UITableView.noIntrinsicMetric,
												   height: CompositeEditDetailsViewController.defaultSectionHeaderHeight)
	
	var tableView = UITableView(frame: CGRect.zero, style: .grouped)
	
	var viewModel : DetailsViewModel?
	
	var registerCellTypes : RegisterCellsForCompositeDatailsTableViewController?
	
	var loadDetails: LoadCompositeEditDetails?
	
	let navbarCustomization = NavigationBarCustomization()
	
	
	fileprivate var addedNewContact : Bool = false
	
	// MARK: - Button Getters
	
	fileprivate let doneButton : UIBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(doneButtonTapped(sender:)))
	
	fileprivate let cancelButton : UIBarButtonItem = UIBarButtonItem(barButtonSystemItem: .cancel, target: self, action: #selector(cancelButtonTapped(sender:)))
	
	// MARK: - Layout
	
	fileprivate func layoutTableView() {
		guard !self.view.subviews.contains(tableView) else { return }
		tableView.translatesAutoresizingMaskIntoConstraints = false
		view.addSubview(tableView)
		tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor).isActive = true
		tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor).isActive = true
		tableView.topAnchor.constraint(equalTo: view.topAnchor).isActive = true
		tableView.bottomAnchor.constraint(equalTo: view.bottomAnchor).isActive = true
	}
	
	@objc fileprivate func keyboardWillResize(_ note: Notification) {
		guard let userInfo = note.userInfo else { return }
		let keyboardTop = view.convert(view.frame, to: nil).maxY - (userInfo[UIResponder.keyboardFrameEndUserInfoKey] as! NSValue).cgRectValue.minY
		let animationCurve = UIView.AnimationCurve(rawValue: userInfo[UIResponder.keyboardAnimationCurveUserInfoKey] as! Int)!
		let duration = userInfo[UIResponder.keyboardAnimationDurationUserInfoKey] as! TimeInterval
		
		UIView.setAnimationCurve(animationCurve)
		UIView.animate(withDuration: duration, animations: {
			// Always base the additional safe area inset needed on the default safe area inset
			let defaultSafeAreaInsetBottom = self.view.safeAreaInsets.bottom - self.additionalSafeAreaInsets.bottom
			self.additionalSafeAreaInsets.bottom = max(keyboardTop - defaultSafeAreaInsetBottom, 0)
		})
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle { return Theme.current.statusBarStyle }
	
    override func viewDidLoad() {
        super.viewDidLoad()
		
		viewModel?.delegate = self
		configureTableView()
		viewModel?.updateData()
		layoutTableView()
		NotificationCenter.default.addObserver(self, selector: #selector(determineValidity), name: Notification.Name.didEditFieldForDetails , object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(keyboardWillResize(_:)), name: UIResponder.keyboardWillChangeFrameNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactRemoved), name: .SCINotificationContactRemoved, object: nil)
		
		navbarCustomization.add(to: self)
		navbarCustomization.navigationItem.leftBarButtonItem = cancelButton
		navbarCustomization.navigationItem.rightBarButtonItem = doneButton
		doneButton.isEnabled = false
    }
	
	override func viewDidLayoutSubviews() {
		super.viewDidLayoutSubviews()
		if #available(iOS 11.0, *) {
			tableView.contentInset.top = navbarCustomization.navigationBar.frame.height
		}
		else {
			tableView.contentInset.top = navbarCustomization.navigationBar.frame.height
				+ UIApplication.shared.statusBarFrame.height
		}
		determineValidity()
	}
	
	var delegate : CompositeEditDetailsViewControllerDelegate?
	
	@objc
	func determineValidity() {
		guard let viewModel = self.viewModel as? EditContactDetailsViewModel else { return }
		if viewModel.purpose == .newContact {
			doneButton.isEnabled = viewModel.isValid
		} else {
			doneButton.isEnabled = viewModel.isValid && viewModel.needsUpdate
		}
	}
    
    func dismissIfUnauthorized(){
        guard SCIVideophoneEngine.shared.isAuthenticated,
            !isBeingDismissed else {
            navigationController?.popToRootViewController(animated: true)
            self.dismiss(animated: true) {
                debugPrint("Dismissing Composite Edit Details View Controller... User is not authenticated.")
            }
            return
        }
    }
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		dismissIfUnauthorized()
		navbarCustomization.willAppear(animated: animated)
		
		viewModel?.addObservers()
		tableView.reloadData()
		applyTheme()
		
		
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		self.viewModel?.removeObservers()
		super.viewWillDisappear(animated)
		navbarCustomization.willDisappear(animated: animated)
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
	}
	
	
	
	override func viewDidDisappear(_ animated: Bool) {
		super.viewDidDisappear(animated)
		NotificationCenter.default.removeObserver(self, name: Theme.themeDidChangeNotification, object: nil)
	}
    
	
    // MARK: - Navigation
	
	@objc
	fileprivate func cancelButtonTapped(sender: Any? ) {
		debugPrint(#function)
        
        navigationController?.popViewController(animated: true)
        self.dismiss(animated: true, completion: {
            debugPrint("Dismissing : \(self)")
        })
	}
	
	@objc
	fileprivate func doneButtonTapped(sender: Any?) {
		debugPrint(#function)
		guard let viewModel = self.viewModel as? EditContactDetailsViewModel else {
			debugPrint("Something went wrong.. view model is nil or incorrect type.")
            navigationController?.popViewController(animated: true)
			dismiss(animated: true, completion: nil)
			return
		}

		if let navigationController = self.navigationController {
			viewModel.onDone()
			navigationController.popViewController(animated: true)
		}
		else {
			self.dismiss(animated: true, completion: {
				viewModel.onDone()
				debugPrint("Done! - dismissing \(self)")
			})
		}
	}
	
	// MARK : - UITableViewDelegate Methods
	
	var selectedIndexPath: IndexPath?

	
	func configureTableView(){
		
		tableView.delegate = self
		tableView.dataSource = viewModel
		let footerView = UIView()
		footerView.backgroundColor = UIColor.clear
		tableView.tableFooterView = footerView
		tableView.keyboardDismissMode = .interactive
		
		guard let _ = self.viewModel else {
			debugPrint(#function, "View Model is null!")
			return
		}
		
		if #available(iOS 11.0, *) {
			tableView.contentInsetAdjustmentBehavior = .automatic
		} else {
			// Fallback on earlier versions
		}
		
		self.registerCellTypes = RegisterCellsForCompositeDatailsTableViewController(tableView: tableView)
		registerCellTypes?.register()
	}
	
	@objc
	func applyTheme() {
		self.view.backgroundColor = Theme.current.backgroundColor
	}
	
	@objc
	func authenticatedDidChange(_ note: Notification?) {
		if !SCIVideophoneEngine.shared.isAuthenticated {
			presentingViewController?.dismiss(animated: true)
			navigationController?.popToRootViewController(animated: true)
		}
	}
	
	@objc func notifyContactRemoved(_ note: Notification) {
		// The contact we were modifying was removed. Pop ourselves off the navigation stack or dismiss modally.
		if note.object != nil && note.object as? SCIContact == self.viewModel?.contact {
			presentingViewController?.dismiss(animated: true)
				if let viewControllers = navigationController?.viewControllers, let index = viewControllers.firstIndex(of: self) {
				let count = viewControllers.endIndex - index
				for _ in 0..<count {
					navigationController?.popViewController(animated: true)
				}
			}
		}
	}
}

// MARK: - Table View Delegate Methods

extension CompositeEditDetailsViewController: UITableViewDelegate {
	
	func tableView(_ tableView: UITableView, estimatedHeightForRowAt indexPath: IndexPath) -> CGFloat {
		return 44.0
	}
	
	func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
		guard let viewModel = self.viewModel else { return CGFloat.leastNonzeroMagnitude }
		return viewModel.items[section].shouldHideSection ? CGFloat.leastNonzeroMagnitude : CompositeEditDetailsViewController.defaultSectionHeaderHeight
	}
	
	func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
		return CGFloat.leastNonzeroMagnitude
	}
	
	func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		guard let viewModel = self.viewModel else { return }
		selectedIndexPath = indexPath
		let item = viewModel.items[indexPath.section]
		let selectedCell = tableView.cellForRow(at: indexPath)
		
		switch item.type {
		case .nameAndPhoto:
			debugPrint("🌠 Name and Photo item selected.")
		case .relayLanguage:
			debugPrint("🗣 Relay Language item selected.")
			if let _ = item as? EditRelayLanguageDetailTableViewCell {
			
			}
		case .constructiveOptions:
			debugPrint(#function, "🎚", "constructive option pressed")
			if let constructiveItem = item as? ConstructiveOptionsDetailsViewModelItem {
				debugPrint(constructiveItem)
				constructiveItem.options[indexPath.row].action(sender: ViewControllerAndSelectionView(vc: self, selectionView: selectedCell))
			}
		case .destructiveOptions:
			debugPrint(#function, "🎚", "destructive option pressed")
			if let destructiveitem = item as? DestructiveOptionsDetailsViewModelItem {
				let subItem = destructiveitem.options[indexPath.row]
				subItem.action(sender: ViewControllerAndSelectionView(vc: self, selectionView: selectedCell))
			}
		default:
			debugPrint(#function, "🎚", " ⚠️ unknown pressed")
		}
		tableView.deselectRow(at: indexPath, animated: true)
	}
}

// MARK: - Details View Model Delegate Methods

extension CompositeEditDetailsViewController : DetailsViewModelDelegate {
	
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, didFinishUpdatesFor modelItems: [DetailsViewModelItem]) {
		debugPrint(#function)
		
		self.tableView.reloadData()
	}
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, updating modelItem: DetailsViewModelItem) -> DetailsViewModelItem? {
		guard let newModelItem = delegate?.compositeEditDetailsViewController(self, willAdd: modelItem) else { return nil }
		return newModelItem
	}
    func detailsViewModel(_ detailsViewModel: DetailsViewModel, missing data: Any?) {
        //Temporarily this is the check for authentication
        dismissIfUnauthorized()
    }
}

extension CompositeEditDetailsViewController : UIPickerViewDelegate {}


// MARK: UIPopoverPresentationControllerDelegate

extension CompositeEditDetailsViewController: UIPopoverPresentationControllerDelegate {
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



