//
//  ShareLocationViewController.swift
//  ntouch
//
//  Created by Kevin Selman on 11/11/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import Foundation
import MapKit
import CoreLocation
import Contacts
import AddressBookUI

@objc
class ShareLocationViewController : UIViewController
{
	@IBOutlet weak var mapView: MKMapView!
	@IBOutlet weak var confirmButton: UIBarButtonItem!
	@IBOutlet var settingsButton: UIButton! // A button that shows when the user isn't sharing their location
	
	@objc public weak var delegate: ShareLocationDelegate?
	public var placemark: CLPlacemark? {
		didSet {
			// Disable the confirm button if an address string can't be extracted from the placemark
			confirmButton.isEnabled = (addressString != nil)
		}
	}
	
	public var addressString: String? {
		// We want just the Street, City, and ZIP. Otherwise we would just use
		// addressDictionary["FormattedAddressLines"].joined(separator: ", ")
		guard let placemark = placemark else {
			return nil
		}
		
		let address: CNPostalAddress
		if #available(iOS 11.0, *) {
			address = placemark.postalAddress!
		} else {
			let mutableAddress = CNMutablePostalAddress()
			mutableAddress.street = "\(placemark.subThoroughfare ?? "") \(placemark.thoroughfare ?? "")" // Street Address
			mutableAddress.city = placemark.locality ?? "" // City
			mutableAddress.state = placemark.administrativeArea ?? "" // State
			mutableAddress.postalCode = placemark.postalCode ?? "" // ZIP
			address = mutableAddress
		}
		return CNPostalAddressFormatter.string(from: address, style: .mailingAddress)
			.trimmingCharacters(in: CharacterSet.whitespacesAndNewlines)
			.replacingOccurrences(of: "\n", with: ", ")
	}
	
	var locationManager = CLLocationManager()
	override func viewDidLoad() {
		super.viewDidLoad()
		self.title = "Share Location".localized
		mapView.showsUserLocation = true
		mapView.userTrackingMode = .follow
		updateWithUserLocation(mapView.userLocation)
		settingsButton.isHidden = (CLLocationManager.authorizationStatus() != .denied)
		locationManager.delegate = self
		locationManager.requestWhenInUseAuthorization()
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		updateWithUserLocation(mapView.userLocation)
	}
	
	@IBAction func confirmButtonTapped(_ sender: UIButton) {
		if let addressString = addressString {
			delegate?.shareLocationViewController(self, didSelectLocation: addressString)
		}
	}
	
	@IBAction func cancelButtonTapped(_ sender: UIButton) {
		dismiss(animated: true)
	}
	
	func updateWithUserLocation(_ userLocation: MKUserLocation) {
		if let location = userLocation.location {
			CLGeocoder().reverseGeocodeLocation(location) { placemarks, error in
				self.placemark = placemarks?.first
				userLocation.title = self.addressString
				self.mapView.selectAnnotation(userLocation, animated: true)
			}
		}
	}
	
	@IBAction func openSettings(_ sender: Any) {
		guard
			let settingsURL = URL(string: UIApplication.openSettingsURLString),
		    UIApplication.shared.canOpenURL(settingsURL)
		else {
			return
		}
		
		UIApplication.shared.open(settingsURL) { success in
			print("Settings opened \(success)")
		}
	}
}

extension ShareLocationViewController: CLLocationManagerDelegate {
	func locationManager(_ manager: CLLocationManager, didChangeAuthorization status: CLAuthorizationStatus) {
		settingsButton.isHidden = (CLLocationManager.authorizationStatus() != .denied)
	}
}

extension ShareLocationViewController: MKMapViewDelegate {
	func mapView(_ mapView: MKMapView, didUpdate userLocation: MKUserLocation) {
		updateWithUserLocation(userLocation)
	}
	
	func mapView(_ mapView: MKMapView, didFailToLocateUserWithError error: Error) {
		placemark = nil
		mapView.userLocation.title = nil
	}
}

@objc
protocol ShareLocationDelegate {
	func shareLocationViewController(_ viewController: ShareLocationViewController, didSelectLocation addressString: String)
}
