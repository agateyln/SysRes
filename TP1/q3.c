#include <stdio.h>
#include <stdlib.h>

#define n 5

typedef struct Node {
    int data;
    struct Node *next;
} Node;

Node *head = NULL;
Node *head2 = NULL;

void add_node(Node **p,int value) {
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) exit(EXIT_FAILURE); // check for malloc failure
    new_node->data = value;
    new_node->next = *p;
    *p = new_node;
}

void print_list(Node *p) {
    while (p) {
        printf("%p %d \n",p,p->data);
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

void add_end_node(Node **p, int value) {
    Node *new_node=malloc(sizeof(Node));
    if (!new_node) exit(EXIT_FAILURE); // check for malloc failure
    new_node->data=value;
    new_node->next=NULL;

    if (*p==NULL) {
        *p=new_node;
    } else {
        Node *current=*p;
        while (current->next) {
            current=current->next;
        }
        current->next=new_node;
    }
}
void fuse(Node **p1last, Node **p2first){
    // check for malloc failure
    if (!p1last || !p2first) {
        fprintf(stderr, "Error: Null pointer passed to fuse function\n");
        exit(EXIT_FAILURE);
    }
    while (*p1last) {
        p1last = &((*p1last)->next);
    }
    *p1last = *p2first;
}

void apply_function(Node *p, void (*func)(int *)) {
    while (p) {
        func(&(p->data));
        p = p->next;
    }
}

void double_value(int *value) {
    *value *= 2;
}

int main() {
    for (int i=n-1;i>=0;i--) {
        add_node(&head,i);
    }
    for (int i=n-1;i>=0;i--) {
        add_node(&head2,i+10);
    }
    printf("\n3.2 - Length of the list: %d\n", length(head)); 
    print_list(head);

    remove_first_node(&head);
    printf("\n3.4 -Length of the list after removing first node: %d\n", length(head));
    print_list(head);

    remove_last_node(&head);
    printf("\n3.5 -Length of the list after removing last node: %d\n", length(head));
    print_list(head);

    add_end_node(&head, 99);
    printf("\n3.6 -Length of the list after adding a node at the end: %d\n", length(head));
    print_list(head);

    add_node(&head, 100);
    printf("\n3.7 -Length of the list after adding a node at the beginning: %d\n", length(head));
    print_list(head);


    fuse(&head, &head2);
    printf("\n3.8 -Length of the list after fusing two lists: %d\n", length(head));
    print_list(head);

    apply_function(head, double_value);
    printf("\n3.9 -Length of the list after applying function to double the values: %d\n", length(head));
    print_list(head);

    
    return 0;
}
