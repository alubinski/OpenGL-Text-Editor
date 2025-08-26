#include "rope.h"
#include <stddef.h>

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

Memento *create_memento(RopeTree *tree);

RopeTree *restore_from_memento(Memento *m);

Caretaker *create_caretaker(size_t capacity);

void save_memento(Caretaker *c, Memento *m);

Memento *get_memento(Caretaker *c, size_t idx);
