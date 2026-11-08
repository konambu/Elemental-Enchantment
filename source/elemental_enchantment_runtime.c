/** @file elemental_enchantment_runtime.c
 * @brief Kenshi plugin entry point and engine-facing runtime coordinator.
 * @details Owns hook registration and shared live state. Game-independent
 * rules and lookup tables live in the elemental_*.c modules; small .inl
 * fragments remain here because they use this translation unit's engine ABI.
 */
typedef unsigned long DWORD;
int _fltused = 0;
typedef unsigned long long U64;
typedef void *MODULE;
#include "elemental_game_string.h"
typedef EEGameString String;
typedef struct {
    float x, y, z;
} Vec;
typedef struct {
    float w, x, y, z;
} Quat;
static const String *element_template(String *storage, int kind, int element);
#ifdef EE_ENABLE_PERSISTENCE
#include "elemental_persistence.h"
#include "elemental_gamedata.h"
#endif
#include "elemental_boss_profiles.h"
#include "elemental_effect_templates.h"
#ifdef EE_ENABLE_UI
#include "elemental_mygui.h"
#include "elemental_localization.h"
#include "elemental_mcm_strings.h"
#endif
typedef struct {
    void *owner;
    void *blade;
    void *tag;
    void *scene;
    void *parent;
    void *node;
    void *effect;
    void *effect2;
    void *frost;
    void *frost_scene;
    void *frost_node;
    Quat frost_rotation;
    Vec frost_offset;
    int frost_bone, frost_attached;
    U64 born;
    float length;
    float applied_scale;
    int pole;
    int element;
    int secondary;
    int level;
#ifdef EE_ENABLE_TRAIL
    void *ribbon;
    void *ribbon_node;
    void *ribbon_root;
    Vec trail_direction;
    int trail_has_direction;
    float trail_idle;
    int trail_element, trail_linked, trail_attached;
#endif
} EEWeaponVisual;
__declspec(dllimport) MODULE __stdcall GetModuleHandleA(const char *);
__declspec(dllimport) void *__stdcall GetProcAddress(MODULE, const char *);
__declspec(dllimport) DWORD __stdcall GetModuleFileNameA(MODULE, char *, DWORD);
__declspec(dllimport) unsigned int __stdcall
GetPrivateProfileIntA(const char *, const char *, int, const char *);
__declspec(dllimport) DWORD __stdcall
GetPrivateProfileStringA(const char *, const char *, const char *, char *, DWORD, const char *);
__declspec(dllimport) DWORD __stdcall GetCurrentThreadId(void);
__declspec(dllimport) U64 __stdcall GetTickCount64(void);
__declspec(dllimport) int __stdcall QueryPerformanceCounter(U64 *);
__declspec(dllimport) void __stdcall AcquireSRWLockExclusive(void *);
__declspec(dllimport) void __stdcall ReleaseSRWLockExclusive(void *);
__declspec(dllimport) void *__stdcall
CreateFileA(const char *, DWORD, DWORD, void *, DWORD, DWORD, void *);
__declspec(dllimport) int __stdcall WriteFile(void *, const void *, DWORD, DWORD *, void *);
__declspec(dllimport) int __stdcall CloseHandle(void *);
#define EE_WEAPON_VISUAL_LIMIT 512
#define EE_TRACKED_ACTOR_LIMIT 512
static EEWeaponVisual weapon_visuals[EE_WEAPON_VISUAL_LIMIT];
typedef struct {
    void *character;
    void *body;
    void *left;
    void *right;
    int unarmed;
    int element;
    int attacking;
    int secondary;
    int level;
    int melee_weapon;
    float hit_glow;
} EETrackedActor;
static EETrackedActor tracked_actors[EE_TRACKED_ACTOR_LIMIT];
/* A conservative history filter: false positives cost a scan, false negatives
 * are impossible because bits are set before a character enters tracked_actors. */
static unsigned int tracked_actor_seen_bits[2048];
static unsigned int tracked_actor_seen_bit(void *character) {
    unsigned int x = (unsigned int)((U64)character >> 4);
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    return x & 65535u;
}
static int tracked_actor_seen_maybe(void *character) {
    unsigned int bit = tracked_actor_seen_bit(character);
    return (tracked_actor_seen_bits[bit >> 5] & (1u << (bit & 31))) != 0;
}
static void tracked_actor_seen_remember(void *character) {
    unsigned int bit = tracked_actor_seen_bit(character);
    tracked_actor_seen_bits[bit >> 5] |= 1u << (bit & 31);
}
typedef struct {
    void *character;
    void *body;
    void *bone;
    void *scene;
    void *parent;
    void *node;
    void *effect;
    void *effect2;
    float applied_scale;
    int element;
    int secondary;
} HandEffect;
static HandEffect hand_effects[EE_TRACKED_ACTOR_LIMIT * 2];
static void *guard;
static DWORD internal;
static int active, logged_full;
static U64 visual_serial;
static char log_path[1024];
static MODULE self;
static char settings_path[1024];
static int all_aptitudes;
static float effect_multiplier = 1.0f;
static unsigned int xp_tenths = 10, ui_scale_tenths = 10;
static int show_trails = 1, show_impacts = 1;
#ifdef EE_ENABLE_PERSISTENCE
/* Called only when an unsaved player record is first created, under guard. */
static unsigned int fresh_aptitude_seed(unsigned int seed) {
    static unsigned int sequence;
    U64 ticks = 0;
    QueryPerformanceCounter(&ticks);
    ++sequence;
    return seed ^ (unsigned int)ticks ^ (unsigned int)(ticks >> 32) ^
           (unsigned int)GetTickCount64() ^ (sequence * 0x9e3779b9u);
}
#endif
#ifdef EE_ENABLE_UI
static void select_game_locale(void);
#endif
typedef struct MCM_Api MCM_Api;
struct MCM_Api {
    int version;
    void *impl;
    void(__cdecl *beginMod)(
        MCM_Api *, const char *, const char *, const char *, void(__cdecl *)(void));
    void(__cdecl *section)(MCM_Api *, const char *);
    void(__cdecl *toggle)(
        MCM_Api *, const char *, const char *, const char *, const char *, int, unsigned);
    void(__cdecl *integer)(
        MCM_Api *, const char *, const char *, const char *, const char *, int, int, int, unsigned);
    void(__cdecl *number)(MCM_Api *,
                          const char *,
                          const char *,
                          const char *,
                          const char *,
                          float,
                          float,
                          float,
                          int,
                          unsigned);
    void(__cdecl *choice)(MCM_Api *,
                          const char *,
                          const char *,
                          const char *,
                          const char *,
                          const char *,
                          const char *const *,
                          const char *const *,
                          int,
                          unsigned);
};
static unsigned int parse_multiplier_tenths(const char *s, unsigned int maximum) {
    unsigned int whole = 0, decimal = 0, value;
    int digits = 0;
    if (!s)
        return 10;
    while (*s == ' ' || *s == '\t')
        ++s;
    while (*s >= '0' && *s <= '9') {
        if (whole > 1000)
            return maximum;
        whole = whole * 10u + (unsigned int)(*s++ - '0');
        digits = 1;
    }
    if (!digits)
        return 10;
    if (*s == '.') {
        ++s;
        if (*s >= '0' && *s <= '9')
            decimal = (unsigned int)(*s++ - '0');
        while (*s >= '0' && *s <= '9')
            ++s;
    }
    while (*s == ' ' || *s == '\t')
        ++s;
    if (*s)
        return 10;
    value = whole * 10u + decimal;
    return value > maximum ? maximum : value;
}
/** @brief Reload the persisted MCM options before the next update. */
static void __cdecl reload_settings(void) {
    char m[16];
    if (settings_path[0])
        GetPrivateProfileStringA("General", "EffectMultiplier", "1", m, sizeof(m), settings_path);
    else {
        m[0] = '1';
        m[1] = 0;
    }
    all_aptitudes = settings_path[0] &&
                    GetPrivateProfileIntA("General", "AllAptitudes100", 0, settings_path) != 0;
    effect_multiplier = (float)parse_multiplier_tenths(m, 50u) / 10.0f;
    if (settings_path[0])
        GetPrivateProfileStringA(
            "General", "ExperienceMultiplier", "1", m, sizeof(m), settings_path);
    else {
        m[0] = '1';
        m[1] = 0;
    }
    xp_tenths = parse_multiplier_tenths(m, 200u);
    if (settings_path[0])
        GetPrivateProfileStringA("General", "UIScale", "1", m, sizeof(m), settings_path);
    else {
        m[0] = '1';
        m[1] = 0;
    }
    ui_scale_tenths = parse_multiplier_tenths(m, 20u);
    if (ui_scale_tenths < 8)
        ui_scale_tenths = 8;
    show_trails = !settings_path[0] ||
                  GetPrivateProfileIntA("General", "ShowWeaponTrails", 1, settings_path) != 0;
    show_impacts = !settings_path[0] ||
                   GetPrivateProfileIntA("General", "ShowImpactEffects", 1, settings_path) != 0;
#ifdef EE_ENABLE_UI
    ee_ui_view_set_effect_multiplier(effect_multiplier);
    ee_ui_view_set_scale(ui_scale_tenths);
#endif
}
/**
 * @brief Register this mod's options with Mod Config Menu.
 * @details The exported entry is optional; all option callbacks are guarded
 * against an absent or older MCM API.
 */
__declspec(dllexport) void __cdecl MCM_Describe(MCM_Api *api) {
    if (!api || api->version < 1 || !api->beginMod || !api->section || !api->toggle ||
        !settings_path[0])
        return;
#ifdef EE_ENABLE_UI
    select_game_locale();
#endif
    api->beginMod(
        api, "ElementalEnchantment", "Elemental Enchantment", settings_path, reload_settings);
#ifdef EE_ENABLE_UI
    {
        const EEMcmText *t = ee_mcm_text(ee_locale_get()->id);
        api->section(api, ee_locale_get()->mcm_section);
        api->toggle(api,
                    "General",
                    "AllAptitudes100",
                    ee_locale_get()->mcm_label,
                    ee_locale_get()->mcm_tip,
                    0,
                    0);
        if (api->number)
            api->number(
                api, "General", "ExperienceMultiplier", t->xp, t->xp_tip, 1.0f, 0.0f, 20.0f, 1, 0);
        api->section(api, t->section);
        if (api->number)
            api->number(api,
                        "General",
                        "EffectMultiplier",
                        t->strength,
                        t->strength_tip,
                        1.0f,
                        0.0f,
                        5.0f,
                        1,
                        0);
        api->toggle(api, "General", "ShowWeaponTrails", t->trails, t->trails_tip, 1, 0);
        api->toggle(api, "General", "ShowImpactEffects", t->impact, t->impact_tip, 1, 0);
        api->section(api, t->ui_section);
        if (api->number)
            api->number(
                api, "General", "UIScale", t->ui_scale, t->ui_scale_tip, 1.0f, 0.8f, 2.0f, 1, 0);
    }
#else
    api->section(api, "Aptitude");
    api->toggle(api,
                "General",
                "AllAptitudes100",
                "All elemental aptitudes 100%",
                "All characters gain elemental experience at 100% speed.",
                0,
                0);
    if (api->number)
        api->number(api,
                    "General",
                    "ExperienceMultiplier",
                    "Elemental XP multiplier",
                    "Adjust Elemental XP gain. Aptitudes still apply; 0 stops XP gain.",
                    1.0f,
                    0.0f,
                    20.0f,
                    1,
                    0);
    api->section(api, "Effects");
    if (api->number)
        api->number(api,
                    "General",
                    "EffectMultiplier",
                    "Effect strength (x)",
                    "0: visuals only; 1: normal; 5: five times.",
                    1.0f,
                    0.0f,
                    5.0f,
                    1,
                    0);
    api->toggle(api,
                "General",
                "ShowWeaponTrails",
                "Weapon trails",
                "Show weapon trails while attacking.",
                1,
                0);
    api->toggle(api,
                "General",
                "ShowImpactEffects",
                "Impact effects",
                "Show elemental bursts on hits from level 40.",
                1,
                0);
    api->section(api, "Interface");
    if (api->number)
        api->number(api,
                    "General",
                    "UIScale",
                    "Enchantment panel scale",
                    "Change the size of the enchantment panel.",
                    1.0f,
                    0.8f,
                    2.0f,
                    1,
                    0);
#endif
}

