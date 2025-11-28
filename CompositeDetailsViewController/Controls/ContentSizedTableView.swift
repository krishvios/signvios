//
//  ContentSizedTableView.swift
//  ntouch
//
//  Created by Cody Nelson on 2/14/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

class ContentSizedTableView: UITableView {
	
	override var contentSize: CGSize {
		didSet{
			invalidateIntrinsicContentSize()
		}
	}
	override var intrinsicContentSize: CGSize {
		layoutIfNeeded()
		return CGSize(width: UIView.noIntrinsicMetric, height: contentSize.height)
	}
	
	override func reloadData() {
		super.reloadData()
		self.invalidateIntrinsicContentSize()
		self.layoutIfNeeded()
	}
}

class ContentSizedCollectionView: UICollectionView {
	override var contentSize: CGSize {
		didSet{
			invalidateIntrinsicContentSize()
		}
	}
	override var intrinsicContentSize: CGSize {
		layoutIfNeeded()
		
		return CGSize(width: UIView.noIntrinsicMetric, height: contentSize.height)
	}
}
