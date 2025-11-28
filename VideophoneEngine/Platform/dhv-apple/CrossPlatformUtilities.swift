//
//  CrossPlatformUtilities.swift
//  ntouch Mac
//
//  Created by Daniel Shields on 2/28/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
#if os(iOS)
import UIKit
#endif
import CoreGraphics

struct CrossPlatformUtilities {
#if os(macOS)
	static let blackColor = NSColor.black.cgColor
	static let whiteColor = NSColor.white.cgColor
	static func image(named name: String) -> CGImage? {
		return NSImage(named: NSImage.Name(name))?.cgImage(forProposedRect: nil, context: nil, hints: nil)
	}
#else
	static let blackColor = UIColor.black.cgColor
	static let whiteColor = UIColor.white.cgColor
	static func image(named name: String) -> CGImage? {
		return UIImage(named: name)?.cgImage
	}
#endif
}

func assertOnQueue(_ queue: @autoclosure () -> DispatchQueue) {
	// This is an assertion to catch errors in development, so it's okay if we don't check on every version
	if #available(iOS 10.0, macOS 10.12, *) {
		dispatchPrecondition(condition: .onQueue(queue()))
	}
}
