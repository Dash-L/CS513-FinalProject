#include "math.h"
#include "sharedtypes.h"
#include "vec.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

VEC_IMPL(EDGE);

VEC_IMPL(NODE_INFO);

typedef struct DIST_INFO {
  double dist;
  ssize_t thru;
  size_t idx;
} DIST_INFO;

int compare_edges(const void *a, const void *b) {
  return (((EDGE *)a)->c > ((EDGE *)b)->c) - (((EDGE *)a)->c < ((EDGE *)b)->c);
}

typedef struct {
  DIST_INFO *arr;
  size_t size;
} MIN_HEAP;

void MIN_HEAP_heapify(MIN_HEAP *heap) {
  
}

// Key is the index in the nodes array
void MIN_HEAP_delete(MIN_HEAP *heap, size_t key) {
  
}

// Runs Dijkstra's from this node
void link_state_update_router(NODE_INFO_VEC *nodes, size_t node_idx) {
  DIST_INFO *dists = malloc(nodes->size * sizeof(*dists));
  for (int i = 0; i < nodes->size ; i++) {
    dists[i].dist = INFINITY;
  }
  dists[node_idx].dist = 0;
}

// Runs Floyd-Warshall on the graph
// Used pseudocode from https://en.wikipedia.org/wiki/Floyd%E2%80%93Warshall_algorithm
void link_state_update_all_routers(NODE_INFO_VEC *nodes) {
  DIST_INFO **dists = malloc(nodes->size * sizeof(*dists));
  for (int i = 0; i < nodes->size; i++) {
    dists[i] = malloc(nodes->size * sizeof(*dists[i]));
    for (int j = 0; j < nodes->size; j++) {
      dists[i][j] = (DIST_INFO){.dist = INFINITY, .thru = -1};
    }
  }

  for (int i = 0; i < nodes->size; i++) {
    dists[i][i].dist = 0;
    EDGE_VEC *edges = nodes->data[i].edges;
    for (int j = 0; j < edges->size; j++) {
      dists[i][edges->data[j].b_idx].dist = edges->data[j].c;
      dists[i][edges->data[j].b_idx].thru = i;
    }
  }

  for (int k = 0; k < nodes->size; k++) {
    for (int i = 0; i < nodes->size; i++) {
      for (int j = 0; j < nodes->size; j++) {
        if (dists[i][j].dist > dists[i][k].dist + dists[k][j].dist) {
          dists[i][j].dist = dists[i][k].dist + dists[k][j].dist;
          dists[i][j].thru = k;
        }
      }
    }
  }

  for (int i = 0; i < nodes->size; i++) {
    nodes->data[i].routing_table->size = 0;
    for (int j = 0; j < nodes->size; j++) {
      if (dists[i][j].dist == INFINITY)
        continue;

      EDGE_VEC_append(
          nodes->data[i].routing_table,
          (EDGE){.b_idx = dists[i][j].thru, .a_idx = j, .c = dists[i][j].dist});
    }
    qsort(nodes->data[i].routing_table->data, nodes->data[i].routing_table->size,
          sizeof(EDGE), compare_edges);
  }
}

void NODE_INFO_print_routing_table(const NODE_INFO *node,
                                   const NODE_INFO_VEC *nodes) {
  for (int i = 0; i < node->routing_table->size; i++) {
    EDGE table_entry = node->routing_table->data[i];
    printf("%c %c %0.f\n", nodes->data[table_entry.a_idx].name,
           table_entry.b_idx == -1 ? '-' : nodes->data[table_entry.b_idx].name,
           table_entry.c);
  }
}
