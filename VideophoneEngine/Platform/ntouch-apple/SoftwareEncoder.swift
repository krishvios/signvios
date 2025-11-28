//
//  SoftwareEncoder.swift
//  ntouch
//
//  Created by Daniel Shields on 4/4/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import AVFoundation
import CoreMedia

// Sometimes swift struggles with importing C macros...
let AV_NOPTS_VALUE = Int64(bitPattern: 0x8000000000000000)

///
/// SoftwareEncoder is implemented using libavcodec and libavformat. It encodes
/// H263 video using libavcodec, rescaling it if necessary using libswscale, and
/// packetizes (muxes) it using libavformat.
///
class SoftwareEncoder: VideoEncoder {
	var codec: CMVideoCodecType
	var outputCallback: (CMSampleBuffer?, Error?) -> Void

	let targetDimensions: CMVideoDimensions

	let encoder: UnsafeMutablePointer<AVCodec>
	var encoderCtx: UnsafeMutablePointer<AVCodecContext>?
	var packetizer: RtpPacketizer?

	enum SoftwareEncoderError: Error {
		case unsupportedCodec
		case failedToAllocEncoderContext
		case failedToOpenEncoderContext(status: Int32)
		case requiresOpenEncoderContext
		case failedToCreateOutputBuffer(status: OSStatus)
		case failedToCreateFormatDescription(status: Int32)
		case failedToCreateSampleBuffer(status: Int32)
		case failedToSendToEncoder(status: Int32)
	}

	required init(
		codec: CMVideoCodecType,
		targetDimensions: CMVideoDimensions,
		outputCallback: @escaping (CMSampleBuffer?, Error?) -> Void) throws {
		self.codec = codec
		self.outputCallback = outputCallback
		self.targetDimensions = targetDimensions

		var codecId: AVCodecID?
		switch codec { 
		case kCMVideoCodecType_H263:
			codecId = AV_CODEC_ID_H263
		case kCMVideoCodecType_H264:
			codecId = AV_CODEC_ID_H264
		case kCMVideoCodecType_HEVC:
			codecId = AV_CODEC_ID_HEVC
		default:
			codecId = nil
		}

		if let codecId = codecId {
			self.encoder = avcodec_find_encoder(codecId)
		} else {
			throw SoftwareEncoderError.unsupportedCodec
		}

		// The videophone engine only requires us to packetize H263 data.
		if codec == kCMVideoCodecType_H263 {
			packetizer = try RtpPacketizer(codec: self.encoder)
		}
	}
	
	func invalidate() {
	}

	deinit {
		invalidate()
		
		var frame: UnsafeMutablePointer<AVFrame>? = self.frame
		av_frame_free(&frame)

		if let encoderCtx = encoderCtx {
			avcodec_close(encoderCtx)
			avcodec_free_context(&self.encoderCtx)
			self.encoderCtx = nil
		}

		scaledData?.deallocate()
		if scalingCtx != nil {
			sws_freeContext(scalingCtx)
		}
	}

