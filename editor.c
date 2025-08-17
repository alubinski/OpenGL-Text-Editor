#include "data_structure.h"
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

XIM xim;
XIC xic;

int32_t line_cursor;
int32_t line_cursor_prev = INT_MIN;

RnFont *_font;

struct State _state;

Line *lines, *current_line;

void create_gl_context() {
  int screen_id = DefaultScreen(_state.dsp);

  int attribs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};

  XVisualInfo *visual_info = glXChooseVisual(_state.dsp, screen_id, attribs);
  if (!visual_info) {
    fprintf(stderr, "No suitable Opengl visual found\n");
    return;
  }

  _state.gl_context = glXCreateContext(_state.dsp, visual_info, NULL, GL_TRUE);
  if (!_state.gl_context) {
    fprintf(stderr, "OpenGL cannot be created\n");
    return;
  }
}

RnTextProps render_text(RnState *state, const char *text, RnFont *font,
                        vec2s pos, RnColor color, int32_t cursor, bool render) {

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
    RnGlyph glyph =
        rn_glyph_from_codepoint(state, font, hb_text->glyph_info[i].codepoint);

    uint32_t text_length = strlen(text);
    uint32_t codepoint =
        rn_utf8_to_codepoint(text, hb_text->glyph_info[i].cluster, text_length);
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

    // If the glyph is not within the font, dont render it
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

      if (i == cursor) {
        rn_rect_render(state, pos, (vec2s){1, font->size}, RN_WHITE);
      }
    }

    if (glyph.height > textheight) {
      textheight = glyph.height;
    }

    // Advance to the next glyph
    pos.x += x_advance;
    pos.y += y_advance;
  }

  if (cursor == strlen(text)) {
    rn_rect_render(state, pos, (vec2s){1, font->size}, RN_WHITE);
  }

  return (RnTextProps){
      .width = pos.x - start_pos.x, .height = textheight, .paragraph_pos = pos};
}

void render(uint32_t render_w, uint32_t render_h) {
  glClear(GL_COLOR_BUFFER_BIT);
  vec4s clear_color = rn_color_to_zto(rn_color_from_hex(0x282828));
  glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

  rn_resize_display(_state.render_state, render_w, render_h);

  rn_begin(_state.render_state);

  float y = 20;
  for (Line *l = lines; l != NULL; l = l->next) {
    render_text(_state.render_state, l->data, _font, (vec2s){20, y}, RN_WHITE,
                l == current_line ? line_cursor : -1, True);
    y += _font->size;
  }

  rn_end(_state.render_state);

  glXSwapBuffers(_state.dsp, _state.win);
}

int main() {
  _state.dsp = XOpenDisplay(0);

  xim = XOpenIM(_state.dsp, NULL, NULL, NULL);
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
                  XNClientWindow, _state.win, NULL);

  XFlush(_state.dsp);

  Atom atom_delete_window = XInternAtom(_state.dsp, "WM_DELETE_WINDOW", True);
  if (!XSetWMProtocols(_state.dsp, _state.win, &atom_delete_window, 1)) {
    printf("Couldn't register WM_DELETE_WINDOW property \n");
  }

  create_gl_context();

  glXMakeCurrent(_state.dsp, _state.win, _state.gl_context);

  _state.render_state =
      rn_init(window_x, window_height, (RnGLLoader)glXGetProcAddressARB);

  _font = rn_load_font(_state.render_state, "./Iosevka-Regular.ttf", font_size);

  current_line = line_append(&lines, "");

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
        if (line_cursor == strlen(current_line->data)) {
          current_line = line_add_after(lines, current_line, "");
        } else {
          uint32_t new_line_len = strlen(current_line->data) - line_cursor;
          char *new_line = malloc(new_line_len + 1);
          strcpy(new_line, current_line->data + line_cursor);
          current_line->data[line_cursor] = '\0';
          current_line = line_add_after(lines, current_line, new_line);
          free(new_line);
        }
        line_cursor = 0;
        line_cursor_prev = line_cursor;
        render(window_width, window_height);
        break;
      }
      if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Left)) {
        if (line_cursor - 1 >= 0) {
          line_cursor--;
        } else if (current_line->prev) {
          current_line = current_line->prev;
          line_cursor = strlen(current_line->data);
        }
        line_cursor_prev = line_cursor;
        render(window_width, window_height);
        break;
      }
      if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Right)) {
        if (line_cursor + 1 <= strlen(current_line->data)) {
          line_cursor++;
        } else if (current_line->next) {
          current_line = current_line->next;
          line_cursor = 0;
        }
        line_cursor_prev = line_cursor;
        render(window_width, window_height);
        break;
      }
      if (event->keycode == XKeysymToKeycode(_state.dsp, XK_BackSpace)) {
        if (line_cursor - 1 >= 0) {
          line_remove_at(current_line, --line_cursor);
        } else if (current_line->prev) {
          current_line = current_line->prev;
          line_cursor = strlen(current_line->data);
          if (strlen(current_line->next->data) != 0)
            current_line->data =
                strcat(current_line->data, current_line->next->data);

          line_remove(&lines, current_line->next);
        }
        line_cursor_prev = line_cursor;
        render(window_width, window_height);
        break;
      }
      if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Up)) {
        if (current_line->prev)
          current_line = current_line->prev;
        line_cursor = strlen(current_line->data) < line_cursor_prev
                          ? strlen(current_line->data)
                          : line_cursor_prev;

        render(window_width, window_height);
        break;
      }
      if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Down)) {
        if (current_line->next)
          current_line = current_line->next;
        line_cursor = strlen(current_line->data) < line_cursor_prev
                          ? strlen(current_line->data)
                          : line_cursor_prev;
        render(window_width, window_height);
        break;
      }
      if (event->keycode == XKeysymToKeycode(_state.dsp, XK_plus) &&
          event->state & ControlMask) {
        if (font_size + 6 <= 90) {
          font_size += 6;
          // WORKAROUND - rn_set_font_size causes segmentation fault
          _font = rn_load_font(_state.render_state, "./Iosevka-Regular.ttf",
                               font_size);
        }
        render(window_width, window_height);
        break;
      }
      if (event->keycode == XKeysymToKeycode(_state.dsp, XK_minus) &&
          event->state & ControlMask) {
        if (font_size - 6 >= 6) {
          font_size -= 6;
          // WORKAROUND - rn_set_font_size causes segmentation fault
          _font = rn_load_font(_state.render_state, "./Iosevka-Regular.ttf",
                               font_size);
        }
        render(window_width, window_height);
        break;
      }

      KeySym key_sym;
      char utf8_str[32];
      Status status;
      int len_utf8_str = Xutf8LookupString(
          xic, event, utf8_str, sizeof(utf8_str) - 1, &key_sym, &status);
      utf8_str[len_utf8_str] = '\0';

      if (len_utf8_str != 0)
        line_insert(current_line, utf8_str, line_cursor++);
      line_cursor_prev = line_cursor;
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