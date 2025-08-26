#include "rope.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

RopeTree *create_tree() {
    RopeTree *tree = malloc(sizeof(RopeTree));
    tree->root = NULL;
    tree->length = 0;
    tree->height = 0;
    tree->nodes_count = 0;
    return tree;
}

Node *create_node(const char *value) {
    Node *node = malloc(sizeof(Node));
    // strncpy(node->data, value, sizeof(node->data) - 1);
    node->data = strdup(value);
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    node->rank = strlen(value);
    return node;
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
    current_root->parent = NULL;
    leaves = leaves->next;

    while (leaves) {
        if (!leaves->leaf) {
            leaves = leaves->next;
            continue;
        }
        leaves->leaf->parent = NULL;
        current_root = concat(current_root, leaves->leaf);
        leaves = leaves->next;
    }

    rebalanced_tree->root = current_root;
    return rebalanced_tree;
}

Node *copy_tree(Node *root) {
    if (!root)
        return NULL;

    Node *new_node = create_node(root->data);
    new_node->rank = root->rank;
    new_node->parent = root->parent;
    new_node->left = copy_tree(root->left);
    new_node->right = copy_tree(root->right);

    return new_node;
}

void split(RopeTree *tree, uint32_t idx, RopeTree **split_tree_1,
           RopeTree **split_tree_2) {
    Node *current = get_index_node(tree, &idx);
    if (!current) {
        return;
    }
    printf("Splitting on node %p\n", current);
    uint32_t weight_to_substract = current->rank;

    *split_tree_1 = create_tree();
    *split_tree_2 = create_tree();

    while (current) {

        if (current->right) {
            Node *node_to_concat = current->right;

            if (!(*split_tree_2)->root) {
                current->right->parent = NULL;
                current->right = NULL;
                (*split_tree_2)->root = node_to_concat;
                current = current->parent;
                continue;
            }

            if (current->parent && current == current->parent->right) {
                current = current->parent->parent;
                continue;
            }

            if (current->parent)
                current->parent->rank -= weight_to_substract;
            current->right->parent = NULL;
            current->right = NULL;

            (*split_tree_2)->root =
                concat((*split_tree_2)->root, node_to_concat);
        }
        current = current->parent;

        // skipping next node
        // if (current->parent && current == current->parent->right) {
        //     current = current->parent;
        // }
        // current = current->parent;
        // if (current) {
        //     current->rank -= weight_to_substract;
        //     if (current->rank <= 0) {
        //         if (current->parent)
        //             current->parent->left = current->left;
        //         current->left->parent = current->parent;
        //     }
        // }
    }
    (*split_tree_1)->root = copy_tree(tree->root);
}

void split_rec(Node *node, uint32_t idx, Node **left, Node **right) {
    if (!node) {
        *left = *right = NULL;
        return;
    }

    if (!node->left && !node->right) {
        if (idx >= node->rank) {
            *left = node;
            *right = NULL;
        } else {
            char *left_str = strndup(node->data, idx);
            char *right_str = strdup(node->data + idx);
            *left = strlen(left_str) > 0 ? create_node(left_str) : NULL;
            *right = strlen(right_str) > 0 ? create_node(right_str) : NULL;
            free(left_str);
            free(right_str);
        }
        return;
    }

    if (idx < node->rank) {
        Node *left_left, *left_right;
        split_rec(node->left, idx, &left_left, &left_right);
        Node *new_right = concat(left_right, node->right);
        *left = left_left;
        *right = new_right;
    } else {
        Node *right_left, *right_right;
        split_rec(node->right, idx - node->rank, &right_left, &right_right);
        Node *new_left = concat(node->left, right_left);
        *left = new_left;
        *right = right_right;
    }
}

Node *concat(Node *tree_1, Node *tree_2) {
    Node *new_root = create_node("");
    new_root->left = tree_1;
    new_root->right = tree_2;
    if (tree_1)
        tree_1->parent = new_root;
    if (tree_2)
        tree_2->parent = new_root;
    new_root->rank = calculate_rank(new_root);

    return new_root;
}

