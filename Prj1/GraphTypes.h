#ifndef GRAPH_TYPES_H
#define GRAPH_TYPES_H

#define TABLESIZE 26	/* 26 letters in the alphabet */

typedef enum {FALSE, TRUE} Boolean;
typedef enum {NOT_VISITED, VISITED} State;
typedef char* vertex_data;

struct adjacency_list_node {

	vertex_data name;			/* bank account name */
	int weight;				/* edge weight */
	struct adjacency_list_node *next;	/* pointer to the next adjacent vertex */	
};
typedef struct adjacency_list_node adj_node;

struct bucket_list_node {

	vertex_data name;
	State st;				/* this field is used primarly in the circlefind command */
	struct bucket_list_node *b_next;	/* pointer to the next bucket node in the bucket's linked list */
	adj_node *head;				/* list of the outgoing edges of this node */
	int active_stack_count;
};
typedef struct bucket_list_node bucket_node;


struct graph {

	unsigned int V;					/* number of vertices in the graph */
	bucket_node *b_heads[TABLESIZE];		/* a hash table(array of pointers), each element is a "bucket" that stores all the vertices starting with the specific letter */
};
typedef struct graph Graph;


adj_node *stack_top;
adj_node *cycle;
adj_node *blocked_set;
char source[25];
char dest[25];
Boolean  cycle_flag;

#endif
