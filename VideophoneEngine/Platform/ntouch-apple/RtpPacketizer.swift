//
//  RtpPacketizer.swift
//  ntouch
//
//  Created by Daniel Shields on 5/17/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import CoreMedia

class RtpPacketizer {

	// Muxing
	private var formatCtx: UnsafeMutablePointer<AVFormatContext>
	private var stream: UnsafeMutablePointer<AVStream>
	private var needsToWriteHeader: Bool = false

	enum PacketizerError: Error {
		case failedToCreateContext(status: Int32)
		case failedToConfigureEncoder(status: Int32)
		case failedToConfigureContext(status: Int32)
		case failedToCreateOutputBuffer(status: Int32)
		case failedToWriteHeader(status: Int32)
		case failedToWriteFrame(status: Int32)
		case failedToFlushOutput(status: Int32)
	}

	init(codec: UnsafeMutablePointer<AVCodec>) throws {		
		var formatCtx_: UnsafeMutablePointer<AVFormatContext>?
		let status = avformat_alloc_output_context2(&formatCtx_, nil, "rtp", nil)
		guard let formatCtx = formatCtx_, status >= 0 else {
			print("SoftwareEncoder: Failed to create format context")
			throw PacketizerError.failedToCreateContext(status: status)
		}

		self.formatCtx = formatCtx
		self.stream = avformat_new_stream(formatCtx, codec)
		self.formatCtx.pointee.pb = avio_alloc_context(
			av_malloc(1500).assumingMemoryBound(to: UInt8.self), 1500, 1, // 1 = writable buffer
			nil, nil, nil, nil)
		self.formatCtx.pointee.pb.pointee.max_packet_size = 1400
	}

	deinit {
		av_free(formatCtx.pointee.pb.pointee.buffer)
		var pb = formatCtx.pointee.pb
		avio_context_free(&pb)
		formatCtx.pointee.pb = pb
		avformat_free_context(formatCtx)
	}

	func configureEncoderContext(_ encoderCtx: UnsafeMutablePointer<AVCodecContext>) throws {

		var status = avcodec_parameters_from_context(stream.pointee.codecpar, encoderCtx)
		guard status >= 0 else {
			throw PacketizerError.failedToConfigureEncoder(status: status)
		}

		var options: OpaquePointer?
		defer { av_dict_free(&options) }
		av_dict_set(&options, "rtpflags", "6", 0)
		status = avformat_init_output(formatCtx, &options)
		guard status >= 0 else {
			throw PacketizerError.failedToConfigureContext(status: status)
		}
		if status == AVSTREAM_INIT_IN_WRITE_HEADER {
			needsToWriteHeader = true
		} else if status == AVSTREAM_INIT_IN_INIT_OUTPUT {
			needsToWriteHeader = false
		}
	}

	var memoryPool: CMMemoryPool = CMMemoryPoolCreate(options: nil)
	var currentBlockBuffer: CMBlockBuffer?
	func muxPacket(_ packet: UnsafeMutablePointer<AVPacket>, timebase: AVRational) throws -> CMBlockBuffer {
		// Prepare for muxing
		var blockBuffer_: CMBlockBuffer?
		var status = CMBlockBufferCreateEmpty(allocator: kCFAllocatorDefault, capacity: 3, flags: 0, blockBufferOut: &blockBuffer_)
		guard let blockBuffer = blockBuffer_, status == 0 else {
			throw PacketizerError.failedToCreateOutputBuffer(status: status)
		}
		currentBlockBuffer = blockBuffer

		func writePacket(opaque: UnsafeMutableRawPointer?, data: UnsafeMutablePointer<UInt8>?, length: Int32) -> Int32 {

			let this = Unmanaged<RtpPacketizer>.fromOpaque(opaque!).takeUnretainedValue()
			let allocator = CMMemoryPoolGetAllocator(this.memoryPool)
			let buffer = this.currentBlockBuffer!

			var lengthData: Int32 = length
			let offset = CMBlockBufferGetDataLength(buffer)
			var status = CMBlockBufferAppendMemoryBlock(
				buffer,
				memoryBlock: nil,
				length: Int(length) + MemoryLayout.size(ofValue: lengthData),
				blockAllocator: allocator,
				customBlockSource: nil,
				offsetToData: 0,
				dataLength: Int(length) + MemoryLayout.size(ofValue: lengthData),
				flags: 0)
			guard status == 0 else { print("Failed to allocate room for output \(status)"); return 0; }

			status = CMBlockBufferReplaceDataBytes(
				with: &lengthData,
				blockBuffer: buffer,
				offsetIntoDestination: offset,
				dataLength: MemoryLayout.size(ofValue: lengthData))
			guard status == 0 else { print("Failed to write output \(status)"); return 0; }
			status = CMBlockBufferReplaceDataBytes(
				with: UnsafeRawPointer(data!),
				blockBuffer: buffer,
				offsetIntoDestination: offset + MemoryLayout.size(ofValue: lengthData),
				dataLength: Int(length))
			guard status == 0 else { print("Failed to write output \(status)"); return 0; }
			return length
		}

		formatCtx.pointee.pb.pointee.write_packet = writePacket
		formatCtx.pointee.pb.pointee.opaque = Unmanaged.passUnretained(self).toOpaque()
		av_packet_rescale_ts(packet, timebase, stream.pointee.time_base)
		packet.pointee.stream_index = stream.pointee.index

		if needsToWriteHeader {
			status = avformat_write_header(formatCtx, nil)
			guard status >= 0 else {
				throw PacketizerError.failedToWriteHeader(status: status)
			}
			needsToWriteHeader = false
		}

		status = av_write_frame(formatCtx, packet)
		guard status >= 0 else {
			throw PacketizerError.failedToWriteFrame(status: status)
		}

		// Flush the frame if required
		if status != 1 {
			status = av_write_frame(formatCtx, nil)
			guard status >= 0 else {
				throw PacketizerError.failedToFlushOutput(status: status)
			}
		}

		currentBlockBuffer = nil
		return blockBuffer
	}
}
