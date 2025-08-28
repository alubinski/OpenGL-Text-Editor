#include "memento.h"
#include "rope.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cglm/types-struct.h>
#include <limits.h>
#include <runara/runara.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct State {
    Window win;
    Display *dsp;
    GLXContext gl_context;
    RnState *render_state;
};

typedef struct Line Line;

struct Cursor {
    int32_t x;
    int32_t x_swap;
    Line *line;
    int32_t char_idx;
};

struct Line {
    uint32_t length;
    int32_t idx;
    Line *next;
    Line *prev;
};

struct Cursor cursor =
    (struct Cursor){.x = 0, .line = nullptr, .char_idx = 0, .x_swap = INT_MIN};

XIM xim;
XIC xic;

int32_t line_cursor_prev = INT_MIN;

RnFont *_font;

struct State _state;

RopeTree *rope_tree;

uint32_t line_num = 0;

vec2s get_cursor_pos() {
    return (vec2s){.x = cursor.x * _font->space_w,
                   .y = (cursor.line->idx - 1) * _font->size * 1.5f};
}

void create_gl_context() {
    int screen_id = DefaultScreen(_state.dsp);

    int attribs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};

    XVisualInfo *visual_info = glXChooseVisual(_state.dsp, screen_id, attribs);
    if (!visual_info) {
        fprintf(stderr, "No suitable Opengl visual found\n");
        return;
    }

    _state.gl_context =
        glXCreateContext(_state.dsp, visual_info, nullptr, GL_TRUE);
    if (!_state.gl_context) {
        fprintf(stderr, "OpenGL cannot be created\n");
        return;
    }
}

vec2s render_text(RnState *state, const char *text, RnFont *font, vec2s pos,
                  RnColor color, int32_t cursor, bool render) {

    // Get the harfbuzz text information for the string
    RnHarfbuzzText *hb_text = rn_hb_text_from_str(state, *font, text);

    // Set highest bearing as font size
    hb_text->highest_bearing = font->size;

    vec2s start_pos = (vec2s){.x = pos.x, .y = pos.y};

    // New line characters
    const int32_t line_feed = 0x000A;
    const int32_t carriage_return = 0x000D;
    const int32_t line_seperator = 0x2028;
    const int32_t paragraph_seperator = 0x2029;

    float textheight = 0;

    float scale = 1.0f;
    if (font->selected_strike_size)
        scale = ((float)font->size / (float)font->selected_strike_size);

    for (unsigned int i = 0; i < hb_text->glyph_count; i++) {
        // Get the glyph from the glyph index
        RnGlyph glyph = rn_glyph_from_codepoint(
            state, font, hb_text->glyph_info[i].codepoint);

        uint32_t text_length = strlen(text);
        uint32_t codepoint = rn_utf8_to_codepoint(
            text, hb_text->glyph_info[i].cluster, text_length);
        // Check if the unicode codepoint is a new line and advance
        // to the next line if so
        if (codepoint == line_feed || codepoint == carriage_return ||
            codepoint == line_seperator || codepoint == paragraph_seperator) {
            float font_height = font->face->size->metrics.height / 64.0f;
            pos.x = start_pos.x;
            pos.y += font_height;
            textheight += font_height;
            continue;
        }

        // Advance the x position by the tab width if
        // we iterate a tab character
        if (codepoint == '\t') {
            pos.x += font->tab_w * font->space_w;
            continue;
        }

        // If the glyph is not with
        if (!hb_text->glyph_info[i].codepoint) {
            continue;
        }
        float x_advance = (hb_text->glyph_pos[i].x_advance / 64.0f) * scale;
        float y_advance = (hb_text->glyph_pos[i].y_advance / 64.0f) * scale;
        float x_offset = (hb_text->glyph_pos[i].x_offset / 64.0f) * scale;
        float y_offset = (hb_text->glyph_pos[i].y_offset / 64.0f) * scale;

        vec2s glyph_pos = {pos.x + x_offset,
                           pos.y + hb_text->highest_bearing - y_offset};

        // Render the glyph
        if (render) {
            rn_glyph_render(state, glyph, *font, glyph_pos, color);
        }
        if (glyph.height > textheight) {
            textheight = glyph.height;
        }

        // Advance to the next glyph
        pos.x += x_advance;
        pos.y += y_advance;
    }

    return (vec2s){.x = 0, .y = 0};
}

