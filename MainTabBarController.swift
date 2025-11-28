import UIKit

class MainTabBarController: UITabBarController, UITabBarControllerDelegate {

	var animateNextTabTransition = false
	weak var cameraBackgroundViewController: CameraBackgroundViewController?
	var observerTokens: [Any] = []
	var activityTimeOutTimer :Timer?
	
	@objc public enum Tab: Int, CaseIterable {
		case keypad
        case recents
		case contacts
        case signmail
        case settings
	
		var description: String {
			switch self {
			case .keypad: return "dial"
			case .recents: return "history"
			case .contacts: return "contacts"
			case .signmail: return "signmail"
			case .settings: return "settings"
			}
		}
		
		var title: String {
			switch self {
			case .keypad: return "Dial"
			case .recents: return "History"
			case .contacts: return "Contacts"
			case .signmail: return "SignMail"
			case .settings: return "Settings"
			}
		}
		
		var identifier: String {
			switch self {
			case .keypad: return "ntouchKeypadViewController"
			case .recents: return "RecentsNavController"
			case .contacts: return "PhonebookNavController"
			case .signmail:	return "MessageNavController"
			case .settings: return "SettingsNavController"
			}
		}
	}

	@objc func getIndexForTab(_ tab: Tab) -> Int
	{
		guard let vc = viewControllers?.first(where: { $0.restorationIdentifier == tab.identifier }) else { return -1 }
		return viewControllers?.firstIndex(of: vc) ?? -1
	}

    @objc func select(tab: Tab) {
		let index = getIndexForTab(tab)
		if index == -1
		{
			return
		}
		
		selectedIndex = index
    }
	
	override public func loadView() {
		super.loadView()
		restoreAllTabs() // Make sure we match the Tab's enum tab order.
	}

    override public func viewDidLoad() {
        super.viewDidLoad()
		delegate = self
		NotificationCenter.default.addObserver(self, selector: #selector(interfaceModeChanged),
											   name: NSNotification.Name.SCINotificationInterfaceModeChanged, object: nil)
		
		NotificationCenter.default.addObserver(forName: CallController.awaitingCallRequirementsBegan,
											   object: nil,
											   queue: OperationQueue.main) { (note) in
			self.showActivityIndicator()
		}
		NotificationCenter.default.addObserver(forName: CallController.awaitingCallRequirementsEnded,
											   object: nil,
											   queue: OperationQueue.main) { (note) in
			self.dismissActivityIndicator()
		}

		
		let token1 = observe(\.selectedViewController, options: [.initial]) { [weak self] (_, _) in
			self?.cameraBackgroundViewController?.isMirrorEnabled = (self?.selectedViewController is SCIKeypadViewController)
		}
		observerTokens.append(token1)

		let token2 = observe(\.selectedIndex) { [weak self] (_, _) in
			self?.cameraBackgroundViewController?.isMirrorEnabled = (self?.selectedViewController is SCIKeypadViewController)
		}
		observerTokens.append(token2)
    }
	
    @objc
    func didLogOut() {
        debugPrint(#function)
        
    }
	
	override public var supportedInterfaceOrientations: UIInterfaceOrientationMask {
		if (UIDevice.current.userInterfaceIdiom == .pad) {
			return .all
		}
		else {
			return .portrait
		}
	}
	
	@objc func interfaceModeChanged() //SCINotificationInterfaceModeChanged
	{
		let mode = SCIVideophoneEngine.shared.interfaceMode
		switch mode {
		case .standard:
			restoreAllTabs()
			(UIApplication.shared.delegate as! AppDelegate).sendNotificationTypes()
		case .hearing:
			configureTabsForHearingMode()
		case .public:
			configureTabsForPublicMode()
		default:
			return
		}
	}
	
	func restoreAllTabs()
	{
		viewControllers?.removeAll()
		let storyboard = (UIApplication.shared.delegate as! AppDelegate).startTopLevelUIUtilities.storyboard
		
		for tab in Tab.allCases {
			let viewController = storyboard.instantiateViewController(withIdentifier: tab.identifier);
			viewController.title = tab.title.localized
			viewControllers?.append(viewController)
		}
	}
	
	@objc func configureTabsForPublicMode()
	{
		viewControllers = viewControllers?.filter { $0.restorationIdentifier == Tab.keypad.identifier }
	}
	
	func configureTabsForHearingMode()
	{
		
	}
	
	func showActivityIndicator() {
		MBProgressHUD.showAdded(to: self.view, animated: true).labelText = "call.wait".localized
	}
		
	func dismissActivityIndicator() {
		MBProgressHUD.hide(for: self.view, animated: true)
	}
    
    // MARK: - UITabBarController Delegate Methods
	
	func tabBarController(_ tabBarController: UITabBarController, animationControllerForTransitionFrom fromVC: UIViewController, to toVC: UIViewController) -> UIViewControllerAnimatedTransitioning? {
		if animateNextTabTransition {
			animateNextTabTransition = false
			return TabAnimator()
		}
		else {
			return nil
		}
	}
	
	func tabBarController(_ tabBarController: UITabBarController, didSelect viewController: UIViewController) {
		if getIndexForTab(Tab.signmail) >= 0 && getIndexForTab(Tab.recents) >= 0
		{
			// Pops navigation controllers for the signmail and recents tabs each time a tab is selected.
			[Tab.signmail, Tab.recents].lazy
				.compactMap { self.viewControllers?[$0.rawValue] as? UINavigationController }
				.forEach {
					$0.setNavigationBarHidden(false, animated: false)
					$0.popToRootViewController(animated: false)
			}
			
			// Don't stay on the video center segment of the sign mail tab
			let signMailNavTab = self.viewControllers?[Tab.signmail.rawValue] as? UINavigationController
			let signMailTab = signMailNavTab?.viewControllers.first as? SignMailListViewController
			signMailTab?.showSignMail()
		}
		
		let settingsIndex = getIndexForTab(.settings)
		if settingsIndex > 0 && selectedIndex != settingsIndex {
			UserDefaults.standard.set(selectedIndex, forKey: kSavedTabSelectedIndex)
			UserDefaults.standard.synchronize()
		}
		
		if let tab = Tab(rawValue: selectedIndex) {
			logSelectedIndex(index: tab)
		}
		
		if UIDevice.current.userInterfaceIdiom == .phone {
			if selectedIndex == getIndexForTab(.keypad) {
				cameraBackgroundViewController?.tryStartCameraBackground()
			}
			else {
				cameraBackgroundViewController?.stopCameraBackground()
			}
		}
	}
	
	func logSelectedIndex(index: Tab) {
		AnalyticsManager.shared.trackUsage(AnalyticsEvent.mainTab, properties: ["description" : index.description])
	}
}

class TabAnimator : NSObject, UIViewControllerAnimatedTransitioning {
	func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
		return 0.3
	}
	
	func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
		let containerView = transitionContext.containerView
		let toView = transitionContext.view(forKey: .to)
		let fromView = transitionContext.view(forKey: .from)
		if let toView = toView {
			containerView.addSubview(toView)
			toView.frame.origin.x = containerView.frame.maxX
		}
		
		UIView.animate(withDuration: transitionDuration(using: transitionContext), animations: {
			toView?.frame.origin.x = containerView.frame.minX
			fromView?.frame.origin.x -= containerView.frame.width * 0.3
		}) { (isfinished) in
			transitionContext.completeTransition(isfinished)
		}
	}
}
