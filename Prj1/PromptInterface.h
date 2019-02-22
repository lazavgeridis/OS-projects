/* PromptInterface.h */

#include "GraphTypes.h"

/* Command prompt functions */
void InsertNode(Graph **, vertex_data);
void InsertEdge(Graph **, vertex_data, vertex_data, int);
void DeleteNode(Graph **, vertex_data);
void DeleteEdge(Graph **, vertex_data, vertex_data, int);
void DeleteAllEdges(Graph **, vertex_data, vertex_data);
void ModifyWeight(Graph **, vertex_data, vertex_data, int, int);
void ReceivingEdges(Graph **, vertex_data);
void CircleFind(Graph **, vertex_data);
void FindCircles(Graph **, vertex_data, int);
void Traceflow(Graph **, vertex_data, vertex_data, int);
void Exit(Graph **);
void Exit_out(Graph **, char *);
