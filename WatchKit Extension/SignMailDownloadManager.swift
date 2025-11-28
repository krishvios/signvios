//
//  SignMailDownloadManager.swift
//  ntouch
//
//  Created by Daniel Shields on 5/19/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

import Foundation
import WatchKit

@objc protocol SignMailDownloadDelegate: AnyObject {
	@objc func downloadProgressed(receivedBytes: Int64, ofTotalBytes: Int64);
	@objc func downloadDidFinishDownloadingToURL(_ url: URL);
	@objc func downloadFailed(withError: Error?);
}

/// Manages download and caching of signmail videos
///
/// On watchOS 3.0 this class also handles downloading signmail videos in the
/// background for e.g. push notifications.
@objc class SignMailDownloadManager: NSObject, URLSessionDownloadDelegate {
	
	static let sessionIdentifier = "com.sorenson.ntouch.signmail"

	// Media needs to be downloaded to an shared group container so that the app
	// can play the video.
	let cacheDirectory = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: "group.com.sorenson.ntouch.watch-data")!
		.appendingPathComponent("Library", isDirectory: true)
		.appendingPathComponent("Caches", isDirectory: true)
	
	/// The maximum number of bytes of video to keep cached
	let maxCacheSize: Int64 = 30_000_000
	
	/// The URL for the file keeping track of the map from background download 
	/// task IDs to message IDs.
	let persistenceURL = try! FileManager.default.url(for: .documentDirectory,
	                                                  in: .userDomainMask,
	                                                  appropriateFor: nil,
	                                                  create: true)
		.appendingPathComponent("downloadTasks", isDirectory: false)
		.appendingPathExtension("plist")
	
	// Swift doesn't support @available for stored properties, so we have to do
	// this hack.
	private var _backgroundTask: AnyObject?
	@available(watchOSApplicationExtension 3.0, *)
	var backgroundTask: WKURLSessionRefreshBackgroundTask? {
		get {
			return _backgroundTask as? WKURLSessionRefreshBackgroundTask
		}
		set {
			_backgroundTask = newValue
		}
	}
	
	var urlSession: URLSession!
	var messageIds: [Int: String] = [:]
	var delegates: NSMapTable<AnyObject, AnyObject> = NSMapTable.strongToWeakObjects()
	
	@objc static let shared = SignMailDownloadManager()
	
	override init() {
		super.init()
		
		let sessionConfig = URLSessionConfiguration.background(
			withIdentifier: SignMailDownloadManager.sessionIdentifier)
		urlSession = URLSession(configuration: sessionConfig,
		                        delegate: self,
		                        delegateQueue: OperationQueue.main)
	}
	
	/// Completion may return immediately if we already have signmail with the 
	/// id on disk.
	///
	/// The idea behind allowsCellularAccess is that if the user is on a WiFi
	/// network, we can download signmail videos as we get them from 
	/// notifications
	@objc func downloadSignMailFrom(_ url: URL, messageId: String,
	                          allowsCellularAccess: Bool = true,
	                          delegate: SignMailDownloadDelegate? = nil)
	{
		
		let cacheURL = cacheURLForMessageID(messageId)
		
		if FileManager.default.fileExists(atPath: cacheURL.path) {
			delegate?.downloadDidFinishDownloadingToURL(cacheURL)
		} else if let taskIdentifier = messageIds.first(where: { $0.value == messageId })?.key {
			// If we're already downloading this message, override the other callback.
			delegates.setObject(delegate, forKey: taskIdentifier as AnyObject)
			
			// TODO: Ensure the download task with the given task identifier is
			// actually running.
		} else {
			var urlRequest = URLRequest(url: url)
			urlRequest.allowsCellularAccess = allowsCellularAccess
			
			let task = urlSession.downloadTask(with: urlRequest)
			messageIds[task.taskIdentifier] = messageId
			delegates.setObject(delegate, forKey: task.taskIdentifier as AnyObject)
			if #available(watchOSApplicationExtension 3.0, *) {
				persistTaskMetadataIfNeeded()
			}
			task.resume()
		}
	}
	
	func evictOldVideosFromCache(toMakeRoomForBytes reserved: Int64 = 0) throws {
		// Get a list of files and sizes in the cache directory
		let urls = try FileManager.default.contentsOfDirectory(at: cacheDirectory,
		                                                       includingPropertiesForKeys: [.creationDateKey, .fileSizeKey],
		                                                       options: [.skipsSubdirectoryDescendants, .skipsPackageDescendants])
		
		var totalSize: Int64 = 0
		var files: [(url: URL, date: Date, size: Int64)] = try urls.map { url in
			let resourceValues = try url.resourceValues(forKeys: [.creationDateKey, .fileSizeKey])
			let creationDate = resourceValues.creationDate!
			let size = Int64(resourceValues.fileSize!)
			totalSize += size
			return (url, creationDate, size)
		}.sorted { $0.date > $1.date }

		// Keep removing old items until we have room to add reserved bytes.
		while totalSize > maxCacheSize - reserved, let fileToRemove = files.popLast()
		{
			try FileManager.default.removeItem(at: fileToRemove.url)
			totalSize -= fileToRemove.size
		}
	}
	
	func clearCache() throws {
		try evictOldVideosFromCache(toMakeRoomForBytes: maxCacheSize)
	}
	
	func cacheURLForMessageID(_ messageId: String) -> URL {
		return cacheDirectory
			.appendingPathComponent(messageId, isDirectory: false)
			.appendingPathExtension("mp4")
	}
	
	/// Should be called when the extension has entered the background so the
	/// download manager can persist our map of download task ids to signmail
	/// message ids
	@available(watchOSApplicationExtension 3.0, *)
	func didEnterBackground() {
		persistTaskMetadataIfNeeded()
	}
	
	/// Should be called when the extension has awaken from the background to 
	/// handle background tasks so that the download manager can load persisted 
	/// data about tasks and handle their events.
	@available(watchOSApplicationExtension 3.0, *)
	func didAwakeForBackgroundTask(_ backgroundTask: WKURLSessionRefreshBackgroundTask) {
		
		// Load persisted messageIds. 
		
		// TODO: Make sure that if the archive isn't valid, we don't crash. 
		// Otherwise our extension would never successfully start in the 
		// unlikely event the archive was somehow corrupted. We need to handle 
		// an invalidArgumentException in an Objective-C wrapper (as of Swift 3,
		// we cannot handle Obj-C NSExceptions in Swift) to delete the file if 
		// it is corrupted.
		
		let persisted = NSKeyedUnarchiver.unarchiveObject(withFile: persistenceURL.path) as? [Int: String] ?? [:]
		for (key, value) in persisted {
			messageIds[key] = value
		}
	}
	
	@available(watchOSApplicationExtension 3.0, *)
	func persistTaskMetadataIfNeeded() {
		if WKExtension.shared().applicationState == .background {
			NSKeyedArchiver.archiveRootObject(messageIds, toFile: persistenceURL.path)
		}
	}
	
	func urlSession(_ session: URLSession,
	                downloadTask: URLSessionDownloadTask,
	                didFinishDownloadingTo location: URL)
	{
		// Move the file to the cache directory with the file name matching the
		// messageId if it exists.
		var cacheURL: URL? = nil
		if let messageId = messageIds.removeValue(forKey: downloadTask.taskIdentifier) {
			cacheURL = cacheURLForMessageID(messageId)
			
			do {
				try evictOldVideosFromCache(toMakeRoomForBytes: downloadTask.countOfBytesReceived)
				try FileManager.default.moveItem(at: location, to: cacheURL!)
			}
			catch {
				print("Failed to move temporary download file to the video cache. \(error)")
				cacheURL = nil
			}
		}
		
		if let delegate = delegates.object(forKey: downloadTask.taskIdentifier as AnyObject) as! SignMailDownloadDelegate? {
			if let cacheURL = cacheURL {
				delegate.downloadDidFinishDownloadingToURL(cacheURL)
			}
			else {
				delegate.downloadFailed(withError: downloadTask.error)
			}
			
			delegates.removeObject(forKey: downloadTask.taskIdentifier as AnyObject)
		}
	}
	
	func urlSession(_ session: URLSession,
	                downloadTask: URLSessionDownloadTask,
	                didWriteData bytesWritten: Int64,
	                totalBytesWritten: Int64,
	                totalBytesExpectedToWrite: Int64)
	{
		if let delegate = delegates.object(forKey: downloadTask.taskIdentifier as AnyObject) as! SignMailDownloadDelegate? {
			delegate.downloadProgressed(receivedBytes: totalBytesWritten,
			                            ofTotalBytes: totalBytesExpectedToWrite)
		}
	}
	
	func urlSession(_ session: URLSession, task: URLSessionTask,
	                didCompleteWithError error: Error?)
	{
		// If for any reason the callback hasn't been called and removed yet,
		// call and remove it.
		if let delegate = delegates.object(forKey: task.taskIdentifier as AnyObject) as! SignMailDownloadDelegate? {
			print("Failed download response: \(String(describing: task.response))")
			delegate.downloadFailed(withError: error ?? task.error)
			delegates.removeObject(forKey: task.taskIdentifier as AnyObject)
		}
	}
	
	func urlSession(_ session: URLSession, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
			// URLSession will fail if host server uses a self signed cert.
			// since we control the URL and the server, we will trust credentials.
			completionHandler(.useCredential, URLCredential(trust: challenge.protectionSpace.serverTrust!))
	}
	
	func urlSessionDidFinishEvents(forBackgroundURLSession session: URLSession) {
		if #available(watchOSApplicationExtension 3.0, *) {
			OperationQueue.main.addOperation {
				self.backgroundTask?.setTaskCompleted()
				self.backgroundTask = nil
			}
		}
	}
}
