#include "cursor.h"
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
#include <stddef.h>
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

struct Cursor cursor =
    (struct Cursor){.line = 0, .column = 0, .desired_column = 0};

Line *head;

XIM xim;
XIC xic;

int32_t line_cursor_prev = INT_MIN;

RnFont *_font;

struct State _state;

RopeTree *rope_tree;

uint32_t line_num = 0;

vec2s get_cursor_pos() {
    return (vec2s){.x = cursor.column * _font->space_w,
                   .y = (cursor.line) * _font->size * 1.5f};
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
            pos.y += _font->size * 1.5f;
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

    return (vec2s){.x = pos.x, .y = pos.y};
}

void render(uint32_t render_w, uint32_t render_h) {
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

    const float new_line_start_x = x + cursor_pos.x - x_offset + max_width + 10;

    rn_rect_render(
        _state.render_state,
        (vec2s){x + cursor_pos.x - x_offset + max_width + 10, y + cursor_pos.y},
        (vec2s){1, 1.5f * _font->size}, RN_WHITE);
    List *leaves = get_leaves(rope_tree);
    List *leaves_start = leaves;

    vec2s rendering_start_point =
        (vec2s){.x = x - x_offset + max_width + 10, .y = y - y_offset};

    size_t total_text_length = 0;
    // TODO: find better sollution this might be to slow for big files
    for (List *l = leaves; l != nullptr; l = l->next) {
        total_text_length += strlen(l->leaf->data);
    }
    char *text = malloc(total_text_length + 1);
    if (!text) {
        return;
    }
    text[0] = '\0';

    for (List *l = leaves; l != nullptr; l = l->next) {
        strcat(text, l->leaf->data);
    }

    render_text(_state.render_state, text, _font,
                (vec2s){x - x_offset + max_width + 10, y - y_offset}, RN_WHITE,
                -1, True);
    // rn_text_render_ex(_state.render_state, text, _font,
    //                   (vec2s){x - x_offset + max_width + 10, y - y_offset},
    //                   RN_WHITE, _font->size * 1.5f, True);

    // for (List *l = leaves; l != nullptr; l = l->next) {
    //     text_props = rn_text_render(
    //         _state.render_state, l->leaf->data, _font,
    //         (vec2s){x - x_offset + max_width + 10, y - y_offset}, RN_WHITE);
    //     int new_line_count = 0;
    //     const char *last_nl = nullptr;
    //     for (const char *p = l->leaf->data; p && *p != '\0'; ++p) {
    //         if (*p == '\n') {
    //             new_line_count++;
    //             last_nl = p;
    //         }
    //     }
    //
    //     const char *after_last_nl = last_nl ? last_nl + 1 : l->leaf->data;
    //
    //     printf("string after last nl: %s\n", after_last_nl);
    //     printf("Before adjust x=%f, y=%f\b", x, y);
    //     y += new_line_count * _font->size * 1.5f;
    //     if (new_line_count > 0) {
    //         x = strlen(after_last_nl) * _font->space_w + 20;
    //     } else {
    //         x += strlen(after_last_nl) * _font->space_w;
    //     }
    //     printf("After adjust x=%f, y=%f\n", x, y);
    //     // if (strncmp(l->leaf->data, "\n", 1) == 0) {
    //     //     y += _font->size * 1.5f;
    //     //     x = 20;
    //     //     continue;
    //     // }
    // }

    rn_end(_state.render_state);

    glXSwapBuffers(_state.dsp, _state.win);

    free_list(leaves_start);
    leaves = nullptr;
    leaves_start = nullptr;
}

