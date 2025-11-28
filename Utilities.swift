import Foundation

extension StringProtocol where Self.Index == String.Index {
	func trimmingWhitespace() -> String {
		return self.trimmingCharacters(in: CharacterSet.whitespacesAndNewlines)
	}

	/// This function considers a string to be `empty`
	/// if it is nil, contains no characters, or
	/// consists entirely of whitespace characters.
	public var isBlank: Bool {
		return self.trimmingWhitespace() == ""
	}

	public var treatingBlankAsNil: Self? {
		return self.isBlank ? nil : self
	}
}

extension Optional where Wrapped: StringProtocol, Wrapped.Index == String.Index {
	var isBlankOrNil: Bool {
		return self?.trimmingWhitespace() ?? "" == ""
	}
}

extension Optional where Wrapped: Collection {
	var isEmptyOrNil: Bool {
		return self?.isEmpty ?? true
	}
}

extension UIScrollView {
	func scrollToBottom(animated: Bool) {
		let bottomOffset = CGPoint(x: 0, y: contentSize.height - bounds.height + contentInset.bottom);
		setContentOffset(bottomOffset, animated: animated);
	}
}

extension UIImage {
	@objc
	func resize(_ size: CGSize) -> UIImage
	{
		UIGraphicsBeginImageContextWithOptions(size, false, 0.0)
		defer { UIGraphicsEndImageContext() }
		draw(in: AVMakeRect(aspectRatio: self.size, insideRect: CGRect(origin: .zero, size: size)))
		return UIGraphicsGetImageFromCurrentImageContext()!
	}
}

extension UIColor {
	func createImage() -> UIImage {
		UIGraphicsBeginImageContextWithOptions(CGSize(width: 1, height: 1), false, 1)
		defer { UIGraphicsEndImageContext() }
		setFill()
		UIBezierPath(rect: CGRect(x: 0, y: 0, width: 1, height: 1)).fill()
		return UIGraphicsGetImageFromCurrentImageContext()!.resizableImage(withCapInsets: .zero)
	}
}

class ForwardTouchesStackView: UIStackView {
	@IBOutlet var reciever: UIView!
	override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
		if self.point(inside: point, with: event) {
			return reciever
		}
		else {
			return nil
		}
	}
}

// As of iOS 12, 08/08/2019, image views don't draw the image with vector data until you reset the image
class UIImageViewWithPreserveVectorDataFix: UIImageView {
	override func awakeFromNib() {
		super.awakeFromNib()
		let image = self.image
		self.image = nil
		self.image = image
	}
}

extension NSAttributedString {
	static func + (left: NSAttributedString, right: NSAttributedString) -> NSAttributedString {
		let result = NSMutableAttributedString()
		result.append(left)
		result.append(right)
		return result.copy() as! NSAttributedString
	}
}

extension NSMutableAttributedString {
	static func + (left: NSMutableAttributedString, right: NSMutableAttributedString) -> NSMutableAttributedString {
		let result = NSMutableAttributedString()
		result.append(left)
		result.append(right)
		return result
	}
}

/// Reformats a string of digits using a format string, and updates the given range such that the same digits are in the range. Returns whether the format succeeded.
///
/// The format is given in a string of digits and non-digits, like "(000) 000-0000".
///
fileprivate func reformatDigits(input: String, format: String, selection: inout Range<String.Index>) -> String {
	
	// The selection range in the sequence of characters (before converting to String.Index)
	var startOffset: String.IndexDistance = input.distance(from: input.startIndex, to: selection.lowerBound)
	var endOffset: String.IndexDistance = startOffset + input.distance(from: selection.lowerBound, to: selection.upperBound)
	var inputSeq: [Character] = Array(input)
	
	// Now loop through the format, removing input characters and inserting format characters where appropriate
	var nextToFormat = inputSeq.startIndex
	for char in format {
		
		// Remove garbage characters from input
		while nextToFormat != inputSeq.endIndex && inputSeq[nextToFormat] != char && !"0123456789".contains(inputSeq[nextToFormat]) {
			inputSeq.remove(at: nextToFormat)
			if nextToFormat < endOffset {
				endOffset -= 1
			}
			if nextToFormat < startOffset {
				startOffset -= 1
			}
		}
		
		guard nextToFormat < inputSeq.endIndex else { break }
		
		// If the format says a format character goes here, insert it before nextToFormat (if needed)
		if !"0123456789".contains(char) && inputSeq[nextToFormat] != char {
			inputSeq.insert(char, at: nextToFormat)
			if nextToFormat < endOffset {
				endOffset += 1
			}
			if nextToFormat < startOffset {
				startOffset += 1
			}
		}
		
		// Move to the next character to format
		nextToFormat = inputSeq.index(after: nextToFormat)
	}
	
	let result = String(inputSeq)
	let startIndex = result.index(result.startIndex, offsetBy: startOffset)
	let endIndex = result.index(startIndex, offsetBy: endOffset - startOffset)
	selection = startIndex ..< endIndex
	
	return result
	
}



/// Format the specified string as a Phone Number
///
fileprivate func formatForPhoneNumber(_ input: String) -> String {
	let unformatted = input.filter { "0123456789".contains($0) }
	
	if unformatted.starts(with: "1") {
		if 4 ... 11 ~= unformatted.count {
			return "0 (000) 000-0000"
		}
		else if unformatted.count > 11 {
			return "0 (000) 000-0000 0000"
		}
		else {
			return "000"
		}
	}
	else {
		if 4 ... 11 ~= unformatted.count {
			return "(000) 000-0000"
		}
		else if unformatted.count > 11 {
			return "(000) 000-0000 0000"
		}
		else {
			return "000"
		}
	}
}


