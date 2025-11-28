//
//  XMLParser.swift
//  ntouchMac
//
//  Created by @mmccoy on 10/31/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import Foundation

class XMLParser : NSObject, XMLParserDelegate {
	
	var allElements = NSMutableArray()
	var elements = NSMutableDictionary()
	var element = NSString()
	
	// keys to be parsed out of unencrypted XML
	// additional properties can be added here and to the delegate methods below
	var imagePublicId = NSMutableString()
	var imageUploadUrl = NSMutableString()
	
	// these delegate methods still work despite the warning, appears to be a bug in Xcode 8: http://stackoverflow.com/questions/39495773/xcode-8-warning-instance-method-nearly-matches-optional-requirement
	@objc func parser(_ parser: XMLParser, didStartElement elementName: String, namespaceURI: String?, qualifiedName qName: String?, attributes attributeDict: [String : String] = [:]) {
		element = elementName as NSString
		if elementName == "ImageUploadURLGetResult" {
			elements = [:]
			imagePublicId = ""
			imageUploadUrl = ""
		}
	}
	
	@objc func parser(_ parser: XMLParser, foundCharacters string: String) {
		let trimmedString = string.trimmingCharacters(in: .whitespacesAndNewlines)
		if element.isEqual(to: "ImagePublicId") {
			imagePublicId.append(trimmedString)
		}
		else if element.isEqual(to: "ImageUploadURL1") {
			imageUploadUrl.append(trimmedString)
		}
	}
	
	@objc func parser(_ parser: XMLParser, didEndElement elementName: String, namespaceURI: String?, qualifiedName qName: String?) {
		if elementName == "ImageUploadURLGetResult" {
			if !imagePublicId.isEqual(nil) {
				elements.setObject(imagePublicId as String, forKey: "ImagePublicId" as NSCopying)
			}
			if !imageUploadUrl.isEqual(nil) {
				elements.setObject(imageUploadUrl as String, forKey: "ImageUploadURL1" as NSCopying)
			}
			allElements.add(elements)
		}
	}
}

