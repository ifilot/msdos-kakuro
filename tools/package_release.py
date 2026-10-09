#!/usr/bin/env python3
"""Package the DOS executable, resource packs and documentation."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import shutil

ROOT = Path(__file__).resolve().parent.parent


def main():
    source = ROOT / 'build/app'
    destination = ROOT / 'build/release'
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir(parents=True)
    for name in ('KAKURO.EXE', 'KAKURO.DAT', 'PUZZLES.DAT', 'SOUND.DAT'):
        shutil.copyfile(source / name, destination / name)
    for name, path in (('LICENSE.TXT', 'LICENSE'),
                       ('NOTICE.TXT', 'NOTICE.md'),
                       ('PROFONT.TXT', 'assets/fonts/PROFONT-LICENSE.txt'),
                       ('FONTS.TXT', 'assets/fonts/internet/README.md')):
        shutil.copyfile(ROOT / path, destination / name)
    shutil.copyfile(ROOT / 'assets/quickmenu/KAKURO.ICC', destination / 'KAKURO.ICC')
    version = (ROOT / 'VERSION').read_text().strip()
    (destination / 'README.TXT').write_bytes(
        ('Version ' + version + '\r\n').encode('ascii') +
        b'KAKURO - 8086/8088 DOS, VGA 640x480, 16 colors\r\n'
        b'Source: https://github.com/ifilot/msdos-kakuro\r\n'
        b'License: GNU GPL version 3; see LICENSE.TXT and NOTICE.TXT.\r\n'
        b'Run KAKURO from this directory. Keep KAKURO.DAT and PUZZLES.DAT beside it.\r\n'
        b'Enter opens the journal. Arrows select, PgUp/PgDn change pages.\r\n'
        b'F1 opens Help; F2 opens About. Arrows/PgUp/PgDn scroll.\r\n'
        b'Speaker F3/E toggles effects; music note F4/M toggles music.\r\n'
        b'Buttons work in journal and puzzle. Cog/F5 opens hardware settings.\r\n'
        b'With a DOS mouse driver: click cards, buttons and answer cells.\r\n'
        b'Click digit/Clear buttons to enter numbers; Back returns to journal.\r\n'
        b'Right-click acts as Esc. Mouse support is optional.\r\n'
        b'Keep SOUND.DAT beside KAKURO.EXE. Sound settings save to SOUND.CFG.\r\n'
        b'AdLib / Sound Blaster FM / MPU-401 General MIDI; no PC speaker.\r\n'
        b'MPU-401 requires a connected General MIDI synthesizer.\r\n'
        b'Enter plays; 1-9 enter digits, Backspace/Delete/0 clear.\r\n'
        b'When solved: Enter returns to overview; Esc views the board.\r\n'
        b'Esc returns to the journal; Esc there exits.\r\n'
        b'Quickmenu: import KAKURO.ICC in its icon editor for your shortcut.\r\n'
        b'Entries and status marks last for this session only.\r\n')
    archive = ROOT / 'build/KAKURO.ZIP'
    with ZipFile(archive, 'w', ZIP_DEFLATED) as bundle:
        for path in sorted(destination.rglob('*')):
            if path.is_file():
                bundle.write(path, path.relative_to(destination))
    print(f'Packaged {archive} ({archive.stat().st_size:,} bytes)')


if __name__ == '__main__':
    main()
