/* Private, opt-in Ogre ribbon test. Caller holds the render guard. */
typedef struct EETrailColour {
    float r, g, b, a;
} EETrailColour;
static int trail_supported;
static int trail_resolve(MODULE ogre) {
    *(void **)(&trail_root) = GetProcAddress(
        ogre,
        "?getRootSceneNode@SceneManager@Ogre@@QEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z");
    *(void **)(&trail_create) =
        GetProcAddress(ogre, "?createRibbonTrail@SceneManager@Ogre@@UEAAPEAVRibbonTrail@2@XZ");
    *(void **)(&trail_destroy) =
        GetProcAddress(ogre, "?destroyRibbonTrail@SceneManager@Ogre@@UEAAXPEAVRibbonTrail@2@@Z");
    *(void **)(&trail_add) = GetProcAddress(ogre, "?addNode@RibbonTrail@Ogre@@UEAAXPEAVNode@2@@Z");
    *(void **)(&trail_remove) =
        GetProcAddress(ogre, "?removeNode@RibbonTrail@Ogre@@UEAAXPEAVNode@2@@Z");
    *(void **)(&trail_derive) =
        GetProcAddress(ogre, "?_getDerivedPositionUpdated@Node@Ogre@@QEAA?AVVector3@2@XZ");
    *(void **)(&trail_colour_change) =
        GetProcAddress(ogre, "?setColourChange@RibbonTrail@Ogre@@UEAAX_KMMMM@Z");
    trail_get_colour =
        GetProcAddress(ogre, "?getInitialColour@RibbonTrail@Ogre@@UEBAAEBVColourValue@2@_K@Z");
    trail_get_width = GetProcAddress(ogre, "?getInitialWidth@RibbonTrail@Ogre@@UEBAM_K@Z");
    trail_get_material = GetProcAddress(ogre,
                                        "?getMaterialName@BillboardChain@Ogre@@UEBAAEBV?$basic_"
                                        "string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ");
    trail_supported = trail_root && trail_create && trail_destroy && trail_add && trail_remove &&
                      trail_derive && trail_colour_change && trail_get_colour && trail_get_width &&
                      trail_get_material;
    return trail_supported;
}
static void trail_drop(Entry *e) {
    if (e->ribbon && e->ribbon_node && e->trail_linked)
        trail_remove(e->ribbon, e->ribbon_node);
    if (e->ribbon && e->ribbon_root && e->trail_attached)
        original_scene_detach(e->ribbon_root, e->ribbon);
    if (e->ribbon && e->scene)
        trail_destroy(e->scene, e->ribbon);
    if (e->ribbon_node && e->scene)
        node_destroy(e->scene, e->ribbon_node);
    e->ribbon = 0;
    e->ribbon_node = 0;
    e->ribbon_root = 0;
    e->trail_idle = 0;
    e->trail_element = 0;
    e->trail_linked = 0;
    e->trail_attached = 0;
}
static void trail_cleanup(Entry *e) {
    trail_drop(e);
    e->trail_has_direction = 0;
}
static int trail_start(Entry *e, int element, const Vec *initial_tip) {
    void **vtable;
    EETrailColour color;
    Vec derived;
    String material = {{"EE_Trail"}, 8, 15, 0}, group = {{"Autodetect"}, 10, 15, 0};
    if (!trail_supported || !e->scene)
        return 0;
    e->ribbon_root = trail_root(e->scene, 0);
    if (!e->ribbon_root)
        return 0;
    e->ribbon_node = node_create(e->scene, 0);
    e->ribbon = trail_create(e->scene);
    if (!e->ribbon_node || !e->ribbon) {
        trail_drop(e);
        return 0;
    }
    vtable = *(void ***)e->ribbon;
    if (!vtable || vtable[36] != trail_get_material || vtable[52] != trail_get_colour ||
        vtable[56] != trail_get_width || !vtable[37] || !vtable[48] || !vtable[51] || !vtable[55]) {
        trail_supported = 0;
        trail_drop(e);
        return 0;
    }
    if (element == 1) {
        color.r = 1.0f;
        color.g = .20f;
        color.b = .04f;
    } else if (element == 2) {
        color.r = .45f;
        color.g = .72f;
        color.b = 1.0f;
    } else if (element == 3) {
        color.r = .65f;
        color.g = 1.0f;
        color.b = .82f;
    } else if (element == 4) {
        color.r = .12f;
        color.g = .55f;
        color.b = 1.0f;
    } else {
        color.r = .72f;
        color.g = .92f;
        color.b = 1.0f;
    }
    color.a = .28f;
    ((void (*)(void *, const String *, const String *))vtable[37])(e->ribbon, &material, &group);
    ((void (*)(void *, float))vtable[48])(e->ribbon, e->length * 1.2f);
    ((void (*)(void *, U64, const EETrailColour *))vtable[51])(e->ribbon, 0, &color);
    ((void (*)(void *, U64, float))vtable[55])(e->ribbon, 0, e->length * (e->pole ? .53f : .86f));
    trail_colour_change(e->ribbon, 0, 0, 0, 0, .59f);
    /* Tag positions are in the same actor-relative space used by the blade
     * effect. Parent
     * the tracked node to that actor node; the ribbon itself
     * remains at the scene root
     * because it stores world-space history. */
    if (!e->parent) {
        trail_drop(e);
        return 0;
    }
    node_add(e->parent, e->ribbon_node);
    node_position(e->ribbon_node, initial_tip);
    trail_derive(e->ribbon_node, &derived);
    node_attach(e->ribbon_root, e->ribbon);
    e->trail_attached = 1;
    trail_add(e->ribbon, e->ribbon_node);
    e->trail_linked = 1;
    set_queue(e->ribbon, 84);
    e->trail_element = element;
    return 1;
}
static void trail_tick(Entry *e, const Vec *p, const Quat *q, float dt) {
    float length = e->length * .98f - 5.0f,
          center = (e->length * (e->pole ? .45f : .12f) + e->length * .98f) * .5f - 5.0f, dx, dy,
          dz, threshold;
    Vec direction, tip;
    if (!trail_supported || !p || !q || dt <= 0.0f || dt > .1f)
        return;
    direction.x = 2 * (q->x * q->y - q->w * q->z) * length;
    direction.y = (1 - 2 * (q->x * q->x + q->z * q->z)) * length;
    direction.z = 2 * (q->y * q->z + q->w * q->x) * length;
    tip.x = p->x + 2 * (q->x * q->y - q->w * q->z) * center;
    tip.y = p->y + (1 - 2 * (q->x * q->x + q->z * q->z)) * center;
    tip.z = p->z + 2 * (q->y * q->z + q->w * q->x) * center;
    if (!e->trail_has_direction) {
        if (e->ribbon)
            trail_drop(e);
        e->trail_direction = direction;
        e->trail_has_direction = 1;
        return;
    }
    dx = direction.x - e->trail_direction.x;
    dy = direction.y - e->trail_direction.y;
    dz = direction.z - e->trail_direction.z;
    e->trail_direction = direction;
    if (e->ribbon && e->trail_element != e->element)
        trail_drop(e);
    threshold = 12.0f * dt;
    if (dx * dx + dy * dy + dz * dz > threshold * threshold) {
        e->trail_idle = 0;
        if (!e->ribbon && !trail_start(e, e->element, &tip))
            return;
    } else if (e->ribbon) {
        e->trail_idle += dt;
        if (e->trail_idle > .48f) {
            trail_drop(e);
            return;
        }
    }
    if (e->ribbon_node) {
        node_position(e->ribbon_node, &tip);
        node_rotation(e->ribbon_node, q->w, q->x, q->y, q->z);
    }
}
static void trail_finish(Entry *e, float dt) {
    /* Stop recording movement at the end of an attack, but let existing
     * segments fade
     * naturally instead of destroying them on the same frame. */
    e->trail_has_direction = 0;
    if (e->ribbon && dt > 0.0f && dt <= .1f) {
        e->trail_idle += dt;
        if (e->trail_idle > .48f)
            trail_drop(e);
    }
}
