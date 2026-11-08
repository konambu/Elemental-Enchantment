"""Check the editable UI and MCM translations against the compiled tables."""

import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "source"


def decode_literals(row):
    groups = re.findall(r'(?:(?:"\\x[0-9a-f]{2}")+)', row)
    return [
        bytes.fromhex(" ".join(re.findall(r"\\x([0-9a-f]{2})", group))).decode("utf-8")
        for group in groups
    ]


def main():
    translations = json.loads((SOURCE / "translations.json").read_text(encoding="utf-8"))
    code = (SOURCE / "elemental_localization.c").read_text(encoding="ascii")
    rows = re.findall(r'^\{"\\x.*$', code, re.MULTILINE)
    decoded = [decode_literals(row) for row in rows]
    if len(decoded) != len(translations) or {row[0] for row in decoded} != set(translations):
        raise SystemExit("UI localization language list does not match translations.json")
    for row in decoded:
        locale, *actual = row
        source = translations[locale]
        expected = [*source["element"], *(v for k, v in source.items() if k != "element")]
        # The final three UI terms come from the game's own translation catalogs.
        if len(row) != len(expected) + 4 or actual[:len(expected)] != expected:
            raise SystemExit(f"UI localization does not match translations.json: {locale}")

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
