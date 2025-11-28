//
//  PanningViewDelegate.swift
//  ntouch
//
//  Created by Cody Nelson on 1/31/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit
protocol PanningViewDelegate : class {
	var panningView : PanningView? {get set}
	func panningView(_ panningView: PanningView, didChange state: PanningViewState )
	func panningView(_ panningView: PanningView, willChange state: PanningViewState )	
}
