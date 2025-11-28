//
//  DualSlideButton.swift
//  ntouch
//
//  Created by Joseph Laser on 9/13/22.
//  Copyright © 2022 Sorenson Communications. All rights reserved.
//
import Foundation
import UIKit

@objc protocol DualSlideButtonDelegate {
    func rightAction(sender: DualSlideButton)
    func leftAction(sender: DualSlideButton)
    func tapAction(sender: DualSlideButton)
    func rightTapAction(sender: DualSlideButton)
    func leftTapAction(sender: DualSlideButton)
}

class DualSlideButton: UIView {

    var delegate: DualSlideButtonDelegate?

    private var dragView = UIView()
    private var rightLabel = UILabel()
    private var leftLabel = UILabel()
    private var rightDragView = UIView()
    private var leftDragView = UIView()
    private var rightDragViewLabel = UILabel()
    private var leftDragViewLabel = UILabel()
    private var imageView = UIImageView()
    private var complete = false
    private var layoutSet = false

    private var dragPointWidth: CGFloat = 60
    private var dragPointCornerRadius: CGFloat = 30

    private var dragPointImage: UIImage = UIImage()
    private var rightButtonText: String = ""
    private var leftButtonText: String = ""
    private var rightDragViewText: String = ""
    private var leftDragViewText: String = ""
    private var rightDragCompleteText: String = ""
    private var leftDragCompleteText: String = ""
    private var tapAnimationDuration = 0.3


    //MARK: Inits
    override init (frame: CGRect) {
        super.init(frame: frame)
    }

