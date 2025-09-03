#include "rope.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Node *create_leaf(const char *data) {
    Node *node = malloc(sizeof(Node));
    if (!node) {
        perror("Failed to allocate leaf node");
    }

    node->rank = strlen(data);
    node->data = strndup(data, node->rank + 1);
    node->data[node->rank] = '\0';
    node->left = nullptr;
    node->right = nullptr;
    return node;
}

Node *create_internal(Node *left, Node *right) {
    Node *node = malloc(sizeof(Node));
    if (!node) {
        perror("Failed to allocate internal node");
    }
    node->data = nullptr;
    node->left = left;
    node->right = right;
    node->rank = calculate_rank(node);
    return node;
}

RopeTree *build_rope(char **chunks, size_t start, size_t end) {
    RopeTree *tree = malloc(sizeof(RopeTree));
    tree->root = _build_rope(chunks, start, end);
    tree->height = calc_tree_height(tree->root);
    tree->length = calculate_length(tree->root);
    tree->nodes_count = count_nodes(tree->root);
    return tree;
}

Node *_build_rope(char **chunks, size_t start, size_t end) {
    if (start == end) {
        return create_leaf(chunks[start]);
    }
    size_t mid = (start + end) / 2;
    Node *left = _build_rope(chunks, start, mid);
    Node *right = _build_rope(chunks, mid + 1, end);
    return create_internal(left, right);
}

RopeTree *create_tree() {
    RopeTree *tree = malloc(sizeof(RopeTree));
    tree->root = nullptr;
    tree->length = 0;
    tree->height = 0;
    tree->nodes_count = 0;
    return tree;
}

RopeTree *append(RopeTree *tree, char *data) {
    tree->nodes_count++;
    tree->length += strlen(data);
    if (!tree->root) {
        Node *new_node = create_leaf(data);
        tree->root = new_node;
        return tree;
    }

    Node *last = get_last_node(tree);
    if (strlen(data) + strlen(last->data) < CHUNK_BASE) {
        strcat(last->data, data);
        last->rank = strlen(last->data);
        return tree;
    }

    Node *new_node = create_leaf(data);
    tree->root = concat(tree->root, new_node);
    tree->nodes_count++;
    tree->height++;
    return tree;
}

RopeTree *prepend(RopeTree *tree, char *data) {
    tree->nodes_count++;
    tree->length += strlen(data);
    if (!tree->root) {
        Node *new_node = create_leaf(data);
        tree->root = new_node;
        return tree;
    }

    Node *first = get_first_node(tree);
    if (strlen(data) + strlen(first->data) < CHUNK_BASE) {
        strcat(first->data, data);
        first->rank = strlen(first->data);
        return tree;
    }

    Node *new_node = create_leaf(data);
    tree->root = concat(new_node, tree->root);
    tree->nodes_count++;
    tree->height++;
    return tree;
}

RopeTree *insert(RopeTree *tree, uint32_t idx, char *data) {
    strcat(data, "\0");
    if (idx == tree->length) {
        return append(tree, data);
    }

    if (idx == 0) {
        return prepend(tree, data);
    }
    int32_t length = tree->length + strlen(data);
    Node *new_node = create_leaf(data);
    tree->length += strlen(data);
    Node *left_tree, *right_tree;

    split(tree->root, idx, &left_tree, &right_tree);

    left_tree = concat(left_tree, new_node);

    tree->root = concat(left_tree, right_tree);

    tree->nodes_count = count_nodes(tree->root);
    tree->height = calc_tree_height(tree->root);
    tree->length = length;

    if (is_tree_balanced(tree)) {
        return tree;
    }

    List *leaves = get_leaves(tree);
    free_internal_nodes(tree->root);
    RopeTree *balanced = rebalance(leaves);
    free_list(leaves);
    leaves = nullptr;
    free(tree);
    tree = nullptr;

    balanced->length = length;
    balanced->height = calc_tree_height(balanced->root);
    balanced->nodes_count = count_nodes(balanced->root);

    return balanced;
}

