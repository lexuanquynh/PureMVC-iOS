package com.codetoanbug.androidpuremvc

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * Drives the shared Core auth pipeline through JNI on a device/emulator. Run with
 * `./gradlew :app:connectedDebugAndroidTest`.
 */
@RunWith(AndroidJUnit4::class)
class AndroidAuthClientTest {

    @Test
    fun loginSucceedsAndStoresToken() {
        val client = AndroidAuthClient()
        val latch = CountDownLatch(1)
        var success = false
        var message = ""
        client.login("a@b.com", "pw") { s, m ->
            success = s; message = m; latch.countDown()
        }
        assertTrue(latch.await(5, TimeUnit.SECONDS))
        assertTrue(success)
        assertEquals("Login successful", message)
        assertNotNull(client.currentAccessToken())
        client.close()
    }

    @Test
    fun emptyEmailFailsValidation() {
        val client = AndroidAuthClient()
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
}
