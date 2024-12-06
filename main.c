#include <stdio.h>
#include <stdlib.h>


// For describing the thread
typedef struct {
    
} ROUTER_INFO;

#define VEC(typ) typedef struct { struct typ *start; size_t size; size_t cap; } typ ## _VEC; \
typ ## _VEC *typ ## _VEC_create() { \
    typ ## _VEC *vec = malloc(sizeof(*vec)); \
    vec->cap = 8; \
    vec->start = malloc(sizeof(typ) * vec->cap); \
    vec->size = 0; \
} \
void typ ## _VEC_append(typ ## _VEC *vec, typ elem) { \
    if (vec->size >= vec->cap) { \
        vec->cap *= 2; \
        vec->start = realloc(vec->start, vec->cap); \
    } \
    vec->start[vec->size++] = elem; \
} \
void typ ## _VEC_remove(typ ## _VEC *vec, size_t idx) { \
    memmove(vec->start + idx, vec->start + idx + 1, vec->size - idx - 1); \
}


// For representing the graph in the controller thread
struct EDGE;

VEC(EDGE);

typedef struct {
    char name;
    EDGEVEC edges;
} NODE_INFO;

typedef struct EDGE {
    NODE_INFO *a;
    NODE_INFO *b;
    double c;
} EDGE;

VEC(NODE_INFO);

// Router thread:
// Responsible for acting as a router
// Will be threadsafe queues for:
//  - Receiving messages from controller thread
//  - Sending messages to neighbors
//  - Receiving messages from neighbors
// 
void *router(void *args) {
}

// "Threadsafe" queue

// Controller thread:
// Responsible for reading input n stuff
int main(int argc, char **argv) {
    return 0;
}