void render(uint32_t render_w, uint32_t render_h) {
    // printf("Cursor: Line: %d, X: %d, char_idx=%d\n", cursor.line->idx,
    // cursor.x,
    //        cursor.char_idx);
    glClear(GL_COLOR_BUFFER_BIT);
    vec4s clear_color = rn_color_to_zto(rn_color_from_hex(0x282828));
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

    rn_resize_display(_state.render_state, render_w, render_h);

    rn_begin(_state.render_state);

    vec2s cursor_pos = get_cursor_pos();

    float x_offset = 0;
    if (cursor_pos.x >= render_w) {
        x_offset = cursor_pos.x - render_w + _font->size;
    }

    float y_offset = 0;
    if (cursor_pos.y >= render_h) {
        y_offset = cursor_pos.y - render_h + _font->size;
    }

    float max_width = 0;
    float x = 20;
    float y = 20;
    char buff[16];
    sprintf(buff, "%d", line_num);
    float width = rn_text_props(_state.render_state, buff, _font).width;
    if (width > max_width) {
        max_width = width;
    }
    for (int i = 1; i <= line_num; i++) {
        sprintf(buff, "%d", i);
        render_text(_state.render_state, buff, _font,
                    (vec2s){20 - x_offset, y - y_offset},
                    (RnColor){150, 150, 150, 255}, -1, True);
        y += _font->size * 1.5f;
    }
    y = 20;
    rn_rect_render(
        _state.render_state,
        (vec2s){x + cursor_pos.x - x_offset + max_width + 10, y + cursor_pos.y},
        (vec2s){1, 1.5f * _font->size}, RN_WHITE);
    List *leaves = get_leaves(rope_tree);
    List *leaves_start = leaves;

    for (List *l = leaves; l != nullptr; l = l->next) {

        render_text(_state.render_state, l->leaf->data, _font,
                    (vec2s){x - x_offset + max_width + 10, y - y_offset},
                    RN_WHITE, -1, True);

        if (strncmp(l->leaf->data, "\n", 1) == 0) {
            y += _font->size * 1.5f;
            x = 20;
            continue;
        }
        x += _font->space_w;
    }

    rn_end(_state.render_state);

    glXSwapBuffers(_state.dsp, _state.win);

    free_list(leaves_start);
    leaves = nullptr;
    leaves_start = nullptr;
}

Line *add_new_line(Line *prev) {
    Line *new_line = malloc(sizeof(Line));
    new_line->length = 0;
    new_line->idx = prev ? prev->idx + 1 : 1;
    new_line->prev = prev;
    if (prev) {
        new_line->next = prev->next;
        prev->next = new_line;
    }
    line_num++;
    return new_line;
}

