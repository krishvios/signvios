import UIKit

// Allows for more semantic usage of font names and types
// titleLabel.font = UIFont(for: .callout, at: .semibold)

private extension UIFontDescriptor {

	static func preferredFontDescriptor(for textStyle: UIFont.TextStyle, at weight: FontWeight?,
                                        scaledBy scale: CGFloat?, features: [FontFeatures]) -> UIFontDescriptor {
        var descriptor = UIFontDescriptor.preferredFontDescriptor(withTextStyle: textStyle)
        let fontSize = descriptor.pointSize * (scale ?? 1)

        if let weight = weight {
			descriptor = UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight(rawValue: weight.rawValue)).fontDescriptor
        } else if scale != nil {
            descriptor = descriptor.withSize(fontSize)
        }

        if !features.isEmpty {
            descriptor = descriptor.addingAttributes([
				UIFontDescriptor.AttributeName.featureSettings: features.flatMap { $0.rawValue }
                ])
        }

        return descriptor
    }

}

public struct FontWeight {
    fileprivate var rawValue: CGFloat

    fileprivate init(_ rawValue: CGFloat) {
        self.rawValue = rawValue
    }

	public static var ultraLight: FontWeight { return .init(UIFont.Weight.ultraLight.rawValue) }
	public static var thin: FontWeight { return .init(UIFont.Weight.thin.rawValue) }
	public static var light: FontWeight { return .init(UIFont.Weight.light.rawValue) }
	public static var regular: FontWeight { return .init(UIFont.Weight.regular.rawValue) }
	public static var medium: FontWeight { return .init(UIFont.Weight.medium.rawValue) }
	public static var semibold: FontWeight { return .init(UIFont.Weight.semibold.rawValue) }
	public static var bold: FontWeight { return .init(UIFont.Weight.bold.rawValue) }
	public static var heavy: FontWeight { return .init(UIFont.Weight.heavy.rawValue) }
	public static var black: FontWeight { return .init(UIFont.Weight.black.rawValue) }
}

public struct FontFeatures {
    fileprivate var rawValue: [[String: Int]]

    fileprivate init(_ rawValue: [[String: Int]]) {
        self.rawValue = rawValue
    }

    /// The monospaced numbers font feature when it would be more
    /// readable for numbers to align.
    ///
    /// - note: May not be available in all fonts.
    public static var monospacedNumbers: FontFeatures {
        return .init([[
			UIFontDescriptor.FeatureKey.featureIdentifier.rawValue: kNumberSpacingType,
			UIFontDescriptor.FeatureKey.typeIdentifier.rawValue: kMonospacedNumbersSelector
            ]])
    }

    /// The small-caps font feature to match iOS convention for headers.
    ///
    /// - note: May not be available in all fonts.
    public static var allCaps: FontFeatures {
        return .init([[
			UIFontDescriptor.FeatureKey.featureIdentifier.rawValue: kLowerCaseType,
			UIFontDescriptor.FeatureKey.typeIdentifier.rawValue: kLowerCaseSmallCapsSelector
            ], [
				UIFontDescriptor.FeatureKey.featureIdentifier.rawValue: kUpperCaseType,
				UIFontDescriptor.FeatureKey.typeIdentifier.rawValue: kUpperCaseSmallCapsSelector
            ]])
    }
}

extension UIFont {

    /// Creates a customized font associated with the `textStyle`, scaled
    /// appropriately for the user's selected content size category, and
    /// adjusted from these defaults by a `weight`, `scale`, or `features`.
	@nonobjc public convenience init(for textStyle: UIFont.TextStyle, at weight: FontWeight? = nil,
                                     scaledBy scale: CGFloat? = nil, features: [FontFeatures] = []) {
        self.init(descriptor: .preferredFontDescriptor(for: textStyle, at: weight, scaledBy: scale, features: features), size: 0)
    }
	
	@nonobjc public convenience init(for textStyle: UIFont.TextStyle, at weight: FontWeight? = nil,
									 maxSize: CGFloat, features: [FontFeatures] = []) {
		let tempFont = UIFont(for: textStyle, at: weight, size: 0, features: features)
		if tempFont.lineHeight > maxSize {
			self.init(descriptor: .preferredFontDescriptor(for: textStyle, at: weight, scaledBy: nil, features: features), size: maxSize)
		} else {
			self.init(descriptor: .preferredFontDescriptor(for: textStyle, at: weight, scaledBy: nil, features: features), size: 0)
		}
	}

	@nonobjc public convenience init(for textStyle: UIFont.TextStyle, at weight: FontWeight? = nil,
                                     size: CGFloat, features: [FontFeatures] = []) {
        self.init(descriptor: .preferredFontDescriptor(for: textStyle, at: weight, scaledBy: nil, features: features), size: size)
    }

	public func font(byAdding traits: UIFontDescriptor.SymbolicTraits) -> UIFont {
        let descriptor = fontDescriptor
        guard let newDescriptor = descriptor.withSymbolicTraits(descriptor.symbolicTraits.union(traits)) as UIFontDescriptor?
            else { return self }
        return UIFont(descriptor: newDescriptor, size: 0)
    }

}
