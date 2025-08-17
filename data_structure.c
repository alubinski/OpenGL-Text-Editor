#include "data_structure.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint32_t utf8_char_len(unsigned char c) {
  if (c < 0x80)
    return 1;
  if ((c & 0xE0) == 0xC0)
    return 2;
  if ((c & 0xF0) == 0xE0)
    return 3;
  if ((c & 0xF8) == 0xF0)
    return 4;
  return 0;
}

int32_t find_byte_index_by_char_index(const char *str, int32_t idx) {
  int32_t byte_idx = 0;
  int i = 0;
  while (str[byte_idx] != '\0' && i < idx) {
    byte_idx += utf8_char_len(str[byte_idx]);
    i++;
  }
  return byte_idx;
}

Line *line_append(Line **head, const char *text) {
  Line *line = malloc(sizeof(Line));
  if (!line)
    return NULL;
  line->data = strdup(text);
  if (!line->data)
    return NULL;

  line->next = NULL;
  if (!(*head)) {
    line->prev = NULL;
    *head = line;
    return line;
  }

  Line *last = *head;
  while (last->next != NULL) {
    last = last->next;
  }
  last->next = line;
  line->prev = last;
  return line;
}

Line *line_add_after(Line *head, Line *before, const char *text) {
  Line *line = malloc(sizeof(Line));
  if (!line)
    return NULL;
  line->data = strdup(text);
  if (!line->data)
    return NULL;

  before->next = line;
  line->prev = before;
  if (line->next != NULL) {
    line->next->prev = line;
  }
  return line;
}

void line_remove(Line **head, Line *line) {
  if (*head == line) {
    *head = line->next;
  }

  if (line->prev) {
    line->prev->next = line->next;
  }

  if (line->next) {
    line->next->prev = line->prev;
  }

  free(line->data);
  free(line);
}

void line_insert(Line *line, char *unicode, uint32_t idx) {
  int32_t len_text = strlen(line->data);
  int32_t len_unicode = strlen(unicode);

  line->data = realloc(line->data, len_text + len_unicode + 1);

  int32_t byte_idx = find_byte_index_by_char_index(line->data, idx);

  for (int32_t i = len_text - 1; i >= byte_idx; i--) {
    line->data[i + len_unicode] = line->data[i];
  }

  for (int32_t i = 0; i < len_unicode; i++) {
    line->data[i + byte_idx] = unicode[i];
  }

  line->data[len_text + len_unicode] = '\0';
}

void line_remove_at(Line *line, uint32_t idx) {
  int32_t byte_idx = find_byte_index_by_char_index(line->data, idx);
  int32_t unicode_len = utf8_char_len(line->data[idx]);

  int32_t i;
  for (i = byte_idx; line->data[i + unicode_len] != '\0'; i++) {
    line->data[i] = line->data[i + unicode_len];
  }

  line->data[i] = '\0';
}