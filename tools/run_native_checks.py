"""Run engine-independent native checks in separate processes on Windows."""

import ctypes
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DLL = ROOT / "build" / "ElementalEnchantmentTests.dll"
CHECKS = (
    "ee_run_game_string_tests",
    "ee_run_persistence_tests",
    "ee_run_gamedata_tests",
    "ee_run_affinity_threshold_tests",
    "ee_run_affinity_migration_tests",
    "ee_run_characters_tests",
    "ee_test_new_aptitudes",
    "ee_run_ui_model_tests",
    "ee_run_ui_view_tests",
    "ee_run_ui_resize_tests",
    "ee_run_radar_tests",
    "probeTestHalfEffectDisplay",
    "probeTestTemplateStrings",
    "probeTestXPScaling",
    "probeTestEffectMultiplier",
    "probeTestXPScalingHitHooks",
    "probeTestMultiplierHitHooks",
    "probeTestCrossbowWound",
    "probeTestCrossbowCapture",
    "probeTestCrossbowCoordinates",
    "probeTestImpactPositions",
    "probeTestImpactQueue",
    "probeTestImpactSetting",
    "probeCheckVisualStages",
    "probeCheckMeleeContexts",
    "probeTestMcmRegistration",
    "probeTestAllyHistoryFilter",
    "probeTestBossProfileLookup",
    "probeTestBossCombatAffinity",
    "probeTestHitXP",
    "probeTestHookTargets",
    "probeTestWindReload",
    "probeTestWindReloadIntegration",
    "probeTestCrossbowSpeed",
    "probeTestLocaleDetection",
    "probeTestWindSpeed",
    "probeTestPersistenceHooks",
    "probeTestIdentity",
    "probeTestAptitudeOverride",
    "probeTestNewGameAptitude",
    "probeTestAffinity",
    "probeTestUIHooks",
)


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--one":
        if sys.argv[2] not in CHECKS:
            raise SystemExit("Unknown check")
        dll = ctypes.CDLL(str(DLL))
        raise SystemExit(getattr(dll, sys.argv[2])())
    if not DLL.is_file():
        raise SystemExit("Build the test DLL first: python build.py --test")
    failed = []
    for name in CHECKS:
        try:
            result = subprocess.run(
                [sys.executable, str(Path(__file__).resolve()), "--one", name],
                capture_output=True,
                text=True,
                timeout=20,
            )
        except subprocess.TimeoutExpired:
            failed.append(name)
            print(f"FAIL {name}: timed out")
            continue
        if result.returncode:
            failed.append(name)
            print(f"FAIL {name}: exit {result.returncode}")
            if result.stderr:
                print(result.stderr.strip())
        else:
            print(f"PASS {name}")
    scripts = (("particle resources", "check_particle_resources.py"),
               ("localization", "check_localization.py"))
    for label, script in scripts:
        try:
            result = subprocess.run(
                [sys.executable, str(ROOT / "tools" / script)],
                capture_output=True,
                text=True,
                timeout=20,
            )
        except subprocess.TimeoutExpired:
            failed.append(label)
            print(f"FAIL {label}: timed out")
        else:
            if result.returncode:
                failed.append(label)
                print(f"FAIL {label}")
                print((result.stdout + result.stderr).strip())
            else:
                print(f"PASS {label}")
    total = len(CHECKS) + len(scripts)
    print(f"{total - len(failed)}/{total} checks passed")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
