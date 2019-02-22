/* Utilities.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GraphTypes.h"


/********************************************************** Utility Functions ************************************************************/


int Hash(vertex_data str_to_hash)			/* hash a string to an integer value using the string's first letter */
{
	int h = str_to_hash[0] % 65; /* % 97  */	/* take the integer value of the first character of the string and do modulus 65('A' = 65)  */
							/* or modulus 97 if our dataset consists of lowercase words/names */
	return h;					/* h is in the range [0, 25] */
}

void InitializeGraph(Graph **g)				/* function to initialize the graph */
{
	int i;

	(*g)->V = 0;
	for(i = 0; i < TABLESIZE; ++i) 			/* make every bucket linked list point to null */
		(*g)->b_heads[i] = NULL;
}	

bucket_node *NewBucketNode(vertex_data n)  		/* create a new bucket node with vertex_data n */
{
	bucket_node *node = (bucket_node *)malloc( sizeof(bucket_node) );	/* allocate memory for a new bucket node */
	if(node == NULL) {							
		printf("\nNot enough memory!\n");
		exit(-1);
	}
	int length = strlen(n) + 1;			/* allocate memory for the name */
	node->name = (char *)malloc( length * sizeof(char) );
	strcpy(node->name, n);				/* copy the string argument to the field "name" of the node */
	node->st = NOT_VISITED;				/* the st field will be used in the last 3 prompt commands */
	node->b_next = NULL;				/* no next bucket node atm, set next to null */
	node->head = NULL;				/* no adjacent node atm, set next to null */
	node->active_stack_count = 0;			/* used in DFS_f, DFS_t */

	return node;
}

adj_node *NewAdjNode(vertex_data n, int wt)		/* create a new adj.list node with vertex_data n and weight wt */
{
	adj_node *node = (adj_node *)malloc( sizeof(adj_node) );	/* allocate memory for a new node */
	if(node == NULL) {							
		printf("\nNot enough memory!\n");
		exit(-1);
	}
	int length = strlen(n) + 1;			/* allocate memory for the name */
	node->name = (char *)malloc( length * sizeof(char) );
	strcpy(node->name, n);				/* copy the string argument to the field "name" of the node */
	node->weight = wt;				/* set weight to wt, since this node is about to be added to the adj.list of a BUCKET node(resembles an outgoing edge) */
	node->next = NULL;				/* no next adjacent node atm, set next to null */

	return node;
}

Boolean SearchBucketNode(Graph **g, vertex_data ni, int pos)	/* search if a bucket node with the name "ni" exists */
{								
	bucket_node *ptraverse = (*g)->b_heads[pos];
	
	if(ptraverse != NULL) {			/* if bucket's linked list is empty bucket node "ni" certainly does not exist */
		while(ptraverse) {
			if(strcmp(ptraverse->name, ni) == 0) return TRUE;	/* we found the node we were looking for */
			ptraverse = ptraverse->b_next;
		}
	}
	
	return FALSE;
}

void remove_newline(char *str) {			/* function to remove the '\n' character from a string - used after getline() in main */

	int i;

	for(i = 0; i < (int)strlen(str); ++i) {
		if(str[i] == '\n') {
			str[i] = '\0';

			break;
		}
	}
}
/*********************************************************/

Boolean emptyStack()
{
	if(stack_top == NULL)	return TRUE;

	return FALSE;
}

void initStack()
{
	stack_top = NULL;
}

void initCycle()
{
	cycle = NULL;
}

void initBlockedSet()
{
	blocked_set = NULL;
}

/*********************************************************/


void pushStack(vertex_data d, int weight)		/* a routine for pushing an adj.node into the stack */
{
	adj_node *stack_node = NewAdjNode(d, weight);	/* create a new (stack) adj.node */

	if(emptyStack() == TRUE) 	/* stack is empty */
		stack_top = stack_node;
	else {				/* stack is not empty */	
		stack_node->next = stack_top;
		stack_top = stack_node;
	}
}

