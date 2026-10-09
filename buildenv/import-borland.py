#!/usr/bin/env python3
"""Extract the DOS tools from the pinned Borland C++ 3.1 disk archive."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile

ARCHIVE_SHA256 = "76e155a7fb7eb0590930ecd513644c977497a020466a272943776fbf683d0f27"


def run(*args):
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--destination", type=Path, default=Path(__file__).resolve().parent / "BC")
    parser.add_argument("--sevenzip", default="7z")
    args = parser.parse_args()
    archive = args.archive.resolve()
    if hashlib.sha256(archive.read_bytes()).hexdigest() != ARCHIVE_SHA256:
        parser.error("Archive checksum differs from the documented WinWorld media")
    destination = args.destination.resolve()
    if destination.exists():
        parser.error("Destination already exists; choose an empty destination")
    with tempfile.TemporaryDirectory(prefix="kakuro-borland-") as scratch:
        root = Path(scratch)
        disks = root / "disks"
        setup = root / "setup"
        installed = root / "BC"
        setup.mkdir()
        run(args.sevenzip, "x", str(archive), "-o" + str(disks))
        images = sorted(disks.rglob("Disk*.img"))
        if len(images) != 15:
            parser.error("Expected fifteen installation disks")
        for image in images:
            run("mcopy", "-o", "-i", str(image), "::*", str(setup))
        # Borland's split archives have a five-byte volume header on each part.
        for name in ("CMDLINE", "BC", "TASM", "TD"):
            parts = sorted(setup.glob(name + ".CA*"))
            (setup / (name + ".ZIP")).write_bytes(b"".join(p.read_bytes()[5:] for p in parts))
        groups = {
            "BIN": ("BIN", "CMDLINE", "BC", "IDE", "CHELP31", "TASM", "TD", "TDUTIL"),
            "INCLUDE": ("INCLUDE", "CLASSINC"),
            "LIB": ("SLIB", "CLIB", "MLIB", "LLIB", "HLIB", "XLIB", "CLASSLIB"),
            "DOC": ("DOC",),
        }
        for directory, packages in groups.items():
            target = installed / directory
            target.mkdir(parents=True)
            for package in packages:
                # Info-ZIP supports the historical ZIP implode compression.
                run("unzip", "-oq", str(setup / (package + ".ZIP")), "-d", str(target))
        sysdir = installed / "INCLUDE" / "SYS"
        sysdir.mkdir()
        for name in ("STAT.H", "TIMEB.H", "TYPES.H"):
            shutil.copyfile(installed / "INCLUDE" / name, sysdir / name)
        shutil.copyfile(setup / "README", installed / "README")
        shutil.copyfile(setup / "FILELIST.DOC", installed / "DOC" / "FILELIST.DOC")
        (installed / "BIN" / "BCC.CFG").write_bytes(b"-IC:\\BC\\INCLUDE\r\n-LC:\\BC\\LIB\r\n")
        (installed / "BIN" / "TLINK.CFG").write_bytes(b"/LC:\\BC\\LIB\r\n")
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(installed, destination)
    print("Installed DOS toolchain:", destination)


if __name__ == "__main__":
    main()
