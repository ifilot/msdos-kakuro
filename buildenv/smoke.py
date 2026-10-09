#!/usr/bin/env python3
"""Build and run a real 16-bit C++ executable in DOSBox."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    toolchain = Path(os.environ.get("DOS_TOOLCHAIN", ROOT / "buildenv")).resolve()
    compiler = toolchain / "BC" / "BIN" / "BCC.EXE"
    if not compiler.is_file():
        raise SystemExit("Missing compiler: " + str(compiler))
    emulator = os.environ.get("DOSBOX") or shutil.which("dosbox-x") or shutil.which("dosbox")
    if not emulator:
        raise SystemExit("Install DOSBox-X/DOSBox or use the documented Docker image")
    output = ROOT / "build" / "smoke"
    output.mkdir(parents=True, exist_ok=True)
    for name in ("SMOKE.EXE", "COMPILE.TXT", "RESULT.TXT", "EMULATOR.TXT"):
        (output / name).unlink(missing_ok=True)
    # A fresh DOS drive prevents stale success files from masking failures.
    with tempfile.TemporaryDirectory(prefix="kakuro-smoke-") as scratch:
        drive = Path(scratch)
        shutil.copyfile(ROOT / "buildenv" / "SMOKE.CPP", drive / "SMOKE.CPP")
        batch = "\r\n".join([
            "@echo off", "path C:\\BC\\BIN", "D:",
            "bcc -ml -v -IC:\\BC\\INCLUDE -LC:\\BC\\LIB -eSMOKE.EXE SMOKE.CPP > COMPILE.TXT",
            "if errorlevel 1 goto failed", "if not exist SMOKE.EXE goto failed",
            "SMOKE.EXE", "if errorlevel 1 goto failed",
            "echo passed>PASS.TXT", "goto done", ":failed", "echo failed>FAIL.TXT",
            ":done", "exit", "",
        ])
        (drive / "BUILD.BAT").write_bytes(batch.encode("ascii"))
        config = drive / "dosbox.conf"
        config.write_text("[sdl]\nfullscreen=false\noutput=surface\n[dosbox]\nmachine=svga_s3\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=fixed 30000\n[mixer]\nnosound=true\n[autoexec]\n")
        env = os.environ.copy()
        env.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
        command = [emulator, "-conf", str(config), "-c", 'mount c "' + str(toolchain) + '"',
                   "-c", 'mount d "' + str(drive) + '"', "-c", "D:", "-c", "BUILD.BAT"]
        try:
            result = subprocess.run(command, env=env, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=60)
        except subprocess.TimeoutExpired as exc:
            (output / "EMULATOR.TXT").write_bytes(exc.stdout or b"")
            raise SystemExit("DOSBox timed out; see build/smoke/EMULATOR.TXT")
        (output / "EMULATOR.TXT").write_bytes(result.stdout)
        for name in ("COMPILE.TXT", "RESULT.TXT"):
            target = output / name
            target.unlink(missing_ok=True)
            if (drive / name).exists():
                shutil.copyfile(drive / name, target)
        (output / "SMOKE.EXE").unlink(missing_ok=True)
        log = (drive / "COMPILE.TXT").read_text(errors="replace") if (drive / "COMPILE.TXT").exists() else ""
        print(log.rstrip())
        expected = "Borland C++ DOS smoke test passed: sum=45"
        runtime = (drive / "RESULT.TXT").read_text().strip() if (drive / "RESULT.TXT").exists() else ""
        if result.returncode or not (drive / "PASS.TXT").exists() or (drive / "FAIL.TXT").exists() or runtime != expected:
            raise SystemExit("DOS compilation or execution failed; see build/smoke logs")
        shutil.copyfile(drive / "SMOKE.EXE", output / "SMOKE.EXE")
        print(runtime)
        print("Executable:", output / "SMOKE.EXE")


if __name__ == "__main__":
    main()