	func updateSettings(
		targetFrameDuration: CMTime,
		profile: VideoProfile,
		level: Int32,
		targetBitRate: Int,
		maxPacketSize: Int,
		intraFrameRate: Int,
		videoGravity: AVLayerVideoGravity) throws {
		// Close and free the current encoder context if it exists
		if let encoderCtx = encoderCtx {
			avcodec_close(encoderCtx)
			avcodec_free_context(&self.encoderCtx)
			self.encoderCtx = nil
		}

		encoderCtx = avcodec_alloc_context3(encoder)
		guard let encoderCtx = encoderCtx else {
			print("SoftwareEncoder: Failed to alloc software encoder context")
			throw SoftwareEncoderError.failedToAllocEncoderContext
		}

		// Frame format
		encoderCtx.pointee.pix_fmt = AV_PIX_FMT_YUV420P
		encoderCtx.pointee.width = targetDimensions.width
		encoderCtx.pointee.height = targetDimensions.height

		// Set frame duration
		encoderCtx.pointee.time_base.den = targetFrameDuration.timescale
		encoderCtx.pointee.time_base.num = Int32(targetFrameDuration.value)

		// Set bitrate/quality
		encoderCtx.pointee.bit_rate = Int64(targetBitRate * 3 / 4)
		encoderCtx.pointee.bit_rate_tolerance = Int32(targetBitRate / 2)
		encoderCtx.pointee.rc_min_rate = 0
		encoderCtx.pointee.rc_max_rate = Int64(targetBitRate)
		encoderCtx.pointee.rc_buffer_size = Int32(targetBitRate * 2)

		// Frame settings
		encoderCtx.pointee.gop_size = intraFrameRate == 0 || intraFrameRate == Int.max ? 250 : Int32(intraFrameRate)
		encoderCtx.pointee.keyint_min = 30 // Don't send a keyframe more than once a second
		encoderCtx.pointee.max_b_frames = 0 // Don't send B-frames

		// Ensure the packet can be split up into smaller peices by including
		// macroblock boundary information every 1380 bytes.
		var options: OpaquePointer?
		defer { av_dict_free(&options) }

		encoderCtx.pointee.level = level
		encoderCtx.pointee.profile = profile.rawValue

		switch codec {
		case kCMVideoCodecType_H263:
			configureH263(options: &options)
		case kCMVideoCodecType_H264:
			configureH264(options: &options)
		case kCMVideoCodecType_HEVC:
			configureH265(options: &options)
		default:
			break
		}

		let status = avcodec_open2(encoderCtx, encoder, &options)
		guard status >= 0 else {
			avcodec_free_context(&self.encoderCtx)
			self.encoderCtx = nil
			throw SoftwareEncoderError.failedToOpenEncoderContext(status: status)
		}

		do {
			try packetizer?.configureEncoderContext(encoderCtx)
		} catch {
			avcodec_free_context(&self.encoderCtx)
			self.encoderCtx = nil
			throw error
		}
	}

	func configureH263(options: inout OpaquePointer?) {
		av_dict_set(&options, "mb_info", "1300", 0)
		av_dict_set(&options, "qsquish", "0", 0)
		av_dict_set(&options, "rc_eq", "1", 0)
	}

	func configureH264(options: inout OpaquePointer?) {
		encoderCtx?.pointee.flags |= AV_CODEC_FLAG_LOOP_FILTER // Enable deblocking filter
		encoderCtx?.pointee.thread_type = FF_THREAD_SLICE // Enable multithreaded encoding of slices
		av_dict_set(&options, "preset", "ultrafast", 0)
		av_dict_set(&options, "tune", "fastdecode,zerolatency", 0) 
		av_dict_set(&options, "slice-max-size", "1300", 0)
		av_dict_set(&options, "forced-idr", "1", 0)

		// annexb=0: We don't want annex b, which is NAL start codes instead of NAL unit sizes
		// deblock=1: Enable deblocking filter. fastdecode and zerolatency override our setting so we have to set it again.
		av_dict_set(&options, "x264-params", "annexb=0:deblock=1", 0)
	}

	func configureH265(options: inout OpaquePointer?) {
		av_dict_set(&options, "preset", "ultrafast", 0)
		av_dict_set(&options, "tune", "fastdecode,zerolatency", 0) 
		av_dict_set(&options, "forced-idr", "1", 0)
		av_dict_set(&options, "x265-params", "annexb=0", 0)
	}

