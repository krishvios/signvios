import UIKit

@IBDesignable open class RoundContactImageView: UIImageView {
	
	private var placeHolderImage: UIImage = UIImage.init(named: "avatar_default")!
	let kContactPhotoThumbnailSize: CGFloat = 48
	
	fileprivate struct Constants {
		static let preferredSize = CGSize(width: 60, height: 60)
		
		static let imageInset: CGFloat = 0
	}
	
	public override init(frame: CGRect) {
		super.init(frame: frame)
	}
	
	public required init?(coder aDecoder: NSCoder) {
		super.init(coder: aDecoder)
	}
	
	override open func layoutSubviews() {
		super.layoutSubviews()
		commonInit()
	}
	
	func resetToPlaceholder()
	{
		self.image = placeHolderImage
	}
	
	fileprivate func commonInit() {
		
		if self.placeHolderImage.size.width != kContactPhotoThumbnailSize || self.placeHolderImage.size.height != kContactPhotoThumbnailSize
		{
			let thumbnailSize = CGSize(width: kContactPhotoThumbnailSize, height: kContactPhotoThumbnailSize)
			placeHolderImage = placeHolderImage.resize(thumbnailSize)
		}
		self.image = self.placeHolderImage
		
		makeImageView()
	}
	
	fileprivate func makeImageView() {
		let imageLayer = self.layer
		imageLayer.cornerRadius = square(centeredIn: bounds).width / 2.0
		imageLayer.masksToBounds = true
		
		self.translatesAutoresizingMaskIntoConstraints = false
		NSLayoutConstraint.activate(constraintsUniformlyInsetting(self, by: Constants.imageInset))
	}
	
	private func constraintsUniformlyInsetting(_ view: UIView, by inset: CGFloat) -> [NSLayoutConstraint] {
		return [
			view.leftAnchor.constraint(equalTo: self.leftAnchor, constant: inset),
			view.rightAnchor.constraint(equalTo: self.rightAnchor, constant: inset),
			view.topAnchor.constraint(equalTo: self.topAnchor, constant: inset),
			view.bottomAnchor.constraint(equalTo: self.bottomAnchor, constant: inset),
		]
	}
	
	fileprivate func square(centeredIn rect: CGRect) -> CGRect {
		let minDimension = min(rect.height, rect.width)
		let squareAtOrigin = CGRect(x: 0, y: 0, width: minDimension, height: minDimension)
		let dx = (rect.width - squareAtOrigin.width) / 2.0
		let dy = (rect.height - squareAtOrigin.height) / 2.0
		let squareCentered = squareAtOrigin.offsetBy(dx: dx, dy: dy)
		return squareCentered
	}
	
	// MARK: - Layout
	
	open override var intrinsicContentSize: CGSize {
		return Constants.preferredSize
	}
	
}