static void clear_crossbow_scene(void *);
static void clear_crossbow_character(void *);
static void clear_crossbow_source(void *);
static void clear_crossbow_projectile(void *);
static void impact_clear_scene(void *);
static void impact_clear_parent(void *);
static void impact_queue_character(void *, int, int);
static int impact_queue_position(void *, const Vec *, int, int);
static int crossbow_impact_sample(void *, void **, Vec *);
static void *entity_vtable;
static void *(*entity_mesh)(void *);
static const String *(*resource_name)(void *);
static void *(*mesh_bounds)(void *);
static const Vec *(*bounds_maximum)(void *);
static void *(*parent_node)(void *);
static void *(*node_scene)(void *);
static const Vec *(*tag_position)(void *);
static const Quat *(*tag_rotation)(void *);
static void *(*particle_manager)(void);
static void *(*particle_template)(void *, const String *);
static void *(*particle_create)(void *, const String *, const String *, void *);
static void (*particle_destroy)(void *, void *, void *);
static void (*particle_start)(void *);
static void (*particle_update)(void *, float);
static void (*particle_local)(void *, unsigned char);
static U64 (*particle_count)(void *);
static void (*particle_bounds)(void *, unsigned char, float);
static void *(*particle_technique)(void *, U64);
static void *(*technique_emitter)(void *, U64);
static void (*box_height)(void *, float);
static void (*set_aabb)(void *, const float *);
static void (*set_queue)(void *, unsigned char);
static void *(*node_create)(void *, int);
static void (*node_destroy)(void *, void *);
static void *(*frost_entity_create)(void *, const void *, int);
static void (*frost_entity_material)(void *, const String *, const String *);
static unsigned char (*frost_get_visible)(void *);
static void (*frost_set_visible)(void *, unsigned char);
static void (*frost_cast_shadows)(void *, unsigned char);
static void (*node_add)(void *, void *);
static void (*node_position)(void *, const Vec *);
static void (*node_scale)(void *, const Vec *);
static void (*node_rotation)(void *, float, float, float, float);
static void (*node_attach)(void *, void *);
static void *(*original_attach)(void *, const void *, void *, const void *, const void *);
static void (*original_detach)(void *, void *);
static void (*original_detach_all)(void *);
static void (*original_clear)(void *);
static void (*original_destroy_entity)(void *, void *);
static void (*original_destroy_node)(void *, void *);
static void (*original_scene_detach)(void *, void *);
static void (*original_gpu)(void *, float);
static void (*original_free_tag)(void *, void *);
static void (*original_free_object)(void *, void *);
#ifdef EE_ENABLE_TRAIL
static void *(*trail_root)(void *, int);
static void *(*trail_create)(void *);
static void (*trail_destroy)(void *, void *);
static void (*trail_add)(void *, void *);
static void (*trail_remove)(void *, void *);
static void (*trail_colour_change)(void *, U64, float, float, float, float);
static void *(*trail_derive)(void *, Vec *);
static void *trail_get_colour;
static void *trail_get_width;
static void *trail_get_material;
static void *(*trail_character_combat)(void *);
static int (*trail_combat_state)(void *);
static void trail_cleanup(EEWeaponVisual *);
static void trail_tick(EEWeaponVisual *, const Vec *, const Quat *, float);
#endif
static void (*original_character_update)(void *);
static void (*original_character_destroy)(void *);
static unsigned char (*character_is_player)(void *);
static void *(*character_appearance)(void *);
static void *(*appearance_body)(void *);
static const void *(*character_handle)(void *);
static unsigned char (*character_combat)(void *, unsigned char, unsigned char);
static void *(*character_current_weapon)(void *);
static int (*weapon_category)(void *);
static void *(*appearance_attached)(void *, const String *);
static void *(*appearance_skeleton)(void *);
static void *(*skeleton_bone)(void *, const String *);
#ifdef EE_ENABLE_PERSISTENCE
static EEPersistence persistence;
static EEGameDataAPI gamedata_api;
static void *(*character_stats)(void *);
static void (*original_stats_load)(void *, void *);
static void (*original_stats_save)(void *, void *);
static void (*original_stats_destroy)(void *);
static void (*original_character_set_handle)(void *, const void *);
static int (*original_character_hit)(void *, int, void *, void *, void *, int);
static void *(*original_crossbow_wound)(
    void *, unsigned char, int, void *, int *, void *, int *, void *);
#ifdef ELEMENTAL_NATIVE_TEST
static float (*original_attack_speed)(void *);
static float (*original_block_speed)(void *);
#endif
static float (*original_calculate_combat_speed)(void *, float, float, unsigned char);
#endif
/* Caller holds guard. Entries contain identities only; no stored character is dereferenced. */
static EETrackedActor *find_tracked_actor_by_body(void *body) {
    int i;
    if (!body)
        return 0;
    for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
        if (tracked_actors[i].body == body && tracked_actors[i].character)
            return &tracked_actors[i];
    return 0;
}
#ifdef ELEMENTAL_NATIVE_TEST
static int is_tracked_actor_body(void *body) {
    EETrackedActor *a = find_tracked_actor_by_body(body);
    return a && a->element != 0;
}
#endif
/* Character::getCurrentWeapon is virtual. 0x3c0 is the slot documented by the
 * same KenshiLib headers used for the verified game build. */
static void *current_weapon_virtual(void *character) {
    void **table = character ? *(void ***)character : 0;
    return table && table[0x3c0 / sizeof(void *)]
               ? ((void *(*)(void *))table[0x3c0 / sizeof(void *)])(character)
               : 0;
}
/* 0: ranged/unknown, 1: empty hands, 2: drawn melee weapon. */
static int melee_context(void *character) {
    void *weapon;
    int category;
    if (!character || !character_current_weapon || !weapon_category || !character_combat)
        return 0;
    weapon = character_current_weapon(character);
    if (!weapon)
        return character_combat(character, 0, 1) ? 0 : 1;
    category = weapon_category(weapon);
    if (category == 5)
        return character_combat(character, 0, 1) ? 0 : 1;
    return (category >= 0 && category <= 4) || category == 8 ? 2 : 0;
}
static unsigned int affinity_seed(unsigned int index, unsigned int serial_number) {
    unsigned int x = index ^ (serial_number * 0x9e3779b9u);
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    if ((x >> 1) % 5u == 0)
        return 4u;
    if ((x >> 1) % 5u == 1)
        return 5u;
    return ((x >> 1) % 3u == 0) ? 3u : ((x & 1u) ? 2u : 1u);
}
/* Initial selected element derived from the handle. A bound saved affinity
 * takes precedence; this value is not the independent aptitude roll. */
static int element_from_character(void *character) {
    const unsigned char *h = (const unsigned char *)character_handle(character);
    if (!h)
        return 1;
    return (int)affinity_seed(*(const unsigned int *)(h + 0x14), *(const unsigned int *)(h + 0x18));
}
static EETrackedActor *track_actor(
    void *character, void *body, int eligible, void *left, void *right, int unarmed, int element) {
    int i, slot = -1;
    if (!character)
        return 0;
    if (eligible && body)
        tracked_actor_seen_remember(character);
    for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i) {
        if (tracked_actors[i].character == character) {
            slot = i;
            break;
        }
        if (slot < 0 && !tracked_actors[i].character)
            slot = i;
    }
    if (slot < 0)
        return 0;
    if (!eligible || !body) {
        if (tracked_actors[slot].character == character) {
            tracked_actors[slot].character = 0;
            tracked_actors[slot].body = 0;
            tracked_actors[slot].left = 0;
            tracked_actors[slot].right = 0;
            tracked_actors[slot].unarmed = 0;
            tracked_actors[slot].element = 0;
        }
        return 0;
    }
    if (tracked_actors[slot].character != character) {
        tracked_actors[slot].secondary = 0;
        tracked_actors[slot].level = 0;
        tracked_actors[slot].hit_glow = 0.0f;
        tracked_actors[slot].attacking = 0;
        tracked_actors[slot].melee_weapon = 0;
    }
#ifdef EE_ENABLE_PERSISTENCE
    tracked_actors[slot].element = element;
#else
    if (tracked_actors[slot].character != character)
        tracked_actors[slot].element = element;
#endif
    tracked_actors[slot].character = character;
    tracked_actors[slot].body = body;
    tracked_actors[slot].left = left;
    tracked_actors[slot].right = right;
    tracked_actors[slot].unarmed = unarmed;
    return &tracked_actors[slot];
}
static void forget_body(void *body) {
    int i;
    for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
        if (tracked_actors[i].body == body) {
            tracked_actors[i].character = 0;
            tracked_actors[i].body = 0;
        }
}
typedef struct {
    const char *message;
    U64 tick;
} LogLimit;
static LogLimit log_limits[64];
static void *log_guard;
static void logline(const char *s) {
    DWORD n = 0, w;
    void *f;
    unsigned i, slot = (unsigned)(((U64)s >> 4) & 63u);
    U64 now = GetTickCount64();
    int allowed = 0;
    AcquireSRWLockExclusive(&log_guard);
    for (i = 0; i < 64; ++i) {
        LogLimit *entry = &log_limits[(slot + i) & 63u];
        if (!entry->message || entry->message == s) {
            if (!entry->message || now - entry->tick >= 60000) {
                entry->message = s;
                entry->tick = now;
                allowed = 1;
            }
            break;
        }
    }
    ReleaseSRWLockExclusive(&log_guard);
    if (!allowed)
        return;
    while (s[n])
        ++n;
    f = CreateFileA(log_path, 4, 3, 0, 4, 0x80, 0);
    if (f != (void *)(U64)-1) {
        WriteFile(f, s, n, &w, 0);
        CloseHandle(f);
    }
}
/* Cache profile IDs for loaded characters. Caller holds guard; cached pointer
 * values are only compared, never dereferenced. */
