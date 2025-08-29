#ifndef ROPE_H
#define ROPE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Utility Macros
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define CHUNK_BASE 16

// Forward Declarations
typedef struct Node Node;
typedef struct RopeTree RopeTree;
typedef struct List List;

// RopeTree Structure
struct RopeTree {
    Node *root;
    uint32_t length;
    uint32_t height;
    uint32_t nodes_count;
};

// Node Structure
struct Node {
    uint32_t rank;
    char *data; // Only used in leaf nodes
    Node *left;
    Node *right;
};

// List of Leaf Nodes
struct List {
    Node *leaf;
    List *next;
};

// Node Creation
[[nodiscard]]
Node *create_leaf(const char *data);
[[nodiscard]]
Node *create_internal(Node *left, Node *right);

// Tree Construction & Modification
[[nodiscard]]
RopeTree *build_rope(char **chunks, size_t start, size_t end);
[[nodiscard]]
Node *_build_rope(char **chunks, size_t start, size_t end);
[[nodiscard]]
RopeTree *create_tree();
[[nodiscard]]
RopeTree *append(RopeTree *tree, char *data);
[[nodiscard]]
RopeTree *prepend(RopeTree *tree, char *data);
[[nodiscard]]
RopeTree *insert(RopeTree *tree, uint32_t idx, char *data);
[[nodiscard]]
RopeTree *rope_delete(RopeTree *tree, uint32_t start, uint32_t length);
[[nodiscard]]
Node *concat(Node *tree_1, Node *tree_2);

// Tree Traversal & Inspection
[[nodiscard]]
Node *get_last_node(RopeTree *tree);
[[nodiscard]]
Node *get_first_node(RopeTree *tree);
[[nodiscard]]
Node *get_index_node(RopeTree *tree, uint32_t *idx);
[[nodiscard]]
List *get_leaves(RopeTree *tree);
uint32_t calculate_length(Node *root);
uint32_t calculate_rank(Node *node);
int count_nodes(Node *root);
int calc_tree_height(Node *root);
bool is_tree_balanced(RopeTree *tree);

// Tree Algorithms
[[nodiscard]]
RopeTree *rebalance(List *leaves);
void split(Node *tree, uint32_t idx, Node **left, Node **right);
[[nodiscard]]
Node *copy_tree(Node *root);

// Memory Management
void free_tree(Node *root);
void free_internal_nodes(Node *root);
void free_list(List *list);

// Math Utilities
int fibonacci(int n);
int smallest_fib_GE(int n);

// File operations
void save_to_file(Node *root, FILE *fp);

// Debugging & Visualization
void print_RT(char *prefix, const Node *node, bool is_left);

#endif
