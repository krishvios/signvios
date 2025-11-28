//  Vrcl.swift
//  ntouch
//  Created on 10/29/15.
//  Copyright © 2015 Sorenson Communications. All rights reserved.
//

import Foundation
import XCTest

func commonTransform(w: UInt64 , s: UInt64, x: UInt64 ) -> UInt64 {
	let ww: UInt64 =  w & 0xffffffff
	let ww2: UInt64 = (((ww<<s)&0xffffffff | (ww>>(32-s))&0xffffffff) + x)
	let ww3: UInt64 = ww2 & 0xffffffff
	return ww3
}

func MD5StepF1(w: UInt64, x: UInt64 , y: UInt64, z : UInt64, i: UInt64, s : UInt64) -> UInt64 {
	var ww: UInt64 = w
	ww +=  (  (z ^ (x & (y ^ z))) )  + i
	return commonTransform(w: ww, s: s, x: x)
}

func MD5StepF2(w:UInt64,x:UInt64,y:UInt64,z:UInt64,i:UInt64,s:UInt64) -> UInt64{
	var ww: UInt64 = w
	ww += (y ^ (z & (x ^ y))) + i
	return commonTransform(w: ww, s: s,x: x)
}

func MD5StepF3(w: UInt64,x: UInt64,y: UInt64,z: UInt64,i: UInt64,s: UInt64) -> UInt64{
	var ww: UInt64 = w
	ww += (x ^ y ^ z) + i
	return commonTransform(w: ww,s: s, x: x)
}

func MD5StepF4(w: UInt64,x: UInt64,y: UInt64,z: UInt64,i: UInt64,s: UInt64) -> UInt64 {
	//  need to handle the overflow and &+= is not a valid identifier
	var ww: UInt64? = nil
	ww = w &+  ( ( y ^ ( x | ~z ) ) &+ i )
	return commonTransform(w: ww!,s: s, x: x)
}

