#!/usr/bin/env python3
"""Build, test, or visibly run the DOS application, locally or with Docker."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import re

ROOT = Path(__file__).resolve().parent.parent


def emulator():
    return os.environ.get("DOSBOX") or shutil.which("dosbox-x") or shutil.which("dosbox")


def docker(action, puzzle="001", fonts=False):
    command = ["docker", "run", "--rm", "--network", "none", "--user",
               f"{os.getuid()}:{os.getgid()}", "--volume", f"{ROOT}:/workspace"]
    for name in ("DOS_CPU", "DOS_CYCLES", "DOS_SOURCE_ROOT", "DOS_COMPILER_FLAGS", "DOS_OUTPUT_NAME", "BUILD_COMMIT", "SOURCE_DATE_EPOCH", "DOS_SOUND_CARD", "DOS_MIDI_DEVICE", "DOS_MIDI_CONFIG"):
        if name in os.environ: command += ["--env", f"{name}={os.environ[name]}"]
    # Docker's compiler image need not include Git; capture the host checkout ID.
    if "BUILD_COMMIT" not in os.environ:
        try:
            revision = subprocess.run(["git", "rev-parse", "--short=12", "HEAD"],
                                      cwd=ROOT, capture_output=True, text=True, timeout=5)
            if revision.returncode == 0:
                command += ["--env", "BUILD_COMMIT=" + revision.stdout.strip()]
        except (FileNotFoundError, subprocess.TimeoutExpired):
            pass
    if action == "run":
        display = os.environ.get("DISPLAY")
        if not display or not Path("/tmp/.X11-unix").is_dir():
            raise SystemExit("Visible Docker execution needs X11/WSLg. Alternatively install desktop DOSBox-X and set DOSBOX.")
        command += ["--env", f"DISPLAY={display}", "--env", "SDL_VIDEODRIVER=x11",
                    "--volume", "/tmp/.X11-unix:/tmp/.X11-unix"]
        authority = os.environ.get("XAUTHORITY")
        if authority and Path(authority).is_file():
            command += ["--volume", f"{authority}:/tmp/kakuro-xauthority:ro",
                        "--env", "XAUTHORITY=/tmp/kakuro-xauthority"]
        # WSLg exposes PulseAudio over a Unix socket outside the container.
        pulse = os.environ.get('PULSE_SERVER', '')
        if pulse.startswith('unix:') and Path(pulse[5:]).is_socket():
            command += ['--volume', pulse[5:] + ':/tmp/kakuro-pulse',
                        '--env', 'PULSE_SERVER=unix:/tmp/kakuro-pulse',
                        '--env', 'SDL_AUDIODRIVER=pulseaudio']
        soundfont = os.environ.get('DOS_SOUNDFONT')
        if soundfont and Path(soundfont).is_file():
            command += ['--volume', str(Path(soundfont).resolve()) + ':/tmp/kakuro-gm.sf2:ro',
                        '--env', 'DOS_SOUNDFONT=/tmp/kakuro-gm.sf2']
    command += [os.environ.get("DOS_IMAGE", "kakuro-build"), "python3", "tools/dos.py", action]
    if action == "run": command += ["--puzzle", puzzle]
    if fonts: command += ["--fonts"]
    try:
        subprocess.run(command, check=True)
    except FileNotFoundError:
        raise SystemExit("Install DOSBox-X or Docker; see buildenv/README.md")
    except subprocess.CalledProcessError as exc:
        raise SystemExit(exc.returncode)


def configuration(path, cpu=None, cycles=None, audio=False):
    cpu = cpu or os.environ.get("DOS_CPU", "8086")
    cycles = cycles or os.environ.get("DOS_CYCLES", "30000")
    card = os.environ.get('DOS_SOUND_CARD', 'sbpro2')
    soundfont = os.environ.get('DOS_SOUNDFONT', '/usr/share/sounds/sf2/default-GM.sf2')
    default_midi = 'fluidsynth' if Path(soundfont).is_file() and 'dosbox-x' in (emulator() or '') else 'none'
    midi = os.environ.get('DOS_MIDI_DEVICE', default_midi) if audio else 'none'
    midi_config = os.environ.get('DOS_MIDI_CONFIG', '') if audio else ''
    path.write_text(f"[sdl]\nfullscreen=false\nwindowresolution=960x720\noutput=surface\n[render]\naspect=true\nscaler=normal3x\n[dosbox]\nquit warning=false\nmachine=svga_s3\nmemsize=16\n[cpu]\ncore=normal\ncputype={cpu}\ncycles=fixed {cycles}\n[mixer]\nnosound={'false' if audio else 'true'}\n[sblaster]\nsbtype={card}\nsbbase=220\noplmode=opl2\n[speaker]\npcspeaker=false\n[midi]\nmpu401=intelligent\nmididevice={midi}\nmidiconfig={midi_config}\nfluid.soundfont={soundfont if audio else ''}\n[autoexec]\n")


def copy_puzzles(destination):
    destination.mkdir(parents=True, exist_ok=True)
    for source in (ROOT / "assets/puzzles").glob("*.puz"):
        shutil.copyfile(source, destination / source.name.upper())


def copy_assets(destination, loose=True):
    shutil.copyfile(ROOT / 'assets/KAKURO.DAT', destination / 'KAKURO.DAT')
    shutil.copyfile(ROOT / 'assets/PUZZLES.DAT', destination / 'PUZZLES.DAT')
    shutil.copyfile(ROOT / 'assets/SOUND.DAT', destination / 'SOUND.DAT')
    if loose:
        for name in ('START.VGA', 'START.PAL'):
            shutil.copyfile(ROOT / 'assets/splash' / name, destination / name)
        shutil.copyfile(ROOT / 'assets/background/BOARD.VGA', destination / 'BOARD.VGA')
        shutil.copyfile(ROOT / 'assets/menu/MENU.VGA', destination / 'MENU.VGA')
    else:
        for name in ('START.VGA', 'START.PAL', 'BOARD.VGA', 'MENU.VGA'):
            (destination / name).unlink(missing_ok=True)


def build(test=None):
    subprocess.run(["python3", str(ROOT / "tools/generate_buildinfo.py")], check=True)
    toolchain = Path(os.environ.get("DOS_TOOLCHAIN", ROOT / "buildenv")).resolve()
    if not (toolchain / "BC/BIN/BCC.EXE").is_file():
        raise SystemExit("Missing Borland C++ compiler: " + str(toolchain))
    output = ROOT / "build" / os.environ.get("DOS_OUTPUT_NAME", test + "-test" if test else "app")
    output.mkdir(parents=True, exist_ok=True)
    test_main = {"video": "VGATEST", "puzzle": "PUZTEST", "fonts": "FONTTEST", "start": "STARTTST", "menu": "MENUTEST", "perf": "PERFTST", "assets": "ASSETTST", "documents": "DOCTEST", "archive": "PACKTEST", "sound": "SNDTEST"}.get(test)
    executable = test_main + ".EXE" if test else "KAKURO.EXE"
    (output / executable).unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(prefix="kakuro-build-") as temporary:
        drive = Path(temporary)
        for source in (ROOT / os.environ.get("DOS_SOURCE_ROOT", "src")).glob("*"):
            if source.is_file():
                shutil.copyfile(source, drive / source.name)
        if test:
            shutil.copyfile(ROOT / ("tests/" + test_main + ".CPP"), drive / (test_main + ".CPP"))
        if test == "archive": copy_puzzles(drive / "SOURCE")
        copy_assets(drive)
        if test == "start":
            shutil.copyfile(ROOT / 'assets/splash/courtyard.bin', drive / 'SOURCE.BIN')
        if test == "puzzle":
            shutil.copytree(ROOT / "tests/fixtures", drive / "FIXTURES")
        main = test_main + ".CPP" if test else "MAIN.CPP"
        # DOS limits a command tail to 126 bytes; keep growing source lists in a response file.
        extra = " ASSETS.CPP" if (drive / "ASSETS.CPP").exists() else ""
        (drive / "COMPILE.RSP").write_text(
            f"-ml {os.environ.get('DOS_COMPILER_FLAGS', '-G -1-')} -IC:\\BC\\INCLUDE -LC:\\BC\\LIB -e{executable}\n"
            f"{main} VGA.CPP PUZZLE.CPP GAME.CPP DIGITS.CPP START.CPP PROFONT.CPP MENU.CPP BLOSSOM.CPP DOCVIEW.CPP SOUND.CPP SNDDRV.CPP SOUNDMNU.CPP{extra}\n")
        lines = ["@echo off", "path C:\\BC\\BIN", "D:",
                 "bcc @COMPILE.RSP > COMPILE.TXT",
                 "if errorlevel 1 goto failed", f"if not exist {executable} goto failed"]
        lines += ["echo passed>PASS.TXT", "goto done", ":failed", "echo failed>FAIL.TXT", ":done", "exit", ""]
        (drive / "BUILD.BAT").write_bytes("\r\n".join(lines).encode("ascii"))
        config = drive / "dosbox.conf"
        # The compiler needs a newer CPU; its output is explicitly 8086/8088 code.
        configuration(config, cpu="386", cycles="30000")
        env = os.environ.copy()
        env.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
        command = [emulator(), "-conf", str(config), "-c", f'mount c "{toolchain}"',
                   "-c", f'mount d "{drive}"', "-c", "D:", "-c", "BUILD.BAT"]
        try:
            result = subprocess.run(command, env=env, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=180 if test == "perf" else 60)
        except subprocess.TimeoutExpired as exc:
            (output / "EMULATOR.TXT").write_bytes(exc.stdout or b"")
            raise SystemExit("DOSBox timed out; see " + str(output))
        if test and not result.returncode and (drive / "PASS.TXT").exists() and not (drive / "FAIL.TXT").exists():
            (drive / "PASS.TXT").unlink()
            test_lines = ["@echo off", "D:", executable, "if errorlevel 1 goto failed",
                          "echo passed>PASS.TXT", "goto done", ":failed", "echo failed>FAIL.TXT",
                          ":done", "exit", ""]
            (drive / "TEST.BAT").write_bytes("\r\n".join(test_lines).encode("ascii"))
            configuration(config)
            command[-1] = "TEST.BAT"
            try:
                tested = subprocess.run(command, env=env, stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT, timeout=180 if test == "perf" else 60)
            except subprocess.TimeoutExpired as exc:
                (output / "EMULATOR.TXT").write_bytes(result.stdout + (exc.stdout or b""))
                raise SystemExit("DOS test timed out; see " + str(output))
            result = subprocess.CompletedProcess(command, tested.returncode, result.stdout + tested.stdout)
        (output / "EMULATOR.TXT").write_bytes(result.stdout)
        for name in ("COMPILE.TXT", "RESULT.TXT", "SCREEN.RAW", "PALETTE.RAW",
                     "MODAL.RAW", "MODAL.PAL", "CHECKS.TXT", "PERFORM.TXT", "ABOUT.RAW"):
            (output / name).unlink(missing_ok=True)
            if (drive / name).exists():
                shutil.copyfile(drive / name, output / name)
        log = output / "COMPILE.TXT"
        if log.exists():
            print(log.read_text(errors="replace").rstrip())
        if result.returncode or not (drive / "PASS.TXT").exists() or (drive / "FAIL.TXT").exists():
            checks = output / "CHECKS.TXT"
            if checks.exists():
                print(checks.read_text(errors="replace").rstrip())
            raise SystemExit("DOS build/test failed; see " + str(output))
        expected = {"video": "VGA tests passed", "puzzle": "Puzzle tests passed", "fonts": "Font tests passed", "start": "Start screen tests passed", "menu": "Menu tests passed", "perf": "Performance tests passed", "assets": "Asset tests passed", "documents": "Document tests passed", "archive": "Puzzle archive tests passed", "sound": "Sound tests passed"}.get(test)
        if test and (not (output / "RESULT.TXT").exists()
                     or (output / "RESULT.TXT").read_text().strip() != expected):
            raise SystemExit("Missing test result")
        shutil.copyfile(drive / executable, output / executable)
        if not test:
            if (output / "PUZZLES").exists(): shutil.rmtree(output / "PUZZLES")
            copy_assets(output, loose=False)
        print(expected if test else "Built " + str(output / executable))


def run(puzzle, fonts=False):
    output = ROOT / "build/app"
    if not (output / "KAKURO.EXE").exists():
        build()
    config = output / "run.conf"
    configuration(config, audio=not fonts)
    env = os.environ.copy()
    if env.get("SDL_VIDEODRIVER") == "dummy":
        env.pop("SDL_VIDEODRIVER")
    if env.get('SDL_AUDIODRIVER') == 'dummy':
        env.pop('SDL_AUDIODRIVER')
    print("Opening " + ("font samples" if fonts else "Kakuro puzzle " + puzzle)
          + (". Press any key to exit." if fonts else ". Enter opens the journal; arrows select, PgUp/PgDn change pages. Escape returns to the menu or exits."), flush=True)
    app_command = "KAKURO.EXE /FONTS" if fonts else "KAKURO.EXE PUZZLES\\" + puzzle.upper() + ".PUZ"
    subprocess.run([emulator(), "-conf", str(config), "-c", f'mount c "{output}"',
                    "-c", "C:", "-c", app_command, "-c", "exit"], env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("build", "run", "test", "test-puzzle", "test-fonts", "test-start", "test-menu", "test-perf", "test-assets", "test-documents", "test-archive", "test-sound"))
    parser.add_argument("--puzzle", default="001")
    parser.add_argument("--fonts", action="store_true")
    args = parser.parse_args()
    action = args.action
    if not re.fullmatch(r"[0-9]{3}", args.puzzle):
        parser.error("Puzzle must be a three-digit ID")
    if action == "run" and not args.fonts:
        path = ROOT / "assets/puzzles" / (args.puzzle.lower() + ".puz")
        if not path.is_file() or not path.stat().st_size:
            parser.error("Puzzle is missing or empty: " + args.puzzle)
    if not emulator():
        docker(action, args.puzzle, args.fonts)
    elif action == "run":
        run(args.puzzle, args.fonts)
    else:
        build(test={"test": "video", "test-puzzle": "puzzle", "test-fonts": "fonts", "test-start": "start", "test-menu": "menu", "test-perf": "perf", "test-assets": "assets", "test-documents": "documents", "test-archive": "archive", "test-sound": "sound"}.get(action))


if __name__ == "__main__":
    main()
