//
//  CMVideoDimensions+Additions.swift
//  ntouch
//
//  Created by Kevin Selman on 10/7/25.
//  Copyright © 2025 Sorenson Communications. All rights reserved.
//

import CoreMedia

extension CMVideoDimensions: Equatable {
	public static func == (a: CMVideoDimensions, b: CMVideoDimensions) -> Bool {
		return a.width == b.width && a.height == b.height
	}
}
