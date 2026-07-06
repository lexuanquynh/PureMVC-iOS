//
//  jni_bridge.cpp
//  Android JNI bridge to the shared C++ Core.
//
//  Runs the real Core auth pipeline (LoginUseCase → AuthRepository →
//  HttplibHttpClient) driven from Kotlin, over real HTTPS with the shared
//  certificate pinner (OpenSSL comes from the NDK prefab package).
//

#include <jni.h>
#include <memory>
#include <string>

#include "Domain/UseCases/LoginUseCase.hpp"
#include "Domain/Ports/IAuthRepository.hpp"
#include "Domain/Ports/ITokenStore.hpp"
#include "Infrastructure/Auth/AuthRepository.hpp"
#include "Infrastructure/Http/HttpClientConfig.hpp"
#include "Infrastructure/Http/IHttpClient.hpp"
#include "Infrastructure/Http/HttplibHttpClient.hpp"
#include "Infrastructure/Http/MockHttpClient.hpp"
#include "Infrastructure/Concurrency/ThreadExecutor.hpp"
#include "Infrastructure/Security/ISecureStorage.hpp"
#include "Infrastructure/Security/SecureTokenStore.hpp"

using namespace core;

namespace {

JavaVM* g_vm = nullptr;

std::string jstringToStd(JNIEnv* env, jstring s) {
    if (s == nullptr) return {};
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out = c ? c : "";
    env->ReleaseStringUTFChars(s, c);
    return out;
}

// RAII: a JNIEnv for the current thread, attaching to the JVM if needed.
struct ScopedEnv {
    JNIEnv* env = nullptr;
    bool attached = false;
    ScopedEnv() {
        if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED) {
            g_vm->AttachCurrentThread(&env, nullptr);
            attached = true;
        }
    }
    ~ScopedEnv() { if (attached) g_vm->DetachCurrentThread(); }
    JNIEnv* operator->() const { return env; }
};

// In-memory token store, used only by the PureMVCCore smoke.
class InMemoryTokenStore : public ITokenStore {
public:
    Token load() override { return token_; }
    void save(const Token& token) override { token_ = token; }
    void clear() override { token_ = Token{}; }
private:
    Token token_;
};

// ISecureStorage backed by a Kotlin SecureStorage object (EncryptedSharedPreferences
// over the Android Keystore). Calls up into the JVM via JNI.
class JniSecureStorage : public ISecureStorage {
public:
    JniSecureStorage(JNIEnv* env, jobject storage) {
        obj_ = env->NewGlobalRef(storage);
        jclass cls = env->GetObjectClass(storage);
        getM_ = env->GetMethodID(cls, "get", "(Ljava/lang/String;)Ljava/lang/String;");
        setM_ = env->GetMethodID(cls, "set", "(Ljava/lang/String;Ljava/lang/String;)V");
        removeM_ = env->GetMethodID(cls, "remove", "(Ljava/lang/String;)V");
        env->DeleteLocalRef(cls);
    }
    ~JniSecureStorage() override {
        ScopedEnv env;
        env->DeleteGlobalRef(obj_);
    }

    bool get(const std::string& key, std::string& out) override {
        ScopedEnv env;
        jstring jk = env->NewStringUTF(key.c_str());
        auto jv = static_cast<jstring>(env->CallObjectMethod(obj_, getM_, jk));
        env->DeleteLocalRef(jk);
        if (jv == nullptr) return false;
        out = jstringToStd(env.env, jv);
        env->DeleteLocalRef(jv);
        return true;
    }
    bool set(const std::string& key, const std::string& value) override {
        ScopedEnv env;
        jstring jk = env->NewStringUTF(key.c_str());
        jstring jv = env->NewStringUTF(value.c_str());
        env->CallVoidMethod(obj_, setM_, jk, jv);
        env->DeleteLocalRef(jk);
        env->DeleteLocalRef(jv);
        return true;
    }
    bool remove(const std::string& key) override {
        ScopedEnv env;
        jstring jk = env->NewStringUTF(key.c_str());
        env->CallVoidMethod(obj_, removeM_, jk);
        env->DeleteLocalRef(jk);
        return true;
    }

private:
    jobject obj_ = nullptr;
    jmethodID getM_ = nullptr;
    jmethodID setM_ = nullptr;
    jmethodID removeM_ = nullptr;
};

HttpClientConfig makeConfig(const std::string& host, int port) {
    HttpClientConfig config;
    config.host = host;
    config.port = port;
    config.useSSL = true;
    config.verifySSL = true;
    config.connectionTimeoutSec = 5;
    config.readTimeoutSec = 5;
    // Add real SPKI pins here (config.pinnedSpkiSha256Base64) for a real backend.
    return config;
}

