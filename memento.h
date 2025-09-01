#ifndef MEMENTO_h
#define MEMENTO_h

#include "cursor.h"
#include "rope.h"
#include <X11/X.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Memento {
    char *serialized_rope;
} Memento;

typedef struct Caretaker {
    Memento **history;
    size_t size;
    size_t capacity;
} Caretaker;

typedef struct Buffer {
    char *data;
    size_t length;
    size_t capacity;
} Buffer;

void serialize(Node *root, Buffer *buffer);

Node *deserialize_heler(char **str);

RopeTree *deserialize(char *str);

Memento *create_memento(RopeTree *tree, Line *head);

RopeTree *restore_from_memento(Memento *m, Line **head);

Caretaker *create_caretaker(size_t capacity);

void save_memento(Caretaker *c, Memento *m);

Memento *get_memento(Caretaker *c, size_t idx);

Memento *pop_memento(Caretaker *c);

void clear_caretaker(Caretaker *c);

#endif
