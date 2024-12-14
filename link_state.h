#include "sharedtypes.h"
#include <stdlib.h>

void link_state_update_router(NODE_INFO_VEC *nodes, size_t node_idx);
void link_state_update_all_routers(NODE_INFO_VEC *nodes);
void NODE_INFO_print_routing_table(const NODE_INFO *node,
                                   const NODE_INFO_VEC *nodes);