void render_bottom_bar(uint32_t render_w, uint32_t render_h, Window win,
                       char *buffer) {
    glClear(GL_COLOR_BUFFER_BIT);
    vec4s clear_color = rn_color_to_zto(rn_color_from_hex(0x282828));
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

    rn_resize_display(_state.render_state, render_w, render_h);

    rn_begin(_state.render_state);

    float x = 20;
    float y = render_h - 40;

    rn_text_render(_state.render_state, buffer, _font, (vec2s){x, y}, RN_WHITE);

    rn_end(_state.render_state);

    glXSwapBuffers(_state.dsp, _state.win);
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

char *open_bottom_bar(int window_width, int window_height) {
    bool is_bottom_bar_open = true;
    XSelectInput(_state.dsp, _state.win, ExposureMask | KeyPressMask);

    char utf8_str[32];
    int idx = 0;
    char *buff = calloc(sizeof(char), 256);
    while (is_bottom_bar_open) {

        XEvent general_event;
        XNextEvent(_state.dsp, &general_event);

        switch (general_event.type) {
        case KeyPress: {

            XKeyPressedEvent *event = (XKeyPressedEvent *)&general_event;
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Return)) {
                is_bottom_bar_open = false;
                break;
            }
            KeySym key_sym;
            XComposeStatus status;
            char c;
            int written =
                XLookupString(event, &c, sizeof(c), &key_sym, &status);
            if (written) {
                buff[idx] = c;
                buff[idx + 1] = '\0';
                idx++;
            }

            render_bottom_bar(window_width, window_height, _state.win, buff);
            //  render(window_width, window_height);
        } break;
        case Expose: {
            render_bottom_bar(window_width, window_height, _state.win, buff);
            // render(window_width, window_height);

        } break;
        }
    }
    return buff;
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
    size_t carataker_capacity = 100;
    Caretaker *undo_carataker = create_caretaker(carataker_capacity);
    Caretaker *redo_caretaker = create_caretaker(carataker_capacity);

    LineIndex line_index = {nullptr, nullptr, 0, 0};
    add_line_to_index(&line_index, 0, 0);

    render(window_width, window_height);
    int is_window_open = 1;
    while (is_window_open) {
        XEvent general_event;
        XNextEvent(_state.dsp, &general_event);

        switch (general_event.type) {
        case KeyPress: {
            XKeyPressedEvent *event = (XKeyPressedEvent *)&general_event;
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Escape)) {
                is_window_open = 0;
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Return)) {
                rope_tree =
                    insert(rope_tree,
                           line_column_to_offset(line_index, cursor.line,
                                                 cursor.column++),
                           "\n");
                line_index.line_length[cursor.line]++;

                if (cursor.column != line_index.line_length[cursor.line]) {
                    size_t tmp = line_index.line_length[cursor.line];
                    line_index.line_length[cursor.line] = cursor.column;
                    add_line_to_index(
                        &line_index,
                        line_column_to_offset(line_index, cursor.line,
                                              cursor.column),
                        tmp - line_index.line_length[cursor.line]);
                    cursor.line++;
                } else {
                    add_line_to_index(&line_index,
                                      line_column_to_offset(line_index,
                                                            cursor.line++,
                                                            cursor.column),
                                      0);
                }
                cursor.column = 0;
                cursor.desired_column = cursor.column;
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Left)) {
                if (cursor.column > 0) {
                    cursor.column--;
                } else if (cursor.line > 0) {
                    cursor.line--;
                    cursor.column = line_index.line_length[cursor.line] - 1;
                }
                cursor.desired_column = cursor.column;
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Right)) {
                if ((cursor.line != line_index.line_num - 1 &&
                     cursor.column < line_index.line_length[cursor.line] - 1) ||
                    (cursor.line == line_index.line_num - 1 &&
                     cursor.column < line_index.line_length[cursor.line])) {
                    cursor.column++;
                } else if (cursor.line + 1 < line_index.line_num) {
                    cursor.column = 0;
                    cursor.line++;
                }
                cursor.desired_column = cursor.column;
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_BackSpace)) {
                line_index.line_length[cursor.line]--;
                if (cursor.column > 0) {
                    cursor.column--;
                } else if (cursor.line > 0) {
                    delete_line_from_index(&line_index, cursor.line--);
                    cursor.column = line_index.line_length[cursor.line];
                }
                rope_tree =
                    rope_delete(rope_tree,
                                line_column_to_offset(line_index, cursor.line,
                                                      cursor.column),
                                1);

                cursor.desired_column = cursor.column;
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Up)) {
                if (cursor.line > 0) {
                    cursor.line--;
                    size_t line_lenght =
                        cursor.line == line_index.line_num - 1
                            ? line_index.line_length[cursor.line]
                            : line_index.line_length[cursor.line] - 1;
                    cursor.column = MIN(line_lenght, cursor.desired_column);
                }
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Down)) {
                if (cursor.line < line_index.line_num - 1) {
                    cursor.line++;
                    size_t line_lenght =
                        cursor.line == line_index.line_num - 1
                            ? line_index.line_length[cursor.line]
                            : line_index.line_length[cursor.line] - 1;
                    cursor.column = MIN(line_lenght, cursor.desired_column);
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

                char *f_path = open_bottom_bar(window_width, window_height);
                FILE *fp = fopen(f_path, "w");
                if (!fp) {
                    perror("Failed to open file");
                    return EXIT_FAILURE;
                }
                save_to_file(rope_tree->root, fp);
                fclose(fp);

                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_L) &&
                event->state & ControlMask) {
                char *f_path = open_bottom_bar(window_width, window_height);

                FILE *fp = fopen(f_path, "r");

                if (!fp) {
                    perror("Failed to open file");
                    return EXIT_FAILURE;
                }
                fseek(fp, 0, SEEK_END);
                size_t file_size = ftell(fp);
                rewind(fp);

                size_t chunk_num = (file_size + CHUNK_BASE - 1) / CHUNK_BASE;
                char **chunks = malloc(chunk_num * sizeof(char *));

                for (size_t i = 0; i < chunk_num; ++i) {
                    chunks[i] = malloc(chunk_num + 1);
                    size_t read = fread(chunks[i], 1, CHUNK_BASE, fp);
                    chunks[i][read] = '\0';
                }
                fclose(fp);
                rope_tree = build_rope(chunks, 0, chunk_num - 1);
                for (size_t i = 0; i < chunk_num; ++i) {
                    free(chunks[i]);
                }
                free(chunks);

                List *leaves = get_leaves(rope_tree);
                travelse_list_and_index_lines(leaves, &line_index);
                cursor.line = line_index.line_num - 1;
                cursor.column = line_index.line_length[cursor.line];

                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_U) &&
                event->state & ControlMask) {
                Memento *m = pop_memento(undo_carataker);
                save_memento(redo_caretaker, m);
                rope_tree = restore_from_memento(m, &head);
                cursor.line = 0;
                cursor.column = 0;
                render(window_width, window_height);
                break;
            }
            if (event->keycode == XKeysymToKeycode(_state.dsp, XK_R) &&
                event->state & ControlMask) {
                Memento *m = pop_memento(redo_caretaker);
                save_memento(undo_carataker, m);
                rope_tree = restore_from_memento(m, &head);
                cursor.line = 0;
                cursor.column = 0;
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
                Memento *m = create_memento(rope_tree, head);
                save_memento(undo_carataker, m);
                clear_caretaker(redo_caretaker);
                rope_tree = insert(rope_tree,
                                   line_column_to_offset(
                                       line_index, cursor.line, cursor.column),
                                   utf8_str);
                cursor.column++;
                cursor.desired_column = cursor.column;
                line_index.line_length[cursor.line]++;
            }
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
