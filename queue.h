#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h>
#include <string.h>
/*
Overview of Queue Implementation
--------------------
==Reminder==
Inserts new nodes at the end of data structure. Pops at the beginning.
==USAGE==
Declare with "queue_node MyQueue;" 
Then call "init_queue(&MyQueue);"
*Make sure the queue is cleared upon exiting the scope of its declaration or it leads to memory leaks!
==FUNCTIONS==
queue_node* init_queue(queue_node*) - Initializes a queue. Required for all functions to work. Returns the queue address.
int is_empty(queue_node*) - Returns whether a given queue is empty or not. Returns 1 if empty otherwise returns 0.
const char* peek(queue_node*) - Returns the file path of the first node.
int queue_len(queue_node*) - Returns the length of the queue.
queue_node* pop_node(queue_node*) - Deletes the first node and returns the queue address.
queue_node* insert_node(queue_node*,char*,int len) - Inserts a node at the end containing a string of length len and returns the queue address.
queue_node* clear_queue(queue_node*) - Clears the entire queue. (Calls repeated pop_node() calls until is_empty() is true for the queue given)
*/


typedef struct queue_node {
    char* file_path; 
    struct queue_node* next;
} queue_node;

queue_node* init_queue(queue_node* start) { //Setups up a queue and returns itself
    start->file_path = NULL;
    start->next = NULL;
    return start;
}

int is_empty(queue_node* start) {//returns 1 if empty, else 0
    return start->file_path == NULL;
}

char* peek(queue_node* start) { //returns contents of first node
    return start->file_path;
}

int queue_len(queue_node* start) { // returns length of queue
    if(is_empty(start))
        return 0;
    int i = 0;
    queue_node* temp = start;
    while(temp != NULL) {
        i++;
        temp = temp->next;
    }
    return i;
}

char* pop_node(queue_node* start) { //deletes first node, returns char of header
    char* temp = start->file_path;
    if(start->next != NULL) {
        queue_node* temp = start->next;
        start->next = temp->next;
        start->file_path = temp->file_path;
        free(temp);
    } else { //No next node. Node size must be one or zero
        start->file_path = NULL;
    }
    return temp;
}

queue_node* insert_node(queue_node* start,char* txt,int len) { //inserts a node at the end containing txt. Returns address of first node. Len should be length of text including null terminator
    if(is_empty(start)) { //empty! Lets allocate space for the new filename in the first node
        start->file_path = malloc(len * sizeof(char));
        memcpy(start->file_path, txt, len);
    } else {
        queue_node* temp = start;
        while(temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = malloc(sizeof(queue_node));
        temp = temp->next;
        temp->next = NULL;
        temp->file_path = malloc(len * sizeof(char));
        memcpy(temp->file_path, txt, len);
    }
    return start;
}

queue_node* clear_queue(queue_node* start) { //clears an entire queue
    while(!is_empty(start)) {
        pop_node(start);
    }
    return start;
}
#endif