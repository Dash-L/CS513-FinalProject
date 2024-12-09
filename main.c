#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For describing the thread
typedef struct {

} ROUTER_INFO;

#define VEC(typ)                                                               \
  typedef struct {                                                             \
    typ *start;                                                                \
    size_t size;                                                               \
    size_t cap;                                                                \
  } typ##_VEC;                                                                 \
  typ##_VEC *typ##_VEC_create() {                                              \
    typ##_VEC *vec = malloc(sizeof(*vec));                                     \
    vec->cap = 8;                                                              \
    vec->start = malloc(sizeof(typ) * vec->cap);                               \
    vec->size = 0;                                                             \
    return vec;                                                                \
  }                                                                            \
  void typ##_VEC_append(typ##_VEC *vec, typ elem) {                            \
    if (vec->size >= vec->cap) {                                               \
      vec->cap *= 2;                                                           \
      vec->start = realloc(vec->start, vec->cap);                              \
    }                                                                          \
    vec->start[vec->size++] = elem;                                            \
  }                                                                            \
  void typ##_VEC_remove(typ##_VEC *vec, size_t idx) {                          \
    memmove(vec->start + idx, vec->start + idx + 1, vec->size - idx - 1);      \
    vec->size -= 1;                                                            \
  }                                                                            \
  typ typ##_VEC_pop(typ##_VEC *vec) { return vec->start[--vec->size]; }

struct NODE_INFO;

// For representing the graph in the controller thread
typedef struct EDGE {
  // I'm not sure if we really need both of these, since edges are stored in
  // `NODE_INFO`s, so we already know one end of the edge I'm keeping both for
  // now, but only using `b` as the "to" node.
  struct NODE_INFO *a;
  struct NODE_INFO *b;
  double c;
} EDGE;

VEC(EDGE);

typedef struct NODE_INFO {
  char name;
  EDGE_VEC *edges;
} NODE_INFO;

VEC(NODE_INFO);

// Router thread:
// Responsible for acting as a router
// Will be threadsafe queues for:
//  - Receiving messages from controller thread
//  - Sending messages to neighbors
//  - Receiving messages from neighbors
//
void *router(void *args) {}

// Threadsafe queue
#define QUEUE(typ)                                                             \
  typedef struct {                                                             \
    typ##_VEC *vec;                                                            \
    pthread_mutex_t mutex;                                                     \
    pthread_cond_t nonempty;                                                   \
  } typ##_QUEUE;                                                               \
                                                                               \
  typ##_QUEUE *typ##_QUEUE_create() {                                          \
    typ##_QUEUE *queue = malloc(sizeof(*queue));                               \
    typ##_VEC *vec = typ##_VEC_create();                                       \
    queue->vec = vec;                                                          \
    pthread_mutex_init(&queue->mutex, NULL);                                   \
    pthread_cond_init(&queue->nonempty, NULL);                                 \
    return queue;                                                              \
  }                                                                            \
  void typ##_QUEUE_push(typ##_QUEUE *queue, typ elem) {                        \
    pthread_mutex_lock(&queue->mutex);                                         \
    typ##_VEC_append(queue->vec, elem);                                        \
    pthread_cond_signal(&queue->nonempty);                                     \
    pthread_mutex_unlock(&queue->mutex);                                       \
  }                                                                            \
  /* Guaranteed to pop an element, might wait a while for one to be there */   \
  void typ##_QUEUE_pop(typ##_QUEUE *queue, typ *out) {                         \
    pthread_mutex_lock(&queue->mutex);                                         \
    while (queue->vec->size == 0) {                                            \
      pthread_cond_wait(&queue->nonempty);                                     \
    }                                                                          \
    *out = typ##_VEC_pop(queue->vec);                                          \
    pthread_mutex_unlock(&queue->mutex);                                       \
  }                                                                            \
  /* Returns -1 if there is nothing to pop. Still waits on mutex until it      \
   * unlocks */                                                                \
  int typ##_QUEUE_trypop(typ##_QUEUE *queue, typ *out) {                       \
    if (queue->vec->size == 0)                                                 \
      return -1;                                                               \
    pthread_mutex_lock(&queue->mutex);                                         \
    if (queue->vec->size == 0)                                                 \
      return -1;                                                               \
    *out = typ##_VEC_pop(queue->vec);                                          \
    pthread_mutex_unlock(&queue->mutex);                                       \
    return 0;                                                                  \
  }

