#ifdef ELEMENTAL_NATIVE_TEST
/* Only exported in the isolated engine test build; never in the game package. */
__declspec(dllexport) int probeTestTemplateStrings(void) {
    int kind, element;
    for (kind = EE_TEMPLATE_HAND; kind <= EE_TEMPLATE_CROSSBOW; ++kind) {
        for (element = 1; element <= 5; ++element) {
            String storage;
            const EEEffectTemplate *expected = ee_effect_template(kind, element);
            const String *actual = element_template(&storage, kind, element);
            const char *text;
            unsigned int i;
            if (!expected || !actual || actual->size != expected->size ||
                actual->capacity != expected->capacity)
                return 1;
            text = actual->capacity < 16 ? actual->text.small : actual->text.heap;
            for (i = 0; i < expected->size; ++i)
                if (text[i] != expected->resource[i])
                    return 2;
            if (text[expected->size] != '\0')
                return 3;
        }
    }
    return 0;
}
static void *impact_test_appearance(void *target) {
    return target ? (void *)11 : 0;
}
static void *impact_test_body(void *appearance) {
    return appearance ? (void *)12 : 0;
}
static void *impact_test_parent(void *body) {
    return body ? (void *)13 : 0;
}
static void *impact_test_scene(void *node) {
    return node ? (void *)123 : 0;
}
static void *impact_test_derived(void *node, Vec *out) {
    if (!node)
        return 0;
    *out = (Vec){1, 2, 3};
    return out;
}
__declspec(dllexport) int probeTestImpactPositions(void) {
    int i;
    Vec p;
    void *scene = 0;
    for (i = 0; i < EE_IMPACT_LIMIT; ++i)
        impacts[i] = (EEImpact){0};
    character_appearance = impact_test_appearance;
    appearance_body = impact_test_body;
    parent_node = impact_test_parent;
    node_scene = impact_test_scene;
    trail_derive = impact_test_derived;
    particle_manager = 0;
    impact_queue_character((void *)1, 4, 5);
    if (impacts[0].scene != (void *)123 || impacts[0].parent != (void *)13 ||
        impacts[0].position.y != 10 || impacts[0].element != 4 || impacts[0].secondary != 5)
        return 1;
    impact_clear_parent((void *)13);
    if (impacts[0].scene)
        return 3;
    cb_flights[0] = (CBFlight){0};
    cb_flights[0].projectile = (void *)20;
    cb_flights[0].source = (void *)13;
    cb_flights[0].frames = 1;
    cb_flights[0].offset = (Vec){10, 20, 30};
    if (!crossbow_impact_sample((void *)20, &scene, &p) || scene != (void *)123 || p.x != 11 ||
        p.y != 22 || p.z != 33)
        return 2;
    cb_flights[0] = (CBFlight){0};
    return 0;
}
__declspec(dllexport) int probeTestImpactQueue(void) {
    Vec p = {1, 2, 3};
    int i;
    void *scene = (void *)123;
    for (i = 0; i < EE_IMPACT_LIMIT; ++i)
        impacts[i] = (EEImpact){0};
    if (impact_queue_position(scene, &p, 0, 0) || impact_queue_position(0, &p, 1, 0))
        return 1;
    for (i = 0; i < EE_IMPACT_LIMIT; ++i)
        if (!impact_queue_position(scene, &p, 1, 2))
            return 2;
    if (impact_queue_position(scene, &p, 1, 0))
        return 3;
    particle_manager = 0;
    impact_clear_scene(scene);
    for (i = 0; i < EE_IMPACT_LIMIT; ++i)
        if (impacts[i].scene)
            return 4;
    for (i = 1; i <= 5; ++i)
        if (!ee_effect_template(EE_TEMPLATE_IMPACT, i))
            return 5;
    return 0;
}
__declspec(dllexport) int probeTestImpactSetting(void) {
    Vec p = {1, 2, 3};
    int i;
    for (i = 0; i < EE_IMPACT_LIMIT; ++i)
        impacts[i] = (EEImpact){0};
    particle_manager = 0;
    show_impacts = 0;
    if (impact_queue_position((void *)123, &p, 1, 0))
        return 1;
    show_impacts = 1;
    if (!impact_queue_position((void *)123, &p, 1, 0))
        return 2;
    show_impacts = 0;
    impact_update(0, .016f);
    if (impacts[0].scene)
        return 3;
    show_impacts = 1;
    return 0;
}
__declspec(dllexport) float probeTestMultiplierFromIni(const char *path) {
    unsigned i;
    if (!path)
        return -1.0f;
    for (i = 0; i < sizeof(settings_path) - 1 && path[i]; ++i)
        settings_path[i] = path[i];
    settings_path[i] = 0;
    reload_settings();
    return effect_multiplier;
}
__declspec(dllexport) int probeTestImpactSettingFromIni(const char *path) {
    if (!path)
        return -1;
    probeTestMultiplierFromIni(path);
    return show_impacts;
}
__declspec(dllexport) int probeTestXPSettingFromIni(const char *path) {
    if (!path)
        return -1;
    probeTestMultiplierFromIni(path);
    return (int)xp_tenths;
}
__declspec(dllexport) int probeTestUIScaleFromIni(const char *path) {
    if (!path)
        return -1;
    probeTestMultiplierFromIni(path);
    return (int)ui_scale_tenths;
}
__declspec(dllexport) int probeTestXPScaling(void) {
    EEAffinity base, a, b;
    int i, record[3];
    for (i = 0; i < 10000; ++i) {
        ee_affinity_create(&base, (unsigned)i, 1);
        if (base.rates[0] == 60)
            break;
    }
    if (i == 10000)
        return 1;
    a = base;
    a.xp_fraction = 37;
    if (!ee_affinity_award_hit_rate_scaled(&a, 0, 0) || a.total_xp || a.xp_fraction != 37)
        return 2;
    a = base;
    for (i = 0; i < 25; ++i)
        ee_affinity_award_hit_rate_scaled(&a, 0, 5);
    if (a.total_xp != 30 || a.xp_fraction)
        return 3;
    a = base;
    for (i = 0; i < 25; ++i)
        ee_affinity_award_hit_rate_scaled(&a, 0, 10);
    if (a.total_xp != 60 || a.xp_fraction)
        return 4;
    a = base;
    for (i = 0; i < 25; ++i)
        ee_affinity_award_hit_rate_scaled(&a, 0, 20);
    if (a.total_xp != 120 || a.xp_fraction)
        return 5;
    a = base;
    for (i = 0; i < 25; ++i)
        ee_affinity_award_hit_rate_scaled(&a, 1, 20);
    if (a.total_xp != 200 || a.rates[0] != 60)
        return 6;
    a = base;
    ee_affinity_award_hit_rate_scaled(&a, 0, 5);
    if (a.total_xp != 1 || a.xp_fraction != 20 || !ee_affinity_encode(&a, record) ||
        !ee_affinity_decode(&b, record) || b.total_xp != a.total_xp || b.xp_fraction != a.xp_fraction)
        return 7;
    a = base;
    a.total_xp = 47599;
    ee_affinity_award_hit_rate_scaled(&a, 1, 200);
    if (a.total_xp != 47600 || a.xp_fraction)
        return 8;
    a = base;
    ee_affinity_select(&a, 0);
    if (ee_affinity_award_hit_rate_scaled(&a, 1, 200) || a.total_xp)
        return 9;
    a = base;
    for (i = 0; i < 25; ++i)
        ee_affinity_award_hit_rate_scaled(&a, 0, 1);
    if (a.total_xp != 6 || a.xp_fraction)
        return 10;
    a = base;
    for (i = 0; i < 25; ++i)
        ee_affinity_award_hit_rate_scaled(&a, 0, 200);
    if (a.total_xp != 1200 || a.xp_fraction)
        return 11;
    return 0;
}
__declspec(dllexport) int probeTestEffectMultiplier(void) {
    EEAffinity a;
    EEDamage original = {30, 10, 0, 0, 1, 0}, zero, normal, five, half;
    int i, j;
    for (i = 1; i <= 5; ++i) {
        if (!ee_affinity_create(&a, 42, (unsigned int)i))
            return 1;
        a.total_xp = 47600;
        zero = normal = five = original;
        if (ee_affinity_damage_scaled(&a, &zero, 0))
            return 2;
        if (ee_affinity_damage(&a, &normal) != ee_affinity_damage_scaled(&a, &five, 5))
            return 3;
        half = original;
        ee_affinity_damage_scaled(&a, &half, 0.5f);
        {
            float *o = (float *)&original;
            float *z = (float *)&zero;
            float *n = (float *)&normal;
            float *f = (float *)&five;
            for (j = 0; j < 6; ++j) {
                float diff = f[j] - (o[j] + 5.0f * (n[j] - o[j]));
                float hd = ((float *)&half)[j] - (o[j] + 0.5f * (n[j] - o[j]));
                if (z[j] != o[j] || diff < -.001f || diff > .001f || hd < -.001f || hd > .001f)
                    return 4;
            }
        }
        {
            float n = ee_affinity_attack_speed(&a, 1),
                  f = ee_affinity_attack_speed_scaled(&a, 1, 5), expected = 1 + 5 * (n - 1);
            if (ee_affinity_attack_speed_scaled(&a, 1, 0) != 1 || f < expected - .001f ||
                f > expected + .001f)
                return 5;
            n = ee_affinity_combat_speed(&a, 1);
            f = ee_affinity_combat_speed_scaled(&a, 1, 5);
            expected = 1 + 5 * (n - 1);
            if (ee_affinity_combat_speed_scaled(&a, 1, 0) != 1 || f < expected - .001f ||
                f > expected + .001f)
                return 6;
        }
    }
    ee_affinity_create(&a, 42, 3);
    a.total_xp = 47600;
    if (!ee_affinity_select_secondary(&a, 2))
        return 7;
    if (ee_affinity_attack_speed_scaled(&a, 1, 5) < 1.687f ||
        ee_affinity_attack_speed_scaled(&a, 1, 5) > 1.688f)
        return 8;
    if (ee_affinity_attack_speed_scaled(&a, 1, 0.5f) < 1.068f ||
        ee_affinity_attack_speed_scaled(&a, 1, 0.5f) > 1.069f)
        return 10;
    a.total_xp = 0;
    if (!ee_affinity_award_hit(&a) || !a.total_xp)
        return 9;
    if (parse_multiplier_tenths("0.5", 50) != 5 || parse_multiplier_tenths("1", 50) != 10 ||
        parse_multiplier_tenths("1.5", 50) != 15 || parse_multiplier_tenths("5", 50) != 50 ||
        parse_multiplier_tenths("5.5", 50) != 50 || parse_multiplier_tenths("20.0", 200) != 200 ||
        parse_multiplier_tenths("0.1", 200) != 1)
        return 11;
    return 0;
}
static int test_mcm_int_seen, test_mcm_toggle_count, test_mcm_bad;
static void __cdecl test_mcm_begin(
    MCM_Api *a, const char *id, const char *title, const char *path, void(__cdecl *apply)(void)) {
    (void)a;
    if (!id || !title || !path || !apply)
        test_mcm_bad = 1;
}
static void __cdecl test_mcm_section(MCM_Api *a, const char *title) {
    (void)a;
    if (!title || !*title)
        test_mcm_bad = 1;
}
static void __cdecl test_mcm_toggle(MCM_Api *a,
                                    const char *section,
                                    const char *key,
                                    const char *label,
                                    const char *tip,
                                    int def,
                                    unsigned flags) {
    (void)a;
    (void)section;
    (void)key;
    (void)flags;
    if (!label || !*label || !tip || !*tip || def != (test_mcm_toggle_count ? 1 : 0))
        test_mcm_bad = 1;
    ++test_mcm_toggle_count;
}
static void __cdecl test_mcm_number(MCM_Api *a,
                                    const char *section,
                                    const char *key,
                                    const char *label,
                                    const char *tip,
                                    float def,
                                    float min,
                                    float max,
                                    int decimals,
                                    unsigned flags) {
    float expected_min = test_mcm_int_seen == 2 ? 0.8f : 0.0f;
    float expected_max = test_mcm_int_seen == 0 ? 20.0f : test_mcm_int_seen == 1 ? 5.0f : 2.0f;
    (void)a;
    (void)section;
    (void)key;
    (void)flags;
    if (!label || !*label || !tip || !*tip || def != 1.0f || min != expected_min ||
        max != expected_max || decimals != 1)
        test_mcm_bad = 1;
    ++test_mcm_int_seen;
}
__declspec(dllexport) int probeTestMcmRegistration(void) {
    MCM_Api api = {1, 0, test_mcm_begin, test_mcm_section, test_mcm_toggle, 0, test_mcm_number, 0};
    char old = settings_path[0];
    test_mcm_bad = test_mcm_int_seen = test_mcm_toggle_count = 0;
    settings_path[0] = 'x';
    MCM_Describe(&api);
    settings_path[0] = old;
    return test_mcm_bad || test_mcm_int_seen != 3 || test_mcm_toggle_count != 3;
}
static Vec *(*test_node_position)(void *, Vec *);
static float (*test_box_height)(void *);
static int test_free_calls;
static void test_free_original(void *skeleton, void *value) {
    (void)skeleton;
    (void)value;
    ++test_free_calls;
}
static int test_character_calls, test_player_flag;
static void test_character_original(void *c) {
    (void)c;
    ++test_character_calls;
}
static unsigned char test_is_player(void *c) {
    (void)c;
    return (unsigned char)test_player_flag;
}
static int test_weapon_kind = -1, test_ranged_mode;
static void *test_current_weapon(void *c) {
    (void)c;
    return test_weapon_kind < 0 ? 0 : (void *)11;
}
static int test_weapon_category(void *w) {
    (void)w;
    return test_weapon_kind;
}
static unsigned char test_combat_mode(void *c, unsigned char melee, unsigned char ranged) {
    (void)c;
    (void)melee;
    return (unsigned char)(ranged && test_ranged_mode);
}
static void test_melee_api(void) {
    character_current_weapon = test_current_weapon;
    weapon_category = test_weapon_category;
    character_combat = test_combat_mode;
    test_weapon_kind = -1;
    test_ranged_mode = 0;
}
static unsigned int test_handle_values[8];
static const void *test_handle(void *c) {
    (void)c;
    return test_handle_values;
}
__declspec(dllexport) int probeTestBossProfileLookup(void) {
    union { U64 alignment; unsigned char bytes[0x50]; } character = {0};
    union { U64 alignment; unsigned char bytes[0x80]; } data = {0};
    String *id = (String *)(data.bytes + 0x58);
    int profile;
    character_handle = test_handle;
    test_handle_values[5] = 4321;
    test_handle_values[6] = 7;
    *(void **)(character.bytes + 0x40) = data.bytes;
    if (!ee_game_string_init(id, "17225-Dialogue.mod", 18, 18))
        return 1;
    profile = boss_profile_for_character(character.bytes);
    if (profile != 1 || boss_profile_for_character(character.bytes) != profile)
        return 2;
    test_handle_values[6] = 8;
    if (!ee_game_string_init(id, "unknown.mod", 11, 15))
        return 3;
    if (boss_profile_for_character(character.bytes) != 0)
        return 4;
    return 0;
}
#ifdef EE_ENABLE_PERSISTENCE
static void *test_stats(void *c) {
    (void)c;
    return 0;
}
static void *test_hit_stats(void *c) {
    (void)c;
    return (void *)123;
}
__declspec(dllexport) int probeTestAllyHistoryFilter(void) {
    void *a = (void *)0x1040;
    void *b;
    unsigned int i, bit;
    if (tracked_actor_seen_maybe(a))
        return 1;
    if (!track_actor(a, (void *)0x2000, 1, 0, 0, 0, 1) || !tracked_actor_seen_maybe(a))
        return 2;
    if (track_actor(a, 0, 0, 0, 0, 0, 0) || !tracked_actor_seen_maybe(a))
        return 3;
    if (!track_actor(a, (void *)0x2000, 1, 0, 0, 0, 1))
        return 4;
    bit = tracked_actor_seen_bit(a);
    for (i = 0x3000; i < 0x1000000; i += 16)
        if (tracked_actor_seen_bit((void *)(U64)i) == bit)
            break;
    if (i >= 0x1000000)
        return 5;
    b = (void *)(U64)i;
    if (!tracked_actor_seen_maybe(b) || track_actor(b, 0, 0, 0, 0, 0, 0))
        return 6;
    if (!find_tracked_actor_by_body((void *)0x2000))
        return 7;
    return 0;
}
static int test_hit_result;
static float test_seen_cut;
static int
test_hit_original(void *target, int dir, void *dmg, void *attacker, void *tech, int combo) {
    (void)target;
    (void)dir;
    (void)attacker;
    (void)tech;
    (void)combo;
    if (dmg)
        test_seen_cut = ((EEDamage *)dmg)->cut;
    return test_hit_result;
}
__declspec(dllexport) int probeTestBossCombatAffinity(void) {
    unsigned int slot;
    EEAffinity a;
    EEDamage d = {10, 10, 0, 0, 0, 0};
    ee_persistence_init(&persistence);
    character_is_player = test_is_player;
    character_handle = test_handle;
    test_player_flag = 0;
    test_handle_values[5] = 188;
    test_handle_values[6] = 7;
    slot = (188u ^ (7u * 2654435761u)) & 4095u;
    boss_cache[slot].character = (void *)9;
    boss_cache[slot].index = 188;
    boss_cache[slot].generation = 7;
    boss_cache[slot].boss_profile = 1;
    boss_cache[(slot + 1) & 4095u].character = (void *)10;
    boss_cache[(slot + 1) & 4095u].index = 188;
    boss_cache[(slot + 1) & 4095u].generation = 7;
    boss_cache[(slot + 1) & 4095u].boss_profile = 0;
    if (!combat_affinity_locked((void *)9, 0, &a) || a.selected != 1 || a.secondary != 2 ||
        ee_affinity_level(&a) != 80)
        return 1;
    if (!ee_affinity_damage(&a, &d) || d.blunt <= 10.0f || d.armour <= 0.0f)
        return 2;
    if (combat_affinity_locked((void *)10, 0, &a))
        return 3;
    boss_cache[slot].character = 0;
    boss_cache[(slot + 1) & 4095u].character = 0;
    return 0;
}
__declspec(dllexport) int probeTestHitXP(void) {
    EECharacterRecord *r;
    unsigned int old;
    int i;
    EEDamage d = {10, 10, 0, 0, 1, 0};
    ee_persistence_init(&persistence);
    character_is_player = test_is_player;
    character_handle = test_handle;
    character_stats = test_hit_stats;
    original_character_hit = test_hit_original;
    test_melee_api();
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    test_player_flag = 1;
    active = 1;
    r = ee_persistence_bind(&persistence, (void *)123, (void *)9, 42, 7, 99, 4);
    if (!r)
        return 1;
    test_hit_result = 0;
    character_hit_hook((void *)8, 0, 0, (void *)9, 0, 0);
    test_hit_result = 5;
    character_hit_hook((void *)8, 0, 0, (void *)9, 0, 0);
    if (r->affinity.total_xp)
        return 2;
    test_hit_result = 2;
    for (i = 0; i < 10; ++i)
        character_hit_hook((void *)8, 0, 0, (void *)9, 0, 0);
    old = r->affinity.total_xp;
    if (!old)
        return 3;
    character_hit_hook((void *)8, 0, &d, (void *)9, 0, 0);
    if (test_seen_cut <= 10.0f || d.cut != 10.0f)
        return 5;
    old = r->affinity.total_xp;
    test_weapon_kind = 6;
    test_seen_cut = 0;
    character_hit_hook((void *)8, 0, &d, (void *)9, 0, 0);
    if (test_seen_cut != 10.0f || d.cut != 10.0f || r->affinity.total_xp != old)
        return 6;
    test_weapon_kind = -1;
    test_player_flag = 0;
    character_hit_hook((void *)8, 0, 0, (void *)9, 0, 0);
    if (r->affinity.total_xp != old)
        return 4;
    active = 0;
    return 0;
}
__declspec(dllexport) int probeTestHookTargets(void) {
    return !distinct_hook_targets((void *)1, (void *)2) ||
           distinct_hook_targets((void *)1, (void *)1) || distinct_hook_targets(0, (void *)1);
}
static float test_wound_seen_cut, test_wound_seen_blunt, test_wound_seen_pierce;
static void *test_crossbow_wound_original(
    void *a, unsigned char b, int c, void *damage, int *d, void *e, int *f, void *g) {
    EEDamage *x = (EEDamage *)damage;
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    (void)f;
    (void)g;
    test_wound_seen_cut = x->cut;
    test_wound_seen_blunt = x->blunt;
    test_wound_seen_pierce = x->pierce;
    return (void *)1;
}
__declspec(dllexport) int probeTestXPScalingHitHooks(void) {
    EECharacterRecord *r;
    EEDamage d = {10, 10, 100, 0, 1, 0};
    unsigned int old;
    int fail = 0;
    ee_persistence_init(&persistence);
    character_is_player = test_is_player;
    character_handle = test_handle;
    character_stats = test_hit_stats;
    original_character_hit = test_hit_original;
    original_crossbow_wound = test_crossbow_wound_original;
    test_melee_api();
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    test_player_flag = 1;
    test_hit_result = 2;
    active = 1;
    all_aptitudes = 1;
    r = ee_persistence_bind(&persistence, (void *)123, (void *)9, 42, 7, 99, 4);
    if (!r)
        return 1;
    xp_tenths = 0;
    character_hit_hook((void *)8, 0, &d, (void *)9, 0, 0);
    if (r->affinity.total_xp || test_seen_cut <= 10.0f)
        fail = 2;
    xp_tenths = 20;
    character_hit_hook((void *)8, 0, &d, (void *)9, 0, 0);
    if (r->affinity.total_xp != 8)
        fail = 3;
    old = r->affinity.total_xp;
    crossbow_wound_hook((void *)8, 0, 0, &d, 0, (void *)9, 0, (void *)7);
    if (r->affinity.total_xp != old + 8)
        fail = 4;
    xp_tenths = 10;
    all_aptitudes = 0;
    active = 0;
    return fail;
}
__declspec(dllexport) int probeTestMultiplierHitHooks(void) {
    EECharacterRecord *r;
    EEDamage d = {30, 10, 0, 0, 1, 0};
    float one, five;
    int fail = 0;
    ee_persistence_init(&persistence);
    character_is_player = test_is_player;
    character_handle = test_handle;
    character_stats = test_hit_stats;
    original_character_hit = test_hit_original;
    original_crossbow_wound = test_crossbow_wound_original;
    test_melee_api();
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    test_player_flag = 1;
    test_hit_result = 2;
    active = 1;
    r = ee_persistence_bind(&persistence, (void *)123, (void *)9, 42, 7, 99, 4);
    if (!r)
        return 1;
    r->affinity.total_xp = 47600;
    effect_multiplier = 0;
    character_hit_hook((void *)8, 0, &d, (void *)9, 0, 0);
    if (test_seen_cut != 30)
        fail = 2;
    effect_multiplier = 1;
    character_hit_hook((void *)8, 0, &d, (void *)9, 0, 0);
    one = test_seen_cut;
    effect_multiplier = 5;
    character_hit_hook((void *)8, 0, &d, (void *)9, 0, 0);
    five = test_seen_cut;
    if (one < 35.99f || one > 36.01f || five < 59.99f || five > 60.01f)
        fail = 3;
    ee_affinity_select(&r->affinity, 1);
    d = (EEDamage){0, 0, 100, 0, 1, 0};
    effect_multiplier = 0;
    crossbow_wound_hook((void *)8, 0, 0, &d, 0, (void *)9, 0, (void *)7);
    if (test_wound_seen_blunt != 0)
        fail = 4;
    effect_multiplier = 1;
    crossbow_wound_hook((void *)8, 0, 0, &d, 0, (void *)9, 0, (void *)7);
    one = test_wound_seen_blunt;
    effect_multiplier = 5;
    crossbow_wound_hook((void *)8, 0, 0, &d, 0, (void *)9, 0, (void *)7);
    five = test_wound_seen_blunt;
    if (one < 14.99f || one > 15.01f || five < 74.99f || five > 75.01f)
        fail = 5;
    effect_multiplier = 1;
    active = 0;
    return fail;
}
__declspec(dllexport) int probeTestCrossbowWound(void) {
    EECharacterRecord *r;
    EEDamage d = {0, 0, 100, 0, 1, 0};
    unsigned int old;
    ee_persistence_init(&persistence);
    character_is_player = test_is_player;
    character_handle = test_handle;
    character_stats = test_hit_stats;
    original_crossbow_wound = test_crossbow_wound_original;
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    test_player_flag = 1;
    active = 1;
    all_aptitudes = 1;
    r = ee_persistence_bind(&persistence, (void *)123, (void *)9, 42, 7, 99, 1);
    if (!r)
        return 1;
    r->affinity.total_xp = 0;
    crossbow_wound_hook((void *)8, 0, 0, &d, 0, (void *)9, 0, (void *)7);
    if (test_wound_seen_blunt < 4.99f || test_wound_seen_blunt > 5.01f ||
        test_wound_seen_pierce != 100 || d.blunt != 0 || r->affinity.total_xp != 4)
        return 2;
    old = r->affinity.total_xp;
    test_player_flag = 0;
    crossbow_wound_hook((void *)8, 0, 0, &d, 0, (void *)9, 0, (void *)7);
    if (test_wound_seen_blunt != 0 || r->affinity.total_xp != old)
        return 3;
    test_player_flag = 1;
    crossbow_wound_hook((void *)8, 0, 0, &d, 0, (void *)9, 0, 0);
    if (test_wound_seen_blunt != 0 || r->affinity.total_xp != old)
        return 4;
    active = 0;
    all_aptitudes = 0;
    return 0;
}
static void test_cb_trace_original(void *projectile, const Vec *p, const Vec *d) {
    (void)projectile;
    (void)p;
    (void)d;
}
static void test_cb_update_original(void *ranged, float dt, const void *p, const void *d) {
    (void)ranged;
    (void)dt;
    (void)p;
    (void)d;
}
static Vec *test_cb_barrel(void *gun, Vec *out) {
    (void)gun;
    *out = (Vec){4, 5, 6};
    return out;
}
__declspec(dllexport) int probeTestCrossbowCapture(void) {
    static unsigned char bolt[128], ranged[160];
    Vec pos = {1, 2, 3}, direction = {0, 1, 0};
    int i;
    void *actor = (void *)9;
    for (i = 0; i < 128; ++i)
        ((volatile unsigned char *)bolt)[i] = 0;
    for (i = 0; i < 160; ++i)
        ((volatile unsigned char *)ranged)[i] = 0;
    for (i = 0; i < CB_LIMIT; ++i) {
        cb_charges[i] = (CBCharge){0};
        cb_reset_flight(&cb_flights[i]);
    }
    for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
        tracked_actors[i].character = 0;
    test_player_flag = 1;
    character_is_player = test_is_player;
    character_handle = test_handle;
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    original_projectile_trace = test_cb_trace_original;
    original_ranged_update = test_cb_update_original;
    cb_barrel = test_cb_barrel;
    active = 1;
    tracked_actors[0].character = actor;
    tracked_actors[0].element = 1;
    tracked_actors[0].secondary = 2;
    *(unsigned *)(bolt + 0x34) = 42;
    *(unsigned *)(bolt + 0x38) = 7;
    *(void **)(bolt + 0x58) = (void *)11;
    cb_trace_hook(bolt, &pos, &direction);
    if (cb_flights[0].projectile != bolt || cb_flights[0].element != 1 ||
        cb_flights[0].secondary != 2)
        return 1;
    clear_crossbow_projectile(bolt);
    if (cb_flights[0].projectile)
        return 2;
    *(unsigned *)(bolt + 0x34) = 99;
    cb_trace_hook(bolt, &pos, &direction);
    if (cb_flights[0].projectile)
        return 3;
    *(void **)(ranged + 0x68) = actor;
    *(void **)(ranged + 0x28) = (void *)12;
    *(int *)ranged = 2;
    *(float *)(ranged + 0x74) = 6;
    *(float *)(ranged + 0x78) = 3;
    cb_update_hook(ranged, 0, 0, 0);
    if (cb_charges[0].character != actor || cb_charges[0].progress < .499f ||
        cb_charges[0].progress > .501f || cb_charges[0].position.z != 6)
        return 4;
    clear_crossbow_character(actor);
    if (cb_charges[0].character)
        return 5;
    test_player_flag = 0;
    cb_update_hook(ranged, 0, 0, 0);
    if (cb_charges[0].character)
        return 6;
    active = 0;
    return 0;
}
__declspec(dllexport) int probeTestCrossbowCoordinates(void) {
    static CBFlight f;
    Vec p;
    cb_reset_flight(&f);
    f.origin = (Vec){-94584, 344, -43728};
    f.has_origin = 1;
    p = cb_align_position(&f, (Vec){-2140, 343, 47});
    if (p.x != -94584 || p.y != 344 || p.z != -43728)
        return 1;
    p = cb_align_position(&f, (Vec){-2316, 334, -52});
    if (p.x != -94760 || p.y != 335 || p.z != -43827)
        return 2;
    return 0;
}
static unsigned char test_cb_shot_bolt[128], test_cb_shot_bolt2[128];
static unsigned char *test_cb_active_bolt = test_cb_shot_bolt;
static void *test_cb_create_original(void *manager, const void *mesh, const void *material) {
    (void)manager;
    (void)mesh;
    (void)material;
    return test_cb_active_bolt;
}
static void test_cb_shoot_original(void *gun, void *actor, void *target, int stat, const Vec *aim) {
    (void)gun;
    (void)actor;
    (void)target;
    (void)stat;
    (void)aim;
    cb_create_hook(0, 0, 0);
    *(float *)(test_cb_active_bolt + 0x44) = 500.0f;
}
__declspec(dllexport) int probeTestSetLogPath(const char *path) {
    unsigned i;
    if (!path)
        return 0;
    for (i = 0; i < sizeof(log_path) - 1 && path[i]; ++i)
        log_path[i] = path[i];
    log_path[i] = 0;
    return path[i] == 0;
}
__declspec(dllexport) int probeTestLogThrottle(void) {
    static const char msg[] = "TEST ERROR\r\n";
    logline(msg);
    logline(msg);
    return 0;
}
__declspec(dllexport) int probeTestWindReload(void) {
    EEAffinity a;
    float rate;
    if (!ee_affinity_create(&a, 1, 3))
        return 1;
    a.total_xp = 47600;
    rate = ee_affinity_combat_speed(&a, 1.0f);
    if (rate < 1.099f || rate > 1.101f || cb_reload_adjust(3.0f, .1f, rate) > 2.991f ||
        cb_reload_adjust(3.0f, .1f, rate) < 2.989f)
        return 2;
    a.selected = 1;
    a.secondary = 3;
    rate = ee_affinity_combat_speed(&a, 1.0f);
    if (rate < 1.074f || rate > 1.076f)
        return 3;
    a.secondary = 0;
    if (cb_reload_adjust(3.0f, .1f, ee_affinity_combat_speed(&a, 1.0f)) != 3.0f)
        return 4;
    if (cb_reload_adjust(.001f, .1f, 1.1f) != 0.0f || cb_reload_adjust(3.0f, -1.0f, 1.1f) != 3.0f)
        return 5;
    return 0;
}
__declspec(dllexport) int probeTestWindReloadIntegration(void) {
    static unsigned char ranged[160];
    EECharacterRecord *r;
    int i;
    for (i = 0; i < 160; ++i)
        ((volatile unsigned char *)ranged)[i] = 0;
    ee_persistence_init(&persistence);
    character_is_player = test_is_player;
    character_handle = test_handle;
    character_stats = test_hit_stats;
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    test_player_flag = 1;
    active = 1;
    for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
        tracked_actors[i].character = 0;
    r = ee_persistence_bind(&persistence, (void *)123, (void *)9, 42, 7, 99, 3);
    if (!r)
        return 1;
    r->affinity.total_xp = 47600;
    tracked_actors[0].character = (void *)9;
    tracked_actors[0].element = 3;
    *(void **)(ranged + 0x68) = (void *)9;
    *(int *)ranged = 2;
    *(float *)(ranged + 0x74) = 6.0f;
    *(float *)(ranged + 0x78) = 3.0f;
    cb_apply_wind_reload(ranged, .1f);
    if (*(float *)(ranged + 0x78) > 2.991f || *(float *)(ranged + 0x78) < 2.989f)
        return 2;
    *(float *)(ranged + 0x78) = 3.0f;
    r->affinity.selected = 1;
    tracked_actors[0].element = 1;
    cb_apply_wind_reload(ranged, .1f);
    if (*(float *)(ranged + 0x78) != 3.0f)
        return 3;
    tracked_actors[0].secondary = 3;
    r->affinity.secondary = 3;
    cb_apply_wind_reload(ranged, .1f);
    if (*(float *)(ranged + 0x78) > 2.993f || *(float *)(ranged + 0x78) < 2.992f)
        return 4;
    *(float *)(ranged + 0x78) = 3.0f;
    *(int *)ranged = 0;
    cb_apply_wind_reload(ranged, .1f);
    if (*(float *)(ranged + 0x78) != 3.0f)
        return 5;
    active = 0;
    return 0;
}
__declspec(dllexport) int probeTestCrossbowSpeed(void) {
    int i;
    EECharacterRecord *r;
    Vec aim = {0};
    for (i = 0; i < 128; ++i) {
        ((volatile unsigned char *)test_cb_shot_bolt)[i] = 0;
        ((volatile unsigned char *)test_cb_shot_bolt2)[i] = 0;
    }
    test_cb_active_bolt = test_cb_shot_bolt;
    for (i = 0; i < CB_LIMIT; ++i)
        cb_reset_flight(&cb_flights[i]);
    for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
        tracked_actors[i].character = 0;
    ee_persistence_init(&persistence);
    character_is_player = test_is_player;
    character_handle = test_handle;
    character_stats = test_hit_stats;
    original_projectile_trace = test_cb_trace_original;
    original_crossbow_shoot = test_cb_shoot_original;
    original_harpoon_create = test_cb_create_original;
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    test_player_flag = 1;
    active = 1;
    r = ee_persistence_bind(&persistence, (void *)123, (void *)9, 42, 7, 99, 3);
    if (!r)
        return 1;
    r->affinity.total_xp = 47600;
    tracked_actors[0].character = (void *)9;
    tracked_actors[0].element = 3;
    *(unsigned *)(test_cb_shot_bolt + 0x34) = 42;
    *(unsigned *)(test_cb_shot_bolt + 0x38) = 7;
    *(void **)(test_cb_shot_bolt + 0x58) = (void *)11;
    cb_shoot_hook((void *)12, (void *)9, 0, 0, &aim);
    if (cb_flights[0].projectile != test_cb_shot_bolt || cb_flights[0].element != 3)
        return 6;
    if (*(float *)(test_cb_shot_bolt + 0x44) < 549.9f ||
        *(float *)(test_cb_shot_bolt + 0x44) > 550.1f)
        return 2;
    clear_crossbow_projectile(test_cb_shot_bolt);
    test_player_flag = 0;
    cb_shoot_hook((void *)12, (void *)9, 0, 0, &aim);
    if (*(float *)(test_cb_shot_bolt + 0x44) != 500)
        return 3;
    clear_crossbow_projectile(test_cb_shot_bolt);
    test_player_flag = 1;
    r->affinity.selected = 1;
    tracked_actors[0].element = 1;
    cb_shoot_hook((void *)12, (void *)9, 0, 0, &aim);
    if (*(float *)(test_cb_shot_bolt + 0x44) != 500)
        return 4;
    test_cb_active_bolt = test_cb_shot_bolt2;
    *(unsigned *)(test_cb_shot_bolt2 + 0x34) = 42;
    *(unsigned *)(test_cb_shot_bolt2 + 0x38) = 7;
    *(void **)(test_cb_shot_bolt2 + 0x58) = (void *)11;
    r->affinity.selected = 3;
    tracked_actors[0].element = 3;
    cb_shoot_hook((void *)12, (void *)9, 0, 0, &aim);
    if (*(float *)(test_cb_shot_bolt + 0x44) != 500 ||
        *(float *)(test_cb_shot_bolt2 + 0x44) < 549.9f)
        return 5;
    clear_crossbow_projectile(test_cb_shot_bolt);
    clear_crossbow_projectile(test_cb_shot_bolt2);
    active = 0;
    return 0;
}
typedef struct TestLocale {
    U64 pad;
    String id;
} TestLocale;
static TestLocale test_locale;
static void *test_locale_manager(void) {
    return (void *)1;
}
static void *test_locale_current(void *manager) {
    (void)manager;
    return &test_locale;
}
__declspec(dllexport) int probeTestLocaleDetection(void) {
    const char *p;
    int i;
    const char *id = "ja_JP";
    test_locale.id.size = 5;
    test_locale.id.capacity = 15;
    for (i = 0; i <= 5; ++i)
        test_locale.id.text.small[i] = id[i];
    locale_manager_instance = test_locale_manager;
    locale_current = test_locale_current;
    ui_locale_ready = 0;
    ee_locale_set("en_GB");
    select_game_locale();
    p = ee_locale_get()->id;
    if (p[0] != 'j' || p[1] != 'a' || !ui_locale_ready)
        return 1;
    ee_locale_set("unsupported");
    p = ee_locale_get()->id;
    if (p[0] != 'e' || p[1] != 'n')
        return 2;
    ee_locale_set("en_GB");
    locale_manager_instance = 0;
    locale_current = 0;
    ui_locale_ready = 0;
    return 0;
}
__declspec(dllexport) const char *probeTestWindDetail(const char *id) {
    ee_locale_set(id);
    return ee_locale_get()->winddetail;
}
__declspec(dllexport) const char *probeTestLocaleText(const char *id, int field) {
    const EELocale *l;
    ee_locale_set(id);
    l = ee_locale_get();
    return field == 0   ? l->id
           : field == 1 ? l->element[1]
           : field == 2 ? l->armour
           : field == 3 ? l->blunt
           : field == 4 ? l->cut
                        : l->nextlevel;
}
static void test_set_handle_original(void *character, const void *next) {
    const unsigned int *h = (const unsigned int *)next;
    test_handle_values[5] = h[5];
    test_handle_values[6] = h[6];
    (void)ee_character_observe(&persistence.characters, character, h[5], h[6], 123, 5);
}
typedef struct EETestGameData {
    int value[3];
    int present;
} EETestGameData;
typedef struct EETestStats {
    U64 padding[2];
    void *character;
} EETestStats;
static float test_speed_original(void *stats) {
    (void)stats;
    return 1.0f;
}
static float test_speed_calculation(void *stats, float weight, float skill, unsigned char block) {
    (void)stats;
    (void)weight;
    (void)skill;
    (void)block;
    return 1.0f;
}
__declspec(dllexport) int probeTestWindSpeed(void) {
    EETestStats stats = {{0, 0}, (void *)9};
    EECharacterRecord *r;
    float x;
    ee_persistence_init(&persistence);
    character_is_player = test_is_player;
    character_handle = test_handle;
    original_attack_speed = test_speed_original;
    original_block_speed = test_speed_original;
    test_melee_api();
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    test_player_flag = 1;
    active = 1;
    r = ee_persistence_bind(&persistence, &stats, (void *)9, 42, 7, 123, 3);
    if (!r)
        return 1;
    x = attack_speed_hook(&stats);
    if (x < 1.019f || x > 1.021f)
        return 2;
    r->affinity.total_xp = 47600;
    x = block_speed_hook(&stats);
    if (x < 1.099f || x > 1.101f)
        return 3;
    original_calculate_combat_speed = test_speed_calculation;
    x = calculate_combat_speed_hook(&stats, 1.0f, 1.0f, 1);
    if (x < 1.099f || x > 1.101f)
        return 10;
    r->affinity.selected = 1;
    if (attack_speed_hook(&stats) != 1.0f)
        return 4;
    r->affinity.selected = 2;
    r->affinity.total_xp = 47600;
    x = attack_speed_hook(&stats);
    if (x < 1.049f || x > 1.051f || block_speed_hook(&stats) != 1.0f)
        return 6;
    test_weapon_kind = 6;
    if (attack_speed_hook(&stats) != 1.0f)
        return 7;
    test_weapon_kind = 8;
    test_ranged_mode = 1;
    if (attack_speed_hook(&stats) < 1.049f)
        return 8;
    test_weapon_kind = -1;
    if (attack_speed_hook(&stats) != 1.0f)
        return 9;
    test_ranged_mode = 0;
    r->affinity.selected = 3;
    test_player_flag = 0;
    if (block_speed_hook(&stats) != 1.0f)
        return 5;
    active = 0;
    return 0;
}
static int test_stats_calls;
static void test_stats_original(void *stats, void *data) {
    (void)stats;
    (void)data;
    ++test_stats_calls;
}
static void test_stats_destroy_original(void *stats) {
    (void)stats;
}
static unsigned char test_list_exists(void *data, const EEGameString *list) {
    (void)list;
    return (unsigned char)((EETestGameData *)data)->present;
}
static int test_list_size(void *data, const EEGameString *list) {
    (void)list;
    return ((EETestGameData *)data)->present;
}
static const EEGameString *test_list_name(void *data, const EEGameString *list, int index) {
    static const EEGameString name = {{"SB_Data"}, 7, 15, 0};
    (void)data;
    (void)list;
    (void)index;
    return &name;
}
static const EEGameString *test_unreadable_list_name(void *data, const EEGameString *list, int index) {
    (void)data;
    (void)list;
    (void)index;
    return 0;
}
static const int *test_list_values(void *data, const EEGameString *list, int index) {
    (void)list;
    (void)index;
    return ((EETestGameData *)data)->value;
}
static void *test_find_reference(void *data, const EEGameString *list, const EEGameString *item) {
    (void)list;
    (void)item;
    return ((EETestGameData *)data)->value;
}
static void test_add_reference(
    void *data, const EEGameString *list, const EEGameString *item, int a, int b, int c) {
    EETestGameData *d = (EETestGameData *)data;
    (void)list;
    (void)item;
    d->value[0] = a;
    d->value[1] = b;
    d->value[2] = c;
    d->present = 1;
}
__declspec(dllexport) int probeTestPersistenceHooks(void) {
    EETestGameData data = {{0, 0, 0}, 0};
    EETestStats stats = {{0, 0}, (void *)9};
    EECharacterRecord *r;
    unsigned char rates[5];
    int i;
    ee_persistence_init(&persistence);
    character_handle = test_handle;
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    gamedata_api.list_exists = test_list_exists;
    gamedata_api.list_size = test_list_size;
    gamedata_api.list_name = test_list_name;
    gamedata_api.list_values = test_list_values;
    gamedata_api.find_reference = test_find_reference;
    gamedata_api.add_reference = test_add_reference;
    original_stats_save = test_stats_original;
    original_stats_load = test_stats_original;
    original_stats_destroy = test_stats_destroy_original;
    test_stats_calls = 0;
    active = 1;
    r = ee_persistence_bind(&persistence, &stats, (void *)9, 42, 7, 123, 4);
    if (!r)
        return 1;
    for (i = 0; i < 5; ++i)
        rates[i] = r->affinity.rates[i];
    stats_save_hook(&stats, &data);
    if (!data.present || ((data.value[0] >> 8) & 255) != 4 || (data.value[0] & 7) != 4 ||
        test_stats_calls != 1)
        return 2;
    if (!ee_character_select(&persistence.characters, (void *)9, 42, 7, 2))
        return 3;
    stats_save_hook(&stats, &data);
    if ((data.value[0] & 7) != 2 || test_stats_calls != 2)
        return 4;
    ee_persistence_forget(&persistence, &stats, (void *)9);
    stats_load_hook(&stats, &data);
    if (test_stats_calls != 3)
        return 5;
    r = ee_persistence_bind(&persistence, &stats, (void *)10, 42, 7, 999, 1);
    if (!r || r->affinity.selected != 2)
        return 6;
    for (i = 0; i < 5; ++i)
        if (r->affinity.rates[i] != rates[i])
            return 7;
    {
        unsigned int next[8] = {0};
        next[5] = 84;
        next[6] = 8;
        original_character_set_handle = test_set_handle_original;
        character_set_handle_hook((void *)10, next);
        r = ee_character_observe(&persistence.characters, (void *)10, 84, 8, 7, 5);
        if (!r || r->affinity.selected != 2)
            return 8;
    }
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    stats_destroy_hook(&stats);
    ee_persistence_forget(&persistence, 0, (void *)10);
    {
        int original_meta = data.value[0];
        ee_persistence_init(&persistence);
        gamedata_api.list_name = test_unreadable_list_name;
        stats_load_hook(&stats, &data);
        r = ee_persistence_bind(&persistence, &stats, (void *)9, 42, 7, 999, 1);
        if (!r || !r->read_only)
            return 9;
        stats_save_hook(&stats, &data);
        if (data.value[0] != original_meta)
            return 10;
        gamedata_api.list_name = test_list_name;
    }
    active = 0;
    return 0;
}
#endif
static unsigned char test_no_combat(void *c, unsigned char a, unsigned char b) {
    (void)c;
    (void)a;
    (void)b;
    return 0;
}
static void *test_appearance(void *c) {
    (void)c;
    return (void *)2;
}
static void *test_body(void *a) {
    (void)a;
    return (void *)3;
}
__declspec(dllexport) int probeTestIdentity(void) {
    test_melee_api();
    original_character_update = test_character_original;
    original_character_destroy = test_character_original;
    character_is_player = test_is_player;
    character_appearance = test_appearance;
    appearance_body = test_body;
    character_handle = test_handle;
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
#ifdef EE_ENABLE_PERSISTENCE
    character_stats = test_stats;
    ee_persistence_init(&persistence);
#endif
    character_combat = test_no_combat;
    test_character_calls = 0;
    test_player_flag = 1;
    active = 1;
    character_update_hook((void *)1);
    if (!is_tracked_actor_body((void *)3) || test_character_calls != 1)
        return 1;
    test_player_flag = 0;
    character_update_hook((void *)1);
    if (is_tracked_actor_body((void *)3) || test_character_calls != 2)
        return 2;
    test_player_flag = 1;
    character_update_hook((void *)1);
    character_destroy_hook((void *)1);
    if (is_tracked_actor_body((void *)3) || test_character_calls != 4)
        return 3;
    active = 0;
    return 0;
}
__declspec(dllexport) int probeTestAptitudeOverride(void) {
    EEAffinity a, b;
    int i, record[3];
    if (!ee_affinity_create(&a, 7, 1))
        return 1;
    a.rates[0] = 60;
    a.rates[1] = 60;
    a.rates[2] = 80;
    a.rates[3] = 80;
    a.rates[4] = 100;
    for (i = 0; i < 25; ++i)
        if (!ee_affinity_award_hit_rate(&a, 1))
            return 2;
    if (a.total_xp != 100 || a.rates[0] != 60 || !ee_affinity_encode(&a, record) ||
        !ee_affinity_decode(&b, record) || b.rates[0] != 60)
        return 3;
    for (i = 0; i < 25; ++i)
        ee_affinity_award_hit_rate(&b, 0);
    return b.total_xp == 160 ? 0 : 4;
}
__declspec(dllexport) int probeTestNewGameAptitude(void) {
    static EECharacters chars;
    unsigned int counts[5] = {0}, i, j;
    EECharacterRecord *r;
    ee_characters_init(&chars);
    ee_characters_set_seed_provider(fresh_aptitude_seed);
    for (i = 0; i < 1000; ++i) {
        r = ee_character_observe(&chars, (void *)1, 10, 20, 99, 0);
        if (!r || r->affinity.selected != 0)
            return 1;
        for (j = 0; j < 5; ++j)
            if (r->affinity.rates[j] == 100)
                ++counts[j];
        ee_character_forget(&chars, (void *)1);
    }
    ee_characters_set_seed_provider(0);
    for (i = 0; i < 5; ++i)
        if (counts[i] < 80 || counts[i] > 330)
            return 2;
    return 0;
}
__declspec(dllexport) int probeTestAffinity(void) {
    unsigned int i, counts[6] = {0};
    for (i = 1; i < 1000; ++i) {
        unsigned int a = affinity_seed(i, 7);
        if (a != affinity_seed(i, 7) || a < 1 || a > 5)
            return 1;
        ++counts[a];
    }
    for (i = 1; i <= 5; ++i)
        if (counts[i] < 120)
            return 2;
    return 0;
}
__declspec(dllexport) int
probeTestFrostSetup(int slot, void *owner, void *blade, void *scene, int bone, void *tag) {
    MODULE ogre = GetModuleHandleA("OgreMain_x64.dll");
    EEWeaponVisual *e;
    if (!ogre || slot < 0 || slot >= 2 || !owner || !blade || !scene)
        return 0;
    e = &weapon_visuals[slot];
    *(void **)(&entity_mesh) =
        GetProcAddress(ogre, "?getMesh@Entity@Ogre@@QEBAAEBV?$SharedPtr@VMesh@Ogre@@@2@XZ");
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
    *(void **)(&parent_node) =
        GetProcAddress(ogre, "?getParentSceneNode@MovableObject@Ogre@@QEBAPEAVSceneNode@2@XZ");
    *(void **)(&tag_position) =
        GetProcAddress(ogre, "?_getDerivedPosition@OldNode@Ogre@@UEBAAEBVVector3@2@XZ");
    *(void **)(&tag_rotation) =
        GetProcAddress(ogre, "?_getDerivedOrientation@OldNode@Ogre@@UEBAAEBVQuaternion@2@XZ");
    *(void **)(&node_create) = GetProcAddress(
        ogre, "?createSceneNode@SceneManager@Ogre@@UEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z");
    *(void **)(&node_destroy) =
        GetProcAddress(ogre, "?destroySceneNode@SceneManager@Ogre@@UEAAXPEAVSceneNode@2@@Z");
    *(void **)(&node_add) = GetProcAddress(ogre, "?addChild@Node@Ogre@@QEAAXPEAV12@@Z");
    *(void **)(&node_position) =
        GetProcAddress(ogre, "?setPosition@Node@Ogre@@QEAAXAEBVVector3@2@@Z");
    *(void **)(&node_rotation) = GetProcAddress(ogre, "?setOrientation@Node@Ogre@@QEAAXMMMM@Z");
    *(void **)(&node_attach) =
        GetProcAddress(ogre, "?attachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    *(void **)(&original_scene_detach) =
        GetProcAddress(ogre, "?detachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    *(void **)(&original_attach) = GetProcAddress(
        ogre,
        "?attachObjectToBone@Entity@Ogre@@QEAAPEAVTagPoint@2@AEBV?$basic_string@DU?$char_traits@D@"
        "std@@V?$allocator@D@2@@std@@PEAVMovableObject@2@AEBVQuaternion@2@AEBVVector3@2@@Z");
    *(void **)(&original_detach) =
        GetProcAddress(ogre, "?detachObjectFromBone@Entity@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    *(void **)(&original_destroy_entity) =
        GetProcAddress(ogre, "?destroyEntity@SceneManager@Ogre@@UEAAXPEAVEntity@2@@Z");
    if (!entity_mesh || !frost_entity_create || !frost_entity_material || !frost_get_visible ||
        !frost_set_visible || !frost_cast_shadows || !parent_node || !tag_position ||
        !tag_rotation || !node_create || !node_destroy || !node_add || !node_position ||
        !node_rotation || !node_attach || !original_scene_detach || !original_destroy_entity)
        return 0;
    e->owner = owner;
    e->blade = blade;
    e->scene = scene;
    e->parent = parent_node(owner);
    e->tag = tag;
    e->frost_bone = bone;
    return e->parent && e->tag;
}
__declspec(dllexport) int probeTestFrostStep(int slot, int wanted) {
    if (slot < 0 || slot >= 2)
        return -1;
    frost_update(&weapon_visuals[slot], wanted);
    return weapon_visuals[slot].frost && weapon_visuals[slot].frost_attached;
}
__declspec(dllexport) int probeTestFrostVisible(int slot) {
    if (slot < 0 || slot >= 2 || !weapon_visuals[slot].frost || !frost_get_visible)
        return -1;
    return frost_get_visible(weapon_visuals[slot].frost);
}
__declspec(dllexport) void *probeTestFrostObject(int slot) {
    return slot >= 0 && slot < 2 ? weapon_visuals[slot].frost : 0;
}
__declspec(dllexport) void *probeTestFrostNode(int slot) {
    return slot >= 0 && slot < 2 ? weapon_visuals[slot].frost_node : 0;
}
__declspec(dllexport) int probeTestInit(void *owner, void *blade, void *tag) {
    MODULE ogre = GetModuleHandleA("OgreMain_x64.dll"),
           pu = GetModuleHandleA("Plugin_ParticleUniverse_x64.dll");
    int ok = 1;
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
    API(particle_local, pu, "?setKeepLocal@ParticleSystem@ParticleUniverse@@QEAAX_N@Z");
    API(particle_count,
        pu,
        "?getNumberOfEmittedParticles@ParticleSystem@ParticleUniverse@@QEAA_KXZ");
    API(particle_bounds, pu, "?setBoundsAutoUpdated@ParticleSystem@ParticleUniverse@@QEAAX_NM@Z");
    API(particle_technique,
        pu,
        "?getTechnique@ParticleSystem@ParticleUniverse@@QEBAPEAVParticleTechnique@2@_K@Z");
    API(technique_emitter,
        pu,
        "?getEmitter@ParticleTechnique@ParticleUniverse@@QEBAPEAVParticleEmitter@2@_K@Z");
    API(box_height, pu, "?setHeight@BoxEmitter@ParticleUniverse@@QEAAXM@Z");
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
    API(test_node_position, ogre, "?getPosition@Node@Ogre@@QEBA?AVVector3@2@XZ");
    API(test_box_height, pu, "?getHeight@BoxEmitter@ParticleUniverse@@QEBA?BMXZ");
#ifdef EE_ENABLE_TRAIL
    *(void **)(&original_scene_detach) =
        GetProcAddress(ogre, "?detachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    if (!original_scene_detach || !trail_resolve(ogre))
        ok = 0;
#endif
    *(void **)(&cb_normalise_vec) = GetProcAddress(ogre, "?normalise@Vector3@Ogre@@QEAAMXZ");
    *(void **)(&cb_normalise_quat) = GetProcAddress(ogre, "?normalise@Quaternion@Ogre@@QEAAMXZ");
    *(void **)(&cb_particle_scale) =
        GetProcAddress(pu, "?setScale@ParticleSystem@ParticleUniverse@@QEAAXAEBVVector3@Ogre@@@Z");
    if (!cb_normalise_vec || !cb_normalise_quat)
        ok = 0;
    if (!ok)
        return 0;
    weapon_visuals[0].owner = owner;
    weapon_visuals[0].blade = blade;
    weapon_visuals[0].tag = tag;
    weapon_visuals[0].born = GetTickCount64();
    weapon_visuals[0].length = 10.0f;
    track_actor((void *)1, owner, 1, 0, 0, 0, 1);
    tracked_actors[0].melee_weapon = 1;
    return 1;
}
__declspec(dllexport) int probeTestCBNativeStart(void *source) {
    if (!source || !tracked_actors[0].character || !trail_derive)
        return 0;
    cb_clear_flight(&cb_flights[0]);
    cb_clear_charge(&cb_charges[0]);
    cb_reset_flight(&cb_flights[0]);
    cb_flights[0].projectile = (void *)21;
    cb_flights[0].source = source;
    cb_flights[0].born = GetTickCount64();
    cb_flights[0].element = 1;
    cb_flights[0].secondary = 2;
    cb_charges[0] = (CBCharge){0};
    cb_charges[0].character = tracked_actors[0].character;
    cb_charges[0].position = (Vec){1, 2, 3};
    cb_charges[0].progress = .5f;
    cb_charges[0].sample = GetTickCount64();
    return 1;
}
__declspec(dllexport) void probeTestCBNativeElement(int element, int secondary) {
    tracked_actors[0].element = element;
    tracked_actors[0].secondary = secondary;
    cb_flights[0].element = element;
    cb_flights[0].secondary = secondary;
}
__declspec(dllexport) void probeTestCBPreviewCharge(float progress, float x, float y, float z) {
    cb_charges[0].character = tracked_actors[0].character;
    cb_charges[0].position = (Vec){x, y, z};
    cb_charges[0].progress = progress;
    cb_charges[0].sample = GetTickCount64();
}
__declspec(dllexport) void probeTestCBPreviewRotation(float w, float x, float y, float z) {
    cb_charges[0].rotation = (Quat){w, x, y, z};
}
__declspec(dllexport) void probeTestCBPreviewStopCharge(void) {
    cb_clear_charge(&cb_charges[0]);
}
__declspec(dllexport) void probeTestCBPreviewStopFlight(void) {
    cb_clear_flight(&cb_flights[0]);
}
__declspec(dllexport) int probeTestCBNativeTick(float dt) {
    cb_charges[0].sample = GetTickCount64();
    update(dt);
    return cb_flights[0].fx && cb_charges[0].fx ? 1 : 0;
}
__declspec(dllexport) int probeTestCBNativeCount(int index) {
    void *p = index == 0   ? cb_flights[0].fx
              : index == 1 ? cb_charges[0].fx
              : index == 2 ? cb_flights[0].fx2
                           : cb_charges[0].fx2;
    return p ? (int)particle_count(p) : -1;
}
__declspec(dllexport) void probeTestCBNativeStop(void) {
    cb_clear_flight(&cb_flights[0]);
    cb_clear_charge(&cb_charges[0]);
}
__declspec(dllexport) void probeTestAlly(int enabled) {
    track_actor((void *)1, enabled ? weapon_visuals[0].owner : 0, enabled, 0, 0, 0, 1);
    if (enabled)
        tracked_actors[0].melee_weapon = 1;
}
__declspec(dllexport) void probeTestMeleeVisual(int enabled) {
    if (tracked_actors[0].character)
        tracked_actors[0].melee_weapon = enabled;
}
__declspec(dllexport) int probeTestMeleeContext(int category, int ranged) {
    test_melee_api();
    test_weapon_kind = category;
    test_ranged_mode = ranged;
    return melee_context((void *)1);
}
__declspec(dllexport) int probeCheckMeleeContexts(void) {
    int category, ranged;
    for (ranged = 0; ranged <= 1; ++ranged) {
        if (probeTestMeleeContext(-1, ranged) != (ranged ? 0 : 1) ||
            probeTestMeleeContext(5, ranged) != (ranged ? 0 : 1))
            return 1;
        for (category = 0; category <= 4; ++category)
            if (probeTestMeleeContext(category, ranged) != 2)
                return 2;
        if (probeTestMeleeContext(8, ranged) != 2 ||
            probeTestMeleeContext(6, ranged) != 0 ||
            probeTestMeleeContext(7, ranged) != 0 ||
            probeTestMeleeContext(9, ranged) != 0)
            return 3;
    }
    return 0;
}
__declspec(dllexport) void probeTestHands(void *owner, void *left, void *right, int enabled) {
    track_actor((void *)1, owner, 1, left, right, enabled, 1);
}
__declspec(dllexport) void probeTestElement(int element) {
    if (tracked_actors[0].character)
        tracked_actors[0].element = element;
}
__declspec(dllexport) void probeTestVisualSecondary(int element) {
    if (tracked_actors[0].character) {
        tracked_actors[0].secondary = element;
        tracked_actors[0].level = 100;
    }
}
__declspec(dllexport) int probeTestVisualStages(void) {
    if (visual_scale(0, 0.0f) > .801f || visual_scale(20, 0.0f) < .899f ||
        visual_scale(40, 0.0f) < .999f || visual_scale(80, 0.0f) < 1.099f ||
        visual_scale(100, 0.0f) < 1.149f || visual_scale(40, .1f) < 1.299f)
        return 0;
    return 1;
}
/* Keep the existing boolean probe for visual harnesses; the check runner
 * expects zero on success. */
__declspec(dllexport) int probeCheckVisualStages(void) {
    return probeTestVisualStages() ? 0 : 1;
}
__declspec(dllexport) int probeTestVisualSecondCount(void) {
    return weapon_visuals[0].effect2 ? (int)particle_count(weapon_visuals[0].effect2) : -1;
}
__declspec(dllexport) int probeTestVisualSecondHandCount(int side) {
    return side >= 0 && side < 2 && hand_effects[side].effect2
               ? (int)particle_count(hand_effects[side].effect2)
               : -1;
}
__declspec(dllexport) void probeTestExpirePending(void) {
    weapon_visuals[0].born = GetTickCount64() - 6000;
}
__declspec(dllexport) int probeTestTracked(void) {
    return weapon_visuals[0].blade != 0;
}
__declspec(dllexport) int probeTestBladeElement(void) {
    return weapon_visuals[0].effect ? weapon_visuals[0].element : 0;
}
__declspec(dllexport) int probeTestHandElement(int side) {
    return side >= 0 && side < 2 && hand_effects[side].effect ? hand_effects[side].element : 0;
}
__declspec(dllexport) int probeTestHandCount(int side) {
    HandEffect *h;
    if (side < 0 || side > 1)
        return -1;
    h = &hand_effects[side];
    return h->effect ? (int)particle_count(h->effect) : -1;
}
__declspec(dllexport) int probeTestStep(float dt) {
    update(dt);
    return weapon_visuals[0].effect ? (int)particle_count(weapon_visuals[0].effect) : -1;
}
#ifdef EE_ENABLE_TRAIL
__declspec(dllexport) int probeTestTrailLifecycle(void) {
    EEWeaponVisual *e = &weapon_visuals[0];
    Vec p = {1, 2, 3};
    Quat q1 = {1, 0, 0, 0}, q2 = {.9238795f, 0, 0, .3826834f};
    int i, element;
    if (!e->scene || !trail_supported)
        return 1;
    for (element = 1; element <= 5; ++element) {
        EETrailColour *color;
        trail_cleanup(e);
        e->element = element;
        trail_tick(e, &p, &q1, .016f);
        p.x += 10;
        trail_tick(e, &p, &q1, .016f);
        if (e->ribbon)
            return 2;
        trail_tick(e, &p, &q2, .016f);
        if (!e->ribbon || !e->trail_linked || !e->trail_attached)
            return 3;
        color = ((EETrailColour * (*)(void *, U64)) trail_get_colour)(e->ribbon, 0);
        if (!color || color->a < .27f || color->a > .29f)
            return 4;
        trail_finish(e, .016f);
        if (!e->ribbon)
            return 6;
        for (i = 0; i < 32; ++i)
            trail_finish(e, .016f);
        if (e->ribbon)
            return 7;
        trail_tick(e, &p, &q1, .016f);
        trail_tick(e, &p, &q2, .016f);
        if (!e->ribbon)
            return 8;
        for (i = 0; i < 40; ++i)
            trail_tick(e, &p, &q2, .016f);
        if (e->ribbon || e->ribbon_node)
            return 5;
    }
    e->element = 1;
    trail_cleanup(e);
    return 0;
}
__declspec(dllexport) int probeTestTrailDual(void) {
    EEWeaponVisual *a = &weapon_visuals[0], *b = &weapon_visuals[1];
    Vec p = {1, 2, 3};
    Quat q1 = {1, 0, 0, 0}, q2 = {.9238795f, 0, 0, .3826834f};
    if (!a->scene || !b->scene || !trail_supported)
        return 1;
    trail_cleanup(a);
    trail_cleanup(b);
    trail_tick(a, &p, &q1, .016f);
    trail_tick(b, &p, &q1, .016f);
    trail_tick(a, &p, &q2, .016f);
    trail_tick(b, &p, &q2, .016f);
    if (!a->ribbon || !b->ribbon || a->ribbon == b->ribbon)
        return 2;
    trail_cleanup(a);
    if (!b->ribbon)
        return 3;
    trail_cleanup(b);
    return 0;
}
__declspec(dllexport) int probeTestTrailActorSpace(void) {
    EEWeaponVisual temp;
    Vec p = {1, 2, 3}, offset = {80, 30, 0}, world;
    Quat q1 = {1, 0, 0, 0}, q2 = {.9238795f, 0, 0, .3826834f};
    void *root;
    float expected_x, expected_y;
    unsigned z;
    for (z = 0; z < sizeof(temp); ++z)
        ((volatile unsigned char *)&temp)[z] = 0;
    if (!weapon_visuals[0].scene || !trail_supported)
        return 1;
    temp.scene = weapon_visuals[0].scene;
    temp.length = 20;
    temp.element = 1;
    root = trail_root(temp.scene, 0);
    temp.parent = node_create(temp.scene, 0);
    if (!root || !temp.parent)
        return 2;
    node_add(root, temp.parent);
    node_position(temp.parent, &offset);
    trail_derive(temp.parent, &world);
    trail_tick(&temp, &p, &q1, .016f);
    trail_tick(&temp, &p, &q2, .016f);
    if (!temp.ribbon_node)
        return 3;
    trail_derive(temp.ribbon_node, &world);
    expected_x = offset.x + p.x + 2 * (q2.x * q2.y - q2.w * q2.z) * (temp.length * .55f - 5.0f);
    expected_y =
        offset.y + p.y + (1 - 2 * (q2.x * q2.x + q2.z * q2.z)) * (temp.length * .55f - 5.0f);
    if ((world.x - expected_x) * (world.x - expected_x) > .01f ||
        (world.y - expected_y) * (world.y - expected_y) > .01f)
        return 4;
    node_rotation(temp.parent, .70710678f, 0, 0, .70710678f);
    trail_derive(temp.parent, &world);
    trail_derive(temp.ribbon_node, &world);
    {
        float x = offset.x - (expected_y - offset.y), y = offset.y + (expected_x - offset.x);
        if ((world.x - x) * (world.x - x) > .01f || (world.y - y) * (world.y - y) > .01f)
            return 5;
    }
    trail_cleanup(&temp);
    node_destroy(temp.scene, temp.parent);
    return 0;
}
#endif
__declspec(dllexport) int probeTestAddSecond(void *owner, void *blade, void *tag) {
    if (weapon_visuals[1].blade)
        return 0;
    weapon_visuals[1].owner = owner;
    weapon_visuals[1].blade = blade;
    weapon_visuals[1].tag = tag;
    weapon_visuals[1].born = GetTickCount64();
    weapon_visuals[1].length = 20;
    return 1;
}
__declspec(dllexport) int probeTestSecondCount(void) {
    return weapon_visuals[1].effect ? (int)particle_count(weapon_visuals[1].effect) : -1;
}
__declspec(dllexport) int probeTestSecondElement(void) {
    return weapon_visuals[1].effect ? weapon_visuals[1].element : 0;
}
__declspec(dllexport) float probeTestHeight(int i) {
    void *tech;
    void *emitter;
    if (i < 0 || i >= EE_WEAPON_VISUAL_LIMIT || !weapon_visuals[i].effect)
        return -1;
    tech = particle_technique(weapon_visuals[i].effect, 0);
    emitter = tech ? technique_emitter(tech, 0) : 0;
    return emitter ? test_box_height(emitter) : -1;
}
__declspec(dllexport) void probeTestCleanupSecond(void) {
    erase(&weapon_visuals[1]);
}
__declspec(dllexport) int probeTestFreeTag(void) {
    void *tag = weapon_visuals[0].tag;
    test_free_calls = 0;
    original_free_tag = test_free_original;
    free_tag_hook((void *)1, tag);
    return test_free_calls == 1 && !weapon_visuals[0].blade;
}
__declspec(dllexport) int probeTestFreeObject(void) {
    void *blade = weapon_visuals[0].blade;
    test_free_calls = 0;
    original_free_object = test_free_original;
    free_object_hook((void *)1, blade);
    return test_free_calls == 1 && !weapon_visuals[0].blade;
}
__declspec(dllexport) int probeTestPositionMatches(void) {
    const Vec *p;
    const Quat *q;
    Vec n;
    float dx, dy, dz, start, off;
    if (!weapon_visuals[0].node)
        return 0;
    p = tag_position(weapon_visuals[0].tag);
    q = tag_rotation(weapon_visuals[0].tag);
    start = weapon_visuals[0].length * (weapon_visuals[0].pole ? .45f : .12f);
    off = (start + weapon_visuals[0].length * .98f) * .5f - 5;
    test_node_position(weapon_visuals[0].node, &n);
    dx = n.x - (p->x + 2 * (q->x * q->y - q->w * q->z) * off);
    dy = n.y - (p->y + (1 - 2 * (q->x * q->x + q->z * q->z)) * off);
    dz = n.z - (p->z + 2 * (q->y * q->z + q->w * q->x) * off);
    return dx * dx + dy * dy + dz * dz < .0001f;
}
__declspec(dllexport) void probeTestPositions(Vec *actual, Vec *target) {
    if (weapon_visuals[0].node) {
        test_node_position(weapon_visuals[0].node, actual);
        *target = *tag_position(weapon_visuals[0].tag);
    }
}
__declspec(dllexport) void probeTestCleanup(void) {
    erase(&weapon_visuals[0]);
}
#ifdef EE_ENABLE_UI
typedef struct EETestWidget {
    int visible, selected, font_height;
    EEMyGUICoord rect;
} EETestWidget;
static EETestWidget test_widgets[256];
static int test_widget_count, test_canvas_created, test_window_calls;
static unsigned char test_canvas_pixels[256 * 256 * 4];
static void *test_mouse_focus;
static unsigned char test_window_bytes[0xe0];
static void *test_gui_instance(void) {
    return (void *)1;
}
static void *test_ui_create(void *parent,
                            const EEGameString *type,
                            const EEGameString *skin,
                            const EEMyGUICoord *coord,
                            int align,
                            const EEGameString *name) {
    EETestWidget *w;
    (void)parent;
    (void)type;
    (void)skin;
    (void)name;
    if (align != 10 || test_widget_count >= 256)
        return 0;
    w = &test_widgets[test_widget_count++];
    w->rect = *coord;
    w->font_height = 20;
    return w;
}
static void test_ui_destroy(void *gui, void *widget) {
    (void)gui;
    (void)widget;
}
static void test_ui_visible(void *widget, unsigned char on) {
    ((EETestWidget *)widget)->visible = on;
}
static void test_ui_caption(void *widget, const EEGameString *text) {
    (void)widget;
    (void)text;
}
static void test_ui_selected(void *widget, unsigned char on) {
    ((EETestWidget *)widget)->selected = on;
}
static void test_ui_font(void *widget, const EEGameString *name) {
    (void)widget;
    (void)name;
}
static int test_ui_font_height(void *widget) {
    return ((EETestWidget *)widget)->font_height;
}
static void test_ui_set_font_height(void *widget, int height) {
    ((EETestWidget *)widget)->font_height = height;
}
static unsigned char test_ui_canvas_created(void *widget) {
    (void)widget;
    return (unsigned char)test_canvas_created;
}
static void test_ui_canvas_create(void *widget, int w, int h, int mode, int usage, int format) {
    (void)widget;
    if (w == 256 && h == 256 && mode == 0 && usage == 18 && format == 4)
        test_canvas_created = 1;
}
static int test_ui_canvas_dimension(void *widget) {
    (void)widget;
    return 256;
}
static void *test_ui_canvas_lock(void *widget, int usage) {
    (void)widget;
    return usage == 16 ? test_canvas_pixels : 0;
}
static void test_ui_canvas_unlock(void *widget) {
    (void)widget;
}
static void test_ui_set_size(void *widget, int width, int height) {
    (void)widget;
    (void)width;
    (void)height;
}
static void test_ui_set_position(void *widget, int x, int y) {
    ((EETestWidget *)widget)->rect.left = x;
    ((EETestWidget *)widget)->rect.top = y;
}
static int test_ui_window_width = 700;
static int test_ui_width(void *window) {
    (void)window;
    return test_ui_window_width;
}
static int test_ui_height(void *window) {
    (void)window;
    return 500;
}
static unsigned char test_ui_window_visible(void *window) {
    (void)window;
    return 1;
}
static void *test_ui_focus(void *input) {
    (void)input;
    return test_mouse_focus;
}
static void *test_ui_parent(void *widget) {
    (void)widget;
    return 0;
}
static void test_ui_window_original(void *window) {
    (void)window;
    ++test_window_calls;
}
static unsigned char test_ui_mouse_original(void *input, int x, int y, int button) {
    (void)input;
    (void)x;
    (void)y;
    (void)button;
    return 1;
}
__declspec(dllexport) int probeTestUIHooks(void) {
    unsigned char *window = test_window_bytes;
    EETestStats stats = {{0, 0}, (void *)9};
    EETestGameData data = {{0, 0, 0}, 0};
    int slot;
    EEUIView *view;
    *(void **)(window + 8) = (void *)7;
    *(void **)(window + 0xd0) = (void *)9;
    ee_persistence_init(&persistence);
    test_widget_count = 0;
    test_canvas_created = 0;
    test_window_calls = 0;
    test_ui_window_width = 700;
    ui_failed = 0;
    ui_panel_preference = 1;
    ui_scale_tenths = 10;
    ee_ui_view_set_scale(10);
    active = 1;
    test_player_flag = 1;
    character_is_player = test_is_player;
    character_handle = test_handle;
    character_stats = test_stats;
    test_handle_values[5] = 42;
    test_handle_values[6] = 7;
    mygui_api.gui_instance = test_gui_instance;
    mygui_api.create_widget = test_ui_create;
    mygui_api.destroy_widget = test_ui_destroy;
    mygui_api.set_visible = test_ui_visible;
    mygui_api.set_caption = test_ui_caption;
    mygui_api.set_selected = test_ui_selected;
    mygui_api.set_font = test_ui_font;
    mygui_api.get_font_height = test_ui_font_height;
    mygui_api.set_font_height = test_ui_set_font_height;
    mygui_api.canvas_created = test_ui_canvas_created;
    mygui_api.canvas_create = test_ui_canvas_create;
    mygui_api.canvas_width = test_ui_canvas_dimension;
    mygui_api.canvas_height = test_ui_canvas_dimension;
    mygui_api.canvas_lock = test_ui_canvas_lock;
    mygui_api.canvas_unlock = test_ui_canvas_unlock;
    mygui_api.set_size = test_ui_set_size;
    mygui_api.set_position = test_ui_set_position;
    ee_mygui_backend(&mygui_api, &ui_api);
    stats_window_width = test_ui_width;
    stats_window_height = test_ui_height;
    stats_window_visible = test_ui_window_visible;
    input_focus = test_ui_focus;
    widget_parent = test_ui_parent;
    original_stats_window_update = test_ui_window_original;
    original_stats_window_clear = test_ui_window_original;
    original_stats_window_destroy = test_ui_window_original;
    original_mouse_press = test_ui_mouse_original;
    original_mouse_move = test_ui_mouse_original;
    original_mouse_release = test_ui_mouse_original;
    original_stats_save = test_stats_original;
    gamedata_api.list_exists = test_list_exists;
    gamedata_api.list_size = test_list_size;
    gamedata_api.list_name = test_list_name;
    gamedata_api.list_values = test_list_values;
    gamedata_api.find_reference = test_find_reference;
    gamedata_api.add_reference = test_add_reference;
    stats_window_update_hook(window);
    slot = ui_find(window);
    if (slot < 0 || test_window_calls != 1)
        return 1;
    view = &stats_views[slot].view;
    if (!view->model.visible)
        return 12;
    track_actor((void *)9, (void *)3, 1, 0, 0, 0, 1);
    {
        int px = ((EETestWidget *)view->panel)->rect.left,
            py = ((EETestWidget *)view->panel)->rect.top;
        test_mouse_focus = view->header;
        mouse_press_hook((void *)2, 100, 100, 0);
        mouse_move_hook((void *)2, 145, 130, 0);
        mouse_release_hook((void *)2, 145, 130, 0);
        if (((EETestWidget *)view->panel)->rect.left != px + 45 ||
            ((EETestWidget *)view->panel)->rect.top != py + 30)
            return 22;
        mouse_move_hook((void *)2, 180, 180, 0);
        if (((EETestWidget *)view->panel)->rect.left != px + 45)
            return 23;
    }
    test_ui_window_width = 800;
    stats_window_update_hook(window);
    if (!view->model.visible || ((EETestWidget *)view->panel)->rect.left != 857 ||
        ((EETestWidget *)view->panel)->rect.top != 40)
        return 26;
    test_ui_window_width = 700;
    stats_window_update_hook(window);
    if (!view->model.visible || ((EETestWidget *)view->panel)->rect.left != 757 ||
        ((EETestWidget *)view->panel)->rect.top != 40)
        return 27;
    test_mouse_focus = view->opener;
    if (!mouse_press_hook((void *)2, 0, 0, 0) || view->model.visible)
        return 2;
    mouse_press_hook((void *)2, 0, 0, 0);
    if (!view->model.visible)
        return 13;
    stats_window_update_hook(window);
    if (!view->graph_ready || !test_canvas_created)
        return 3;
    test_mouse_focus = view->buttons[4];
    mouse_press_hook((void *)2, 0, 0, 0);
    if (!find_tracked_actor_by_body((void *)3) ||
        find_tracked_actor_by_body((void *)3)->element != 4)
        return 10;
    stats_window_update_hook(window);
    if (view->model.affinity.selected != 4)
        return 4;
    {
        EECharacterRecord *r =
            ee_character_observe(&persistence.characters, (void *)9, 42, 7, 0, 4);
        if (!r)
            return 7;
        r->affinity.total_xp = 13800;
        stats_window_update_hook(window);
        test_mouse_focus = view->mode_second;
        mouse_press_hook((void *)2, 0, 0, 0);
        if (!view->second_mode)
            return 8;
        test_mouse_focus = view->buttons[2];
        mouse_press_hook((void *)2, 0, 0, 0);
        if (!find_tracked_actor_by_body((void *)3) ||
            find_tracked_actor_by_body((void *)3)->secondary != 2)
            return 11;
        stats_window_update_hook(window);
        if (view->model.affinity.secondary != 2)
            return 9;
    }
    stats_save_hook(&stats, &data);
    if (!data.present || (data.value[0] & 7) != 4 || ((data.value[0] >> 16) & 7) != 2)
        return 5;
    *(void **)(window + 0xd0) = (void *)10;
    test_handle_values[5] = 43;
    stats_window_update_hook(window);
    if (!view->model.visible || view->model.character != (void *)10 || view->model.index != 43)
        return 14;
    if (((EETestWidget *)view->panel)->rect.left != stats_window_width(window) + 12 + 45 ||
        ((EETestWidget *)view->panel)->rect.top != 40)
        return 24;
    track_actor((void *)10, (void *)4, 1, 0, 0, 0, 1);
    test_mouse_focus = view->buttons[3];
    mouse_press_hook((void *)2, 0, 0, 0);
    if (!find_tracked_actor_by_body((void *)4) ||
        find_tracked_actor_by_body((void *)4)->element != 3 ||
        !find_tracked_actor_by_body((void *)3) ||
        find_tracked_actor_by_body((void *)3)->element != 4)
        return 19;
    test_mouse_focus = view->close;
    mouse_press_hook((void *)2, 0, 0, 0);
    if (view->model.visible || ui_panel_preference)
        return 15;
    *(void **)(window + 0xd0) = (void *)9;
    test_handle_values[5] = 42;
    stats_window_update_hook(window);
    if (view->model.visible || view->model.character != (void *)9)
        return 16;
    test_mouse_focus = view->opener;
    mouse_press_hook((void *)2, 0, 0, 0);
    if (!view->model.visible)
        return 17;
    ui_scale_tenths = 15;
    ee_ui_view_set_scale(15);
    stats_window_update_hook(window);
    if (!view->model.visible || view->model.character != (void *)9 ||
        ((EETestWidget *)view->panel)->rect.width != 795 ||
        ((EETestWidget *)view->effect)->font_height != 30)
        return 18;
    ui_scale_tenths = 10;
    ee_ui_view_set_scale(10);
    if (((EETestWidget *)view->panel)->rect.left != stats_window_width(window) + 18 + 45 ||
        ((EETestWidget *)view->panel)->rect.top != 45)
        return 25;
    ui_test_native_x = 0.87f;
    ui_test_native_y = 0.97f;
    stats_window_update_hook(window);
    if (!view->model.visible || ((EETestWidget *)view->panel)->rect.width != 461 ||
        ((EETestWidget *)view->panel)->rect.height != 435 ||
        ((EETestWidget *)view->effect)->font_height != 20)
        return 20;
    ui_test_native_x = 1.0f;
    ui_test_native_y = 1.0f;
    stats_window_update_hook(window);
    if (((EETestWidget *)view->panel)->rect.width != 530)
        return 21;
    stats_window_clear_hook(window);
    if (ui_find(window) >= 0)
        return 6;
    track_actor((void *)9, 0, 0, 0, 0, 0, 0);
    track_actor((void *)10, 0, 0, 0, 0, 0, 0);
    active = 0;
    return 0;
}
#endif
#endif
