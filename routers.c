#include "sharedtypes.h"
#include "routers.h"

#include <stdio.h>
#include <math.h>
#include <stdbool.h>

VEC_IMPL(ROUTER_MESSAGE);

QUEUE_IMPL(ROUTER_MESSAGE);

VEC_IMPL(ROUTER_EDGE_RECORD);

ROUTER_INFO *ROUTER_INFO_create(char name) {
    ROUTER_INFO *dat = malloc(sizeof(ROUTER_INFO));
    dat->edges = ROUTER_EDGE_RECORD_VEC_create();
    dat->name = name;
    dat->incomingMessageQueue = ROUTER_MESSAGE_QUEUE_create();
    return dat;
}

ROUTER_EDGE_RECORD *getEdgeByPtr(ROUTER_INFO *self, ROUTER_INFO *other) {
    for (int i = 0; i < self->edges->size; i++) {
        if (self->edges->data[i].end == other) {
            return &self->edges->data[i];
        }
    }
    return NULL;
}

void addOrUpdateEdge(ROUTER_INFO* self, ROUTER_INFO* other, double weight) {
    ROUTER_EDGE_RECORD myNewEdge;
    myNewEdge.cost = weight;
    myNewEdge.end = other;
    for (int j = 0; j < MAX_NODES; j++) {
        myNewEdge.distanceVector[j] = INFINITY;
    }
    
    for (int i = 0; i < self->edges->size; i++) {
        if (self->edges->data[i].end == other) {
            self->edges->data[i] = myNewEdge;
            return;
        }
    }

    ROUTER_EDGE_RECORD_VEC_append(self->edges, myNewEdge);
}

void sendDistanceVectorToNeighbors(ROUTER_INFO *selfRouter, double *myDistVec) {
    for (int i = 0; i < selfRouter->edges->size; i++) {
        ROUTER_EDGE_RECORD edge = selfRouter->edges->data[i];
        if (!isfinite(edge.cost) || edge.end == NULL) {
            continue;
        }

        double *newCopy = malloc(sizeof(double) * MAX_NODES);
        for (int j = 0; j < MAX_NODES; j++) {
            newCopy[j] = myDistVec[j];
        }

        ROUTER_MESSAGE myMsg;
        myMsg.msgType = ROUTER_MESSAGE_WEIGHTS_UPDATE;
        myMsg.contents.distanceVectorMessage.distVec = newCopy;
        myMsg.contents.distanceVectorMessage.source = selfRouter;

        ROUTER_MESSAGE_QUEUE_push(edge.end->incomingMessageQueue, myMsg);
    }
}

bool updateMyDistanceVector(ROUTER_INFO *self, double *myVec, int *edgeIndOuts) {
    bool toRet = false;
    
    for (int i = 0; i < MAX_NODES; i++) {
        double minSoFar = INFINITY;
        int outgoingEdgeInd = -1;
        if (i == self->name) {
            minSoFar = 0;
            outgoingEdgeInd = -2;
        }
        for (int j = 0; j < self->edges->size; j++) {
            if (self->edges->data[j].end == NULL || isinf(self->edges->data[j].cost)) {
                continue;
            }

            double toCompTo = self->edges->data[j].distanceVector[i] + self->edges->data[j].cost;
            if (toCompTo < minSoFar) {
                minSoFar = toCompTo;
                outgoingEdgeInd = j;
            }
        }
        if (minSoFar != myVec[i] || outgoingEdgeInd != edgeIndOuts[i]) {
            toRet = true;
        }
        myVec[i] = minSoFar;
        edgeIndOuts[i] = outgoingEdgeInd;
    }
    return toRet;
}

void printYourDistanceVector(ROUTER_INFO *self, double *myVec, int *edgeIndOuts) {
    pthread_mutex_lock(&printMutex);
    printf("Node %c speaking!\n", self->name);
    for (int i = 0; i < MAX_NODES; i++) {
        if (!isfinite(myVec[i])) continue;

        //printf("%i %f %i\n", i, myVec[i], edgeIndOuts[i]);
        printf("To %c: %f via %c\n", i, myVec[i], edgeIndOuts[i] < 0 ? self->name : self->edges->data[edgeIndOuts[i]].end->name);
    }
    pthread_mutex_unlock(&printMutex);
}

