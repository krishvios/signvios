//
//  MenuItemViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/17/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
open class MenuItemViewController : UIViewController, HasGenericTableView {
	
	
	typealias Item = MenuItem
	typealias Cell = UITableViewCell
	
	var items: [MenuItem] = [MenuItem]()
	
	var registerCells: RegisterCellsForTableView?
	
	override open func viewWillAppear(_ animated: Bool) {
		registerCells = RegisterCellsForTableView()
		registerCells?.execute()
		super.viewWillAppear(animated)
	}
	override open func viewDidLoad() {
		super.viewDidLoad()
	}
	
	// MARK: - TableViewDelegate & TableViewDataSource
	public func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int { return items.count }
	
	public func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: "Cell", for: indexPath)
		let item = items[indexPath.row]
		item.onLayout(cell: cell, controller: self)
		return cell
	}
	public func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: true)
		guard let cell = tableView.cellForRow(at: indexPath) else { return }
		let item = items[indexPath.row]
		item.onSelect(cell: cell, controller: self)
	}
	override open func prepare(for segue: UIStoryboardSegue, sender: Any?) {
		super.prepare(for: segue, sender: sender)
		guard let identifier = segue.identifier else {return}
		
	}
}
