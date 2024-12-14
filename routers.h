#pragma once
#include <stdlib.h>
#include <pthread.h>
#include "vec.h"
#include "queue.h"


enum ROUTER_MESSAGE_TYPE {
  PACKET,
  EDGE_ADD,
  //EDGE_REMOVE,
  //EDGE_UPDATE,
  WEIGHTS_UPDATE
};

typedef struct ROUTER_MESSAGE {
  enum ROUTER_MESSAGE_TYPE msgType;
} ROUTER_MESSAGE;

struct ROUTER_INFO;

typedef struct ROUTER_EDGE_RECORD {
  struct ROUTER_INFO *a;
  struct ROUTER_INFO *b;
  double cost;
} ROUTER_EDGE_RECORD;

VEC(ROUTER_MESSAGE);

QUEUE(ROUTER_MESSAGE);

VEC(ROUTER_EDGE_RECORD);

// For describing the thread
typedef struct ROUTER_INFO {
  ROUTER_MESSAGE_QUEUE *incomingMessageQueue;
  char name;
  ROUTER_EDGE_RECORD_VEC *edges;
} ROUTER_INFO;

ROUTER_INFO *ROUTER_INFO_create(char n);
