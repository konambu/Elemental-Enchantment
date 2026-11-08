"""Build the local Elemental Enchantment plugin candidate.

Default: normal game plugin written to build/ElementalEnchantmentTest.dll.
--test: include native test hooks and use a separate output DLL.
--frost-dev and --ice-experimental: build isolated development variants.
This script never copies a DLL into the game or uploads to Steam Workshop.
"""
import argparse
import os
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--test', action='store_true', help='build the separate native test DLL')
variants = parser.add_mutually_exclusive_group()
variants.add_argument('--frost-dev', action='store_true', help='build the frost development variant')
variants.add_argument('--ice-experimental', action='store_true', help='build the experimental ice variant')
args = parser.parse_args()

root = Path(__file__).resolve().parent
source = root / 'source'
out = root / 'build'
out.mkdir(parents=True, exist_ok=True)
program_files = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)'))
vcpaths = list(program_files.glob('Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/Hostx64/x64'))
vcpaths = [path for path in vcpaths if all((path / tool).is_file() for tool in ('cl.exe', 'lib.exe', 'link.exe'))]
if not vcpaths:
    raise SystemExit('Install Visual Studio Build Tools with x64 C++ tools.')
vc = max(vcpaths, key=lambda path: tuple(int(v) for v in path.parents[2].name.split('.')))

def run_build(args):
    """Run a build tool from the output directory and stop on failure."""
    subprocess.run([str(arg) for arg in args], check=True, cwd=out)

kernel = out / 'kernel32.lib'
ntdll = out / 'ntdll.lib'
for name in ('kernel32', 'ntdll'):
    run_build([vc / 'lib.exe', '/nologo', '/def:' + str(source / (name + '.def')),
               '/machine:x64', '/out:' + str(out / (name + '.lib'))])

test = args.test
target = out / ('ElementalEnchantmentTests.dll' if test else 'ElementalEnchantmentTest.dll')
flags = ['/DEE_ENABLE_PERSISTENCE', '/DEE_ENABLE_UI', '/DEE_ENABLE_TRAIL']
if test:
    flags += ['/DELEMENTAL_NATIVE_TEST']
if args.frost_dev:
    target = out / ('FrostReloadDevTest.dll' if test else 'FrostReloadDev.dll')
    flags += ['/DEE_FROST_RELOAD_DEV']
if args.ice_experimental:
    target = out / ('IceSceneNodeTest.dll' if test else 'IceSceneNodeTrial.dll')
    flags += ['/DEE_ICE_OVERLAY_EXPERIMENTAL']

extra = [
    'elemental_affinity.c', 'elemental_save.c', 'elemental_gamedata.c', 'elemental_game_string.c',
    'elemental_characters.c', 'elemental_persistence.c', 'elemental_boss_profiles.c',
    'elemental_effect_templates.c', 'elemental_localization.c', 'elemental_ui_model.c',
    'elemental_ui_view.c', 'elemental_radar.c', 'elemental_mygui.c',
]
if test:
    extra += [
        'elemental_ui_model_test.c', 'elemental_ui_view_test.c', 'elemental_characters_test.c',
        'elemental_game_string_test.c', 'elemental_persistence_test.c',
        'elemental_affinity_test.c', 'elemental_effect_templates_test.c',
    ]

run_build([
    vc / 'cl.exe', '/nologo', '/LD', '/O2', '/GS-', '/Oi', '/Zl', *flags,
    '/Fo' + str(out) + '/', source / 'elemental_enchantment_runtime.c',
    *(source / filename for filename in extra), '/link', '/NODEFAULTLIB', '/ENTRY:DllMain',
    '/MACHINE:X64', '/DYNAMICBASE', '/NXCOMPAT', '/DEF:' + str(source / 'hit_observer.def'),
    '/IMPLIB:' + str(out / 'probe.lib'), '/OUT:' + str(target), kernel, ntdll,
])
print('Built local plugin candidate:', target)
