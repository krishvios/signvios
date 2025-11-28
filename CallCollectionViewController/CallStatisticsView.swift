//
//  CallStatisticsView.swift
//  ntouch
//
//  Created by Dan Shields on 10/18/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

class CallStatisticsView: UIView {
	@IBOutlet var contentView: UIView!

	@IBOutlet var localVideoSizeLabel: UILabel!
	@IBOutlet var localVideoFrameRateLabel: UILabel!
	@IBOutlet var localVideoTargetDataRateLabel: UILabel!
	@IBOutlet var localVideoActualDataRateLabel: UILabel!
	@IBOutlet var localVideoVideoCodecLabel: UILabel!
	
	@IBOutlet var remoteVideoSizeLabel: UILabel!
	@IBOutlet var remoteVideoFrameRateLabel: UILabel!
	@IBOutlet var remoteVideoTargetDataRateLabel: UILabel!
	@IBOutlet var remoteVideoActualDataRateLabel: UILabel!
	@IBOutlet var remoteVideoVideoCodecLabel: UILabel!
	
	@IBOutlet var totalPacketsLabel: UILabel!
	@IBOutlet var packetLossCountLabel: UILabel!
	@IBOutlet var packetLossPercentLabel: UILabel!
	
	@IBOutlet var cpuUsageLabel: UILabel!
	@IBOutlet var memoryUsageLabel: UILabel!
	@IBOutlet var energyLabel: UILabel!
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		setup()
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		setup()
	}
	
	func setup() {
		Bundle.main.loadNibNamed("CallStatisticsView", owner: self)
		addSubview(contentView)
		contentView.frame = bounds
		contentView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
		contentView.layer.masksToBounds = true
		contentView.layer.cornerRadius = 8
	}
	
	var statistics: SCICallStatistics! {
		didSet {
			let codecs = [0: "None", 2: "H263", 3: "H264", 8: "H265"]
			
			localVideoSizeLabel.text = "\(statistics.recordStatistics.videoWidth)×\(statistics.recordStatistics.videoHeight)"
			localVideoSizeLabel.accessibilityIdentifier = "LocalSize"
			localVideoFrameRateLabel.text = String(format: "%.1f fps", statistics.recordStatistics.actualFrameRate)
			localVideoFrameRateLabel.accessibilityIdentifier = "LocalFPS"
			localVideoTargetDataRateLabel.text = "\(statistics.recordStatistics.targetVideoDataRate) kbps"
			localVideoTargetDataRateLabel.accessibilityIdentifier = "LocalTargetKbps"
			localVideoActualDataRateLabel.text = "\(statistics.recordStatistics.actualVideoDataRate) kbps"
			localVideoActualDataRateLabel.accessibilityIdentifier = "LocalActualKbps"
			localVideoVideoCodecLabel.text = codecs[Int(statistics.recordStatistics.videoCode)] ?? "Unknown"
			localVideoVideoCodecLabel.accessibilityIdentifier = "LocalCodec"
			
			remoteVideoSizeLabel.text = "\(statistics.playbackStatistics.videoWidth)×\(statistics.playbackStatistics.videoHeight)"
			remoteVideoSizeLabel.accessibilityIdentifier = "RemoteSize"
			remoteVideoFrameRateLabel.text = String(format: "%.1f fps", statistics.playbackStatistics.actualFrameRate)
			remoteVideoFrameRateLabel.accessibilityIdentifier = "RemoteFPS"
			remoteVideoTargetDataRateLabel.text = "\(statistics.playbackStatistics.targetVideoDataRate) kbps"
			remoteVideoTargetDataRateLabel.accessibilityIdentifier = "RemoteTargetKbps"
			remoteVideoActualDataRateLabel.text = "\(statistics.playbackStatistics.actualVideoDataRate) kbps"
			remoteVideoActualDataRateLabel.accessibilityIdentifier = "RemoteActualKbps"
			remoteVideoVideoCodecLabel.text = codecs[Int(statistics.playbackStatistics.videoCode)] ?? "Unknown"
			remoteVideoVideoCodecLabel.accessibilityIdentifier = "RemoteCodec"
			
			totalPacketsLabel.text = "\(statistics.totalVideoPackets)"
			totalPacketsLabel.accessibilityIdentifier = "TotalPackets"
			packetLossCountLabel.text = "\(statistics.lostVideoPackets + statistics.discardedOutOfOrderPackets)"
			packetLossCountLabel.accessibilityIdentifier = "PacketsLost"
			packetLossPercentLabel.text = String(format: "%.2f%%", statistics.receivedPacketsLostPercent)
			packetLossPercentLabel.accessibilityIdentifier = "PacketLoss"
			
			cpuUsageLabel.text = cpuUsage.map { String(format: "%.2f%%", $0 * 100) } ?? "Unknown"
			cpuUsageLabel.accessibilityIdentifier = "CpuUsage"
			memoryUsageLabel.text = memoryUsage.map { ByteCountFormatter.string(fromByteCount: $0, countStyle: .memory) } ?? "Unknown"
			memoryUsageLabel.accessibilityIdentifier = "MemoryUsage"
			
			let totalEnergy = self.totalEnergy.map { (value: $0, time: Date()) }
			var energyPerSecond: Double?
			if let totalEnergy = totalEnergy, let previousEnergy = previousEnergy {
				energyPerSecond = Double(totalEnergy.value - previousEnergy.value) / totalEnergy.time.timeIntervalSince(previousEnergy.time)
			}
			previousEnergy = totalEnergy
			energyLabel.text = energyPerSecond.map { String(format: "%.2f", $0 / 1e6) } ?? "Unknown"
			energyLabel.accessibilityIdentifier = "Energy"
		}
	}
	
	var cpuUsage: Double? {
		var _threadList: thread_act_array_t?
		var threadCount: mach_msg_type_number_t = 0
		let kerr = task_threads(mach_task_self_, &_threadList, &threadCount)
		
		guard let threadList = _threadList else { return nil }
		defer {
			vm_deallocate(
				mach_task_self_,
				vm_address_t(UInt(bitPattern: threadList)),
				vm_size_t(threadCount) * vm_size_t(MemoryLayout<thread_act_array_t.Pointee>.size))
		}
		
		guard kerr == KERN_SUCCESS else { return nil }
		
		return UnsafeBufferPointer(start: threadList, count: Int(threadCount)).lazy
			.compactMap { thread -> thread_basic_info? in
				var threadInfo = thread_basic_info_data_t()
				let kerr: kern_return_t = withUnsafeMutableBuffer(to: &threadInfo, reboundTo: thread_info_t.Pointee.self) {
					var count = mach_msg_type_number_t($0.count)
					return thread_info(thread, thread_flavor_t(THREAD_BASIC_INFO), $0.baseAddress!, &count)
				}
				
				return kerr == KERN_SUCCESS ? threadInfo : nil
			}
			.filter { threadInfo in
				threadInfo.flags & TH_FLAGS_IDLE == 0
			}
			.map { threadInfo in
				Double(threadInfo.cpu_usage) / Double(TH_USAGE_SCALE)
			}
			.reduce(0, +)
	}
	
	var memoryUsage: Int64? {
		var vmInfo = task_vm_info_data_t()
		let kerr: kern_return_t = withUnsafeMutableBuffer(to: &vmInfo, reboundTo: task_info_t.Pointee.self) {
			var count = mach_msg_type_number_t($0.count)
			return task_info(mach_task_self_, task_flavor_t(TASK_VM_INFO), $0.baseAddress!, &count)
		}
		
		return kerr == KERN_SUCCESS && vmInfo.phys_footprint != 0 ? Int64(vmInfo.phys_footprint) : nil
	}
	
	var previousEnergy: (value: Int64, time: Date)?
	var totalEnergy: Int64? {
		#if arch(arm) || arch(arm64)
		var powerInfo = task_power_info_v2_data_t()
		let kerr: kern_return_t = withUnsafeMutableBuffer(to: &powerInfo, reboundTo: task_info_t.Pointee.self) {
			var count = mach_msg_type_number_t($0.count)
			return task_info(mach_task_self_, task_flavor_t(TASK_POWER_INFO_V2), $0.baseAddress!, &count)
		}
		
		return kerr == KERN_SUCCESS && powerInfo.task_energy != 0 ? Int64(powerInfo.task_energy) : nil
		#else
		return nil
		#endif
	}
}

@inlinable func withUnsafeMutableBuffer<T, S, Result>(to value: inout T, reboundTo type: S.Type, _ body: (UnsafeMutableBufferPointer<S>) throws -> Result) rethrows -> Result {
	let count = MemoryLayout<T>.size / MemoryLayout<S>.size
	return try withUnsafeMutablePointer(to: &value) { pointer -> Result in
		try pointer.withMemoryRebound(to: type, capacity: count) { reboundPointer -> Result in
			try body(UnsafeMutableBufferPointer(start: reboundPointer, count: count))
		}
	}
}
