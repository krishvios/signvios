//
//  CaptureController.swift
//  ntouch
//
//  Created by Daniel Shields on 10/26/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import Foundation
import VideoToolbox
import AVFoundation
#if os(iOS)
import UIKit
#endif

@objc(SCICaptureController)
class CaptureController : NSObject, AVCaptureVideoDataOutputSampleBufferDelegate {

	public typealias VideoGravity = AVLayerVideoGravity

	@objc public static let shared = CaptureController()
	@objc public static let targetDimensionsChanged = Notification.Name("SCICaptureControllerTargetDimensionsChanged")
	@objc public static let privacyChanged = Notification.Name("SCICaptureControllerPrivacyChanged")
	@objc public static let captureBegan = Notification.Name("SCICaptureControllerCaptureBegan")
	@objc public static let captureStopped = Notification.Name("SCICaptureControllerCaptureStopped")
	@objc public static let captureExclusiveChanged = Notification.Name("SCICaptureExclusiveChanged")
	@objc public static let captureExclusiveAllowPrivacyKey = "privacyAllowed"
	@objc public let captureSession = AVCaptureSession()
	let captureOutput = AVCaptureVideoDataOutput()
	let stillCaptureOutput = AVCaptureStillImageOutput()
	@objc public var captureQueue = DispatchQueue.init(label: "com.sorenson.ntouch.capture")
	@objc var outputFrameCallback: ((CMSampleBuffer) -> Void)?
	@objc var recalculateCaptureSizeCallback: (() -> Void)?
	@objc let videoLayers = NSHashTable<VideoInputLayer>.weakObjects()
	var recycledVideoLayers: [VideoInputLayer] = [] // We recycle preview layers because we crash otherwise on 10.9
	var defaultCaptureDeviceIdObserver: NSObjectProtocol?
	var encoders: [VideoEncoder.Type] = [AppleEncoder.self]
	
	func createVideoInputLayer() -> VideoInputLayer {
		
		if let videoLayer = recycledVideoLayers.first {
			recycledVideoLayers.remove(at: 0)
			videoLayer.isActive = true
			return videoLayer
		}
		else {
			let videoLayer = VideoInputLayer()
			register(videoLayer)
			videoLayer.isActive = true
			return videoLayer
		}
	}
	
	func releaseVideoInputLayer(_ layer: VideoInputLayer) {
		layer.isActive = false
		recycledVideoLayers.append(layer)
	}

	@objc(registerVideoLayer:)
	public func register(_ videoLayer: VideoInputLayer) {
		assertOnQueue(.main)

		captureQueue.async {
			videoLayer.session = self.captureSession
			
			DispatchQueue.main.async {
				videoLayer.connection?.videoOrientation = self.videoOrientation
				if videoLayer.connection?.automaticallyAdjustsVideoMirroring == false {
					videoLayer.connection?.isVideoMirrored = (self.captureDevice?.position != .back)
				}
			}
		}
		
		videoLayer.videoGravity = videoGravity
		videoLayer.videoDimensions = targetDimensions

		if privacy {
			videoLayer.state = .privacy
		} else if receivingVideo {
			videoLayer.state = .video
		}
		
		videoLayer.captureController = self
		videoLayers.add(videoLayer)

		updatePreviewing()
		
#if os(iOS)
		// Make sure that the videoOrientation in the CaptureController matches the device orientation
		if let orientation = convertToVideoOrientation(from: UIApplication.shared.statusBarOrientation), videoOrientation != orientation {
			videoOrientation = orientation
		}
#endif
	}

	public override init() {
		super.init()
		captureOutput.videoSettings = [
			kCVPixelBufferPixelFormatTypeKey as String : kCVPixelFormatType_420YpCbCr8BiPlanarFullRange as AnyObject
		]
		captureOutput.setSampleBufferDelegate(self, queue: captureQueue)
#if os(iOS)
		captureSession.sessionPreset = .inputPriority
#endif
		if captureSession.canAddOutput(captureOutput) {
			captureSession.addOutput(captureOutput)
		} else {
			print("SCICaptureController: Could not add video capture output to capture session.")
		}

		stillCaptureOutput.outputSettings = [
			kCVPixelBufferPixelFormatTypeKey as String : kCVPixelFormatType_32BGRA as AnyObject
		]
		if captureSession.canAddOutput(stillCaptureOutput) {
			captureSession.addOutput(stillCaptureOutput)
		} else {
			print("SCICaptureController: Could not add still capture output to capture session.")
		}

		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureSessionRuntimeError(_:)),
			name: .AVCaptureSessionRuntimeError,
			object: captureSession)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureSessionDidStartRunning(_:)),
			name: .AVCaptureSessionDidStartRunning,
			object: nil)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureSessionDidStopRunning(_:)),
			name: .AVCaptureSessionDidStopRunning,
			object: nil)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureDeviceWasConnected(_:)),
			name: .AVCaptureDeviceWasConnected,
			object: nil)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureDeviceWasDisconnected(_:)),
			name: .AVCaptureDeviceWasDisconnected,
			object: nil)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureInputPortFormatDescriptionDidChange(_:)),
			name: .AVCaptureInputPortFormatDescriptionDidChange,
			object: nil)
		defaultCaptureDeviceIdObserver = UserDefaults.standard.observe(\.SCIVideoCaptureDevice, options: [.new]) { (_, change) in
			if let uniqueId = change.newValue!,
				let device = AVCaptureDevice(uniqueID: uniqueId) {
				
				self.captureDevice = device
			}
		}
