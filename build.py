"""Build the local Elemental Enchantment plugin candidate."""
from pathlib import Path
import os, subprocess
import sys
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
target=out/('ElementalEnchantmentTests.dll' if test else 'ElementalEnchantmentTest.dll')
flags=['/DEE_ENABLE_PERSISTENCE','/DEE_ENABLE_UI','/DEE_ENABLE_TRAIL']
if test: flags+=['/DELEMENTAL_NATIVE_TEST']
if '--frost-dev' in sys.argv:
    target=out/('FrostReloadDevTest.dll' if test else 'FrostReloadDev.dll')
    flags+=['/DEE_FROST_RELOAD_DEV']
if '--ice-experimental' in sys.argv:
    target=out/('IceSceneNodeTest.dll' if test else 'IceSceneNodeTrial.dll')
    flags+=['/DEE_ICE_OVERLAY_EXPERIMENTAL']
extra=['elemental_affinity.c','elemental_save.c','elemental_gamedata.c','elemental_characters.c','elemental_persistence.c','elemental_boss_profiles.c',
       'elemental_localization.c','elemental_ui_model.c','elemental_ui_view.c','elemental_radar.c','elemental_mygui.c']
if test: extra+=['elemental_ui_model_test.c','elemental_ui_view_test.c','elemental_characters_test.c']
build.run([str(vc/'cl.exe'),'/nologo','/LD','/O2','/GS-','/Oi','/Zl',*flags,'/Fo'+str(out)+'/',
           str(source/'elemental_enchantment_runtime.c'),*(str(source/f) for f in extra),'/link','/NODEFAULTLIB','/ENTRY:DllMain',
           '/MACHINE:X64','/DYNAMICBASE','/NXCOMPAT','/DEF:'+str(source/'hit_observer.def'),
           '/IMPLIB:'+str(out/'probe.lib'),'/OUT:'+str(target),str(kernel),str(ntdll)])
print('Built local plugin candidate:',target)
