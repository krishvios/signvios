//
//  SimpleViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/17/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//
let kDefaultCellHeight : CGFloat = 44

import Foundation
open class MenuItemViewController : UIViewController, HasGenericTableView {
	typealias Item = MenuItem
	typealias Cell = UITableViewCell

	var items: [MenuItem] = [MenuItem]()
	
	override open func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
	}
	override open func viewDidLoad() {
		super.viewDidLoad()
		
	}
	
	public var registerCells: RegisterCellsForTableView?
	
	
	// MARK: - TableViewDelegate & TableViewDataSource
	public func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int { return items.count }
	
	public func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let item = items[indexPath.row]
		guard let cell = item.onLayout(tableView: tableView, controller: self) else {fatalError()}
		return cell
	}
	public func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: true)
		
		let item = items[indexPath.row]
		item.onSelect(tableView: tableView, controller: self)
	}
	public func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
		let item = items[indexPath.row]
		return item.height ?? kDefaultCellHeight
	}
	public func tableView(_ tableView: UITableView, estimatedHeightForRowAt indexPath: IndexPath) -> CGFloat {
		let item = items[indexPath.row]
		return item.height ?? kDefaultCellHeight
	}
}
