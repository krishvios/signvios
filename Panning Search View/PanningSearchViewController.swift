//
//  PanningSearchViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 11/14/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import UIKit
import Foundation
let kDefaultPanningSearchTableViewCellName = "DefaultPanningSearchTableViewCell"
let kDefaultPanningSearchTableViewCellIdentifier = "DefaultPanningSearchTableViewCell"
let kFullView : CGFloat = 100
var kPartialView: CGFloat {
	return UIScreen.main.bounds.height - 150
}

protocol HasPanningSearchViewController {
	var panningSearchViewController : PanningSearchViewController? { get }
	func addPanningSearchViewController()
}
extension HasPanningSearchViewController where Self:UIViewController {
	func addPanningSearchViewController(){
		let panningVC = PanningSearchViewController()
		self.addChild(panningVC)
		self.view.addSubview(panningVC.view)
		panningVC.didMove(toParent: self)
		
		let height = view.frame.height
		let width  = view.frame.width
		panningVC.view.frame = CGRect(x: 0, y: self.view.frame.maxY, width: width, height: height)
	}
}
class PanningSearchViewController : UIViewController {
	@IBOutlet
	weak var headerView :  UIView!
	@IBOutlet
	weak var tableView : UITableView!
	@IBOutlet
	weak var searchBar : UISearchBar!
	
	enum PanningState {
		
		case extended
		case collapsed
	}
	
	override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
		self.prepareBackgroundView()
		
		self.view.setNeedsLayout()
		
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		tableView.delegate = self
		tableView.dataSource = self
		tableView.register(UINib(nibName: kDefaultPanningSearchTableViewCellName, bundle: nil), forCellReuseIdentifier: kDefaultPanningSearchTableViewCellIdentifier)
		
		searchBar.isUserInteractionEnabled = false
		
		let gesture = UIPanGestureRecognizer.init(target: self, action: #selector(PanningSearchViewController.panGesture))
		gesture.delegate = self
		view.addGestureRecognizer(gesture)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		prepareBackgroundView()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		
		UIView.animate(withDuration: 0.6, animations: { [weak self] in
			let frame = self?.view.frame
			let yComponent = kPartialView
			self?.view.frame = CGRect(x: 0, y: yComponent, width: frame!.width, height: frame!.height - 100)
		})
	}
	
	override func didReceiveMemoryWarning() {
		super.didReceiveMemoryWarning()
		// Dispose of any resources that can be recreated.
	}
	
	@objc func panGesture(_ recognizer: UIPanGestureRecognizer) {
		
		let translation = recognizer.translation(in: self.view)
		let velocity = recognizer.velocity(in: self.view)
		
		let y = self.view.frame.minY
		if (y + translation.y >= kFullView) && (y + translation.y <= kPartialView) {
			self.view.frame = CGRect(x: 0, y: y + translation.y, width: view.frame.width, height: view.frame.height)
			recognizer.setTranslation(CGPoint.zero, in: self.view)
		}
		
		if recognizer.state == .ended {
			var duration =  velocity.y < 0 ? Double((y - kFullView) / -velocity.y) : Double((kPartialView - y) / velocity.y )
			
			duration = duration > 1.3 ? 1 : duration
			
			UIView.animate(withDuration: duration, delay: 0.0, options: [.allowUserInteraction], animations: {
				if  velocity.y >= 0 {
					self.view.frame = CGRect(x: 0, y: kPartialView, width: self.view.frame.width, height: self.view.frame.height)
				} else {
					self.view.frame = CGRect(x: 0, y: kFullView, width: self.view.frame.width, height: self.view.frame.height)
				}
				
			}, completion: { [weak self] _ in
				if ( velocity.y < 0 ) {
					self?.tableView.isScrollEnabled = true
				}
			})
		}
	}
	var blurEffect = UIBlurEffect(style: .dark)
	var visualEffect: UIVisualEffectView?
	var blurredView : UIVisualEffectView?

	func prepareBackgroundView(){
		if visualEffect == nil {
			visualEffect = UIVisualEffectView(effect: blurEffect)
		}
		if blurredView == nil {
			blurredView = UIVisualEffectView(effect: blurEffect)
		}
		guard let blurredView = self.blurredView,
			let visualEffect = self.visualEffect else {return}
		blurredView.contentView.addSubview(visualEffect)
		visualEffect.frame = self.view.bounds
		blurredView.frame = self.view.bounds
		view.insertSubview(blurredView, at: 0)
	}
	
}

extension PanningSearchViewController: UITableViewDelegate, UITableViewDataSource {
	
	func numberOfSections(in tableView: UITableView) -> Int {
		return 1
	}
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return 30
	}
	
	func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
		return 50
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		return tableView.dequeueReusableCell(withIdentifier: kDefaultPanningSearchTableViewCellIdentifier)!
	}
}

extension PanningSearchViewController: UIGestureRecognizerDelegate {
	
	// Solution
	func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
		let gesture = (gestureRecognizer as! UIPanGestureRecognizer)
		let direction = gesture.velocity(in: view).y
		
		let y = view.frame.minY
		if (y == kFullView && tableView.contentOffset.y == 0 && direction > 0) || (y == kPartialView) {
			tableView.isScrollEnabled = false
		} else {
			tableView.isScrollEnabled = true
		}
		
		return false
	}
	
}
