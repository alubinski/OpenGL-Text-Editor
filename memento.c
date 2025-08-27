#include "memento.h"
#include <stdarg.h>
#include <stddef.h>
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

    if (type == 'L') {
        char *start = *str;
        while (**str != ' ')
            (*str)++;
        ptrdiff_t len = (ptrdiff_t)(*str - start);
        char *data = (char *)malloc(len + 1);
        strncpy(data, start, len);
        data[len] = '\0';
        (*str)++;
        Node *node = create_node(data);
        free(data);
        node->rank = rank;
        return node;
    } else {
        Node *node = create_node("");
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
Memento *create_memento(RopeTree *tree) {
    Memento *m = malloc(sizeof(Memento));
    Buffer buffer;
    buffer_init(&buffer);
    serialize(tree->root, &buffer);
    m->serialized_rope = buffer.data;
    return m;
}

[[nodiscard]]
RopeTree *restore_from_memento(Memento *m) {
    RopeTree *restored = deserialize(m->serialized_rope);
    restored->nodes_count = count_nodes(restored->root);
    restored->height = calc_tree_height(restored->root);
    restored->length = get_length(restored->root);
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
