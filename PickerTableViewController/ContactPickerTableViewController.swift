//
//  ContactPickerTableViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 4/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit


class ContactPickerTableViewController : ContactPickerController {
	static let storyboardIdentifier = "ContactPIckerTableViewController"
	static let storyboardName = "ContactPicker"
	static let segueIdentifier = "DetailsToPickerSegueIdentifier"
	private enum RestorationKeys: String {
		case viewControllerTitle
		case searchControllerIsActive
		case searchBarText
		case searchBarIsFirstResponder
	}
	
	private enum ExpressionKeys : String {
		case name
		case companyName
		case homePhone
		case cellPhone
		case workPhone
	}
	
	private struct SearchControllerRestorableState {
		var wasActive = false
		var wasFirstResponder = false
	}
	
	// MARK: - Properties
	
	private var searchController : UISearchController!
	
	private var resultsTableViewController : ResultsTableViewController!
	
	private var restoredState = SearchControllerRestorableState()
	
	override var delegate: ContactPickerControllerDelegate? {
		didSet {
			if let resultsTableViewController = self.resultsTableViewController {
				resultsTableViewController.delegate = delegate
			}
		}
	}
	
	// MARK: - View Life Cycle
	
	override func viewDidLoad() {
		super.viewDidLoad()
		registerCells.execute(tableView: tableView)
		
		resultsTableViewController = ResultsTableViewController()
		resultsTableViewController.tableView.delegate = self
		resultsTableViewController.delegate = delegate
		searchController = UISearchController(searchResultsController: resultsTableViewController)
		searchController.searchResultsUpdater = self
		searchController.searchBar.autocapitalizationType = .none
		self.title = "Contacts".localized
		
		
		if #available(iOS 11.0, *) {
			// iOS 11 + searchBar should reside in the navigation bar.
			navigationItem.searchController = searchController
			// Searchbar should always be visible.
			navigationItem.hidesSearchBarWhenScrolling = false
			self.navigationController?.navigationBar.prefersLargeTitles = true
			self.navigationItem.largeTitleDisplayMode = .automatic
		} else {
			// Fallback on earlier versions ( iOS 10 and earlier )
			// place search controller's search bar in the tableViewHeader.
			tableView.tableHeaderView = searchController.searchBar
		}
		searchController.delegate = self
		searchController.obscuresBackgroundDuringPresentation = false // the default is true
		searchController.searchBar.delegate = self // Monitor when the search button is tapped.
		/**
		Search presents a view controller by applying normal view controller presentation semantics.
		This means that the presentation moves up the view controller hierarchy until it finds the root
		view controller or one that defines a presentation context.
		*/
		
		/**
		Specify that this view controller determines how the search controller is presented.
		The search controller should be presented modally and match the physical size of this view controller.
		*/
		definesPresentationContext = true
		items = SCIContactManager.shared.contacts as! [SCIContact]
		self.tableView.reloadData()
		
		tableView.backgroundColor = Theme.current.backgroundColor
	}
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		if restoredState.wasActive {
			searchController.isActive = restoredState.wasActive
			restoredState.wasActive = false
			
			if restoredState.wasFirstResponder {
				searchController.searchBar.becomeFirstResponder()
				restoredState.wasFirstResponder = false
			}
		}
	}
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
	}
	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
	}
	
	@IBAction
	func closeButtonTapped(_ sender: Any?) {
		debugPrint(#function)
		navigationController?.popToRootViewController(animated: true)
		dismiss(animated: true) {
			debugPrint("Dismissing Picker..")
		}
	}
	
	// MARK: - UITableViewDelegate
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return items.count
	}

	override func  tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: false)
		debugPrint(#function)
		let selectedItem : SCIContact
		//Check to see which cell was selected
		if tableView === self.tableView {
			selectedItem = items[indexPath.row]
		} else {
			selectedItem = resultsTableViewController.items[indexPath.row]
		}
		// Perform segue to select Number
		let selectContactController = SelectContactNumberDetailController(contact: selectedItem, blockedManager: blockedManager) { [weak self] (contact, numberDetail) in
			self?.selectedContactNumberDetail = (contact, numberDetail)
		}
		selectContactController.configure()
		let selectedCell = tableView.cellForRow(at: indexPath)
		selectContactController.popoverPresentationController?.permittedArrowDirections = .any
		selectContactController.popoverPresentationController?.sourceView = selectedCell
		selectContactController.popoverPresentationController?.sourceRect = selectedCell?.bounds ?? view.bounds
		DispatchQueue.main.async {
			self.present(selectContactController, animated: true, completion: {
				debugPrint("Presenting Select Contact Controller")
			})
		}
	}
}

extension ContactPickerTableViewController: UISearchBarDelegate {
	func searchBarSearchButtonClicked(_ searchBar: UISearchBar) {
		searchBar.resignFirstResponder()
	}
}

// MARK: - UISearchController Delegate

