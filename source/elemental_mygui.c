#include "elemental_mygui.h"
#include "elemental_radar.h"
static void make_game_string(EEGameString *s, const char *p) {
    unsigned long long n = 0;
    while (p[n])
        ++n;
    ee_game_string_init(s, p, n, n < 16 ? 15 : n);
}
static void *create_view_widget(void *c, void *parent, int kind, int x, int y, int w, int h) {
    EEMyGUI *a = (EEMyGUI *)c;
    EEGameString type, skin, name, font;
    EEMyGUICoord rect = {x, y, w, h};
    void *widget;
    unsigned int scale, sx, sy;
    if (!a || !a->create_widget || !parent)
        return 0;
    scale = a->scale_tenths >= 8 && a->scale_tenths <= 20 ? a->scale_tenths : 10;
    sx = a->scale_x100 ? a->scale_x100 : 100;
    sy = a->scale_y100 ? a->scale_y100 : 100;
    if (kind == EE_UI_OPENER) {
        rect.left = (x * (int)sx + 50) / 100;
        rect.top = (y * (int)sy + 50) / 100;
        rect.width = (w * (int)sx + 50) / 100;
        rect.height = (h * (int)sy + 50) / 100;
    } else {
        sx = sx * scale / 10;
        sy = sy * scale / 10;
        if (kind == EE_UI_PANEL)
            rect.left = x - 12 + (12 * (int)sx + 50) / 100;
        else
            rect.left = (x * (int)sx + 50) / 100;
        rect.top = (y * (int)sy + 50) / 100;
        rect.width = (w * (int)sx + 50) / 100;
        rect.height = (h * (int)sy + 50) / 100;
    }
    if (kind == EE_UI_PANEL) {
        make_game_string(&type, "Widget");
        make_game_string(&skin, "PanelEmpty");
    } else if (kind == EE_UI_BUTTON || kind == EE_UI_OPENER) {
        make_game_string(&type, "Button");
        make_game_string(&skin, "Kenshi_Button1Skin");
    } else if (kind == EE_UI_LABEL) {
        make_game_string(&type, "TextBox");
        make_game_string(&skin, "Kenshi_TextboxStandardText");
    } else if (kind == EE_UI_AXIS_LABEL || kind == EE_UI_BAR_TEXT) {
        make_game_string(&type, "TextBox");
        make_game_string(&skin, "Kenshi_TextBoxEmptySkin");
    } else if (kind == EE_UI_RADAR) {
        make_game_string(&type, "Canvas");
        make_game_string(&skin, "Canvas");
    } else if (kind == EE_UI_BAR_BG) {
        make_game_string(&type, "Widget");
        make_game_string(&skin, "Kenshi_ProgressBarEmptySkin");
    } else if (kind == EE_UI_BAR_FILL) {
        make_game_string(&type, "Widget");
        make_game_string(&skin, "Kenshi_ProgressBarFilledSkin");
    } else if (kind == EE_UI_WINDOW_BODY) {
        make_game_string(&type, "Widget");
        make_game_string(&skin, "Kenshi_GenericWindowSkin");
    } else if (kind == EE_UI_PAPER) {
        make_game_string(&type, "Widget");
        make_game_string(&skin, "Kenshi_GenericTextBoxSkin");
    } else if (kind == EE_UI_WINDOW_HEADER) {
        make_game_string(&type, "Widget");
        make_game_string(&skin, "Kenshi_GenericWindowHeaderSkin");
    } else if (kind == EE_UI_WINDOW_TITLE) {
        make_game_string(&type, "TextBox");
        make_game_string(&skin, "Kenshi_TextBoxEmptySkin");
    } else if (kind == EE_UI_WINDOW_CLOSE) {
        make_game_string(&type, "Button");
        make_game_string(&skin, "Kenshi_CloseButtonSkin");
    } else
        return 0;
    make_game_string(&name, "");
    widget = a->create_widget(parent, &type, &skin, &rect, 10, &name);
    /* A popup retains its parent lifecycle while escaping its clipping area. */
    if (widget && a->set_style && kind == EE_UI_PANEL) {
        EEGameString layer;
        make_game_string(&layer, "Popup");
        a->set_style(widget, 1, &layer);
    }
    if (widget && a->set_font &&
        (kind == EE_UI_BUTTON || kind == EE_UI_OPENER || kind == EE_UI_LABEL ||
         kind == EE_UI_AXIS_LABEL || kind == EE_UI_BAR_TEXT || kind == EE_UI_WINDOW_TITLE)) {
        make_game_string(&font, "Kenshi_PaintedTextFont_Medium");
        a->set_font(widget, &font);
        if (kind != EE_UI_OPENER && scale != 10 && a->get_font_height && a->set_font_height) {
            int size = a->get_font_height(widget);
            if (size > 0 && size < 200)
                a->set_font_height(widget, (size * (int)scale + 5) / 10);
        }
    }
    if (widget && a->set_property && kind == EE_UI_AXIS_LABEL) {
        EEGameString key, value;
        make_game_string(&key, "TextColour");
        make_game_string(&value, "0.0784314 0.0313726 0.0235294");
        a->set_property(widget, &key, &value);
    }
    if (widget && a->set_property && kind == EE_UI_BAR_TEXT) {
        EEGameString key, value;
        make_game_string(&key, "TextColour");
        make_game_string(&value, "0.90 0.86 0.76");
        a->set_property(widget, &key, &value);
    }
    if (widget && a->set_property && (kind == EE_UI_BUTTON || kind == EE_UI_OPENER)) {
        EEGameString key, value;
        make_game_string(&key, "TextAlign");
        make_game_string(&value, "Center");
        a->set_property(widget, &key, &value);
    }
    if (widget && a->set_property && kind == EE_UI_WINDOW_TITLE) {
        EEGameString key, value;
        make_game_string(&key, "TextColour");
        make_game_string(&value, "0.78 0.72 0.62");
        a->set_property(widget, &key, &value);
        make_game_string(&key, "TextAlign");
        make_game_string(&value, "Center");
        a->set_property(widget, &key, &value);
    }
    return widget;
}
static void remove_widget(void *c, void *widget) {
    EEMyGUI *a = (EEMyGUI *)c;
    void *gui;
    if (!a || !a->gui_instance || !a->destroy_widget || !widget)
        return;
    gui = a->gui_instance();
    if (gui)
        a->destroy_widget(gui, widget);
}
static void set_view_visible(void *c, void *widget, int show) {
    EEMyGUI *a = (EEMyGUI *)c;
    if (a && a->set_visible && widget)
        a->set_visible(widget, (unsigned char)(show != 0));
}
static void set_view_caption(void *c, void *widget, const char *text) {
    EEMyGUI *a = (EEMyGUI *)c;
    EEGameString value;
    if (!a || !a->set_caption || !widget || !text)
        return;
    make_game_string(&value, text);
    a->set_caption(widget, &value);
}
static void set_view_selected(void *c, void *widget, int on) {
    EEMyGUI *a = (EEMyGUI *)c;
    if (a && a->set_selected && widget)
        a->set_selected(widget, (unsigned char)(on != 0));
}
static void set_view_progress(void *c, void *widget, unsigned int pct) {
    EEMyGUI *a = (EEMyGUI *)c;
    unsigned int scale, sx, sy;
    if (!a || !widget || !a->set_size || !a->set_visible)
        return;
    scale = a->scale_tenths >= 8 && a->scale_tenths <= 20 ? a->scale_tenths : 10;
    sx = (a->scale_x100 ? a->scale_x100 : 100) * scale / 10;
    sy = (a->scale_y100 ? a->scale_y100 : 100) * scale / 10;
    if (pct > 100)
        pct = 100;
    a->set_visible(widget, (unsigned char)(pct != 0));
    if (pct)
        a->set_size(widget, (int)(490u * sx * pct / 10000u), (int)((14u * sy + 50u) / 100u));
}
static int draw_view_radar(void *c, void *widget, const EEAffinity *affinity) {
    EEMyGUI *a = (EEMyGUI *)c;
    unsigned char *pixels;
    int ok;
    if (!a || !widget || !affinity || !a->canvas_created || !a->canvas_create || !a->canvas_width ||
        !a->canvas_height || !a->canvas_lock || !a->canvas_unlock)
        return 0;
    /* A fixed power-of-two texture avoids resize churn. The MyGUI texture
     * interface presents
     * a linear buffer; actual D3D compatibility must be
     * verified in game before shipping
     * this backend. */
    if (!a->canvas_created(widget))
        a->canvas_create(widget, 256, 256, 0, 18, 4);
    if (a->canvas_width(widget) != 256 || a->canvas_height(widget) != 256)
        return 0;
    pixels = (unsigned char *)a->canvas_lock(widget, 16);
    if (!pixels)
        return 0;
    __try {
        ok = ee_radar_draw(pixels, 256, 256, affinity);
    } __finally {
        a->canvas_unlock(widget);
    }
    return ok;
}
void ee_mygui_backend(EEMyGUI *a, EEUIViewAPI *out) {
    if (!out)
        return;
    out->context = a;
    out->create = create_view_widget;
    out->destroy = remove_widget;
    out->visible = set_view_visible;
    out->caption = set_view_caption;
    out->selected = set_view_selected;
    out->radar = draw_view_radar;
    out->progress = set_view_progress;
}
