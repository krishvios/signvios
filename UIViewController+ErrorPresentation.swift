import UIKit

extension UIViewController {
	func presentingAlertOnFailure(_ body: () throws -> Void) {
		do {
			try body()
		} catch {
			let alert = UIAlertController.basicAlert(error: error)
			present(alert, animated: true, completion: nil)
		}
	}
}

