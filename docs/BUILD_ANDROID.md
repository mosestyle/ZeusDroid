# Building the Android APK

Target: `arm64-v8a`, minSdk 26 (Android 8.0), targetSdk 35. Native code: eZeus engine as
`libmain.so` + SDL2 2.32.10, SDL2_image 2.8.12, SDL2_mixer 2.8.2, SDL2_ttf 2.24.0 (all
built from source by the engine's CMake via FetchContent), static libc++.

There are two ways to build.

## A. Android Studio / Gradle (recommended on your own PC)

Prerequisites: Android Studio (Ladybug or newer) with SDK Platform 35, NDK
**27.2.12479018** (r27c) and CMake 3.22.1+ installed through the SDK Manager.

```
cd android
gradle wrapper --gradle-version 8.9      # once; the wrapper jar is not committed
./gradlew assembleDebug                  # Windows: gradlew.bat assembleDebug
```
Output: `android/app/build/outputs/apk/debug/app-debug.apk`.

Status: the Gradle files (`android/settings.gradle`, `android/build.gradle`,
`android/app/build.gradle`, AGP 8.7.3) are written against released versions but have
**not been executed** – Gradle, Google Maven and dl.google.com are blocked in the
environment where this port was developed.

## B. Gradle-free script (what produced the current APK)

`tools/build_android.sh` runs: CMake/Ninja native build → strip → `aapt2 compile/link` →
`javac` → `d8` → zip → `zipalign -p` → `apksigner` (debug key generated locally in
`build/keys/`, git-ignored).

Environment variables (defaults are the agent environment's paths):

| Variable | Meaning |
|---|---|
| `ANDROID_NDK` | NDK root (needs `build/cmake/android.toolchain.cmake`, sysroot, clang) |
| `ANDROID_JAR` | `platforms/android-35/android.jar` (javac bootclasspath) |
| `D8_JAR` | `build-tools/<v>/lib/d8.jar` |
| `RES_JAR` | framework jar for `aapt2 link` (default Debian `android-23` jar, see below) |
| `JOBS` | ninja parallelism |

```
tools/build_android.sh
adb install -r dist/ZeusDroid-debug-arm64-v8a.apk
```

### How the restricted agent environment was set up (and its caveats)

The development container could reach GitHub, Ubuntu archives, PyPI and npm, but **not**
`dl.google.com`, `maven.google.com`, `services.gradle.org` or Maven Central. Therefore:

* **Compiler:** Ubuntu's `clang-20`/`lld-20` (x86-64, Ubuntu archive), wrapped as the NDK's
  `clang`/`clang++` with `--rtlib=compiler-rt --unwindlib=libunwind` and a resource dir
  combining clang-20 headers with the NDK's AArch64 runtime libraries.
* **NDK sysroot + runtime libs, SDK `android.jar` (35) and `d8.jar`:** taken from the
  third-party GitHub release `lzhiyong/termux-ndk` (`android-ndk-r29-aarch64.tar.xz`,
  sha256 `02e10e4d…684b`; `android-sdk-aarch64.tar.xz`, sha256 `8a23d2a1…71ad`). The host
  binaries in those archives (aarch64) were **not** used; only platform-independent
  headers, stub libraries, `android.jar` and `d8.jar`. **Provenance could not be verified
  against Google's checksums** from inside the sandbox. For a release build, rebuild with
  an official NDK/SDK (option A or this script with official paths).
* **aapt2, zipalign, apksigner:** Debian/Ubuntu packages (`aapt2` 2.19, `zipalign`,
  `apksigner` 0.9). Debian's aapt2 cannot parse the sparse resource tables of android-33+
  framework jars, so resources are linked against Debian's `android-23` jar. Consequence:
  the manifest only uses attributes that exist in API 23 (the "Manage game data" launcher
  shortcut is registered at runtime via `ShortcutManager`).
* **Lambdas:** `android.jar` lacks `LambdaMetafactory.metafactory`; a compile-time-only stub
  written for this project (`tools/android/lambda-stubs`) is on javac's bootclasspath.
  d8 desugars lambdas, so the stub is never packaged.
* **Tests on ARM64:** unit tests are additionally built as static `aarch64-linux-android26`
  executables and run under `qemu-aarch64` (Ubuntu `qemu-user`).

### Checks performed by the script

* Every packaged `.so` is an AArch64 ELF and none depends on `libc++_shared.so`.
* `apksigner verify` (v2/v3 signature).
* No x86 code, no Box86/Box64/Wine/emulator components exist anywhere in the project.

## Installing on a phone

1. Enable *Developer options → USB debugging* (or allow installs from your file manager).
2. `adb install -r dist/ZeusDroid-debug-arm64-v8a.apk`, or copy the APK to the phone and
   open it.
3. Launch **eZeus Port**, import your game folder and the eZeus data pack (GAME_DATA.md).
4. Logs: `adb logcat -s SDL SDL/APP ZeusImport`.