func sci_transform (seed: UInt64) -> UInt64 {
	
	var a: UInt64 = (0x67452301 + seed)&0xffffffff
	var b: UInt64 = 0xefcdab89
	var c: UInt64 = 0x98badcfe
	var d: UInt64 = 0x10325476
	
	a = MD5StepF1(w: a, x:b, y:c, z:d, i:0xd76aa478, s:7)
	d = MD5StepF1(w: d, x:a, y:b, z:c, i:0xe8c7b756, s:12)
	c = MD5StepF1(w: c, x:d, y:a, z:b, i:0x242070db, s:17)
	b = MD5StepF1(w: b, x:c, y:d, z:a, i:0xc1bdceee, s:22)
	a = MD5StepF1(w: a, x:b, y:c, z:d, i:0xf57c0faf, s:7)
	d = MD5StepF1(w: d, x:a, y:b, z:c, i:0x4787c62a, s:12)
	c = MD5StepF1(w: c, x:d, y:a, z:b, i:0xa8304613, s:17)
	b = MD5StepF1(w: b, x:c, y:d, z:a, i:0xfd469501, s:22)
	a = MD5StepF1(w: a, x:b, y:c, z:d, i:0x698098d8, s:7)
	d = MD5StepF1(w: d, x:a, y:b, z:c, i:0x8b44f7af, s:12)
	c = MD5StepF1(w: c, x:d, y:a, z:b, i:0xffff5bb1, s:17)
	b = MD5StepF1(w: b, x:c, y:d, z:a, i:0x895cd7be, s:22)
	a = MD5StepF1(w: a, x:b, y:c, z:d, i:0x6b901122, s:7)
	d = MD5StepF1(w: d, x:a, y:b, z:c, i:0xfd987193, s:12)
	c = MD5StepF1(w: c, x:d, y:a, z:b, i:0xa679438e, s:17)
	b = MD5StepF1(w: b, x:c, y:d, z:a, i:0x49b40821, s:22)
	
	a = MD5StepF2(w: a, x:b, y:c, z:d, i:0xf61e2562, s:5)
	d = MD5StepF2(w: d, x:a, y:b, z:c, i:0xc040b340, s:9)
	c = MD5StepF2(w: c, x:d, y:a, z:b, i:0x265e5a51, s:14)
	b = MD5StepF2(w: b, x:c, y:d, z:a, i:0xe9b6c7aa, s:20)
	a = MD5StepF2(w: a, x:b, y:c, z:d, i:0xd62f105d, s:5)
	d = MD5StepF2(w: d, x:a, y:b, z:c, i:0x02441453, s:9)
	c = MD5StepF2(w: c, x:d, y:a, z:b, i:0xd8a1e681, s:14)
	b = MD5StepF2(w: b, x:c, y:d, z:a, i:0xe7d3fbc8, s:20)
	a = MD5StepF2(w: a, x:b, y:c, z:d, i:0x21e1cde6, s:5)
	d = MD5StepF2(w: d, x:a, y:b, z:c, i:0xc33707d6, s:9)
	c = MD5StepF2(w: c, x:d, y:a, z:b, i:0xf4d50d87, s:14)
	b = MD5StepF2(w: b, x:c, y:d, z:a, i:0x455a14ed, s:20)
	a = MD5StepF2(w: a, x:b, y:c, z:d, i:0xa9e3e905, s:5)
	d = MD5StepF2(w: d, x:a, y:b, z:c, i:0xfcefa3f8, s:9)
	c = MD5StepF2(w: c, x:d, y:a, z:b, i:0x676f02d9, s:14)
	b = MD5StepF2(w: b, x:c, y:d, z:a, i:0x8d2a4c8a, s:20)
	
	a = MD5StepF3(w: a, x:b, y:c, z:d, i:0xfffa3942, s:4)
	d = MD5StepF3(w: d, x:a, y:b, z:c, i:0x8771f681, s:11)
	c = MD5StepF3(w: c, x:d, y:a, z:b, i:0x6d9d6122, s:16)
	b = MD5StepF3(w: b, x:c, y:d, z:a, i:0xfde5380c, s:23)
	a = MD5StepF3(w: a, x:b, y:c, z:d, i:0xa4beea44, s:4)
	d = MD5StepF3(w: d, x:a, y:b, z:c, i:0x4bdecfa9, s:11)
	c = MD5StepF3(w: c, x:d, y:a, z:b, i:0xf6bb4b60, s:16)
	b = MD5StepF3(w: b, x:c, y:d, z:a, i:0xbebfbc70, s:23)
	a = MD5StepF3(w: a, x:b, y:c, z:d, i:0x289b7ec6, s:4)
	d = MD5StepF3(w: d, x:a, y:b, z:c, i:0xeaa127fa, s:11)
	c = MD5StepF3(w: c, x:d, y:a, z:b, i:0xd4ef3085, s:16)
	b = MD5StepF3(w: b, x:c, y:d, z:a, i:0x04881d05, s:23)
	a = MD5StepF3(w: a, x:b, y:c, z:d, i:0xd9d4d039, s:4)
	d = MD5StepF3(w: d, x:a, y:b, z:c, i:0xe6db99e5, s:11)
	c = MD5StepF3(w: c, x:d, y:a, z:b, i:0x1fa27cf8, s:16)
	b = MD5StepF3(w: b, x:c, y:d, z:a, i:0xc4ac5665, s:23)
	
	a = MD5StepF4(w: a, x:b, y:c, z:d, i:0xf4292244, s:6)
	d = MD5StepF4(w: d, x:a, y:b, z:c, i:0x432aff97, s:10)
	c = MD5StepF4(w: c, x:d, y:a, z:b, i:0xab9423a7, s:15)
	b = MD5StepF4(w: b, x:c, y:d, z:a, i:0xfc93a039, s:21)
	a = MD5StepF4(w: a, x:b, y:c, z:d, i:0x655b59c3, s:6)
	d = MD5StepF4(w: d, x:a, y:b, z:c, i:0x8f0ccc92, s:10)
	c = MD5StepF4(w: c, x:d, y:a, z:b, i:0xffeff47d, s:15)
	b = MD5StepF4(w: b, x:c, y:d, z:a, i:0x85845dd1, s:21)
	a = MD5StepF4(w: a, x:b, y:c, z:d, i:0x6fa87e4f, s:6)
	d = MD5StepF4(w: d, x:a, y:b, z:c, i:0xfe2ce6e0, s:10)
	c = MD5StepF4(w: c, x:d, y:a, z:b, i:0xa3014314, s:15)
	b = MD5StepF4(w: b, x:c, y:d, z:a, i:0x4e0811a1, s:21)
	a = MD5StepF4(w: a, x:b, y:c, z:d, i:0xf7537e82, s:6)
	d = MD5StepF4(w: d, x:a, y:b, z:c, i:0xbd3af235, s:10)
	c = MD5StepF4(w: c, x:d, y:a, z:b, i:0x2ad7d2bb, s:15)
	b = MD5StepF4(w: b, x:c, y:d, z:a, i:0xeb86d391, s:21)
	
	return a
}

