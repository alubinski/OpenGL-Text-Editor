#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

int main() {
  Display *main_display = XOpenDisplay(0);
  Window root_window = DefaultRootWindow(main_display);

  int window_x = 0;
  int window_y = 0;
  int window_width = 800;
  int window_height = 600;
  int border_width = 0;
  int window_depth = CopyFromParent;
  int window_class = CopyFromParent;
  Visual *window_visual = CopyFromParent;

  int attribute_value_mask = CWBackingPixel | CWEventMask;
  XSetWindowAttributes window_attributes;
  window_attributes.backing_pixel = 0xffffccaa;
  window_attributes.event_mask =
      StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;

  Window main_window =
      XCreateWindow(main_display, root_window, window_x, window_y, window_width,
                    window_height, border_width, window_depth, window_class,
                    window_visual, attribute_value_mask, &window_attributes);

  XMapWindow(main_display, main_window);
  XFlush(main_display);

  int is_window_open = 1;
  while (is_window_open) {
    XEvent general_event;
    XNextEvent(main_display, &general_event);

    switch (general_event.type) {
    case KeyPress:
    case KeyRelease: {
      XKeyPressedEvent *event = (XKeyPressedEvent *)&general_event;
      if (event->keycode == XKeysymToKeycode(main_display, XK_Escape)) {
        is_window_open = 0;
      }
    } break;
    }
  }

  return 0;
}