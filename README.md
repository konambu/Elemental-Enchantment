# Elemental Enchantment

A work-in-progress Kenshi mod that gives player characters elemental enchantments for melee weapons, unarmed combat and crossbows. Fire, lightning, wind, water and ice have distinct effects and combat bonuses. Characters share one Elemental level, have individual aptitudes and unlock a secondary element at level 50.

**Status:** beta. Published on [Steam Workshop](https://steamcommunity.com/sharedfiles/filedetails/?id=3832978749). Do not enable this package alongside another copy of the mod.

## Requirements

- Kenshi and [RE_Kenshi](https://github.com/BFrizzleFoShizzle/RE_Kenshi).
- Arcane Magic and a dual-wielding mod are not required.

The interface supports Kenshi's nine official languages. Player-controlled characters and selected named bosses can use enchantments. Boss combat effects are experimental.

Crossbow support is experimental: loading and flying bolts have elemental visuals, while damage calculations, armour penetration, bleeding, bolt speed and experience need further testing and may not work correctly. The crossbow projectile visual and bolt-speed hooks are enabled only for the verified RE_Kenshi executable build; other builds leave them disabled.

Wind also increases crossbow reload speed by 2–10%, depending on Elemental level. If using Nitoryu, load Elemental Enchantment above Nitoryu so its speed bonus is calculated before Nitoryu's speed limits.

## Optional MCM setting

With Mod Config Menu installed, open Options > MCM > Elemental Enchantment to enable **All elemental aptitudes 100%**. This removes elemental experience penalties for all player characters. The option is off by default; turning it off restores each character's original aptitude.

## Repository

- `mod/Elemental Enchantment/`: Workshop package, including its plugin DLL.
- `source/`: plugin source and the tests included in the local build.
- `docs/`: code map, release instructions, and Workshop text drafts.

The `mod` folder matches the uploaded Workshop files. The plugin uses game and KenshiLib APIs, so compatibility with other versions has not been established.

## Build

On Windows, install Python 3 and Visual Studio Build Tools with x64 C++ tools, then run `python build.py`. The DLL is written to `build/ElementalEnchantmentTest.dll`. Build output is not installed automatically. See [the release checklist](docs/releasing.md) for copying it into Kenshi and updating Steam Workshop.

The module roles and stable save/resource names are listed in [Code organization](docs/code-organization.md).
Public C interfaces use Doxygen-style comments. If Doxygen is installed, run `doxygen Doxyfile` to generate browsable API docs under `docs/api/`.
Use the checked-in `.clang-format` when editing C files; run `clang-format -i <files>` to keep them readable and consistent.

## 日本語

近接武器・素手・クロスボウに５属性のエンチャントを付与するKenshi用Modです。キャラクターごとの適性と共通のElementalレベルがあり、レベル50で副属性が解放されます。一部のボスも属性を使用します。ボスとクロスボウの戦闘効果は検証中で、正しく動かない場合があります。

ベータ版です。動作にはRE_Kenshiが必要です。二刀流には別のModが必要です。

風属性はクロスボウの装填速度もレベルに応じて2〜10％上げます。Nitoryuと併用する場合は、Elemental EnchantmentをNitoryuより上に配置してください。
