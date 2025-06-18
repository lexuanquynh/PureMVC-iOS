//
//  ViewController.swift
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

import UIKit

class ViewController: UIViewController {
    
    // PureMVC Wrapper instance
    let pureMVC = PureMVCWrapper.sharedInstance()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Set delegate to receive callbacks
        pureMVC.delegate = self
    }
    
    @IBAction func loginButtonTapped(_ sender: UIButton) {
        // Get username and password from UI
        let username = "sample@gmail.com"
        let password = "password123"
        
        // Call PureMVC
        pureMVC.onLoginButtonPressed(username, password: password)
    }
    
    @IBAction func logoutButtonTapped(_ sender: UIButton) {
        pureMVC.onLogoutButtonPressed()
    }
    
    @IBAction func refreshButtonTapped(_ sender: UIButton) {
        pureMVC.onDataRefreshRequested()
    }
}

// MARK: - PureMVCDelegate
extension ViewController: PureMVCDelegate {
    
    func onNotificationReceived(_ notificationName: String, withData data: Any?) {
        print("Received notification: \(notificationName)")
        
        switch notificationName {
        case PureMVCNotifications.loginSuccess:
            print("Login successful!")
            // Update UI for successful login
            showAlert(title: "Success", message: "Login successful!")
            
        case PureMVCNotifications.loginFailed:
            print("Login failed!")
            // Show error message
            showAlert(title: "Error", message: "Invalid username or password")
            
        case PureMVCNotifications.logoutSuccess:
            print("Logout successful!")
            // Update UI for logout
        case "ERROR":
                if let error = data as? [String: Any] {
//                    showError(error["message"] as? String ?? "Unknown error")
                }
        default:
            break
        }
    }
    
    func onCommandExecuted(_ commandName: String, withData data: Any?) {
        print("Command executed: \(commandName)")
    }
    
    // Helper method
    func showAlert(title: String, message: String) {
        let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: "OK", style: .default))
        present(alert, animated: true)
    }
}
