package com.codetoanbug.androidpuremvc

/**
 * Kotlin entry point to the shared C++ Core (via the JNI bridge in
 * app/src/main/cpp). First bring-up exposes a business-logic smoke; the auth
 * facade (login/logout/currentTokens) will be added in a later slice.
 */
object PureMVCCore {
    init {
        System.loadLibrary("puremvc_jni")
    }

    /** Runs the shared LoginUseCase validation and returns its message. */
    external fun nativeLoginValidationMessage(email: String, password: String): String
}
