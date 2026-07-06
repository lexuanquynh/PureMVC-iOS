package com.codetoanbug.androidpuremvc

import android.content.Context
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey

/**
 * Secure key/value store backed by EncryptedSharedPreferences (Android Keystore).
 * Called from C++ (via JNI) as the Android implementation of core::ISecureStorage
 * — the counterpart of iOS KeychainSecureStore.
 *
 * Method names/signatures are referenced by the native bridge; keep them in sync.
 */
class SecureStorage(context: Context) {
    private val prefs = EncryptedSharedPreferences.create(
        context,
        "puremvc_secure_store",
        MasterKey.Builder(context).setKeyScheme(MasterKey.KeyScheme.AES256_GCM).build(),
        EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
        EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM,
    )

    fun get(key: String): String? = prefs.getString(key, null)

    fun set(key: String, value: String) {
        prefs.edit().putString(key, value).apply()
    }

    fun remove(key: String) {
        prefs.edit().remove(key).apply()
    }
}
