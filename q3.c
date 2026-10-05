#include <stdio.h>
#include <stdlib.h>

#define n 5

typedef struct Node {
    int data;
    struct Node *next;
} Node;

Node *head = NULL;

void add_node(Node **p,int value) {
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) exit(EXIT_FAILURE); // check for malloc failure
    new_node->data = value;
    new_node->next = *p;
    *p = new_node;
}

void print_list(Node *p) {
    while (p) {
        printf("%d -> ",p->data);
        p = p->next;
    }
}

int length(Node *p) {
    int count = 0;
    while (p) {
        count++;
        p = p->next;
    }
    return count;
}

void remove_first_node(Node **p) {
    if (*p) {
        Node *temp = *p;
        *p = (*p)->next;
        free(temp);
    }
}

void remove_last_node(Node **p) {
    if (*p) {
        if ((*p)->next == NULL) {
            free(*p);
            *p = NULL;
        } else {
            Node *current = *p;
            while (current->next->next) {
                current = current->next;
            }
            free(current->next);
            current->next = NULL;
        }
    }
}

int main() {
    for (int i=n-1;i>=0;i--) {
        add_node(&head,i);
    }
    print_list(head);
    printf("\nLength of the list: %d\n", length(head)); 

    remove_first_node(&head);
    print_list(head);
    printf("\nLength of the list after removing first node: %d\n", length(head));

    remove_last_node(&head);
    print_list(head);
    printf("\nLength of the list after removing last node: %d\n", length(head));

    return 0;
}
