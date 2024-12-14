#pragma once
#include <stdlib.h>
#include "vec.h"
#include "queue.h"

struct NODE_INFO;

// For representing the graph in the controller thread
typedef struct EDGE {
  size_t a_idx;
  size_t b_idx;
  double c;
} EDGE;

VEC(EDGE);

typedef struct NODE_INFO {
  char name;
  EDGE_VEC *edges;
  EDGE_VEC *routing_table;
} NODE_INFO;

VEC(NODE_INFO)