func createKey(clientIP: UInt32, serverIP: UInt32 ) -> String {
	var new32Key = [UInt64](repeating: 0, count: 4)
	new32Key[0] = 0x532fa3db ^ UInt64(clientIP)
	
	for x in 0...2 {
	new32Key[x + 1] = sci_transform(seed:
		new32Key[x]);
	}
	
	new32Key[0] ^= UInt64(serverIP)
	
	for x in 0...2 {
	new32Key[x + 1] = sci_transform(seed: new32Key[x])
	}
	
	var newKey = [UInt8](repeating: 0 , count: 16)
	
	for x in 0...3 {
	let temp = new32Key[x];
	newKey[x * 4] =  UInt8(temp >> 24 & 0x000000ff);
	newKey[x * 4 + 1] = UInt8(temp >> 16 & 0x000000ff);
	newKey[x * 4 + 2] = UInt8(temp >> 8 & 0x000000ff);
	newKey[x * 4 + 3] = UInt8(temp & 0x000000ff);
	}
	
	let newKeyData = Data(bytes: UnsafePointer<UInt8>(newKey), count: 16)
	let base64String = newKeyData.base64EncodedString(options: NSData.Base64EncodingOptions())
	//print("b64str: \(base64String)")
	return base64String
}


func createPassword(key1: String, key2: String) throws -> String? {
	let key1decoded = Data(base64Encoded: key1, options: NSData.Base64DecodingOptions(rawValue: 0))
	let key2decoded = Data(base64Encoded: key2, options: NSData.Base64DecodingOptions(rawValue: 0))
	
	let count1  = (key1decoded?.count)! / MemoryLayout<UInt8>.size
	let count2  = (key2decoded?.count)! / MemoryLayout<UInt8>.size
	
	var key1bytearray = [UInt8](repeating: 0, count: count1)
	var key2bytearray = [UInt8](repeating: 0, count: count2)
	
	(key1decoded! as NSData).getBytes(&key1bytearray, length: count1 * MemoryLayout<UInt8>.size)  // this is good.
	(key2decoded! as NSData).getBytes(&key2bytearray, length: count2 * MemoryLayout<UInt8>.size)
	
	var newkey1buffer = [UInt64](repeating: 0, count: 4)
	var newkey2buffer = [UInt64](repeating: 0, count: 4)
	
	for x in 0...3 {
	var temp: UInt64 = 0
	temp |= UInt64(key1bytearray[x * 4 + 0]) << 0
	temp |= UInt64(key1bytearray[x * 4 + 1]) << 8
	temp |= UInt64(key1bytearray[x * 4 + 2]) << 16
	temp |= UInt64(key1bytearray[x * 4 + 3]) << 24
	newkey1buffer[x] = temp
	}
	
	for x in 0...3 {
	var temp: UInt64 = 0
	temp |= UInt64(key2bytearray[x * 4 + 0]) << 0
	temp |= UInt64(key2bytearray[x * 4 + 1]) << 8
	temp |= UInt64(key2bytearray[x * 4 + 2]) << 16
	temp |= UInt64(key2bytearray[x * 4 + 3]) << 24
	newkey2buffer[x] = temp
	}
	
	for x in 0...3 {
	newkey1buffer[x] ^= sci_transform(seed: newkey2buffer[x])
	}
	for x in 0...2 {
	newkey1buffer[ x + 1 ] ^= sci_transform(seed: newkey1buffer[x])
	}
	newkey1buffer[0] = sci_transform(seed: newkey1buffer[3])
	
	//print("newkey1buffer:  \(newkey1buffer)")
	//print("newkey2buffer:  \(newkey2buffer)")
	var newPassword = [UInt8](repeating: 0 , count: 16)
	for x in 0...3 {
	let temp = newkey1buffer[x];
	newPassword[x * 4 + 0] = UInt8(temp & 0x000000ff)
	newPassword[x * 4 + 1] = UInt8(temp >> 8 & 0x000000ff)
	newPassword[x * 4 + 2] = UInt8(temp >> 16 & 0x000000ff)
	newPassword[x * 4 + 3] = UInt8(temp >> 24 & 0x000000ff)
	
	}
	
	let newpwData = Data(bytes: UnsafePointer<UInt8>(newPassword), count: 16)
	//print("newNSData:  \(newpwData)")
	let newb64String = newpwData.base64EncodedString(options: NSData.Base64EncodingOptions([.endLineWithLineFeed]))
	//print("b64str: \(newb64String)")
	return newb64String
}

