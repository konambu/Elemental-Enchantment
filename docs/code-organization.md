# Code organization

## Runtime

`source/elemental_enchantment_runtime.c` owns the Kenshi and Ogre hooks, shared runtime state, and plugin startup. It includes the private runtime fragments for trails, impacts, crossbow visuals, and UI because those features still share engine pointers and state with the hooks.

The runtime's tracked actors include player characters and supported bosses. Weapon visuals, hand effects, and tracked actors are separate caches with their own cleanup paths.

Native diagnostic hooks are kept in `source/elemental_runtime_tests.inl`. They share internal state with the runtime and compile only when `ELEMENTAL_NATIVE_TEST` is enabled; they are excluded from the normal game build.

The standalone `elemental_*.c` modules contain affinity rules, character state, persistence, GameData access, localization, UI model/view logic, boss profiles, and the element-to-particle resource table. Add future element visual resources to `elemental_effect_templates.c`; it maps the same five elements across hand, blade, impact, and crossbow effects.

`elemental_affinity.c` owns the shared XP thresholds, level calculation, secondary-element strength, and aptitude ring order. UI and radar code consume those definitions so their display stays aligned with the gameplay rules.

`EEAffinity` stores one shared XP value in memory. The version 3 decoder still reads all five historical XP values and migrates the highest; version 4's three-integer save format is unchanged.

`elemental_game_string.c` owns the shared Kenshi/MyGUI string layout and initialization. It copies short strings into inline storage and borrows longer strings; callers must keep the longer text alive while the game uses it.

Keep engine-specific hook installation in the runtime coordinator unless a module boundary can be given a small explicit interface.

## Stable game data names

The following strings are part of the existing game data or visual resources. Renaming them would stop the mod from finding existing character data or particle templates:

- `SB_Affinity` and `SB_Data`: character GameData list and entry used to save affinity and progress.
- `SB_Prototype_*`: particle templates supplied by the mod's package.

These identifiers intentionally retain their historical prefix. C filenames and internal C types/functions use the `elemental_` / `EE` naming instead.

## Build

Run `python build.py` to build the local plugin candidate. Native test helpers are enabled only with `python build.py --test`.
