#include <stdio.h>
#include <stdlib.h>

#define n 5

// structure for a doubly linked list node
typedef struct DoubleNode {
    int data;
    struct DoubleNode *next;
    struct DoubleNode *prev;
} DoubleNode;

// structure for the pointers to the first and last nodes of the doubly linked list
typedef struct DoublePointer {
    DoubleNode *first;
    DoubleNode *last;
} DoublePointer;

// function to initiailze the doubly linked list pointers
void initialize(DoublePointer *l) {
    l->first=NULL;
    l->last=NULL;
}

void remove_first_node(DoublePointer *l) {
    DoubleNode *temp=l->first;
    if (!temp) return;
    l->first=temp->next;
    if (l->first) l->first->prev=NULL;
    else l->last=NULL;
    free(temp);
}

void remove_last_node(DoublePointer *l) {
    DoubleNode *temp=l->last;
    if (!temp) return;
    l->last=temp->prev;
    if (l->last) l->last->next=NULL;
    else l->first=NULL;
    free(temp);
}

void add_end_node(DoublePointer *l, int value) {
    DoubleNode *new_node=malloc(sizeof(DoubleNode));
    if (!new_node) exit(EXIT_FAILURE);
    new_node->data=value;
    new_node->prev=l->last;
    new_node->next=NULL;
    if (l->last) l->last->next=new_node;
    else l->first=new_node;
    l->last=new_node;
}

void add_beginning_node(DoublePointer *l, int value) {
    DoubleNode *new_node=malloc(sizeof(DoubleNode));
    if (!new_node) exit(EXIT_FAILURE);
    new_node->data=value;
    new_node->next=l->first;
    new_node->prev=NULL;
    if (l->first) l->first->prev=new_node;
    else l->last=new_node;
    l->first=new_node;
}

void print_list(DoublePointer l) { 
    DoubleNode *current=l.first;
    while (current) {
        printf("%p %d \n",current,current->data);
        current=current->next;
    }
}

int main() {
    DoublePointer l;
    initialize(&l);
    for (int i=n-1;i>=0;i--) {
        add_beginning_node(&l,i);
    }

    printf("List after adding nodes at the beginning: \n");
    print_list(l);

    add_end_node(&l,99);
    printf("\n List after adding a node at the end: \n");
    print_list(l);

    remove_first_node(&l);
    printf("\n List after removing the first node: \n");
    print_list(l);

    remove_last_node(&l);
    printf("\n List after removing the last node: \n");
    print_list(l);

    add_beginning_node(&l,100);
    printf("\n List after adding a node at the beginning: \n");
    print_list(l);  

    return 0;
}
