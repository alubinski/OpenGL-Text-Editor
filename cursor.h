#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>
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

#endif
