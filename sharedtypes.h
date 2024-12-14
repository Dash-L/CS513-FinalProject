#pragma once
#include <stdlib.h>
#include "vec.h"
#include "queue.h"

struct NODE_INFO;

// For representing the graph in the controller thread
typedef struct EDGE {
  // I'm not sure if we really need both of these, since edges are stored in
  // `NODE_INFO`s, so we already know one end of the edge I'm keeping both for
  // now, but only using `b` as the "to" node.
  size_t a_idx;
  size_t b_idx;
  double c;
} EDGE;

VEC(EDGE);

typedef struct NODE_INFO {
  char name;
  EDGE_VEC *edges;
} NODE_INFO;

VEC(NODE_INFO);

struct ROUTER_INFO;

typedef struct ROUTER_MESSAGE {
  
} ROUTER_MESSAGE;

typedef struct ROUTER_EDGE {
    EDGE edge;

} ROUTER_EDGE;

// For describing the thread
typedef struct ROUTER_INFO {
    // TEDGE_VEC
} ROUTER_INFO;
