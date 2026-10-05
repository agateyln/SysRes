#include <stdio.h>
#include <stdlib.h>

#define n 5

// This file keeps the same structure as q3_10.c but implements a circular doubly linked list

// structure for a doubly linked list node
typedef struct DoubleNode {
    int data;
    struct DoubleNode *next;
    struct DoubleNode *prev;
} DoubleNode;

// structure for the pointers to the first and last nodes of the list
typedef struct DoublePointer {
    DoubleNode *first;
    DoubleNode *last;
} DoublePointer;

void initialize(DoublePointer *l) {
    l->first = NULL;
    l->last = NULL;
}

void remove_first_node(DoublePointer *l) {
    DoubleNode *temp = l->first;
    if (!temp) return;
    if (l->first == l->last) { // if there is only one node in the list, the node points to itself, so we set both first and last to NULL
        l->first = NULL;
        l->last = NULL;
    } else {
        l->first = temp->next;
        l->first->prev = l->last; // update the last node's next pointer to point to the new first node
        l->last->next = l->first; // this is where the circularity is maintained: the last node points to the first node and the first node points to the last
    }
    free(temp);
}

void remove_last_node(DoublePointer *l) {
    DoubleNode *temp = l->last;
    if (!temp) return;
    if (l->first == l->last) {
        l->first = NULL;
        l->last = NULL;
    } else {
        l->last = temp->prev;
        l->last->next = l->first;
        l->first->prev = l->last;
    }
    free(temp);
}

void add_end_node(DoublePointer *l, int value) {
    DoubleNode *new_node = malloc(sizeof(DoubleNode));
    if (!new_node) exit(EXIT_FAILURE);
    new_node->data = value;
    if (!l->first) { // if the list is empty, the new node points to itself and becomes the first and last node
        new_node->next = new_node;
        new_node->prev = new_node;
        l->first = new_node;
        l->last = new_node;
    } else {
        new_node->prev = l->last;
        new_node->next = l->first;
        l->last->next = new_node;
        l->first->prev = new_node;
        l->last = new_node;
    }
}

void add_beginning_node(DoublePointer *l, int value) {
    DoubleNode *new_node = malloc(sizeof(DoubleNode));
    if (!new_node) exit(EXIT_FAILURE);
    new_node->data = value;
    if (!l->first) {
        new_node->next = new_node;
        new_node->prev = new_node;
        l->first = new_node;
        l->last = new_node;
    } else {
        new_node->next = l->first;
        new_node->prev = l->last; // here, the new first node points to the last node as its previous node
        l->last->next = new_node; // the last node points to the new first node as its next node
        l->first->prev = new_node;
        l->first = new_node;
    }
}

void print_list(DoublePointer l) {
    DoubleNode *current = l.first;
    if (!current) return;
    do { // do-while loop: print the nodes in the list until we reach the first node again, meaning that we did a full circle
        printf("%p %d\n", (void *) current, current->data);
        current = current->next;
    } while (current != l.first);
}

int main() {
    DoublePointer l;
    initialize(&l);
    for (int i = n - 1; i >= 0; i--) {
        add_beginning_node(&l, i);
    }

    printf("List after adding nodes at the beginning:\n");
    print_list(l);

    add_end_node(&l, 99);
    printf("\nList after adding a node at the end:\n");
    print_list(l);

    remove_first_node(&l);
    printf("\nList after removing the first node:\n");
    print_list(l);

    remove_last_node(&l);
    printf("\nList after removing the last node:\n");
    print_list(l);

    add_beginning_node(&l, 100);
    printf("\nList after adding a node at the beginning:\n");
    print_list(l);

    while (l.first) {
        remove_first_node(&l);
    }

    return 0;
}
