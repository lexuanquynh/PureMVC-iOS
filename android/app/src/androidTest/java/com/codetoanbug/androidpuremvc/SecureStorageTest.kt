package com.codetoanbug.androidpuremvc

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Verifies the Keystore-backed EncryptedSharedPreferences store (the Android
 * implementation of core::ISecureStorage) on a device/emulator.
 */
@RunWith(AndroidJUnit4::class)
class SecureStorageTest {

    private val storage =
        SecureStorage(InstrumentationRegistry.getInstrumentation().targetContext)

    @Test
    fun setGetRemoveRoundTrips() {
        storage.remove("k")
        assertNull(storage.get("k"))

        storage.set("k", "v")
        assertEquals("v", storage.get("k"))

        storage.set("k", "v2")
        assertEquals("v2", storage.get("k"))

        storage.remove("k")
        assertNull(storage.get("k"))
    }
}
