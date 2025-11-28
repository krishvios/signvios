import UIKit

extension UITableView {
	func dequeue<Cell: UITableViewCell>(_ cell: Cell.Type, for indexPath: IndexPath, identifier: String = NSStringFromClass(Cell.self)) -> Cell {
		let cell = dequeueReusableCell(withIdentifier: identifier, for: indexPath) as! Cell
		return cell
	}
}