void popStack(adj_node **popped)			/* a routine for popping an adj.node from the stack */
{
	adj_node *temp;

	if(emptyStack() == TRUE) 
		printf("\nCan't pop an empty stack\n");
	else {
		temp = stack_top;
		stack_top = stack_top->next;
		(*popped) = temp;
		(*popped)->next = NULL;
	}
}


void delete_list(adj_node **start)			/* delete the outgoing edges of a vertex(bucket node) */
{
	adj_node *n, *temp;

	for(n = *start; n != NULL; n = temp) {
		temp = n->next;
		free(n->name);	/* first, free the name field of the node */
		free(n);	/* then, free the node */
	}
}


void DFS_c(Graph **g, vertex_data ni, int pos)		/* dfs traversal modified for the circlefind prompt command - start from vertex "source" */
{
	int index;
	bucket_node *ptraverse1, *ptraverse2;
	adj_node *temp, *popped, *cycle_traverse, *scycle_traverse, *prev, *cur;

	/* locate vertex ni in bucket[pos]'s linked list */
	ptraverse1 = (*g)->b_heads[pos];
	while(strcmp(ptraverse1->name, ni) != 0) 
		ptraverse1 = ptraverse1->b_next;

	/* when vertex is located start performing dfs */
	ptraverse1->st = VISITED; 			/* mark the node as visited */
	temp = ptraverse1->head;

	/* start traversing its adj.list */
	while(temp != NULL) {					
		
		/* THIS LOOP OPERATES AS LONG AS THE ADJ.LIST OF THE NODE IS NOT EMPTY */

		/* we need to locate this adjacent node as a BUCKET NODE first,
		 * in order to start "exploring" its outgoing edges
		 */
		index = Hash(temp->name);			
		ptraverse2 = (*g)->b_heads[index];
		while(strcmp(ptraverse2->name, temp->name) != 0)
			ptraverse2 = ptraverse2->b_next;
		/* when the bucket node with the same name is located examine if it has already been visited */
		if( (ptraverse2->st == VISITED) && (strcmp(ptraverse2->name, source) == 0) ) {	

			/* WE FOUND A CYCLE! */
			if(!cycle_flag) {		/* this is the first cycle we find while dfs_c is operating */
				cycle_flag = TRUE;
				printf("\n\nCir-found |%s|" , source);
			}
			else
				printf("\n|%s|" , source); /* not the first simple cycle we find, given this specific source vertex */

			pushStack(temp->name, temp->weight);	/* push the last node, which is actually an edge in the form: ("somename"->)weight->"source" into the stack */
			popStack(&popped);
			cycle_traverse = popped;
			/* build the simple cycle path by popping the stack */
			/* we add every node we pop at the start of the list cycle_traverse */
			/* so the simple cycle is built backwards. 
			 * The first node we pop is the last one in the simple cycle path */

			while(emptyStack() == FALSE) {
				popStack(&popped);
				popped->next = cycle_traverse;
				cycle_traverse = popped;
			}
			/* if the global linked list "cycle" is not empty, we add the simple cycle path we previously constructed(by popping the stack) to it */
			/* after the last node of the global linked list "cycle". We basically merge the 2 lists */

			if(cycle == NULL) {		
				cycle = cycle_traverse;
			}
			else {
				scycle_traverse = cycle;
				while(1) {
					if(scycle_traverse->next == NULL) break;
					scycle_traverse = scycle_traverse->next;
				}
				scycle_traverse->next = cycle_traverse; /* merge... */
			}					
			/* print out the completed simple cycle path 
			 * reminder: the head of the new list is now "cycle"
			 */

			cycle_traverse = cycle;
			while(cycle_traverse != NULL) {
				printf("->%d->|%s|" , cycle_traverse->weight, cycle_traverse->name);
				cycle_traverse = cycle_traverse->next;
			}
			 
			/* delete the last element of the list "cycle"(which is basically the source vertex), since
			 * there might be more than one edges that lead to a simle cycle
			 * e.g 	 ...->"somename"->weight1->"source" and 
			 * 	 ...->"somename"->weight2->"source" etc.
			 */
			prev = cycle;
			cur = prev->next;
			while(strcmp(cur->name, source) != 0) {
				prev = cur;
				cur = cur->next;
			}
			prev->next = NULL;
			free(cur->name);
			free(cur);

			/* notice that the list "cycle" is not emptied out, because by continuing dfs from where we left off, we might stumble upon another simple cycle */
		}

		/* if the bucket node is not currently visiting */
		if(ptraverse2->st != VISITED) {
			if(ptraverse2->head != NULL) {				/* in this case push the node to the stack */
				pushStack(temp->name, temp->weight);		/* we need the edge's weight */
				DFS_c(g, ptraverse2->name, index);		/* continue performing dfs with this node */
			}
			else
				ptraverse2->st = VISITED;
		}

		temp = temp->next;	/* go to the next adjacency node */
	}
	/*** after the end of the while() loop ***/
	ptraverse1->st = VISITED;	/* after we have visited all the neighbours of this particular bucket node, mark it as visited */	
	if(emptyStack() == FALSE) 	/* if the bucket node pointed by "ptraverse1" was the last to be pushed into the stack, pop it */
		if(strcmp(stack_top->name, ptraverse1->name) == 0) {	
			popStack(&popped);
			free(popped->name);
			free(popped);
		}

	/* when a node is completely visited and its copy exists in the cycle linked list(at the end of "cycle"), remove it */
	if(cycle) {
		if(strcmp(cycle->name, ptraverse1->name) == 0) {	/* this node is the only element in the list */
			free(cycle->name);
			free(cycle);
			cycle = NULL;
		}
		else {					/* simply, delete the last element of the list */
			prev = cycle;
			cur = cycle->next;
			while(cur) {
				if(cur->next == NULL) break;
				prev = cur;
				cur = cur->next;
			}
			if( (cur != NULL) && (strcmp(cur->name, ptraverse1->name) == 0) ) {
				prev->next = NULL;
				free(cur->name);
				free(cur);
			}
		}
	}
}


