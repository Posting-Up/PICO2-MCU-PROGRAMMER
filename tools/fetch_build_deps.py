"""Fetch portable build dependencies into ignored .deps; no system installation."""
from pathlib import Path
import hashlib
import json
import subprocess
import urllib.request
import zipfile

root = Path(__file__).resolve().parents[1]
deps = root / ".deps"
deps.mkdir(exist_ok=True)
sdk = deps / "pico-sdk"
if not sdk.exists():
    subprocess.run(["git", "clone", "--depth", "1", "--branch", "2.2.0",
                    "https://github.com/raspberrypi/pico-sdk.git", str(sdk)], check=True)
subprocess.run(["git", "-C", str(sdk), "submodule", "update", "--init", "--depth", "1", "lib/tinyusb"], check=True)
tag = "v14.2.1-1.1"
filename = "xpack-arm-none-eabi-gcc-14.2.1-1.1-win32-x64.zip"
base = f"https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/{tag}/"
archive = deps / filename
checksum_file = deps / (filename + ".sha")
# Obtain the published checksum directly from the distribution's release.
request = urllib.request.Request(
    f"https://api.github.com/repos/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/tags/{tag}",
    headers={"User-Agent": "ATxmega192A3U-Programmer-build"})
with urllib.request.urlopen(request, timeout=60) as response:
    release = json.load(response)
asset = next(a for a in release["assets"] if a["name"] == filename + ".sha")
if not archive.exists():
    print("Downloading portable Arm compiler...", flush=True)
    urllib.request.urlretrieve(base + filename, archive)
urllib.request.urlretrieve(asset["browser_download_url"], checksum_file)
expected = checksum_file.read_text().split()[0]
actual = hashlib.sha256(archive.read_bytes()).hexdigest()
if actual != expected:
    raise RuntimeError("Arm compiler archive checksum mismatch")
toolchain = deps / "xpack-arm-none-eabi-gcc-14.2.1-1.1"
if not toolchain.exists():
    with zipfile.ZipFile(archive) as bundle:
        bundle.extractall(deps)
print(f"SDK: {sdk}")
print(f"Compiler: {toolchain / 'bin'}")
