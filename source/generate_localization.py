"""Generate the ASCII C localization table from the tracked UTF-8 inputs."""

import argparse
import json
from pathlib import Path

DIRECTORY = Path(__file__).resolve().parent
FIELDS = (
    "main", "sub", "locked", "opener", "elemental", "level", "master", "next",
    "trail", "glow", "dual", "second", "nextlevel", "maxlevel", "readonly",
    "unavailable", "disabled", "noelement", "choose", "base", "winddetail",
    "bleed", "bolt", "attackblock", "mcm_section", "mcm_label", "mcm_tip",
)
GAME_FIELDS = ("armour", "blunt", "cut")


def c_string(text):
    if not isinstance(text, str) or "\0" in text:
        raise ValueError("Localization entries must be strings without NUL characters")
    return '"' + '""'.join(f'\\x{byte:02x}' for byte in text.encode('utf-8')) + '"'


def generate():
    translations = json.loads((DIRECTORY / 'translations.json').read_text(encoding='utf-8'))
    game_terms = json.loads((DIRECTORY / 'game-terms.json').read_text(encoding='utf-8'))
    if not translations or next(iter(translations)) != 'en_GB' or translations.keys() != game_terms.keys():
        raise ValueError('English must be first and both input files must list the same locales')
    lines = ['#include "elemental_localization.h"',
             '/* Generated from translations.json and game-provided gettext catalogs. */',
             'static const EELocale table[] = {']
    for locale, values in translations.items():
        if set(values) != {'element', *FIELDS} or len(values['element']) != 6:
            raise ValueError(f'Invalid UI fields for {locale}')
        if set(game_terms[locale]) != set(GAME_FIELDS):
            raise ValueError(f'Invalid game terms for {locale}')
        elements = ','.join(c_string(value) for value in values['element'])
        fields = [values[key] for key in FIELDS] + [game_terms[locale][key] for key in GAME_FIELDS]
        lines.append('{' + c_string(locale) + ',{' + elements + '},' +
                     ','.join(c_string(value) for value in fields) + '},')
    lines.extend([
        '};',
        'static unsigned current;',
        'static int eq(const char* a,const char* b){if(!a||!b)return 0;while(*a&&*a==*b){++a;++b;}return *a==*b;}',
        'void ee_locale_set(const char* id){unsigned i;current=0;for(i=0;i<sizeof(table)/sizeof(table[0]);++i)if(eq(id,table[i].id)){current=i;return;}}',
        'const EELocale* ee_locale_get(void){return &table[current];}',
    ])
    return ('\n'.join(lines) + '\n').encode('ascii')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='check without changing the C file')
    args = parser.parse_args()
    target = DIRECTORY / 'elemental_localization.c'
    generated = generate()
    if args.check:
        if target.read_bytes().replace(b'\r\n', b'\n') != generated:
            raise SystemExit('UI localization is out of date; run source/generate_localization.py')
        print('UI localization matches its tracked inputs')
    else:
        target.write_bytes(generated)


if __name__ == '__main__':
    main()
