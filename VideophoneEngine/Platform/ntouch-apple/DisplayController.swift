//
//  DisplayController.swift
//  ntouch
//
//  Created by Daniel Shields on 1/12/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import AVFoundation
import VideoToolbox

@objc(SCIDisplayController)
class DisplayController: NSObject {
	@objc public static let shared = DisplayController()

	var observerTokens: [NSObjectProtocol] = []
	override init() {
		super.init()
		
		#if !os(macOS)
		let token = NotificationCenter.default.addObserver(
			forName: VideoOutputLayer.didLayout,
			object: nil,
			queue: .main) { note in
				
			if let videoLayer = note.object as? VideoOutputLayer,
				self.videoLayers.contains(videoLayer) {
				
				self.updatePreferredDisplaySize()
			}
		}
		observerTokens.append(token)
		#endif
	}

	deinit {
		observerTokens.forEach {
			NotificationCenter.default.removeObserver($0)
		}
	}

	private let videoLayers = NSHashTable<VideoOutputLayer>.weakObjects()
	@objc(registerVideoLayer:)
	public func register(_ videoLayer: VideoOutputLayer) {

		if videoLayers.contains(videoLayer) {
			return
		}

		if hold {
			videoLayer.state = .hold
		} else if privacy {
			videoLayer.state = .privacy
		}

		videoLayers.add(videoLayer)
		#if !os(macOS)
		updatePreferredDisplaySize()
		#endif
	}

	@objc public var privacy: Bool = false {
		didSet {
			DispatchQueue.main.async { [hold, privacy] in
				self.videoLayers.allObjects.forEach { videoLayer in
					if hold {
						videoLayer.state = .hold
					} else if privacy {
						videoLayer.state = .privacy
					}
				}
			}
		}
	}

	@objc public var hold: Bool = false {
		didSet {
			DispatchQueue.main.async { [hold, privacy] in
				self.videoLayers.allObjects.forEach { videoLayer in
					if hold {
						videoLayer.state = .hold
					} else if privacy {
						videoLayer.state = .privacy
					}
				}
			}
		}
	}

	@available(macOS 10.10, *)
	@objc public var status: AVQueuedSampleBufferRenderingStatus {
		assertOnQueue(.main)

		// Return the "worst" status.  If any view is rendering, we return
		// .rendering status. If any view has a failure, we return .failed
		// status.
		return videoLayers.allObjects.max { a, b in
			switch a.status {
			case .unknown:
				return b.status == .rendering || b.status == .failed
			case .rendering:
				return b.status == .failed
			case .failed:
				return false
			@unknown default:
				return false
			}
		}?.status ?? .unknown
	}

	@available(macOS 10.10, *)
	@objc public var error: Error? {
		assertOnQueue(.main)
		return videoLayers.allObjects.first { $0.error != nil }?.error
	}

	@objc public func flushAndRemoveImage() {
		DispatchQueue.main.async {
			self.videoLayers.allObjects.forEach { $0.flushAndRemoveImage() }
		}
	}

	@objc public func flush() {
		DispatchQueue.main.async {
			self.videoLayers.allObjects.forEach { $0.flush() }
		}
	}

	// Only allow a certain number of frames in flight to the main thread before
	// blocking for at most a certain amount of time before discarding the frame.
	//
	// This prevents the main thread from getting bogged down by frame enqueue
	// requests, and also prevents large numbers of playback buffers from being
	// tied up here waiting to be enqueued when the main thread is particularly
	// busy.
	let framesInFlight = DispatchSemaphore(value: 5)
	let enqueueTimeout = DispatchTimeInterval.milliseconds(100)
	@objc(enqueueSampleBuffer:)
	public func enqueue(_ sampleBuffer: CMSampleBuffer) {
		if framesInFlight.wait(timeout: .now() + enqueueTimeout) == .success {
			DispatchQueue.main.async {
				self.videoLayers.allObjects.forEach { $0.enqueue(sampleBuffer) }
				self.framesInFlight.signal()
			}
		}
	}

	@objc static let preferredDisplaySizeChanged = Notification.Name("SCIPreferredDisplaySizeChanged")

	@objc
	public var preferredDisplaySize: CGSize = CGSize.zero {
		didSet {
			if oldValue != preferredDisplaySize {
				NotificationCenter.default.post(
					name: DisplayController.preferredDisplaySizeChanged,
					object: self)
			}
		}
	}

	@available(macOS, unavailable)
	func updatePreferredDisplaySize() {
		preferredDisplaySize = videoLayers.allObjects.max { a, b in
			let aScore = a.bounds.width > a.bounds.height ? a.bounds.width / a.bounds.height : a.bounds.height / a.bounds.width
			let bScore = b.bounds.width > b.bounds.height ? b.bounds.width / b.bounds.height : b.bounds.height / b.bounds.width
			return aScore < bScore
		}?.bounds.size ?? CGSize.zero
	}

	@objc public var forcedCodec: CMVideoCodecType = 0
	@objc public var enableHEVC = false
	@objc public var supportsHEVC: Bool {
		if #available(iOS 11.0, macOS 10.13, *) {
			return VTIsHardwareDecodeSupported(kCMVideoCodecType_HEVC)
		} else {
			return false
		}
	}
}
