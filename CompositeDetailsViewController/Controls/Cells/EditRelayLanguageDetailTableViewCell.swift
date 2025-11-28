//
//  EditRelayLanguageDetailTableViewCell.swift
//  ntouch
//
//  Created by Cody Nelson on 4/3/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

class EditRelayLanguageDetailTableViewCell: ThemedTableViewCell {
	static let identifier: String = "EditRelayLanguageDetailTableViewCell"
	
	func commonInit(){
		self.selectionStyle = .none
		self.accessoryView = switchControl
		switchControl.addTarget(self, action: #selector(switchFlipped(sender:)), for: .touchUpInside)
	}
	
	let switchControl = UISwitch()

	var item : EditRelayLanguageDetailsViewModelItem? {
		didSet{
			guard let item = self.item else { return }
			self.textLabel?.text = item.title
			switch item.editRelayLanguage {
			case SCIRelayLanguageSpanish:
				// Switch on
				guard !self.switchControl.isOn else { return }
				self.switchControl.setOn(true, animated: true)
				return
			case SCIRelayLanguageEnglish:
				// Switch off
				guard self.switchControl.isOn else { return }
				self.switchControl.setOn(false, animated: true)
				return
			default:
				break
			}
			NotificationCenter.default.post(name: Notification.Name.didEditFieldForDetails, object: nil)
		}
	}
	
	@objc
	func switchFlipped(sender: Any?){
		self.item?.editRelayLanguage = switchControl.isOn ? SCIRelayLanguageSpanish : SCIRelayLanguageEnglish
		NotificationCenter.default.post(name: .didEditFieldForDetails, object: nil)
	}
	
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: .default, reuseIdentifier: reuseIdentifier)
		commonInit()
	}
	
	required init?(coder aDecoder: NSCoder) {
		super.init(coder: aDecoder)
		commonInit()
		
	}
}
