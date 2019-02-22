/* Utilities.h */

#include "GraphTypes.h"

/* Utility Functions */
int Hash(vertex_data);
void DFS_t(Graph **, vertex_data, int, int);
void DFS_f(Graph **, vertex_data, int, int);
void DFS_c(Graph **, vertex_data, int);
void pushStack(vertex_data, int);
void InitializeGraph(Graph **);
void delete_list(adj_node **);
void remove_newline(char *);
void popStack(adj_node **);
void initBlockedSet();
void initStack();
void initCycle();
bucket_node *NewBucketNode(vertex_data);
adj_node *NewAdjNode(vertex_data, int);
Boolean SearchBucketNode(Graph **, vertex_data, int);
Boolean emptyStack();
