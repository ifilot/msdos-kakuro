#!/usr/bin/env python3
"""Extract the current VERSION's changelog entry for a GitHub release."""
import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent


def release_notes(changelog, version):
    sections = list(re.finditer(r'^## \[([^\]\n]+)\][^\n]*$', changelog, re.MULTILINE))
    matches = [index for index, section in enumerate(sections) if section.group(1) == version]
    if len(matches) != 1:
        raise ValueError(f'CHANGELOG.md must contain exactly one ## [{version}] entry')
    index = matches[0]
    end = sections[index+1].start() if index+1 < len(sections) else len(changelog)
    body = changelog[sections[index].end():end].strip()
    if not body:
        raise ValueError(f'CHANGELOG.md entry [{version}] is empty')
    return f'# Kakuro {version}\n\n{body}\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    version = (ROOT / 'VERSION').read_text().strip()
    notes = release_notes((ROOT / 'CHANGELOG.md').read_text(), version)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(notes)
    print(f'Release notes for {version}: {args.output}')


if __name__ == '__main__':
    main()