typedef struct {
    void *character;
    unsigned int index, generation;
    int boss_profile;
} BossCacheEntry;
static BossCacheEntry boss_cache[4096];
static unsigned int boss_cache_count;
/** @brief Resolve and cache a configured boss profile from Kenshi's stable ID. */
static int boss_profile_for_character(void *character) {
    const unsigned char *h;
    void *data;
    const String *sid;
    unsigned int i, index, generation, slot;
    int profile = 0;
    if (!character)
        return 0;
    h = (const unsigned char *)character_handle(character);
    if (!h)
        return 0;
    index = *(const unsigned int *)(h + 0x14);
    generation = *(const unsigned int *)(h + 0x18);
    slot = (index ^ (generation * 2654435761u)) & 4095u;
    for (i = 0; i < 64; ++i) {
        BossCacheEntry *seen = &boss_cache[(slot + i) & 4095u];
        if (seen->character == character && seen->index == index && seen->generation == generation)
            return seen->boss_profile;
        if (!seen->character)
            break;
    }
    if (i == 64 || boss_cache_count >= 3072) {
        for (i = 0; i < 4096; ++i)
            boss_cache[i].character = 0;
        boss_cache_count = 0;
        i = 0;
    }
    slot = (slot + i) & 4095u;
    data = *(void **)((unsigned char *)character + 0x40);
    if (!data)
        return 0;
    sid = (const String *)((unsigned char *)data + 0x58);
    if (!sid->size || sid->size > 160 || sid->capacity < sid->size)
        return 0;
    {
        const char *text = sid->capacity < 16 ? sid->text.small : sid->text.heap;
        unsigned int j;
        for (j = 0; text && j < ee_boss_profile_count(); ++j) {
            const EEBossProfile *boss_entry = ee_boss_profile_at(j);
            const char *target;
            if (!boss_entry || !boss_entry->id)
                continue;
            target = boss_entry->id;
            for (i = 0; i < sid->size && target[i] && text[i] == target[i]; ++i) {
            }
            if (i == sid->size && !target[i]) {
                profile = (int)j + 1;
                break;
            }
        }
    }
    boss_cache[slot].character = character;
    boss_cache[slot].index = index;
    boss_cache[slot].generation = generation;
    boss_cache[slot].boss_profile = profile;
    ++boss_cache_count;
    return profile;
}
#ifdef EE_ENABLE_TRAIL
#include "elemental_trail_runtime.inl"
#endif
static int eq(const String *s, const char *text) {
    U64 i;
    const char *p;
    if (!s || s->size > 1024 || s->capacity < s->size)
        return 0;
    p = s->capacity < 16 ? s->text.small : s->text.heap;
    if (!p)
        return 0;
    for (i = 0; i < s->size && text[i]; ++i)
        if (p[i] != text[i])
            return 0;
    return i == s->size && !text[i];
}
static int has(const String *s, const char *part) {
    U64 i, j;
    const char *p;
    if (!s || !s->size || s->size > 1024 || s->capacity < s->size)
        return 0;
    p = s->capacity < 16 ? s->text.small : s->text.heap;
    if (!p)
        return 0;
    for (i = 0; i < s->size; ++i) {
        for (j = 0; part[j] && i + j < s->size; ++j) {
            char x = p[i + j];
            if (x >= 'A' && x <= 'Z')
                x += 'a' - 'A';
            if (x != part[j])
                break;
        }
        if (!part[j])
            return 1;
    }
    return 0;
}
static int blade_name(const String *s) {
    return has(s, "katana") || has(s, "sword") || has(s, "sabre") || has(s, "blade");
}
static void frost_clear(EEWeaponVisual *e) {
    void *obj = e->frost;
    void *scene = e->frost_scene;
    void *node = e->frost_node;
    int attached = e->frost_attached;
    e->frost = 0;
    e->frost_scene = 0;
    e->frost_node = 0;
    e->frost_attached = 0;
    if (obj && scene) {
        if (attached && node)
            original_scene_detach(node, obj);
        original_destroy_entity(scene, obj);
    }
    if (node && scene)
        node_destroy(scene, node);
}
static void frost_update(EEWeaponVisual *e, int wanted) {
    void *mesh;
    void *obj;
    const Vec *p;
    const Quat *q;
    String material = {{"EE_IceFrost"}, 11, 15, 0}, group = {{"Autodetect"}, 10, 15, 0};
    if (!wanted) {
        if (e->frost || e->frost_node)
            frost_clear(e);
        return;
    }
    __try {
        p = e->tag ? tag_position(e->tag) : 0;
        q = e->tag ? tag_rotation(e->tag) : 0;
        if (!p || !q)
            return;
        if (e->frost) {
            if (frost_get_visible && frost_set_visible) {
                unsigned char visible = frost_get_visible(e->blade);
                if (frost_get_visible(e->frost) != visible)
                    frost_set_visible(e->frost, visible);
            }
            if (e->frost_node) {
                node_position(e->frost_node, p);
                node_rotation(e->frost_node, q->w, q->x, q->y, q->z);
            }
            return;
        }
        if (!e->frost_bone || !e->scene || !e->parent || !frost_entity_create ||
            !frost_entity_material || !frost_get_visible || !frost_set_visible ||
            !frost_cast_shadows || !original_scene_detach)
            return;
        mesh = entity_mesh(e->blade);
        if (!mesh || !*(void **)mesh)
            return;
        e->frost_node = node_create(e->scene, 0);
        if (!e->frost_node)
            return;
        e->frost_scene = e->scene;
        node_add(e->parent, e->frost_node);
        node_position(e->frost_node, p);
        node_rotation(e->frost_node, q->w, q->x, q->y, q->z);
        obj = frost_entity_create(e->scene, mesh, 0);
        if (!obj) {
            frost_clear(e);
            return;
        }
        e->frost = obj;
        frost_entity_material(obj, &material, &group);
        frost_cast_shadows(obj, 0);
        node_attach(e->frost_node, obj);
        e->frost_attached = 1;
        frost_set_visible(obj, frost_get_visible(e->blade));
    } __except (1) {
        __try {
            frost_clear(e);
        } __except (1) {
            e->frost = 0;
            e->frost_scene = 0;
            e->frost_node = 0;
            e->frost_attached = 0;
        }
        e->frost_bone = 0;
        logline("FROST: disabled for an unsupported weapon.\r\n");
    }
}
static void cleanup(EEWeaponVisual *e) {
    if (e->frost || e->frost_node)
        frost_clear(e);
#ifdef EE_ENABLE_TRAIL
    trail_cleanup(e);
#endif
    if (e->effect2 && e->scene)
        particle_destroy(particle_manager(), e->effect2, e->scene);
    if (e->effect && e->scene)
        particle_destroy(particle_manager(), e->effect, e->scene);
    if (e->node && e->scene)
        node_destroy(e->scene, e->node);
    e->effect2 = 0;
    e->secondary = 0;
    e->effect = 0;
    e->node = 0;
    e->scene = 0;
    e->parent = 0;
    e->element = 0;
    e->applied_scale = 0.0f;
}
static void erase(EEWeaponVisual *e) {
    cleanup(e);
    e->owner = 0;
    e->blade = 0;
    e->tag = 0;
}
static void clear_hand(HandEffect *h) {
    if (h->effect2 && h->scene)
        particle_destroy(particle_manager(), h->effect2, h->scene);
    if (h->effect && h->scene)
        particle_destroy(particle_manager(), h->effect, h->scene);
    if (h->node && h->scene)
        node_destroy(h->scene, h->node);
    h->character = 0;
    h->body = 0;
    h->bone = 0;
    h->scene = 0;
    h->parent = 0;
    h->node = 0;
    h->effect = 0;
    h->effect2 = 0;
    h->secondary = 0;
    h->element = 0;
    h->applied_scale = 0.0f;
}
static void clear_character_hands(void *character) {
    int i;
    if (!character)
        return;
    for (i = 0; i < EE_TRACKED_ACTOR_LIMIT * 2; ++i)
        if (hand_effects[i].character == character)
            clear_hand(&hand_effects[i]);
}
static void *attach_hook(
    void *owner, const void *bone, void *object, const void *orientation, const void *offset) {
    void *tag = original_attach(owner, bone, object, orientation, offset);
    int i, pole;
    void **mesh;
    void *box;
    const Vec *max;
    float length;
    if (!active || internal == GetCurrentThreadId() || !tag || !object)
        return tag;
    __try {
        if (*(void **)object != entity_vtable)
            return tag;
        mesh = (void **)entity_mesh(object);
        if (!mesh || !*mesh)
            return tag;
        if (!eq((const String *)bone, "Bip01 Prop2") && !eq((const String *)bone, "Bip01 Prop1") &&
            !(eq((const String *)bone, "Bip01 L Hand") &&
              (has(resource_name(*mesh), "_bare") || blade_name(resource_name(*mesh)))))
            return tag;
        if (has(resource_name(*mesh), "sheath") || has(resource_name(*mesh), "crossbow") ||
            has(resource_name(*mesh), "arcane_focus") || has(resource_name(*mesh), "holster"))
            return tag;
        box = mesh_bounds(*mesh);
        max = box ? bounds_maximum(box) : 0;
        length = max ? max->y : 10.0f;
        pole = has(resource_name(*mesh), "polearm") || has(resource_name(*mesh), "spear");
        AcquireSRWLockExclusive(&guard);
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].blade == object)
                break;
        if (i == EE_WEAPON_VISUAL_LIMIT) {
            for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
                if (!weapon_visuals[i].blade) {
                    weapon_visuals[i].owner = owner;
                    weapon_visuals[i].blade = object;
                    weapon_visuals[i].tag = tag;
                    weapon_visuals[i].born = GetTickCount64();
                    weapon_visuals[i].length = (length > 2 && length < 80) ? length : 10.0f;
                    weapon_visuals[i].pole = pole;
                    weapon_visuals[i].frost_bone = eq((const String *)bone, "Bip01 Prop1")   ? 1
                                                   : eq((const String *)bone, "Bip01 Prop2") ? 2
                                                                                             : 3;
                    if (orientation && offset) {
                        weapon_visuals[i].frost_rotation = *(const Quat *)orientation;
                        weapon_visuals[i].frost_offset = *(const Vec *)offset;
                    } else
                        weapon_visuals[i].frost_bone = 0;
                    break;
                }
            if (i == EE_WEAPON_VISUAL_LIMIT && !logged_full) {
                logged_full = 1;
                logline("CAPACITY: all weapon tracking slots are in use.\r\n");
            }
        }
        ReleaseSRWLockExclusive(&guard);
    } __except (1) {
        logline("SKIP: blade metadata unavailable.\r\n");
    }
    return tag;
}
static void detach_hook(void *owner, void *blade) {
    int i;
    if (internal == GetCurrentThreadId()) {
        original_detach(owner, blade);
        return;
    }
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].owner == owner && weapon_visuals[i].blade == blade)
                erase(&weapon_visuals[i]);
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
    original_detach(owner, blade);
}
static void detach_all_hook(void *owner) {
    int i;
    if (internal == GetCurrentThreadId()) {
        original_detach_all(owner);
        return;
    }
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].owner == owner)
                erase(&weapon_visuals[i]);
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
    original_detach_all(owner);
}
static void clear_hook(void *scene) {
    int i;
    if (internal == GetCurrentThreadId()) {
        original_clear(scene);
        return;
    }
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        impact_clear_scene(scene);
        clear_crossbow_scene(scene);
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].blade &&
                (!weapon_visuals[i].scene || weapon_visuals[i].scene == scene))
                erase(&weapon_visuals[i]);
        for (i = 0; i < EE_TRACKED_ACTOR_LIMIT * 2; ++i)
            if (hand_effects[i].scene == scene)
                clear_hand(&hand_effects[i]);
        for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i) {
            tracked_actors[i].character = 0;
            tracked_actors[i].body = 0;
        }
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
    original_clear(scene);
}
static void entity_hook(void *scene, void *object) {
    int i;
    if (internal == GetCurrentThreadId()) {
        original_destroy_entity(scene, object);
        return;
    }
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].owner == object || weapon_visuals[i].blade == object)
                erase(&weapon_visuals[i]);
        for (i = 0; i < EE_TRACKED_ACTOR_LIMIT * 2; ++i)
            if (hand_effects[i].body == object)
                clear_hand(&hand_effects[i]);
        forget_body(object);
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
    original_destroy_entity(scene, object);
}
static void node_hook(void *scene, void *node) {
    int i;
    if (internal == GetCurrentThreadId()) {
        original_destroy_node(scene, node);
        return;
    }
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        impact_clear_parent(node);
        clear_crossbow_source(node);
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].parent == node)
                erase(&weapon_visuals[i]);
        for (i = 0; i < EE_TRACKED_ACTOR_LIMIT * 2; ++i)
            if (hand_effects[i].parent == node)
                clear_hand(&hand_effects[i]);
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
    original_destroy_node(scene, node);
}
static void scene_detach_hook(void *node, void *object) {
    int i;
    if (internal == GetCurrentThreadId()) {
        original_scene_detach(node, object);
        return;
    }
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].owner == object)
                cleanup(&weapon_visuals[i]);
        for (i = 0; i < EE_TRACKED_ACTOR_LIMIT * 2; ++i)
            if (hand_effects[i].body == object)
                clear_hand(&hand_effects[i]);
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
    original_scene_detach(node, object);
}
static void free_tag_hook(void *skeleton, void *tag) {
    int i;
    if (internal == GetCurrentThreadId()) {
        original_free_tag(skeleton, tag);
        return;
    }
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].tag == tag && tag)
                erase(&weapon_visuals[i]);
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
    original_free_tag(skeleton, tag);
}
static void free_object_hook(void *skeleton, void *object) {
    int i;
    if (internal == GetCurrentThreadId()) {
        original_free_object(skeleton, object);
        return;
    }
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].blade == object && object)
                erase(&weapon_visuals[i]);
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
    original_free_object(skeleton, object);
}
#ifdef EE_ENABLE_PERSISTENCE
static int failed_save_read(void *context, int out[3]) {
    (void)context;
    (void)out;
    return -1;
}
/* Caller holds guard. Player saves stay untouched; non-player profiles are
 * transient and never inserted into persistence or awarded experience. */
