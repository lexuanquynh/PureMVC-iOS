package com.codetoanbug.androidpuremvc

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * Drives the shared Core auth pipeline (real HttplibHttpClient over OpenSSL +
 * Keystore-backed token store) through JNI on a device/emulator. Run with
 * `./gradlew :app:connectedDebugAndroidTest`.
 */
@RunWith(AndroidJUnit4::class)
class AndroidAuthClientTest {

    private val context = InstrumentationRegistry.getInstrumentation().targetContext

    @Test
    fun emptyEmailFailsValidationWithoutNetwork() {
        val client = AndroidAuthClient(context, host = "10.255.255.1")
        val latch = CountDownLatch(1)
        var success = true
        var message = ""
        client.login("", "pw") { s, m ->
            success = s; message = m; latch.countDown()
        }
        assertTrue(latch.await(5, TimeUnit.SECONDS))
        assertFalse(success)
        assertEquals("Email is required", message)
        client.close()
    }

    @Test
    fun mockLoginSucceedsAndStoresToken() {
        val client = AndroidAuthClient(context, host = "mock", mock = true)
        client.logout()
        val latch = CountDownLatch(1)
        var success = false
        var message = ""
        client.login("a@b.com", "pw") { s, m ->
            success = s; message = m; latch.countDown()
        }
        assertTrue(latch.await(5, TimeUnit.SECONDS))
        assertTrue(success)
        assertEquals("Login successful", message)
        assertEquals("mock-access-token", client.currentAccessToken())
        client.logout()
        client.close()
    }

    @Test
    fun loginToUnreachableHostReportsFailure() {
        val client = AndroidAuthClient(context, host = "10.255.255.1")
        val latch = CountDownLatch(1)
        var success = true
        client.login("a@b.com", "pw") { s, _ ->
            success = s; latch.countDown()
        }
        assertTrue(latch.await(20, TimeUnit.SECONDS))
        assertFalse(success)
        client.close()
    }
}
