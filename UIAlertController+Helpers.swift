import UIKit

extension UIAlertController {
	static func basicAlert(error: Error) -> UIAlertController {
		return basicAlert(errorMessage: error.localizedDescription)
	}
	
	static func basicAlert(errorMessage: String) -> UIAlertController {
		let alert = UIAlertController(title: nil, message: errorMessage, preferredStyle: .alert)
		let okTitle = NSLocalizedString("Error Has Occured", comment: "Title for error alert button")
		alert.addAction(UIAlertAction(title: okTitle, style: .default, handler: nil))
		return alert
	}
}

