package com.codetoanbug.androidpuremvc

import android.os.Handler
import android.os.Looper

/** Result callback from the native auth pipeline. */
fun interface AuthCallback {
    fun onResult(success: Boolean, message: String)
}

/**
 * Kotlin wrapper around the shared C++ auth pipeline (via JNI). Holds a native
 * handle; the native callback fires on a worker thread, so results are posted to
 * the main thread before reaching [AuthCallback].
 */
class AndroidAuthClient(host: String, port: Int = 443) : AutoCloseable {
    private val handle: Long = nativeCreate(host, port)
    private val main = Handler(Looper.getMainLooper())

    fun login(email: String, password: String, callback: AuthCallback) {
        nativeLogin(handle, email, password, AuthCallback { success, message ->
            main.post { callback.onResult(success, message) }
        })
    }

    fun logout() = nativeLogout(handle)

    fun currentAccessToken(): String? = nativeCurrentAccessToken(handle)

    override fun close() = nativeDestroy(handle)

    private external fun nativeCreate(host: String, port: Int): Long
    private external fun nativeLogin(handle: Long, email: String, password: String, callback: AuthCallback)
    private external fun nativeLogout(handle: Long)
    private external fun nativeCurrentAccessToken(handle: Long): String?
    private external fun nativeDestroy(handle: Long)

    companion object {
        init { System.loadLibrary("puremvc_jni") }
    }
}