extension String {
	var normalizedDialString: String {
		return count == 10 ? "1" + self : self
	}
	
	func reformatDigits(format: String) -> String {
		var selectionUnused: Range<String.Index> = startIndex..<endIndex
		return ntouch.reformatDigits(input: self, format: format, selection: &selectionUnused)
	}
	
	func formatAsPhoneNumber() -> String {
		return reformatDigits(format: formatForPhoneNumber(self))
	}
	
	func trimmingUnsupportedCharacters() -> String {
		var result = self.folding(options: .diacriticInsensitive, locale: .current)
		result = result.filter { $0.isASCII }
		return result.trimmingWhitespace()
	}
	
	func containsUnsupportedharacters() -> Bool {
		var result = false
		self.forEach { (character: Character) in
			if !character.isASCII {
				result = true
			}
		}
		return result
	}
	
	func subString(start: Int, end: Int) -> String {
		if start > end {
			return subString(start: end, end: start)
		}
        let startIndex = self.index(self.startIndex, offsetBy: start)
		let endIndex = self.index(self.startIndex, offsetBy: end)

		return String(self[startIndex..<endIndex])
    }
	
	func subString(start: Int, length: Int) -> String {
		return subString(start: start, end: start+length)
    }
	
	var wrapNavButtonString: String {
		let maxLength = 20
		if self.count > maxLength {
			return self.subString(start: 0, end: maxLength - 1 - 3) + "..."
		}
		return self
	}
	
	var localized: String {
		return NSLocalizedString(self, comment:"")
	}
	
	func localizeWithFormat(arguments: CVarArg...) -> String {
		return String(format: self.localized, arguments: arguments)
	}
}

@objc extension NSString {
	@objc func trimmingUnsupportedCharacters() -> String {
		return String.trimmingUnsupportedCharacters(self as String)()
	}
	
	func isMatchedByRegex(_ pattern: String) -> Bool {
		do {
			let regex = try NSRegularExpression(pattern: pattern, options: .caseInsensitive)
			return regex.numberOfMatches(in: self as String, options: [], range: NSRange(location: .zero, length: self.length)) == 1
		} catch let error {
			print(error.localizedDescription)
			return false
		}
	}
	
	func stringByReplacingOccurrencesOfRegex(_ pattern: String, withString: String) -> String {
		do {
			let regex = try NSRegularExpression(pattern: pattern, options: .caseInsensitive)
			return regex.stringByReplacingMatches(in: self as String, options: [], range: NSRange(location: .zero, length: self.length), withTemplate: withString)
		} catch let error {
			print(error.localizedDescription)
			return self as String
		}
	}
}

extension UITextInput {
	func reformatDigits(format: String) {
		guard let entireRange = textRange(from: beginningOfDocument, to: endOfDocument) else {
			print("Could not determine document range!")
			return
		}
		
		// Formatting an empty string is a no-op
		guard let string = text(in: entireRange), !string.isEmpty else {
			return
		}
		
		// Convert from UITextRange to NSRange
		let range: NSRange
		if let selectedTextRange = selectedTextRange {
			let location = offset(from: beginningOfDocument, to: selectedTextRange.start)
			let length = offset(from: selectedTextRange.start, to: selectedTextRange.end)
			range = NSRange(location: location, length: length)
		} else {
			range = NSRange()
		}

		var selection = Range(range, in: string) ?? string.endIndex..<string.endIndex
		let result = ntouch.reformatDigits(input: string, format: format, selection: &selection)
		let newRange = NSRange(selection, in: result)
		
		// Only update the selection if we had something selected
		let newTextRange: UITextRange?
		if selectedTextRange != nil,
			let start = position(from: beginningOfDocument, offset: newRange.location),
			let end = position(from: start, offset: newRange.length)
		{
			newTextRange = textRange(from: start, to: end)
		} else {
			newTextRange = nil
		}
		
		replace(entireRange, withText: result)
		selectedTextRange = newTextRange ?? textRange(from: endOfDocument, to: endOfDocument)
	}
	
	func formatAsPhoneNumber() {
		guard let entireRange = textRange(from: beginningOfDocument, to: endOfDocument) else {
			print("Could not determine document range!")
			return
		}
		
		// Formatting an empty string is a no-op
		guard let string = text(in: entireRange), !string.isEmpty else {
			return
		}
		
		return reformatDigits(format: formatForPhoneNumber(string))
	}
}

extension UIImageView {
	func downloaded(from url: URL, contentMode mode: ContentMode = .scaleAspectFit, errorImage: UIImage) {
		contentMode = mode
		URLSession.shared.dataTask(with: url) { data, response, error in
			if let httpURLResponse = response as? HTTPURLResponse, httpURLResponse.statusCode == 200,
			   let mimeType = response?.mimeType, mimeType.hasPrefix("image"),
			   let data = data, error == nil,
			   let image = UIImage(data: data) {
				DispatchQueue.main.async() { [weak self] in
					self?.image = image
				}
			}
			else {
				DispatchQueue.main.async() { [weak self] in
					self?.image = errorImage
				}
			}
		}.resume()
	}
	func downloaded(from link: String, contentMode mode: ContentMode = .scaleAspectFit, errorImage: UIImage) {
		guard let url = URL(string: link) else { return }
		downloaded(from: url, contentMode: mode, errorImage: errorImage)
	}
}
