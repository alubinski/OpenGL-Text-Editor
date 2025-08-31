#include "memento.h"
#include <X11/X.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void buffer_init(Buffer *buff) {
    buff->capacity = 256;
    buff->length = 0;
    buff->data = malloc(buff->capacity);
    buff->data[0] = '\0';
}

void buffer_append(Buffer *buf, const char *format, ...) {
    va_list args;
    va_start(args, format);

    char temp[256];
    int written = vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);

    if (written < 0)
        return;

    if (buf->length + written + 1 >= buf->capacity) {
        buf->capacity = (buf->length + written + 1) * 2;
        buf->data = realloc(buf->data, buf->capacity);
    }

    snprintf(buf->data + buf->length, buf->capacity - buf->length, "%s", temp);
    buf->length += written;
}

void serialize(Node *root, Buffer *buffer) {
    if (!root) {
        buffer_append(buffer, "#");
        return;
    }

    // is leaf
    if (root->data) {
        buffer_append(buffer, "L %d %s ", root->rank, root->data);
    } else {
        buffer_append(buffer, "I %d ", root->rank);
        serialize(root->left, buffer);
        serialize(root->right, buffer);
    }
}

[[nodiscard]]
Node *deserialize_heler(char **str) {
    if (**str == '#') {
        *str += 2;
        return nullptr;
    }

    char type = **str;
    *str += 2;

    int rank;
    sscanf(*str, "%d", &rank);
    while (**str != ' ')
        (*str)++;
    (*str)++; // skip space

    Node *node = malloc(sizeof(Node));
    if (type == 'L') {
        char *start = *str;
        while (**str != ' ')
            (*str)++;
        ptrdiff_t len = (ptrdiff_t)(*str - start);
        char *data = (char *)malloc(len + 1);
        strncpy(data, start, len);
        data[len] = '\0';
        (*str)++;
        node->data = strdup(data);
        free(data);
        node->rank = rank;
        node->left = nullptr;
        node->right = nullptr;
        return node;
    } else {
        node->rank = rank;
        node->data = nullptr;
        node->left = deserialize_heler(str);
        node->right = deserialize_heler(str);
        return node;
    }
}

[[nodiscard]]
RopeTree *deserialize(char *str) {
    RopeTree *rope = create_tree();
    rope->root = deserialize_heler(&str);
    return rope;
}

[[nodiscard]]
Memento *create_memento(RopeTree *tree, Line *head) {
    Memento *m = malloc(sizeof(Memento));
    Buffer buffer;
    buffer_init(&buffer);
    serialize(tree->root, &buffer);
    m->serialized_rope = buffer.data;
    m->serialized_lines = malloc(128 * sizeof(uint8_t));
    m->line_buffer_size = serialize_lines(head, m->serialized_lines,
                                          128 * sizeof(m->serialized_lines));
    return m;
}

[[nodiscard]]
RopeTree *restore_from_memento(Memento *m, Line **head) {
    RopeTree *restored = deserialize(m->serialized_rope);
    restored->nodes_count = count_nodes(restored->root);
    restored->height = calc_tree_height(restored->root);
    restored->length = calculate_length(restored->root);
    *head = deserialize_lines(m->serialized_lines, m->line_buffer_size);
    return restored;
}

Caretaker *create_caretaker(size_t capacity) {
    Caretaker *c = malloc(sizeof(Caretaker));
    c->history = malloc(sizeof(Memento *) * capacity);
    c->size = 0;
    c->capacity = capacity;
    return c;
}

void save_memento(Caretaker *c, Memento *m) {
    if (c->size < c->capacity) {
        c->history[c->size++] = m;
    }
}

Memento *get_memento(Caretaker *c, size_t idx) {
    return (idx >= 0 && idx < c->size) ? c->history[idx] : nullptr;
}

Memento *pop_memento(Caretaker *c) {
    Memento *m = get_memento(c, c->size - 1);
    c->size--;
    return m;
}
void clear_caretaker(Caretaker *c) {
    // free(c->history);
    c->size = 0;
}

[[nodiscard]]
size_t serialize_lines(const Line *head, uint8_t *buffer, size_t buff_size) {
    size_t line_count = 0;
    for (const Line *cur = head; cur; cur = cur->next)
        line_count++;

    size_t required =
        sizeof(uint32_t) * line_count * (sizeof(int32_t) + sizeof(uint32_t));
    if (buff_size < required) {
        perror("Not enough space allocated to serialize lines");
        return 0;
    }

    uint32_t cnt = (uint32_t)line_count;
    memcpy(buffer, &cnt, sizeof(cnt));
    size_t offset = sizeof(cnt);

    for (const Line *cur = head; cur; cur = cur->next) {
        memcpy(buffer + offset, &cur->idx, sizeof(cur->idx));
        offset += sizeof(cur->idx);
        memcpy(buffer + offset, &cur->length, sizeof(cur->length));
        offset += sizeof(cur->length);
    }
    return offset;
}

[[nodiscard]]
Line *deserialize_lines(const uint8_t *buffer, size_t buff_size) {
    if (buff_size < sizeof(uint32_t)) {
        perror("Buff size is incorrect. Can not deserialize lines");
        return nullptr;
    }

    uint32_t count;
    memcpy(&count, buffer, sizeof(count));
    size_t offset = sizeof(count);

    if (buff_size < offset + count * (sizeof(int32_t) + sizeof(uint32_t))) {
        perror("Buff size is incorrect. Can not deserialize lines");
        return nullptr;
    }

    Line *head = nullptr, *prev = nullptr;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t idx;
        uint32_t length;
        memcpy(&idx, buffer + offset, sizeof(idx));
        offset += sizeof(idx);
        memcpy(&length, buffer + offset, sizeof(length));
        offset += sizeof(length);

        Line *line = malloc(sizeof(Line));
        if (!line) {
            while (head) {
                Line *tmp = head;
                head = head->next;
                free(tmp);
            }
            perror("Memory allocation for line failed");
            return nullptr;
        }

        line->idx = idx;
        line->length = length;
        line->next = nullptr;
        line->prev = prev;

        if (prev)
            prev->next = line;
        else
            head = line;
        prev = line;
    }
    return head;
}