void packetFwd(ROUTER_INFO *self, char message, char dest, double *distVec, int *edgeOutInds) {
    pthread_mutex_lock(&printMutex);

    printf("Packet received at %c!\n", self->name);
    if (self->name == dest) {
        printf("Packet arrived at desination. Message: %c Yay!\n", message);
    }
    else {
        if (edgeOutInds[dest] >= 0) {
            printf("Forwarding packet through %c\n", edgeOutInds[dest]);
            ROUTER_MESSAGE toSend;
            toSend.msgType = ROUTER_MESSAGE_PACKET;
            toSend.contents.packetMessage.destination = dest;
            toSend.contents.packetMessage.message = message;
            ROUTER_MESSAGE_QUEUE_push(self->edges->data[edgeOutInds[dest]].end->incomingMessageQueue, toSend);
        }
        else {
            printf("Impossible to forward. Packet dies here.\n");
        }
    }
    pthread_mutex_unlock(&printMutex);
}

void *router(void *arg) {
    ROUTER_INFO *myRouter = arg;

    double myDistanceVector[MAX_NODES];
    int outgoingEdgeInds[MAX_NODES];
    for (int i = 0; i < MAX_NODES; i++) {
        myDistanceVector[i] = INFINITY;
        outgoingEdgeInds[i] = -1;
    }

    while (1) {
        ROUTER_MESSAGE incomingMsg;
        ROUTER_MESSAGE_QUEUE_pop(myRouter->incomingMessageQueue, &incomingMsg);


        enum ROUTER_MESSAGE_TYPE msgType = incomingMsg.msgType;
        ROUTER_MESSAGE_CONTENTS content = incomingMsg.contents;

        // printf("%c %d\n", myRouter->name, msgType);

        switch (msgType) {
            case ROUTER_MESSAGE_EDGE_ADD: {
                // printf("other: %c\n", content.edgeAdditionMessage.other->name);
                addOrUpdateEdge(myRouter, content.edgeAdditionMessage.other, content.edgeAdditionMessage.weight);
                if (content.edgeAdditionMessage.repRequired == ROUTER_EDGE_ADD_REPLY_IN_KIND) {
                    ROUTER_MESSAGE toSend;
                    toSend.msgType = ROUTER_MESSAGE_EDGE_ADD;
                    toSend.contents.edgeAdditionMessage.other = myRouter;
                    toSend.contents.edgeAdditionMessage.weight = content.edgeAdditionMessage.weight;
                    toSend.contents.edgeAdditionMessage.repRequired = ROUTER_EDGE_ADD_REPLY_SEND_DIST_VEC;
                    ROUTER_MESSAGE_QUEUE_push(content.edgeAdditionMessage.other->incomingMessageQueue, toSend);

                    if (!isfinite(content.edgeAdditionMessage.weight)) {
                        updateMyDistanceVector(myRouter, myDistanceVector, outgoingEdgeInds);
                        sendDistanceVectorToNeighbors(myRouter, myDistanceVector);
                    }
                }
                else if (content.edgeAdditionMessage.repRequired == ROUTER_EDGE_ADD_REPLY_SEND_DIST_VEC) {
                    /*ROUTER_MESSAGE toSend;
                    toSend.msgType = ROUTER_MESSAGE_EDGE_ADD;
                    toSend.contents.edgeAdditionMessage.other = myRouter;
                    toSend.contents.edgeAdditionMessage.weight = content.edgeAdditionMessage.weight;
                    toSend.contents.edgeAdditionMessage.repRequired = ROUTER_EDGE_ADD_DO_NOT_REPLY;
                    ROUTER_MESSAGE_QUEUE_push(content.edgeAdditionMessage.other->incomingMessageQueue, toSend);*/
                    updateMyDistanceVector(myRouter, myDistanceVector, outgoingEdgeInds);
                    sendDistanceVectorToNeighbors(myRouter, myDistanceVector);
                }

                //else if (content.edgeAdditionMessage.repRequired == ROUTER_EDGE_ADD_DO_NOT_REPLY) {
                //    sendDistanceVectorToNeighbors(myRouter, myDistanceVector);
                //}
                break;
            }
            case ROUTER_MESSAGE_DEBUG: {
                printYourDistanceVector(myRouter, myDistanceVector, outgoingEdgeInds);
                break;
            }
            case ROUTER_MESSAGE_PACKET: {
                packetFwd(myRouter, content.packetMessage.message, content.packetMessage.destination, myDistanceVector, outgoingEdgeInds);
                break;
            }
            case ROUTER_MESSAGE_WEIGHTS_UPDATE: {
                // printf("%c from: %c\n", myRouter->name, content.distanceVectorMessage.source->name);
                ROUTER_EDGE_RECORD *myEdge = getEdgeByPtr(myRouter, content.distanceVectorMessage.source);
                for (int i = 0; i < MAX_NODES; i++) {
                    myEdge->distanceVector[i] = content.distanceVectorMessage.distVec[i];
                }
                free(content.distanceVectorMessage.distVec);

                bool anyChanges = updateMyDistanceVector(myRouter, myDistanceVector, outgoingEdgeInds);
                if (anyChanges) {
                    sendDistanceVectorToNeighbors(myRouter, myDistanceVector);
                }
            }
        }

        //printf("Haha hello from %c\n", myRouter->name);
    }
}