static int combat_affinity_locked(void *character, void *stats, EEAffinity *out) {
    const unsigned char *h;
    unsigned int index, serial_number;
    int boss;
    if (!character || !out)
        return 0;
    if (character_is_player(character)) {
        EECharacterRecord *r;
        h = (const unsigned char *)character_handle(character);
        if (!stats || !h)
            return 0;
        index = *(const unsigned int *)(h + 0x14);
        serial_number = *(const unsigned int *)(h + 0x18);
        r = ee_persistence_bind(&persistence,
                                stats,
                                character,
                                index,
                                serial_number,
                                index ^ (serial_number * 0x9e3779b9u),
                                (unsigned int)element_from_character(character));
        if (!r || r->read_only || !r->affinity.selected)
            return 0;
        *out = r->affinity;
        return 1;
    }
    boss = boss_profile_for_character(character);
    if (!boss)
        return 0;
    {
        const EEBossProfile *p = ee_boss_profile_at((unsigned int)(boss - 1));
        unsigned int l;
        if (!p)
            return 0;
        l = (unsigned int)p->level;
        if (!ee_affinity_create(out, 0, (unsigned int)p->primary))
            return 0;
        out->total_xp = (unsigned short)ee_affinity_xp_for_level(l);
        out->secondary = (unsigned int)p->secondary;
    }
    return out->selected != 0;
}
/** @brief Apply the affinity speed bonus to melee combat or attack speed. */
static float element_speed(void *stats, float base, int attacking) {
    float result = base;
    if (!active || !stats || internal == GetCurrentThreadId())
        return base;
    __try {
        void *character = *(void **)((unsigned char *)stats + 0x10);
        if (character && melee_context(character)) {
            EEAffinity a;
            AcquireSRWLockExclusive(&guard);
            __try {
                if (combat_affinity_locked(character, stats, &a))
                    result = attacking
                                 ? ee_affinity_attack_speed_scaled(&a, base, effect_multiplier)
                                 : ee_affinity_combat_speed_scaled(&a, base, effect_multiplier);
            } __finally {
                ReleaseSRWLockExclusive(&guard);
            }
        }
    } __except (1) {
        logline("WIND: speed bonus skipped after native error.\r\n");
    }
    return result;
}
static float
calculate_combat_speed_hook(void *stats, float weight, float skill, unsigned char is_block) {
    float base = original_calculate_combat_speed(stats, weight, skill, is_block);
    return element_speed(stats, base, is_block ? 0 : 1);
}
#ifdef ELEMENTAL_NATIVE_TEST
static float attack_speed_hook(void *stats) {
    float base = original_attack_speed(stats);
    return element_speed(stats, base, 1);
}
static float block_speed_hook(void *stats) {
    float base = original_block_speed(stats);
    return element_speed(stats, base, 0);
}
#endif
static int character_hit_hook(
    void *target, int direction, void *damage, void *attacker, void *technique, int combo) {
    int result, has_bonus = 0;
    EEDamage original = {0}, modified = {0}, bonus;
    if (active && damage && attacker && target && attacker != target &&
        internal != GetCurrentThreadId())
        __try {
            if (melee_context(attacker)) {
                void *stats = character_stats(attacker);
                EEAffinity a;
                AcquireSRWLockExclusive(&guard);
                __try {
                    if (combat_affinity_locked(attacker, stats, &a)) {
                        original = *(EEDamage *)damage;
                        modified = original;
                        has_bonus = ee_affinity_damage_scaled(&a, &modified, effect_multiplier);
                    }
                } __finally {
                    ReleaseSRWLockExclusive(&guard);
                }
            }
        } __except (1) {
            has_bonus = 0;
            logline("EFFECT: hit bonus skipped after native error.\r\n");
        }
    if (has_bonus) {
        bonus.cut = modified.cut - original.cut;
        bonus.blunt = modified.blunt - original.blunt;
        bonus.pierce = modified.pierce - original.pierce;
        bonus.stun = modified.stun - original.stun;
        bonus.bleed = modified.bleed - original.bleed;
        bonus.armour = modified.armour - original.armour;
        result = original_character_hit(target, direction, &modified, attacker, technique, combo);
        __try {
            EEDamage *d = (EEDamage *)damage;
            d->cut = modified.cut - bonus.cut;
            d->blunt = modified.blunt - bonus.blunt;
            d->pierce = modified.pierce - bonus.pierce;
            d->stun = modified.stun - bonus.stun;
            d->bleed = modified.bleed - bonus.bleed;
            d->armour = modified.armour - bonus.armour;
        } __except (1) {
            logline("EFFECT: output copy skipped after native error.\r\n");
        }
    } else
        result = original_character_hit(target, direction, damage, attacker, technique, combo);
    if (active && attacker && target && attacker != target &&
        (result == 1 || result == 2 || result == 6) && internal != GetCurrentThreadId())
        __try {
            if (character_is_player(attacker) && melee_context(attacker)) {
                void *stats = character_stats(attacker);
                const unsigned char *h = (const unsigned char *)character_handle(attacker);
                if (stats && h) {
                    unsigned int index = *(const unsigned int *)(h + 0x14),
                                 serial = *(const unsigned int *)(h + 0x18);
                    AcquireSRWLockExclusive(&guard);
                    __try {
                        EECharacterRecord *r =
                            ee_persistence_bind(&persistence,
                                                stats,
                                                attacker,
                                                index,
                                                serial,
                                                index ^ (serial * 0x9e3779b9u),
                                                (unsigned int)element_from_character(attacker));
                        if (r && !r->read_only &&
                            ee_affinity_award_hit_rate_scaled(
                                &r->affinity, all_aptitudes, xp_tenths)) {
                            int k;
                            if (show_impacts && ee_affinity_level(&r->affinity) >= 40) {
                                impact_queue_character(
                                    target, (int)r->affinity.selected, (int)r->affinity.secondary);
                                for (k = 0; k < EE_TRACKED_ACTOR_LIMIT; ++k)
                                    if (tracked_actors[k].character == attacker) {
                                        tracked_actors[k].hit_glow = .18f;
                                        break;
                                    }
                            }
                        }
                    } __finally {
                        ReleaseSRWLockExclusive(&guard);
                    }
                }
            }
        } __except (1) {
            logline("XP: hit ignored after native error.\r\n");
        }
    return result;
}
static void *crossbow_wound_hook(void *medical,
                                 unsigned char low,
                                 int direction,
                                 void *damage,
                                 int *material,
                                 void *attacker,
                                 int *attackdir,
                                 void *bolt) {
    int applied = 0, impact_element = 0, impact_secondary = 0;
    EEDamage modified = {0}, original = {0}, bonus;
    void *result;
    void *stats = 0;
    void *impact_scene = 0;
    Vec impact_position = {0};
    unsigned int index = 0, serial_number = 0;
    if (active && bolt && damage && attacker && internal != GetCurrentThreadId())
        __try {
            {
                int player = character_is_player(attacker);
                const unsigned char *h =
                    player ? (const unsigned char *)character_handle(attacker) : 0;
                void *character_stat = character_stats(attacker);
                EEAffinity a;
                if (player && character_stat && h) {
                    stats = character_stat;
                    index = *(const unsigned int *)(h + 0x14);
                    serial_number = *(const unsigned int *)(h + 0x18);
                }
                AcquireSRWLockExclusive(&guard);
                __try {
                    if (combat_affinity_locked(attacker, character_stat, &a)) {
                        float pierce;
                        if (show_impacts && player && ee_affinity_level(&a) >= 40 &&
                            crossbow_impact_sample(bolt, &impact_scene, &impact_position)) {
                            impact_element = (int)a.selected;
                            impact_secondary = (int)a.secondary;
                        }
                        original = *(EEDamage *)damage;
                        modified = original;
                        pierce = modified.pierce > 0.0f ? modified.pierce : 0.0f;
                        modified.cut += pierce;
                        applied = ee_affinity_damage_scaled(&a, &modified, effect_multiplier);
                        modified.cut -= pierce;
                    }
                } __finally {
                    ReleaseSRWLockExclusive(&guard);
                }
            }
        } __except (1) {
            applied = 0;
            stats = 0;
            logline("CROSSBOW: damage skipped after error.\r\n");
        }
    if (applied) {
        bonus.cut = modified.cut - original.cut;
        bonus.blunt = modified.blunt - original.blunt;
        bonus.pierce = modified.pierce - original.pierce;
        bonus.stun = modified.stun - original.stun;
        bonus.bleed = modified.bleed - original.bleed;
        bonus.armour = modified.armour - original.armour;
        result = original_crossbow_wound(
            medical, low, direction, &modified, material, attacker, attackdir, bolt);
        __try {
            EEDamage *d = (EEDamage *)damage;
            d->cut = modified.cut - bonus.cut;
            d->blunt = modified.blunt - bonus.blunt;
            d->pierce = modified.pierce - bonus.pierce;
            d->stun = modified.stun - bonus.stun;
            d->bleed = modified.bleed - bonus.bleed;
            d->armour = modified.armour - bonus.armour;
        } __except (1) {
            /* The target damage buffer may no longer be writable. */
        }
    } else
        result = original_crossbow_wound(
            medical, low, direction, damage, material, attacker, attackdir, bolt);
    if (bolt && active && internal != GetCurrentThreadId()) {
        AcquireSRWLockExclusive(&guard);
        internal = GetCurrentThreadId();
        __try {
            void *live_scene = 0;
            Vec live_position;
            if (result && impact_scene && impact_element &&
                crossbow_impact_sample(bolt, &live_scene, &live_position) &&
                live_scene == impact_scene)
                impact_queue_position(live_scene, &live_position, impact_element, impact_secondary);
            clear_crossbow_projectile(bolt);
        } __finally {
            internal = 0;
            ReleaseSRWLockExclusive(&guard);
        }
    }
    if (result && stats && attacker && bolt && active)
        __try {
            AcquireSRWLockExclusive(&guard);
            __try {
                EECharacterRecord *r =
                    ee_persistence_bind(&persistence,
                                        stats,
                                        attacker,
                                        index,
                                        serial_number,
                                        index ^ (serial_number * 0x9e3779b9u),
                                        (unsigned int)element_from_character(attacker));
                if (r && !r->read_only)
                    ee_affinity_award_hit_rate_scaled(&r->affinity, all_aptitudes, xp_tenths);
            } __finally {
                ReleaseSRWLockExclusive(&guard);
            }
        } __except (1) {
            logline("CROSSBOW: hit XP skipped after error.\r\n");
        }
    return result;
}
static void stats_load_hook(void *stats, void *data) {
    if (active && stats && data && internal != GetCurrentThreadId())
        __try {
            EEGameDataContext c = {data, &gamedata_api};
            EESaveBackend b = {&c, ee_gamedata_read, ee_gamedata_write};
            int status;
            AcquireSRWLockExclusive(&guard);
            __try {
                status = ee_persistence_stage(&persistence, stats, &b);
            } __finally {
                ReleaseSRWLockExclusive(&guard);
            }
            if (status == EE_SAVE_UNSUPPORTED)
                logline("SAVE: unknown affinity version retained without changes.\r\n");
        } __except (1) {
            /* A fault while reading must not turn unknown saved data into a new,
             * writable default. Stage a read-only sentinel without touching data. */
            EESaveBackend failed = {0, failed_save_read, 0};
            AcquireSRWLockExclusive(&guard);
            __try {
                ee_persistence_stage(&persistence, stats, &failed);
            } __finally {
                ReleaseSRWLockExclusive(&guard);
            }
            logline("SAVE: affinity read failed; preserving data as read-only.\r\n");
        }
    original_stats_load(stats, data);
}
static void stats_save_hook(void *stats, void *data) {
    original_stats_save(stats, data);
    if (active && stats && data && internal != GetCurrentThreadId())
        __try {
            void *character = *(void **)((unsigned char *)stats + 0x10);
            const unsigned char *h =
                character ? (const unsigned char *)character_handle(character) : 0;
            if (h) {
                EEGameDataContext c = {data, &gamedata_api};
                EESaveBackend b = {&c, ee_gamedata_read, ee_gamedata_write};
                int status;
                AcquireSRWLockExclusive(&guard);
                __try {
                    status = ee_persistence_save(&persistence,
                                                 character,
                                                 *(const unsigned int *)(h + 0x14),
                                                 *(const unsigned int *)(h + 0x18),
                                                 &b);
                } __finally {
                    ReleaseSRWLockExclusive(&guard);
                }
                if (status == EE_SAVE_ERROR || status == EE_SAVE_UNSUPPORTED)
                    logline("SAVE: affinity was not written.\r\n");
            }
        } __except (1) {
            logline("SAVE: skipped affinity write after error.\r\n");
        }
}
static void stats_destroy_hook(void *stats) {
    if (internal != GetCurrentThreadId()) {
        AcquireSRWLockExclusive(&guard);
        ee_persistence_forget(&persistence, stats, 0);
        ReleaseSRWLockExclusive(&guard);
    }
    original_stats_destroy(stats);
}
static void character_set_handle_hook(void *character, const void *next) {
    int started = 0;
    unsigned int old_index = 0, old_serial = 0;
    const unsigned char *h;
    if (active && character && internal != GetCurrentThreadId())
        __try {
            h = (const unsigned char *)character_handle(character);
            if (h) {
                old_index = *(const unsigned int *)(h + 0x14);
                old_serial = *(const unsigned int *)(h + 0x18);
                AcquireSRWLockExclusive(&guard);
                started = ee_character_handle_begin(
                    &persistence.characters, character, old_index, old_serial);
                ReleaseSRWLockExclusive(&guard);
            }
        } __except (1) {
            started = 0;
        }
    original_character_set_handle(character, next);
    if (started) {
        unsigned int index = old_index, serial_number = old_serial;
        __try {
            h = (const unsigned char *)character_handle(character);
            if (h) {
                index = *(const unsigned int *)(h + 0x14);
                serial_number = *(const unsigned int *)(h + 0x18);
            }
        } __except (1) {
            logline("SAVE: handle continuity check failed.\r\n");
        }
        AcquireSRWLockExclusive(&guard);
        ee_character_handle_end(&persistence.characters, character, index, serial_number);
        ReleaseSRWLockExclusive(&guard);
    }
}
#endif
/**
 * @brief Refresh tracked ally or boss state from one character update.
 * @details Unknown NPCs are filtered before they enter the tracking cache.
 */
