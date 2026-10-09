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
    for name, path in (('PROFONT.TXT', 'assets/fonts/PROFONT-LICENSE.txt'),
                       ('FONTS.TXT', 'assets/fonts/internet/README.md')):
        shutil.copyfile(ROOT / path, destination / name)
    version = (ROOT / 'VERSION').read_text().strip()
    (destination / 'README.TXT').write_bytes(
        ('Version ' + version + '\r\n').encode('ascii') +
        b'KAKURO - 8086/8088 DOS, VGA 640x480, 16 colors\r\n'
        b'Source: https://github.com/ifilot/msdos-kakuro\r\n'
        b'Run KAKURO from this directory. Keep KAKURO.DAT and PUZZLES.DAT beside it.\r\n'
        b'Enter opens the journal. Arrows select, PgUp/PgDn change pages.\r\n'
        b'F1 opens Help; F2 opens About. Arrows/PgUp/PgDn scroll.\r\n'
        b'F3 opens sound settings in the journal or puzzle.\r\n'
        b'Keep SOUND.DAT beside KAKURO.EXE. Sound settings save to SOUND.CFG.\r\n'
        b'AdLib / Sound Blaster FM / MPU-401 General MIDI; no PC speaker.\r\n'
        b'MPU-401 requires a connected General MIDI synthesizer.\r\n'
        b'Enter plays; 1-9 enter digits, Backspace/Delete/0 clear.\r\n'
        b'Esc returns to the journal; Esc there exits.\r\n'
        b'Entries and status marks last for this session only.\r\n')
    archive = ROOT / 'build/KAKURO.ZIP'
    with ZipFile(archive, 'w', ZIP_DEFLATED) as bundle:
        for path in sorted(destination.rglob('*')):
            if path.is_file():
                bundle.write(path, path.relative_to(destination))
    print(f'Packaged {archive} ({archive.stat().st_size:,} bytes)')


if __name__ == '__main__':
    main()