RopeTree *rope_delete(RopeTree *tree, uint32_t start, uint32_t length) {
    uint32_t size = tree->length;

    if (tree->root->left == nullptr && tree->root->right == nullptr) {
        free(tree->root);
        tree->root = nullptr;
        return tree;
    }

    Node *first_split_left;
    Node *first_split_right;
    split(tree->root, start, &first_split_left, &first_split_right);

    Node *second_split__left;
    Node *second_split_right;
    split(tree->root, start + length, &second_split__left, &second_split_right);

    tree->root = concat(first_split_left, second_split_right);

    if (tree->root == nullptr) {
        return nullptr;
    }

    tree->length = size - length;
    tree->height = calc_tree_height(tree->root);
    tree->nodes_count = count_nodes(tree->root);

    if (is_tree_balanced(tree)) {
        return tree;
    }

    List *leaves = get_leaves(tree);
    free_internal_nodes(tree->root);
    RopeTree *balanced = rebalance(leaves);
    free_list(leaves);
    leaves = nullptr;
    free(tree);
    tree = nullptr;

    balanced->length = size - length;
    balanced->height = calc_tree_height(balanced->root);
    balanced->nodes_count = count_nodes(balanced->root);

    return balanced;
}

Node *concat(Node *tree_1, Node *tree_2) {
    Node *new_root = create_internal(tree_1, tree_2);
    return new_root;
}

Node *get_last_node(RopeTree *tree) {
    Node *current = tree->root;
    while (current->right) {
        current = current->right;
    }
    return current;
}

Node *get_first_node(RopeTree *tree) {
    Node *current = tree->root;
    while (current->left) {
        current = current->left;
    }
    return current;
}

Node *get_index_node(RopeTree *tree, uint32_t *idx) {
    Node *current = tree->root;
    while (current) {
        if (current->rank == *idx) {
            // possible bug with rankings
            if (current->right)
                return current->right;
            return current;
        }
        if (current->rank <= *idx) {
            if (!current->right)
                break;
            *idx -= current->rank;
            current = current->right;
        } else {
            if (!current->left)
                break;
            current = current->left;
        }
    }
    return current;
}

List *get_leaves(RopeTree *tree) {
    if (!tree || !tree->root)
        return nullptr;

    List *leaves = nullptr;
    List *leaves_start = nullptr;
    int32_t stack_capacity = tree->nodes_count;
    Node **stack = malloc(stack_capacity * sizeof(*stack));
    Node *current = tree->root;
    size_t counter = 0;
    size_t stack_index = 0;

    while (1) {
        while (current) {
            if (stack_index >= stack_capacity) {
                stack_capacity *= 2;
                stack = realloc(stack, stack_capacity * sizeof(*stack));
            }
            stack[stack_index++] = current;
            current = current->left;
        }
        if (stack_index) {
            current = stack[--stack_index];

            if (current->left == nullptr && current->right == nullptr) {
                List *new_leaf = malloc(sizeof(List));
                new_leaf->leaf = current;
                new_leaf->next = nullptr;

                if (!leaves_start) {
                    leaves_start = new_leaf;
                    leaves = new_leaf;
                } else {
                    leaves->next = new_leaf;
                    leaves = new_leaf;
                }
            }
            current = current->right;
        } else {
            break;
        }
    }
    free(stack);

    return leaves_start;
}

uint32_t calculate_length(Node *root) {
    if (!root)
        return 0;
    if (!root->left && !root->right) {
        return strlen(root->data);
    }
    return calculate_length(root->left) + calculate_length(root->right);
}

uint32_t calculate_rank(Node *node) {
    Node *current = node->left;
    uint32_t rank = 0;
    while (current) {
        rank += current->rank;
        current = current->right;
    }
    return rank;
}

int count_nodes(Node *root) {
    if (!root)
        return 0;
    return 1 + count_nodes(root->left) + count_nodes(root->right);
}

int calc_tree_height(Node *root) {
    if (root == nullptr)
        return 0;
    int leftHeight = calc_tree_height(root->left);
    int rightHeight = calc_tree_height(root->right);
    return 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
}

