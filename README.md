# Elemental Enchantment

A work-in-progress Kenshi mod that gives player characters elemental enchantments for melee weapons and unarmed combat. Fire, lightning, wind, water and ice have distinct effects and combat bonuses. Characters share one Elemental level, have individual aptitudes and unlock a secondary element at level 50.

**Status:** beta / work in progress. The latest unarmed classification fix and particle orientation changes still need verification in actual gameplay. The checked-in package currently uses its internal test name; do not install it alongside another copy.

## Requirements

- Kenshi and [RE_Kenshi](https://github.com/BFrizzleFoShizzle/RE_Kenshi), including KenshiLib.
- Arcane Magic and a dual-wielding mod are not required.

The interface is currently in English. Effects apply to player-controlled melee and unarmed combat.

## Repository

- `mod/Elemental Enchantment Test/`: current test package, including its plugin DLL.
- `source/`: plugin source, particle code helpers and standalone tests.
- `docs/`: draft Workshop description and change notes.

The `mod` folder matches the local test build at the time of the commit. It is not a finished Workshop release. The plugin uses game and KenshiLib APIs, so compatibility with other versions has not been established.

## Build

On Windows, install Python 3 and Visual Studio Build Tools with x64 C++ tools, then run `python build.py`. The DLL is written to `build/ElementalEnchantmentTest.dll`. Build output is not installed automatically.

## 日本語

近接武器や素手に５属性のエンチャントを付与するKenshi用Modです。キャラクターごとの適性と共通のElementalレベルがあり、レベル50で副属性が解放されます。

開発中のテスト版です。動作にはRE_KenshiとKenshiLibが必要です。Arcane Magicは必要ありません。素手の修正を含め、ゲーム内での最終確認を進めています。
