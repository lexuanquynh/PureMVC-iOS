package com.codetoanbug.androidpuremvc

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import com.codetoanbug.androidpuremvc.ui.theme.AndroidPureMVCTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            AndroidPureMVCTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    LoginScreen(modifier = Modifier.padding(innerPadding))
                }
            }
        }
    }
}

/**
 * Login UI driven by the shared C++ Core auth pipeline (via [AndroidAuthClient]).
 */
@Composable
fun LoginScreen(modifier: Modifier = Modifier) {
    val client = remember { AndroidAuthClient(host = "sample.com") }
    DisposableEffect(Unit) { onDispose { client.close() } }

    var email by remember { mutableStateOf("sample@gmail.com") }
    var password by remember { mutableStateOf("password123") }
    var status by remember { mutableStateOf("") }
    var loading by remember { mutableStateOf(false) }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        OutlinedTextField(
            value = email,
            onValueChange = { email = it },
            label = { Text("Email") },
            modifier = Modifier.fillMaxWidth(),
        )
        OutlinedTextField(
            value = password,
            onValueChange = { password = it },
            label = { Text("Password") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth(),
        )
        Button(
            enabled = !loading,
            onClick = {
                loading = true
                status = "Logging in…"
                client.login(email, password) { success, message ->
                    loading = false
                    status = if (success) {
                        "✅ $message\ntoken: ${client.currentAccessToken()}"
                    } else {
                        "❌ $message"
                    }
                }
            },
            modifier = Modifier.fillMaxWidth(),
        ) { Text("Login") }
        OutlinedButton(
            enabled = !loading,
            onClick = {
                client.logout()
                status = "Logged out"
            },
            modifier = Modifier.fillMaxWidth(),
        ) { Text("Logout") }
        if (status.isNotEmpty()) {
            Text(status)
        }
    }
}
