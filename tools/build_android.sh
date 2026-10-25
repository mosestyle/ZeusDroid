#!/usr/bin/env bash
# Builds a debug arm64-v8a APK without Gradle (used in restricted CI/agent
# environments). Android Studio users can use android/build.gradle instead.
#
# Requirements (see docs/BUILD_ANDROID.md):
#   ANDROID_NDK   NDK r26+ (sysroot + cmake toolchain); host clang via wrappers is fine
#   ANDROID_JAR   platforms/android-35/android.jar
#   D8_JAR        build-tools/*/lib/d8.jar
#   aapt or aapt2, zipalign, apksigner, javac, keytool on PATH
#
# Output: dist/ZeusDroid-debug-arm64-v8a.apk
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build/android-arm64}"
OUT="$ROOT/build/apk"
DIST="$ROOT/dist"
ANDROID_NDK="${ANDROID_NDK:-/home/claude/android/android-ndk-r29}"
ANDROID_JAR="${ANDROID_JAR:-/home/claude/android/android-sdk/platforms/android-35/android.jar}"
D8_JAR="${D8_JAR:-/home/claude/android/android-sdk/build-tools/37.0.0/lib/d8.jar}"
# Framework resource table used by aapt2 link. Debian's aapt2 2.19 cannot read
# the sparse tables of android-33+ jars, so the trusted Debian android-23 jar
# is used for resource linking only (javac still compiles against ANDROID_JAR).
RES_JAR="${RES_JAR:-/usr/lib/android-sdk/platforms/android-23/android.jar}"
API=26
APK_NAME="ZeusDroid-debug-arm64-v8a.apk"
TB="$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64"
STRIP="$TB/bin/llvm-strip"

step() { echo "==> $*"; }

step "native build (cmake/ninja, arm64-v8a, android-$API)"
if [ ! -f "$BUILD/build.ninja" ]; then
  cmake -S "$ROOT/engine" -B "$BUILD" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-$API -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=Release
fi
ninja -C "$BUILD" -j"${JOBS:-2}"

rm -rf "$OUT" && mkdir -p "$OUT"/{classes,dex,lib/arm64-v8a,res-compiled,gen,stubs}

step "collect and strip shared libraries"
for lib in libmain.so libSDL2.so libSDL2_image.so libSDL2_mixer.so libSDL2_ttf.so; do
  src="$(find "$BUILD" -name "$lib" -type f | head -1)"
  [ -n "$src" ] || { echo "missing $lib"; exit 1; }
  "$STRIP" --strip-unneeded -o "$OUT/lib/arm64-v8a/$lib" "$src"
done
# Sanity: every library must be AArch64 ELF and must not need libc++_shared.
for f in "$OUT"/lib/arm64-v8a/*.so; do
  file "$f" | grep -q "ARM aarch64" || { echo "not arm64: $f"; exit 1; }
  if "$TB/bin/llvm-readelf" -d "$f" | grep -q "libc++_shared"; then
    echo "$f needs libc++_shared.so (expected static libc++)"; exit 1
  fi
done

step "package resources + manifest (aapt2) and generate R.java"
PKG=org.zeusport.game
VERSION_CODE="${VERSION_CODE:-1}"
VERSION_NAME="${VERSION_NAME:-0.1.0-dev}"
# Inject the package attribute (Gradle supplies it via `namespace`).
sed "s|<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"|<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\" package=\"$PKG\"|" \
  "$ROOT/android/app/src/main/AndroidManifest.xml" > "$OUT/AndroidManifest.xml"
mkdir -p "$OUT/assets/licenses"
cp "$ROOT"/LICENSES/* "$OUT/assets/licenses/"
# Engine files from the eZeus source (GPL): font, interface texts and its own
# adventures; installed by the importer so no download is needed.
rm -rf "$OUT/assets/ezeus"
mkdir -p "$OUT/assets/ezeus/Fonts" "$OUT/assets/ezeus/Text"
cp "$ROOT/engine/fonts/Zeus.ttf" "$OUT/assets/ezeus/Fonts/Zeus.ttf"
cp "$ROOT/engine/text/language.txt" "$OUT/assets/ezeus/Text/language.txt"
cp -r "$ROOT/engine/Adventures" "$OUT/assets/ezeus/Adventures"
aapt2 compile --dir "$ROOT/android/app/src/main/res" -o "$OUT/res-compiled/res.zip"
aapt2 link -o "$OUT/base.apk" -I "$RES_JAR" -A "$OUT/assets" \
  --manifest "$OUT/AndroidManifest.xml" \
  --min-sdk-version $API --target-sdk-version 35 \
  --version-code "$VERSION_CODE" --version-name "$VERSION_NAME" \
  --java "$OUT/gen" "$OUT/res-compiled/res.zip"

step "compile Java (SDL glue + app)"
javac -nowarn -Xlint:-options -source 8 -target 8 -bootclasspath "$ANDROID_JAR" \
  -d "$OUT/stubs" "$ROOT/tools/android/lambda-stubs/java/lang/invoke/LambdaMetafactory.java"
find "$ROOT/android/app/src/main/java" "$OUT/gen" -name '*.java' > "$OUT/sources.txt"
javac -nowarn -Xlint:-options -source 8 -target 8 -encoding UTF-8 \
  -bootclasspath "$ANDROID_JAR:$OUT/stubs" -d "$OUT/classes" @"$OUT/sources.txt"

step "dex (d8, min-api $API)"
find "$OUT/classes" -name '*.class' > "$OUT/classes.txt"
java -cp "$D8_JAR" com.android.tools.r8.D8 --release --min-api $API \
  --lib "$ANDROID_JAR" --output "$OUT/dex" @"$OUT/classes.txt"

step "add dex and native libs"
cp "$OUT/base.apk" "$OUT/unaligned.apk"
( cd "$OUT/dex" && zip -q -X "$OUT/unaligned.apk" classes*.dex )
# Store .so uncompressed so they can be mmapped directly (page aligned below).
( cd "$OUT" && zip -q -X -0 "$OUT/unaligned.apk" lib/arm64-v8a/*.so )

step "zipalign (-p: page-align .so)"
zipalign -f -p 4 "$OUT/unaligned.apk" "$OUT/aligned.apk"

step "sign (debug key, never committed)"
KS="${DEBUG_KEYSTORE:-$ROOT/build/keys/debug.keystore}"
if [ ! -f "$KS" ]; then
  mkdir -p "$(dirname "$KS")"
  keytool -genkeypair -v -keystore "$KS" -storepass android -keypass android \
    -alias androiddebugkey -dname "CN=Android Debug,O=Android,C=US" \
    -keyalg RSA -keysize 2048 -validity 10000 >/dev/null 2>&1
fi
mkdir -p "$DIST"
apksigner sign --ks "$KS" --ks-pass pass:android --key-pass pass:android \
  --ks-key-alias androiddebugkey --out "$DIST/$APK_NAME" "$OUT/aligned.apk"
apksigner verify --verbose "$DIST/$APK_NAME" | head -5

step "result"
ls -l "$DIST/$APK_NAME"
sha256sum "$DIST/$APK_NAME"
unzip -l "$DIST/$APK_NAME" | tail -n +1 | grep -E "lib/|classes|AndroidManifest" || true