	var frame: UnsafeMutablePointer<AVFrame> = av_frame_alloc()
	func createFrameWithSampleBuffer(_ sampleBuffer: CMSampleBuffer) -> UnsafeMutablePointer<AVFrame> {
		// Here we decide if we can use the CMSampleBuffer's buffers directly or
		// if we need to create our own AVFrame.
		let format = CMSampleBufferGetFormatDescription(sampleBuffer)!
		let captureDimensions = CMVideoFormatDescriptionGetDimensions(format)
		let capturePixelFormat = CMFormatDescriptionGetMediaSubType(format)

		frame.pointee.width = targetDimensions.width
		frame.pointee.height = targetDimensions.height
		frame.pointee.format = AV_PIX_FMT_YUV420P.rawValue
		frame.pointee.color_range = (capturePixelFormat == kCVPixelFormatType_420YpCbCr8PlanarFullRange ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG)

		var presentationTime = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
		presentationTime = CMTimeConvertScale(presentationTime, timescale: encoderCtx!.pointee.time_base.den, method: .quickTime)
		frame.pointee.pts = presentationTime.value / Int64(encoderCtx!.pointee.time_base.num)

		let supportedPixelFormats = [
			kCVPixelFormatType_420YpCbCr8Planar,
			kCVPixelFormatType_420YpCbCr8PlanarFullRange
		]

		let pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer)!
		if captureDimensions != targetDimensions || !supportedPixelFormats.contains(capturePixelFormat) {
			// We have to do a format conversion
			rescalePixelBufferIntoFrame(pixelBuffer, frame)
		} else {
			// We can use the sample buffer's buffers directly
			frame.pointee.opaque = Unmanaged.passRetained(pixelBuffer).toOpaque()

			CVPixelBufferLockBaseAddress(pixelBuffer, .readOnly)
			frame.pointee.data.0 = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0)?.assumingMemoryBound(to: UInt8.self)
			frame.pointee.data.1 = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1)?.assumingMemoryBound(to: UInt8.self)
			frame.pointee.data.2 = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 2)?.assumingMemoryBound(to: UInt8.self)
			frame.pointee.linesize.0 = Int32(CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0))
			frame.pointee.linesize.1 = Int32(CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1))
			frame.pointee.linesize.2 = Int32(CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 2))
		}

		return frame
	}

	func cleanupFrame(_ frame: UnsafeMutablePointer<AVFrame>) {
		if frame.pointee.opaque != nil {
			let pixelBuffer = Unmanaged<CVPixelBuffer>.fromOpaque(frame.pointee.opaque).takeRetainedValue()
			frame.pointee.opaque = nil
			CVPixelBufferUnlockBaseAddress(pixelBuffer, .readOnly)
		}
	}

	func createSampleBuffer(data: CMBlockBuffer, timing: CMSampleTimingInfo, isKeyframe: Bool) throws -> CMSampleBuffer {
		// Using target dimensions here would be a problem if we had a frame
		// delay on the encoder. We don't use B-frames so we don't have a
		// frame delay.
		var formatDesc_: CMVideoFormatDescription?
		var status = CMVideoFormatDescriptionCreate(
			allocator: kCFAllocatorDefault,
			codecType: codec,
			width: targetDimensions.width,
			height: targetDimensions.height,
			extensions: nil, formatDescriptionOut: &formatDesc_)
		guard let formatDesc = formatDesc_, status == 0 else {
			throw SoftwareEncoderError.failedToCreateFormatDescription(status: status)
		}

		var sampleTiming = timing
		var sampleSize = CMBlockBufferGetDataLength(data)
		var sampleBuffer_: CMSampleBuffer?
		status = CMSampleBufferCreate(
			allocator: kCFAllocatorDefault,
			dataBuffer: data, dataReady: true, makeDataReadyCallback: nil, refcon: nil,
			formatDescription: formatDesc, sampleCount: 1,
			sampleTimingEntryCount: 1, sampleTimingArray: &sampleTiming,
			sampleSizeEntryCount: 1, sampleSizeArray: &sampleSize,
			sampleBufferOut: &sampleBuffer_)
		guard let sampleBuffer = sampleBuffer_, status == 0 else {
			throw SoftwareEncoderError.failedToCreateSampleBuffer(status: status)
		}

		let attachmentsArray = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, createIfNecessary: true)! as NSArray
		let attachments = attachmentsArray[0] as? NSMutableDictionary
		attachments?[kCMSampleAttachmentKey_DependsOnOthers] = (isKeyframe ? kCFBooleanFalse : kCFBooleanTrue)
		attachments?[kCMSampleAttachmentKey_NotSync] = (isKeyframe ? kCFBooleanFalse : kCFBooleanTrue)

		return sampleBuffer
	}

	func encode(_ sampleBuffer: CMSampleBuffer, forceKeyframe: Bool) throws {

		guard let encoderCtx = self.encoderCtx else {
			throw SoftwareEncoderError.requiresOpenEncoderContext
		}

		// Send a frame to be encoded
		let frame = createFrameWithSampleBuffer(sampleBuffer)
		frame.pointee.key_frame = forceKeyframe ? 1 : 0
		frame.pointee.pict_type = forceKeyframe ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE
		let status = avcodec_send_frame(encoderCtx, frame)
		guard status >= 0 else {
			throw SoftwareEncoderError.failedToSendToEncoder(status: status)
		}
		cleanupFrame(frame)

		// Retrieve any encoded frames
		var packet: AVPacket = AVPacket()
		while avcodec_receive_packet(encoderCtx, &packet) == 0 {

			var sampleTiming = CMSampleTimingInfo()
			if packet.dts != AV_NOPTS_VALUE {
				sampleTiming.decodeTimeStamp = CMTime(
					value: packet.dts * Int64(encoderCtx.pointee.time_base.num),
					timescale: encoderCtx.pointee.time_base.den)
			}
			if packet.pts != AV_NOPTS_VALUE {
				sampleTiming.presentationTimeStamp = CMTime(
					value: packet.pts * Int64(encoderCtx.pointee.time_base.num),
					timescale: encoderCtx.pointee.time_base.den)
			}
			if packet.duration != 0 {
				sampleTiming.duration = CMTime(
					value: packet.duration * Int64(encoderCtx.pointee.time_base.num),
					timescale: encoderCtx.pointee.time_base.den)
			}

			let isKeyframe = ((packet.flags & AV_PKT_FLAG_KEY) != 0)
			var buffer: CMBlockBuffer?
			if let packetizer = packetizer {
				buffer = try packetizer.muxPacket(&packet, timebase: encoderCtx.pointee.time_base)
			} else {
				let copiedPacket = av_packet_clone(&packet)!
				var blockSource = CMBlockBufferCustomBlockSource()
				blockSource.version = kCMBlockBufferCustomBlockSourceVersion
				blockSource.refCon = UnsafeMutableRawPointer(copiedPacket)
				blockSource.FreeBlock = { refCon, _, _ in
					let packet = refCon?.assumingMemoryBound(to: AVPacket.self)
					av_packet_unref(packet)
				}

				let status = CMBlockBufferCreateWithMemoryBlock(
					allocator: kCFAllocatorDefault,
					memoryBlock: copiedPacket.pointee.data,
					blockLength: Int(copiedPacket.pointee.size),
					blockAllocator: kCFAllocatorNull,
					customBlockSource: &blockSource,
					offsetToData: 0, dataLength: Int(copiedPacket.pointee.size),
					flags: 0, blockBufferOut: &buffer)
				guard buffer != nil && status == 0 else {
					throw SoftwareEncoderError.failedToCreateOutputBuffer(status: status)
				}
			}

			let sampleBuffer = try createSampleBuffer(
				data: buffer!, 
				timing: sampleTiming,
				isKeyframe: isKeyframe)
			outputCallback(sampleBuffer, nil)
		}
	}

	static var supportedCodecs: [CMVideoCodecType] {
		var codecs: [CMVideoCodecType] = []
		var state: UnsafeMutableRawPointer?
		while let codec = av_codec_iterate(&state) {
			if codec.pointee.id == AV_CODEC_ID_H264 {
				codecs.append(kCMVideoCodecType_H264)
			} else if codec.pointee.id == AV_CODEC_ID_HEVC {
				codecs.append(kCMVideoCodecType_HEVC)
			}
		}

		return codecs
	}

	// MARK: Scaling

	var scalingCtx: OpaquePointer?
	var sourceDimensions: CMVideoDimensions?
	var scaledData: UnsafeMutableRawBufferPointer?

	func cropDimensions(_ sourceDimensions: CMVideoDimensions, targetAspectRatio: CMVideoDimensions) -> CMVideoDimensions {
		var result = sourceDimensions
		let alignment: Int32 = 16
		if sourceDimensions.width * targetAspectRatio.height > sourceDimensions.height * targetAspectRatio.width {
			result.width = (sourceDimensions.height * targetAspectRatio.width / targetAspectRatio.height) & ~(alignment - 1)
		} else if sourceDimensions.width * targetAspectRatio.height < sourceDimensions.height * targetAspectRatio.width {
			result.height = (sourceDimensions.width * targetAspectRatio.height / targetAspectRatio.width) & ~(alignment - 1)
		}

		return result
	}

	func rescalePixelBufferIntoFrame(_ pixelBuffer: CVPixelBuffer, _ frame: UnsafeMutablePointer<AVFrame>) {

		// Check if the source dimensions are different than the sample buffer we have.
		// If it is, we need to destroy some invalidated data structures.
		let sampleDimensions = CMVideoDimensions(
			width: Int32(CVPixelBufferGetWidth(pixelBuffer)),
			height: Int32(CVPixelBufferGetHeight(pixelBuffer)))
		let croppedDimensions = cropDimensions(sampleDimensions, targetAspectRatio: targetDimensions)
		if let sourceDimensions = sourceDimensions, sourceDimensions != croppedDimensions {
			if let scalingCtx = scalingCtx {
				sws_freeContext(scalingCtx)
				self.scalingCtx = nil
			}
		}
		sourceDimensions = croppedDimensions
		let crop = CMVideoDimensions(
			width: sampleDimensions.width - croppedDimensions.width,
			height: sampleDimensions.height - croppedDimensions.height)

		let dataSize = av_image_get_buffer_size(
			AV_PIX_FMT_YUV420P,
			targetDimensions.width,
			targetDimensions.height,
			Int32(MemoryLayout<SIMD4<Int32>>.alignment))
		if scaledData == nil || scaledData!.count < dataSize {
			scaledData?.deallocate()
			scaledData = UnsafeMutableRawBufferPointer.allocate(
				byteCount: Int(dataSize),
				alignment: MemoryLayout<SIMD4<Int32>>.alignment)
		}

		if scalingCtx == nil {
			scalingCtx = sws_getContext(
				croppedDimensions.width, croppedDimensions.height, AV_PIX_FMT_NV12,
				targetDimensions.width, targetDimensions.height, AV_PIX_FMT_YUV420P,
				SWS_POINT, nil, nil, nil)
		}

		CVPixelBufferLockBaseAddress(pixelBuffer, .readOnly)

		let srcStrides: [Int32] = [
			Int32(CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0)),
			Int32(CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1))
		]
		let srcSlices: [UnsafePointer<UInt8>?] = [
			UnsafeRawPointer(CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0)!)
				.assumingMemoryBound(to: UInt8.self)
				.advanced(by: Int(crop.width) / 2)
				.advanced(by: Int(srcStrides[0] * crop.height / 2)),
			UnsafeRawPointer(CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1)!)
				.assumingMemoryBound(to: UInt8.self)
				.advanced(by: Int(crop.width) / 2)
				.advanced(by: Int(srcStrides[1] * crop.height / 4))
		]

		let dstBuffer = scaledData?.baseAddress?.assumingMemoryBound(to: UInt8.self)
		let dstSlices: [UnsafeMutablePointer<UInt8>?] = [
			dstBuffer,
			dstBuffer?.advanced(by: Int(targetDimensions.width) * Int(targetDimensions.height)),
			dstBuffer?.advanced(by: Int(targetDimensions.width) * Int(targetDimensions.height) * 5 / 4)
		]
		let dstStrides: [Int32] = [
			targetDimensions.width,
			targetDimensions.width / 2,
			targetDimensions.width / 2
		]

		sws_scale(
			scalingCtx!,
			srcSlices, srcStrides,
			0, croppedDimensions.height,
			dstSlices, dstStrides)

		CVPixelBufferUnlockBaseAddress(pixelBuffer, .readOnly)

		frame.pointee.width = targetDimensions.width
		frame.pointee.height = targetDimensions.height
		frame.pointee.format = AV_PIX_FMT_YUV420P.rawValue
		frame.pointee.data.0 = dstSlices[0]
		frame.pointee.data.1 = dstSlices[1]
		frame.pointee.data.2 = dstSlices[2]
		frame.pointee.linesize.0 = dstStrides[0]
		frame.pointee.linesize.1 = dstStrides[1]
		frame.pointee.linesize.2 = dstStrides[2]
	}
}

extension CMVideoDimensions: Equatable {
	public static func == (a: CMVideoDimensions, b: CMVideoDimensions) -> Bool {
		return a.width == b.width && a.height == b.height
	}
}