static void character_update_hook(void *character) {
    void *app = 0;
    void *body = 0;
    void *skeleton = 0;
    void *left = 0;
    void *right = 0;
    int player = 0, unarmed = 0, element = 1;
    int secondary = 0, level = 0, melee = 0, boss = 0, skip_untracked = 0;
#ifdef EE_ENABLE_TRAIL
    int attacking = 0;
#endif
#ifdef EE_ENABLE_PERSISTENCE
    void *stats = 0;
    unsigned int index = 0, serial_number = 0;
    int has_handle = 0;
#endif
    String hands = {{"hands"}, 5, 15, 0}, lb = {{"Bip01 L Hand"}, 12, 15, 0},
           rb = {{"Bip01 R Hand"}, 12, 15, 0};
    if (active && internal != GetCurrentThreadId())
        __try {
            player = character_is_player(character);
            if (!player) {
                AcquireSRWLockExclusive(&guard);
                __try {
                    __try {
                        boss = boss_profile_for_character(character);
                    } __except (1) {
                        boss = 0;
                    }
                    skip_untracked = !boss && !tracked_actor_seen_maybe(character);
                } __finally {
                    ReleaseSRWLockExclusive(&guard);
                }
            }
            if (!skip_untracked) {
                if (player || boss) {
                    const EEBossProfile *profile =
                        boss ? ee_boss_profile_at((unsigned int)(boss - 1)) : 0;
                    element = player ? element_from_character(character)
                                     : (profile ? profile->primary : 1);
                    melee = melee_context(character);
#ifdef EE_ENABLE_PERSISTENCE
                    {
                        const unsigned char *h = (const unsigned char *)character_handle(character);
                        if (h) {
                            index = *(const unsigned int *)(h + 0x14);
                            serial_number = *(const unsigned int *)(h + 0x18);
                            has_handle = 1;
                        }
                        stats = character_stats(character);
                    }
#endif
                    app = character_appearance(character);
                    if (app) {
                        body = appearance_body(app);
                        unarmed = melee == 1 && character_combat(character, 1, 0) &&
                                  !appearance_attached(app, &hands);
                        if (unarmed) {
                            skeleton = appearance_skeleton(app);
                            if (skeleton) {
                                left = skeleton_bone(skeleton, &lb);
                                right = skeleton_bone(skeleton, &rb);
                            }
                        }
                    }
#ifdef EE_ENABLE_TRAIL
                    if (trail_character_combat && trail_combat_state &&
                        character_combat(character, 1, 0)) {
                        void *combat = trail_character_combat(character);
                        if (combat)
                            attacking = trail_combat_state(combat) == 0;
                    }
#endif
                }
                AcquireSRWLockExclusive(&guard);
                __try {
                    if (boss) {
                        const EEBossProfile *profile = ee_boss_profile_at((unsigned int)(boss - 1));
                        if (profile) {
                            secondary = profile->secondary;
                            level = profile->level;
                        }
                    }
#ifdef EE_ENABLE_PERSISTENCE
                    if (player && body && has_handle) {
                        EECharacterRecord *r =
                            ee_persistence_bind(&persistence,
                                                stats,
                                                character,
                                                index,
                                                serial_number,
                                                index ^ (serial_number * 0x9e3779b9u),
                                                (unsigned int)element);
                        if (r) {
                            element = (int)r->affinity.selected;
                            if (!r->read_only) {
                                secondary = (int)r->affinity.secondary;
                                level = (int)ee_affinity_level(&r->affinity);
                            }
                        }
                    }
#endif
                    {
                        EETrackedActor *ally = track_actor(
                            character, body, player || boss, left, right, unarmed, element);
                        if (ally) {
                            ally->secondary = secondary;
                            ally->level = level;
                            ally->melee_weapon = melee == 2;
#ifdef EE_ENABLE_TRAIL
                            ally->attacking = attacking;
#endif
                        }
                    }
                } __finally {
                    ReleaseSRWLockExclusive(&guard);
                }
            }
        } __except (1) {
            logline("SKIP: player identity or hand bones unavailable.\r\n");
        }
    original_character_update(character);
}
static void character_destroy_hook(void *character) {
    if (internal != GetCurrentThreadId()) {
        AcquireSRWLockExclusive(&guard);
        internal = GetCurrentThreadId();
        __try {
            clear_crossbow_character(character);
            clear_character_hands(character);
            track_actor(character, 0, 0, 0, 0, 0, 0);
#ifdef EE_ENABLE_PERSISTENCE
            ee_persistence_forget(&persistence, 0, character);
#endif
        } __finally {
            internal = 0;
            ReleaseSRWLockExclusive(&guard);
        }
    }
    original_character_destroy(character);
}
#ifdef EE_ENABLE_UI
#include "elemental_ui_runtime.inl"
#endif
static void *
create_secondary_effect(void *manager, void *scene, void *node, const String *tmpl, float height) {
    String name = {{"EE_S00000000"}, 12, 15, 0};
    unsigned int n = (unsigned int)++visual_serial, j;
    void *effect;
    void *tech;
    void *emitter;
    if (!scene || !node || !particle_template(manager, tmpl))
        return 0;
    for (j = 0; j < 8; ++j)
        name.text.small[4 + j] = "0123456789abcdef"[(n >> (28 - j * 4)) & 15];
    effect = particle_create(manager, &name, tmpl, scene);
    if (!effect)
        return 0;
    if (height > 0.0f) {
        tech = particle_technique(effect, 0);
        emitter = tech ? technique_emitter(tech, 0) : 0;
        if (!emitter) {
            particle_destroy(manager, effect, scene);
            return 0;
        }
        box_height(emitter, height);
        if (tmpl->size == 27 || tmpl->size == 23) {
            tech = particle_technique(effect, 1);
            emitter = tech ? technique_emitter(tech, 0) : 0;
            if (emitter)
                box_height(emitter, height);
        }
        if (tmpl->size == 23) {
            tech = particle_technique(effect, 2);
            emitter = tech ? technique_emitter(tech, 0) : 0;
            if (emitter)
                box_height(emitter, height);
        }
    }
    node_attach(node, effect);
    particle_local(effect, 1);
    set_queue(effect, 84);
    particle_start(effect);
    particle_bounds(effect, 0, 0);
    return effect;
}
#include "elemental_impact_visual.inl"
#include "elemental_crossbow_visual.inl"
static float visual_scale(int level, float glow) {
    float scale = level >= 100
                      ? 1.15f
                      : (level >= 80 ? 1.1f : (level >= 40 ? 1.0f : (level >= 20 ? .9f : .8f)));
    return glow > 0.0f ? scale * 1.3f : scale;
}
static const String *element_template(String *storage, int kind, int element) {
    if (kind < EE_TEMPLATE_HAND || kind > EE_TEMPLATE_CROSSBOW)
        return 0;
    if (element < 1 || element > 5) {
        if (kind == EE_TEMPLATE_IMPACT)
            return 0;
        element = 1;
    }
    const EEEffectTemplate *source = ee_effect_template(kind, element);
    if (!storage || !source)
        return 0;
    return ee_game_string_init(storage, source->resource, source->size, source->capacity) ? storage
                                                                                          : 0;
}
static void update_hands(void *manager, float dt) {
    int i, k;
    const float bounds[6] = {0, 0, 0, 20, 20, 20};
    for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
        for (k = 0; k < 2; ++k) {
            EETrackedActor *a = &tracked_actors[i];
            HandEffect *h = &hand_effects[i * 2 + k];
            void *bone = k ? a->right : a->left;
            void *parent;
            const Vec *p;
            const Quat *q;
            if (!a->character || !a->body || !a->unarmed || !a->element || !bone) {
                if (h->effect)
                    clear_hand(h);
                continue;
            }
            parent = parent_node(a->body);
            if (!parent) {
                if (h->effect)
                    clear_hand(h);
                continue;
            }
            if (h->effect && (h->character != a->character || h->body != a->body ||
                              h->bone != bone || h->parent != parent || h->element != a->element))
                clear_hand(h);
            if (!h->effect) {
                String name = {{"EE_H00000000"}, 12, 15, 0}, template_storage;
                unsigned int n = (unsigned int)++visual_serial, j;
                h->character = a->character;
                h->body = a->body;
                h->bone = bone;
                h->parent = parent;
                h->scene = node_scene(parent);
                if (!h->scene) {
                    clear_hand(h);
                    continue;
                }
                const String *tmpl =
                    element_template(&template_storage, EE_TEMPLATE_HAND, a->element);
                if (!particle_template(manager, tmpl)) {
                    clear_hand(h);
                    continue;
                }
                h->element = a->element;
                for (j = 0; j < 8; ++j)
                    name.text.small[4 + j] = "0123456789abcdef"[(n >> (28 - j * 4)) & 15];
                h->effect = particle_create(manager, &name, tmpl, h->scene);
                h->node = node_create(h->scene, 0);
                if (!h->effect || !h->node) {
                    clear_hand(h);
                    continue;
                }
                node_add(parent, h->node);
                node_attach(h->node, h->effect);
                particle_local(h->effect, 1);
                set_queue(h->effect, 84);
                particle_start(h->effect);
                particle_bounds(h->effect, 0, 0);
            }
            if (h->effect2 && h->secondary != a->secondary) {
                particle_destroy(manager, h->effect2, h->scene);
                h->effect2 = 0;
                h->secondary = 0;
            }
            if (a->secondary && !h->effect2) {
                String second_storage;
                const String *second =
                    element_template(&second_storage, EE_TEMPLATE_HAND, a->secondary);
                h->effect2 = create_secondary_effect(manager, h->scene, h->node, second, 0);
                if (h->effect2)
                    h->secondary = a->secondary;
            }
            {
                float scale = visual_scale(a->level, a->hit_glow);
                if (scale != h->applied_scale) {
                    Vec size = {scale, scale, scale};
                    node_scale(h->node, &size);
                    h->applied_scale = scale;
                }
            }
            p = tag_position(bone);
            q = tag_rotation(bone);
            if (p && q) {
                node_position(h->node, p);
                node_rotation(h->node, q->w, q->x, q->y, q->z);
            }
            particle_update(h->effect, dt);
            set_aabb(h->effect, bounds);
            if (h->effect2) {
                particle_update(h->effect2, dt);
                set_aabb(h->effect2, bounds);
            }
        }
}
/**
 * @brief Update attached weapon, hand, impact, and bolt visuals.
 * @details Called from the Ogre update hook while the shared render-state
 * lock is held. Particle resource names come from elemental_effect_templates.c.
 */