RopeTree *prepend(RopeTree *tree, char *data) {
    tree->nodes_count++;
    tree->length += strlen(data);
    if (!tree->root) {
        Node *new_node = create_node(data);
        tree->root = new_node;
        return tree;
    }

#ifdef APPEND_TO_STRING
    Node *last = get_last_node(tree);
    if (strlen(data) + strlen(last->data) < sizeof(last->data)) {
        strcat(last->data, data);
        last->rank = strlen(last->data);
        Node *parent = last->parent;
        while (parent) {
            parent->rank = calculate_rank(parent);
            parent = parent->parent;
        }
        return;
    }
#endif

    Node *new_node = create_node(data);
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
#ifdef APPEND_TO_STRING
    Node *index_node = get_index_node(tree, &idx);
    if (strlen(data) + strlen(index_node->data) < sizeof(index_node->data)) {
        char *new_string = calloc(2 * CHUNK_BASE, sizeof(char));
        strncpy(new_string, index_node->data, idx);
        strncpy(new_string + strlen(new_string), data, strlen(data));
        strncpy(new_string + strlen(new_string), index_node->data + idx,
                strlen(index_node->data) - idx);

        strcpy(index_node->data, new_string);
        free(new_string);
        index_node->rank = strlen(index_node->data);
        return;
    }
#endif
    int32_t length = tree->length + strlen(data);
    Node *new_node = create_node(data);
    tree->length += strlen(data);
    Node *left_tree, *right_tree;

    split_rec(tree->root, idx, &left_tree, &right_tree);

    left_tree = concat(left_tree, new_node);

    tree->root = concat(left_tree, right_tree);

    tree->nodes_count = count_nodes(tree->root);
    tree->height = calc_tree_height(tree->root);
    tree->length = length;

    if (is_tree_balanced(tree)) {
        return tree;
    }

    List *leaves = get_leaves(tree);
    free_not_leaves_nodes(tree->root);
    RopeTree *balanced = rebalance(leaves);
    free_list(leaves);
    leaves = NULL;
    free(tree);
    tree = NULL;

    balanced->length = length;
    balanced->height = calc_tree_height(balanced->root);
    balanced->nodes_count = count_nodes(balanced->root);

    return balanced;
}

RopeTree *append(RopeTree *tree, char *data) {
    tree->nodes_count++;
    tree->length += strlen(data);
    if (!tree->root) {
        Node *new_node = create_node(data);
        tree->root = new_node;
        return tree;
    }

#ifdef APPEND_TO_STRING
    Node *last = get_last_node(tree);
    if (strlen(data) + strlen(last->data) < sizeof(last->data)) {
        strcat(last->data, data);
        last->rank = strlen(last->data);
        Node *parent = last->parent;
        while (parent) {
            parent->rank = calculate_rank(parent);
            parent = parent->parent;
        }
        return;
    }
#endif

    Node *new_node = create_node(data);
    tree->root = concat(tree->root, new_node);
    tree->nodes_count++;
    tree->height++;
    return tree;
}

RopeTree *rope_delete(RopeTree *tree, uint32_t start, uint32_t length) {
    uint32_t size = tree->length;
    // RopeTree *split_tree_1 = NULL;
    // RopeTree *split_tree_2 = NULL;
    // RopeTree *split_tree_3 = NULL;
    // RopeTree *split_tree_4 = NULL;

    if (tree->root->left == NULL && tree->root->right == NULL) {
        free(tree->root);
        tree->root = NULL;
        return tree;
    }

    Node *first_split_left;
    Node *first_split_right;
    split_rec(tree->root, start, &first_split_left, &first_split_right);
    print_RT("", first_split_left, false);
    print_RT("", first_split_right, false);

    Node *second_split__left;
    Node *second_split_right;
    split_rec(tree->root, start + length, &second_split__left,
              &second_split_right);

    tree->root = concat(first_split_left, second_split_right);

    // RopeTree *tree_copy = create_tree();
    // tree_copy->root = copy_tree(tree->root);

    // print_RT("", tree->root, false);
    // split(tree, start, &split_tree_1, &split_tree_2);
    //
    // print_RT("", tree_copy->root, false);
    // split(tree_copy, start + length, &split_tree_3, &split_tree_4);
    //
    // post_order_delete(tree->root);
    // free(tree);
    // tree = NULL;
    // // if (split_tree_2->root) {
    // //     post_order_delete(split_tree_2->root);
    // //     free(split_tree_2);
    // //     split_tree_2 = NULL;
    // // }
    // //
    // // if (split_tree_3->root) {
    //     post_order_delete(split_tree_3->root);
    //     // Gives Exception Why?
    //     // free(split_tree_3);
    //     split_tree_3 = NULL;
    // }

    // tree = create_tree();
    // if (split_tree_1 && split_tree_4) {
    //     tree->root = concat(split_tree_1->root, split_tree_4->root);
    // } else if (split_tree_1) {
    //     tree->root = split_tree_1->root;
    // } else if (split_tree_4) {
    //     tree->root = split_tree_4->root;
    // }
    //
    if (tree->root == NULL) {
        return NULL;
    }

    tree->length = size - length;
    tree->height = calc_tree_height(tree->root);
    tree->nodes_count = count_nodes(tree->root);

    if (is_tree_balanced(tree)) {
        return tree;
    }

    List *leaves = get_leaves(tree);
    free_not_leaves_nodes(tree->root);
    RopeTree *balanced = rebalance(leaves);
    free_list(leaves);
    leaves = NULL;
    free(tree);
    tree = NULL;

    balanced->length = size - length;
    balanced->height = calc_tree_height(balanced->root);
    balanced->nodes_count = count_nodes(balanced->root);

    return balanced;
}

