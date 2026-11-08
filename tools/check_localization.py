"""Check the editable UI and MCM translations against the compiled tables."""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "source"


def main():
    subprocess.run([sys.executable, str(SOURCE / "generate_localization.py"), "--check"], check=True)

    header = SOURCE / "elemental_mcm_strings.h"
    before = header.read_bytes()
    # Generate in a temporary copy so checking never modifies tracked files.
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        for name in ("generate_mcm_strings.py", "mcm-translations.json"):
            (directory / name).write_bytes((SOURCE / name).read_bytes())
        subprocess.run([sys.executable, str(directory / "generate_mcm_strings.py")], check=True)
        if (directory / header.name).read_bytes() != before.replace(b"\r\n", b"\n"):
            raise SystemExit("MCM header is out of date; run source/generate_mcm_strings.py")
    print("UI and MCM translations match their source data")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