// Return IP address of WiFi interface (en0) as a String, or `nil`
func getLocalIPAddress() -> String {
	var address : String?
	
	// Get list of all interfaces on the local machine:
	var ifaddr : UnsafeMutablePointer<ifaddrs>? = nil
	if getifaddrs(&ifaddr) == 0 {
		
		// For each interface ...
		var ptr = ifaddr
		while ptr != nil {
			let interface = ptr?.pointee
			let flags = Int32((ptr?.pointee.ifa_flags)!)
			
			if (flags & (IFF_UP|IFF_RUNNING|IFF_LOOPBACK)) == (IFF_UP|IFF_RUNNING) {
				// Check for IPv4 or IPv6 interface:
				let addrFamily = interface?.ifa_addr.pointee.sa_family
				//if addrFamily == UInt8(AF_INET) || addrFamily == UInt8(AF_INET6) {
				if addrFamily == UInt8(AF_INET) {
					// Check interface name:
					if let name = String(validatingUTF8: (interface?.ifa_name)!) , name == "en0" {
						// Convert interface address to a human readable string:
						var addr = interface?.ifa_addr.pointee
						var hostname = [CChar](repeating: 0, count: Int(NI_MAXHOST))
						getnameinfo(&addr!, socklen_t((interface?.ifa_addr.pointee.sa_len)!),
							&hostname, socklen_t(hostname.count),
							nil, socklen_t(0), NI_NUMERICHOST)
						address = String(cString: hostname)
					}
				}
			}
		ptr = ptr?.pointee.ifa_next
		}
		freeifaddrs(ifaddr)
	}
	return address!
}

func ipToNetworkOrder(ip: String) -> UInt32{
	// this is not really network order
	let ipList = ip.components(separatedBy: ".")
	let ipNo = (Int(ipList[0])!*256*256*256)  + (Int(ipList[1])!*256*256) + (Int(ipList[2])!*256) + (Int(ipList[3])!)
	return UInt32(ipNo)
}

enum  NetworkErrors : Error {
	case failedToConnect
	case connect
	case connectAck
	case connectAckKey
	case connectValidate
	case connectFoo
}

enum TransformErrors : Error {
	case passwordCreateError
}

class Connection : NSObject, StreamDelegate {
	
	var inputStream: InputStream?
	var outputStream: OutputStream?
	var localIP = getLocalIPAddress()
	var remoteIP: String? = nil
	var connectAck : String? = nil
	var connected: Bool? = false
	var oepNum = ""
	
