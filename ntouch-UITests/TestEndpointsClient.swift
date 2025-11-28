//
//  TestEndpointsClient.swift
//  ntouchUITests
//
//  Created by mmccoy on 9/9/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import XCTest

enum EndpointTypes : String {
	case vp = "VP"
	case vp2 = "VP2"
	case vp3 = "VP3"
	case pc = "PC"
	case mac = "Mac"
	case ios = "iOS"
	case android = "Android"
	case hearing = "hearing device"
	case callWaitingOff = "call waiting off device"
}

var checkedOutEndpoints = [Endpoint]()
var vrclConnections = [Connection]()

class TestEndpointsClient {
	static let shared = TestEndpointsClient()
	let session = URLSession.shared
	let scheme = "http"
	let host = "10.20.133.99"
	let basePath = "/TestConfig/api/Endpoints"
	let requestTimeOut = 5.0
	
	func checkOut(_ endpointType: EndpointType? = nil, callingFunction: String = #function) -> Endpoint {
		let maxRetries = 10
		var rv = Endpoint()
		var checkedOut = false
		var method = "CheckOut"
		if let endpointType = endpointType {
			if endpointType == EndpointType.tollFree {
				method += "TollFree"
			}
			else {
				method += "ByType/\(String(endpointType.rawValue))"
			}
		}
		method += "/\(UserConfig()["Phone"]!)/\(callingFunction)"
		
		for _ in 1...maxRetries {
			print(method)
			self.makeRequest(method: method, completion: { result -> Void in
				switch result {
				case let .success(endpoint):
					if let address = endpoint.address, let number = endpoint.number {
						print("Check out endpoint: \(address), \(number)")
						checkedOut = true
					}
					rv = endpoint
				case let .failure(error):
					print("Test endpoint checkout error: \(error.localizedDescription) \(#file) \(#function) \(#line)")
				}
			})
			if checkedOut {
				break
			}
			else {
				print("Checkout failed, trying again in 1 minute.")
				sleep(60)
			}
		}
		if !checkedOut {
			XCTFail("Test endpoint checkout failed - \(testUtils.getTimeStamp())")
		}
		return rv
	}
	
	private func makeRequest(method: String, completion: @escaping (EndpointResult) -> Void) {
		let expectation = XCTestExpectation(description: "\(#function)\(#line)")
		if let request = createRequest(method: method) {
			let task = session.dataTask(with: request, completionHandler: { (data, _, error) in
				completion(self.processEndpointRequest(data: data, error: error))
				expectation.fulfill()
			})
			task.resume()
			switch XCTWaiter.wait(for: [expectation], timeout: requestTimeOut, enforceOrder: false) {
				case .completed:
					print("Test endpoint checkout completed")
				case .timedOut:
					print("Test endpoint checkout timeout of \(String(requestTimeOut)) reached \(#file) \(#function) \(#line)")
				default:
					print("Test endpoint function async waiter failed. \(#file) \(#function) \(#line)")
			}
		}
	}
	
	func checkIn(_ id: Int?) {
		if let id = id, let request = createRequest(method: "CheckIn/\(String(id))") {
			let task = session.dataTask(with: request, completionHandler: { (_, response, error) in
				guard error == nil else {
					if let error = error {
						// We don't care enough about check-in errors to XCTFail() here
						// So just log it to the console
						print("checkIn error: \(error.localizedDescription)")
					}
					return
				}
			})
			task.resume()
		}
	}
	
	private func createRequest(method: String) -> URLRequest? {
		guard let url = createBaseURLComponents(withPath: method).url else { return nil }
		var request = URLRequest(url: url)
		request.httpMethod = "GET"
		request.setValue("text/plain", forHTTPHeaderField: "accept")
		request.setValue("application/json; charset=utf-8", forHTTPHeaderField: "Content-Type")
		return request
	}
	
	private func createBaseURLComponents(withPath path: String) -> URLComponents {
		var components = URLComponents()
		components.scheme = scheme
		components.host = host
		components.path = basePath + "/" + path
		return components
	}
	
	private func processEndpointRequest(data: Data?, error: Error?) -> EndpointResult {
		do {
			guard let endpointData = data, let endpoint = try JSONDecoder().decode(Endpoint.self, from: endpointData) as Endpoint? else {
				if data == nil {
					return .failure(error!)
				} else {
					return .failure(EndpointError.jsonDecodeError)
				}
			}
			checkedOutEndpoints.append(endpoint)
			return .success(endpoint)
		} catch let error {
			return .failure(error)
		}
	}
	
	func tryToConnectToEndpoint(_ endpoint: Endpoint) -> Connection? {
		guard let address = endpoint.address else {
			XCTFail("Endpoint has no IP address to connect to.")
			return nil
		}
		do {
			let connection = Connection(host: address)
			try connection.connect()
			vrclConnections.append(connection)
			return connection
		}
		catch {
			XCTFail("Unable to connect to \(endpoint.address ?? "endpoint").")
			return nil
		}
	}
	
	func tryToConnectToEndpoint(ip: String) -> Connection? {
		do {
			let connection = Connection(host: ip)
			try connection.connect()
			vrclConnections.append(connection)
			return connection
		} catch {
			XCTFail("Unable to connect to \(ip).")
			return nil
		}
	}
	
	func checkInEndpoints() {
		for endpoint in checkedOutEndpoints {
			self.checkIn(endpoint.id)
			if let address = endpoint.address, let number = endpoint.number {
				print("Check in endpoint: \(address), \(number)")
			}
		}
		checkedOutEndpoints.removeAll()
	}
	
	func closeVrclConnections() {
		vrclConnections.forEach { $0.close() }
		vrclConnections.removeAll()
	}
}

class Endpoint: Codable {
	var id: Int?
	var description: String?
	var number: String?
	var tollFreeNumber: String?
	var address: String?
	var state: Int?
	var endpointType: EndpointType?
	var lastStatusCheckTime: String?
	var inMyPhoneGroup: Bool?
	var allowCheckOut: Bool?
	var supportsHEVC: Bool?
	var userName: String?
	
	private enum CodingKeys: String, CodingKey {
		case id
		case description
		case number
		case tollFreeNumber
		case address
		case state
		case endpointType
		case lastStatusCheckTime
		case inMyPhoneGroup
		case allowCheckOut
		case supportsHEVC
		case userName
	}
	
	func encode(to encoder: Encoder) throws {
		var container = encoder.container(keyedBy: CodingKeys.self)
		try container.encode(id, forKey: .id)
		try container.encode(description, forKey: .description)
		try container.encode(number, forKey: .number)
		try container.encode(tollFreeNumber, forKey: .tollFreeNumber)
		try container.encode(address, forKey: .address)
		try container.encode(state, forKey: .state)
		try container.encode(endpointType, forKey: .endpointType)
		try container.encode(lastStatusCheckTime, forKey: .lastStatusCheckTime)
		try container.encode(inMyPhoneGroup, forKey: .inMyPhoneGroup)
		try container.encode(allowCheckOut, forKey: .allowCheckOut)
		try container.encode(supportsHEVC, forKey: .supportsHEVC)
		try container.encode(userName, forKey: .userName)
	}
}

enum EndpointType: Int, Codable {
	case vp1
	case vp2
	case ios
	case android
	case pc
	case mac
	case vp3
	case tollFree
}

enum EndpointResult {
	case success(Endpoint)
	case failure(Error)
}

enum EndpointError: Error {
	case jsonDecodeError
}