    required init(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)!
    }
    
    init(frame: CGRect,
         dragPointImage: UIImage? = nil,
         rightButtonText: String? = "RIGHT",
         leftButtonText: String? = "LEFT",
         rightDragViewText: String? = "RIGHT ACTION",
         leftDragViewText: String? = "LEFT ACTION",
         rightDragCompleteText: String? = "RIGHT ACTION COMPLETE",
         leftDragCompleteText: String? = "LEFT ACTION COMPLETE") {
        super.init(frame: frame)
        
        self.dragPointImage = dragPointImage ?? UIImage()
        self.rightButtonText = rightButtonText ?? ""
        self.leftButtonText = leftButtonText ?? ""
        self.rightDragViewText = rightDragViewText ?? ""
        self.leftDragViewText = leftDragViewText ?? ""
        self.rightDragCompleteText = rightDragCompleteText ?? ""
        self.leftDragCompleteText = leftDragCompleteText ?? ""
        setStyle()
    }

    override func layoutSubviews() {
        if !layoutSet {
            self.setUpButton()
            self.layoutSet = true
        }
    }

    //MARK: setStyle
    func setStyle() {
        self.rightLabel.text = NSLocalizedString(self.rightButtonText, comment: "")
        self.leftLabel.text = NSLocalizedString(self.leftButtonText, comment: "")
        self.rightDragViewLabel.text = NSLocalizedString(self.rightDragViewText, comment: "")
        self.leftDragViewLabel.text = NSLocalizedString(self.leftDragViewText, comment: "")
        
        self.imageView.image = dragPointImage
        
        self.backgroundColor = Theme.current.keypadButtonBackgroundColor
        self.layer.borderColor = Theme.current.keypadButtonBorderColor.cgColor
        self.layer.borderWidth = Theme.current.keypadButtonBorderWidth
        self.dragView.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor
        self.dragView.tintColor = Theme.current.keypadPrimaryButtonTextColor
        self.rightDragView.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor.withAlphaComponent(0)
        self.leftDragView.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor.withAlphaComponent(0)
        self.rightLabel.textColor = Theme.current.keypadButtonTextColor
        self.leftLabel.textColor = Theme.current.keypadButtonTextColor
        self.rightDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(0)
        self.leftDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(0)
        
        self.dragPointWidth = self.frame.size.height
        self.dragView.frame.size.width = self.dragPointWidth
        self.dragView.layer.cornerRadius = self.dragPointWidth / 2
        self.layer.cornerRadius = self.dragPointWidth / 2
    }

    func setFont(){
        self.rightLabel.font = Theme.current.keypadButtonFont
        self.leftLabel.font = Theme.current.keypadButtonFont
        self.rightDragViewLabel.font = Theme.current.keypadButtonFont
        self.leftDragViewLabel.font = Theme.current.keypadButtonFont
    }

    //MARK: setUpButton
    func setUpButton() {
        self.backgroundColor = Theme.current.keypadButtonBackgroundColor
        
        self.rightDragView = UIView(frame: self.frame)
        self.rightDragView.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor.withAlphaComponent(0)
        self.addSubview(rightDragView)
        
        self.leftDragView = UIView(frame: self.frame)
        self.leftDragView.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor.withAlphaComponent(0)
        self.addSubview(leftDragView)

        self.dragView = UIView(frame: CGRect(x: (self.frame.size.width - dragPointWidth) / 2, y: 0, width: dragPointWidth, height: self.frame.size.height))
        self.dragView.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor
        self.dragView.tintColor = Theme.current.keypadPrimaryButtonTextColor
        self.dragView.frame.size.width = self.dragPointWidth
        self.dragView.layer.cornerRadius = self.dragPointWidth / 2
        self.addSubview(self.dragView)
        self.bringSubviewToFront(self.dragView)

        //Labels
        if !self.rightButtonText.isEmpty {
            self.rightLabel = UILabel(frame: CGRect(x: (self.frame.size.width + self.dragPointWidth) / 2, y: 0, width: (self.frame.size.width - self.dragPointWidth) / 2, height: self.frame.size.height))
            self.rightLabel.textColor = Theme.current.keypadButtonTextColor
            self.rightLabel.textAlignment = .center
            self.rightLabel.text = NSLocalizedString(self.rightButtonText, comment: "")
            self.addSubview(self.rightLabel)
            self.sendSubviewToBack(self.rightLabel)

            self.rightDragViewLabel = UILabel(frame: CGRect(x: 0, y: 0, width: self.frame.size.width, height: self.frame.size.height))
            self.rightDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(0)
            self.rightDragViewLabel.textAlignment = .center
            self.rightDragViewLabel.text = NSLocalizedString(self.rightDragViewText, comment: "")
            self.rightDragView.addSubview(self.rightDragViewLabel)
        }
        if !self.leftButtonText.isEmpty {
            self.leftLabel = UILabel(frame: CGRect(x: 0, y: 0, width: (self.frame.size.width - self.dragPointWidth) / 2, height: self.frame.size.height))
            self.leftLabel.textColor = Theme.current.keypadButtonTextColor
            self.leftLabel.textAlignment = .center
            self.leftLabel.text = NSLocalizedString(self.leftButtonText, comment: "")
            self.addSubview(self.leftLabel)
            self.sendSubviewToBack(self.leftLabel)
            
            self.leftDragViewLabel = UILabel(frame: CGRect(x: 0, y: 0, width: self.frame.size.width, height: self.frame.size.height))
            self.leftDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(0)
            self.leftDragViewLabel.textAlignment = .center
            self.leftDragViewLabel.text = NSLocalizedString(self.leftDragViewText, comment: "")
            self.leftDragView.addSubview(self.leftDragViewLabel)
        }
        self.bringSubviewToFront(self.dragView)

        //Image
        if self.dragPointImage != UIImage() { //user chose a custom image to display on top of the slider
            self.imageView = UIImageView(frame: CGRect(x: 0, y: 0, width: self.dragPointWidth, height: self.frame.size.height))
            self.imageView.contentMode = .center
            self.imageView.image = self.dragPointImage
            self.dragView.addSubview(self.imageView)
        }

        self.dragView.layer.masksToBounds = true
        self.layer.masksToBounds = true
        
        let tapGestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(self.tapDetected(sender:)))
        self.addGestureRecognizer(tapGestureRecognizer)

        // Pan gesture recognition
        let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(self.panDetected(sender:)))
        panGestureRecognizer.minimumNumberOfTouches = 1
        self.dragView.addGestureRecognizer(panGestureRecognizer)
        
        let dragPointTapGestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(self.dragPointTapDetected(sender:)))
        self.dragView.addGestureRecognizer(dragPointTapGestureRecognizer)
    }
    
    @objc func dragPointTapDetected(sender: UITapGestureRecognizer) {
        self.delegate?.tapAction(sender: self)
    }
    
    @objc func tapDetected(sender: UITapGestureRecognizer) {
        if complete {
            dragPointTapDetected(sender: sender)
            return
        }
        
        let point = sender.location(in: sender.view)
        if let width = sender.view?.frame.size.width {
            complete = true
            if point.x < width / 2 { //tap left side
                UIView.transition(with: self, duration: self.tapAnimationDuration, options: .curveEaseOut, animations: {
                    self.dragView.frame = CGRect(x: 0, y: 0, width: self.dragPointWidth, height: self.dragView.frame.size.height)
                    
                    self.leftDragView.backgroundColor = self.leftDragView.backgroundColor?.withAlphaComponent(1)
                    self.leftDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(1)
                }) { status in
                    if status {
                        self.leftDragViewLabel.text = NSLocalizedString(self.leftDragCompleteText, comment: "")
                        
                        self.delegate?.leftTapAction(sender: self)
                    }
                }
            } else if point.x > width / 2 { //tap right side
                UIView.transition(with: self, duration: self.tapAnimationDuration, options: .curveEaseOut, animations: {
                    self.dragView.frame = CGRect(x: self.frame.size.width - self.dragPointWidth, y: 0, width: self.dragPointWidth, height: self.dragView.frame.size.height)
                    
                    self.rightDragView.backgroundColor = self.rightDragView.backgroundColor?.withAlphaComponent(1)
                    self.rightDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(1)
                }) { status in
                    if status {
                        self.rightDragViewLabel.text = NSLocalizedString(self.rightDragCompleteText, comment: "")
                        
                        self.delegate?.rightTapAction(sender: self)
                    }
                }
            }
        }
    }

    //MARK: panDetected
    @objc func panDetected(sender: UIPanGestureRecognizer) {
        if complete {
            return
        }
        
        //Call delegate?.statusUpdated(status: .Sliding, sender: self) if desired
        var translatedPoint = sender.translation(in: self)
        translatedPoint = CGPoint(x: translatedPoint.x, y: self.frame.size.height / 2)
        if translatedPoint.x > 0 { //moving right
            
            sender.view?.frame.origin.x = min(((self.frame.size.width - dragPointWidth) / 2) + translatedPoint.x, (self.frame.size.width - dragPointWidth))
            
            let changePercent = translatedPoint.x / ((self.frame.size.width - self.dragPointWidth) / 2)
            self.rightDragView.backgroundColor = self.rightDragView.backgroundColor?.withAlphaComponent(changePercent)
            self.rightDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(changePercent)
        } else if translatedPoint.x < 0 { //moving left
            
            sender.view?.frame.origin.x = max(((self.frame.size.width - dragPointWidth) / 2) + translatedPoint.x, 0)
            
            let changePercent = abs(translatedPoint.x) / ((self.frame.size.width - self.dragPointWidth) / 2)
            self.leftDragView.backgroundColor = self.leftDragView.backgroundColor?.withAlphaComponent(changePercent)
            self.leftDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(changePercent)
        } else {
            self.rightDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(0)
            self.leftDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(0)
        }
        if sender.state == .ended {
            let velocityX = sender.velocity(in: self).x * 0.2
            let finalX = translatedPoint.x + velocityX
            if finalX + (self.dragPointWidth / 2) >= (self.frame.size.width / 2) {
                completeRightDrag()
            } else if finalX - (self.dragPointWidth / 2) <= (self.frame.size.width / -2) {
                completeLeftDrag()
            }

            let animationDuration: Double = abs(Double(velocityX) * 0.0002) + 0.2
            UIView.transition(with: self, duration: animationDuration, options: .curveEaseOut, animations: {
            }, completion: { status in
                    if status {
                        self.animationFinished()
                    }
                })
        }
    }

    //MARK: Animations
    func animationFinished() {
        if !complete {
            self.reset()
        }
    }

    ///Complete right animation, when the slider reaches the end of the button
    func completeRightDrag() {
        complete = true
        UIView.transition(with: self, duration: 0.2, options: .curveEaseOut, animations: {
            self.dragView.frame = CGRect(x: self.frame.size.width - self.dragPointWidth, y: 0, width: self.dragPointWidth, height: self.dragView.frame.size.height)
            
            self.rightDragView.backgroundColor = self.rightDragView.backgroundColor?.withAlphaComponent(1)
            self.rightDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(1)
        }) { status in
            if status {
                self.rightDragViewLabel.text = NSLocalizedString(self.rightDragCompleteText, comment: "")
                
                self.delegate?.rightAction(sender: self)
            }
        }
    }
    
    ///Complete left animation, when the slider reaches the end of the button
    func completeLeftDrag() {
        complete = true
        UIView.transition(with: self, duration: 0.2, options: .curveEaseOut, animations: {
            self.dragView.frame = CGRect(x: 0, y: 0, width: self.dragPointWidth, height: self.dragView.frame.size.height)
            
            self.leftDragView.backgroundColor = self.leftDragView.backgroundColor?.withAlphaComponent(1)
            self.leftDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(1)
        }) { status in
            if status {
                self.leftDragViewLabel.text = NSLocalizedString(self.leftDragCompleteText, comment: "")
                
                self.delegate?.leftAction(sender: self)
            }
        }
    }

    ///Resets the button
    func reset() {
        UIView.transition(with: self, duration: 0.2, options: .curveEaseOut, animations: {
            self.dragView.frame = CGRect(x: (self.frame.size.width - self.dragPointWidth) / 2, y: 0, width: self.dragPointWidth, height: self.frame.size.height)
            self.imageView.frame = CGRect(x: 0, y: 0, width: self.dragPointWidth, height: self.frame.size.height)
            
            self.rightDragView.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor.withAlphaComponent(0)
            self.leftDragView.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor.withAlphaComponent(0)
        }) { status in
            if status {
                self.imageView.isHidden = false
                self.rightDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(0)
                self.leftDragViewLabel.textColor = Theme.current.segmentedControlSecondaryTextColor.withAlphaComponent(0)
                self.rightDragViewLabel.text = NSLocalizedString(self.rightDragViewText, comment: "")
                self.complete = false
            }
        }
    }

}