int main() {
    _state.dsp = XOpenDisplay(0);

    xim = XOpenIM(_state.dsp, nullptr, nullptr, nullptr);
    XSetLocaleModifiers("");

    Window root_window = DefaultRootWindow(_state.dsp);

    int window_x = 0;
    int window_y = 0;
    int window_width = 1280;
    int window_height = 720;
    int border_width = 0;
    int window_depth = CopyFromParent;
    int window_class = CopyFromParent;
    Visual *window_visual = CopyFromParent;

    int font_size = 24;

    int attribute_value_mask = CWBackingPixel | CWEventMask;
    XSetWindowAttributes window_attributes;
    window_attributes.backing_pixel = 0xffffccaa;
    window_attributes.event_mask =
        StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;

    _state.win =
        XCreateWindow(_state.dsp, root_window, window_x, window_y, window_width,
                      window_height, border_width, window_depth, window_class,
                      window_visual, attribute_value_mask, &window_attributes);

    XSelectInput(_state.dsp, _state.win, window_attributes.event_mask);
    XMapWindow(_state.dsp, _state.win);

    xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                    XNClientWindow, _state.win, nullptr);

    XFlush(_state.dsp);

    Atom atom_delete_window = XInternAtom(_state.dsp, "WM_DELETE_WINDOW", True);
    if (!XSetWMProtocols(_state.dsp, _state.win, &atom_delete_window, 1)) {
        printf("Couldn't register WM_DELETE_WINDOW property \n");
    }

    create_gl_context();

    glXMakeCurrent(_state.dsp, _state.win, _state.gl_context);

    _state.render_state =
        rn_init(window_x, window_height, (RnGLLoader)glXGetProcAddressARB);

    _font =
        rn_load_font(_state.render_state, "./Iosevka-Regular.ttf", font_size);

    rope_tree = create_tree();

    Memento *m;
    cursor.line = add_new_line(nullptr);

    render(window_width, window_height);
    int is_window_open = 1;
    while (is_window_open) {
        XEvent general_event;
        XNextEvent(_state.dsp, &general_event);

        switch (general_event.type) {
            // case KeyRelease:
        case KeyPress: {
            XKeyPressedEvent *event = (XKeyPressedEvent *)&general_event;
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Escape)) {
                is_window_open = 0;
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Return)) {

                rope_tree = insert(rope_tree, cursor.char_idx, "\n");
                cursor.char_idx++;

                if (cursor.x != cursor.line->length) {
                    int32_t tmp = cursor.line->length;
                    cursor.line->length = cursor.x;
                    cursor.line = add_new_line(cursor.line);
                    cursor.line->length = tmp - cursor.x;
                } else {
                    cursor.line = add_new_line(cursor.line);
                }

                cursor.x = 0;
                cursor.x_swap = cursor.x;
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Left)) {
                if (cursor.x - 1 >= 0) {
                    cursor.x--;
                    cursor.char_idx--;
                } else if (cursor.line->idx - 1 > 0) {
                    cursor.line = cursor.line->prev;
                    cursor.x = cursor.line->length;
                    cursor.char_idx -= 1;
                }
                cursor.x_swap = cursor.x;
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Right)) {
                if (cursor.x + 1 <= cursor.line->length) {
                    cursor.x++;
                    cursor.char_idx++;
                } else if (cursor.line->idx + 1 <= line_num) {
                    cursor.x = 0;
                    cursor.line = cursor.line->next;
                    cursor.char_idx += 1;
                }
                cursor.x_swap = cursor.x;
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_BackSpace)) {
                // if (cursor.x == cursor.line->length) {
                //     cursor.char_idx--;
                // }
                cursor.line->length--;
                if (cursor.x - 1 >= 0) {
                    cursor.x--;
                } else if (cursor.line->idx - 1 > 0) {
                    cursor.line = cursor.line->prev;
                    cursor.x = cursor.line->length;
                    line_num--;
                }
                if (cursor.char_idx - 1 >= 0) {
                    cursor.char_idx -= 1;
                    rope_tree = rope_delete(rope_tree, cursor.char_idx, 1);
                }
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Up)) {
                if (cursor.line->idx - 1 > 0) {
                    int32_t tmp_x = cursor.x;
                    cursor.line = cursor.line->prev;
                    cursor.x = MIN(cursor.line->length, cursor.x_swap);
                    cursor.char_idx -=
                        (tmp_x + cursor.line->length - cursor.x + 1);
                }
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Down)) {
                if (cursor.line->idx + 1 <= line_num) {
                    int32_t tmp_x = cursor.x;
                    cursor.line = cursor.line->next;
                    cursor.x = MIN(cursor.line->length, cursor.x_swap);
                    cursor.char_idx +=
                        cursor.line->prev->length - tmp_x + cursor.x + 1;
                }
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_plus) &&
                event->state & ControlMask) {
                if (font_size + 6 <= 90) {
                    font_size += 6;
                    // WORKAROUND - rn_set_font_size causes segmentation fault
                    _font = rn_load_font(_state.render_state,
                                         "./Iosevka-Regular.ttf", font_size);
                }
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_minus) &&
                event->state & ControlMask) {
                if (font_size - 6 >= 6) {
                    font_size -= 6;
                    // WORKAROUND - rn_set_font_size causes segmentation fault
                    _font = rn_load_font(_state.render_state,
                                         "./Iosevka-Regular.ttf", font_size);
                }
                render(window_width, window_height);
                break;
            }
            struct Cursor prev_cursor;
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_S) &&
                event->state & ControlMask) {
                m = create_memento(rope_tree);
                prev_cursor = cursor;
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_U) &&
                event->state & ControlMask) {
                rope_tree = restore_from_memento(m);
                cursor = prev_cursor;
                render(window_width, window_height);
                break;
            }

            KeySym key_sym;
            char utf8_str[32];
            Status status;

            int len_utf8_str = Xutf8LookupString(
                xic, event, utf8_str, sizeof(utf8_str) - 1, &key_sym, &status);
            utf8_str[len_utf8_str] = '\0';

            if (len_utf8_str != 0) {

                rope_tree = insert(rope_tree, cursor.char_idx, utf8_str);
                cursor.char_idx++;
                cursor.x++;
                cursor.line->length++;
            }
            // print_RT("", rope_tree->root, false);
            render(window_width, window_height);
        } break;
        case ClientMessage: {
            if ((Atom)general_event.xclient.data.l[0] == atom_delete_window) {
                is_window_open = 0;
            }
        } break;
        case Expose: {
            render(window_width, window_height);
        } break;
        }
    }

    return 0;
}
