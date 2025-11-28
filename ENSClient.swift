//
//  ENSClient.swift
//  ntouch
//
//  Created by Dan Shields on 12/4/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

struct HTTPError: LocalizedError {
	var request: URLRequest
	var response: HTTPURLResponse
	var responseBody: Data?

	var errorDescription: String? {
		return """
		Request to \(request.url!) failed with status \(response.statusCode): \(HTTPURLResponse.localizedString(forStatusCode: response.statusCode))
		Request body: \(request.httpBody.flatMap { String(bytes: $0, encoding: .utf8) } ?? "")
		Response body: \(responseBody.flatMap { String(bytes: $0, encoding: .utf8) } ?? "")
		"""
	}
}

/// An Endpoint Notification Service client
public class ENSClient {

	public enum Error: Swift.Error {
		case badResponse
		case ensOffline(message: String)
	}

	private struct PhoneNumber: Encodable {
		enum CodingKeys: String, CodingKey {
			case isShared = "IsSharedNumber"
			case phoneNumber = "PhoneNumber"
		}

		var isShared: Bool
		var phoneNumber: String
	}

	private enum DeviceType: Int, Codable {
		case iOS = 1
		case android = 2
		case windows = 3
		case macOS = 4
		case watchOS = 5
	}

	private struct Device: Codable {
		enum CodingKeys: String, CodingKey {
			case token = "Token"
			case type = "Type"
		}

		var token: String
		var type: DeviceType
	}

	private struct RegisterRequestBody: Encodable {
		enum CodingKeys: String, CodingKey {
			case deviceToken = "DeviceToken"
			case deviceType = "ClientType"
			case devices = "Devices"
			case ringCount = "RingCount"
			case isDevelopment = "Development"
			case phoneNumbers = "Registrations"
			case sipServer = "SIPServerIP"
		}

		var deviceToken: String
		var deviceType: DeviceType
		// In the future, we'll send the primary device and type in this list. For now, this list is just for additional devices like the watch.
		var devices: [Device]
		var ringCount: Int
		var isDevelopment: Bool
		var phoneNumbers: [PhoneNumber]
		var sipServer: String?
	}

	private struct UnregisterRequestBody: Encodable {
		enum CodingKeys: String, CodingKey {
			case deviceToken = "DeviceToken"
			case phoneNumbers = "Registrations"
			case devices = "Devices"
		}

		var deviceToken: String
		var phoneNumbers: [PhoneNumber]
		var devices: [Device]
	}

	private struct RespondToCallRequestBody: Encodable {
		enum CodingKeys: String, CodingKey {
			case identifier = "ENSIdentifier"
			case callID = "CallID"
			case sipServer = "SIPServerIP"
		}

		var identifier: String // This identifies the call to ENS
		var callID: String // This is the SIP Call ID
		var sipServer: String

		init(pushInfo: PushInfo) {
			identifier = pushInfo.identifier
			callID = pushInfo.callId
			sipServer = pushInfo.sipServer
		}
	}

	public struct PushInfo {
		var identifier: String // This identifies the call to ENS
		var callId: String // This is the SIP Call ID
		var sipServer: String
		var displayName: String?
		var phoneNumber: String?
		var isShared: Bool?
		var userAgent: String?

		init?(payloadDictionary: [String: Any]) {
			guard
				let identifier = payloadDictionary["ENSIdentifier"] as? String,
				let callID = payloadDictionary["CallID"] as? String,
				let sipServer = payloadDictionary["SIPServerIP"] as? String
			else {
				return nil
			}

			self.identifier = identifier
			self.callId = callID
			self.sipServer = sipServer
			self.displayName = payloadDictionary["DisplayName"] as? String
			self.phoneNumber = payloadDictionary["PhoneNumber"] as? String
			self.isShared = payloadDictionary["IsSharedNumber"] as? Bool
			self.userAgent = payloadDictionary["UserAgent"] as? String
		}
	}

	private let baseURL: URL
	private let encoder = JSONEncoder()
	private var registeredPhoneNumbers: [PhoneNumber]?
	private var registeredDeviceToken: Data?
	private var registeredWatchToken: Data?
	public var isRegistered: Bool { registeredDeviceToken != nil }

	init() {
		#if DEBUG
		let server = UserDefaults.standard.string(forKey: "SCIDefaultENSServer")!
		#else
		let server = UserDefaults.standard.string(forKey: "SCIDefaultENSServer")!
		#endif

		baseURL = URL(string: "http://\(server)/EndpointNotificationService/EndpointService.svc/")!
	}

	private static func validateResponse(_ response: URLResponse?, request: URLRequest, data: Data?, error: Swift.Error?) -> Swift.Error? {
		guard error == nil else {
			return error
		}

		guard let response = response as? HTTPURLResponse else {
			return ENSClient.Error.badResponse
		}

		guard (200..<300).contains(response.statusCode) else {
			return HTTPError(request: request, response: response, responseBody: data)
		}

		return nil
	}

