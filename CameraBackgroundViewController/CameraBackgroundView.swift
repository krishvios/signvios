import UIKit
import AVFoundation

class VideoCameraBackgroundView: UIView {
	
	override class var layerClass: AnyClass {
		return AVCaptureVideoPreviewLayer.self
	}
	
	private enum CameraError: LocalizedError {
		case unavailable
		case badPictureData
		
		var errorDescription: String? {
			switch self {
			case .unavailable:
				return NSLocalizedString("Camera is unavailable", comment: "Error message displayed if the camera is unavailable when the user goes to the image capture screen")
			case .badPictureData:
				return NSLocalizedString("Picture data failed", comment: "Error message displayed if the camera returns invalid picture data (unlikely)")
			}
		}
	}
	
	private let captureSession = AVCaptureSession()
	private let cameraMediaType = AVMediaType.video
	private var previewLayer: AVCaptureVideoPreviewLayer {
		return self.layer as! AVCaptureVideoPreviewLayer
	}
	var videoOrientation: AVCaptureVideoOrientation {
		if (UIDevice.current.userInterfaceIdiom == .pad) {
			if #available(iOS 13.0, *) {
				if let sceneOrientation = UIApplication.shared.windows.first?.windowScene?.interfaceOrientation {
					return convertToVideoOrientation(from: sceneOrientation)
				}
			}
			else {
				return convertToVideoOrientation(from: UIApplication.shared.statusBarOrientation)
			}
		}

		return .portrait
	}
	
	var isActive: Bool {
		return captureSession.isRunning || captureSession.isInterrupted
	}
	
	var delegate: UIViewController?
	
	override func awakeFromNib() {
		super.awakeFromNib()
		
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureSessionBegan(_:)),
			name: .AVCaptureSessionDidStartRunning,
			object: captureSession)
		NotificationCenter.default.addObserver(
			self,
			selector: #selector(notifyCaptureSessionEnded(_:)),
			name: .AVCaptureSessionDidStopRunning,
			object: captureSession)
		
		if #available(iOS 11.0, *) {
			accessibilityIgnoresInvertColors = true
		}
		

		
		alpha = 0
	}
	
	func startCameraBackground() {
		if captureSession.inputs.isEmpty {
			delegate?.presentingAlertOnFailure {
				#if !targetEnvironment(simulator)
				try setupCamera()
				#else
				do {
					try setupCamera()
				}
				catch CameraError.unavailable { // Ignore this error in the simulator
					print("Camera unavailable in simulator")
				}
				#endif
			}
		}
		
		if !captureSession.inputs.isEmpty {
			previewLayer.isHidden = false
			
			DispatchQueue.global().async {
				self.captureSession.startRunning()
			}
		}
	}
	
	func stopCameraBackground() {
		previewLayer.isHidden = true
		if captureSession.isRunning || captureSession.isInterrupted {
			captureSession.stopRunning()
		}
	}
	
	func didRotate() {
		if previewLayer.connection != nil {
			self.previewLayer.connection?.videoOrientation = self.videoOrientation
		}
	}
	
	private func setupCamera() throws {
		guard let captureDevice = getCameraDevice() else {
			throw CameraError.unavailable
		}
		captureSession.addInput(try AVCaptureDeviceInput(device: captureDevice))
		captureSession.sessionPreset = AVCaptureSession.Preset.hd1280x720
		previewLayer.session = captureSession
		previewLayer.videoGravity = AVLayerVideoGravity.resizeAspectFill
		previewLayer.frame = self.bounds
		previewLayer.connection?.videoOrientation = videoOrientation
	}
	
	func convertToVideoOrientation(from interfaceOrientation: UIInterfaceOrientation) -> AVCaptureVideoOrientation {
		switch interfaceOrientation {
		case UIInterfaceOrientation.portrait:
			return AVCaptureVideoOrientation.portrait
		case UIInterfaceOrientation.portraitUpsideDown:
			return AVCaptureVideoOrientation.portraitUpsideDown
		case UIInterfaceOrientation.landscapeLeft:
			return AVCaptureVideoOrientation.landscapeLeft
		case UIInterfaceOrientation.landscapeRight:
			return AVCaptureVideoOrientation.landscapeRight
		default:
			return AVCaptureVideoOrientation.portrait
		}
	}
	
	private func getCameraDevice() -> AVCaptureDevice? {
		let targetPosition = AVCaptureDevice.Position.front
		
		if #available(iOS 10, *) {
			return AVCaptureDevice.default(.builtInWideAngleCamera, for: AVMediaType.video, position: targetPosition)
		} else {
			return AVCaptureDevice
				.devices(for: cameraMediaType)
				.first(where: { ($0).position == targetPosition } )
		}
	}
	
	@objc func notifyCaptureSessionBegan(_ note: Notification) {
		DispatchQueue.main.async {
			UIView.animate(withDuration: 0.3) {
				self.alpha = 1
			}
		}
	}
	
	@objc func notifyCaptureSessionEnded(_ note: Notification) {
		DispatchQueue.main.async {
			UIView.animate(withDuration: 0.3) {
				self.alpha = 0
			}
		}
	}
}

