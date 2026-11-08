"""Check that the normal plugin exports only its expected public entry points."""

import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DLL = ROOT / 'build' / 'ElementalEnchantmentTest.dll'
EXPECTED = {'?startPlugin@@YAXXZ', 'startPlugin', 'MCM_Describe'}


def main():
    if not DLL.is_file():
        raise SystemExit('Build the normal plugin first: python build.py')
    program_files = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)'))
    tools = list(program_files.glob(
        'Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe'
    ))
    if not tools:
        raise SystemExit('dumpbin.exe not found; install Visual Studio x64 C++ tools')
    dumpbin = max(tools, key=lambda p: tuple(int(part) for part in p.parents[3].name.split('.')))
    result = subprocess.run([str(dumpbin), '/exports', str(DLL)], capture_output=True,
                            text=True, errors='replace', check=True)
    exports = set(re.findall(r'^\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)',
                             result.stdout, re.MULTILINE))
    if exports != EXPECTED:
        raise SystemExit(f'Unexpected exports: {sorted(exports)}; expected {sorted(EXPECTED)}')
    print('Normal plugin exports only the expected entry points')


if __name__ == '__main__':
    main()