#if os(iOS)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureSessionWasInterrupted(_:)),
			name: .AVCaptureSessionWasInterrupted,
			object: nil)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureSessionInterruptionEnded(_:)),
			name: .AVCaptureSessionInterruptionEnded,
			object: nil)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyStatusBarOrientationChanged(_:)),
			name: UIApplication.didChangeStatusBarOrientationNotification,
			object: nil)

		if let orientation = convertToVideoOrientation(from: UIApplication.shared.statusBarOrientation) {
			videoOrientation = orientation
		}
#endif
	}

	// MARK: Public Attributes

	func checkEnabled(previewing: Bool, recording: Bool, privacy: Bool) -> Bool {
		// We want to be enabled while previewing or while recording, but not
		// while recording and privacy. It does not matter if privacy is enabled
		// while we're only previewing.
		return (previewing || recording) && !privacy
	}

	@objc public var enabled: Bool {
		return checkEnabled(previewing: previewing, recording: recording, privacy: privacy)
	}

	func didSetEnabled(oldValue: Bool) {
		assertOnQueue(.main)
		if oldValue == enabled && self.captureSession.isRunning {
			return
		}

		print("SCICaptureController: didSetEnabled enabled=\(enabled)")

		if enabled {
			videoLayers.allObjects.forEach { $0.state = .cameraStarting }

			if captureDevice == nil {
				captureDevice = nextCaptureDevice
			}

			requestKeyframe()
			tryStart()
			NotificationCenter.default.post(name: CaptureController.captureBegan, object: self)
		} else {
			tryStop()
			NotificationCenter.default.post(name: CaptureController.captureStopped, object: self)
		}
	}

	@objc public private(set) var previewing = false {
		didSet {
			assertOnQueue(.main)
			didSetEnabled(oldValue: checkEnabled(previewing: oldValue, recording: recording, privacy: privacy))
		}
	}

	func updatePreviewing() {
		assertOnQueue(.main)
		previewing = videoLayers.allObjects.contains { $0.isActive }
	}

	@objc public var recording = false {
		didSet {
			assertOnQueue(.main)

			// Check and see which codecs we can open hardware encoders for at
			// the beginning of each call.
			if recording {
				captureQueue.async {
					AppleEncoder.recomputeHardwareCodecs()
				}
			} else {
				// It's important we destroy the encoder, because otherwise it
				// may output an old frame the next time we try to use it.
				captureQueue.async {
					self.videoEncoder?.invalidate()
					self.videoEncoder = nil
				}
			}

			didSetEnabled(oldValue: checkEnabled(previewing: previewing, recording: oldValue, privacy: privacy))
		}
	}

	@objc public var privacy = false {
		didSet {
			assertOnQueue(.main)

			if privacy || interrupted {
				captureQueue.async {
					self.receivingVideo = false
				}
				self.videoLayers.allObjects.forEach { $0.state = .privacy }
			}
			didSetEnabled(oldValue: checkEnabled(previewing: previewing, recording: recording, privacy: oldValue))

			if privacy != oldValue {
				NotificationCenter.default.post(name: CaptureController.privacyChanged, object: self)
			}
		}
	}

	// We want to show privacy during capture interruptions, but we don't want
	// to stop the capture session or we won't know when the interruption ended.
	@objc public var interrupted = false {
		didSet {
			assertOnQueue(.main)

			if privacy || interrupted {
				captureQueue.async {
					self.receivingVideo = false
				}
				self.videoLayers.allObjects.forEach { $0.state = .privacy }
			}

			if interrupted != oldValue {
				NotificationCenter.default.post(name: CaptureController.privacyChanged, object: self)
			}
		}
	}

	@objc public var videoGravity = VideoGravity.resizeAspectFill {
		didSet {
			assertOnQueue(.main)

			videoLayers.allObjects.forEach { $0.videoGravity = videoGravity }
			captureQueue.async {
				self.reconfigureCaptureFormat()
			}
		}
	}

	@objc public var videoOrientation = AVCaptureVideoOrientation.portrait {
		didSet {
			assertOnQueue(.main)

			recalculateCaptureSizeCallback?()
			captureQueue.async {
				self.reconfigureCaptureFormat()
			}
		}
	}

	@objc public var targetDimensions = CMVideoDimensions(width: 352, height: 288) {
		didSet {
			if targetDimensions != oldValue {
				encoderSettingsInvalidated = true
				captureQueue.async {
					self.videoEncoder?.invalidate()
					self.videoEncoder = nil
					self.reconfigureCaptureFormat()
				}
				
				DispatchQueue.main.async {
					let targetDimensions = self.targetDimensions
					// Listeners use this notification to change the preview layer's
					// dimensions to match the aspect ratio of the target dimensions.
					NotificationCenter.default.post(
						name: CaptureController.targetDimensionsChanged,
						object: self)
					self.videoLayers.allObjects.forEach { $0.videoDimensions = targetDimensions }
				}
			} else {
				print("SCICaptureController: targetDimensions unchanged, skipping reconfigure.")
			}
		}
	}

	@objc public var targetFrameDuration = CMTime(value: 1, timescale: 30) {
		didSet {
			encoderSettingsInvalidated = true
			captureQueue.async {
				self.reconfigureCaptureFormat()
			}
		}
	}

	var viableFormats: [AVCaptureDevice.Format] {
		return (captureDevice?.formats ?? [])
	}

	var biggestFormat: AVCaptureDevice.Format? {
		let filteredFormats: [AVCaptureDevice.Format]
		
		if #available(iOS 17.0, *) {
			filteredFormats = viableFormats.filter { !$0.reactionEffectsSupported }
		} else {
			filteredFormats = viableFormats
		}
		
		return filteredFormats.max { (formatA, formatB) -> Bool in
			let sizeA = CMVideoFormatDescriptionGetDimensions(formatA.formatDescription)
			let sizeB = CMVideoFormatDescriptionGetDimensions(formatB.formatDescription)

			if sizeB.width > 1920 || sizeB.height > 1080 {
				return false
			}
			if sizeA.width > 1920 || sizeA.height > 1080 {
				return true
			}

			// Pick the largest format
			return sizeA.width * sizeA.height < sizeB.width * sizeB.height
		}
	}

	// The best capture format for the given target dimensions, video gravity, video orientation, and target frame duration
	var idealFormat: AVCaptureDevice.Format? {
		return viableFormats.max { (formatA, formatB) -> Bool in
			// Return true if formatB is better than formatA
			var sizeA = CMVideoFormatDescriptionGetDimensions(formatA.formatDescription)
			var sizeB = CMVideoFormatDescriptionGetDimensions(formatB.formatDescription)
			if videoOrientation == .portrait || videoOrientation == .portraitUpsideDown {
				// In portrait, the video capture is sideways
				swap(&sizeA.width, &sizeA.height)
				swap(&sizeB.width, &sizeB.height)
			}

			// If one supports the framerate we want to capture at and one does not, pick the one that does.
			let supportsFrameRate: ([AVFrameRateRange]) -> Bool = { ranges in
				ranges.contains {
					CMTimeCompare(self.targetFrameDuration, $0.minFrameDuration) >= 0 &&
					CMTimeCompare(self.targetFrameDuration, $0.maxFrameDuration) <= 0
				}
			}
			let supportsFrameRateA = supportsFrameRate(formatA.videoSupportedFrameRateRanges)
			let supportsFrameRateB = supportsFrameRate(formatB.videoSupportedFrameRateRanges)
			if supportsFrameRateA != supportsFrameRateB {
				return supportsFrameRateB
			}

			// If one format fits one dimension and one does not, pick the one that does
			let fitsOneDimensionA = (sizeA.width >= targetDimensions.width || sizeA.height >= targetDimensions.height)
			let fitsOneDimensionB = (sizeB.width >= targetDimensions.width || sizeB.height >= targetDimensions.height)
			if fitsOneDimensionA != fitsOneDimensionB {
				return fitsOneDimensionB
			}

			// If one format fits both dimensions and one does not...
			let fitsBothDimensionsA = (sizeA.width >= targetDimensions.width && sizeA.height >= targetDimensions.height)
			let fitsBothDimensionsB = (sizeB.width >= targetDimensions.width && sizeB.height >= targetDimensions.height)
			if fitsBothDimensionsA != fitsBothDimensionsB {
				// If ResizeAspectFill, pick the one that does fit both dimensions
				// Otherwise, pick the one that does not fit both dimensions (less zoom effect after cropping this way)
				return fitsBothDimensionsB == (videoGravity == .resizeAspectFill)
			}

			// Pick the smallest format
			return sizeB.width * sizeB.height < sizeA.width * sizeA.height
		}
	}

	@objc public var captureSize: CMVideoDimensions {
		if let formatDescription = captureDevice?.activeFormat.formatDescription {
			return CMVideoFormatDescriptionGetDimensions(formatDescription)
		} else {
			return CMVideoDimensions(width: 0, height: 0)
		}
	}

	// MARK: Capture devices

	@objc public var availableCaptureDevices: [AVCaptureDevice] {
#if os(iOS)
		if #available(iOS 10.0, *) {
			return AVCaptureDevice.DiscoverySession(deviceTypes: [.builtInWideAngleCamera],
			                                        mediaType: .video,
			                                        position: .unspecified).devices
		} else {
			return AVCaptureDevice.devices(for: .video)
		}
