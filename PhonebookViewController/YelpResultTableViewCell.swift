//
//  YelpResultTableViewCell.swift
//  ntouch
//
//  Created by Dan Shields on 5/21/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

@objc class YelpResultTableViewCell: ThemedTableViewCell {
	
	@IBOutlet var cellImageView: UIImageView!
	override var imageView: UIImageView? { return cellImageView }
	@IBOutlet var name: UILabel!
	@IBOutlet var rating: UIImageView!
	@IBOutlet var reviewCount: UILabel!
	@IBOutlet var address: UILabel!
	@IBOutlet var distance: UILabel!
	@IBOutlet var categories: UILabel!
	var imageDownloadTask: URLSessionDataTask?
	var business: YelpBusiness?
	
	override func applyTheme() {
		super.applyTheme()
		name.font = Theme.current.tableCellFont
		name.textColor = Theme.current.tableCellTextColor
		
		reviewCount.font = Theme.current.tableCellSecondaryFont
		address.font = Theme.current.tableCellSecondaryFont
		distance.font = Theme.current.tableCellSecondaryFont
		categories.font = Theme.current.tableCellSecondaryFont
		
		reviewCount.textColor = Theme.current.tableCellSecondaryTextColor
		address.textColor = Theme.current.tableCellSecondaryTextColor
		distance.textColor = Theme.current.tableCellSecondaryTextColor
		categories.textColor = Theme.current.tableCellSecondaryTextColor
	}
	
	func configure(with business: YelpBusiness) {
		self.business = business
		
		separatorInset.left = name.frame.origin.x
		imageView?.layer.masksToBounds = true
		
		self.name.text = business.name
		self.rating.image = self.loadRatingImageFromRating(business.rating)
		self.reviewCount.text = "\(business.reviewCount) " + "Reviews".localized
		self.imageView?.image = nil
		
		let addressComponents = [
			business.location["address1"] as? String,
			business.location["address2"] as? String,
			business.location["address3"] as? String,
			business.location["city"] as? String
		].compactMap { $0?.treatingBlankAsNil }
		self.address.text = addressComponents.joined(separator: ", ")
		
		if business.distance != -1 {
			let formatter = MeasurementFormatter()
			formatter.numberFormatter.maximumFractionDigits = 1
			formatter.numberFormatter.roundingMode = .halfUp
			formatter.unitStyle = .medium
			formatter.unitOptions = .naturalScale
			
			// The formatter will automatically choose a unit for the user's locale.
			let distance = Measurement(value: business.distance, unit: UnitLength.meters)
			self.distance.text = formatter.string(from: distance)
		}
		else {
			self.distance.text = ""
		}
		
		self.categories.text = business.categories.map { $0.name }.joined(separator: ", ")
		
		guard let imageUrl = URL(string: business.imageURL) else {
			print("Invalid image URL")
			return
		}
		
		imageDownloadTask?.cancel()
		imageDownloadTask = URLSession.shared.dataTask(with: imageUrl) { data, _, error in
			self.imageDownloadTask = nil
			if let error = error {
				print("Error getting business image: \(error)")
				return
			}
			
			guard let data = data, let image = UIImage(data: data) else {
				print("Error getting image data")
				return
			}
			
			DispatchQueue.main.async {
				// Make sure this cell is still for this business
				guard self.business == business else { return }
				self.imageView?.image = image
			}
		}
		imageDownloadTask?.resume()
	}
	
	func loadRatingImageFromRating(_ rating: YelpRating) -> UIImage?
	{
		var res: UIImage?
		switch rating {
		case YelpRatingOne:
			res = UIImage(named: "small_1")
		case YelpRatingOneHalf:
			res = UIImage(named: "small_1_half")
		case YelpRatingTwo:
			res = UIImage(named: "small_2")
		case YelpRatingTwoHalf:
			res = UIImage(named: "small_2_half")
		case YelpRatingThree:
			res = UIImage(named: "small_3")
		case YelpRatingThreeHalf:
			res = UIImage(named: "small_3_half")
		case YelpRatingFour:
			res = UIImage(named: "small_4")
		case YelpRatingFourHalf:
			res = UIImage(named: "small_4_half")
		case YelpRatingFive:
			res = UIImage(named: "small_5")
		default:
			res = UIImage(named: "small_0")
		}
		return res
	}
}
