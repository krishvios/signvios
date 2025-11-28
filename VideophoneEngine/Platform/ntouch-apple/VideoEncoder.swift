//
//  Encoder.swift
//  ntouch
//
//  Created by Daniel Shields on 4/4/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import AVFoundation
import VideoToolbox

@objc(SCIVideoProfile)
enum VideoProfile: Int32 {
	case h264Baseline = 0x42
	case h264Main = 0x4d
	case h264Extended = 0x58
	case h264High = 0x64
	case h265Main = 1
	case h265Main10 = 2
	
	static let h263Zero: VideoProfile = .h265Main // These are both equal to 1
}

protocol VideoEncoder {
	var codec: CMVideoCodecType { get }
	var outputCallback: (CMSampleBuffer?, Error?) -> Void { get }
	init(codec: CMVideoCodecType, targetDimensions: CMVideoDimensions, outputCallback: @escaping (CMSampleBuffer?, Error?) -> Void) throws
	
	/// Users of VideoEncoder should invalidate the encoder *before* it gets cleaned up. The encoder references
	/// itself every time it gets a frame so it can call outputCallback, and if there are encoded frames waiting
	/// to be output they will be during deinit causing it to try to increment its reference count while it's
	/// being deallocated, crashing.
	func invalidate()

	func updateSettings(
		targetFrameDuration: CMTime,
		profile: VideoProfile,
		level: Int32,
		targetBitRate: Int,
		maxPacketSize: Int,
		intraFrameRate: Int,
		videoGravity: AVLayerVideoGravity) throws

	func encode(_ sampleBuffer: CMSampleBuffer, forceKeyframe: Bool) throws

	static var supportedCodecs: [CMVideoCodecType] { get }
}

class AppleEncoder: VideoEncoder {
	let codec: CMVideoCodecType
	let outputCallback: (CMSampleBuffer?, Error?) -> Void
	var compressionSession: VTCompressionSession!

	enum AppleEncoderError: Error {
		case failedToCreateCompressionSession(OSStatus)
		case failedToEncode(OSStatus)
		case failedToGetSupportedProperties(OSStatus)
		case failedToUpdateProperties(OSStatus)
		case failedToSendToEncoder(OSStatus)
	}

	required init(
		codec: CMVideoCodecType,
		targetDimensions: CMVideoDimensions,
		outputCallback: @escaping (CMSampleBuffer?, Error?) -> Void) throws {
		
		self.codec = codec
		self.outputCallback = outputCallback

		let callback: VTCompressionOutputCallback = { (rawContext, _, status, _, sampleBuffer) in
			let context = Unmanaged<AppleEncoder>.fromOpaque(rawContext!).takeUnretainedValue()

			if status == 0 {
				context.outputCallback(sampleBuffer, nil)
			} else {
				context.outputCallback(nil, AppleEncoderError.failedToEncode(status))
			}
		}

#if os(macOS)
		let encoderSpec: [CFString : CFTypeRef] = [
			kVTVideoEncoderSpecification_RequireHardwareAcceleratedVideoEncoder : kCFBooleanTrue as CFTypeRef
		]
#elseif os(iOS)
		// iOS always requires hardware encoders
		let encoderSpec: [CFString : CFTypeRef] = [:]
#endif

		var compressionSession_: VTCompressionSession?
		var status = VTCompressionSessionCreate(
			allocator: kCFAllocatorDefault,
			width: targetDimensions.width, height: targetDimensions.height,
			codecType: codec,
			encoderSpecification: encoderSpec as CFDictionary,
			imageBufferAttributes: nil, compressedDataAllocator: nil,
			outputCallback: callback,
			refcon: Unmanaged.passUnretained(self).toOpaque(),
			compressionSessionOut: &compressionSession_)
		guard let compressionSession = compressionSession_, status == 0 else {
			throw AppleEncoderError.failedToCreateCompressionSession(status)
		}

		self.compressionSession = compressionSession

#if os(macOS)
		var hardwareEncoding: CFBoolean?
		status = VTSessionCopyProperty(compressionSession, key: kVTCompressionPropertyKey_UsingHardwareAcceleratedVideoEncoder, allocator: kCFAllocatorDefault, valueOut: &hardwareEncoding)
		if status != 0 || !CFBooleanGetValue(hardwareEncoding) {
			print("VideoEncoder: Not using hardware encoding for resolution \(targetDimensions.width)x\(targetDimensions.height)")
		} else {
			print("VideoEncoder: Using hardware encoding for resolution \(targetDimensions.width)x\(targetDimensions.height)")
		}
#endif
	}
	