#else
		return AVCaptureDevice.devices(for: .video).filter { $0.modelID != "LogiCapture" } // Merely accessing Logi Capture as a virtual camera crashes macOS versions <= 10.15
#endif
	}

	@objc public var nextCaptureDevice: AVCaptureDevice? {
		let captureDevices = availableCaptureDevices
		if let captureDevice = captureDevice, var index = captureDevices.firstIndex(of: captureDevice) {

			captureDevices.formIndex(after: &index)
			if index == captureDevices.endIndex {
				index = captureDevices.startIndex
			}

			return captureDevices[index]
		}
		// If we don't have a capture device, the default should be the front facing camera, or specified by the user.
		else {
			if let defaultUID = UserDefaults.standard.string(forKey: "SCIVideoCaptureDevice"),
				let device = AVCaptureDevice(uniqueID: defaultUID) {
				return device
			}

#if os(iOS)
			if #available(iOS 10.0, *) {
				return AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .front) ?? captureDevices.first
			}
#endif
			return captureDevices.first { $0.position == .front } ?? captureDevices.first
		}
	}

	// MARK: Managing the Capture Session

	var _captureDevice: AVCaptureDevice?
	@objc public var captureDevice: AVCaptureDevice? {
		get { return _captureDevice }
		set(newDevice) {
			if newDevice == _captureDevice {
				return
			}
			captureQueue.sync {
				captureSession.beginConfiguration()
				defer {
					captureSession.commitConfiguration()
				}

				for oldInput in captureSession.inputs {
					captureSession.removeInput(oldInput)
				}

				guard let newDevice = newDevice else {
					_captureDevice = nil
					DispatchQueue.main.async {
						self.videoLayers.allObjects.forEach { $0.state = .error }
					}
					return
				}

				var input: AVCaptureInput! = nil
				do {
					input = try AVCaptureDeviceInput.init(device: newDevice)
				} catch {
					print("SCICaptureController: FATAL: Failed to create capture device input!")
					DispatchQueue.main.async {
						self.videoLayers.allObjects.forEach { $0.error = error }
					}
					return
				}

				guard captureSession.canAddInput(input) else {
					print("SCICaptureController: FATAL: Can not currently add capture device's input to the capture session!")
					return
				}

				_captureDevice = newDevice
				receivingVideo = false
				DispatchQueue.main.async {
					self.videoLayers.allObjects.forEach {
						if $0.state != .privacy {
							$0.state = .cameraStarting
						}
					}
				}

				captureSession.addInput(input)
				reconfigureCaptureFormat()
				recalculateCaptureSizeCallback?()
			}
		}
	}

	func reconfigureCaptureOrientation() {
		guard let captureDevice = captureDevice else {
			// We can't do anything if we don't have a capture device.
			return
		}
		
		DispatchQueue.main.async { [videoOrientation] in
			self.videoLayers.allObjects.forEach { videoLayer in
				videoLayer.connection?.videoOrientation = videoOrientation
				if videoLayer.connection?.automaticallyAdjustsVideoMirroring == false {
					videoLayer.connection?.isVideoMirrored = (self.captureDevice?.position != .back)
				}
			}
		}
		for output in self.captureSession.outputs {
			if let connection = output.connection(with: AVMediaType.video),
				connection.isVideoOrientationSupported {
				do {
					try captureDevice.lockForConfiguration()
					connection.videoOrientation = self.videoOrientation
					captureDevice.unlockForConfiguration()
				} catch {
					print("SCICaptureController: Could not lock device for orientation! \(error)")
					DispatchQueue.main.async {
						self.videoLayers.allObjects.forEach { $0.error = error }
					}
				}
			}
		}
	}

	func reconfigureCaptureFormat() {

		guard let captureDevice = captureDevice else {
			// We can't do anything if we don't have a capture device.
			return
		}

		do {
			try captureDevice.lockForConfiguration()
			// Use the biggest capture format and crop
			if let biggestFormat = biggestFormat {
				captureDevice.activeFormat = biggestFormat
				let videoDimensions = CMVideoFormatDescriptionGetDimensions(captureDevice.activeFormat.formatDescription)
				print("SCICaptureController: Capture dimensions: \(videoDimensions)")

				// Find the best frame rate given the supported frame rate ranges.
				var bestFrameDuration: CMTime?
				for rateRange in captureDevice.activeFormat.videoSupportedFrameRateRanges {
					// If we can use our target frame duration, bail.
					if CMTimeCompare(targetFrameDuration, rateRange.minFrameDuration) >= 0 &&
						CMTimeCompare(targetFrameDuration, rateRange.maxFrameDuration) <= 0 {
						bestFrameDuration = targetFrameDuration
						break
					}

					guard let currentBest = bestFrameDuration else {
						bestFrameDuration = rateRange.minFrameDuration
						continue
					}

					if CMTimeCompare(currentBest, rateRange.minFrameDuration) > 0 {
						bestFrameDuration = rateRange.minFrameDuration
					}
				}

				captureDevice.activeVideoMinFrameDuration = bestFrameDuration!
				captureDevice.activeVideoMaxFrameDuration = bestFrameDuration!
			} else {
				print("SCICaptureController: Could not find an ideal format!")
			}
			captureDevice.unlockForConfiguration()
		} catch {
			print("SCICaptureController: Could not lock device for configuration! \(error)")
			DispatchQueue.main.async {
				self.videoLayers.allObjects.forEach { $0.error = error }
			}
		}

		reconfigureCaptureOrientation()
	}

	func tryStart() {
		captureQueue.async {
			if self.enabled {
				self.lastFrameTime = nil
				self.frameDurations = []
				self.receivingVideo = false
				self.captureSession.startRunning()
			}
		}
	}

	func tryStop() {
		captureQueue.async {
			DispatchQueue.main.async {
				self.interrupted = false
			}
			if !self.enabled {
				self.captureSession.stopRunning()
				self.receivingVideo = false
			}
		}
	}

	@objc var receivingVideo = false

	// MARK: Encoder

	// TODO: We probably want setting this to be atomic, but no interface to
	// atomics comes with Swift...
	var requestedKeyframe: Bool = false
	var requestedKeyframeLock: NSLock = NSLock()
	@objc public func requestKeyframe() {
		requestedKeyframeLock.lock()
		requestedKeyframe = true
		requestedKeyframeLock.unlock()
	}

	var encoderSettingsInvalidated = true
	@objc var videoCodec: CMVideoCodecType = 0 {
		didSet {
			captureQueue.async {
				self.videoEncoder?.invalidate()
				self.videoEncoder = nil
			}
		}
	}
	@objc var profile: VideoProfile = .h264Baseline { didSet { encoderSettingsInvalidated = true } }
	@objc var level: Int32 = 0 { didSet { encoderSettingsInvalidated = true } }
	@objc var targetBitRate: Int = 0 { didSet { encoderSettingsInvalidated = true } }
	@objc var maxPacketSize: Int = 0 { didSet { encoderSettingsInvalidated = true } }
	@objc var intraFrameRate: Int = Int.max { didSet { encoderSettingsInvalidated = true } }
	var videoEncoder: VideoEncoder? = nil { didSet { encoderSettingsInvalidated = true } }

	// Measuring frame rate
	let maxFrameDurations: Int = 30
	var lastFrameTime: CMTime?
	var frameDurations: [CMTime] = []
	@objc var averageFrameDuration: CMTime {
		let sum = frameDurations.reduce(.zero, +)
		if #available(macOS 10.10, *) {
			return CMTimeMultiplyByRatio(sum, multiplier: 1, divisor: Int32(frameDurations.count))
		} else {
			return CMTimeMultiplyByFloat64(sum, multiplier: 1.0 / Float64(frameDurations.count))
		}
	}

	// NOTE: Not thread safe, only call on camera tread
	func updateEncoderSettings() {
		assertOnQueue(captureQueue)
		if let videoEncoder = videoEncoder {
			do {
				try videoEncoder.updateSettings(
					targetFrameDuration: targetFrameDuration,
					profile: profile,
					level: level,
					targetBitRate: targetBitRate,
					maxPacketSize: maxPacketSize,
					intraFrameRate: intraFrameRate,
					videoGravity: videoGravity)
				encoderSettingsInvalidated = false
			} catch {
				print("SCICaptureController: Failed to update settings of encoder: \(error)")
			}
		}
	}

	@objc public var enableHEVC = false
	@objc public var supportsHEVC: Bool {
		return AppleEncoder.supportedCodecs.contains(kCMVideoCodecType_HEVC)
	}

	func createEncoder() {
		assertOnQueue(captureQueue)
		let callback: (CMSampleBuffer?, Error?) -> Void = { sampleBuffer, error in
			self.encoded(sampleBuffer, error: error)
		}

		videoEncoder = encoders.lazy
			.filter { $0.supportedCodecs.contains(self.videoCodec) }
			.compactMap { try? $0.init(codec: self.videoCodec, targetDimensions: self.targetDimensions, outputCallback: callback) }
			.first

		if videoEncoder == nil {
			print("SCICaptureController: Failed to create encoder: Codec not supported")
		}
	}

	func captureOutput(_ captureOutput: AVCaptureOutput,
	                   didOutput sampleBuffer: CMSampleBuffer,
	                   from connection: AVCaptureConnection) {
		assertOnQueue(captureQueue)
		if !receivingVideo && enabled && !interrupted && connection == captureOutput.connection(with: .video) {
			receivingVideo = true
			DispatchQueue.main.async {
				self.videoLayers.allObjects.forEach { $0.state = .video }
			}
		}

		if !recording || privacy || interrupted || outputFrameCallback == nil {
			// Don't send frames unless we're recording
			return
		}

		if videoEncoder == nil {
			createEncoder()
		}

		guard let videoEncoder = videoEncoder else {
			print("SCICaptureController: Failed to encode sample buffer, encoder is not valid")
			return
		}

		if encoderSettingsInvalidated {
			updateEncoderSettings()
		}

		// Calculate average frame rate
		let currentFrameTime = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
		if let lastFrameTime = lastFrameTime {
			if frameDurations.count >= maxFrameDurations {
				frameDurations.removeFirst()
			}

			frameDurations.append(CMTimeSubtract(currentFrameTime, lastFrameTime))
		}
		lastFrameTime = currentFrameTime
		var currentBuffer = sampleBuffer
		// https://sorenson.atlassian.net/browse/FBC-821
		// Sending an image instead of the video for automation testing.
		// If the environment parameter is set, use the image.
		if let nocInstructionsImage = ProcessInfo.processInfo.environment["NOC_INSTRUCTIONS_IMAGE"],
		   let image = UIImage(named: nocInstructionsImage),
		   let imageBuffer = uiImageToCMSampleBuffer(image: image) {
			currentBuffer = imageBuffer
			// Force Landscape mode.
			if !UIDevice.current.orientation.isLandscape {
				DispatchQueue.main.async {
					let orientation = UIInterfaceOrientation.landscapeRight.rawValue
					UIDevice.current.setValue(orientation, forKey: "orientation")
				}
			}
		}

		do {
			try videoEncoder.encode(currentBuffer, forceKeyframe: requestedKeyframe)
			requestedKeyframeLock.lock()
			self.requestedKeyframe = false
			requestedKeyframeLock.unlock()
		} catch {
			print("SCICaptureController: Video encoder failed to encode a frame \(error), attempting to restart encoder")
			self.videoEncoder?.invalidate()
			self.videoEncoder = nil
		}
	}

	func encoded(_ sampleBuffer: CMSampleBuffer?, error: Error?) {
		if let sampleBuffer = sampleBuffer {
			self.outputFrameCallback?(sampleBuffer)
		} else if let error = error {
			print("SCICaptureController: Error encoding frame: \(error)")
		}
		// If we don't have an error or a sample buffer, the frame was dropped.
	}
	
	@objc public var forcedCodec: CMVideoCodecType = 0
	@objc public var availableCodecs: [CMVideoCodecType] {
		guard forcedCodec == 0 else {
			return [forcedCodec]
		}

		return encoders.flatMap { $0.supportedCodecs }
	}

	// MARK: Calculate capture size

	@objc var preferWidescreen = true {
		didSet {
			recalculateCaptureSizeCallback?()
		}
	}

	static let captureBandsH264_16x9 = [
		CaptureBand(width: 432, height: 240),
		CaptureBand(width: 640, height: 360),
		CaptureBand(width: 864, height: 480),
		CaptureBand(width: 1280, height: 720),
		CaptureBand(width: 1680, height: 896, bitRate: 2048000),
		CaptureBand(width: 1920, height: 1080, bitRate: 3072000)
	]
	static let captureBandsH264_4x3 = [
		CaptureBand(width: 320, height: 240),
		CaptureBand(width: 480, height: 360),
		CaptureBand(width: 640, height: 480),
		CaptureBand(width: 960, height: 720)
	]
	static let captureBandsH264_1x1 = [
		CaptureBand(width: 240, height: 240),
		CaptureBand(width: 360, height: 360),
		CaptureBand(width: 480, height: 480),
		CaptureBand(width: 720, height: 720)
	]
	static let captureBandsH265_16x9 = [
		CaptureBand(width: 432, height: 240, bitRate: 64000),
		CaptureBand(width: 640, height: 360, bitRate: 128000),
		CaptureBand(width: 864, height: 480, bitRate: 192000),
		CaptureBand(width: 1280, height: 720, bitRate: 640000),
		CaptureBand(width: 1680, height: 896, bitRate: 1768000),
		CaptureBand(width: 1920, height: 1080, bitRate: 2048000)
	]
	static let captureBandsH265_4x3 = [
		CaptureBand(width: 320, height: 240, bitRate: 64000),
		CaptureBand(width: 480, height: 360, bitRate: 128000),
		CaptureBand(width: 640, height: 480, bitRate: 192000),
		CaptureBand(width: 960, height: 720, bitRate: 640000)
	]
	static let captureBandsH265_1x1 = captureBandsH264_1x1

	@objc func calculateCaptureSize(
		maxMacroblocksPerFrame: UInt,
		maxMacroblocksPerSecond: UInt,
		bitRate: Int,
		videoCodec: CMVideoCodecType,
		preferredSize: CMVideoDimensions,
		outVideoSize: UnsafeMutablePointer<CMVideoDimensions>,
		outFrameDuration: UnsafeMutablePointer<CMTime>) {
		if captureDevice == nil || videoCodec == kCMVideoCodecType_H263 {
			outVideoSize.initialize(to: CMVideoDimensions(width: 352, height: 288))
			outFrameDuration.initialize(to: CMTime(value: 1, timescale: 30))
		} else {
#if os(macOS)
			// On macOS we always want to follow the user-selected widescreen setting.
			let automaticallyAdjustsWidescreen = false
			let capturesPortrait = false // Video orientation pretends to be portrait on macOS even though it isn't.
#else
			let automaticallyAdjustsWidescreen = true
			let capturesPortrait = (videoOrientation == .portrait || videoOrientation == .portraitUpsideDown)
#endif

			// Don't send portrait video to devices that don't advertise their size.
			let canSendPortrait = (preferredSize.width > 0 && preferredSize.height > 0)
			let remotePrefersPortrait = canSendPortrait && (preferredSize.height > preferredSize.width)
			let widescreen = preferWidescreen && (!automaticallyAdjustsWidescreen || capturesPortrait == remotePrefersPortrait)

			var captureBands: [CaptureBand] = []
			if videoCodec == kCMVideoCodecType_H264 {
				captureBands = widescreen ? CaptureController.captureBandsH264_16x9 :
					capturesPortrait == remotePrefersPortrait ? CaptureController.captureBandsH264_4x3 : CaptureController.captureBandsH264_1x1
			} else if videoCodec == kCMVideoCodecType_HEVC {
				captureBands = widescreen ? CaptureController.captureBandsH265_16x9 :
					capturesPortrait == remotePrefersPortrait ? CaptureController.captureBandsH265_4x3 : CaptureController.captureBandsH265_1x1
			}

			let chosenBand = chooseCaptureBand(
				from: captureBands,
				withBitRate: bitRate,
				maxMacroblocksPerFrame: Int(maxMacroblocksPerFrame),
				maxMacroblocksPerSecond: Int(maxMacroblocksPerSecond),
				captureFormat: biggestFormat!)

			outVideoSize.initialize(to: chosenBand.frameDimensions)
			outFrameDuration.initialize(to: chosenBand.frameDuration)

			if canSendPortrait && capturesPortrait {
				swap(&outVideoSize.pointee.width, &outVideoSize.pointee.height)
			}
		}
	}

	// MARK: Capturing still images

	@objc(captureStillImageWithCompletion:)
	func captureStillImage(with completion: @escaping (CVPixelBuffer?, Error?) -> Void) {
		stillCaptureOutput.captureStillImageAsynchronously(from: captureOutput.connection(with: .video)!) { sampleBuffer, error in
			if let sampleBuffer = sampleBuffer {
				let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer)
				completion(imageBuffer, error)
			} else {
				completion(nil, error)
			}
		}
	}

	// MARK: Capture session notifications

	@objc func notifyCaptureSessionRuntimeError(_ note: NSNotification) {
		guard let error = note.userInfo?[AVCaptureSessionErrorKey] as? AVError else {
			print("SCICaptureController: Unknown error occurred.")
			return
		}

		print("SCICaptureController: Runtime capture session error: \(error)")
#if os(iOS)
		print("SCICaptureController: Attempting to recover from runtime error")

		// Try starting the capture session again if it should be running
		tryStart()
#endif
	}

	@objc func notifyCaptureSessionDidStartRunning(_ note: NSNotification) {
		if note.object as? AVCaptureSession === captureSession {
			print("Capture: SCICaptureController session began")
		} else {
			print("Capture: session began: \(note.object ?? "Unknown")")
		}
	}

	@objc func notifyCaptureSessionDidStopRunning(_ note: NSNotification) {
		if note.object as? AVCaptureSession === captureSession {
			print("Capture: SCICaptureController session stopped")
		} else {
			print("Capture: session stopped: \(note.object ?? "Unknown")")
		}
		// If a different session stopped running, we may be able to start our session now.
		if note.object as? AVCaptureSession != captureSession && enabled {
			tryStart()
		}
	}

	@objc func notifyCaptureDeviceWasConnected(_ note: NSNotification) {
		let connectedDevice = note.object as? AVCaptureDevice
		if let connectedDevice = connectedDevice, connectedDevice.hasMediaType(.video) && captureDevice == nil && enabled {
			print("SCICaptureController: Capture device was connected")
			captureDevice = connectedDevice
		}
	}

	@objc func notifyCaptureDeviceWasDisconnected(_ note: NSNotification) {
		let disconnectedDevice = note.object as? AVCaptureDevice

		// If we're the capture device that was disconnected, use the next capture device.
		if let disconnectedDevice = disconnectedDevice, disconnectedDevice === captureDevice {
			print("SCICaptureController: Capture device was disconnected")
			if enabled {
				captureDevice = nextCaptureDevice
			} else {
				captureDevice = nil
			}
		}
	}

	@objc func notifyCaptureInputPortFormatDescriptionDidChange(_ note: NSNotification) {
		print("SCICaptureController: Capture input format description changed")
	}

