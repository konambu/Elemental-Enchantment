/* Experimental: caller holds guard when modifying visual records. */
#define CB_LIMIT 64
/* Drop stale loading samples and bound the life of orphaned projectile visuals. */
#define CB_CHARGE_STALE_MS 500u
#define CB_FLIGHT_MAX_AGE_MS 10000u
/* KenshiLib's combat/RangedCombatClass.h identifies the reload state,
 * gun, character, and timer fields below; GunClass.h identifies orientation.
 * These offsets are specific to the supported executable. Flight hooks are
 * gated on its module fingerprint; individual offsets are not probed. */
enum {
    CB_STATE_RELOADING = 2,
    CB_GUN_OFFSET = 0x28,
    CB_CHARACTER_OFFSET = 0x68,
    CB_RELOAD_DURATION_OFFSET = 0x74,
    CB_RELOAD_REMAINING_OFFSET = 0x78,
    CB_GUN_ROTATION_OFFSET = 0xf0,
    CB_PROJECTILE_OWNER_INDEX_OFFSET = 0x34,
    CB_PROJECTILE_OWNER_SERIAL_OFFSET = 0x38,
    CB_PROJECTILE_SPEED_OFFSET = 0x44,
    CB_PROJECTILE_NODE_OFFSET = 0x58,
    CB_HANDLE_INDEX_OFFSET = 0x14,
    CB_HANDLE_SERIAL_OFFSET = 0x18
};
typedef struct {
    void *character;
    void *scene;
    void *node;
    void *fx;
    void *fx2;
    Vec position;
    Quat rotation;
    float progress;
    U64 sample;
    int element;
    int secondary;
} CBCharge;
typedef struct {
    void *projectile;
    void *source;
    void *scene;
    void *node;
    void *fx;
    void *fx2;
    Vec previous, origin, offset;
    U64 born;
    unsigned int shooter_index, shooter_serial, shot_thread;
    U64 shot_sequence;
    unsigned int frames;
    int speed_done;
    int element;
    int secondary;
    int heading;
    int has_origin;
} CBFlight;
typedef struct {
    DWORD thread;
    unsigned index, serial;
    U64 sequence;
    void *created[16];
    unsigned count;
} CBShot;
static CBCharge cb_charges[CB_LIMIT];
static CBFlight cb_flights[CB_LIMIT];
static CBShot cb_shots[CB_LIMIT];
static U64 cb_shot_sequence;
static int cb_visual_enabled = 1;
static void (*original_ranged_update)(void *, float, const void *, const void *);
static void (*original_crossbow_shoot)(void *, void *, void *, int, const Vec *);
static void *(*original_harpoon_create)(void *, const void *, const void *);
static void (*original_projectile_trace)(void *, const Vec *, const Vec *);
static void (*original_projectile_destroy)(void *);
static Vec *(*cb_barrel)(void *, Vec *);
static float (*cb_normalise_vec)(Vec *);
static float (*cb_normalise_quat)(Quat *);
static void (*cb_particle_scale)(void *, const Vec *);
static void cb_clear_charge(CBCharge *c) {
    if (c->fx2 && c->scene)
        particle_destroy(particle_manager(), c->fx2, c->scene);
    if (c->fx && c->scene)
        particle_destroy(particle_manager(), c->fx, c->scene);
    if (c->node && c->scene)
        node_destroy(c->scene, c->node);
    *c = (CBCharge){0};
}
static void cb_reset_flight(CBFlight *f) {
    volatile unsigned char *p = (volatile unsigned char *)f;
    unsigned i;
    for (i = 0; i < sizeof(*f); ++i)
        p[i] = 0;
}
static void cb_clear_flight(CBFlight *f) {
    if (f->fx2 && f->scene)
        particle_destroy(particle_manager(), f->fx2, f->scene);
    if (f->fx && f->scene)
        particle_destroy(particle_manager(), f->fx, f->scene);
    if (f->node && f->scene)
        node_destroy(f->scene, f->node);
    cb_reset_flight(f);
}
static void clear_crossbow_scene(void *scene) {
    int i;
    for (i = 0; i < CB_LIMIT; ++i) {
        if (cb_charges[i].scene == scene)
            cb_clear_charge(&cb_charges[i]);
        if (cb_flights[i].scene == scene)
            cb_clear_flight(&cb_flights[i]);
    }
}
static void clear_crossbow_character(void *c) {
    int i;
    for (i = 0; i < CB_LIMIT; ++i)
        if (cb_charges[i].character == c)
            cb_clear_charge(&cb_charges[i]);
}
static void clear_crossbow_source(void *node) {
    int i;
    for (i = 0; i < CB_LIMIT; ++i)
        if (cb_flights[i].source == node)
            cb_clear_flight(&cb_flights[i]);
}
static void clear_crossbow_projectile(void *projectile) {
    int i;
    for (i = 0; i < CB_LIMIT; ++i)
        if (cb_flights[i].projectile == projectile)
            cb_clear_flight(&cb_flights[i]);
}
static int crossbow_impact_sample(void *projectile, void **scene, Vec *position) {
    int i;
    if (!projectile || !scene || !position || !trail_derive || !node_scene)
        return 0;
    for (i = 0; i < CB_LIMIT; ++i) {
        CBFlight *f = &cb_flights[i];
        if (f->projectile == projectile && f->source) {
            Vec raw;
            if (!trail_derive(f->source, &raw))
                return 0;
            *scene = f->scene ? f->scene : node_scene(f->source);
            if (!*scene)
                return 0;
            if (f->frames) {
                raw.x += f->offset.x;
                raw.y += f->offset.y;
                raw.z += f->offset.z;
            } else if (f->has_origin)
                raw = f->origin;
            if (!(raw.x > -1000000 && raw.x < 1000000 && raw.y > -1000000 && raw.y < 1000000 &&
                  raw.z > -1000000 && raw.z < 1000000))
                return 0;
            *position = raw;
            return 1;
        }
    }
    return 0;
}
/* Keep the engine's reload state machine and shorten only its timer. */
static float cb_reload_adjust(float timer, float dt, float rate) {
    float extra;
    if (!(timer > 0.0f) || !(dt > 0.0f && dt < 1.0f) || !(rate > 1.0f && rate < 2.0f))
        return timer;
    extra = dt * (rate - 1.0f);
    return extra >= timer ? 0.0f : timer - extra;
}
static void cb_apply_wind_reload(void *ranged, float dt) {
    void *actor;
    float *timer;
    float max, old, rate = 1.0f;
    int i, eligible = 0;
    EEAffinity a = {0};
    if (!active || !ranged || !(dt > 0.0f && dt < 1.0f) || internal == GetCurrentThreadId())
        return;
    __try {
        if (*(int *)ranged != CB_STATE_RELOADING)
            return;
        actor = *(void **)((unsigned char *)ranged + CB_CHARACTER_OFFSET);
        if (!actor)
            return;
        timer = (float *)((unsigned char *)ranged + CB_RELOAD_REMAINING_OFFSET);
        max = *(float *)((unsigned char *)ranged + CB_RELOAD_DURATION_OFFSET);
        old = *timer;
        if (!(max > .001f && max < 1000.0f && old > 0.0f && old <= max + .01f))
            return;
        AcquireSRWLockExclusive(&guard);
        __try {
            for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
                if (tracked_actors[i].character == actor &&
                    (tracked_actors[i].element == 3 || tracked_actors[i].secondary == 3)) {
                    eligible = 1;
                    break;
                }
            if (eligible && combat_affinity_locked(actor, character_stats(actor), &a))
                rate = ee_affinity_combat_speed_scaled(&a, 1.0f, effect_multiplier);
            if (rate > 1.0f && rate < 2.0f)
                *timer = cb_reload_adjust(old, dt, rate);
        } __finally {
            ReleaseSRWLockExclusive(&guard);
        }
    } __except (1) {
        logline("CROSSBOW: wind reload bonus skipped after error.\r\n");
    }
}
static void cb_update_hook(void *ranged, float dt, const void *target, const void *suggested) {
    void *actor, *gun;
    float timer, max, progress;
    int state, i, slot = -1;
    Vec barrel;
    Quat aim = {1, 0, 0, 0};
    cb_apply_wind_reload(ranged, dt);
    original_ranged_update(ranged, dt, target, suggested);
    if (!active || !ranged || internal == GetCurrentThreadId())
        return;
    __try {
        actor = *(void **)((unsigned char *)ranged + CB_CHARACTER_OFFSET);
        gun = *(void **)((unsigned char *)ranged + CB_GUN_OFFSET);
        state = *(int *)ranged;
        timer = *(float *)((unsigned char *)ranged + CB_RELOAD_REMAINING_OFFSET);
        max = *(float *)((unsigned char *)ranged + CB_RELOAD_DURATION_OFFSET);
        if (!actor || !gun || !character_is_player(actor) || state > CB_STATE_RELOADING ||
            state < 0 || !(max > .001f && max < 1000.0f) || !(timer >= 0.0f && timer <= max) ||
            !cb_barrel)
            return;
        progress = 1.0f - timer / max;
        if (progress < 0)
            progress = 0;
        if (progress > 1)
            progress = 1;
        cb_barrel(gun, &barrel);
        barrel.y += 1.0f; /* Lift the charge just above the crossbow. */
        if (!(barrel.x > -1000000 && barrel.x < 1000000 && barrel.y > -1000000 &&
              barrel.y < 1000000 && barrel.z > -1000000 && barrel.z < 1000000))
            return;
        __try {
            Quat q = *(Quat *)((unsigned char *)gun + CB_GUN_ROTATION_OFFSET);
            float q2 = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
            if (q2 > .01f && q2 < 4.0f) {
                aim = q;
                cb_normalise_quat(&aim);
            }
        } __except (1) {
            /* Rotation is optional; retain the default orientation. */
        }
        AcquireSRWLockExclusive(&guard);
        __try {
            for (i = 0; i < CB_LIMIT; ++i) {
                if (cb_charges[i].character == actor) {
                    slot = i;
                    break;
                }
                if (slot < 0 && !cb_charges[i].character)
                    slot = i;
            }
            if (slot >= 0) {
                CBCharge *c = &cb_charges[slot];
                c->character = actor;
                c->position = barrel;
                c->rotation = aim;
                c->progress = progress;
                c->sample = GetTickCount64();
            }
        } __finally {
            ReleaseSRWLockExclusive(&guard);
        }
    } __except (1) {
        logline("CROSSBOW: reload sample skipped.\r\n");
    }
}
static void cb_trace_hook(void *projectile, const Vec *pos, const Vec *direction) {
    int i, j, slot = -1;
    unsigned index, serial_number;
    void *node;
    original_projectile_trace(projectile, pos, direction);
    if (!active || !projectile || internal == GetCurrentThreadId())
        return;
    __try {
        index = *(unsigned *)((unsigned char *)projectile + CB_PROJECTILE_OWNER_INDEX_OFFSET);
        serial_number =
            *(unsigned *)((unsigned char *)projectile + CB_PROJECTILE_OWNER_SERIAL_OFFSET);
        node = *(void **)((unsigned char *)projectile + CB_PROJECTILE_NODE_OFFSET);
        if (!node)
            return;
        AcquireSRWLockExclusive(&guard);
        internal = GetCurrentThreadId();
        __try {
            for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
                if (tracked_actors[i].character && tracked_actors[i].element) {
                    const unsigned char *h =
                        (const unsigned char *)character_handle(tracked_actors[i].character);
                    if (h && *(unsigned *)(h + CB_HANDLE_INDEX_OFFSET) == index &&
                        *(unsigned *)(h + CB_HANDLE_SERIAL_OFFSET) == serial_number)
                        break;
                }
            if (i < EE_TRACKED_ACTOR_LIMIT) {
                for (j = 0; j < CB_LIMIT; ++j) {
                    if (cb_flights[j].projectile == projectile) {
                        cb_clear_flight(&cb_flights[j]);
                        slot = j;
                        break;
                    }
                    if (slot < 0 && !cb_flights[j].projectile)
                        slot = j;
                }
                if (slot >= 0) {
                    CBFlight *f = &cb_flights[slot];
                    f->projectile = projectile;
                    f->source = node;
                    f->born = GetTickCount64();
                    f->shooter_index = index;
                    f->shooter_serial = serial_number;
                    f->shot_thread = GetCurrentThreadId();
                    for (j = 0; j < CB_LIMIT; ++j)
                        if (cb_shots[j].thread == f->shot_thread && cb_shots[j].index == index &&
                            cb_shots[j].serial == serial_number &&
                            cb_shots[j].sequence > f->shot_sequence)
                            f->shot_sequence = cb_shots[j].sequence;
                    f->element = tracked_actors[i].element;
                    f->secondary = tracked_actors[i].secondary;
                    f->previous = pos ? *pos : (Vec){0};
                }
            }
        } __finally {
            internal = 0;
            ReleaseSRWLockExclusive(&guard);
        }
    } __except (1) {
        logline("CROSSBOW: projectile sample skipped.\r\n");
    }
}
static void *cb_create_hook(void *manager, const void *mesh, const void *material) {
    void *projectile = original_harpoon_create(manager, mesh, material);
    int i;
    DWORD tid = GetCurrentThreadId();
    if (!active || !projectile || internal == tid)
        return projectile;
    AcquireSRWLockExclusive(&guard);
    __try {
        for (i = 0; i < CB_LIMIT; ++i)
            if (cb_shots[i].sequence && cb_shots[i].thread == tid) {
                CBShot *s = &cb_shots[i];
                if (s->count < 16)
                    s->created[s->count++] = projectile;
                break;
            }
    } __finally {
        ReleaseSRWLockExclusive(&guard);
    }
    return projectile;
}
static void cb_capture_created(CBShot *shot, int element, int secondary) {
    unsigned n;
    int i;
    for (n = 0; n < shot->count; ++n)
        __try {
            void *projectile = shot->created[n];
            void *node = *(void **)((unsigned char *)projectile + CB_PROJECTILE_NODE_OFFSET);
            unsigned index = *(unsigned *)((unsigned char *)projectile +
                                           CB_PROJECTILE_OWNER_INDEX_OFFSET),
                     serial_number = *(unsigned *)((unsigned char *)projectile +
                                                   CB_PROJECTILE_OWNER_SERIAL_OFFSET);
            if (!node || index != shot->index || serial_number != shot->serial)
                continue;
            for (i = 0; i < CB_LIMIT; ++i)
                if (cb_flights[i].projectile == projectile)
                    break;
            if (i < CB_LIMIT)
                continue;
            for (i = 0; i < CB_LIMIT; ++i)
                if (!cb_flights[i].projectile)
                    break;
            if (i < CB_LIMIT) {
                int j;
                CBFlight *f = &cb_flights[i];
                f->projectile = projectile;
                f->source = node;
                f->born = GetTickCount64();
                f->shooter_index = index;
                f->shooter_serial = serial_number;
                f->shot_thread = shot->thread;
                f->shot_sequence = shot->sequence;
                f->element = element;
                f->secondary = secondary;
                for (j = 0; j < CB_LIMIT; ++j)
                    if (cb_charges[j].character &&
                        GetTickCount64() - cb_charges[j].sample < CB_CHARGE_STALE_MS) {
                        const unsigned char *h =
                            (const unsigned char *)character_handle(cb_charges[j].character);
                        if (h && *(unsigned *)(h + CB_HANDLE_INDEX_OFFSET) == index &&
                            *(unsigned *)(h + CB_HANDLE_SERIAL_OFFSET) == serial_number) {
                            f->origin = cb_charges[j].position;
                            f->origin.y -= 1.0f;
                            f->has_origin = 1;
                            break;
                        }
                    }
            }
        } __except (1) {
            logline("CROSSBOW: created projectile sample skipped.\r\n");
        }
}
static void cb_shoot_hook(void *gun, void *actor, void *target, int stat, const Vec *aim) {
    int i, slot = -1, element = 0, secondary = 0;
    unsigned index = 0, serial_number = 0;
    U64 sequence = 0;
    float speed = 1.0f;
    int player = 0;
    DWORD tid = GetCurrentThreadId();
    if (active && actor && internal != tid)
        __try {
            if (character_is_player(actor)) {
                const unsigned char *h = (const unsigned char *)character_handle(actor);
                void *stats = character_stats(actor);
                if (h && stats) {
                    index = *(const unsigned *)(h + CB_HANDLE_INDEX_OFFSET);
                    serial_number = *(const unsigned *)(h + CB_HANDLE_SERIAL_OFFSET);
                    AcquireSRWLockExclusive(&guard);
                    __try {
                        EECharacterRecord *r =
                            ee_persistence_bind(&persistence,
                                                stats,
                                                actor,
                                                index,
                                                serial_number,
                                                index ^ (serial_number * 0x9e3779b9u),
                                                (unsigned)element_from_character(actor));
                        if (r && !r->read_only) {
                            speed = ee_affinity_attack_speed_scaled(
                                &r->affinity, 1.0f, effect_multiplier);
                            player = 1;
                            for (i = 0; i < CB_LIMIT; ++i)
                                if (!cb_shots[i].sequence) {
                                    slot = i;
                                    break;
                                }
                            if (slot >= 0) {
                                sequence = ++cb_shot_sequence;
                                if (!sequence)
                                    sequence = ++cb_shot_sequence;
                                cb_shots[slot].thread = tid;
                                cb_shots[slot].index = index;
                                cb_shots[slot].serial = serial_number;
                                cb_shots[slot].sequence = sequence;
                                cb_shots[slot].count = 0;
                            }
                        }
                    } __finally {
                        ReleaseSRWLockExclusive(&guard);
                    }
                }
            }
        } __except (1) {
            player = 0;
        }
    original_crossbow_shoot(gun, actor, target, stat, aim);
    if (player && sequence) {
        AcquireSRWLockExclusive(&guard);
        __try {
            if (slot >= 0 && cb_shots[slot].sequence == sequence) {
                for (i = 0; i < EE_TRACKED_ACTOR_LIMIT; ++i)
                    if (tracked_actors[i].character == actor) {
                        element = tracked_actors[i].element;
                        secondary = tracked_actors[i].secondary;
                        break;
                    }
                if (element)
                    cb_capture_created(&cb_shots[slot], element, secondary);
                cb_shots[slot].sequence = 0;
                cb_shots[slot].count = 0;
            }
            if (speed > 1.0f && speed < 2.0f)
                for (i = 0; i < CB_LIMIT; ++i) {
                    CBFlight *f = &cb_flights[i];
                    if (f->projectile && !f->speed_done && f->shot_sequence == sequence) {
                        float *velocity =
                            (float *)((unsigned char *)f->projectile + CB_PROJECTILE_SPEED_OFFSET);
                        __try {
                            if (*velocity > 0.0f && *velocity < 10000.0f) {
                                *velocity *= speed;
                                f->speed_done = 1;
                            }
                        } __except (1) {
                            f->speed_done = 1;
                        }
                    }
                }
        } __finally {
            ReleaseSRWLockExclusive(&guard);
        }
    }
}
static void cb_destroy_hook(void *projectile) {
    if (active && projectile && internal != GetCurrentThreadId()) {
        AcquireSRWLockExclusive(&guard);
        internal = GetCurrentThreadId();
        __try {
            clear_crossbow_projectile(projectile);
        } __finally {
            internal = 0;
            ReleaseSRWLockExclusive(&guard);
        }
    }
    original_projectile_destroy(projectile);
}
static int cb_create_fx(
    void *manager, void *scene, void **node, void **fx, void **fx2, int element, int secondary) {
    void *root = trail_root ? trail_root(scene, 0) : 0;
    String primary_storage;
    if (!root)
        return 0;
    *node = node_create(scene, 0);
    if (!*node)
        return 0;
    node_add(root, *node);
    *fx = create_secondary_effect(manager,
                                  scene,
                                  *node,
                                  element_template(&primary_storage, EE_TEMPLATE_CROSSBOW, element),
                                  0);
    if (!*fx)
        return 0;
    if (secondary) {
        String secondary_storage;
        *fx2 = create_secondary_effect(
            manager,
            scene,
            *node,
            element_template(&secondary_storage, EE_TEMPLATE_CROSSBOW, secondary),
            0);
    }
    return 1;
}
static Vec cb_align_position(CBFlight *f, Vec raw) {
    Vec p;
    if (!f->frames) {
        if (f->has_origin) {
            f->offset = (Vec){f->origin.x - raw.x, f->origin.y - raw.y, f->origin.z - raw.z};
        }
    }
    ++f->frames;
    p = (Vec){raw.x + f->offset.x, raw.y + f->offset.y, raw.z + f->offset.z};
    if (f->frames == 1)
        f->previous = p;
    return p;
}
static void cb_update_visuals_impl(void *manager, float dt) {
    int i, j;
    U64 now = GetTickCount64();
    const float bounds[6] = {0, 0, 0, 25, 25, 25};
    for (i = 0; i < CB_LIMIT; ++i) {
        CBCharge *c = &cb_charges[i];
        EETrackedActor *a = 0;
        void *parent;
        float scale;
        Vec sz;
        if (!c->character)
            continue;
        for (j = 0; j < EE_TRACKED_ACTOR_LIMIT; ++j)
            if (tracked_actors[j].character == c->character) {
                a = &tracked_actors[j];
                break;
            }
        if (!a || !a->body || !a->element || now - c->sample > CB_CHARGE_STALE_MS) {
            cb_clear_charge(c);
            continue;
        }
        parent = parent_node(a->body);
        if (!parent) {
            cb_clear_charge(c);
            continue;
        }
        if (c->fx && (c->element != a->element || c->secondary != a->secondary ||
                      c->scene != node_scene(parent))) {
            cb_clear_charge(c);
            continue;
        }
        if (!c->fx) {
            c->scene = node_scene(parent);
            c->element = a->element;
            c->secondary = a->secondary;
            if (!c->scene ||
                !cb_create_fx(
                    manager, c->scene, &c->node, &c->fx, &c->fx2, c->element, c->secondary)) {
                cb_clear_charge(c);
                continue;
            }
        }
        scale = .20f + .80f * c->progress;
        sz = (Vec){scale, scale, scale};
        node_scale(c->node, &sz);
        if (cb_particle_scale) {
            float main_scale = scale * (c->element == 1 ? 1.0f : 1.6f),
                  sub_scale = scale * (c->secondary == 1 ? 1.0f : 1.6f);
            Vec main_size = {main_scale, main_scale, main_scale},
                sub_size = {sub_scale, sub_scale, sub_scale};
            cb_particle_scale(c->fx, &main_size);
            if (c->fx2)
                cb_particle_scale(c->fx2, &sub_size);
        }
        node_position(c->node, &c->position);
        if (c->rotation.w || c->rotation.x || c->rotation.y || c->rotation.z)
            node_rotation(c->node, c->rotation.w, c->rotation.x, c->rotation.y, c->rotation.z);
        particle_update(c->fx, dt);
        set_aabb(c->fx, bounds);
        if (c->fx2) {
            particle_update(c->fx2, dt);
            set_aabb(c->fx2, bounds);
        }
    }
    for (i = 0; i < CB_LIMIT; ++i) {
        CBFlight *f = &cb_flights[i];
        Vec pos, back;
        Quat q;
        float d;
        if (!f->projectile)
            continue;
        if (!f->source || now - f->born > CB_FLIGHT_MAX_AGE_MS) {
            cb_clear_flight(f);
            continue;
        }
        if (!trail_derive(f->source, &pos)) {
            cb_clear_flight(f);
            continue;
        }
        pos = cb_align_position(f, pos);
        back = (Vec){f->previous.x - pos.x, f->previous.y - pos.y, f->previous.z - pos.z};
        d = cb_normalise_vec(&back);
        f->previous = pos;
        if (d > .01f && d < 2000) {
            if (back.y < -.9999f)
                q = (Quat){0, 1, 0, 0};
            else {
                q = (Quat){1 + back.y, back.z, 0, -back.x};
                cb_normalise_quat(&q);
            }
            f->heading = 1;
            pos.x -= back.x * 2.0f;
            pos.y -= back.y * 2.0f;
            pos.z -= back.z * 2.0f;
        } else
            q = (Quat){1, 0, 0, 0};
        if (!f->fx) {
            float main_scale = f->element == 1 ? 1.75f : 2.8f,
                  sub_scale = f->secondary == 1 ? 1.75f : 2.8f;
            Vec size = {2.0f, 2.0f, 2.0f}, main_size = {main_scale, main_scale, main_scale},
                sub_size = {sub_scale, sub_scale, sub_scale};
            f->scene = node_scene(f->source);
            if (!f->scene ||
                !cb_create_fx(
                    manager, f->scene, &f->node, &f->fx, &f->fx2, f->element, f->secondary)) {
                cb_clear_flight(f);
                continue;
            }
            node_position(f->node, &pos);
            if (f->heading)
                node_rotation(f->node, q.w, q.x, q.y, q.z);
            node_scale(f->node, &size);
            if (cb_particle_scale) {
                cb_particle_scale(f->fx, &main_size);
                if (f->fx2)
                    cb_particle_scale(f->fx2, &sub_size);
            }
            particle_local(f->fx, 0);
            if (f->fx2)
                particle_local(f->fx2, 0);
            if (f->element != 2 || f->heading)
                particle_update(f->fx, .10f);
            if (f->fx2 && (f->secondary != 2 || f->heading))
                particle_update(f->fx2, .10f);
        }
        node_position(f->node, &pos);
        if (d > .01f && d < 2000)
            node_rotation(f->node, q.w, q.x, q.y, q.z);
        particle_update(f->fx, dt);
        set_aabb(f->fx, bounds);
        if (f->fx2) {
            particle_update(f->fx2, dt);
            set_aabb(f->fx2, bounds);
        }
    }
}

static void cb_update_visuals(void *manager, float dt) {
    int i;
    if (!cb_visual_enabled)
        return;
    __try {
        cb_update_visuals_impl(manager, dt);
    } __except (1) {
        cb_visual_enabled = 0;
        logline("CROSSBOW: visuals disabled after native error.\r\n");
        for (i = 0; i < CB_LIMIT; ++i) {
            __try {
                cb_clear_charge(&cb_charges[i]);
            } __except (1) {
                cb_charges[i] = (CBCharge){0};
            }
            __try {
                cb_clear_flight(&cb_flights[i]);
            } __except (1) {
                cb_reset_flight(&cb_flights[i]);
            }
        }
    }
}
