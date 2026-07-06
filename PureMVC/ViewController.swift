//
//  ViewController.swift
//  PureMVC
//
//  Created by Prank on 17/6/25.
//
//  Demo login screen. UI is built programmatically; the Login button drives the
//  PureMVC facade → LoginCommand → Core (LoginUseCase, mock data in the demo).
//

import UIKit

class ViewController: UIViewController {

    // PureMVC Wrapper instance
    let pureMVC = PureMVCWrapper.sharedInstance()

    private let emailField = UITextField()
    private let passwordField = UITextField()
    private let statusLabel = UILabel()
    private let activity = UIActivityIndicatorView(style: .medium)

    override func viewDidLoad() {
        super.viewDidLoad()
        pureMVC.delegate = self
        setUpUI()
    }

    private func setUpUI() {
        view.backgroundColor = .systemBackground
        title = "PureMVC Login"

        let titleLabel = UILabel()
        titleLabel.text = "Đăng nhập (demo)"
        titleLabel.font = .preferredFont(forTextStyle: .title2)
        titleLabel.textAlignment = .center

        emailField.placeholder = "Email"
        emailField.text = "sample@gmail.com"
        emailField.borderStyle = .roundedRect
        emailField.autocapitalizationType = .none
        emailField.keyboardType = .emailAddress

        passwordField.placeholder = "Mật khẩu"
        passwordField.text = "password123"
        passwordField.borderStyle = .roundedRect
        passwordField.isSecureTextEntry = true

        let loginButton = UIButton(type: .system)
        loginButton.setTitle("Đăng nhập", for: .normal)
        loginButton.addTarget(self, action: #selector(login), for: .touchUpInside)

        let logoutButton = UIButton(type: .system)
        logoutButton.setTitle("Đăng xuất", for: .normal)
        logoutButton.addTarget(self, action: #selector(logout), for: .touchUpInside)

        statusLabel.numberOfLines = 0
        statusLabel.textAlignment = .center
        statusLabel.textColor = .secondaryLabel

        let stack = UIStackView(arrangedSubviews: [
            titleLabel, emailField, passwordField, loginButton, logoutButton, activity, statusLabel,
        ])
        stack.axis = .vertical
        stack.spacing = 16
        stack.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(stack)

        NSLayoutConstraint.activate([
            stack.centerYAnchor.constraint(equalTo: view.centerYAnchor),
            stack.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 24),
            stack.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -24),
        ])
    }

    @objc private func login() {
        view.endEditing(true)
        activity.startAnimating()
        statusLabel.text = "Đang đăng nhập…"
        pureMVC.onLoginButtonPressed(emailField.text ?? "", password: passwordField.text ?? "")
    }

    @objc private func logout() {
        pureMVC.onLogoutButtonPressed()
    }

    // Kept for the storyboard's existing connections.
    @IBAction func loginButtonTapped(_ sender: UIButton) { login() }
    @IBAction func logoutButtonTapped(_ sender: UIButton) { logout() }
    @IBAction func refreshButtonTapped(_ sender: UIButton) { pureMVC.onDataRefreshRequested() }
}

// MARK: - PureMVCDelegate
extension ViewController: PureMVCDelegate {

    func onNotificationReceived(_ notificationName: String, withData data: Any?) {
        activity.stopAnimating()
        switch notificationName {
        case PureMVCNotifications.loginSuccess:
            statusLabel.textColor = .systemGreen
            statusLabel.text = "✅ Đăng nhập thành công (token đã lưu vào Keychain)"
        case PureMVCNotifications.loginFailed:
            statusLabel.textColor = .systemRed
            statusLabel.text = "❌ Đăng nhập thất bại"
        case PureMVCNotifications.logoutSuccess:
            statusLabel.textColor = .secondaryLabel
            statusLabel.text = "Đã đăng xuất"
        default:
            break
        }
    }

    func onCommandExecuted(_ commandName: String, withData data: Any?) {}
}
