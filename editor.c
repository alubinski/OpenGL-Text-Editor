#include "data_structure.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cglm/types-struct.h>
#include <runara/runara.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

struct State {
  Window win;
  Display *dsp;
  GLXContext gl_context;
  RnState *render_state;
};

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

void render(uint32_t render_w, uint32_t render_h) {
  glClear(GL_COLOR_BUFFER_BIT);
  vec4s clear_color = rn_color_to_zto(rn_color_from_hex(0x282828));
  glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

  rn_resize_display(_state.render_state, render_w, render_h);

  rn_begin(_state.render_state);

  float y = 20;
  for (Line *l = lines; l != NULL; l = l->next) {

    rn_text_render(_state.render_state, l->data, _font, (vec2s){20, y},
                   RN_WHITE);
    y += _font->size;
  }

  rn_end(_state.render_state);

  glXSwapBuffers(_state.dsp, _state.win);
}

int main() {
  _state.dsp = XOpenDisplay(0);
  Window root_window = DefaultRootWindow(_state.dsp);

  int window_x = 0;
  int window_y = 0;
  int window_width = 1280;
  int window_height = 720;
  int border_width = 0;
  int window_depth = CopyFromParent;
  int window_class = CopyFromParent;
  Visual *window_visual = CopyFromParent;

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
  XFlush(_state.dsp);

  Atom atom_delete_window = XInternAtom(_state.dsp, "WM_DELETE_WINDOW", True);
  if (!XSetWMProtocols(_state.dsp, _state.win, &atom_delete_window, 1)) {
    printf("Couldn't register WM_DELETE_WINDOW property \n");
  }

  create_gl_context();

  glXMakeCurrent(_state.dsp, _state.win, _state.gl_context);

  _state.render_state =
      rn_init(window_x, window_height, (RnGLLoader)glXGetProcAddressARB);

  _font = rn_load_font(_state.render_state, "./Iosevka-Regular.ttf", 24);

  for (int i = 0; i < 5; i++) {
    current_line = line_append(&lines, "HĀllo ");
  }

  render(window_width, window_height);
  int is_window_open = 1;
  while (is_window_open) {
    XEvent general_event;
    XNextEvent(_state.dsp, &general_event);

    switch (general_event.type) {
    case KeyPress:
    case KeyRelease: {
      XKeyPressedEvent *event = (XKeyPressedEvent *)&general_event;
      if (event->keycode == XKeysymToKeycode(_state.dsp, XK_Escape)) {
        is_window_open = 0;
      }
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