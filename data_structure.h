#pragma once
#include <stdint.h>

typedef struct Line Line;

struct Line {
  Line *prev;
  Line *next;
  char *data;
};

Line *line_append(Line **head, const char *text);

Line *line_add_after(Line *head, Line *before, const char *text);

void line_remove(Line **head, Line *line);

void line_insert(Line *line, char *unicode, uint32_t idx);

void line_remove_at(Line *line, uint32_t idx);