static void update(float dt) {
    int i;
    void *manager;
    const float bounds[6] = {0, 0, 0, 60, 60, 60};
    String fire_storage;
    if (dt < 0 || dt > .1f)
        dt = .016f;
    manager = particle_manager();
    if (!manager ||
        !particle_template(manager, element_template(&fire_storage, EE_TEMPLATE_BLADE, 1)))
        return;
    AcquireSRWLockExclusive(&guard);
    internal = GetCurrentThreadId();
    __try {
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT; ++i)
            if (weapon_visuals[i].blade) {
                EEWeaponVisual *e = &weapon_visuals[i];
                EETrackedActor *ally = find_tracked_actor_by_body(e->owner);
                void *parent;
                const Vec *p;
                const Quat *q;
                int element = ally ? ally->element : 0, secondary = ally ? ally->secondary : 0;
                String template_storage;
                const String *tmpl =
                    element_template(&template_storage, EE_TEMPLATE_BLADE, element);
                e->level = ally ? ally->level : 0;
                if (!element || !ally->melee_weapon) {
                    if (e->effect)
                        cleanup(e);
                    if (!ally && GetTickCount64() - e->born > 5000)
                        erase(e);
                    continue;
                }
                if (e->effect && e->element != element)
                    cleanup(e);
                parent = parent_node(e->owner);
                if (!parent) {
                    cleanup(e);
                    continue;
                }
                if (e->parent && e->parent != parent)
                    cleanup(e);
                if (!e->effect) {
                    String name = {{"EE_F00000000"}, 12, 15, 0};
                    unsigned int n = (unsigned int)++visual_serial, j;
                    e->scene = node_scene(parent);
                    if (!e->scene)
                        continue;
                    e->parent = parent;
                    for (j = 0; j < 8; ++j)
                        name.text.small[4 + j] = "0123456789abcdef"[(n >> (28 - j * 4)) & 15];
                    if (!particle_template(manager, tmpl)) {
                        cleanup(e);
                        continue;
                    }
                    e->element = element;
                    e->effect = particle_create(manager, &name, tmpl, e->scene);
                    e->node = node_create(e->scene, 0);
                    if (!e->effect || !e->node) {
                        cleanup(e);
                        continue;
                    }
                    {
                        float start = e->length * (e->pole ? .45f : .12f), end = e->length * .98f;
                        void *tech = particle_technique(e->effect, 0);
                        void *emitter = tech ? technique_emitter(tech, 0) : 0;
                        if (!emitter) {
                            cleanup(e);
                            continue;
                        }
                        box_height(emitter, end - start);
                        if (element == 2 || element == 4) {
                            tech = particle_technique(e->effect, 1);
                            emitter = tech ? technique_emitter(tech, 0) : 0;
                            if (emitter)
                                box_height(emitter, end - start);
                        }
                        if (element == 4) {
                            tech = particle_technique(e->effect, 2);
                            emitter = tech ? technique_emitter(tech, 0) : 0;
                            if (emitter)
                                box_height(emitter, end - start);
                        }
                        node_add(parent, e->node);
                        node_attach(e->node, e->effect);
                        particle_local(e->effect, 1);
                    }
                    set_queue(e->effect, 84);
                    particle_start(e->effect);
                    particle_bounds(e->effect, 0, 0);
                }
                if (e->effect2 && e->secondary != secondary) {
                    particle_destroy(manager, e->effect2, e->scene);
                    e->effect2 = 0;
                    e->secondary = 0;
                }
                if (secondary && !e->effect2) {
                    float start = e->length * (e->pole ? .45f : .12f), end = e->length * .98f;
                    String second_storage;
                    const String *second =
                        element_template(&second_storage, EE_TEMPLATE_BLADE, secondary);
                    e->effect2 =
                        create_secondary_effect(manager, e->scene, e->node, second, end - start);
                    if (e->effect2)
                        e->secondary = secondary;
                }
#ifdef EE_ICE_OVERLAY_EXPERIMENTAL
                frost_update(e, element == 5 || secondary == 5);
#endif
                {
                    float scale = visual_scale(e->level, ally->hit_glow);
                    if (scale != e->applied_scale) {
                        Vec size = {scale, scale, scale};
                        node_scale(e->node, &size);
                        e->applied_scale = scale;
                    }
                }
                p = tag_position(e->tag);
                q = tag_rotation(e->tag);
                if (p && q) {
                    float start = e->length * (e->pole ? .45f : .12f), end = e->length * .98f,
                          off = (start + end) * .5f - 5.0f;
                    Vec adjusted = {p->x + 2 * (q->x * q->y - q->w * q->z) * off,
                                    p->y + (1 - 2 * (q->x * q->x + q->z * q->z)) * off,
                                    p->z + 2 * (q->y * q->z + q->w * q->x) * off};
                    node_position(e->node, &adjusted);
                    node_rotation(e->node, q->w, q->x, q->y, q->z);
#ifdef EE_ENABLE_TRAIL
                    if (show_trails && e->level >= 20 && ally->attacking)
                        trail_tick(e, p, q, dt);
                    else
                        trail_finish(e, dt);
#endif
                }
                particle_update(e->effect, dt);
                set_aabb(e->effect, bounds);
                if (e->effect2) {
                    particle_update(e->effect2, dt);
                    set_aabb(e->effect2, bounds);
                }
            }
        update_hands(manager, dt);
        impact_update(manager, dt);
        cb_update_visuals(manager, dt);
        for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
            if (tracked_actors[i].hit_glow > 0.0f) {
                tracked_actors[i].hit_glow -= dt;
                if (tracked_actors[i].hit_glow < 0.0f)
                    tracked_actors[i].hit_glow = 0.0f;
            }
    } __finally {
        internal = 0;
        ReleaseSRWLockExclusive(&guard);
    }
}
#ifdef EE_FROST_RELOAD_DEV
typedef struct {
    void *object;
    void *info;
} EEFrostSharedTexture;
static EEFrostSharedTexture frost_dev_texture;
static int frost_dev_disabled;
static void frost_dev_text(char **p, const char *s) {
    while (*s)
        *(*p)++ = *s++;
}
static void frost_dev_num(char **p, unsigned v) {
    char b[12];
    unsigned n = 0;
    do {
        b[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v && n < 11);
    while (n)
        *(*p)++ = b[--n];
}
static void frost_dev_report(void) {
    MODULE ogre = GetModuleHandleA("OgreMain_x64.dll");
    unsigned char (*queue)(void *);
    unsigned char (*visible)(void *);
    int i, count = 0;
    *(void **)(&queue) = GetProcAddress(ogre, "?getRenderQueueGroup@MovableObject@Ogre@@QEBAEXZ");
    *(void **)(&visible) = GetProcAddress(ogre, "?getVisible@MovableObject@Ogre@@QEBA_NXZ");
    AcquireSRWLockExclusive(&guard);
    __try {
        for (i = 0; i < EE_WEAPON_VISUAL_LIMIT && count < 12; ++i) {
            EEWeaponVisual *e = &weapon_visuals[i];
            char line[512], *p = line;
            void **m;
            const String *s;
            const char *n;
            U64 j;
            if (!e->blade || (e->element != 5 && e->secondary != 5))
                continue;
            __try {
                frost_dev_text(&p, "FROST DIAG: mesh=");
                m = (void **)entity_mesh(e->blade);
                s = m && *m ? resource_name(*m) : 0;
                n = s ? (s->capacity < 16 ? s->text.small : s->text.heap) : 0;
                if (n && s->size <= 160)
                    for (j = 0; j < s->size; ++j)
                        *p++ = n[j];
                else
                    frost_dev_text(&p, "unknown");
                frost_dev_text(&p, " bone=");
                frost_dev_num(&p, (unsigned)e->frost_bone);
                frost_dev_text(&p, " layer=");
                frost_dev_num(&p, e->frost ? 1 : 0);
                frost_dev_text(&p, " attached=");
                frost_dev_num(&p, (unsigned)e->frost_attached);
                if (queue && visible && e->frost) {
                    frost_dev_text(&p, " queue_blade=");
                    frost_dev_num(&p, queue(e->blade));
                    frost_dev_text(&p, " queue_layer=");
                    frost_dev_num(&p, queue(e->frost));
                    frost_dev_text(&p, " visible_blade=");
                    frost_dev_num(&p, visible(e->blade));
                    frost_dev_text(&p, " visible_layer=");
                    frost_dev_num(&p, visible(e->frost));
                }
                *p++ = '\r';
                *p++ = '\n';
                *p = 0;
                {
                    DWORD written;
                    void *file = CreateFileA(log_path, 4, 3, 0, 4, 0x80, 0);
                    if (file != (void *)(U64)-1) {
                        WriteFile(file, line, (DWORD)(p - line), &written, 0);
                        CloseHandle(file);
                    }
                }
                ++count;
            } __except (1) {
            }
        }
    } __finally {
        ReleaseSRWLockExclusive(&guard);
    }
    if (!count)
        logline("FROST DIAG: no currently tracked ice weapon.\r\n");
}
static int frost_dev_reload(void) {
    MODULE ogre = GetModuleHandleA("OgreMain_x64.dll");
    void *manager;
    void *(*instance)(void);
    void *(*get_texture)(void *, EEFrostSharedTexture *, const String *, const String *);
    void (*reload)(void *);
    String name = {{0}, 15, 15, 0}, group = {{"Autodetect"}, 10, 15, 0};
    const char *n = "EE_IceFrost.png";
    unsigned i;
    if (!ogre || frost_dev_disabled)
        return 0;
    *(void **)(&instance) =
        GetProcAddress(ogre, "?getSingletonPtr@TextureManager@Ogre@@SAPEAV12@XZ");
    *(void **)(&get_texture) =
        GetProcAddress(ogre,
                       "?getByName@TextureManager@Ogre@@QEAA?AV?$SharedPtr@VTexture@Ogre@@@2@AEBV?$"
                       "basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z");
    *(void **)(&reload) = GetProcAddress(ogre, "?reload@Resource@Ogre@@UEAAXXZ");
    if (!instance || !get_texture || !reload)
        return 0;
    __try {
        if (!frost_dev_texture.object) {
            manager = instance();
            if (!manager)
                return 0;
            for (i = 0; i < 15; ++i)
                name.text.small[i] = n[i];
            name.text.small[15] = 0;
            get_texture(manager, &frost_dev_texture, &name, &group);
        }
        if (!frost_dev_texture.object)
            return 0;
        reload(frost_dev_texture.object);
        logline("FROST DEV: texture reloaded.\r\n");
        return 1;
    } __except (1) {
        frost_dev_disabled = 1;
        logline("FROST DEV: reload disabled after native error.\r\n");
        return 0;
    }
}
static void frost_dev_poll(void) {
    static U64 next;
    static int seen = -1;
    U64 now = GetTickCount64();
    int serial;
    if (!settings_path[0] || now < next)
        return;
    next = now + 1000;
    serial = (int)GetPrivateProfileIntA("Developer", "FrostReloadSerial", 0, settings_path);
    if (seen < 0) {
        seen = serial;
        return;
    }
    if (serial != seen && frost_dev_reload()) {
        seen = serial;
        frost_dev_report();
    }
}
#ifdef ELEMENTAL_NATIVE_TEST
__declspec(dllexport) int probeTestFrostReload(void) {
    return frost_dev_reload();
}
#endif
#endif
static void gpu_hook(void *world, float dt) {
    original_gpu(world, dt);
    if (active)
        __try {
#ifdef EE_FROST_RELOAD_DEV
            frost_dev_poll();
#endif
            update(dt);
        } __except (1) {
            active = 0;
            logline("DISABLED: enchantment visual update failed.\r\n");
        }
}
static int distinct_hook_targets(void *a, void *b) {
    return a && b && a != b;
}
static int module_matches(MODULE m, unsigned int stamp, unsigned int size) {
    unsigned char *b = (unsigned char *)m;
    unsigned int p;
    if (!b || *(unsigned short *)b != 0x5a4d)
        return 0;
    p = *(unsigned int *)(b + 0x3c);
    return *(unsigned int *)(b + p) == 0x4550 && *(unsigned int *)(b + p + 8) == stamp &&
           *(unsigned int *)(b + p + 80) == size;
}
/**
 * @brief Resolve supported game APIs and install the plugin hooks.
 * @details Hook targets are validated against the supported Kenshi executable;
 * unsupported projectile entry points leave crossbow flight visuals disabled.
 */
__declspec(dllexport) void startPlugin(void) {
    MODULE ogre = GetModuleHandleA("OgreMain_x64.dll"),
           pu = GetModuleHandleA("Plugin_ParticleUniverse_x64.dll"),
           lib = GetModuleHandleA("KenshiLib.dll"), game = GetModuleHandleA("kenshi_x64.exe");
    int ok = 1;
    DWORD i, last = 0;
    void *stub;
    void *gpu_stub;
    void *gpu_target;
    void *target;
    void *char_update;
    void *char_destroy;
    int (*hook)(void *, void *, void **);
    long long (*address)(void *);
#ifdef EE_ENABLE_PERSISTENCE
    void *stats_load;
    void *stats_save;
    void *stats_destroy;
    void *char_set_handle;
    void *char_hit;
    void *combat_speed_calculate;
    void *crossbow_wound;
#endif
#ifdef EE_ENABLE_UI
    MODULE mygui = GetModuleHandleA("MyGUIEngine_x64.dll");
    void *ui_update;
    void *ui_clear;
    void *ui_destroy;
#endif
    if (!self || !GetModuleFileNameA(self, log_path, sizeof(log_path)))
        return;
    for (i = 0; log_path[i]; ++i)
        if (log_path[i] == '\\' || log_path[i] == '/')
            last = i;
    for (i = 0; i <= last; ++i)
        settings_path[i] = log_path[i];
    {
        const char *name = "ElementalEnchantment.ini";
        for (i = 0; name[i] && last + 1 + i < sizeof(settings_path) - 1; ++i)
            settings_path[last + 1 + i] = name[i];
        settings_path[last + 1 + i] = 0;
    }
    reload_settings();
    {
        const char *name = "ElementalEnchantment.log";
        for (i = 0; name[i]; ++i)
            log_path[last + 1 + i] = name[i];
        log_path[last + 1 + i] = 0;
    }
    if (!lib || !module_matches(ogre, 0x5ca5f929, 0x9c9000) ||
        !module_matches(pu, 0x5ca5faa4, 0x718000) ||
        !(module_matches(game, 0x6602d59d, 0x232d000) ||
          module_matches(game, 0x65d604d7, 0x232c000))) {
        logline("INACTIVE: unreviewed game or engine build.\r\n");
        return;
    }
#ifdef EE_ENABLE_UI
    if (!mygui) {
        logline("INACTIVE: MyGUI module missing.\r\n");
        return;
    }
#endif
#define API(f, m, s)                                                                               \
    *(void **)(&f) = GetProcAddress(m, s);                                                         \
    if (!f)                                                                                        \
    ok = 0
    API(hook, lib, "?AddHook@KenshiLib@@YA?AW4HookStatus@1@PEAX0PEAPEAX@Z");
    API(address, lib, "?GetRealAddress@KenshiLib@@YA_JPEAX@Z");
    if (!ok)
        return;
#ifdef EE_ENABLE_UI
    stub = GetProcAddress(lib, "?getInstance@LocaleManager@@SAPEAV1@XZ");
    locale_manager_instance = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib, "?getCurrentLocale@LocaleManager@@QEBAPEAVLocaleInfo@@XZ");
    locale_current = stub ? (void *)(U64)address(stub) : 0;
#endif
#define GAMEAPI(f, s)                                                                              \
    stub = GetProcAddress(lib, s);                                                                 \
    *(void **)(&f) = stub ? (void *)(U64)address(stub) : 0;                                        \
    if (!f)                                                                                        \
    ok = 0
    GAMEAPI(character_is_player, "?isPlayerCharacter@Character@@QEBA_NXZ");
    GAMEAPI(character_appearance, "?getAppearance@Character@@QEAAPEAVAppearanceBase@@XZ");
    GAMEAPI(appearance_body, "?getBody@AppearanceBase@@QEBAPEAVEntity@Ogre@@XZ");
    GAMEAPI(character_handle, "?getHandle@RootObjectBase@@QEBAAEBVhand@@XZ");
#ifdef EE_ENABLE_TRAIL
    stub = GetProcAddress(lib, "?getCombatClass@Character@@QEBAPEAVCombatClass@@XZ");
    *(void **)(&trail_character_combat) = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib, "?getCombatState@CombatClass@@QEBA?AW4swordStateEnum@@XZ");
    *(void **)(&trail_combat_state) = stub ? (void *)(U64)address(stub) : 0;
    if (!trail_character_combat || !trail_combat_state)
        logline("TRAIL: combat state unavailable; trails disabled.\r\n");