void DFS_f(Graph **g, vertex_data ni, int minwt, int pos) 
{
	int index;
	bucket_node *ptraverse1, *ptraverse2;
	adj_node *temp, *popped, *cycle_traverse, *scycle_traverse, *prev, *cur, *new_blocked;
	Boolean blocked; 

	/* locate vertex "ni" in bucket[pos]'s linked list */
	ptraverse1 = (*g)->b_heads[pos];
	while(strcmp(ptraverse1->name, ni) != 0) 
		ptraverse1 = ptraverse1->b_next;

	/* when vertex ni is located start performing dfs */
	ptraverse1->active_stack_count++;		/* stack_count is used towards the end of a function, in order not to remove an edge in "cycle" list which will be used later in a new cycle */
	temp = ptraverse1->head;

	/* start "exploring" its adjacency list */
	while(temp != NULL) {					

		/* THIS LOOP OPERATES AS LONG AS THE ADJ.LIST OF THE NODE IS NOT EMPTY */

		/* If edge's weight value is less than minwt skip the edge */
		if(temp->weight >= minwt) {

			blocked = FALSE;
			/* we need to locate this particular adjacent node as a BUCKET NODE */
			index = Hash(temp->name);			
			ptraverse2 = (*g)->b_heads[index];
			while(strcmp(ptraverse2->name, temp->name) != 0)
				ptraverse2 = ptraverse2->b_next;

			/* when the bucket node with the same name as the source vertex has been found,
			 * check the blocked_set - a cycle has not been found yet. 
			 * if the edge, which normally leads to a cycle(e.g in the form: ...(->"somename")->weight->"source")
			 * is not included in the "blocked_set", then the cycle is valid
			 */
			if(strcmp(ptraverse2->name, source) == 0) {	

				/* in particular, check if the edge that leads to the cycle is included in the blocked_set */
				cur = blocked_set;	
				while(cur != NULL) {
					if( (strcmp(cur->name, temp->name) == 0) && (temp->weight == cur->weight) ) {	/* if the edge's weight in the blocked_set is different, it is safe
															   to assume that the edge is not repeated */
						blocked = TRUE;
						break;
					}
					cur = cur->next;
				}
				if(blocked == FALSE) {

					/* bucket node is not blocked and it's the same as the source vertex, so we found a cycle! */
					if(!cycle_flag) {
						cycle_flag = TRUE;
						printf("\n\nCir-found |%s|" , source);
					}
					else
						printf("\n|%s|" , source);

					pushStack(temp->name, temp->weight); /* push the last node, which is actually an edge in the form: ("somename"->)weight->"source" into the stack */
					popStack(&popped);
					cycle_traverse = popped;
	
					/* build the simple cycle path by popping the stack */
					/* we add every node we pop at the start of the list cycle_traverse */
					while(emptyStack() == FALSE) {
						popStack(&popped);
						popped->next = cycle_traverse;
						cycle_traverse = popped;
					}
	
					if(cycle == NULL) {
						cycle = cycle_traverse;
					}
					else {
						scycle_traverse = cycle;
						while(1) {
							if(scycle_traverse->next == NULL) break;
							scycle_traverse = scycle_traverse->next;
						}
						scycle_traverse->next = cycle_traverse;
					}					
								
					/* print the simple cycle path */
					cycle_traverse = cycle;
					while(cycle_traverse != NULL) {
						printf("->%d->|%s|" , cycle_traverse->weight, cycle_traverse->name);
						cycle_traverse = cycle_traverse->next;
					}

					/* after the cycle path is printed out */
					/* we can keep searching since another cycle might be found */
				}
						 
			}
			/* cycles of the form ni->...->ni->...->ni might exist */
	
			cur = blocked_set;	
			/* search the "blocked_set" for the edge we are about to ("explore")do dfs on */
			/* only if both the name and the weight of the edge matches the one on the blocked set, we skip the edge */
			while(cur != NULL) {
				if( (strcmp(cur->name, temp->name) == 0) && (temp->weight == cur->weight) ) {
					blocked = TRUE;
					break;
				}	
				cur = cur->next;
			}
			if(blocked == FALSE) {	/* if the edge is not blocked */

				if(ptraverse2->head != NULL) {		/* in this case, add the edge to the "blocked_set" */
					new_blocked = NewAdjNode(temp->name, temp->weight);
					if(blocked_set == NULL) {
						blocked_set = new_blocked;
					}
					else {
						new_blocked->next = blocked_set;
						blocked_set = new_blocked;
					}					
					/* the if statement above(which constructs and prints out a cycle) was entered and a cycle was indeed found, 
					 * so do not add the source vertex/edge in the stack again. 
					 * the source edge is already included in the list "cycle" */
					if(strcmp(ptraverse2->name, source) != 0)
						pushStack(temp->name, temp->weight);	/* we need the edge's weight */
					DFS_f(g, ptraverse2->name, minwt, index);	/* recursively, perform DFS_f on the new edge*/
				}
			}
		}
		temp = temp->next;
	}
	/*** after while() loop exits ***/

	if(emptyStack() == FALSE) 		/* therefore, if "ni" is at the top of the stack, pop it */
		if(strcmp(stack_top->name, ptraverse1->name) == 0) {
			popStack(&popped);
			free(popped->name);
			free(popped);
		}
	/* next, delete the completely visited node from both the cycle linked list and the blocked set(if it exists in the 2 lists) */
	/* first delete it from the linked list "cycle"
	 * note: if the node is included in the "cycle" it should  be at the end of the list
	 */
	if(cycle) {
		int num = 0;	
		int asc;

		cycle_traverse = cycle;
		while(cycle_traverse) {
			if(strcmp(cycle_traverse->name, ptraverse1->name) == 0) num++;
			cycle_traverse = cycle_traverse->next;
		}
		if(strcmp(ptraverse1->name, source) == 0) asc = ptraverse1->active_stack_count - 1;
		else asc = ptraverse1->active_stack_count;
		if( asc <= num ) {	/* in some cases an edge inside "cycle" is deleted by mistake resulting in cycles, which are missing an edge. This resolves the issue */
			prev = cycle;
			cur = cycle->next;
			if( (cur == NULL) && (strcmp(prev->name, ptraverse1->name) == 0) ) {	/* node is the only element in the list */
				free(prev->name);
				free(prev);
				cycle = NULL;
			}
			else {
				while(cur != NULL) {
					if( (strcmp(cur->name, ptraverse1->name) == 0) && (cur->next == NULL) ) {
						prev->next = NULL;
						free(cur->name);
						free(cur);
						break;
					}
					prev = cur;
					cur = cur->next;
				}
			}
		}
	}

	/* next delete it from the blocked set */
	if(blocked_set) {
		prev = blocked_set;
		blocked_set = blocked_set->next;	/* is it always at the start of the blocked_set? */
		free(prev->name);
		free(prev);
	}

	/********************************/
	--ptraverse1->active_stack_count;
}


