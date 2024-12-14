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

// Code vaguely from
// https://www.geeksforgeeks.org/c-program-to-implement-min-heap/#
void MIN_HEAP_heapify(MIN_HEAP *heap, size_t idx) {
  // printf("(%ld) %0.f, %ld, %ld\n", idx, heap->arr[0].dist, heap->arr[0].idx, heap->arr[0].thru);
  ssize_t left = idx * 2 + 1;
  ssize_t right = idx * 2 + 2;
  ssize_t min = idx;

  if (left >= heap->size || left < 0)
    left = -1;
  if (right >= heap->size || right < 0)
    right = -1;

  if (left != -1 && heap->arr[left].dist < heap->arr[idx].dist)
    min = left;
  if (right != -1 && heap->arr[right].dist < heap->arr[idx].dist)
    min = right;

  if (min != idx) {
    DIST_INFO temp = heap->arr[min];
    heap->arr[min] = heap->arr[idx];
    heap->arr[idx] = temp;

    MIN_HEAP_heapify(heap, min);
  }
}

int MIN_HEAP_extract_min(MIN_HEAP *heap, DIST_INFO *out) {
  if (heap->size == 0)
    return -1;

  // printf("%0.f, %ld, %ld\n", heap->arr[0].dist, heap->arr[0].idx, heap->arr[0].thru);

  *out = heap->arr[0];

  heap->arr[0] = heap->arr[--heap->size];

  MIN_HEAP_heapify(heap, 0);

  return 0;
}

// Code vaguely from https://www.baeldung.com/cs/min-heaps-decrease-key
void MIN_HEAP_decrease_key(MIN_HEAP *heap, size_t idx, double new_dist) {
  heap->arr[idx].dist = new_dist;
  while (idx > 0) {
    if (heap->arr[idx].dist < heap->arr[(idx - 1) / 2].dist) {
      DIST_INFO temp = heap->arr[idx];
      heap->arr[idx] = heap->arr[(idx - 1) / 2];
      heap->arr[(idx - 1) / 2] = temp;
      idx = (idx - 1) / 2;
    } else {
      break;
    }
  }
}

// Runs Dijkstra's from this node
void link_state_update_router(NODE_INFO_VEC *nodes, size_t node_idx) {
  DIST_INFO *dists = malloc(nodes->size * sizeof(*dists));
  for (int i = 0; i < nodes->size; i++) {
    dists[i].dist = INFINITY;
    dists[i].idx = i;
    dists[i].thru = -1;
  }
  dists[node_idx].dist = 0;
  dists[node_idx].idx = node_idx;
  dists[node_idx].thru = -1;
  MIN_HEAP heap = {.arr = dists, .size = nodes->size};
  MIN_HEAP_heapify(&heap, 0);

  NODE_INFO *node = &nodes->data[node_idx];

  node->routing_table->size = 0;

  DIST_INFO min;
  int res;
  while ((res = MIN_HEAP_extract_min(&heap, &min)) != -1 && min.dist != INFINITY) {
    EDGE_VEC_append(node->routing_table, (EDGE){ .a_idx = min.idx, .b_idx = min.thru, .c = min.dist});
    EDGE_VEC *edges = nodes->data[min.idx].edges;
    for (int i = 0; i < edges->size; i++) {
      ssize_t node_heap_idx = -1;
      for (int j = 0; j < heap.size; j++) {
        if (heap.arr[j].idx == edges->data[i].b_idx) {
          node_heap_idx = j;
          break;
        }
      }
      if (node_heap_idx == -1) {
        continue;
      }

      if (heap.arr[node_heap_idx].dist > min.dist + edges->data[i].c) {
        heap.arr[node_heap_idx].thru = min.idx;
        MIN_HEAP_decrease_key(&heap, node_heap_idx, min.dist + edges->data[i].c);
      }
    }
  }
}

// Runs Floyd-Warshall on the graph
// Used pseudocode from
// https://en.wikipedia.org/wiki/Floyd%E2%80%93Warshall_algorithm
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
    qsort(nodes->data[i].routing_table->data,
          nodes->data[i].routing_table->size, sizeof(EDGE), compare_edges);
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