// Native object graph mirroring iOS PMVCAuthClient (real HTTPS via httplib, or a
// mock client for the offline demo; token storage via the Android Keystore).
struct AndroidAuthClient {
    ThreadExecutor executor;
    std::unique_ptr<IHttpClient> http;
    std::unique_ptr<AuthRepository> repository;
    JniSecureStorage storage;
    std::unique_ptr<SecureTokenStore> store;
    std::unique_ptr<LoginUseCase> login;

    AndroidAuthClient(const std::string& host, int port, bool mock,
                      JNIEnv* env, jobject storageObj)
        : storage(env, storageObj) {
        if (mock) {
            http.reset(new MockHttpClient(executor));
        } else {
            http.reset(new HttplibHttpClient(makeConfig(host, port), executor));
        }
        repository.reset(new AuthRepository(*http, "/api/v1/auth/login"));
        store.reset(new SecureTokenStore(storage));
        login.reset(new LoginUseCase(*repository, *store));
    }
};

// Calls back into a Kotlin AuthCallback (onResult(Boolean, String)) from an
// arbitrary native thread.
void invokeCallback(jobject callbackGlobal, bool success, const std::string& message) {
    ScopedEnv env;
    jclass cls = env->GetObjectClass(callbackGlobal);
    jmethodID method = env->GetMethodID(cls, "onResult", "(ZLjava/lang/String;)V");
    jstring jmsg = env->NewStringUTF(message.c_str());
    env->CallVoidMethod(callbackGlobal, method, static_cast<jboolean>(success), jmsg);
    env->DeleteLocalRef(jmsg);
    env->DeleteLocalRef(cls);
    env->DeleteGlobalRef(callbackGlobal);
}

} // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

// --- PureMVCCore smoke (runs LoginUseCase validation) -----------------------

namespace {
class SmokeRepo : public IAuthRepository {
public:
    void login(const LoginCredentials&, LoginCallback cb) override {
        cb(true, AuthSession{}, DomainError{});
    }
};
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_codetoanbug_androidpuremvc_PureMVCCore_nativeLoginValidationMessage(
        JNIEnv* env, jobject /* thiz */, jstring email, jstring password) {
    SmokeRepo repo;
    InMemoryTokenStore store;
    LoginUseCase useCase(repo, store);
    std::string result;
    useCase.execute(
        LoginCredentials{jstringToStd(env, email), jstringToStd(env, password)},
        [&result](bool, const std::string& message) { result = message; });
    return env->NewStringUTF(result.c_str());
}

// --- AndroidAuthClient facade ----------------------------------------------

extern "C" JNIEXPORT jlong JNICALL
Java_com_codetoanbug_androidpuremvc_AndroidAuthClient_nativeCreate(
        JNIEnv* env, jobject /*thiz*/, jstring host, jint port, jboolean mock, jobject storage) {
    return reinterpret_cast<jlong>(
        new AndroidAuthClient(jstringToStd(env, host), static_cast<int>(port),
                              mock == JNI_TRUE, env, storage));
}

extern "C" JNIEXPORT void JNICALL
Java_com_codetoanbug_androidpuremvc_AndroidAuthClient_nativeDestroy(
        JNIEnv* /*env*/, jobject /*thiz*/, jlong handle) {
    delete reinterpret_cast<AndroidAuthClient*>(handle);
}

extern "C" JNIEXPORT void JNICALL
Java_com_codetoanbug_androidpuremvc_AndroidAuthClient_nativeLogin(
        JNIEnv* env, jobject /*thiz*/, jlong handle, jstring email, jstring password,
        jobject callback) {
    auto* client = reinterpret_cast<AndroidAuthClient*>(handle);
    jobject callbackGlobal = env->NewGlobalRef(callback);
    client->login->execute(
        LoginCredentials{jstringToStd(env, email), jstringToStd(env, password)},
        [callbackGlobal](bool success, const std::string& message) {
            invokeCallback(callbackGlobal, success, message);
        });
}

extern "C" JNIEXPORT void JNICALL
Java_com_codetoanbug_androidpuremvc_AndroidAuthClient_nativeLogout(
        JNIEnv* /*env*/, jobject /*thiz*/, jlong handle) {
    reinterpret_cast<AndroidAuthClient*>(handle)->store->clear();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_codetoanbug_androidpuremvc_AndroidAuthClient_nativeCurrentAccessToken(
        JNIEnv* env, jobject /*thiz*/, jlong handle) {
    Token token = reinterpret_cast<AndroidAuthClient*>(handle)->store->load();
    if (token.accessToken.empty()) return nullptr;
    return env->NewStringUTF(token.accessToken.c_str());
}
