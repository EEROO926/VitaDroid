# VitaDroid

Experimental Android APK analysis and compatibility project
for the PlayStation Vita.

## Vision

VitaDroid aims to provide a single PS Vita application that can:

1. Select an Android APK from storage.
2. Analyze its architecture and dependencies.
3. Determine whether it is a possible compatibility candidate.
4. Apply reusable compatibility components.
5. Launch supported games using the Vita's controls.

The long-term goal is to reduce the need for manually porting
individual older Android games.

## Current status: v0.2 (experimental)

Implemented:
- PS Vita homebrew application prototype
- Basic ZIP/APK directory analysis
- Manifest and DEX file presence detection
- ARMv7, ARM64 and x86 library directory detection
- Basic Unity file signature detection
- Desktop analyzer and five automated tests

Not implemented:
- General APK file picker
- AndroidManifest.xml content decoding
- Android runtime or Java execution
- Automatic compatibility and game launching
- Graphics, audio and input translation
- Full Android application compatibility

The APK analyzer has passed five synthetic desktop tests.
Hardware behavior has not yet been verified.

## Build

Requires VitaSDK and CMake.

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j2
```

The generated VPK is located at:
`build-debug/vitadroid.vpk`

Release builds currently have an unresolved ELF segment
layout problem. Debug builds are the tested configuration.

## Tests

```bash
gcc -std=c11 -Wall -Wextra -O0 \
  tests/host_analyzer.c -o tests/host_analyzer

python3 tests/run_tests.py
```

## Current APK location

The prototype currently reads:
`ux0:data/VitaDroid/test.apk`

The file picker is not implemented yet.

## Legal notes

VitaDroid does not include commercial games, APK files,
proprietary game assets or Android system components.

Users and contributors must respect applicable licenses
and intellectual property rights.

## Third-party credits

Based in part on examples from:
https://github.com/vitasdk/samples

VitaSDK sample code and build scripts are published under CC0 1.0.

## License

MIT for original VitaDroid project code.
Third-party components retain their respective licenses.
