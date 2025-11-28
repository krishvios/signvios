//
//  DetailsHeaderView.swift
//  ntouch
//
//  Created by Dan Shields on 8/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

@objc protocol ContactDetailsHeaderViewDelegate: class {
	func headerViewReceivedCallAction(_ headerView: DetailsHeaderView, from sender: UIView?)
	func headerViewReceivedSignmailAction(_ headerView: DetailsHeaderView, from sender: UIView?)
	func headerViewReceivedFavoriteAction(_ headerView: DetailsHeaderView, from sender: UIView?)
	func headerViewDidChangeHeightCharacteristics(_ headerView: DetailsHeaderView)
}

@IBDesignable
class DetailsHeaderView: UIView {
	
	@IBOutlet public weak var delegate: ContactDetailsHeaderViewDelegate?
	
	@IBOutlet private var titleLabel: UILabel!
	@IBOutlet private var subtitleLabel: UILabel!
	@IBOutlet private var imageView: UIImageView!
	
	@IBOutlet private var callButton: RoundedButton!
	@IBOutlet private var signmailButton: RoundedButton!
	@IBOutlet private var favoriteButton: RoundedButton!
	@IBOutlet private var callLabel: UILabel!
	@IBOutlet private var signmailLabel: UILabel!
	@IBOutlet private var favoriteLabel: UILabel!
	@IBOutlet private var separatorView: UIView!
	
	@IBOutlet private var separatorHeight: NSLayoutConstraint!
	@IBOutlet private var imageMinSizeConstraint: NSLayoutConstraint!
	@IBOutlet private var imageMaxSizeConstraint: NSLayoutConstraint!
	@IBOutlet private var topInsetScalesWithImage: NSLayoutConstraint!
	private var heightConstraint: NSLayoutConstraint!
	private let minTitleFontSize: CGFloat = UIFont(for: .subheadline).pointSize
	private let maxTitleFontSize: CGFloat = UIFont(for: .title1).pointSize
	private let maxSubtitleFontSize: CGFloat = UIFont(for: .subheadline).pointSize
	
	public var minHeight: CGFloat = 0
	public var maxHeight: CGFloat = 0
	public var maxImageSize: CGSize {
		return CGSize(width: imageMaxSizeConstraint.constant, height: imageMaxSizeConstraint.constant)
	}
	
	public var percentOpen: CGFloat = 1.0 {
		didSet {
			updateHeight()
		}
	}
	
	public var title: String? {
		didSet { titleLabel?.text = title; DispatchQueue.main.async { self.updateImageConstraints() } }
	}
	
	public var subtitle: String? {
		didSet { subtitleLabel?.text = subtitle; DispatchQueue.main.async { self.updateImageConstraints() } }
	}
	
	public var image: UIImage? {
		didSet { imageView?.image = image }
	}
	
	public var isCallHidden: Bool {
		get { return callButton.superview!.isHidden }
		set { callButton.superview!.isHidden = newValue; DispatchQueue.main.async { self.updateImageConstraints() } }
	}
	
	public var isSignMailHidden: Bool {
		get { return signmailButton.superview!.isHidden }
		set { signmailButton.superview!.isHidden = newValue; DispatchQueue.main.async { self.updateImageConstraints() } }
	}
	
	public var isFavoriteHidden: Bool {
		get { return favoriteButton.superview!.isHidden }
		set { favoriteButton.superview!.isHidden = newValue; DispatchQueue.main.async { self.updateImageConstraints() } }
	}
	
	var contentView: UIView!
	override init(frame: CGRect) {
		super.init(frame: frame)
		commonInit()
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		commonInit()
	}
	