bool is_tree_balanced(RopeTree *tree) {
    if (tree->height >= smallest_fib_GE(tree->nodes_count)) {
        return false;
    }
    return fibonacci(tree->height + 2) <= tree->root->rank ? true : false;
}

RopeTree *rebalance(List *leaves) {
    RopeTree *rebalanced_tree = create_tree();
    if (!leaves || !leaves->leaf)
        return rebalanced_tree;

    Node *current_root = leaves->leaf;
    leaves = leaves->next;

    while (leaves) {
        if (!leaves->leaf) {
            leaves = leaves->next;
            continue;
        }
        current_root = concat(current_root, leaves->leaf);
        leaves = leaves->next;
    }

    rebalanced_tree->root = current_root;
    return rebalanced_tree;
}

void split(Node *node, uint32_t idx, Node **left, Node **right) {
    if (!node) {
        *left = *right = nullptr;
        return;
    }

    if (!node->left && !node->right) {
        if (idx >= node->rank) {
            *left = node;
            *right = nullptr;
        } else {
            char *left_str = strndup(node->data, idx);
            char *right_str = strdup(node->data + idx);
            *left = strlen(left_str) > 0 ? create_leaf(left_str) : nullptr;
            *right = strlen(right_str) > 0 ? create_leaf(right_str) : nullptr;
            free(left_str);
            free(right_str);
        }
        return;
    }

    if (idx < node->rank) {
        Node *left_left, *left_right;
        split(node->left, idx, &left_left, &left_right);
        Node *new_right = concat(left_right, node->right);
        *left = left_left;
        *right = new_right;
    } else {
        Node *right_left, *right_right;
        split(node->right, idx - node->rank, &right_left, &right_right);
        Node *new_left = concat(node->left, right_left);
        *left = new_left;
        *right = right_right;
    }
}

Node *copy_tree(Node *root) {
    if (!root)
        return nullptr;
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) {
        perror("Failed to allocate mememory for tree copy");
        return nullptr;
    }

    // is leaf
    if (!root->left && !root->right) {
        new_node->data = root->data ? strdup(root->data) : nullptr;
        new_node->left = nullptr;
        new_node->right = nullptr;
    } else { // is internal
        new_node->data = nullptr;
        new_node->left = copy_tree(root->left);
        new_node->right = copy_tree(root->right);
    }

    return new_node;
}

void free_tree(Node *root) {
    if (root) {
        return;
    }
    Node *tmp = root;
    free_tree(root->left);
    free_tree(root->right);
    free(tmp);
}

void free_internal_nodes(Node *root) {
    if (root) {
        return;
    }
    Node *tmp = root;
    free_tree(root->left);
    free_tree(root->right);
    if (tmp->left && tmp->right)
        free(tmp);
}

void free_list(List *list) {
    while (list) {
        List *tmp = list;
        list = list->next;
        free(tmp);
    }
}

void print_RT(char *prefix, const Node *node, bool is_left) {
    if (node) {
        printf("%s", prefix);
        printf("%s", (is_left ? "├──" : "└──"));
        printf("%d %s %p\n", node->rank, node->data, node);

        size_t len = strlen(prefix) + 5;
        char *new_prefix = malloc(len);
        snprintf(new_prefix, len, "%s%s", prefix, is_left ? "│   " : "    ");

        print_RT(new_prefix, node->left, true);
        print_RT(new_prefix, node->right, false);

        free(new_prefix);
    }
}

void save_to_file(Node *root, FILE *fp) {
    if (!root || !fp) {
        return;
    }
    if (!root->left && !root->right) {
        fputs(root->data, fp);
    } else {
        save_to_file(root->left, fp);
        save_to_file(root->right, fp);
    }
}

// Function to generate Fibonacci numbers up to a limit
int fibonacci(int n) {
    if (n <= 1)
        return n;
    int a = 0, b = 1, c;
    for (int i = 2; i <= n; i++) {
        c = a + b;
        a = b;
        b = c;
    }
    return b;
}

// Function to find the smallest Fibonacci number >= n
int smallest_fib_GE(int n) {
    int i = 0;
    while (fibonacci(i) < n) {
        i++;
    }
    return fibonacci(i);
}
