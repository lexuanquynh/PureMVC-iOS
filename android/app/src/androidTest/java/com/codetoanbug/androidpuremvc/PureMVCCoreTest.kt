package com.codetoanbug.androidpuremvc

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Runs on a device/emulator: loads libpuremvc_jni and exercises the shared C++
 * LoginUseCase across JNI. Run with `./gradlew :app:connectedDebugAndroidTest`.
 */
@RunWith(AndroidJUnit4::class)
class PureMVCCoreTest {
    @Test
    fun emptyEmailReturnsValidationMessage() {
        assertEquals("Email is required", PureMVCCore.nativeLoginValidationMessage("", "pw"))
    }

    @Test
    fun emptyPasswordReturnsValidationMessage() {
        assertEquals("Password is required", PureMVCCore.nativeLoginValidationMessage("a@b.com", ""))
    }

    @Test
    fun validCredentialsReportSuccess() {
        assertEquals("Login successful", PureMVCCore.nativeLoginValidationMessage("a@b.com", "pw"))
    }
}
