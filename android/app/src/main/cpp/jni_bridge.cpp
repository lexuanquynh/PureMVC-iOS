//
//  jni_bridge.cpp
//  Android JNI bridge to the shared C++ Core.
//
//  Runs the real Core auth pipeline (LoginUseCase → AuthRepository → IHttpClient)
//  driven from Kotlin. The HTTP client here is a stub returning a canned login
//  response asynchronously via ThreadExecutor; it is swapped for
//  HttplibHttpClient once native OpenSSL lands (the JNI/Kotlin API is unchanged).
//

#include <jni.h>
#include <memory>
#include <string>

#include "Domain/UseCases/LoginUseCase.hpp"
#include "Domain/Ports/IAuthRepository.hpp"
#include "Domain/Ports/ITokenStore.hpp"
#include "Infrastructure/Auth/AuthRepository.hpp"
#include "Infrastructure/Http/IHttpClient.hpp"
#include "Infrastructure/Concurrency/ThreadExecutor.hpp"

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

// In-memory token store (Android Keystore adapter is a later slice).
class InMemoryTokenStore : public ITokenStore {
public:
    Token load() override { return token_; }
    void save(const Token& token) override { token_ = token; }
    void clear() override { token_ = Token{}; }
private:
    Token token_;
};

// Stub HTTP client: canned successful login, delivered off-thread like the real
// one. Replaced by HttplibHttpClient when native OpenSSL is available.
class StubHttpClient : public IHttpClient {
public:
    explicit StubHttpClient(IExecutor& executor) : executor_(executor) {}
    void send(const HttpRequest& /*request*/, Callback callback) override {
        executor_.run([callback]() {
            HttpResponse response;
            response.status = 200;
            response.body =
                R"({"access_token":"demo-access","refresh_token":"demo-refresh","is_verify":true})";
            callback(response);
        });
    }
private:
    IExecutor& executor_;
};

// Native object graph mirroring iOS PMVCAuthClient (with the stub client).
struct AndroidAuthClient {
    ThreadExecutor executor;
    StubHttpClient http{executor};
    AuthRepository repository{http, "/api/v1/auth/login"};
    InMemoryTokenStore store;
    LoginUseCase login{repository, store};
};

// Calls back into a Kotlin AuthCallback (onResult(Boolean, String)) from an
// arbitrary native thread.
void invokeCallback(jobject callbackGlobal, bool success, const std::string& message) {
    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED) {
        g_vm->AttachCurrentThread(&env, nullptr);
        attached = true;
    }
    jclass cls = env->GetObjectClass(callbackGlobal);
    jmethodID method = env->GetMethodID(cls, "onResult", "(ZLjava/lang/String;)V");
    jstring jmsg = env->NewStringUTF(message.c_str());
    env->CallVoidMethod(callbackGlobal, method, static_cast<jboolean>(success), jmsg);
    env->DeleteLocalRef(jmsg);
    env->DeleteLocalRef(cls);
    env->DeleteGlobalRef(callbackGlobal);
    if (attached) {
        g_vm->DetachCurrentThread();
    }
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
        JNIEnv* /*env*/, jobject /*thiz*/) {
    return reinterpret_cast<jlong>(new AndroidAuthClient());
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
    client->login.execute(
        LoginCredentials{jstringToStd(env, email), jstringToStd(env, password)},
        [callbackGlobal](bool success, const std::string& message) {
            invokeCallback(callbackGlobal, success, message);
        });
}

extern "C" JNIEXPORT void JNICALL
Java_com_codetoanbug_androidpuremvc_AndroidAuthClient_nativeLogout(
        JNIEnv* /*env*/, jobject /*thiz*/, jlong handle) {
    reinterpret_cast<AndroidAuthClient*>(handle)->store.clear();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_codetoanbug_androidpuremvc_AndroidAuthClient_nativeCurrentAccessToken(
        JNIEnv* env, jobject /*thiz*/, jlong handle) {
    Token token = reinterpret_cast<AndroidAuthClient*>(handle)->store.load();
    if (token.accessToken.empty()) return nullptr;
    return env->NewStringUTF(token.accessToken.c_str());
}