void DFS_t(Graph **g, vertex_data ni, int max_trace, int pos)
{
	int index, rem;
	bucket_node *ptraverse1, *ptraverse2;
	adj_node *temp, *popped, *cycle_traverse, *scycle_traverse, *prev, *cur, *new_blocked;
	Boolean blocked;


	/* locate vertex ni in bucket[pos]'s linked list */
	ptraverse1 = (*g)->b_heads[pos];
	while(strcmp(ptraverse1->name, ni) != 0) 
		ptraverse1 = ptraverse1->b_next;

	++ptraverse1->active_stack_count;
	/* when vertex ni is located start performing dfs */
	temp = ptraverse1->head;

	while(temp != NULL) {					

		/* THIS LOOP OPERATES AS LONG AS THE ADJ.LIST OF THE NODE IS NOT EMPTY */

		blocked = FALSE;

		/* we need to locate this adjacent node as a BUCKET NODE */
		index = Hash(temp->name);			
		ptraverse2 = (*g)->b_heads[index];
		while(strcmp(ptraverse2->name, temp->name) != 0)
			ptraverse2 = ptraverse2->b_next;

		/* when the bucket node with the same name as the source vertex is found and there are edges remaining */
		if( (strcmp(ptraverse2->name, dest) == 0) && (max_trace > 0) ) {	

			/* Search for the edge that normally leads to a cycle 
			 * if the edge is included in the "blocked_set", it is not valid
			 * and therefore, the cycle is invalid
			 */
			cur = blocked_set;	
			while(cur != NULL) {
				if( (strcmp(cur->name, temp->name) == 0) && (temp->weight == cur->weight) ) {
					blocked = TRUE;
					break;
				}
				cur = cur->next;
			}
			if(blocked == FALSE) {	/* if the edge was not included in the "blocked_set" */

				/* WE FOUND A TRACE! */
				if(!cycle_flag) {
					cycle_flag = TRUE;
					printf("\n\nTra-found |%s|" , source);
				}
				else
					printf("\n|%s|" , source);

				pushStack(temp->name, temp->weight);	    
				popStack(&popped);
				cycle_traverse = popped;
	
				/* build the simple cycle path by popping the stack */
				/* we add every node we pop at the start of the list "cycle_traverse" */
				while(emptyStack() == FALSE) {
					popStack(&popped);
					popped->next = cycle_traverse;
					cycle_traverse = popped;
				}
				if(cycle == NULL) {
					cycle = cycle_traverse;
				}
				else {
					scycle_traverse = cycle;
					while(1) {
						if(scycle_traverse->next == NULL) break;
						scycle_traverse = scycle_traverse->next;
					}
					scycle_traverse->next = cycle_traverse;
				}					
				/* print the simple cycle path */
				cycle_traverse = cycle;
				while(cycle_traverse != NULL) {
					printf("->%d->|%s|" , cycle_traverse->weight, cycle_traverse->name);
					if( (max_trace == 1) && (cycle_traverse->next != NULL) ) {	
						if(cycle_traverse->next->next == NULL) {
							printf("->%d->|%s|" , cycle_traverse->next->weight, cycle_traverse->next->name);
							free(cycle_traverse->next->name);
							free(cycle_traverse->next);
							cycle_traverse->next = NULL;

							break;
						}
					}
					cycle_traverse = cycle_traverse->next;
				}
			}
		}
	
		if( (max_trace - 1) > 0 )  {	/* if maxtrace = 1, there is no need visiting any vertex other than the dest */
						/* and even in this case the previous if statement would have been entered, indicating that a trace was found */

			cur = blocked_set;	/* again, check if the edge we are about to "discover" is in the "blocked_set" */	
			while(cur != NULL) {
				if( (strcmp(cur->name, temp->name) == 0) && (temp->weight == cur->weight) ) {
					blocked = TRUE;
					break;
				}	
				cur = cur->next;
			}
			if(blocked == FALSE) {

				if(ptraverse2->head != NULL) {						
					

					new_blocked = NewAdjNode(temp->name, temp->weight);
					if(blocked_set == NULL)
						blocked_set = new_blocked;
					else {
						new_blocked->next = blocked_set;
						blocked_set = new_blocked;
					}					
					if(strcmp(ptraverse2->name, dest) != 0) 
						pushStack(temp->name, temp->weight);	/* we need the edge's weight */

					/* decrement max_trace since a new edge is about to be visited */
					rem = max_trace - 1;
					DFS_t(g, ptraverse2->name, rem, index);			

				}
			}
		}
		temp = temp->next;
	}
	/*** after the while() loop exits ***/

	/* before returning to the previous call of DFS */
	if(emptyStack() == FALSE) {
		if(strcmp(stack_top->name, ptraverse1->name) == 0) {
			popStack(&popped);
			free(popped->name);
			free(popped);
		}
	}
	/* delete the completely visited node from both the cycle linked list and the blocked set */
	/* first delete it from the cycle linked list */
	if(cycle) {

		int num = 0;
		int asc;

		cycle_traverse = cycle;
		while(cycle_traverse) {
			if(strcmp(cycle_traverse->name, ptraverse1->name) == 0) ++num;
			cycle_traverse = cycle_traverse->next;
		}
		if(strcmp(source, ptraverse1->name) == 0) asc = ptraverse1->active_stack_count - 1;
		else asc = ptraverse1->active_stack_count;

		if(asc <= num) {
			prev = cycle;
			cur = cycle->next;
			if( (cur == NULL) && (strcmp(prev->name, ptraverse1->name) == 0) ) {	/* node is the only element in the list */
				free(prev->name);
				free(prev);
				cycle = NULL;
			}
			else {
				while(cur != NULL) {
					if( (strcmp(cur->name, ptraverse1->name) == 0) && (cur->next == NULL) ) {
						prev->next = NULL;
						free(cur->name);
						free(cur);
						break;
					}
					prev = cur;
					cur = cur->next;
				}
			}
		}
	}
	/* next delete it from the blocked set */
	if(blocked_set) {
		prev = blocked_set;
		blocked_set = blocked_set->next;
		free(prev->name);
		free(prev);
	}

	--ptraverse1->active_stack_count;
}
