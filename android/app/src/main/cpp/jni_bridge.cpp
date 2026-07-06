//
//  jni_bridge.cpp
//  Android JNI bridge to the shared C++ Core.
//
//  First bring-up: a smoke that runs the real LoginUseCase business rule across
//  JNI (no network). Later slices add the auth facade (HttplibHttpClient +
//  AuthRepository) and an Android Keystore ISecureStorage.
//

#include <jni.h>
#include <string>

#include "Domain/UseCases/LoginUseCase.hpp"
#include "Domain/Ports/IAuthRepository.hpp"
#include "Domain/Ports/ITokenStore.hpp"

using namespace core;

namespace {

// Minimal in-memory ports so the smoke exercises LoginUseCase deterministically.
class InMemoryAuthRepository : public IAuthRepository {
public:
    void login(const LoginCredentials&, LoginCallback callback) override {
        callback(true, AuthSession{}, DomainError{});
    }
};

class InMemoryTokenStore : public ITokenStore {
public:
    Token load() override { return token_; }
    void save(const Token& token) override { token_ = token; }
    void clear() override { token_ = Token{}; }
private:
    Token token_;
};

std::string jstringToStd(JNIEnv* env, jstring s) {
    if (s == nullptr) return {};
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out = c ? c : "";
    env->ReleaseStringUTFChars(s, c);
    return out;
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_codetoanbug_androidpuremvc_PureMVCCore_nativeLoginValidationMessage(
        JNIEnv* env, jobject /* thiz */, jstring email, jstring password) {
    InMemoryAuthRepository repository;
    InMemoryTokenStore store;
    LoginUseCase useCase(repository, store);

    std::string result;
    useCase.execute(
        LoginCredentials{jstringToStd(env, email), jstringToStd(env, password)},
        [&result](bool /*success*/, const std::string& message) { result = message; });

    return env->NewStringUTF(result.c_str());
}
