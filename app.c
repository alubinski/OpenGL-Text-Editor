#include <X11/X.h>
#include <X11/Xlib.h>
#include <unistd.h>

int main() {
  Display *dsp = XOpenDisplay(0);

  Window rootWin = DefaultRootWindow(dsp);
  Window win =
      XCreateSimpleWindow(dsp, rootWin, 0, 0, 800, 600, 0, 0, 0x00aade87);
  XMapWindow(dsp, win);
  XFlush(dsp);

  while (1) {
    sleep(1);
  }

  return 0;
}