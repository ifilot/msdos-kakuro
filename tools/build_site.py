#!/usr/bin/env python3
"""Build a self-contained static DOSBox/WASM site from the tested DOS release.

Downloads are pinned and verified; only the first build needs network access.
No third-party requests are made by the resulting website.
"""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import tarfile
import urllib.request
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / 'build/site-cache'
OUT = ROOT / 'build/site'
COMMIT = '6bbf0925d221a7304ea8e0dda1e89b8dc01569b2'
DOSBOX_COMMIT = '528578d5a6f76afd4d5ef4e722b5670826f1183a'
DOWNLOADS = (
    ('emulators-8.5.2.tgz', 'https://registry.npmjs.org/emulators/-/emulators-8.5.2.tgz',
     '99a3e6f257726749915f4dc6828d82cd9834969aa69cc2dcc5ed8babfb323cb4'),
    ('emulators-source.tgz', f'https://codeload.github.com/js-dos/emulators/tar.gz/{COMMIT}',
     '9b7354c2cc853681b99eb288bcdc7e5e0ff7fd2e821485c9b20c12472fbc822e'),
    ('dosbox-source.tgz', f'https://codeload.github.com/js-dos/dosbox/tar.gz/{DOSBOX_COMMIT}',
     '797efaf22246bbc5b2e6be0a6c791a58addd2bca330e0aa93232780a77e06019'),
)
DOSBOX_CONF = '''[sdl]
autolock=false
[dosbox]
machine=svga_s3
memsize=8
[cpu]
core=normal
cputype=auto
cycles=fixed 15000
[mixer]
nosound=false
rate=22050
blocksize=1024
prebuffer=20
[sblaster]
sbtype=sbpro2
sbbase=220
irq=7
dma=1
oplmode=opl2
oplrate=22050
[speaker]
pcspeaker=false
tandy=off
disney=false
[midi]
mpu401=none
mididevice=none
[autoexec]
@echo off
mount c .
c:
KAKURO.EXE
echo KAKURO-BROWSER-SESSION-END
'''


def fetch(name, url, digest):
    path = CACHE / name
    if not path.exists():
        print(f'Downloading {name}', flush=True)
        request = urllib.request.Request(url, headers={'User-Agent': 'msdos-kakuro-site'})
        with urllib.request.urlopen(request, timeout=120) as response:
            data = response.read()
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError(f'Checksum mismatch for {name}')
        path.write_bytes(data)
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise ValueError(f'Checksum mismatch for cached {name}; remove it and rebuild')
    return path


def main():
    release = ROOT / 'build/KAKURO.ZIP'
    if not release.is_file():
        raise SystemExit('DOS archive missing. Run make release first.')
    CACHE.mkdir(parents=True, exist_ok=True)
    downloads = [fetch(*entry) for entry in DOWNLOADS]
    # Prepare in a sibling directory so a failed download never destroys the site.
    staging = ROOT / 'build/site-staging'
    if staging.exists():
        shutil.rmtree(staging)
    shutil.copytree(ROOT / 'site', staging)
    emulator = staging / 'emulator'
    emulator.mkdir()
    with tarfile.open(downloads[0], 'r:gz') as archive:
        for name in ('emulators.js', 'wdosbox.js', 'wdosbox.wasm', 'wlibzip.js', 'wlibzip.wasm'):
            (emulator / name).write_bytes(archive.extractfile('package/dist/' + name).read())
        (emulator / 'LICENSE.txt').write_bytes(archive.extractfile('package/LICENSE').read())
    for path in downloads[1:]:
        shutil.copyfile(path, emulator / path.name)
    (emulator / 'NOTICE.txt').write_text(
        'js-dos emulators 8.5.2 / DOSBox, licensed under GNU GPL version 2.\n'
        'License: LICENSE.txt\n'
        f'Emulators source ({COMMIT}): emulators-source.tgz\n'
        f'DOSBox source submodule ({DOSBOX_COMMIT}): dosbox-source.tgz\n'
        'Source archives are available alongside this notice on this website.\n'
        'Build instructions: emulators-source.tgz README.md and CMakeLists.txt.\n'
        f'Complete upstream tree: https://github.com/js-dos/emulators/tree/{COMMIT}\n'
        'Emscripten GL4ES dependency (MIT):\n'
        'https://github.com/ptitSeb/gl4es/tree/a744af14d4afbda77bf472bc53f43b9ceba39cc0\n'
        'Game source: https://github.com/ifilot/msdos-kakuro\n', encoding='utf-8')
    with ZipFile(release) as dos, ZipFile(staging / 'KAKURO.jsdos', 'w', ZIP_DEFLATED) as bundle:
        for name in ('KAKURO.EXE', 'KAKURO.DAT', 'PUZZLES.DAT', 'SOUND.DAT'):
            bundle.writestr(name, dos.read(name))
        bundle.writestr('.jsdos/dosbox.conf', DOSBOX_CONF)
        bundle.writestr('.jsdos/jsdos.json', json.dumps({'version': '8'}))
        for name in ('PROFONT.TXT', 'LICENSE.TXT', 'NOTICE.TXT'):
            (staging / name).write_bytes(dos.read(name))
    shutil.copyfile(release, staging / 'KAKURO.ZIP')
    shutil.copyfile(ROOT / 'assets/splash/courtyard.png', staging / 'courtyard.png')
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    (staging / 'build.json').write_text(json.dumps({
        'version': (ROOT / 'VERSION').read_text().strip(), 'commit': commit,
        'emulator': '8.5.2',
    }) + '\n')
    (staging / '.nojekyll').touch()
    if OUT.exists():
        shutil.rmtree(OUT)
    staging.rename(OUT)
    print(f'Built {OUT} ({sum(p.stat().st_size for p in OUT.rglob("*") if p.is_file()):,} bytes)')
    print('Preview: python3 -m http.server 8000 --directory build/site')


if __name__ == '__main__':
    main()