#if os(iOS)
	@objc func notifyStatusBarOrientationChanged(_ note: NSNotification) {
		if let orientation = convertToVideoOrientation(from: UIApplication.shared.statusBarOrientation) {
			videoOrientation = orientation
		}
	}

	@objc func notifyCaptureSessionWasInterrupted(_ note: NSNotification) {
		let reason = (note.userInfo?[AVCaptureSessionInterruptionReasonKey] as? Int).map { AVCaptureSession.InterruptionReason(rawValue: $0)! }
		if note.object as? AVCaptureSession === captureSession {
			print("Capture: SCICaptureController session interrupted (\(reason.map { String(describing: $0) } ?? "Unknown"))")
		} else {
			print("Capture: session interrupted (\(reason.map { String(describing: $0) } ?? "Unknown")): \(note.object ?? "Unknown")")
		}
		
		guard note.object as? AVCaptureSession == captureSession else {
			return
		}
		
		switch reason {
		case .videoDeviceNotAvailableInBackground?:
			break // Let the app delegate handle this, put the call on hold
		case nil,
			 .audioDeviceInUseByAnotherClient?,
			 .videoDeviceInUseByAnotherClient?,
			 .videoDeviceNotAvailableWithMultipleForegroundApps?,
			 .videoDeviceNotAvailableDueToSystemPressure?:
			// TODO: Inform the user of system pressure or multiple foreground apps.
			fallthrough
		@unknown default:
			DispatchQueue.main.async {
				self.interrupted = true
			}
		}
	}

	@objc func notifyCaptureSessionInterruptionEnded(_ note: NSNotification) {
		if note.object as? AVCaptureSession === captureSession {
			print("Capture: SCICaptureController session interruption ended")
		} else {
			print("Capture: session interruption ended: \(note.object ?? "Unknown")")
		}
		
		if note.object as? AVCaptureSession == captureSession {
			DispatchQueue.main.async {
				self.interrupted = false
			}
		}
	}

	func convertToVideoOrientation(from interfaceOrientation: UIInterfaceOrientation) -> AVCaptureVideoOrientation? {
		switch interfaceOrientation {
		case UIInterfaceOrientation.portrait:
			return AVCaptureVideoOrientation.portrait
		case UIInterfaceOrientation.portraitUpsideDown:
			return AVCaptureVideoOrientation.portraitUpsideDown
		case UIInterfaceOrientation.landscapeLeft:
			return AVCaptureVideoOrientation.landscapeLeft
		case UIInterfaceOrientation.landscapeRight:
			return AVCaptureVideoOrientation.landscapeRight
		default:
			return nil
		}
	}
