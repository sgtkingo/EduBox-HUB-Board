"""Compile actual Board router/drivers against simulated GPIO; artifacts stay in temp."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
VSCP = ROOT.parent / "vscp/src"
DEVICES = ROOT.parent / "devices/src"
SOURCES = [VSCP / name for name in (
    "vscp_client.cpp", "vscp_server.cpp", "vscp_codec.cpp", "vscp_types.cpp", "io/vscp_transport.cpp",
)]
with tempfile.TemporaryDirectory(prefix="edubox-control-safety-") as directory:
    for test in sorted((ROOT / "tests").glob("*_test.cpp")):
        executable = Path(directory) / (test.stem + ".exe")
        subprocess.run([
            "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Wno-sign-compare", "-DARDUINO_H_ENV",
            "-I", str(ROOT / "tests/stubs"), "-I", str(ROOT / "src"),
            "-I", str(VSCP), "-I", str(DEVICES), str(test),
            str(ROOT / "src/VscpDeviceRouter.cpp"),
            str(DEVICES / "DC.cpp"), str(DEVICES / "BuzzP.cpp"), str(DEVICES / "Stepper.cpp"),
            str(DEVICES / "SG90.cpp"), *map(str, SOURCES), "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True, timeout=15)
        print(f"PASS {test.name}", flush=True)
