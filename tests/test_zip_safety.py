import subprocess
import tempfile
import zipfile
import struct
from pathlib import Path

reader = Path("tests/test_zip_reader").resolve()
target = "AndroidManifest.xml"

def check(name, content, should_succeed):
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "test.apk"
        path.write_bytes(content)

        try:
            result = subprocess.run(
                [str(reader), str(path), target],
                capture_output=True,
                text=True,
                timeout=5
            )
            success = result.returncode == 0
            passed = success == should_succeed
        except subprocess.TimeoutExpired:
            passed = False

        print(f"{'PASS' if passed else 'FAIL'}: {name}")
        return passed

def make_zip(data, compression=zipfile.ZIP_DEFLATED):
    import io
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w", compression=compression) as z:
        z.writestr(target, data)
    return stream.getvalue()

results = []

normal = make_zip(b"VitaDroid" * 1000)

results.append(check(
    "Valid Deflate ZIP", normal, True
))

results.append(check(
    "Invalid ZIP header", b"NOT A ZIP FILE", False
))

# Central-directory CRC32 value is corrupted.
bad_crc = bytearray(normal)
central = bad_crc.find(b"PK\x01\x02")
assert central >= 0
crc = struct.unpack_from("<I", bad_crc, central + 16)[0]
struct.pack_into("<I", bad_crc, central + 16, crc ^ 0xFFFFFFFF)

results.append(check(
    "Incorrect CRC32", bytes(bad_crc), False
))

# The entry advertises more than the 1 MiB output limit.
oversized = make_zip(b"A" * (1024 * 1024 + 1))

results.append(check(
    "Oversized decompressed entry", oversized, False
))

# ZIP central directory is removed.
results.append(check(
    "Truncated ZIP", normal[:-22], False
))

print(f"\nRESULT: {sum(results)}/{len(results)} safety tests passed")
raise SystemExit(0 if all(results) else 1)