	init(host: String, port: Int = 15327) {
		super.init()
		self.remoteIP = host
		//autoreleasepool {
		//    NSStream.getStreamsToHostWithName(host, port:port, inputStream: &inputStream, outputStream: &outputStream)
		//}
		Stream.getStreamsToHost(withName: host, port:port, inputStream: &inputStream, outputStream: &outputStream)
		
		self.inputStream!.delegate = self
		
		DispatchQueue.global().async {
		
			self.inputStream!.schedule(in: .current, forMode: RunLoop.Mode.default)
			self.inputStream!.open()
		}
		self.outputStream!.delegate = self
		self.outputStream!.schedule(in: .current, forMode: RunLoop.Mode.default)
		self.outputStream!.open()
	}

	func connect() throws {
		// print("connect - nstream takes 60 seconds to report failure event ")
		let localIPUI = ipToNetworkOrder(ip: localIP)
		let remoteIPUI = ipToNetworkOrder(ip: self.remoteIP!)
		let connectKey = createKey(clientIP: localIPUI, serverIP: remoteIPUI )
		
		//dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0) )  {
		if let _ = try self.send(text: "<Connect>" + connectKey + "</Connect>") {
			if let conAck = try self.waitFor(text: "<ConnectAck") {
				if let respKey = try self.getAckKey(connectAck: conAck) {
					if let b64Password = try createPassword(key1: connectKey, key2:respKey) {
						try _ = self.send(text: "<Validate>\(b64Password)</Validate>")
						if let _ =  try waitFor(text: "<ValidateAck") {
							connected = true
						} else { throw NetworkErrors.connectValidate }
					} else { throw TransformErrors.passwordCreateError }
				} else { throw NetworkErrors.connectAckKey }
			} else { throw NetworkErrors.connectAck }
		} else { throw NetworkErrors.connect }
		//}
		
		// Get LoggedInNumber
		try _ = self.send(text: "<LoggedInPhoneNumberGet />")
		let data = try self.waitFor(text: "<LoggedInPhoneNumber>") ?? ""
		let start = data.firstIndex(of: "1") ?? data.startIndex
		let end = data.lastIndex(of: "<") ?? data.endIndex
		oepNum = String(data[start..<end])
		print("Connected: \(remoteIP ?? ""), \(oepNum)")
	}
	
	func stream(_ stream: Stream, handle streamEvent: Stream.Event) {
		print("* event *")
		switch streamEvent {
			case Stream.Event():
				print("* NSStreamEvent.None")
			case Stream.Event.openCompleted:
				print("* NSStreamEvent.OpenCompleted")
			self.connected = true
				break
			case Stream.Event.hasBytesAvailable:
				print("* HasBytesAvailable - read ")
				break
			case Stream.Event.hasSpaceAvailable:
				print("*  HasSpaceAvailable - send ")
				break
			case Stream.Event.errorOccurred:
				print("* NSStreamEvent.ErrorOccurred :  \(String(describing: stream.streamError?.localizedDescription))")
			self.connected = false
				break
			case Stream.Event.endEncountered:
				print("* NSStreamEvent.EndEncountered")
			default:
				print("* unknown")
		}
	}
	
	func send(text: String?) throws -> Int? {
		// -1 is write error
		var sendResp = -1
		let buff = [UInt8](text!.utf8)
		sendResp = self.outputStream!.write(buff, maxLength: buff.count)
		print(" > Sent: \(text!)")
		if sendResp == -1 {
			throw NetworkErrors.connect
		}
		return sendResp
	}
	
	func recv() -> String? {
		// hack until I get event processing working
		print(" < Received:")
		var buff = [UInt8](repeating: 0 , count: 1024)
		var out : String? = nil
		while inputStream!.hasBytesAvailable {
			let len = inputStream!.read(&buff , maxLength: buff.count)
			if len > 0 {
				out = NSString(bytes: &buff, length: buff.count, encoding: String.Encoding.utf8.rawValue) as String?
				print(out ?? "")
			}
		}
		return out
	}
	
	func close() {
		self.hangup()
		inputStream!.close()
		outputStream!.close()
		sleep(1)
		print("VRCL Connection closed: \(remoteIP ?? ""), \(oepNum)")
	}
	
