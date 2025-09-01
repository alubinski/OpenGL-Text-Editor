#include "cursor.h"
#include <rope.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t line_column_to_offset(LineIndex idx, size_t line, size_t column) {
    return idx.line_offset[line] + column;
}

void offset_to_line_column(LineIndex idx, size_t byte_offset, size_t *line,
                           size_t *column) {
    for (size_t i = 0; i < idx.line_num; ++i) {
        if (i == idx.line_num - 1 || byte_offset < idx.line_offset[i + 1]) {
            *line = i;
            *column = byte_offset - idx.line_offset[i];
            return;
        }
    }
}

void ensure_line_index_capacity(LineIndex *idx) {
    if (idx->line_num >= idx->capacity) {
        size_t new_cap = idx->capacity ? idx->capacity * 2 : 8;
        idx->line_offset = realloc(idx->line_offset, new_cap * sizeof(size_t));
        idx->line_length = realloc(idx->line_length, new_cap * sizeof(size_t));
        idx->capacity = new_cap;
    }
}

void add_line_to_index(LineIndex *idx, size_t offset, size_t length) {
    ensure_line_index_capacity(idx);
    idx->line_offset[idx->line_num] = offset;
    idx->line_length[idx->line_num] = length;
    idx->line_num++;
}

void delete_line_from_index(LineIndex *idx, size_t line_to_delete) {
    if (line_to_delete >= idx->line_num) {
        perror("Line to delete is out of bound");
        return;
    }

    for (size_t i = line_to_delete; i + 1 < idx->line_num; ++i) {
        idx->line_offset[i] = idx->line_offset[i + 1];
        idx->line_length[i] = idx->line_length[i + 1];
    }
    idx->line_num--;
}

void travelse_list_and_index_lines(List *list, LineIndex *line_index) {
    line_index->line_num = 0;
    size_t offset = 0;
    size_t curr_line_length = 0;
    while (list) {
        for (size_t i = 0; i < strlen(list->leaf->data); ++i) {
            char c = list->leaf->data[i];
            curr_line_length++;
            if (c == '\n') {
                add_line_to_index(line_index, offset, curr_line_length);
                offset += curr_line_length;
                curr_line_length = 0;
            }
        }
        list = list->next;
    }
    if (curr_line_length) {
        add_line_to_index(line_index, offset, curr_line_length);
    }
}