// Controller thread:
// Responsible for reading input n stuff
int main(int argc, char **argv) {
  NODE_INFO_VEC *nodes = NODE_INFO_VEC_create();
  char node1, node2;
  char cost_input[64];
  double cost;

  int res;
  while ((res = scanf(" %c %c %s", &node1, &node2, cost_input)) == 3) {
    ssize_t node1_idx = -1, node2_idx = -1;
    for (int i = 0; i < nodes->size; i++) {
      if (nodes->start[i].name == node1)
        node1_idx = i;
      if (nodes->start[i].name == node2)
        node2_idx = i;
    }

    if (strcmp(cost_input, "-") == 0) {
      if (node1_idx == -1 || node2_idx == -1)
        continue;

      EDGE_VEC *edges = nodes->start[node1_idx].edges;
      for (int i = 0; i < edges->size; i++) {
        NODE_INFO *to_node = edges->start[i].b;
        if (to_node->name == node2) {
          EDGE_VEC_remove(edges, i);
          break;
        }
      }

      edges = nodes->start[node2_idx].edges;
      for (int i = 0; i < edges->size; i++) {
        NODE_INFO *to_node = edges->start[i].b;
        if (to_node->name == node1) {
          EDGE_VEC_remove(edges, i);
          break;
        }
      }

      continue;
    }

    cost = atof(cost_input);

    if (node1_idx == -1) {
      NODE_INFO new_node = {
          .name = node1,
          .edges = EDGE_VEC_create(),
      };
      NODE_INFO_VEC_append(nodes, new_node);
      node1_idx = nodes->size - 1;
    }
    if (node2_idx == -1) {
      NODE_INFO new_node = {
          .name = node2,
          .edges = EDGE_VEC_create(),
      };
      NODE_INFO_VEC_append(nodes, new_node);
      node2_idx = nodes->size - 1;
    }

    int node1_edge_found = 0, node2_edge_found = 0;
    EDGE_VEC *node1_edges = nodes->start[node1_idx].edges;
    EDGE_VEC *node2_edges = nodes->start[node2_idx].edges;

    for (int i = 0; i < node1_edges->size; i++) {
      EDGE *edge = &node1_edges->start[i];
      if (edge->b->name == node2) {
        node1_edge_found = 1;
        edge->c = cost;
        break;
      }
    }

    for (int i = 0; i < node2_edges->size; i++) {
      EDGE *edge = &node2_edges->start[i];
      if (edge->b->name == node1) {
        node2_edge_found = 1;
        edge->c = cost;
        break;
      }
    }

    if (!node1_edge_found) {
      EDGE new_edge = {
          .a = &nodes->start[node1_idx],
          .b = &nodes->start[node2_idx],
          .c = cost,
      };
      EDGE_VEC_append(node1_edges, new_edge);
    }
    if (!node2_edge_found) {
      EDGE new_edge = {
          .a = &nodes->start[node2_idx],
          .b = &nodes->start[node1_idx],
          .c = cost,
      };
      EDGE_VEC_append(node2_edges, new_edge);
    }
  }

  printf("Nodes: %zu\n", nodes->size);
  for (int i = 0; i < nodes->size; i++) {
    printf("%c\n", nodes->start[i].name);
    for (int j = 0; j < nodes->start[i].edges->size; j++) {
      printf("  -> %c %.0f\n", nodes->start[i].edges->start[j].b->name,
             nodes->start[i].edges->start[j].c);
    }
    printf("\n");
  }

  return 0;
}