	func status() {
		print("Status: 2:open, 6: closed,")
		print("In: \(inputStream!.streamStatus.rawValue)")
		print("Out: \(outputStream!.streamStatus.rawValue)")
	}
	
	func waitFor(text: String, assert: Bool = true) throws -> String? {
		var a = 0
		var found = false
		var data : String? = nil
		while a < 30 {
			data = recv()
			if (data?.range(of: text) != nil) {
				print(" < Found: \(text)")
				found = true
				break
			} else {
				sleep(1)
				a+=1
			}
		}
		if (assert) {
			XCTAssertTrue(found, "\(text) not found - Caller: \(testUtils.callerNum), Callee: \(testUtils.calleeNum) - \(testUtils.getTimeStamp())")
		}
		return data
	}
	
	func getAckKey(connectAck: String) throws -> String? {
		// why so complicated
		let keyStart = connectAck.index(connectAck.startIndex, offsetBy: 12)
		let keyEnd = connectAck.index(keyStart, offsetBy: 24)
		return String(connectAck[keyStart ..< keyEnd])
	}
	
	func getAppVersion() -> String {
		var appVersion = ""
		let message = "<ApplicationVersionGet />"
		do {
			try _ = self.send(text: message)
			try appVersion = self.waitFor(text: "<ApplicationVersion>") ?? ""
		}
		catch {
			print("error: get app version failed")
		}
		return appVersion
	}
	
	/*
	Name: answer()
	Description: Wait for Incoming Call and Answer
	Parameters: none
	Example: answer()
	*/
	func answer() {
		do {
			waitForIncomingCall()
			try _ = self.send(text: "<CallAnswer />")
			waitForCallConnected()
		} catch {
			print("error: Failed to send <CallAnswer />")
		}
	}
	
	/*
	Name: clearSharedText()
	Description: Clear Shared Text
	Parameters: none
	Example: clearSharedText()
	*/
	func clearSharedText() {
		do {
			try _ = self.send(text: "<TextMessageClear />")
			sleep(3)
		} catch {
			print("error: Failed to send <TextMessageClear />")
		}
	}
	
	/*
	Name: dial()
	Description: Dial a number
	Parameters: number
	Example: dial(number)
	*/
	func dial(number: String, assert: Bool = true) {
		testUtils.callerNum = oepNum
		testUtils.calleeNum = number
		let appVersion = self.getAppVersion()
		var message = "<PhoneNumberDial><DestNumber>\(number)</DestNumber><AlwaysAllow>true</AlwaysAllow></PhoneNumberDial>"
		if appVersion.contains("8.1") {
			message = "<PhoneNumberDial>\(number)</PhoneNumberDial>" // Old command for VP1
		}
		do {
			try _ = self.send(text: message)
		} catch {
			print("error: Failed to send \(message)")
		}
		if assert {
			do {
				try _ = waitFor(text: "<CallDialed>")
			} catch {
				print("error: Call Dialed Failed to Ring")
			}
		}
	}
	
	/*
	Name: hangup()
	Description: Hangup a call
	Parameters: none
	Example: hangup()
	*/
	func hangup() {
		do {
			try _ = self.send(text: "<CallHangUp />")
			sleep(5)
		} catch {
			print("error: Failed to send <CallHangUp />")
		}
	}
	
	/*
	Name: rejectCall()
	Description: Wait for Incoming Call and Reject
	Parameters: none
	Example: rejectCall()
	*/
	func rejectCall() {
		do {
			self.waitForIncomingCall()
			try _ = self.send(text: "<CallReject />")
			sleep(3)
		} catch {
			print("error: Failed to send <CallReject />")
		}
	}
	
