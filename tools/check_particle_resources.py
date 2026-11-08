"""Check runtime particle descriptors against the packaged particle scripts."""

import argparse
import ctypes
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DLL = ROOT / "build" / "ElementalEnchantmentTests.dll"
PARTICLES = ROOT / "mod" / "Elemental Enchantment" / "particles"
SCRIPTS = PARTICLES / "scripts"
MATERIALS = PARTICLES / "materials"


class EffectTemplate(ctypes.Structure):
    _fields_ = [("resource", ctypes.c_char_p), ("size", ctypes.c_uint), ("capacity", ctypes.c_uint)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-data", type=Path, help="Optional Kenshi data directory")
    args = parser.parse_args()
    if not DLL.is_file():
        raise SystemExit("Build the test DLL first: python build.py --test")
    if not SCRIPTS.is_dir():
        raise SystemExit(f"Particle scripts are missing: {SCRIPTS}")
    if args.game_data and not args.game_data.is_dir():
        raise SystemExit(f"Kenshi data directory is missing: {args.game_data}")
    systems = set()
    material_refs = set()
    for path in SCRIPTS.glob("*.pu"):
        text = path.read_text(encoding="utf-8")
        systems.update(re.findall(r"^\s*system\s+([^\s{}]+)", text, re.MULTILINE))
        material_refs.update(re.findall(r"^\s*material\s+([^\s{}]+)", text, re.MULTILINE))
    local_materials = set()
    texture_refs = set()
    for path in MATERIALS.glob("*.material"):
        text = path.read_text(encoding="utf-8")
        local_materials.update(re.findall(r"^\s*material\s+([^\s{}:]+)", text, re.MULTILINE))
        texture_refs.update(re.findall(r"\btexture\s+([^\s{}]+)", text))
    local_files = {path.name.lower() for path in PARTICLES.rglob("*") if path.is_file()}
    game_materials = set()
    game_files = set()
    if args.game_data:
        for path in args.game_data.rglob("*"):
            if not path.is_file():
                continue
            game_files.add(path.name.lower())
            if path.suffix.lower() == ".material":
                text = path.read_text(encoding="utf-8", errors="replace")
                game_materials.update(
                    re.findall(r"^\s*material\s+([^\s{}:]+)", text, re.MULTILINE)
                )
    dll = ctypes.CDLL(str(DLL))
    lookup = dll.ee_test_effect_template
    lookup.argtypes = [ctypes.c_int, ctypes.c_int]
    lookup.restype = ctypes.POINTER(EffectTemplate)
    errors = []
    available_materials = local_materials | game_materials
    available_files = local_files | game_files
    for name in sorted(material_refs):
        should_check = args.game_data is not None or name.startswith(("EE_", "SB_"))
        if should_check and name not in available_materials:
            errors.append(f"Missing material: {name}")
    for name in sorted(texture_refs):
        should_check = args.game_data is not None or name.startswith(("EE_", "SB_"))
        if should_check and name.lower() not in available_files:
            errors.append(f"Missing texture: {name}")
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
    if args.game_data:
        print("All 20 descriptors and referenced materials/textures match the package or game data")
    else:
        print("All 20 descriptors and mod-owned material/texture references match the package")
        print("Built-in game materials and textures were not checked (supply --game-data to check them)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
