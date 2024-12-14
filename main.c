#include "queue.h"
#include "vec.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sharedtypes.h"

// Router thread:
// Responsible for acting as a router
// Will be threadsafe queues for:
//  - Receiving messages from controller thread
//  - Sending messages to neighbors
//  - Receiving messages from neighbors
//
void *router(void *args) {}

void process_cmds(FILE *);

NODE_INFO_VEC *nodes;


// Controller thread:
// Responsible for reading input n stuff
int main(int argc, char **argv) {
  nodes = NODE_INFO_VEC_create();

  int arg = 1;
  // while (arg < argc) {
    
  // }

  process_cmds(stdin);

  printf("Nodes: %zu\n", nodes->size);
  for (int i = 0; i < nodes->size; i++) {
    printf("%c\n", nodes->data[i].name);
    for (int j = 0; j < nodes->data[i].edges->size; j++) {
      printf("  -> %c %.0f\n",
             nodes->data[nodes->data[i].edges->data[j].b_idx].name,
             nodes->data[i].edges->data[j].c);
    }
    printf("\n");
  }

  return 0;
}

void process_cmds(FILE *fp) {
  char input[1024];
  char input_cmds[4][256];

  while (fgets(input, sizeof(input), fp) != NULL) {
    int i = 0, num_cmds = 1, k = 0;
    while (input[i] != '\n') {
      if (input[i] == ' ') {
        input_cmds[num_cmds - 1][k] = '\0';
        num_cmds++;
        k = 0;
      } else {
        input_cmds[num_cmds - 1][k++] = input[i];
      }
      i++;
    }
    input_cmds[num_cmds - 1][k] = '\0';

    if (strcmp(input_cmds[0], "ls") == 0) {
      if (num_cmds == 1) {
        printf("Nodes: %zu\n", nodes->size);
        for (int i = 0; i < nodes->size; i++) {
          printf("%c\n", nodes->data[i].name);
          for (int j = 0; j < nodes->data[i].edges->size; j++) {
            printf("  -> %c %.0f\n",
                   nodes->data[nodes->data[i].edges->data[j].b_idx].name,
                   nodes->data[i].edges->data[j].c);
          }
          printf("\n");
        }
      }
    } else if (strcmp(input_cmds[0], "dv") == 0) {

    } else {
      if (num_cmds != 3) {
        fprintf(stderr,
                "Expected 3 arguments (<node A> <node B> <cost>); got %d\n",
                num_cmds - (i == 0));
        continue;
      }
      char node1 = input_cmds[0][0];
      char node2 = input_cmds[1][0];
      ssize_t node1_idx = -1, node2_idx = -1;
      for (int i = 0; i < nodes->size; i++) {
        if (nodes->data[i].name == node1)
          node1_idx = i;
        if (nodes->data[i].name == node2)
          node2_idx = i;
      }

      if (strcmp(input_cmds[2], "-") == 0) {
        if (node1_idx == -1 || node2_idx == -1)
          continue;

        EDGE_VEC *edges = nodes->data[node1_idx].edges;
        for (int i = 0; i < edges->size; i++) {
          size_t to_node = edges->data[i].b_idx;
          if (to_node == node2_idx) {
            EDGE_VEC_remove(edges, i);
            break;
          }
        }

        edges = nodes->data[node2_idx].edges;
        for (int i = 0; i < edges->size; i++) {
          size_t to_node = edges->data[i].b_idx;
          if (to_node == node1_idx) {
            EDGE_VEC_remove(edges, i);
            break;
          }
        }

        continue;
      }

      char *left;
      double cost = strtod(input_cmds[2], &left);
      if (*left != '\0' || errno != 0 || left == input_cmds[2]) {
        fprintf(stderr, "Invalid cost: %s\n", input_cmds[2]);
        continue;
      } 

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
      EDGE_VEC *node1_edges = nodes->data[node1_idx].edges;
      EDGE_VEC *node2_edges = nodes->data[node2_idx].edges;

      for (int i = 0; i < node1_edges->size; i++) {
        EDGE *edge = &node1_edges->data[i];
        if (edge->b_idx == node2_idx) {
          node1_edge_found = 1;
          edge->c = cost;
          break;
        }
      }

      for (int i = 0; i < node2_edges->size; i++) {
        EDGE *edge = &node2_edges->data[i];
        if (edge->b_idx == node1_idx) {
          node2_edge_found = 1;
          edge->c = cost;
          break;
        }
      }

      if (!node1_edge_found) {
        EDGE new_edge = {
            .a_idx = node1_idx,
            .b_idx = node2_idx,
            .c = cost,
        };
        EDGE_VEC_append(node1_edges, new_edge);
      }
      if (!node2_edge_found) {
        EDGE new_edge = {
            .a_idx = node2_idx,
            .b_idx = node1_idx,
            .c = cost,
        };
        EDGE_VEC_append(node2_edges, new_edge);
      }
    }
  }
}
