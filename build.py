"""Build the private visual probe. Does not install it into the game."""
from pathlib import Path
import importlib.util
import sys
root=Path.cwd()
import os, subprocess
root=Path(__file__).resolve().parent
source=root/'source'
out=root/'build';out.mkdir(parents=True,exist_ok=True)
pf=Path(os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)'))
vcpaths=list(pf.glob('Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/Hostx64/x64'))
vcpaths=[x for x in vcpaths if all((x/f).is_file() for f in ('cl.exe','lib.exe','link.exe'))]
if not vcpaths: raise SystemExit('Install Visual Studio Build Tools with x64 C++ tools.')
vc=max(vcpaths,key=lambda x:tuple(int(v) for v in x.parents[2].name.split('.')))
class Builder:
    @staticmethod
    def run(args): subprocess.run([str(x) for x in args],check=True,cwd=out)
build=Builder()
kernel=out/'kernel32.lib'
ntdll=out/'ntdll.lib'
for name in ('kernel32','ntdll'):
    build.run([vc/'lib.exe','/nologo','/def:'+str(source/(name+'.def')),'/machine:x64','/out:'+str(out/(name+'.lib'))])
test='--test' in sys.argv
trail=True
ui='--ui' in sys.argv or trail
persistence='--persistence' in sys.argv or ui
target=out/('SpellbladeTrailTest.dll' if test and trail else 'ElementalEnchantmentTest.dll' if trail else 'SpellbladeUITest.dll' if test and ui else 'SpellbladeUICandidate.dll' if ui else 'SpellbladePersistenceTest.dll' if test and persistence else 'SpellbladePersistenceCandidate.dll' if persistence else 'SpellbladeFireProbeTest.dll' if test else 'SpellbladeFireProbe.dll')
flags=(['/DSPELLBLADE_NATIVE_TEST'] if test else [])+(['/DSB_ENABLE_PERSISTENCE'] if persistence else [])+(['/DSB_ENABLE_UI'] if ui else [])+(['/DSB_ENABLE_TRAIL'] if trail else [])
extra=['spellblade_affinity.c','spellblade_save.c','spellblade_gamedata.c','spellblade_characters.c','spellblade_persistence.c'] if persistence else []
if ui: extra+=['spellblade_localization.c','spellblade_ui_model.c','spellblade_ui_view.c','spellblade_radar.c','spellblade_mygui.c']
build.run([str(vc/'cl.exe'),'/nologo','/LD','/O2','/GS-','/Oi','/Zl',*flags,'/Fo'+str(out)+'/',
           str(source/'blade_fire_probe.c'),*(str(source/f) for f in extra),'/link','/NODEFAULTLIB','/ENTRY:DllMain',
           '/MACHINE:X64','/DYNAMICBASE','/NXCOMPAT','/DEF:'+str(source/'hit_observer.def'),
           '/IMPLIB:'+str(out/'probe.lib'),'/OUT:'+str(target),str(kernel),str(ntdll)])
print('Built private visual probe:',target)
