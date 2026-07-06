package com.codetoanbug.androidpuremvc

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * Drives the shared Core auth pipeline (real HttplibHttpClient over OpenSSL)
 * through JNI on a device/emulator. Run with
 * `./gradlew :app:connectedDebugAndroidTest`.
 *
 * No external backend is assumed: we assert the offline-deterministic paths —
 * input validation (no network) and a transport failure to an unreachable host.
 */
@RunWith(AndroidJUnit4::class)
class AndroidAuthClientTest {

    @Test
    fun emptyEmailFailsValidationWithoutNetwork() {
        val client = AndroidAuthClient(host = "10.255.255.1")
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
    fun loginToUnreachableHostReportsFailure() {
        // 10.255.255.1 is non-routable; the real HTTP client fails within the
        // configured timeout, exercising the whole pipeline end to end.
        val client = AndroidAuthClient(host = "10.255.255.1")
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
