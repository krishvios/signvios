//
//  EditNumberDetailTableViewCell.swift
//  ntouch
//
//  Created by Cody Nelson on 2/14/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

class EditNumberDetailTableViewCell: ThemedTableViewCell {

	static let cellIdentifier = "EditNumberDetailTableViewCell"
	fileprivate let numberPlaceholder = "Phone".localized
	fileprivate let noticeText : String = "⚠️"
	
	override var reuseIdentifier: String? {
		return EditNumberDetailTableViewCell.cellIdentifier
	}

	override func prepareForInterfaceBuilder() {
		super.prepareForInterfaceBuilder()
		self.typeLabel.text = ""
		self.numberField.text = nil
		self.numberField.placeholder = numberPlaceholder
	}
	
	@IBOutlet
	weak var typeLabel : UILabel!
	
	@IBOutlet
	weak var numberField: UITextField!
	
	var item: EditNumberDetail? {
		didSet{
			typeLabel.text = ""
			numberField.text = ""
			
			guard let item = self.item else {return}
			typeLabel.text = item.type.typeString
			numberField.text = FormatAsPhoneNumber(item.editNumber)
		}
	}
	
	override func didMoveToSuperview() {
		super.didMoveToSuperview()
		self.numberField.addTarget(self, action: #selector(textFieldDidChange(textField:)), for: UIControl.Event.editingChanged)
		self.selectionStyle = .none
		self.accessibilityHint = "Input a number and select 'Done' to save."
	}
	
	@IBAction
	func detailNumberCellWasTapped(_ sender: Any? ){
		debugPrint(#function)
		
	}
	
	override func awakeFromNib() {
		super.awakeFromNib()
		// Initialization code
	}
	
	lazy var noticeLabel : UILabel = {
		let label = UILabel()
		label.text = noticeText
		return label
	}()
	
	fileprivate func showNoticeIndicatorFor(textField : UITextField) {

		let noticeLabel = self.noticeLabel
		
		noticeLabel.preferredMaxLayoutWidth = 40 + textField.frame.height
		
		noticeLabel.sizeToFit()
		
		textField.rightView = noticeLabel
		textField.rightViewMode = .unlessEditing
		setNeedsLayout()
	}
	fileprivate func hideNoticeIndicatorFor(textField: UITextField) {
		textField.rightView = nil
		textField.rightViewMode = .never
		setNeedsLayout()
	}
	
	// MARK: Themes
	
	override func applyTheme() {
		super.applyTheme()
		numberField.textColor = Theme.current.tableCellTextColor
		numberField.keyboardAppearance = Theme.current.keyboardAppearance
		typeLabel.textColor = Theme.current.tableCellSecondaryTextColor
		numberField.attributedPlaceholder = NSAttributedString(
			string: numberPlaceholder,
			attributes: [.foregroundColor : Theme.current.tableCellPlaceholderTextColor])
	}
}

//MARK: Text Field Delegate

extension EditNumberDetailTableViewCell {
	@objc
	func textFieldDidChange(textField: UITextField){
		if let text = textField.text, !text.starts(with: "1") && !text.starts(with: "011") && text.count > 3 {
			textField.text = FormatAsPhoneNumber("1" + text)
		} else {
			textField.text = FormatAsPhoneNumber(textField.text)
		}

		item?.editNumber = UnformatPhoneNumber(textField.text) ?? ""

		if let item = self.item {
			if !item.isValid {
				showNoticeIndicatorFor(textField: textField)
			} else {
				hideNoticeIndicatorFor(textField: textField)
			}
		}

		// Notifies the Edit Details Controller to determine if all the values are valid.
		NotificationCenter.default.post(name: Notification.Name.didEditFieldForDetails, object: nil)
	}
}