void free_not_leaves_nodes(Node *root) {
    if (root || (root->left != NULL && root->right != NULL)) {
        return;
    }
    Node *tmp = root;
    free_not_leaves_nodes(root->left);
    free_not_leaves_nodes(root->right);
    free(tmp);
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

void in_order_printf(RopeTree *tree) {
    if (!tree)
        return;

    Node **stack = malloc(tree->length * sizeof(*stack));
    Node *current = tree->root;
    size_t counter = 0;
    size_t stack_index = 0;

    while (current || stack_index) {
        while (current) {
            stack[stack_index++] = current;
            current = current->left;
        }
        if (stack_index) {
            current = stack[--stack_index];

            printf("Node rank: %d, strlen(data\\): %ld,  data: %s\n",
                   current->rank, strlen(current->data), current->data);
            current = current->right;
        }
    }
    free(stack);
}

char *in_order(RopeTree *tree) {
    if (!tree)
        return NULL;

    char *text = calloc(tree->length * 2 * CHUNK_BASE, sizeof(char));
    Node **stack = malloc(tree->length * sizeof(*stack));
    Node *current = tree->root;
    size_t counter = 0;
    size_t stack_index = 0;

    while (current || stack_index) {
        while (current) {
            stack[stack_index++] = current;
            current = current->left;
        }
        if (stack_index) {
            current = stack[--stack_index];
            if (current->left == NULL && current->right == NULL) {
                for (int i = 0; i < 2 * CHUNK_BASE; i++) {
                    if (current->data[i] == '\0')
                        break;
                    text[counter++] = current->data[i];
                }
            }
            current = current->right;
        }
    }
    free(stack);
    return text;
}

Node *get_last_node(RopeTree *tree) {
    Node *current = tree->root;
    while (current->right) {
        current = current->right;
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
        return NULL;

    if (tree->length == 1) {
        List *new_leaf = malloc(sizeof(List));
        new_leaf->leaf = tree->root;
        new_leaf->next = NULL;
        return new_leaf;
    }

    List *leaves = NULL;
    List *leaves_start = NULL;
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

            if (current->left == NULL && current->right == NULL) {
                List *new_leaf = malloc(sizeof(List));
                new_leaf->leaf = current;
                new_leaf->next = NULL;

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

void free_list(List *list) {
    while (list) {
        List *tmp = list;
        // printf("Freeing node at %p\n", (void *)tmp);
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

int count_nodes(Node *root) {
    if (!root)
        return 0;
    return 1 + count_nodes(root->left) + count_nodes(root->right);
}

int calc_tree_height(Node *root) {
    if (root == NULL)
        return 0;
    int leftHeight = calc_tree_height(root->left);
    int rightHeight = calc_tree_height(root->right);
    return 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
}

void post_order_delete(Node *root) {
    if (root) {
        return;
    }
    Node *tmp = root;
    post_order_delete(root->left);
    post_order_delete(root->right);
    free(tmp);
    // unsigned n = tree->nodes_count; // unsigned n = tree->length;
    // Node **stack =
    //     malloc(n * sizeof(*stack)); // stack contains pointers to nodes
    // unsigned size = 0;              // current stack size
    // char *boolStack = malloc(
    //     n * sizeof(*boolStack)); // For each element on the node stack, a
    //                              // corresponding value is stored on the bool
    //                              // stack. If this value is true, then we
    //                              need
    //                              // to pop and visit the node on next
    //                              encounter.
    // unsigned boolSize = 0;
    // char alreadyEncountered; // boolean
    // Node *current = tree->root;
    // while (current) {
    //     stack[size++] = current;
    //     boolStack[boolSize++] = 0; // false
    //     current = current->left;
    // }
    // while (size) {
    //     current = stack[size - 1];
    //     alreadyEncountered = boolStack[boolSize - 1];
    //     if (alreadyEncountered) {
    //         free(current); // visit()
    //         size--;
    //         boolSize--;
    //     } else {
    //         boolSize--;
    //         boolStack[boolSize++] = 1; // true
    //         current = current->right;
    //         while (current) {
    //             stack[size++] = current;
    //             boolStack[boolSize++] = 0; // false
    //             current = current->left;
    //         }
    //     }
    // }
    // tree->root = NULL;
    // tree->length = 0;
    // free(stack);
    // free(boolStack);
}