	/*
	* Name: settingSet
	* Description: VRCL Setting Set
	* Parameters: type, name, value
	* Example settingSet(type: "Number", name: "SecureCallMode", value: "1")
	*/
	func settingSet(type: String, name: String, value: String) {
		let vrclCommand = "<SettingSet Type=\"" + type + "\" Name=\"" + name + "\">" + value + "</SettingSet>"
		do {
			try _ = self.send(text: vrclCommand)
			usleep(useconds_t(100))
		}
		catch {
			print("error: Failed to send <Setting Set: \(vrclCommand)")
		}
	}
	
	/*
	Name: shareText(text:)
	Description: Share text
	Parameters: text
	Example: shareText(text: "1234")
	*/
	func shareText(text: String) {
		do {
			try _ = self.send(text: "<TextMessageSend>" + text + "</TextMessageSend>")
			sleep(2)
		} catch {
			print("error: Failed to send <TextMessageSend>" + text + "</TextMessageSend>")
		}
	}
	
	/*
	Name: statusCheck()
	Description: Perform a status check
	Parameters: none
	Example: statusCheck()
	*/
	func statusCheck() {
		do {
			try _ = self.send(text: "<StatusCheck />")
		} catch {
			print("error: Failed to send <StatusCheck />")
		}
	}
	
	/*
	Name: transferCall()
	Description: Transfer call to a number
	Parameters: number
	Example: transferCall(number)
	*/
	func transferCall(number: String) {
		testUtils.callerNum = oepNum
		testUtils.calleeNum = number
		do {
			try _ = self.send(text: "<CallTransfer>" + number + "</CallTransfer>")
		} catch {
			print("error: Failed to send <CallTransfer>" + number + "</CallTransfer>")
		}
	}
	
	/*
	Name: videoPrivacy(setting:)
	Description: Turn Video Privacy On of Off
	Parameters: setting ("On" or "Off")
	Example: videoPrivacy(setting: "On")
	*/
	func videoPrivacy(setting: String) {
		do {
			if setting == "On" {
				try _ = self.send(text: "<VideoPrivacySet>On</VideoPrivacySet>")
			}
			else {
				try _ = send(text: "<VideoPrivacySet>Off</VideoPrivacySet>")
			}
			sleep(3)
		} catch {
			print("error: Failed to send <VideoPrivacySet>" + setting + "</VideoPrivacySet>")
		}
	}
	
	/*
	Name: waitForCallConnected()
	Description: Wait for CallConnected
	Parameters: none
	Example: waitForCallConnected()
	*/
	func waitForCallConnected() {
		do {
			try _ = waitFor(text: "<CallConnected")
		} catch {
			print("error: Failed to find <CallConnected")
		}
	}
	
	/*
	Name: waitForCallConnected(callerID)
	Description: Wait for Call Connected by Caller ID
	Parameters: callerID
	Example: waitForCallConnected(callerID: "No Caller ID")
	*/
	func waitForCallConnected(callerID: String) {
		do {
			try _ = waitFor(text: callerID)
		} catch {
			print("error: Failed to find \(callerID)")
		}
	}
	
	/*
	Name: waitForIncomingCall()
	Description: Wait for an Incoming Call
	Parameters: none
	Example: waitForIncomingCall()
	*/
	func waitForIncomingCall() {
		var a = 0
		var incomingCall = false
		var data : String? = nil
		while a < 30 {
			statusCheck()
			data = recv()
			if (data?.range(of: "CallIncoming") != nil || data?.range(of: "IncomingCall") != nil) {
				print(" < Found: CallIncoming")
				incomingCall = true
				break
			} else {
				sleep(1)
				a+=1
			}
		}
		XCTAssertTrue(incomingCall, "IncomingCall not found - Caller: \(testUtils.callerNum), Callee: \(testUtils.calleeNum) - \(testUtils.getTimeStamp())")
	}
	
	/*
	Name: waitForIncomingCall(callerID:)
	Description: Wait for an Incoming Call by Caller ID
	Parameters: callerID
	Example: waitForIncomingCall(callerID: "No Caller ID")
	*/
	func waitForIncomingCall(callerID: String) {
		do {
			try _ = waitFor(text: "<CallIncoming>" + callerID)
		} catch {
			print("error: Failed to find <CallIncoming>" + callerID)
		}
	}
}

