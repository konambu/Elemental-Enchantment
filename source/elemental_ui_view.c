#include "elemental_ui_view.h"
#include "elemental_localization.h"
static EEUIViewScale ui_view_scale = {100, 100, 10};
void ee_ui_view_set_geometry_scale(unsigned int x, unsigned int y, unsigned int manual) {
    ui_view_scale.x = x;
    ui_view_scale.y = y;
    ui_view_scale.manual = manual;
}
void ee_ui_view_set_scale(unsigned int tenths) {
    tenths = tenths < 8 ? 8 : tenths > 20 ? 20 : tenths;
    ee_ui_view_set_geometry_scale(100, 100, tenths);
}
static unsigned int ui_effect_tenths = 10;
void ee_ui_view_set_effect_multiplier(float multiplier) {
    ui_effect_tenths = !(multiplier > 0.0f) ? 0
                       : multiplier >= 5.0f ? 50
                                            : (unsigned int)(multiplier * 10.0f + 0.5f);
}
static int add(char *out, int pos, int cap, const char *text) {
    while (text && *text && pos < cap - 1)
        out[pos++] = *text++;
    out[pos] = 0;
    return pos;
}
static int num(char *out, int pos, int cap, unsigned value) {
    char tmp[12];
    int n = 0;
    do {
        tmp[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value && n < 11);
    while (n && pos < cap - 1)
        out[pos++] = tmp[--n];
    out[pos] = 0;
    return pos;
}
static void decimal(char *out, int cap, const char *label, unsigned value, int percent) {
    int n = 0;
    n = add(out, n, cap, label);
    n = add(out, n, cap, " +");
    n = num(out, n, cap, value / 10);
    n = add(out, n, cap, ".");
    n = num(out, n, cap, value % 10);
    if (percent)
        add(out, n, cap, "%");
}
static void selection_title(char out[256], const EEAffinity *a) {
    const EELocale *l = ee_locale_get();
    int i = 0;
    unsigned e = a->selected;
    i = add(out, i, 256, l->main);
    i = add(out, i, 256, "  ");
    i = add(out, i, 256, e <= 5 ? l->element[e] : l->element[0]);
    i = add(out, i, 256, "     ");
    i = add(out, i, 256, l->sub);
    i = add(out, i, 256, "  ");
    e = a->secondary;
    add(out,
        i,
        256,
        ee_affinity_level(a) < 50 ? l->locked : (e <= 5 ? l->element[e] : l->element[0]));
}
void ee_ui_view_init(EEUIView *v) {
    int i;
    if (!v)
        return;
    ee_ui_reset(&v->model);
    v->parent = 0;
    v->opener = 0;
    v->panel = 0;
    v->body = 0;
    v->paper = 0;
    v->header = 0;
    v->window_title = 0;
    v->graph = 0;
    v->close = 0;
    v->status = 0;
    v->bar = 0;
    v->fill = 0;
    v->title = 0;
    v->effect = 0;
    v->detail = 0;
    v->mode_main = 0;
    v->mode_second = 0;
    v->milestone = 0;
    v->xp_label = 0;
    v->xp_percent = 0;
    v->second_mode = 0;
    v->graph_ready = 0;
    v->built_parent_width = 0;
    v->built_scale.x = 0;
    v->built_scale.y = 0;
    v->built_scale.manual = 0;
    for (i = 0; i < 5; ++i) {
        v->labels[i] = 0;
        v->axis_labels[i] = 0;
    }
    for (i = 0; i < 6; ++i)
        v->buttons[i] = 0;
}
static int valid(const EEUIViewAPI *a) {
    return a && a->create && a->destroy && a->visible && a->caption && a->selected && a->radar &&
           a->progress;
}
void ee_ui_view_dispose(EEUIView *v, const EEUIViewAPI *a) {
    if (!v || !valid(a))
        return;
    if (v->panel)
        a->destroy(a->context, v->panel);
    if (v->opener)
        a->destroy(a->context, v->opener);
    ee_ui_view_init(v);
}
int ee_ui_view_build(EEUIView *v, const EEUIViewAPI *a, void *parent, int width, int height) {
    int i, x;
    if (!v || !valid(a) || !parent || width < 300 || height < 260)
        return 0;
    if (v->parent == parent && v->opener && v->panel && v->built_parent_width == width &&
        v->built_scale.x == ui_view_scale.x &&
        v->built_scale.y == ui_view_scale.y && v->built_scale.manual == ui_view_scale.manual)
        return 1;
    if (v->parent) {
        EEUIModel saved = v->model;
        int mode = v->second_mode;
        ee_ui_view_dispose(v, a);
        v->model = saved;
        v->second_mode = mode;
    }
    v->parent = parent;
    v->built_parent_width = width;
    v->built_scale.x = ui_view_scale.x;
    v->built_scale.y = ui_view_scale.y;
    v->built_scale.manual = ui_view_scale.manual;
    x = width + 12;
    /* Place inside the unused area below the attributes list. The outer
     * window height includes borders and is not the child client height. */
    v->opener = a->create(a->context, parent, EE_UI_OPENER, 15, 260, 170, 26);
    v->panel = a->create(a->context, parent, EE_UI_PANEL, x, 10, 530, 448);
    if (!v->opener || !v->panel)
        goto fail;
    a->caption(a->context, v->opener, ee_locale_get()->opener);
    a->visible(a->context, v->panel, 0);
    v->body = a->create(a->context, v->panel, EE_UI_WINDOW_BODY, 0, 38, 530, 410);
    v->header = a->create(a->context, v->panel, EE_UI_WINDOW_HEADER, 0, 0, 530, 38);
    if (!v->body || !v->header)
        goto fail;
    v->paper = a->create(a->context, v->body, EE_UI_PAPER, 8, 4, 514, 400);
    if (!v->paper)
        goto fail;
    v->window_title = a->create(a->context, v->header, EE_UI_WINDOW_TITLE, 18, 5, 440, 28);
    v->close = a->create(a->context, v->header, EE_UI_WINDOW_CLOSE, 488, 4, 31, 32);
    v->graph = a->create(a->context, v->body, EE_UI_RADAR, 20, 50, 210, 200);
    v->status = a->create(a->context, v->body, EE_UI_LABEL, 260, 20, 240, 25);
    if (!v->window_title || !v->graph || !v->close || !v->status)
        goto fail;
    a->caption(a->context, v->window_title, "ELEMENTAL ENCHANTMENT");
    {
        const int xy[5][2] = {{100, 28}, {193, 72}, {188, 230}, {20, 230}, {15, 72}};
        for (i = 0; i < 5; ++i) {
            v->axis_labels[i] = a->create(a->context,
                                          v->body,
                                          i ? EE_UI_AXIS_LABEL : EE_UI_LABEL,
                                          xy[i][0],
                                          xy[i][1],
                                          100,
                                          22);
            if (!v->axis_labels[i])
                goto fail;
            a->caption(a->context,
                       v->axis_labels[i],
                       ee_locale_get()->element[ee_affinity_ring_order[i] + 1]);
        }
    }
    for (i = 0; i < 5; ++i) {
        v->labels[i] = a->create(a->context, v->body, EE_UI_LABEL, 400, 57 + i * 35, 110, 23);
        if (!v->labels[i])
            goto fail;
    }
    for (i = 0; i < 6; ++i) {
        int bx = 260, by = i ? 52 + (i - 1) * 35 : 252, bw = i ? 125 : 240;
        v->buttons[i] = a->create(a->context, v->body, EE_UI_BUTTON, bx, by, bw, 27);
        if (!v->buttons[i])
            goto fail;
        a->caption(a->context, v->buttons[i], ee_locale_get()->element[i]);
    }
    v->bar = a->create(a->context, v->body, EE_UI_BAR_BG, 20, 300, 490, 14);
    if (!v->bar)
        goto fail;
    v->fill = a->create(a->context, v->bar, EE_UI_BAR_FILL, 0, 0, 490, 14);
    if (!v->fill)
        goto fail;
    v->title = a->create(a->context, v->body, EE_UI_LABEL, 20, 320, 490, 22);
    v->effect = a->create(a->context, v->body, EE_UI_LABEL, 20, 345, 490, 22);
    v->detail = a->create(a->context, v->body, EE_UI_LABEL, 20, 370, 490, 22);
    if (!v->title || !v->effect || !v->detail)
        goto fail;
    a->caption(a->context, v->title, "ACTIVE EFFECT");
    v->mode_main = a->create(a->context, v->body, EE_UI_BUTTON, 20, 265, 95, 26);
    v->mode_second = a->create(a->context, v->body, EE_UI_BUTTON, 120, 265, 130, 26);
    v->milestone = a->create(a->context, v->body, EE_UI_LABEL, 20, 8, 230, 18);
    v->xp_label = a->create(a->context, v->body, EE_UI_BAR_TEXT, 26, 298, 180, 18);
    v->xp_percent = a->create(a->context, v->body, EE_UI_BAR_TEXT, 450, 298, 60, 18);
    if (!v->mode_main || !v->mode_second || !v->milestone || !v->xp_label || !v->xp_percent)
        goto fail;
    a->caption(a->context, v->mode_main, ee_locale_get()->main);
    return 1;
fail:
    ee_ui_view_dispose(v, a);
    return 0;
}
static void label(char out[32], unsigned int rate) {
    int i = 0;
    out[i++] = 'X';
    out[i++] = 'P';
    out[i++] = ' ';
    if (rate >= 100)
        out[i++] = '1';
    if (rate >= 10)
        out[i++] = (char)('0' + rate / 10 % 10);
    out[i++] = (char)('0' + rate % 10);
    out[i++] = '%';
    out[i] = 0;
}
static void level_text(char out[128], const EEAffinity *a) {
    int i = 0;
    i = add(out, i, 128, ee_locale_get()->elemental);
    i = add(out, i, 128, "   ");
    num(out, i, 128, ee_affinity_level(a));
}
static unsigned int xp_progress_percent(const EEAffinity *a) {
    unsigned int level, xp, base, need;
    if (!a->selected || a->selected > 5)
        return 0;
    xp = a->total_xp;
    level = ee_affinity_level(a);
    if (level == EE_AFFINITY_MAX_LEVEL)
        return 100;
    base = ee_affinity_xp_for_level(level);
    need = ee_affinity_xp_for_level(level + 1u) - base;
    return (xp - base) * 100u / need;
}
static void effect_text(char out[256],
                        char detail_out[256],
                        const char **detail,
                        const EEAffinity *a,
                        unsigned int weight) {
    const EELocale *l = ee_locale_get();
    unsigned int value, e = a->selected;
    float multiplier = (float)(weight * ui_effect_tenths) / 1000.0f;
    EEAffinity single = *a;
    EEDamage damage = {1.0f, 0, 0, 0, 0, 0};
    if (!e || e > 5) {
        add(out, 0, 256, l->noelement);
        *detail = l->choose;
        return;
    }
    /* Calculate one element at a time using the same rules as combat. */
    single.secondary = 0;
    ee_affinity_damage_scaled(&single, &damage, multiplier);
    if (e == 1) {
        value = (unsigned int)(damage.blunt * 1000.0f + 0.5001f);
        decimal(out, 256, l->blunt, value, 1);
        *detail = l->base;
    } else if (e == 2) {
        float speed = ee_affinity_attack_speed_scaled(&single, 1.0f, multiplier) - 1.0f;
        value = (unsigned int)(damage.armour * 1000.0f + 0.5001f);
        decimal(out, 256, l->armour, value, 1);
        decimal(detail_out, 256, l->bolt, (unsigned int)(speed * 1000.0f + 0.5001f), 1);
        *detail = detail_out;
    } else if (e == 3) {
        float speed = ee_affinity_combat_speed_scaled(&single, 1.0f, multiplier) - 1.0f;
        decimal(out, 256, l->attackblock, (unsigned int)(speed * 1000.0f + 0.5001f), 1);
        *detail = l->winddetail;
    } else if (e == 4) {
        value = (unsigned int)((damage.cut - 1.0f) * 1000.0f + 0.5001f);
        decimal(out, 256, l->cut, value, 1);
        *detail = l->base;
    } else {
        int n = 0;
        value = (unsigned int)(damage.bleed * 100.0f + 0.5001f);
        n = add(out, n, 256, l->bleed);
        n = add(out, n, 256, " +");
        n = num(out, n, 256, value / 100);
        n = add(out, n, 256, ".");
        if (value % 100 < 10)
            n = add(out, n, 256, "0");
        num(out, n, 256, value % 100);
        decimal(
            detail_out, 256, l->cut, (unsigned int)((damage.cut - 1.0f) * 1000.0f + 0.5001f), 1);
        *detail = detail_out;
    }
}
#ifdef ELEMENTAL_NATIVE_TEST
static int ui_test_has(const char *s, const char *part) {
    int i, j;
    for (i = 0; s[i]; ++i) {
        for (j = 0; part[j] && s[i + j] == part[j]; ++j) {
        }
        if (!part[j])
            return 1;
    }
    return 0;
}
__declspec(dllexport) int probeTestHalfEffectDisplay(void) {
    EEAffinity a;
    char out[256], detail_out[256];
    const char *detail;
    ee_locale_set("en_GB");
    ee_affinity_create(&a, 1, 1);
    a.total_xp = EE_AFFINITY_MAX_XP;
    ee_ui_view_set_effect_multiplier(0.5f);
    effect_text(out, detail_out, &detail, &a, 100);
    if (!ui_test_has(out, "7.5%"))
        return 1;
    ee_ui_view_set_effect_multiplier(0.3f);
    effect_text(out, detail_out, &detail, &a, 100);
    if (!ui_test_has(out, "4.5%"))
        return 3;
    ee_ui_view_set_effect_multiplier(0.5f);
    ee_affinity_select(&a, 5);
    effect_text(out, detail_out, &detail, &a, 100);
    if (!ui_test_has(out, "0.25"))
        return 2;
    a.total_xp = (unsigned short)ee_affinity_xp_for_level(2);
    ee_ui_view_set_effect_multiplier(5.0f);
    effect_text(out, detail_out, &detail, &a, 100);
    if (!ui_test_has(out, "0.54") || !ui_test_has(detail, "10.8%"))
        return 4;
    ee_ui_view_set_effect_multiplier(1.0f);
    return 0;
}
#endif
static void milestone_text(char out[256], unsigned int level) {
    const EELocale *l = ee_locale_get();
    int n = 0;
    unsigned int dest = 0, pct = 0;
    const char *what = 0;
    if (level >= 100) {
        add(out, 0, 256, l->master);
        return;
    }
    if (level < 20) {
        what = l->trail;
        dest = 20;
    } else if (level < 40) {
        what = l->glow;
        dest = 40;
    } else if (level < 50) {
        what = l->dual;
        dest = 50;
    } else {
        what = l->second;
        if (level < 60) {
            dest = 60;
            pct = 35;
        } else if (level < 70) {
            dest = 70;
            pct = 45;
        } else if (level < 80) {
            dest = 80;
            pct = 55;
        } else if (level < 90) {
            dest = 90;
            pct = 65;
        } else {
            dest = 100;
            pct = 75;
        }
    }
    n = add(out, n, 256, l->next);
    n = add(out, n, 256, ": ");
    n = add(out, n, 256, what);
    if (pct) {
        n = add(out, n, 256, " ");
        n = num(out, n, 256, pct);
        n = add(out, n, 256, "%");
    }
    n = add(out, n, 256, " ");
    n = add(out, n, 256, l->level);
    num(out, n, 256, dest);
}
void ee_ui_view_sync(EEUIView *v,
                     const EEUIViewAPI *a,
                     void *window,
                     void *character,
                     unsigned int index,
                     unsigned int serial,
                     const EEAffinity *affinity,
                     int editable) {
    int i, changed;
    if (!v || !valid(a) || !v->panel)
        return;
    ee_ui_bind(&v->model, window, character, index, serial, affinity, editable);
    a->visible(a->context, v->opener, v->model.has_affinity);
    a->visible(a->context, v->panel, v->model.visible);
    if (!v->model.visible)
        return;
    {
        char next[256];
        milestone_text(next, ee_affinity_level(&v->model.affinity));
        a->caption(a->context, v->milestone, next);
    }
    {
        unsigned int level = ee_affinity_level(&v->model.affinity);
        unsigned int pct = ee_affinity_secondary_percent(&v->model.affinity);
        char text[128], title[256];
        int j = 0;
        if (level < 50) {
            j = add(text, j, 128, "LV50 ");
            j = num(text, j, 128, level);
            j = add(text, j, 128, "/50");
            v->second_mode = 0;
        } else {
            j = add(text, j, 128, ee_locale_get()->second);
            j = add(text, j, 128, " ");
            j = num(text, j, 128, pct);
            j = add(text, j, 128, "%");
        }
        a->caption(a->context, v->mode_second, text);
        a->selected(a->context, v->mode_main, !v->second_mode);
        a->selected(a->context, v->mode_second, v->second_mode);
        selection_title(title, &v->model.affinity);
        a->caption(a->context, v->title, title);
    }
    {
        char text[128], number[8];
        unsigned int pct = xp_progress_percent(&v->model.affinity),
                     level = ee_affinity_level(&v->model.affinity);
        int n = 0;
        a->progress(a->context, v->fill, pct);
        level_text(text, &v->model.affinity);
        a->caption(a->context, v->status, editable ? text : ee_locale_get()->readonly);
        a->caption(a->context,
                   v->xp_label,
                   level == 100 ? ee_locale_get()->maxlevel : ee_locale_get()->nextlevel);
        if (pct >= 100)
            number[n++] = '1';
        if (pct >= 10)
            number[n++] = (char)('0' + pct / 10 % 10);
        number[n++] = (char)('0' + pct % 10);
        number[n++] = '%';
        number[n] = 0;
        a->caption(a->context, v->xp_percent, number);
    }
    {
        char text[256], detail_buf[256];
        const char *detail;
        EEAffinity shown = v->model.affinity;
        unsigned int weight = v->second_mode ? ee_affinity_secondary_percent(&shown) : 100;
        if (v->second_mode)
            shown.selected = shown.secondary;
        effect_text(text, detail_buf, &detail, &shown, weight);
        if (!editable) {
            a->caption(a->context, v->effect, ee_locale_get()->unavailable);
            a->caption(a->context, v->detail, ee_locale_get()->disabled);
        } else {
            a->caption(a->context, v->effect, text);
            a->caption(a->context, v->detail, detail);
        }
    }
    for (i = 0; i < 5; ++i) {
        char text[32];
        label(text, v->model.affinity.rates[i]);
        a->caption(a->context, v->labels[i], text);
    }
    for (i = 0; i < 6; ++i)
        a->selected(a->context,
                    v->buttons[i],
                    (v->second_mode ? v->model.affinity.secondary : v->model.affinity.selected) ==
                        (unsigned int)i);
    changed = !v->graph_ready || v->graph_state.selected != v->model.affinity.selected;
    for (i = 0; i < 5; ++i)
        if (!v->graph_ready || v->graph_state.rates[i] != v->model.affinity.rates[i])
            changed = 1;
    if (changed) {
        v->graph_ready = a->radar(a->context, v->graph, &v->model.affinity);
        if (v->graph_ready)
            v->graph_state = v->model.affinity;
    }
}
int ee_ui_view_click(EEUIView *v, const EEUIViewAPI *a, void *widget) {
    int i;
    void *character;
    unsigned int index, serial;
    if (!v || !valid(a) || !widget)
        return 0;
    if (widget == v->opener) {
        if (ee_ui_toggle(&v->model))
            a->visible(a->context, v->panel, v->model.visible);
        return 0;
    }
    if (widget == v->close && v->model.visible) {
        v->model.visible = 0;
        a->visible(a->context, v->panel, 0);
        return 0;
    }
    if (widget == v->mode_main && v->model.visible) {
        v->second_mode = 0;
        return 0;
    }
    if (widget == v->mode_second && v->model.visible) {
        if (ee_affinity_level(&v->model.affinity) >= 50)
            v->second_mode = 1;
        return 0;
    }
    for (i = 0; i < 6; ++i)
        if (widget == v->buttons[i] &&
            ee_ui_request_selection(&v->model, (unsigned int)i, &character, &index, &serial))
            return i + 1 + (v->second_mode ? 6 : 0);
    return 0;
}
