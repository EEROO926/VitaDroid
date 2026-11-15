import subprocess
import tempfile
import zipfile
from pathlib import Path

analyzer = Path("tests/host_analyzer").resolve()

cases = [
    (
        "ARMv7 APK",
        ["AndroidManifest.xml", "classes.dex",
         "lib/armeabi-v7a/libgame.so"],
        ["Manifest: YES", "DEX: YES", "ARMv7: YES",
         "ARM64: NO", "Unity hints: NO"]
    ),
    (
        "ARM64 APK",
        ["AndroidManifest.xml", "classes.dex",
         "lib/arm64-v8a/libgame.so"],
        ["ARMv7: NO", "ARM64: YES"]
    ),
    (
        "Unity APK",
        ["AndroidManifest.xml", "classes.dex",
         "lib/armeabi-v7a/libunity.so",
         "assets/bin/Data/globalgamemanagers"],
        ["Unity hints: YES", "ARMv7: YES"]
    ),
    (
        "DEX-only APK",
        ["AndroidManifest.xml", "classes.dex"],
        ["DEX: YES", "Native libs: NO"]
    ),
]

passed = 0

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)

    for index, (name, entries, expected) in enumerate(cases):
        path = root / f"test_{index}.apk"

        with zipfile.ZipFile(path, "w",
                             compression=zipfile.ZIP_DEFLATED) as apk:
            for entry in entries:
                apk.writestr(entry, b"TEST DATA")

        result = subprocess.run(
            [str(analyzer), str(path)],
            capture_output=True, text=True
        )

        success = (
            result.returncode == 0
            and all(text in result.stdout for text in expected)
        )

        print(f"{'PASS' if success else 'FAIL'}: {name}")

        if success:
            passed += 1
        else:
            print(result.stdout)
            print(result.stderr)

    invalid = root / "invalid.apk"
    invalid.write_bytes(b"NOT A ZIP FILE" * 5)

    result = subprocess.run(
        [str(analyzer), str(invalid)],
        capture_output=True, text=True
    )

    success = "ZIP directory not found." in result.stdout
    print(f"{'PASS' if success else 'FAIL'}: Invalid ZIP detection")
    passed += int(success)

print(f"\nRESULT: {passed}/5 tests passed")
raise SystemExit(0 if passed == 5 else 1)
