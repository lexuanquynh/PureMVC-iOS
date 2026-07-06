#!/usr/bin/env bash
#
# Builds static OpenSSL 3.x for the Android NDK (arm64-v8a + x86_64) into
# android/openssl/prebuilt/<abi>/{include,lib}. Needed because httplib 0.21
# requires OpenSSL >= 3.0 and Google's NDK prefab package only ships 1.1.1.
#
# The output is gitignored; run this once after cloning (and in CI) before
# building the Android app. Requires perl + make (macOS has both).
#
set -euo pipefail

OPENSSL_VERSION="${OPENSSL_VERSION:-3.0.15}"
API="${ANDROID_API:-24}"
NDK="${ANDROID_NDK_ROOT:-$HOME/Library/Android/sdk/ndk/29.0.13599879}"

HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/prebuilt"
WORK="$(mktemp -d)"

# NDK prebuilt toolchain (host tag: darwin-arm64 / darwin-x86_64 / linux-x86_64).
TC=""
for tag in darwin-arm64 darwin-x86_64 linux-x86_64 windows-x86_64; do
    if [ -d "$NDK/toolchains/llvm/prebuilt/$tag" ]; then
        TC="$NDK/toolchains/llvm/prebuilt/$tag"
        break
    fi
done
if [ -z "$TC" ]; then
    TC="$NDK/toolchains/llvm/prebuilt/$(ls "$NDK/toolchains/llvm/prebuilt" | head -1)"
fi
export PATH="$TC/bin:$PATH"
export ANDROID_NDK_ROOT="$NDK"

echo "OpenSSL $OPENSSL_VERSION, API $API, NDK $NDK"
curl -fsSL "https://github.com/openssl/openssl/releases/download/openssl-$OPENSSL_VERSION/openssl-$OPENSSL_VERSION.tar.gz" \
    -o "$WORK/openssl.tar.gz"

build_abi() {
    local abi="$1" target="$2"
    local dir="$WORK/$abi"
    mkdir -p "$dir"
    tar -xzf "$WORK/openssl.tar.gz" -C "$dir" --strip-components=1
    (
        cd "$dir"
        ./Configure "$target" -D__ANDROID_API__="$API" \
            no-shared no-tests --prefix="$OUT/$abi"
        make -j"$(sysctl -n hw.ncpu)" build_libs
        make install_dev
    )
    echo "built $abi -> $OUT/$abi"
}

build_abi arm64-v8a android-arm64
build_abi x86_64 android-x86_64

rm -rf "$WORK"
echo "OpenSSL prebuilt ready: $OUT"
