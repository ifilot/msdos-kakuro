#!/usr/bin/env python3
"""Capture release metadata on the host before invoking the DOS compiler."""
from datetime import datetime, timezone
from pathlib import Path
import json
import os
import re
import subprocess
ROOT = Path(__file__).resolve().parent.parent


def main():
    version = (ROOT/'VERSION').read_text().strip()
    if not re.fullmatch(r'\d+\.\d+\.\d+(?:[-+][A-Za-z0-9.-]+)?', version) or len(version)>30:
        raise ValueError('VERSION must contain a short semantic version')
    commit = os.environ.get('BUILD_COMMIT', '')
    if not commit:
        try:
            result = subprocess.run(['git','rev-parse','--short=12','HEAD'],cwd=ROOT,
                                    capture_output=True,text=True,timeout=5)
            if result.returncode == 0: commit = result.stdout.strip()
        except (FileNotFoundError, subprocess.TimeoutExpired):
            pass
    if commit and not re.fullmatch(r'[0-9a-fA-F]{7,40}', commit):
        raise ValueError('BUILD_COMMIT must be a Git hexadecimal commit ID')
    epoch = os.environ.get('SOURCE_DATE_EPOCH')
    stamp = datetime.fromtimestamp(int(epoch),timezone.utc) if epoch else datetime.now(timezone.utc)
    values = {'KAKURO_VERSION':version, 'KAKURO_AUTHOR':'Ivo Filot',
              'KAKURO_COMPILER':'Borland C++ 3.1',
              'KAKURO_REPOSITORY':'github.com/ifilot/msdos-kakuro',
              'KAKURO_BUILD_DATE':stamp.strftime('%Y-%m-%d %H:%M UTC'),
              'KAKURO_COMMIT':commit[:12] if commit else 'unavailable'}
    text = '// Generated at build time by tools/generate_buildinfo.py.\n'
    text += ''.join(f'#define {key} {json.dumps(value)}\n' for key,value in values.items())
    (ROOT/'src/BUILDINF.H').write_text(text)


if __name__ == '__main__':
    main()
