//
//  RecentsCollectionView.swift
//  ntouch
//
//  Created by Kevin Selman on 3/6/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class RecentSummaryView: UIView, UICollectionViewDataSource, UICollectionViewDelegate {

	@IBOutlet weak var recentsCollectionView: UICollectionView!
	var detailsItems: [ContactSupplementedDetails] = []
	let nibName = "RecentSummaryView"
	let defaultHeaderHeight = 60
	let maxDisplayedItems = 8
	static let contentHeightRatio: CGFloat = 1.0 / 3.0 // The amount of content shown when scrolled to the top
	let SCIRecentSummaryLastClearedDate = "SCIRecentSummaryLastClearedDate"
	var observeContentSize: NSKeyValueObservation? = nil
	
	// MARK: - View Lifecycle
	
	required init?(coder aDecoder: NSCoder) {
		super.init(coder: aDecoder)
		commonInit()
	}
	
	required override init(frame: CGRect) {
		super.init(frame: frame)
		commonInit()
	}
	
	func commonInit() {
		guard UIDevice.current.userInterfaceIdiom == .pad, let view = loadViewFromNib() else { return }
		view.frame = self.bounds
		self.addSubview(view)
		view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
		setupView()
		
		observeContentSize = recentsCollectionView.observe(\.contentSize, options: [.old])
		{ collectionView, change in
			DispatchQueue.main.async {
				let topInset = collectionView.frame.height - collectionView.contentSize.height
				var newBottomInset: CGFloat = 20
				if #available(iOS 11.0, *) {
					newBottomInset += collectionView.safeAreaInsets.bottom
				}

				let newTop = max(collectionView.frame.height * (1 - RecentSummaryView.contentHeightRatio), topInset - newBottomInset)

				if newTop != collectionView.contentInset.top || newBottomInset != collectionView.contentInset.bottom {
					collectionView.contentInset.top = newTop
					collectionView.contentInset.bottom = newBottomInset
					collectionView.setContentOffset(CGPoint(x: 0, y: -newTop), animated: true)
				}
			}
		}
	}
	
	func loadViewFromNib() -> UIView? {
		let bundle = Bundle(for: type(of: self))
		let view = bundle.loadNibNamed(nibName, owner: self, options: nil)?.first as! UIView
		return view;
	}
	
	override public func didMoveToSuperview() {
		super.didMoveToSuperview()
	}
	
	override func prepareForInterfaceBuilder() {
		commonInit()
	}
	
	override func layoutSubviews() {
		super.layoutSubviews()
		guard let view = recentsCollectionView else { return }
		view.collectionViewLayout.invalidateLayout()
		if let flowLayout = view.collectionViewLayout as? UICollectionViewFlowLayout {
			flowLayout.itemSize.width = view.frame.width
			flowLayout.headerReferenceSize = CGSize(width: bounds.width, height: CGFloat(defaultHeaderHeight))
			flowLayout.estimatedItemSize = CGSize(width: view.frame.width, height: 100)
		}
		
		if SCIVideophoneEngine.shared.interfaceMode == .public {
			isHidden = true
		}
	}
	
	func setupView() {
		NotificationCenter.default.addObserver(self, selector: #selector(notifyAggregateCallListChanged),
											   name: .SCINotificationAggregateCallListItemsChanged, object: SCICallListManager.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCategoryChanged),
											   name: .SCINotificationMessageCategoryChanged, object: SCIVideophoneEngine.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme),
											   name: Theme.themeDidChangeNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(interfaceModeChanged),
											   name: .SCINotificationInterfaceModeChanged, object: nil)
		
		recentsCollectionView.register(UINib(nibName: "RecentSummaryCollectionViewCell",
											 bundle: nil),
											 forCellWithReuseIdentifier: RecentSummaryCollectionViewCell.identifier)
		
		recentsCollectionView.register(UINib(nibName: "RecentSummaryCollectionViewHeader",
											 bundle: nil),
									   forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
									   withReuseIdentifier: RecentSummaryCollectionViewHeader.identifier)
		loadRecents()
	}
	
	// MARK: - Collection View Data source
	
	// Combine aggregated call history with SignMails
	// Wrap objects as RecentSummaryItem, sort by date.
	func loadRecents() {
		let lastClearedDate = UserDefaults.standard.object(forKey: SCIRecentSummaryLastClearedDate) as? Date ?? Date(timeIntervalSince1970: 0)
		
		let callItems = SCICallListManager.shared.aggregatedItems as? [SCICallListItem] ?? []
		let signMailItems = SCIVideophoneEngine.shared.messages(forCategoryId: SCIVideophoneEngine.shared.signMailMessageCategoryId) ?? []

		detailsItems = (callItems.map(RecentDetails.init) + signMailItems.map(MessageDetails.init))
			.map { (details: Details) in ContactSupplementedDetails(details: details) }
			.filter { $0.recents.first!.date > lastClearedDate }
			.sorted { $0.recents.first!.date > $1.recents.first!.date }
		
		recentsCollectionView?.isHidden = detailsItems.isEmpty
		recentsCollectionView?.reloadData()
		recentsCollectionView?.collectionViewLayout.invalidateLayout()
		
	}
	
	public func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
		return min(detailsItems.count, maxDisplayedItems)
	}
	
	public func collectionView(_ collectionView: UICollectionView, viewForSupplementaryElementOfKind kind: String, at indexPath: IndexPath) -> UICollectionReusableView {
	
		if (kind == UICollectionView.elementKindSectionHeader) {
			let headerView = collectionView.dequeueReusableSupplementaryView(ofKind: kind, withReuseIdentifier: RecentSummaryCollectionViewHeader.identifier, for: indexPath) as! RecentSummaryCollectionViewHeader
			headerView.visualEffectView.effect = UIBlurEffect(style: Theme.current.recentsSummaryBlurStyle)
			headerView.visualEffectView.backgroundColor = Theme.current.recentsSummaryBackgroundColor
			headerView.clearButton.tintColor = Theme.current.recentsSummaryTextColor
			headerView.clearButton.addTarget(self, action: #selector(clearButtonTapped(sender:)), for: .touchUpInside)
			return headerView
		}
		
		return UICollectionReusableView()

	}
	
	public func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell{
		let cell = collectionView.dequeueReusableCell(withReuseIdentifier: RecentSummaryCollectionViewCell.identifier, for: indexPath) as! RecentSummaryCollectionViewCell
		cell.details = detailsItems[indexPath.row]
		return cell
	}
	
	public func numberOfSections(in collectionView: UICollectionView) -> Int {
		return 1
	}
	
	public func collectionView(_ collectionView: UICollectionView, didHighlightItemAt indexPath: IndexPath) {
		let cell = recentsCollectionView.cellForItem(at: indexPath) as! RecentSummaryCollectionViewCell
		
		UIView.animate(withDuration: 0.1,
					   delay: 0,
					   options: .allowUserInteraction,
					   animations: {
			cell.visualEffectView.backgroundColor = Theme.current.recentsSummaryHighlightedColor
		}, completion: nil)
	}
	
	public func collectionView(_ collectionView: UICollectionView, didUnhighlightItemAt indexPath: IndexPath) {
		let cell = recentsCollectionView.cellForItem(at: indexPath) as! RecentSummaryCollectionViewCell
		
		UIView.animate(withDuration: 0.1,
					   delay: 0,
					   options: .allowUserInteraction,
					   animations: {
			cell.visualEffectView.backgroundColor = Theme.current.recentsSummaryBackgroundColor
		}, completion: nil)
	}
	
	// MARK: - UICollectionView Delegate
	
	public func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
		let item = detailsItems[indexPath.row]
		
		if let message = (item.baseDetails as? MessageDetails)?.message {
			SCIVideoPlayer.sharedManager.playMessage(message, withCompletion: nil)
		}
		else if let callListItem = (item.baseDetails as? RecentDetails)?.callListItem {
			CallController.shared.makeOutgoingCall(to: callListItem.phone, dialSource: .recentCalls, name: item.name)
		}
	}
	
	@IBAction func clearButtonTapped(sender: UIButton) {
		UserDefaults.standard.set(detailsItems.first!.recents.first!.date, forKey: SCIRecentSummaryLastClearedDate)
		self.loadRecents()
	}
	
	override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {

		guard let recentsView = self.recentsCollectionView else  {
			return nil
		}
			
		let touchedHeader = recentsView.visibleSupplementaryViews(ofKind: UICollectionView.elementKindSectionHeader).contains { (view) -> Bool in
			view.point(inside: convert(point, to: view), with: event)
		}

		if recentsView.indexPathForItem(at: convert(point, to: recentsView)) != nil  || touchedHeader {
			return super.hitTest(point, with: event)
		}
		else {
			return nil
		}
	}
	
	// MARK: - Notifications
	
	// SCINotificationMessageCategoryChanged
	@objc func notifyCategoryChanged(note: NSNotification) {
		loadRecents()
	}
	// SCINotificationAggregateCallListItemsChanged
	@objc func notifyAggregateCallListChanged(note: NSNotification) {
		loadRecents()
	}
	
	// SCINotificationInterfaceModeChanged
	@objc func interfaceModeChanged()
	{
		let mode = SCIVideophoneEngine.shared.interfaceMode
		switch mode {
		case .public:
			isHidden = true
		default:
			isHidden = false
		}
	}

	@objc func applyTheme() {
		recentsCollectionView.reloadData()
	}
}

extension RecentSummaryView : UICollectionViewDelegateFlowLayout {
	
}
