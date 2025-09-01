#ifndef CURSOR_H
#define CURSOR_H

#include "rope.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
typedef struct Line Line;

struct Cursor {
    size_t line;
    size_t column;
    size_t desired_column;
};

struct Line {
    uint32_t length;
    int32_t idx;
    Line *next;
    Line *prev;
};

typedef struct {
    size_t *line_offset; // line_offset[i] - offset in rope of start of line i
    size_t *line_length; // lenghth in bytes of line i
    size_t line_num;     // nuber of lines of file
    size_t capacity;     // allocated capacity
} LineIndex;

size_t line_column_to_offset(LineIndex idx, size_t line, size_t column);

void offset_to_line_column(LineIndex idx, size_t byte_offset, size_t *line,
                           size_t *column);

void ensure_line_index_capacity(LineIndex *idx);

void add_line_to_index(LineIndex *idx, size_t offset, size_t length);

void delete_line_from_index(LineIndex *idx, size_t line_to_delete);

void travelse_list_and_index_lines(List *list, LineIndex *line_index);

#endif
