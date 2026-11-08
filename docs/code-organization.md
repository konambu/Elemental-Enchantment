# Code organization

## Runtime

`source/elemental_enchantment_runtime.c` owns the Kenshi and Ogre hooks, shared runtime state, and plugin startup. It includes the private runtime fragments for trails, impacts, crossbow visuals, and UI because those features still share engine pointers and state with the hooks.

The standalone `elemental_*.c` modules contain affinity rules, character state, persistence, GameData access, localization, UI model/view logic, boss profiles, and the element-to-particle resource table. Add future element visual resources to `elemental_effect_templates.c`; it maps the same five elements across hand, blade, impact, and crossbow effects.

Keep engine-specific hook installation in the runtime coordinator unless a module boundary can be given a small explicit interface.

## Stable game data names

The following strings are part of the existing game data or visual resources. Renaming them would stop the mod from finding existing character data or particle templates:

- `SB_Affinity` and `SB_Data`: character GameData list and entry used to save affinity and progress.
- `SB_Prototype_*`: particle templates supplied by the mod's package.

These identifiers intentionally retain their historical prefix. C filenames and internal C types/functions use the `elemental_` / `EE` naming instead.

## Build

Run `python build.py` to build the local plugin candidate. Native test helpers are enabled only with `python build.py --test`.
