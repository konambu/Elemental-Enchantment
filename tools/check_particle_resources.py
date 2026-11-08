"""Check runtime particle descriptors against the packaged particle scripts."""

import ctypes
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DLL = ROOT / "build" / "ElementalEnchantmentTests.dll"
SCRIPTS = ROOT / "mod" / "Elemental Enchantment" / "particles" / "scripts"


class EffectTemplate(ctypes.Structure):
    _fields_ = [("resource", ctypes.c_char_p), ("size", ctypes.c_uint), ("capacity", ctypes.c_uint)]


def main():
    if not DLL.is_file():
        raise SystemExit("Build the test DLL first: python build.py --test")
    if not SCRIPTS.is_dir():
        raise SystemExit(f"Particle scripts are missing: {SCRIPTS}")
    systems = set()
    for path in SCRIPTS.glob("*.pu"):
        text = path.read_text(encoding="utf-8")
        systems.update(re.findall(r"^\s*system\s+([^\s{}]+)", text, re.MULTILINE))
    dll = ctypes.CDLL(str(DLL))
    lookup = dll.ee_test_effect_template
    lookup.argtypes = [ctypes.c_int, ctypes.c_int]
    lookup.restype = ctypes.POINTER(EffectTemplate)
    errors = []
    for kind in range(4):
        for element in range(1, 6):
            descriptor = lookup(kind, element)
            if not descriptor:
                errors.append(f"Missing descriptor for kind={kind}, element={element}")
                continue
            item = descriptor.contents
            if not item.resource:
                errors.append(f"Missing name for kind={kind}, element={element}")
                continue
            name = item.resource.decode("ascii")
            length = len(item.resource)
            expected_capacity = 15 if length < 16 else length
            if item.size != length or item.capacity != expected_capacity:
                errors.append(f"Incorrect string layout for {name}")
            if name not in systems:
                errors.append(f"Missing particle system: {name}")
    for error in errors:
        print(error)
    if errors:
        return 1
    print("All 20 particle descriptors match packaged systems")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
