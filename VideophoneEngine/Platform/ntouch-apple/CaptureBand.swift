//
//  CaptureBand.swift
//  ntouch
//
//  Created by Daniel Shields on 5/7/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import AVFoundation
import CoreMedia

struct CaptureBand {
	let frameDimensions: CMVideoDimensions
	let frameDuration: CMTime
	let minBitRate: Int
	let maxBitRate: Int

	init(width: Int,
		 height: Int,
		 frameDuration: CMTime = CMTime(value: 1, timescale: 30),
		 compressionFactor: Double = (1.0 / 18.0), // measured in bits/pixel
		 bandSize: Double = 1.25) {
		
		let pixelRate = width * height * Int(frameDuration.timescale) / Int(frameDuration.value)
		let bitRate = Int(Double(pixelRate) * compressionFactor)

		self.frameDimensions = CMVideoDimensions(width: Int32(width), height: Int32(height))
		self.frameDuration = frameDuration
		self.minBitRate = bitRate
		self.maxBitRate = Int(Double(bitRate) * bandSize)
	}

	init(width: Int,
		 height: Int,
		 frameDuration: CMTime = CMTime(value: 1, timescale: 30),
		 bitRate: Int,
		 bandSize: Double = 1.25) {
		
		self.frameDimensions = CMVideoDimensions(width: Int32(width), height: Int32(height))
		self.frameDuration = frameDuration
		self.minBitRate = bitRate
		self.maxBitRate = Int(Double(bitRate) * bandSize)
	}

	var macroblocksPerFrame: Int {
		let macroblockSize = 16
		let macroblocksWide = (Int(frameDimensions.width) + macroblockSize - 1) / macroblockSize
		let macroblocksHigh = (Int(frameDimensions.height) + macroblockSize - 1) / macroblockSize
		return macroblocksWide * macroblocksHigh
	}

	var macroblocksPerSecond: Int {
		return macroblocksPerFrame * Int(frameDuration.timescale) / Int(frameDuration.value)
	}
}

func chooseCaptureBand(
	from captureBands: [CaptureBand],
	withBitRate bitRate: Int,
	maxMacroblocksPerFrame: Int,
	maxMacroblocksPerSecond: Int,
	captureFormat: AVCaptureDevice.Format) -> CaptureBand {
	
	let filteredCaptureBands = captureBands.filter { captureBand in
		let maxTolerableFrameDuration = CMTimeMultiplyByFloat64(captureBand.frameDuration, multiplier: 1.2)
		let supportsFrameDuration = captureFormat.videoSupportedFrameRateRanges.contains {
			return CMTimeCompare(maxTolerableFrameDuration, $0.minFrameDuration) >= 0
		}
		let captureDimensions = CMVideoFormatDescriptionGetDimensions(captureFormat.formatDescription)
		return captureBand.macroblocksPerFrame <= maxMacroblocksPerFrame
			&& captureBand.macroblocksPerSecond <= maxMacroblocksPerSecond
			&& supportsFrameDuration
			&& captureBand.frameDimensions.width <= captureDimensions.width
			&& captureBand.frameDimensions.height <= captureDimensions.height
	}

	let maxBand = filteredCaptureBands.lazy.reversed().first { bitRate > $0.maxBitRate }
	let minBand = filteredCaptureBands.first { bitRate < $0.minBitRate }
	return maxBand ?? minBand ?? captureBands.first!
}