#endif
}

extension UserDefaults {
	@objc dynamic var SCIVideoCaptureDevice: String? { return string(forKey: "SCIVideoCaptureDevice") }
}

extension CMVideoDimensions: Equatable {
	public static func == (a: CMVideoDimensions, b: CMVideoDimensions) -> Bool {
		return a.width == b.width && a.height == b.height
	}
}

extension CaptureController {
	func uiImageToCMSampleBuffer(image: UIImage) -> CMSampleBuffer? {
		guard let cgImage = image.cgImage else { return nil }
		
		let options: [CFString: Any] = [
			kCVPixelBufferCGImageCompatibilityKey: kCFBooleanTrue!,
			kCVPixelBufferCGBitmapContextCompatibilityKey: kCFBooleanTrue!
		]
		
		var pixelBuffer: CVPixelBuffer?
		let width = cgImage.width
		let height = cgImage.height
		
		CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_32ARGB, options as CFDictionary, &pixelBuffer)
		
		guard let buffer = pixelBuffer else { return nil }
		
		CVPixelBufferLockBaseAddress(buffer, [])
		let pixelData = CVPixelBufferGetBaseAddress(buffer)
		
		let colorSpace = CGColorSpaceCreateDeviceRGB()
		let context = CGContext(data: pixelData, width: width, height: height, bitsPerComponent: 8, bytesPerRow: CVPixelBufferGetBytesPerRow(buffer), space: colorSpace, bitmapInfo: CGImageAlphaInfo.noneSkipFirst.rawValue)
		
		context?.draw(cgImage, in: CGRect(x: 0, y: 0, width: width, height: height))
		CVPixelBufferUnlockBaseAddress(buffer, [])
		
		var sampleBuffer: CMSampleBuffer?
		var timingInfo = CMSampleTimingInfo()
		var videoInfo: CMVideoFormatDescription?
		
		CMVideoFormatDescriptionCreateForImageBuffer(allocator: kCFAllocatorDefault, imageBuffer: buffer, formatDescriptionOut: &videoInfo)
		
		guard let videoFormatDescription = videoInfo else { return nil }
		
		CMSampleBufferCreateForImageBuffer(allocator: kCFAllocatorDefault, imageBuffer: buffer, dataReady: true, makeDataReadyCallback: nil, refcon: nil, formatDescription: videoFormatDescription, sampleTiming: &timingInfo, sampleBufferOut: &sampleBuffer)
		
		return sampleBuffer
	}
}