	private func commonInit() {
		let bundle = Bundle(for: type(of: self))
		let nib = UINib(nibName: "DetailsHeaderView", bundle: bundle)
		if let view = nib.instantiate(withOwner: self, options: nil).first as? UIView {
			view.frame = bounds
			self.addSubview(view)
			contentView = view
		}
		else {
			fatalError("Could not load header view")
		}
		
		heightConstraint = heightAnchor.constraint(equalToConstant: 289.5)
		heightConstraint.priority = .fittingSizeLevel - 1
		NSLayoutConstraint.activate([heightConstraint])
		
		contentView.backgroundColor = nil
		separatorHeight.constant = 1.0 / UIScreen.main.scale
		titleLabel.text = title
		subtitleLabel.text = subtitle
		imageView.image = image
		applyTheme()
		updateImageConstraints()
		
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme), name: Theme.themeDidChangeNotification, object: nil)
	}
	
	func updateImageConstraints() {
		titleLabel.preferredMaxLayoutWidth = bounds.width - 16
		subtitleLabel.preferredMaxLayoutWidth = bounds.width - 16
		
		NSLayoutConstraint.deactivate([topInsetScalesWithImage].compactMap { $0 })
		
		let sizeDelta = imageMaxSizeConstraint.constant - imageMinSizeConstraint.constant
		
		// A constraint such that the distance between the image and the top of the screen is the navigation bar height
		// when the image is its max height and 0 when the image is its min height.
		let navigationBarHeight: CGFloat = 44
		var topAnchor: NSLayoutYAxisAnchor
		var topOffset: CGFloat
		if #available(iOS 11.0, *) {
			topAnchor = safeAreaLayoutGuide.topAnchor
			topOffset = 0
		}
		else {
			topAnchor = self.topAnchor
			topOffset = UIApplication.shared.statusBarFrame.height
		}
		topInsetScalesWithImage = topAnchor.anchorWithOffset(to: imageView.topAnchor).constraint(
			equalTo: imageView.heightAnchor,
			multiplier: navigationBarHeight / sizeDelta,
			constant: -navigationBarHeight / sizeDelta * imageMinSizeConstraint.constant + topOffset)
		
		NSLayoutConstraint.activate([topInsetScalesWithImage].compactMap { $0 })
		
		// The min/max header heights need to be updated as they depend on the above constraints
		let currentTitleFont = titleLabel.font!
		let currentSubtitleFont = subtitleLabel.font!
		
		titleLabel.font = titleLabel.font.withSize(maxTitleFontSize)
		subtitleLabel.font = subtitleLabel.font.withSize(maxSubtitleFontSize)
		maxHeight = systemLayoutSizeFitting(UIView.layoutFittingExpandedSize).height
		
		titleLabel.font = titleLabel.font.withSize(minTitleFontSize)
		subtitleLabel.font = subtitleLabel.font.withSize(1)
		minHeight = systemLayoutSizeFitting(UIView.layoutFittingCompressedSize).height
		
		titleLabel.font = currentTitleFont
		subtitleLabel.font = currentSubtitleFont
		
		updateHeight()
		delegate?.headerViewDidChangeHeightCharacteristics(self)
	}
	
	@IBAction func call(_ sender: Any) {
		delegate?.headerViewReceivedCallAction(self, from: callButton)
	}
	
	@IBAction func signmail(_ sender: Any) {
		delegate?.headerViewReceivedSignmailAction(self, from: signmailButton)
	}
	
	@IBAction func favorite(_ sender: Any) {
		delegate?.headerViewReceivedFavoriteAction(self, from: favoriteButton)
	}
	
	private func updateHeight() {
		heightConstraint.constant = minHeight + percentOpen * (maxHeight - minHeight)

		let faded: CGFloat = 0.5 // The percent open at which the subtitle becomes hidden
		subtitleLabel.alpha = (percentOpen - faded) / (1 - faded)
		
		titleLabel.font = titleLabel.font.withSize(percentOpen * (maxTitleFontSize - minTitleFontSize) + minTitleFontSize)
		subtitleLabel.font = subtitleLabel.font.withSize(percentOpen * (maxSubtitleFontSize - 1) + 1)
	}
	
	@available(iOS 11.0, *)
	override func safeAreaInsetsDidChange() {
		updateImageConstraints()
		super.safeAreaInsetsDidChange()
	}
	
	@objc func applyTheme() {
		backgroundColor = Theme.current.tableCellBackgroundColor
		titleLabel.textColor = Theme.current.tableCellTextColor
		subtitleLabel.textColor = Theme.current.tableCellSecondaryTextColor
		separatorView.backgroundColor = Theme.current.tableSeparatorColor
		
		[callButton, signmailButton, favoriteButton].compactMap { $0 }.forEach { button in
			button.tintColor = Theme.current.tableCellBackgroundColor
			button.backgroundColor = Theme.current.tableCellTintColor
			button.highlightedColor = Theme.current.tableCellTintColor.withAlphaComponent(0.5)
		}
		[callLabel, signmailLabel, favoriteLabel].compactMap { $0 }.forEach { label in
			label.textColor = Theme.current.tableCellTintColor
		}
	}
}
