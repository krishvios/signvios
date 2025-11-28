//
//  NetworkMonitor.swift
//  VRI
//
//  Created by Dan Shields on 9/9/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import Network
import CoreTelephony

@objc(SCINetworkMonitorDelegate)
protocol NetworkMonitorDelegate {
	func networkMonitorDidChangePath(_ networkMonitor: NetworkMonitor)
}

@objc(SCINetworkType)
enum NetworkType: Int {
	case none, wired, wifi, cellular, other
}

@objc(SCINetworkMonitor)
class NetworkMonitor: NSObject {
	
	@objc weak var delegate: NetworkMonitorDelegate?
	
	let pathMonitor = NWPathMonitor()
	@objc var bestInterfaceName: String? { return pathMonitor.currentPath.availableInterfaces.first?.name }
	@objc var bestInterfaceType: NetworkType {
		
		switch pathMonitor.currentPath.availableInterfaces.first?.type {
		case .cellular:
			return NetworkType.cellular
		case .wiredEthernet:
			return NetworkType.wired
		case .wifi:
			return NetworkType.wifi
		case .other:
			return NetworkType.other
		default:
			return NetworkType.none
		}
	}
	
	let telecomInfo = CTTelephonyNetworkInfo()
	@objc var telecomCarrier: String?
	@objc var telecomRadioType: String?
	
	override init() {
		super.init()
		if #available(iOS 13.0, *) {
			telecomInfo.delegate = self
		}
		
		telecomInfo.serviceSubscriberCellularProvidersDidUpdateNotifier = { [weak self] serviceProvider in
			guard let self = self else { return }
			self.dataNetworkUpdated()
		}
		
		dataNetworkUpdated()
		
		pathMonitor.pathUpdateHandler = { [weak self] path in
			guard let self = self else { return }
			
			DispatchQueue.main.async {
				if path.supportsIPv4 {
					print("Network path supports IPv4")
				}
				if path.supportsIPv6 {
					print("Network path supports IPv6")
				}
				if path.supportsDNS {
					print("Network path supports DNS")
				}
				
				if path.status == .satisfied {
					self.delegate?.networkMonitorDidChangePath(self)
				}
				else {
					print("Network path not satisfied")
				}
			}
		}
		
		pathMonitor.start(queue: .global())
	}
	
	func dataNetworkUpdated() {
		// If we can determine the data service provider, use that. Otherwise we'll fall back to whatever service provider we can find.
		// This is usually the one we want, but maybe if the user has dual SIM cards AND is on iOS 12 it may not be the correct network?
		// More testing is required, but that scenario is likely a *very* small portion of our users.
		let serviceIdentifier: Optional<String>
		if #available(iOS 13.0, *) {
			serviceIdentifier = telecomInfo.dataServiceIdentifier ?? telecomInfo.serviceCurrentRadioAccessTechnology?.keys.first
		} else {
			serviceIdentifier = telecomInfo.serviceCurrentRadioAccessTechnology?.keys.first
		}
		
		if let serviceIdentifier = serviceIdentifier {
			telecomRadioType = telecomInfo.serviceCurrentRadioAccessTechnology?[serviceIdentifier]
			telecomCarrier = telecomInfo.serviceSubscriberCellularProviders?[serviceIdentifier]?.carrierName
		} else {
			telecomRadioType = nil
			telecomCarrier = nil
		}
	}
}

@objc(SCINetworkMonitor)
extension NetworkMonitor: CTTelephonyNetworkInfoDelegate {
	func dataServiceIdentifierDidChange(_ identifier: String) {
		self.dataNetworkUpdated()
	}
}