extension ContactPickerTableViewController: UISearchControllerDelegate {
	func presentSearchController(_ searchController: UISearchController) {
		debugPrint("UISearchControllerDelegate invoked method \(#function)")
	}
	func willPresentSearchController(_ searchController: UISearchController) {
		debugPrint("UISearchControllerDelegate invoked method \(#function)")
	}
	func didPresentSearchController(_ searchController: UISearchController) {
		debugPrint("UISearchControllerDelegate invoked method \(#function)")
	}
	func willDismissSearchController(_ searchController: UISearchController) {
		debugPrint("UISearchControllerDelegate invoked method \(#function)")
	}
	func didDismissSearchController(_ searchController: UISearchController) {
		debugPrint("UISearchControllerDelegate invoked method \(#function)")
	}
}

// MARK: - UISearchResultsUpdating

extension ContactPickerTableViewController: UISearchResultsUpdating {
	
	private func findMatches(searchString: String) -> NSCompoundPredicate {
		
		var searchItemsPredicate = [NSPredicate]()
		
		// Name
		let nameExpression = NSExpression(forKeyPath: ExpressionKeys.name.rawValue)
		let searchStringExpression = NSExpression(forConstantValue: searchString)
		
		let nameSearchComparisonPredicate = NSComparisonPredicate(leftExpression: nameExpression,
																  rightExpression: searchStringExpression,
																  modifier: .direct,
																  type: .contains,
																  options: [.caseInsensitive, .diacriticInsensitive])
		searchItemsPredicate.append(nameSearchComparisonPredicate)
		
		let targetNumberExpression = NSExpression(forConstantValue: searchString)
		//HomeNumberExpression
		let homeNumberExpression = NSExpression(forKeyPath: ExpressionKeys.homePhone.rawValue)
		let homeNumberPredicate = NSComparisonPredicate(leftExpression: homeNumberExpression,
														rightExpression: targetNumberExpression,
														modifier: .direct,
														type: .contains,
														options: [.caseInsensitive, .diacriticInsensitive])
		searchItemsPredicate.append(homeNumberPredicate)
		
		//CellNumberExpression
		let cellNumberExpression = NSExpression(forKeyPath: ExpressionKeys.cellPhone.rawValue)
		let cellNumberPredicate = NSComparisonPredicate(leftExpression: cellNumberExpression,
														rightExpression: targetNumberExpression,
														modifier: .direct,
														type: .contains,
														options: [.caseInsensitive, .diacriticInsensitive])
		searchItemsPredicate.append(cellNumberPredicate)
		
		//WorkNumberExpression
		let workNumberExpression = NSExpression(forKeyPath: ExpressionKeys.workPhone.rawValue)
		let workNumberPredicate = NSComparisonPredicate(leftExpression: workNumberExpression,
														rightExpression: targetNumberExpression,
														modifier: .direct,
														type: .contains,
														options: [.caseInsensitive, .diacriticInsensitive])
		searchItemsPredicate.append(workNumberPredicate)
		
		let orMatchPredicate = NSCompoundPredicate(orPredicateWithSubpredicates: searchItemsPredicate)
		return orMatchPredicate

	}
	func updateSearchResults(for searchController: UISearchController) {
		
		let searchResults = self.items
		let whitespaceCharacterSet = CharacterSet.whitespaces
		let strippedString = searchController.searchBar.text!.trimmingCharacters(in: whitespaceCharacterSet)
		let searchItems = strippedString.components(separatedBy: " ") as [String]
		let andMatchPredicates: [NSPredicate] = searchItems.map { searchString in
			findMatches(searchString: searchString)
		}
		
		
		
		// AND expressions for each value in search string
		let finalCompoundPredicate = NSCompoundPredicate(andPredicateWithSubpredicates: andMatchPredicates)
		
		let filteredResults = searchResults.filter { finalCompoundPredicate.evaluate(with: $0) }
		
		// Apply the filtered results to the search results table
		if let resultsController = searchController.searchResultsController as? ResultsTableViewController {
			resultsController.items = filteredResults
			resultsController.tableView.reloadData()
		}
	}
}

// MARK: - UIStateRestoration

extension ContactPickerTableViewController {
	override func encodeRestorableState(with coder: NSCoder) {
		super.encodeRestorableState(with: coder)
		//Encode the view state so it can be restored later
		
		coder.encode(navigationItem.title, forKey: RestorationKeys.viewControllerTitle.rawValue)
		coder.encode(searchController.isActive, forKey: RestorationKeys.searchControllerIsActive.rawValue)
		coder.encode(searchController.searchBar.isFirstResponder, forKey: RestorationKeys.searchBarIsFirstResponder.rawValue)
		coder.encode(searchController.searchBar.text, forKey: RestorationKeys.searchBarText.rawValue)
	}
	override func decodeRestorableState(with coder: NSCoder) {
		super.decodeRestorableState(with: coder)
		
		guard let decodedTitle = coder.decodeObject(forKey: RestorationKeys.viewControllerTitle.rawValue) as? String else {
			fatalError("A title does not exist. Handle gracefully.")
		}
		navigationItem.title! = decodedTitle
		
		restoredState.wasActive = coder.decodeBool(forKey: RestorationKeys.searchControllerIsActive.rawValue)
		restoredState.wasFirstResponder = coder.decodeBool(forKey: RestorationKeys.searchBarIsFirstResponder.rawValue)
		searchController.searchBar.text = coder.decodeObject(forKey: RestorationKeys.searchBarText.rawValue) as? String
		
	}
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
}
