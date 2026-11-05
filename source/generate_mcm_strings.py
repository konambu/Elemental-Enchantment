"""Generate an ASCII C header from the UTF-8 MCM translations."""
import json
from pathlib import Path

directory = Path(__file__).resolve().parent
languages = json.loads((directory / 'mcm-translations.json').read_text(encoding='utf-8'))
assert len(languages) == 9 and all(len(values) == 9 for values in languages.values())

def c_string(text):
    return '"' + '""'.join(f'\\x{byte:02x}' for byte in text.encode('utf-8')) + '"'

lines = [
    '#ifndef SB_MCM_STRINGS_H',
    '#define SB_MCM_STRINGS_H',
    'typedef struct SBMcmText {const char* id;const char* section;const char* strength;const char* strength_tip;const char* trails;const char* trails_tip;const char* impact;const char* impact_tip;const char* xp;const char* xp_tip;} SBMcmText;',
    'static const SBMcmText sb_mcm_texts[]={',
]
for locale, strings in languages.items():
    lines.append('{' + ','.join(c_string(s) for s in [locale, *strings]) + '},')
lines.extend([
    '};',
    'static const SBMcmText* sb_mcm_text(const char* id){unsigned i;for(i=0;i<sizeof(sb_mcm_texts)/sizeof(sb_mcm_texts[0]);++i){const char* a=id;const char* b=sb_mcm_texts[i].id;while(a&&*a&&*a==*b){++a;++b;}if(a&&!*a&&!*b)return &sb_mcm_texts[i];}return &sb_mcm_texts[0];}',
    '#endif',
])
(directory / 'spellblade_mcm_strings.h').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')
