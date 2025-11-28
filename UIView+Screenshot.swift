import UIKit

extension UIView {
	
	func image() -> UIImage? {
		UIGraphicsBeginImageContext(self.frame.size);
		
		guard let currentContext = UIGraphicsGetCurrentContext() else  {
			return nil
		}
		
		self.layer.render(in: currentContext)
		let image = UIGraphicsGetImageFromCurrentImageContext();
		
		UIGraphicsEndImageContext();
		
		return image;
	}
	
}