#endif
#ifdef EE_ENABLE_PERSISTENCE
    ee_persistence_init(&persistence);
    ee_characters_set_seed_provider(fresh_aptitude_seed);
    GAMEAPI(character_stats, "?getStats@Character@@QEAAPEAVCharStats@@XZ");
    GAMEAPI(gamedata_api.list_exists,
            "?listExists@GameData@@QEBA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@"
            "2@@std@@@Z");
    GAMEAPI(gamedata_api.list_size,
            "?getListSize@GameData@@QEBAHAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@"
            "2@@std@@@Z");
    GAMEAPI(gamedata_api.list_name,
            "?getFromList@GameData@@QEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@"
            "@std@@AEBV23@H@Z");
    GAMEAPI(gamedata_api.list_values,
            "?getValueFromList@GameData@@QEBAAEBVTripleInt@@AEBV?$basic_string@DU?$char_traits@D@"
            "std@@V?$allocator@D@2@@std@@H@Z");
    GAMEAPI(gamedata_api.find_reference,
            "?getGameDataReferenceObject@GameData@@QEAAPEAVGameDataReference@@AEBV?$basic_string@"
            "DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z");
    GAMEAPI(gamedata_api.add_reference,
            "?addToList@GameData@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
            "std@@0HHH@Z");
    stub = GetProcAddress(lib, "?updateStats@CharStats@@QEAAXPEAVGameData@@@Z");
    stats_load = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib, "?serialise@CharStats@@QEAAXPEAVGameData@@@Z");
    stats_save = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib, "?_DESTRUCTOR@CharStats@@QEAAXXZ");
    stats_destroy = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib, "?_NV_setHandle@Character@@QEAAXAEBVhand@@@Z");
    char_set_handle = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib,
                          "?_NV_hitByMeleeAttack@Character@@QEAA?AW4HitMaterialType@@"
                          "W4CutDirection@@AEAVDamages@@PEAV1@PEAVCombatTechniqueData@@H@Z");
    char_hit = stub ? (void *)(U64)address(stub) : 0;
    combat_speed_calculate = 0;
    if (module_matches(game, 0x65d604d7, 0x232c000))
        __try {
            static const unsigned char prologue[] = {
                0x48, 0x8b, 0xc4, 0x48, 0x81, 0xec, 0x98, 0, 0, 0, 0x0f, 0x29, 0x70, 0xe8};
            const unsigned char *code = (const unsigned char *)game + 0x882bb0;
            unsigned j;
            void *mapped = 0;
            stub = GetProcAddress(lib, "?calculateAttackOrBlockSpeed@CharStats@@QEAAMMM_N@Z");
            if (stub)
                mapped = (void *)(U64)address(stub);
            for (j = 0; j < sizeof(prologue) && code[j] == prologue[j]; ++j) {
            }
            if ((j == sizeof(prologue) || code[0] == 0xe9) && mapped == (void *)code &&
                *((const unsigned char *)game + 0x897802) == 0xe8 &&
                *((const unsigned char *)game + 0x897906) == 0xe8)
                combat_speed_calculate = (void *)code;
        } __except (1) {
            combat_speed_calculate = 0;
        }
    if (!stats_load || !stats_save || !stats_destroy || !char_set_handle || !char_hit)
        ok = 0;
#endif
#ifdef EE_ENABLE_UI
    GAMEAPI(stats_window_width, "?getWidth@GUIWindow@@UEBAHXZ");
    GAMEAPI(stats_window_height, "?getHeight@GUIWindow@@UEBAHXZ");
    GAMEAPI(stats_window_visible, "?_NV_isVisible@CharacterStatsWindow@@QEBA_NXZ");
    stub = GetProcAddress(lib, "?_NV_update@CharacterStatsWindow@@QEAAXXZ");
    ui_update = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib, "?_NV_clear@CharacterStatsWindow@@QEAAXXZ");
    ui_clear = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib, "?_DESTRUCTOR@CharacterStatsWindow@@QEAAXXZ");
    ui_destroy = stub ? (void *)(U64)address(stub) : 0;
    if (!ui_update || !ui_clear || !ui_destroy)
        ok = 0;
    API(mygui_api.gui_instance,
        mygui,
        "?getInstancePtr@?$Singleton@VGui@MyGUI@@@MyGUI@@SAPEAVGui@2@XZ");
    API(mygui_api.create_widget,
        mygui,
        "?createWidgetT@Widget@MyGUI@@QEAAPEAV12@AEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@0AEBU?$TCoord@H@types@2@UAlign@2@0@Z");
    API(mygui_api.set_font,
        mygui,
        "?setFontName@TextBox@MyGUI@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@"
        "2@@std@@@Z");
    *(void **)(&mygui_api.get_font_height) =
        GetProcAddress(mygui, "?getFontHeight@TextBox@MyGUI@@QEAAHXZ");
    *(void **)(&mygui_api.set_font_height) =
        GetProcAddress(mygui, "?setFontHeight@TextBox@MyGUI@@UEAAXH@Z");
    API(mygui_api.set_style,
        mygui,
        "?setWidgetStyle@Widget@MyGUI@@QEAAXUWidgetStyle@2@AEBV?$basic_string@DU?$char_traits@D@"
        "std@@V?$allocator@D@2@@std@@@Z");
    API(mygui_api.set_size, mygui, "?setSize@Widget@MyGUI@@QEAAXHH@Z");
    *(void **)(&mygui_api.set_position) =
        GetProcAddress(mygui, "?setPosition@Widget@MyGUI@@QEAAXHH@Z");
    API(mygui_api.set_property,
        mygui,
        "?setProperty@Widget@MyGUI@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@"
        "2@@std@@0@Z");
    API(mygui_api.destroy_widget, mygui, "?destroyWidget@Gui@MyGUI@@QEAAXPEAVWidget@2@@Z");
    API(mygui_api.set_visible, mygui, "?setVisible@Widget@MyGUI@@UEAAX_N@Z");
    API(mygui_api.set_caption,
        mygui,
        "?setCaptionWithReplacing@TextBox@MyGUI@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@@Z");
    API(mygui_api.set_selected, mygui, "?setStateSelected@Button@MyGUI@@QEAAX_N@Z");
    API(mygui_api.canvas_created, mygui, "?isTextureCreated@Canvas@MyGUI@@QEBA_NXZ");
    API(mygui_api.canvas_create,
        mygui,
        "?createTexture@Canvas@MyGUI@@QEAAXHHW4TextureResizeMode@12@UTextureUsage@2@UPixelFormat@2@"
        "@Z");
    API(mygui_api.canvas_width, mygui, "?getTextureRealWidth@Canvas@MyGUI@@QEBAHXZ");
    API(mygui_api.canvas_height, mygui, "?getTextureRealHeight@Canvas@MyGUI@@QEBAHXZ");
    API(mygui_api.canvas_lock, mygui, "?lock@Canvas@MyGUI@@QEAAPEAXUTextureUsage@2@@Z");
    API(mygui_api.canvas_unlock, mygui, "?unlock@Canvas@MyGUI@@QEAAXXZ");
    API(input_focus, mygui, "?getMouseFocusWidget@InputManager@MyGUI@@QEBAPEAVWidget@2@XZ");
    API(widget_parent, mygui, "?getParent@Widget@MyGUI@@QEBAPEAV12@XZ");
    ee_mygui_backend(&mygui_api, &ui_api);
