#include "sharedtypes.h"
#include "routers.h"

VEC_IMPL(ROUTER_MESSAGE);

QUEUE_IMPL(ROUTER_MESSAGE);

VEC_IMPL(ROUTER_EDGE_RECORD);

ROUTER_INFO *ROUTER_INFO_create(char name) {
    ROUTER_INFO *dat = malloc(sizeof(ROUTER_INFO));
    dat->edges = ROUTER_EDGE_RECORD_VEC_create();
    dat->name = name;
    dat->incomingMessageQueue = ROUTER_MESSAGE_QUEUE_create();
}

void *router(void *arg) {
    ROUTER_INFO *myRouter = arg;

    while (1) {

    }
}

int bob(int a, int b) {
    return a + b;
}
