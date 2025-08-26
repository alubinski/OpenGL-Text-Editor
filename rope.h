#include <stdbool.h>
#include <stdint.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct Node Node;

typedef struct RopeTree RopeTree;

typedef struct List List;

struct RopeTree {
    Node *root;
    uint32_t length;
    uint32_t height;
    uint32_t nodes_count;
};

#define CHUNK_BASE 16

struct Node {
    uint32_t rank;
    // char data[2 * CHUNK_BASE];
    char *data;
    Node *left;
    Node *right;
    Node *parent;
};

struct List {
    Node *leaf;
    List *next;
};

RopeTree *rebalance(List *leaves);

void split(RopeTree *tree, uint32_t idx, RopeTree **split_tree_1,
           RopeTree **split_tree_2);

void split_rec(Node *tree, uint32_t idx, Node **left, Node **right);

Node *concat(Node *tree_1, Node *tree_2);

RopeTree *create_tree();

Node *create_node(const char *value);

void free_not_leaves_nodes(Node *root);

RopeTree *rope_delete(RopeTree *tree, uint32_t start, uint32_t length);

RopeTree *prepend(RopeTree *tree, char *data);

RopeTree *insert(RopeTree *tree, uint32_t idx, char *data);

RopeTree *append(RopeTree *tree, char *data);

char *in_order(RopeTree *tree);

void in_order_printf(RopeTree *tree);

void post_order_free(RopeTree *tree);
uint32_t calculate_rank(Node *node);

Node *get_last_node(RopeTree *tree);

Node *get_index_node(RopeTree *tree, uint32_t *idx);

List *get_leaves(RopeTree *tree);

void free_list(List *list);

void print_RT(char *prefix, const Node *node, bool is_left);

int count_nodes(Node *root);

int calc_tree_height(Node *root);

// Returns the nth Fibonacci number
int fibonacci(int n);

// Returns the smallest Fibonacci number greater than or equal to n
int smallest_fib_GE(int n);

// Checks if the given RopeTree is balanced
bool is_tree_balanced(RopeTree *tree);

Node *copy_tree(Node *root);

void post_order_delete(Node *root);