	func invalidate() {
		if compressionSession != nil {
			VTCompressionSessionInvalidate(compressionSession)
			VTCompressionSessionCompleteFrames(compressionSession, untilPresentationTimeStamp: .indefinite)
			compressionSession = nil
		}
	}
	
	deinit {
		invalidate()
	}

	var currentDimensions: CMVideoDimensions? {
		var pixelBufferAttr_: NSDictionary?
		let status = VTSessionCopyProperty(
			compressionSession,
			key: kVTCompressionPropertyKey_VideoEncoderPixelBufferAttributes,
			allocator: kCFAllocatorDefault,
			valueOut: &pixelBufferAttr_)
		guard let pixelBufferAttr = pixelBufferAttr_, status == 0 else {
			return nil
		}

		let width_ = pixelBufferAttr[kCVPixelBufferWidthKey] as? Int32
		let height_ = pixelBufferAttr[kCVPixelBufferHeightKey] as? Int32
		guard let width = width_, let height = height_ else {
			return nil
		}

		return CMVideoDimensions(width: width, height: height)
	}

	func updateSettings(
		targetFrameDuration: CMTime,
		profile: VideoProfile,
		level: Int32,
		targetBitRate: Int,
		maxPacketSize: Int,
		intraFrameRate: Int,
		videoGravity: AVLayerVideoGravity) throws {
		var scaleMode: CFString = kVTScalingMode_Trim
		switch videoGravity {
		case .resize:
			scaleMode = kVTScalingMode_Normal
		case .resizeAspect:
			scaleMode = kVTScalingMode_Letterbox
		case .resizeAspectFill:
			scaleMode = kVTScalingMode_Trim
		default:
			break
		}
		
		var profileLevel: CFString?
		switch profile {
		case .h264Baseline:
			profileLevel = kVTProfileLevel_H264_Baseline_AutoLevel
		case .h264Main:
			profileLevel = kVTProfileLevel_H264_Main_AutoLevel
		case .h264Extended:
			profileLevel = kVTProfileLevel_H264_Extended_AutoLevel
		case .h264High:
			profileLevel = kVTProfileLevel_H264_High_AutoLevel
		case .h265Main, .h263Zero:
			if codec == kCMVideoCodecType_H263 {
				if level >= 45 {
					profileLevel = kVTProfileLevel_H263_Profile0_Level45
				} else {
					profileLevel = kVTProfileLevel_H263_Profile0_Level10
				}
			} else if codec == kCMVideoCodecType_HEVC {
				if #available(OSX 10.13, iOS 11.0, *) {
					profileLevel = kVTProfileLevel_HEVC_Main_AutoLevel
				} else {
					profileLevel = nil
				}
			}
		case .h265Main10:
			if #available(OSX 10.13, iOS 11.0, *) {
				profileLevel = kVTProfileLevel_HEVC_Main10_AutoLevel
			} else {
				profileLevel = nil
			}
		}

		let properties = [
			kVTCompressionPropertyKey_AllowFrameReordering: false as Any,
			kVTCompressionPropertyKey_RealTime: true as Any,
			kVTCompressionPropertyKey_AverageBitRate: targetBitRate as Any,
			kVTCompressionPropertyKey_DataRateLimits: [targetBitRate / 8 as CFNumber, 1 as CFNumber] as CFArray,
			kVTCompressionPropertyKey_MaxKeyFrameInterval: intraFrameRate as Any,
			kVTCompressionPropertyKey_ProfileLevel: profileLevel as Any,
			kVTCompressionPropertyKey_ExpectedFrameRate: (targetFrameDuration.value != 0 ? Int64(targetFrameDuration.timescale) / targetFrameDuration.value : 0) as Any,
			kVTCompressionPropertyKey_MaxH264SliceBytes: maxPacketSize as Any,
			kVTCompressionPropertyKey_PixelTransferProperties: [kVTPixelTransferPropertyKey_ScalingMode: scaleMode] as Any
		]

		var supportedProperties_: CFDictionary?
		var status = VTSessionCopySupportedPropertyDictionary(compressionSession, supportedPropertyDictionaryOut: &supportedProperties_)
		guard var supportedProperties = supportedProperties_ as? [CFString:Any], status == 0 else {
			print("AppleEncoder: Unable to get supported compression session properties")
			throw AppleEncoderError.failedToGetSupportedProperties(status)
		}

		// In HEVC on iPhone X iOS 11.3, setting data rate limits causes the frame rate to drop to 5fps.
		if codec == kCMVideoCodecType_HEVC {
			supportedProperties.removeValue(forKey: kVTCompressionPropertyKey_DataRateLimits)
		}

		let filteredProperties = properties.filter { supportedProperties.keys.contains($0.key) }
		if filteredProperties.count < properties.count {
			let unsupportedProperties = properties.filter { !supportedProperties.keys.contains($0.key) }

			print("AppleEncoder: Can not set some properties of the encoder: \(Array(unsupportedProperties.keys))")
		}

		status = VTSessionSetProperties(compressionSession, propertyDictionary: filteredProperties as CFDictionary)
		guard status == 0 else {
			throw AppleEncoderError.failedToUpdateProperties(status)
		}
	}

	func encode(_ sampleBuffer: CMSampleBuffer, forceKeyframe: Bool) throws {

		let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer)!
		var timingInfo = CMSampleTimingInfo()
		CMSampleBufferGetSampleTimingInfo(sampleBuffer, at: 0, timingInfoOut: &timingInfo)
		var frameProperties: [CFString:Any] = [:]
		if forceKeyframe {
			frameProperties[kVTEncodeFrameOptionKey_ForceKeyFrame] = true as Any
		}

		let status = VTCompressionSessionEncodeFrame(
			compressionSession, imageBuffer: imageBuffer,
			presentationTimeStamp: timingInfo.presentationTimeStamp,
			duration: timingInfo.duration,
			frameProperties: frameProperties as CFDictionary,
			sourceFrameRefcon: nil,
			infoFlagsOut: nil)
		guard status == 0 else {
			throw AppleEncoderError.failedToSendToEncoder(status)
		}
	}

	static var hardwareCodecs: [CMVideoCodecType] = []
	static func recomputeHardwareCodecs() {
#if os(macOS)
		let encoderSpec: [CFString : CFTypeRef] = [
			kVTVideoEncoderSpecification_RequireHardwareAcceleratedVideoEncoder : kCFBooleanTrue as CFTypeRef
		]
#elseif os(iOS)
		// iOS always requires hardware encoders
		let encoderSpec: [CFString : CFTypeRef] = [:]
#endif
		let supportedCodecs = [kCMVideoCodecType_H264, kCMVideoCodecType_HEVC]

		hardwareCodecs = supportedCodecs.filter { codec in
			var compressionSession: VTCompressionSession?
			let status = VTCompressionSessionCreate(
				allocator: kCFAllocatorDefault, width: 1280, height: 720,
				codecType: codec,
				encoderSpecification: encoderSpec as CFDictionary,
				imageBufferAttributes: nil,
				compressedDataAllocator: kCFAllocatorDefault,
				outputCallback: nil, refcon: nil,
				compressionSessionOut: &compressionSession)
			guard compressionSession != nil else {
				return false
			}
			return status == 0
		}
	}

	static var supportedCodecs: [CMVideoCodecType] {
		if hardwareCodecs.isEmpty {
			recomputeHardwareCodecs()
		}

		return hardwareCodecs
	}
}