#endif
    GAMEAPI(character_combat, "?isInCombatMode@Character@@QEBA_N_N0@Z");
    GAMEAPI(appearance_attached,
            "?getAttachedEntity@AppearanceBase@@QEBAPEAVAttachedEntity@@AEBV?$basic_string@DU?$"
            "char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    GAMEAPI(appearance_skeleton,
            "?getSkeleton@AppearanceBase@@QEBAPEAVOldSkeletonInstance@Ogre@@XZ");
    GAMEAPI(weapon_category, "?getCategory@Weapon@@QEBA?AW4WeaponCategory@@XZ");
    character_current_weapon = current_weapon_virtual;
    API(skeleton_bone,
        ogre,
        "?getBone@Skeleton@Ogre@@UEBAPEAVOldBone@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@@Z");
    stub = GetProcAddress(lib, "?_NV_update@Character@@QEAAXXZ");
    char_update = stub ? (void *)(U64)address(stub) : 0;
    stub = GetProcAddress(lib, "?_DESTRUCTOR@Character@@QEAAXXZ");
    char_destroy = stub ? (void *)(U64)address(stub) : 0;
    if (!char_update || !char_destroy)
        ok = 0;
    entity_vtable = GetProcAddress(ogre, "??_7Entity@Ogre@@6BMovableObject@1@@");
    if (!entity_vtable)
        ok = 0;
    API(entity_mesh, ogre, "?getMesh@Entity@Ogre@@QEBAAEBV?$SharedPtr@VMesh@Ogre@@@2@XZ");
    *(void **)(&frost_entity_create) =
        GetProcAddress(ogre,
                       "?createEntity@SceneManager@Ogre@@UEAAPEAVEntity@2@AEBV?$SharedPtr@VMesh@"
                       "Ogre@@@2@W4SceneMemoryMgrTypes@2@@Z");
    *(void **)(&frost_entity_material) =
        GetProcAddress(ogre,
                       "?setMaterialName@Entity@Ogre@@QEAAXAEBV?$basic_string@DU?$char_traits@D@"
                       "std@@V?$allocator@D@2@@std@@0@Z");
    *(void **)(&frost_get_visible) =
        GetProcAddress(ogre, "?getVisible@MovableObject@Ogre@@QEBA_NXZ");
    *(void **)(&frost_set_visible) =
        GetProcAddress(ogre, "?setVisible@MovableObject@Ogre@@QEAAX_N@Z");
    *(void **)(&frost_cast_shadows) =
        GetProcAddress(ogre, "?setCastShadows@MovableObject@Ogre@@QEAAX_N@Z");
    API(mesh_bounds, ogre, "?getBounds@Mesh@Ogre@@QEBAAEBVAxisAlignedBox@2@XZ");
    API(bounds_maximum, ogre, "?getMaximum@AxisAlignedBox@Ogre@@QEBAAEBVVector3@2@XZ");
    API(resource_name,
        ogre,
        "?getName@Resource@Ogre@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
        "std@@XZ");
    API(parent_node, ogre, "?getParentSceneNode@MovableObject@Ogre@@QEBAPEAVSceneNode@2@XZ");
    API(node_scene, ogre, "?getCreator@SceneNode@Ogre@@QEBAPEAVSceneManager@2@XZ");
    API(tag_position, ogre, "?_getDerivedPosition@OldNode@Ogre@@UEBAAEBVVector3@2@XZ");
    API(tag_rotation, ogre, "?_getDerivedOrientation@OldNode@Ogre@@UEBAAEBVQuaternion@2@XZ");
    API(particle_manager,
        pu,
        "?getSingletonPtr@ParticleSystemManager@ParticleUniverse@@SAPEAV12@XZ");
    API(particle_template,
        pu,
        "?getParticleSystemTemplate@ParticleSystemManager@ParticleUniverse@@QEAAPEAVParticleSystem@"
        "2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    API(particle_create,
        pu,
        "?createParticleSystem@ParticleSystemManager@ParticleUniverse@@QEAAPEAVParticleSystem@2@"
        "AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0PEAVSceneManager@Ogre@@@"
        "Z");
    API(particle_destroy,
        pu,
        "?destroyParticleSystem@ParticleSystemManager@ParticleUniverse@@QEAAXPEAVParticleSystem@2@"
        "PEAVSceneManager@Ogre@@@Z");
    API(particle_start, pu, "?start@ParticleSystem@ParticleUniverse@@QEAAXXZ");
    API(particle_update, pu, "?_update@ParticleSystem@ParticleUniverse@@QEAAXM@Z");
    *(void **)(&particle_count) = GetProcAddress(
        pu, "?getNumberOfEmittedParticles@ParticleSystem@ParticleUniverse@@QEAA_KXZ");
    API(particle_technique,
        pu,
        "?getTechnique@ParticleSystem@ParticleUniverse@@QEBAPEAVParticleTechnique@2@_K@Z");
    API(technique_emitter,
        pu,
        "?getEmitter@ParticleTechnique@ParticleUniverse@@QEBAPEAVParticleEmitter@2@_K@Z");
    API(box_height, pu, "?setHeight@BoxEmitter@ParticleUniverse@@QEAAXM@Z");
    API(particle_local, pu, "?setKeepLocal@ParticleSystem@ParticleUniverse@@QEAAX_N@Z");
    API(particle_bounds, pu, "?setBoundsAutoUpdated@ParticleSystem@ParticleUniverse@@QEAAX_NM@Z");
    API(set_aabb, ogre, "?setLocalAabb@MovableObject@Ogre@@QEAAXUAabb@2@@Z");
    API(set_queue, ogre, "?setRenderQueueGroup@MovableObject@Ogre@@UEAAXE@Z");
    API(node_create,
        ogre,
        "?createSceneNode@SceneManager@Ogre@@UEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z");
    API(node_destroy, ogre, "?destroySceneNode@SceneManager@Ogre@@UEAAXPEAVSceneNode@2@@Z");
    API(node_add, ogre, "?addChild@Node@Ogre@@QEAAXPEAV12@@Z");
    API(node_position, ogre, "?setPosition@Node@Ogre@@QEAAXAEBVVector3@2@@Z");
    API(node_scale, ogre, "?setScale@Node@Ogre@@QEAAXAEBVVector3@2@@Z");
    API(node_rotation, ogre, "?setOrientation@Node@Ogre@@QEAAXMMMM@Z");
    API(node_attach, ogre, "?attachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
#ifdef EE_ENABLE_TRAIL
    if (!trail_resolve(ogre))
        logline("TRAIL: optional ribbon API unavailable.\r\n");
#endif
    gpu_stub = GetProcAddress(lib, "?_NV_mainLoop_GPUSensitiveStuff@GameWorld@@QEAAXM@Z");
    if (!ok || !gpu_stub) {
        logline("INACTIVE: required native API missing.\r\n");
        return;
    }
    *(void **)(&cb_normalise_vec) = GetProcAddress(ogre, "?normalise@Vector3@Ogre@@QEAAMXZ");
    *(void **)(&cb_normalise_quat) = GetProcAddress(ogre, "?normalise@Quaternion@Ogre@@QEAAMXZ");
    *(void **)(&cb_particle_scale) =
        GetProcAddress(pu, "?setScale@ParticleSystem@ParticleUniverse@@QEAAXAEBVVector3@Ogre@@@Z");
    stub = GetProcAddress(lib, "?_NV_getBarrelPos@GunClass@@QEAA?AVVector3@Ogre@@XZ");
    cb_barrel = stub ? (void *)(U64)address(stub) : 0;
    gpu_target = (void *)(U64)address(gpu_stub);
#ifdef EE_ENABLE_PERSISTENCE
    stub =
        GetProcAddress(lib,
                       "?addWound@MedicalSystem@@QEAAPEAVGameData@@_NW4CutDirection@@AEAVDamages@@"
                       "AEAHPEAVRootObject@@AEAW4Enum@AttackDirection@@PEAVHarpoon@@@Z");
    crossbow_wound = stub ? (void *)(U64)address(stub) : 0;
    if (!distinct_hook_targets(gpu_target, crossbow_wound)) {
        logline("INACTIVE: hook targets invalid or overlapping.\r\n");
        return;
    }
#endif
#define HOOK(m, s, d, o)                                                                           \
    target = GetProcAddress(m, s);                                                                 \
    if (!target || hook(target, d, (void **)&o) != 0 || !o)                                        \
    ok = 0
    HOOK(ogre,
         "?attachObjectToBone@Entity@Ogre@@QEAAPEAVTagPoint@2@AEBV?$basic_string@DU?$char_traits@D@"
         "std@@V?$allocator@D@2@@std@@PEAVMovableObject@2@AEBVQuaternion@2@AEBVVector3@2@@Z",
         attach_hook,
         original_attach);
    HOOK(ogre,
         "?detachObjectFromBone@Entity@Ogre@@QEAAXPEAVMovableObject@2@@Z",
         detach_hook,
         original_detach);
    HOOK(ogre,
         "?detachAllObjectsFromBone@Entity@Ogre@@QEAAXXZ",
         detach_all_hook,
         original_detach_all);
    HOOK(ogre, "?clearScene@SceneManager@Ogre@@UEAAXXZ", clear_hook, original_clear);
    HOOK(ogre,
         "?destroyEntity@SceneManager@Ogre@@UEAAXPEAVEntity@2@@Z",
         entity_hook,
         original_destroy_entity);
    HOOK(ogre,
         "?destroySceneNode@SceneManager@Ogre@@UEAAXPEAVSceneNode@2@@Z",
         node_hook,
         original_destroy_node);
    HOOK(ogre,
         "?detachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z",
         scene_detach_hook,
         original_scene_detach);
    HOOK(ogre,
         "?freeTagPoint@OldSkeletonInstance@Ogre@@QEAAXPEAVTagPoint@2@@Z",
         free_tag_hook,
         original_free_tag);
    HOOK(ogre,
         "?freeTagPoint@OldSkeletonInstance@Ogre@@QEAAXPEAVMovableObject@2@@Z",
         free_object_hook,
         original_free_object);
    if (!char_update ||
        hook(char_update, character_update_hook, (void **)&original_character_update) != 0 ||
        !original_character_update)
        ok = 0;
    if (!char_destroy ||
        hook(char_destroy, character_destroy_hook, (void **)&original_character_destroy) != 0 ||
        !original_character_destroy)
        ok = 0;
    /* Flight addresses are enabled only on the specifically verified executable. */
    if (module_matches(game, 0x65d604d7, 0x232c000) && cb_barrel && cb_normalise_vec &&
        cb_normalise_quat && trail_root && trail_derive) {
        unsigned char *code = (unsigned char *)game;
        void *trace = (void *)(code + 0x4354d0);
        void *destroy = (void *)(code + 0x435610);
        void *create = (void *)(code + 0x439f50);
        void *shoot;
        stub = GetProcAddress(lib, "?updateT@RangedCombatClass@@QEAAXMAEBVhand@@0@Z");
        target = stub ? (void *)(U64)address(stub) : 0;
        stub = GetProcAddress(lib,
                              "?shoot@GunClass@@QEAAXPEAVCharacter@@PEAVRootObject@@"
                              "W4StatsEnumerated@@AEBVVector3@Ogre@@@Z");
        shoot = stub ? (void *)(U64)address(stub) : 0;
        if (shoot && shoot != gpu_target && shoot != target && shoot != trace && shoot != destroy &&
            shoot != create && target && target != gpu_target && trace != gpu_target &&
            destroy != gpu_target && create != gpu_target && target != trace && target != destroy &&
            target != create && trace != destroy && trace != create && destroy != create) {
            if (hook(target, cb_update_hook, (void **)&original_ranged_update) != 0 ||
                !original_ranged_update)
                ok = 0;
            if (hook(shoot, cb_shoot_hook, (void **)&original_crossbow_shoot) != 0 ||
                !original_crossbow_shoot)
                ok = 0;
            if (hook(create, cb_create_hook, (void **)&original_harpoon_create) != 0 ||
                !original_harpoon_create)
                ok = 0;
            if (hook(trace, cb_trace_hook, (void **)&original_projectile_trace) != 0 ||
                !original_projectile_trace)
                ok = 0;
            if (hook(destroy, cb_destroy_hook, (void **)&original_projectile_destroy) != 0 ||
                !original_projectile_destroy)
                ok = 0;
        } else
            logline("CROSSBOW: unverified flight entry points; visuals disabled.\r\n");
    } else
        logline("CROSSBOW: flight unsupported on this executable.\r\n");
#ifdef EE_ENABLE_PERSISTENCE
    if (!stats_load || hook(stats_load, stats_load_hook, (void **)&original_stats_load) != 0 ||
        !original_stats_load)
        ok = 0;
    if (!stats_save || hook(stats_save, stats_save_hook, (void **)&original_stats_save) != 0 ||
        !original_stats_save)
        ok = 0;
    if (!stats_destroy ||
        hook(stats_destroy, stats_destroy_hook, (void **)&original_stats_destroy) != 0 ||
        !original_stats_destroy)
        ok = 0;
    if (!char_set_handle ||
        hook(char_set_handle, character_set_handle_hook, (void **)&original_character_set_handle) !=
            0 ||
        !original_character_set_handle)
        ok = 0;
    if (!char_hit || hook(char_hit, character_hit_hook, (void **)&original_character_hit) != 0 ||
        !original_character_hit)
        ok = 0;
    if (!crossbow_wound ||
        hook(crossbow_wound, crossbow_wound_hook, (void **)&original_crossbow_wound) != 0 ||
        !original_crossbow_wound)
        ok = 0;
    if (!combat_speed_calculate ||
        hook(combat_speed_calculate,
             calculate_combat_speed_hook,
             (void **)&original_calculate_combat_speed) != 0 ||
        !original_calculate_combat_speed)
        logline("SPEED: unverified calculation; melee speed bonuses disabled.\r\n");
#endif
#ifdef EE_ENABLE_UI
    if (!ui_update ||
        hook(ui_update, stats_window_update_hook, (void **)&original_stats_window_update) != 0 ||
        !original_stats_window_update)
        ok = 0;
    if (!ui_clear ||
        hook(ui_clear, stats_window_clear_hook, (void **)&original_stats_window_clear) != 0 ||
        !original_stats_window_clear)
        ok = 0;
    if (!ui_destroy ||
        hook(ui_destroy, stats_window_destroy_hook, (void **)&original_stats_window_destroy) != 0 ||
        !original_stats_window_destroy)
        ok = 0;
    HOOK(mygui,
         "?injectMousePress@InputManager@MyGUI@@QEAA_NHHUMouseButton@2@@Z",
         mouse_press_hook,
         original_mouse_press);
    if (mygui_api.set_position) {
        HOOK(mygui,
             "?injectMouseMove@InputManager@MyGUI@@QEAA_NHHH@Z",
             mouse_move_hook,
             original_mouse_move);
        HOOK(mygui,
             "?injectMouseRelease@InputManager@MyGUI@@QEAA_NHHUMouseButton@2@@Z",
             mouse_release_hook,
             original_mouse_release);
    }
#endif
    target = gpu_target;
    if (!ok || !target || hook(target, gpu_hook, (void **)&original_gpu) != 0 || !original_gpu) {
        logline("INACTIVE: native hooks unavailable.\r\n");
        return;
    }
    active = 1;
}
int __stdcall DllMain(MODULE module, DWORD reason, void *unused) {
    (void)unused;
    if (reason == 1)
        self = module;
    return 1;
}
#include "elemental_runtime_tests.inl"