	public func register(
		sharedPhoneNumbers: [String],
		exclusivePhoneNumbers: [String],
		ringCount: Int,
		deviceToken: Data,
		watchToken: Data?,
		sipServer: String?,
		completion: @escaping (Swift.Error?) -> Void)
	{
		#if DEBUG
		let isDevelopment = true
		#else
		let isDevelopment = false
		#endif

		let phoneNumbers = sharedPhoneNumbers.map { PhoneNumber(isShared: true, phoneNumber: $0) }
			+ exclusivePhoneNumbers.map { PhoneNumber(isShared: false, phoneNumber: $0) }

		let devices: [Device]
		if let watchToken = watchToken {
			devices = [Device(token: watchToken.map { String(format: "%02.2hhx", arguments: [$0]) }.joined(), type: .watchOS)]
		} else {
			devices = []
		}

		let requestBody = RegisterRequestBody(
			deviceToken: deviceToken.map { String(format: "%02.2hhx", arguments: [$0]) }.joined(),
			deviceType: .iOS,
			devices: devices,
			ringCount: ringCount,
			isDevelopment: isDevelopment,
			phoneNumbers: phoneNumbers,
			sipServer: sipServer
		)

		let encodedRequestBody: Data
		do {
			encodedRequestBody = try encoder.encode(requestBody)
		} catch {
			completion(error)
			return
		}

		var components = URLComponents()
		components.path = "Login"

		var request = URLRequest(url: components.url(relativeTo: baseURL)!)
		request.httpMethod = "POST"
		request.setValue("application/json; charset=utf-8", forHTTPHeaderField: "content-type")
		request.httpBody = encodedRequestBody

		URLSession.shared.dataTask(with: request) { data, response, error in
			if let error = ENSClient.validateResponse(response, request: request, data: data, error: error) {
				completion(error)
				return
			}

			self.registeredDeviceToken = deviceToken
			self.registeredPhoneNumbers = phoneNumbers
			completion(nil)
		}.resume()
	}

	public func unregister(completion: @escaping (Swift.Error?) -> Void) {
		guard let deviceToken = registeredDeviceToken, let phoneNumbers = registeredPhoneNumbers else {
			// Already unregistered
			completion(nil)
			return
		}

		let devices: [Device]
		if let watchToken = registeredWatchToken {
			devices = [Device(token: watchToken.map { String(format: "%02.2hhx", arguments: [$0]) }.joined(), type: .watchOS)]
		} else {
			devices = []
		}

		let requestBody = UnregisterRequestBody(
			deviceToken: deviceToken.map { String(format: "%02.2hhx", arguments: [$0]) }.joined(),
			phoneNumbers: phoneNumbers,
			devices: devices // TODO
		)

		let encodedRequestBody: Data
		do {
			encodedRequestBody = try encoder.encode(requestBody)
		} catch {
			completion(error)
			return
		}

		var components = URLComponents()
		components.path = "Logout"

		var request = URLRequest(url: components.url(relativeTo: baseURL)!)
		request.httpMethod = "POST"
		request.setValue("application/json; charset=utf-8", forHTTPHeaderField: "content-type")
		request.httpBody = encodedRequestBody

		URLSession.shared.dataTask(with: request) { data, response, error in
			if let error = ENSClient.validateResponse(response, request: request, data: data, error: error) {
				completion(error)
				return
			}

			self.registeredDeviceToken = nil
			self.registeredPhoneNumbers = nil
			completion(nil)
		}.resume()
	}

	private func respondToCall(pushInfo: PushInfo, response: String, completion: @escaping (Swift.Error?) -> Void) {
		let requestBody = RespondToCallRequestBody(pushInfo: pushInfo)

		let encodedRequestBody: Data
		do {
			encodedRequestBody = try encoder.encode(requestBody)
		} catch {
			completion(error)
			return
		}

		var components = URLComponents()
		components.path = response

		var request = URLRequest(url: components.url(relativeTo: baseURL)!)
		request.httpMethod = "POST"
		request.setValue("application/json; charset=utf-8", forHTTPHeaderField: "content-type")
		request.httpBody = encodedRequestBody

		URLSession.shared.dataTask(with: request) { data, response, error in
			if let error = ENSClient.validateResponse(response, request: request, data: data, error: error) {
				completion(error)
				return
			}

			completion(nil)
		}.resume()
	}

	public func reportRinging(pushInfo: PushInfo, completion: @escaping (Swift.Error?) -> Void) {
		respondToCall(pushInfo: pushInfo, response: "Ringing", completion: completion)
	}

	public func reportReady(pushInfo: PushInfo, completion: @escaping (Swift.Error?) -> Void) {
		respondToCall(pushInfo: pushInfo, response: "Answer", completion: completion)
	}

	public func answer(pushInfo: PushInfo, completion: @escaping (Swift.Error?) -> Void) {
		respondToCall(pushInfo: pushInfo, response: "ForceAnswer", completion: completion)
	}

	public func decline(pushInfo: PushInfo, completion: @escaping (Swift.Error?) -> Void) {
		respondToCall(pushInfo: pushInfo, response: "Decline", completion: completion)
	}

	public func checkCall(pushInfo: PushInfo, completion: @escaping (Swift.Error?) -> Void) {
		respondToCall(pushInfo: pushInfo, response: "CallAvailableCheck", completion: completion)
	}

	public func checkStatus(completion: @escaping (Swift.Error?) -> Void) {
		var components = URLComponents()
		components.path = "Status"

		var request = URLRequest(url: components.url(relativeTo: baseURL)!)
		request.httpMethod = "POST"
		request.setValue("application/json; charset=utf-8", forHTTPHeaderField: "content-type")

		URLSession.shared.dataTask(with: request) { data, response, error in
			if let error = ENSClient.validateResponse(response, request: request, data: data, error: error) {
				completion(error)
				return
			}

			guard let responseString = data.flatMap({ String(bytes: $0, encoding: .utf8) }) else {
				completion(Error.badResponse)
				return
			}

			guard responseString.contains("Online") else {
				completion(Error.ensOffline(message: responseString))
				return
			}

			completion(nil)
		}.resume()
	}
}
