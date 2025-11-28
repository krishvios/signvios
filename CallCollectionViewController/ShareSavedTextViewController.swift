//
//  ShareSavedTextViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 9/5/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import Foundation

@objc protocol ShareSavedTextViewControllerDelegate: NSObjectProtocol
{
	@objc optional func shareTextViewController(_ shareTextViewController: ShareSavedTextViewController, didPickText sharedText: String)
	@objc optional func shareTextViewControllerDidCancel(_ shareTextViewController: ShareSavedTextViewController)
}

@objc class ShareSavedTextViewController: UITableViewController, UITextFieldDelegate
{
	lazy var closeButton = UIBarButtonItem(title: "Close".localized, style: .done, target: self, action: #selector(close(_:)))
	var savedTexts: [String] = []
	@objc weak var delegate: ShareSavedTextViewControllerDelegate?
	var interpreterTexts: [String] = []
	var hasChanges = false
	var wasEmpty = false
	
	enum TableSections: Int, CaseIterable {
		case sharedTextSection
		case buttonSection
	}
	
	enum ButtonSectionRows: Int, CaseIterable {
		case addButtonRow
	}
	
	override init(style: UITableView.Style)
	{
		super.init(style: style)
	}
	
	required init?(coder aDecoder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	override func viewDidLoad()
	{
		super.viewDidLoad()
		self.tableView.register(TextInputTableViewCell.nib, forCellReuseIdentifier: TextInputTableViewCell.cellIdentifier)
		
		navigationItem.rightBarButtonItem = editButtonItem
		if presentingViewController != nil // Checking if displayed in a call or from settings
		{
			// This happens during a call.
			navigationItem.leftBarButtonItem = closeButton
		}
		
		title = "Saved Text".localized
		tableView.allowsSelectionDuringEditing = true
		
		interpreterTexts = SCIPropertyManager.shared.interpreterTexts as! [String]
		
		if interpreterTexts.isEmpty
		{
			interpreterTexts.append("")
			wasEmpty = true
			UIApplication.shared.sendAction(editButtonItem.action!, to: editButtonItem.target, from: self, for: nil)
		}
		
		NotificationCenter.default.addObserver(self, selector: #selector(saveShareTexts), name: UIApplication.didEnterBackgroundNotification, object: nil)
	}
	
	override func viewWillAppear(_ animated: Bool)
	{
		super.viewWillAppear(animated)
		AnalyticsManager.shared.trackUsage(.settings, properties: ["description" : "saved_text" as NSObject])
	}
	
	override func viewWillDisappear(_ animated: Bool)
	{
		super.viewWillDisappear(animated)

		tableView.endEditing(false) // Make sure the table has completed all edits
        saveShareTexts()
	}
    
    @objc func saveShareTexts() {
        if hasChanges {
            SCIPropertyManager.shared.interpreterTexts = interpreterTexts.filter({ !$0.isEmpty }) as [Any]
        }
    }
	
	override func didReceiveMemoryWarning()
	{
		super.didReceiveMemoryWarning()
	}
	
	@objc func close(_ sender: Any)
	{
		delegate?.shareTextViewControllerDidCancel?(self)
		dismiss(animated: true)
	}
	
	override func numberOfSections(in tableView: UITableView) -> Int
	{
		return TableSections.allCases.count
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int
	{
		switch TableSections(rawValue: section) {
		case .sharedTextSection?:
			return interpreterTexts.count
		case .buttonSection?:
			return ButtonSectionRows.allCases.count
		case .none:
			fatalError("ShareSavedTextViewController: Unknown table section!")
		}
	}
	
	func createButtonCell() -> ThemedTableViewCell {
		let cell = ThemedTableViewCell(style: .default, reuseIdentifier: "ButtonCell")
		cell.textLabel?.textAlignment = .center
		cell.accessoryType = .none
		return cell
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell
	{
		switch TableSections(rawValue: indexPath.section) {
		case .sharedTextSection?:
			let cell = tableView.dequeueReusableCell(withIdentifier: TextInputTableViewCell.cellIdentifier, for: indexPath) as! TextInputTableViewCell
			cell.textField?.delegate = self
			cell.textField?.keyboardType = .asciiCapable
			cell.textField?.textAlignment = .left
			cell.textField?.clearButtonMode = .whileEditing
			cell.textField?.tag = indexPath.row
			cell.textField?.text = interpreterTexts[indexPath.row]
			cell.textField?.autocapitalizationType = .sentences
			cell.textLabel?.isHidden = true
            
			// Disable editing messages when you click them if we are in a call.
			// You can still edit them by pressing edit.
			if presentingViewController != nil {
				cell.textField?.isEnabled = self.isEditing
			}
			
			if wasEmpty && indexPath.row == 0
			{
				wasEmpty = false
				cell.textField?.becomeFirstResponder()
			}
			return cell
			
		case .buttonSection?:
			let cell = tableView.dequeueReusableCell(withIdentifier: "ButtonCell") as? ThemedTableViewCell ?? createButtonCell()
			cell.textLabel?.text = "Add Saved Text".localized
			return cell
			
		case .none:
			fatalError("ShareSavedTextViewController: Unknown table section!")
		}
	}
	
	// Runs when selecting the edit button. Also when loading into a view.
	override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool
	{
		// Allow editing of saved messages while in editing mode.
		if TableSections(rawValue: indexPath.section) == .sharedTextSection {
			if let cell = tableView.cellForRow(at: indexPath) as? TextInputTableViewCell {
				cell.textField?.isEnabled = isEditing
			}
		}
		return (indexPath.section == TableSections.sharedTextSection.rawValue)
	}
	
	override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath)
	{
		if editingStyle == .delete && indexPath.section == TableSections.sharedTextSection.rawValue
		{
			interpreterTexts.remove(at: indexPath.row)
			hasChanges = true
			tableView.deleteRows(at: [indexPath], with: .fade)
			AnalyticsManager.shared.trackUsage(.settings, properties: ["description" : "saved_text_deleted" as NSObject])
            saveShareTexts()
		}
	}
	
	override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool
	{
		return (indexPath.section == TableSections.sharedTextSection.rawValue)
	}
	
	override func tableView(_ tableView: UITableView, shouldHighlightRowAt indexPath: IndexPath) -> Bool
	{
		return true
	}
	
	// Selecting a saved message or clicking to add a new item.
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath)
	{
		tableView.deselectRow(at: indexPath, animated: true)
		
		switch TableSections(rawValue: indexPath.section) {
		// Selecting a saved text.
		case .sharedTextSection?:
			guard !isEditing else {
				return
			}
			
			let text = self.interpreterTexts[indexPath.row]
			if let delegate = delegate, !text.isEmpty {
				delegate.shareTextViewController?(self, didPickText: text)
			}
			else {
				UIApplication.shared.sendAction(editButtonItem.action!, to: editButtonItem.target, from: self, for: nil)
				let cell = tableView.cellForRow(at: indexPath) as! TextInputTableViewCell
				cell.textField?.becomeFirstResponder()
			}
			
		// Adding a new saved text.
		case .buttonSection?:
			guard interpreterTexts.count < Int(SCIPropertyManager.shared.maxInterpreterTexts) else {
				Alert("Max custom shared texts".localized, "share.save.text.max.text.reached".localized)
				break
			}
			
			interpreterTexts.append("")
			hasChanges = true
			let row = interpreterTexts.endIndex - 1
			let path = IndexPath(row: row, section: TableSections.sharedTextSection.rawValue)
			tableView.insertRows(at: [path], with: .fade)
			
			if !tableView.isEditing
			{
				UIApplication.shared.sendAction(editButtonItem.action!, to: editButtonItem.target, from: self, for: nil)
			}
			let cell = tableView.cellForRow(at: path) as! TextInputTableViewCell
			
			// Without this there is an issue if you create a new text while already in edit more.
			cell.textField?.isEnabled = self.isEditing
			
			cell.textField?.becomeFirstResponder()
			
		case .none:
			fatalError("ShareSavedTextViewController: Unknown table section!")
		}
	}
	
	func textFieldShouldReturn(_ textField: UITextField) -> Bool
	{
		textField.resignFirstResponder()
		setEditing(false, animated: true)
		navigationItem.rightBarButtonItem = editButtonItem
		return false
	}
	
	func textField(_ textField: UITextField, shouldChangeCharactersIn range: NSRange, replacementString string: String) -> Bool
	{
		let newLength = (textField.text?.count ?? 0) + string.count - range.length
		return newLength <= 88
	}
	
	func textFieldDidEndEditing(_ textField: UITextField)
	{
		if textField.tag < interpreterTexts.endIndex
		{
			let text = textField.text?.trimmingUnsupportedCharacters()
			SCIPropertyManager.shared.removeInterpreterText(interpreterTexts[textField.tag])
			SCIPropertyManager.shared.addInterpreterText(text)
			interpreterTexts[textField.tag] = text!
			hasChanges = true
			AnalyticsManager.shared.trackUsage(.settings, properties: ["description" : "saved_text_add_edit" as NSObject])
			textField.isEnabled = self.isEditing
		}
	}
}
