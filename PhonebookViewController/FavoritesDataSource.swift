//
//  ContactsDataSource.swift
//  ntouch
//
//  Created by Dan Shields on 8/6/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// A data source that remains up-to-date with sorenson favorites.
class FavoritesDataSource: NSObject {
	private let tableView: UITableView
	private var favorites: [SCIContact] = []
	
	public init(tableView: UITableView) {
		self.tableView = tableView
		super.init()
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyFavoritesChanged), name: .SCINotificationFavoritesChanged, object: nil)
		
		// If we get a core offline error, an edit operation may have failed, so reload
		NotificationCenter.default.addObserver(self, selector: #selector(notifyFavoritesChanged), name: .SCINotificationCoreOfflineGenericError, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyFavoritesChanged), name: .SCINotificationFavoriteListError, object: nil)

		reload()
	}
	
	private func reload() {
		favorites = (SCIContactManager.shared.compositeFavorites() as? [SCIContact]) ?? []
		if tableView.dataSource === self {
			tableView.reloadData()
		}
	}
	
	@objc func notifyFavoritesChanged(_ note: Notification) {
		reload()
	}
	
	public func favoriteForRow(at indexPath: IndexPath) -> SCIContact? {
		return favorites[indexPath.row]
	}
	
	public func deleteFavorite(_ favorite: SCIContact) {
		guard let indexPath = favorites.firstIndex(of: favorite).map({ IndexPath(row: $0, section: 0) }) else { return }
		
		var phoneType: SCIContactPhone = .none
		if favorite.homeIsFavorite {
			phoneType = .home
		} else if favorite.workIsFavorite {
			phoneType = .work
		} else if favorite.cellIsFavorite {
			phoneType = .cell
		}
		
		if let contact = SCIContactManager.shared.contact(forFavorite: favorite) {
			contact.setIsFavoriteFor(contactPhone: phoneType, isFavorite: false)
			SCIContactManager.shared.updateContact(contact)
		}
		else if let contact = SCIContactManager.shared.ldaPcontact(forFavorite: favorite) {
			SCIContactManager.shared.removeLDAPContact(fromFavorites: contact, phoneType: phoneType)
		}
		else {
			SCIContactManager.shared.removeFavorite(favorite)
		}
		
		favorites.remove(at: indexPath.row)
		tableView.deleteRows(at: [indexPath], with: .fade)
	}
}

extension FavoritesDataSource: UITableViewDataSource {
	func numberOfSections(in tableView: UITableView) -> Int {
		return 1
	}
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return favorites.count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: "Contact") as? ContactCell ?? ContactCell(reuseIdentifier: "Contact")
		let favorite = favorites[indexPath.row]
		cell.setupWithContact(favorite)
		return cell
	}
	
	func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		return favorites.isEmpty ? "contacts.dsource.fav.empty".localized : nil
	}
	
	func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
		return true
	}
	
	func tableView(_ tableView: UITableView, moveRowAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
		let srcIdx = sourceIndexPath.row
		let dstIdx = destinationIndexPath.row
		guard srcIdx != dstIdx else { return }
		
		let movedFavorite = favorites.remove(at: srcIdx)
		favorites.insert(movedFavorite, at: dstIdx)
		movedFavorite.favoriteOrder = NSNumber(value: dstIdx + 1) // Favorite order is 1-based index
		
		if srcIdx <= dstIdx {
			for favorite in favorites[srcIdx..<dstIdx] {
				favorite.favoriteOrder = NSNumber(value: favorite.favoriteOrder.intValue - 1)
			}
		}
		else {
			for favorite in favorites[dstIdx+1...srcIdx] {
				favorite.favoriteOrder = NSNumber(value: favorite.favoriteOrder.intValue + 1)
			}
		}
		
		SCIContactManager.shared.favoritesListSet()
		AnalyticsManager.shared.trackUsage(.favorites, properties: ["description" : "item_reordered" as NSObject])
	}
}

extension FavoritesDataSource: UITableViewDataSourcePrefetching {
	func tableView(_ tableView: UITableView, prefetchRowsAt indexPaths: [IndexPath]) {
		// Ask the photo manager to fetch and cache the photo before we show the contact image
		for favorite in indexPaths.map({ favorites[$0.row] }) {
			PhotoManager.shared.fetchPhoto(for: favorite) { _, _ in }
		}
	}
}