ROUTER_INFO *getOrCreateRouter(ROUTER_MANAGER *manager, char name) {
    if (manager->allRouters[name]) {
        return manager->allRouters[name];
    }
    manager->allRouters[name] = ROUTER_INFO_create(name);
    pthread_t myThread;
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_create(&myThread, &attrs, router, manager->allRouters[name]);
    return manager->allRouters[name];
}

void ROUTER_MANAGER_add_edge(ROUTER_MANAGER *manager, char a, char b, double weight) {
    ROUTER_INFO *first = getOrCreateRouter(manager, a);
    ROUTER_INFO *second = getOrCreateRouter(manager, b);
    ROUTER_MESSAGE toSend;
    toSend.msgType = ROUTER_MESSAGE_EDGE_ADD;
    toSend.contents.edgeAdditionMessage.other = second;
    toSend.contents.edgeAdditionMessage.weight = weight;
    toSend.contents.edgeAdditionMessage.repRequired = ROUTER_EDGE_ADD_REPLY_IN_KIND;
    ROUTER_MESSAGE_QUEUE_push(first->incomingMessageQueue, toSend);
}

void ROUTER_MANAGER_remove_edge(ROUTER_MANAGER *manager, char a, char b) {
    ROUTER_INFO *first = getOrCreateRouter(manager, a);
    ROUTER_INFO *second = getOrCreateRouter(manager, b);
    ROUTER_MESSAGE toSend;
    toSend.msgType = ROUTER_MESSAGE_EDGE_ADD;
    toSend.contents.edgeAdditionMessage.other = second;
    toSend.contents.edgeAdditionMessage.weight = INFINITY;
    toSend.contents.edgeAdditionMessage.repRequired = ROUTER_EDGE_ADD_REPLY_IN_KIND;
    ROUTER_MESSAGE_QUEUE_push(first->incomingMessageQueue, toSend);
}

void ROUTER_MANAGER_print_distance_vec(ROUTER_MANAGER *manager, char name) {
    ROUTER_INFO *route = manager->allRouters[name];
    if (!route) return;

    ROUTER_MESSAGE toSend;
    toSend.msgType = ROUTER_MESSAGE_DEBUG;
    ROUTER_MESSAGE_QUEUE_push(route->incomingMessageQueue, toSend);
}

ROUTER_MANAGER *ROUTER_MANAGER_create() {
    ROUTER_MANAGER *ptr = malloc(sizeof(ROUTER_MANAGER));
    for (int i = 0; i < MAX_NODES; i++) {
        ptr->allRouters[i] = NULL;
    }
    return ptr;
}

void ROUTER_MANAGER_route_packet(ROUTER_MANAGER *manager, char a, char b, char message) {
    ROUTER_INFO *rtr = getOrCreateRouter(manager, a);
    ROUTER_MESSAGE toSend;
    toSend.msgType = ROUTER_MESSAGE_PACKET;
    toSend.contents.packetMessage.destination = b;
    toSend.contents.packetMessage.message = message;
    ROUTER_MESSAGE_QUEUE_push(rtr->incomingMessageQueue, toSend);
}
