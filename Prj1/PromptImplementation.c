/* PromptImplementation.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Utilities.h"


void InsertNode(Graph **g, vertex_data ni)		/* insert a new bucket node with name "ni" */
{
	int index;
	bucket_node *node;

	index = Hash(ni);					/* map the string to an integer value in the range [0, 25] */
	if(SearchBucketNode(g, ni, index) == FALSE) {		/* if the given node does not exist, add it at the "top" of the bucket's linked list */
		node = NewBucketNode(ni);			
		node->b_next = (*g)->b_heads[index];		/* this bucket(b_heads[index]) could either point to NULL(empty) or to a valid bucket node */
		(*g)->b_heads[index] = node;	
		++(*g)->V;					/* increment total vertices in the graph */
		printf("\nInserted |%s|\n" , ni);		/* display message */
	}
	else 	/* if the given node already exists in the bucket's linked list */
		printf("\nNode |%s| Exists;\n" , ni);		/* display message */
}


void InsertEdge(Graph **g, vertex_data ni, vertex_data nj, int wt)		/* insert a new edge: "ni"->weight->"nj" */
{
	int index1, index2;
	bucket_node *ptraverse, *b_new;
	adj_node *a_new;

	index1 = Hash(ni);
	index2 = Hash(nj);

	if(SearchBucketNode(g, ni, index1) == FALSE) {		/* if node "ni" DOES NOT exist, create it and add it at the appropriate bucket */
		b_new = NewBucketNode(ni);
		b_new->b_next = (*g)->b_heads[index1];
		(*g)->b_heads[index1] = b_new;
		++(*g)->V; /* increment number of vertices in the graph */
	}
		
	if(SearchBucketNode(g, nj, index2) == FALSE) {		/* if node "nj" DOES NOT exist, create it and add it to the appropriate bucket list */
		b_new = NewBucketNode(nj);
		b_new->b_next = (*g)->b_heads[index2];
		(*g)->b_heads[index2] = b_new;
		++(*g)->V; /* increment number of vertices in the graph */
	}
		
	/* now insert node nj to the adj.list of Bucket Node ni */
	/* first, search for bucket node ni */
	ptraverse = (*g)->b_heads[index1];		
	while( strcmp(ni, ptraverse->name) != 0 ) 
		ptraverse = ptraverse->b_next;
	/* next, add adj list node "nj" to "ni" */
	a_new = NewAdjNode(nj, wt);			/* create a new adjacency list node with name nj and weight wt */
	a_new->next = ptraverse->head;
	ptraverse->head = a_new;
	printf("\nInserted |%s|->%d->|%s|\n" , ni, wt, nj); /* display message */
}


void DeleteNode(Graph **g, vertex_data ni)			/* delete bucket node with name "ni" and all of its incoming/outgoing edges */
{
	int i, index;
	bucket_node *ptraverse, *b_prev, *b_cur;
	adj_node *a_prev, *a_cur;
	
	index = Hash(ni);					/* map the string ni to an integer */
	if(SearchBucketNode(g, ni, index) == TRUE) {		/* IF THE NODE WITH THE GIVEN NAME EXISTS */

		/* FIRST, locate bucket node ni */
		ptraverse = (*g)->b_heads[index];

		/* 1st CASE: NODE TO BE DELETED IS THE FIRST ONE IN THE BUCKET'S LINKED LIST */	
		if(strcmp(ptraverse->name, ni) == 0) {		/* start the deletion process */		
			if(ptraverse->head != NULL) 		/* bucket node to be deleted has adjacent nodes; delete ni's outgoing edges */
				delete_list( &ptraverse->head );

			/* after we have deleted ni's outgoing edges, we can delete node ni itself */
			(*g)->b_heads[index] = ptraverse->b_next;	/* bucket's linked list is now pointing to the next of ptraverse */
			free(ptraverse->name);
			free(ptraverse);
			--(*g)->V;					/* decrement graph's total (bucket) nodes/vertices */
		}
		
		/* 2nd CASE: NODE TO BE DELETED IS EITHER IN BETWEEN OR THE LAST NODE OF THE BUCKET'S LINKED LIST */
		else {	
			b_prev = (*g)->b_heads[index];	
			b_cur =  b_prev->b_next;
			while(strcmp(b_cur->name, ni) != 0) {		/* traverse the bucket's linked list until we find the bucket node we want to delete */
				b_prev = b_cur;
				b_cur = b_cur->b_next;
			}
			/* we found the bucket node we want to delete */
			if(b_cur->head != NULL) 			/* make sure the node to be deleted has adjacent nodes */
				delete_list( &b_cur->head );

			/* after we have deleted ni's outcoming edges, we can delete node ni itself */
			b_prev->b_next = b_cur->b_next;
			free(b_cur->name);
			free(b_cur);
			--(*g)->V;					/* decrement graph's total (bucket) nodes/vertices */
		}


		/* BUCKET NODE NI AND ITS ADJACENCY LIST HAVE BEEN BOTH DELETED. NOW WE NEED TO DELETE ALL THE INCOMING EDGES OF NI
		 * so, we are basically looking for edges in the form: "somename"->weight->"ni" 
		 */

		/* search through all bucket nodes' adjacency lists(edges) for node ni */
		for(i = 0; i < TABLESIZE; ++i) {		
			ptraverse = (*g)->b_heads[i];			
			while(ptraverse != NULL) {			/* while there are still nodes to traverse in each bucket's linked list */

				/* search the bucket node's adjacency list for node ni */
				if(ptraverse->head != NULL) {		/* if node' s adjacency list is empty continue to the next bucket node */
					a_prev = ptraverse->head;

					/* 1st CASE: NODE NI IS THE FIRST NODE IN THE ADJACENCY LIST */
					if(strcmp(a_prev->name, ni) == 0) {		/* if node ni IS THE FIRST NODE in the adjacency list */
						ptraverse->head = a_prev->next;		/* adjacency list now points to the next of ni(NULL or a valid adj_node) */	
						free(a_prev->name);
						free(a_prev);
					}
					/* 2nd CASE: NODE NI IS EITHER IN BETWEEN OR THE LAST NODE OF THE ADJACENCY LIST */
					else {						/* if node ni IS NOT THE FIRST NODE in the adjacency list */
						a_prev = ptraverse->head;
						a_cur = a_prev->next;
						while(a_cur != NULL) {				/* traverse the adjacency list until we reach NULL */
							if(strcmp(a_cur->name, ni) == 0) {	/* we found the node ni */
								a_prev->next = a_cur->next;	/* adjust the links */
								free(a_cur->name);
								free(a_cur);			/* delete this incoming edge of ni */
								break;
							}
							a_prev = a_cur;
							a_cur = a_cur->next;
						}
					}
				}

				ptraverse = ptraverse->b_next;
			}
		}
		printf("\nDeleted |%s|\n" , ni);
	}
	/* NODE TO BE DELETED DOES NOT EXIST */
	else 
		printf("\nNode |%s| does not exist - abort-d;\n" , ni);
}


void DeleteEdge(Graph **g, vertex_data ni, vertex_data nj, int wt)	/* delete an edge (ni,nj), which has the given weight wt */
{
	int index, flag = 0;
	adj_node *prev, *cur;
	bucket_node *ptraverse;

	index = Hash(ni);
	if(SearchBucketNode(g, ni, index) == TRUE) {			/* bucket node "ni" exists in the graph */
		ptraverse = (*g)->b_heads[index];
		while(strcmp(ptraverse->name, ni) != 0) 		/* loop until you find bucket node "ni" */
			ptraverse = ptraverse->b_next;
		/* we found bucket node ni */
		cur = ptraverse->head;
		if(cur != NULL) {					/* if ni's adjacency list is empty no action is required */

			/* 1st CASE: NODE NJ IS THE FIRST NODE IN THE ADJ.LIST OF NI */
			if(strcmp(cur->name, nj) == 0) {				/* nj is the first node in the adj.list of ni and edge (ni,nj) exists */
				flag = 1;						/* edge (ni,nj) might not have the required weight wt though */
				if(cur->weight == wt) {					/* nj is the first node in the adj.list of ni and the edge (ni,nj) has the required weight */ 
					ptraverse->head = cur->next;
					free(cur->name);
					free(cur);
					printf("\nDel-vertex |%s|->%d->|%s|\n" , ni, wt, nj);	/* display message */
					flag = 2;
				}
			}

			/* 2nd CASE: NODE NJ IS EITHER IN BETWEEN OR IT IS THE LAST NODE IN NI'S ADJ.LIST */
			if(flag != 2) {							/* An edge "ni"->wt->"nj" was not yet found */					
				prev = ptraverse->head;
				cur = prev->next;
				while(cur != NULL) {					/* traverse ni's adj.list until we reach null */
					if(strcmp(cur->name, nj) == 0) {		/* we did find an adj.node nj */
						flag = 1;
						if(cur->weight == wt) {			/* edge (ni,nj) has the required weight wt */
							prev->next = cur->next;
							free(cur->name);
							free(cur);
							printf("\nDel-vertex |%s|->%d->|%s|\n" , ni, wt, nj);	/* display message */
							flag = 2;
							break;
						}
					}
					prev = cur;
					cur = cur->next;
				}
			}
		}
		/* Display appropriate error message */
		if(!flag)
			printf("\n|%s| does not exist - abort-l;\n" , nj);			/* node nj was not found in the adj.list of ni - display error message */	
		if(flag == 1)
			printf("\n|%s|->%d->|%s| does not exist - abort-l;\n" , ni, wt, nj);	/* an edge (ni,nj) was found, but not with the required weight - display error message */	
	}
	else
		printf("\n|%s| does not exist - abort-l;\n" , ni);				/* bucket node ni does not exist in the graph - display error message */

}


void DeleteAllEdges(Graph **g, vertex_data ni, vertex_data nj)			/* delete all the edges in the form (ni,nj). Edge's weight can have any value */
{
	int index, flag = 0;
	bucket_node *ptraverse;
	adj_node *temp, *prev, *cur;

	index = Hash(ni);
	if(SearchBucketNode(g, ni, index) == TRUE) {			/* IF NODE NI EXISTS */

		ptraverse = (*g)->b_heads[index];
		while(strcmp(ptraverse->name, ni) != 0)			/* traverse bucket[index]' s linked list until bucket node ni is found */
			ptraverse = ptraverse->b_next;
		temp = ptraverse->head;
		/* FIRST, CHECK  IF MULTIPLE NJ NODES EXIST AT THE START OF NI'S ADJ.LIST */
		if(temp != NULL) {				/* if ni's adj.list is not empty */
			while(strcmp(temp->name, nj) == 0) {	/* repeat this loop while a node nj is at the start of ni's adj.list */
				ptraverse->head = temp->next;
				printf("\nDel-vertex |%s|->%d->|%s|\n" , ni, temp->weight, nj);	/* display message */
				free(temp->name);
				free(temp);
				temp = ptraverse->head;
				flag = 1;
			}
		}
		/* now if the previous loop did not leave ni's adj.list empty another nj node(s) might exist */
		if(ptraverse->head != NULL) {				
			prev = ptraverse->head;
			cur = prev->next;
			while(cur != NULL) { 				
				
				/* 2nd CASE: NODE NJ IS EITHER IN BETWEEN OR IT IS THE LAST NODE IN NI'S ADJ.LIST */
				if(strcmp(cur->name, nj) == 0) {		/* we did find an adj.node nj */
						prev->next = cur->next;
						printf("\nDel-vertex |%s|->%d->|%s|\n" , ni, cur->weight, nj);	/* display message */
						free(cur->name);
						free(cur);
						flag = 1;
				}
				prev = cur;
				cur = cur->next;
			}
		}
		/* Display appropriate error message */
		if(!flag)
			printf("\n|%s| does not exist - abort-l;\n" , nj);		/* node nj was not found in the adj.list of ni, display error message */	
	}
	/* NODE NI DOES NOT EXIST */
	else
		printf("\n|%s| does not exist - abort-l;\n", ni);
}


void ModifyWeight(Graph **g, vertex_data ni, vertex_data nj, int wt, int nwt)		/* find the edge (ni,nj) with weight wt and change it to nwt */ 
{
	int  index, flag = 0;
	adj_node *temp;
	bucket_node *ptraverse;

	index = Hash(ni);
	if(SearchBucketNode(g, ni, index) == TRUE) {		/* if bucket node "ni" exists */
		ptraverse = (*g)->b_heads[index];
		while(strcmp(ptraverse->name, ni) != 0) 	/* traverse bucket[index]' s linked list unti node ni is found */
			ptraverse = ptraverse->b_next;
		/* bucket node ni was found */
		temp = ptraverse->head;
		while(temp != NULL) {				/* now traverse ni's adj.list until we reach null */
			if(strcmp(temp->name, nj) == 0) {	/* if an edge (ni,nj) is found check its weight */
				flag = 1;
				if(temp->weight == wt) {	/* if edge (ni,nj) has the required weight, modify it */
					flag = 2;
					temp->weight = nwt;
					printf("\nMod-vertex |%s|->%d->|%s|\n", ni, nwt, nj);		/* edge (ni,nj) with the given weight was changed to nwt */
					break;	/* since the edge's weight was modified break the loop */
				}
			}
			temp = temp->next;
		}
		/* print out the appropriate error message */
		if(flag == 1)							/* an edge (ni,nj) was found but not with the given weight wt */ 
			printf("\n|%s|->%d->|%s| does not exist - abort-m;\n" , ni, wt, nj);/* display error message */	
		if(!flag)							
			printf("\n|%s| does not exist - abort-m;\n" , nj);		/* node nj was not found in the adj. list of ni, display error message */	
	}
	/* NODE NI DOES NOT EXIST */
	else
		printf("\n|%s| does not exist - abort-m;\n" , ni);		/* if head node ni does not exist in the graph, display error message */
}


void ReceivingEdges(Graph **g, vertex_data ni)					/* find all the receiving(incoming) edges of node "ni" */
{
	int i, index, flag = 0;
	bucket_node *ptraverse;
	adj_node *temp;

	index = Hash(ni);
	if(SearchBucketNode(g, ni, index) == TRUE) {				/* if bucket node ni exists in the graph */
		
		for(i = 0; i < TABLESIZE; ++i) {				/* traverse each bucket's linked list in the graph  */
			ptraverse = (*g)->b_heads[i];
			while(ptraverse != NULL) {				/* while we haven't reached the end of the bucket's linked list */
				temp = ptraverse->head;
				while(temp != NULL) {				/* while we haven't reached the end of the bucket node's adj.list */
					if(strcmp(temp->name, ni) == 0) { 	/* if we did find node ni in the adj.list of another bucket node */
						if(!flag) {
							flag = 1;
							printf("\nRec-edges |%s|->%d->|%s|\n" , ptraverse->name, temp->weight, ni);	/* display message */
						}
						else
							printf("|%s|->%d->|%s|\n" , ptraverse->name, temp->weight, ni);
					}
					temp = temp->next;
				}
				ptraverse = ptraverse->b_next;
			}
		}
		if(!flag)					/* if flag was not changed to 1 during the FOR loop, no rec-edges were found for node ni */
			printf("\nNo-rec-edges |%s|\n" , ni);
	
	}
	else 	/* node ni does not exist in the graph */
		printf("\n|%s| does not exist - abort-r;\n" , ni);
}


void CircleFind(Graph **g, vertex_data ni)				/* given a source vertex "ni" this function discovers simple cycles: e.g ni->nj->nk->ni */
{
	int index = Hash(ni);
	if(SearchBucketNode(g, ni, index) == TRUE) {			/* if node ni exists in the graph */
		cycle_flag = FALSE;
		strcpy(source, ni);
		DFS_c(g, ni, index);					/* perform dfs with "ni" as the source vertex */
		if(!cycle_flag) 
			printf("\nNo-circle-found |%s|\n" , ni);
	}		
	/* node ni does not exist in the graph */
	else	
		printf("|%s| does not exist - abort-c;\n" , ni);
}


void FindCircles(Graph **g, vertex_data ni, int minwt)
{
	int index = Hash(ni);
	if(SearchBucketNode(g, ni, index) == TRUE) {			/* if node ni exists in the graph */

		cycle_flag = FALSE;
		strcpy(source, ni);
		DFS_f(g, ni, minwt, index);				/* perform dfs with "ni" as the source vertex */
		if(!cycle_flag)
			printf("\nNo-circle-found involving |%s| %d\n" , ni, minwt);
	}		
	/* node ni does not exist in the graph */
	else	
		printf("|%s| does not exist - abort-f;\n" , ni);
}

void Traceflow(Graph **g, vertex_data ni, vertex_data nj, int trace_length)
{
	int flag = 0;

	int index1 = Hash(ni);
	int index2 = Hash(nj);
	if(SearchBucketNode(g, ni, index1) == TRUE) {
		flag = 1;
		if(SearchBucketNode(g, nj, index2) == TRUE) {
			
			flag = 2;
			cycle_flag = FALSE;
			strcpy(source, ni);
			strcpy(dest, nj);
			DFS_t(g, ni, trace_length, index1);		/* perform dfs with "ni" as the source vertex */
			if(!cycle_flag)
				printf("\nNo-trace from |%s| to |%s|\n" , ni, nj);

			/* free "cycle" list */
			if(cycle) {
				adj_node *n, *temp;
				for(n = cycle; n != NULL; n = temp) {
					temp = n->next;
					free(n->name);
					free(n);
				}
				cycle = NULL;
			}
		}
	}
	if(!flag)
		printf("\n|%s| does not exist - abort-t;\n" , ni);
	if(flag == 1)
		printf("\n|%s| does not exist - abort-t;\n" , nj);

}

void Exit(Graph **g)
{
	int i;
	bucket_node *b_prev, *b_cur;

	for(i = 0; i < TABLESIZE; ++i) {		/* run this loop for every bucket */

		if( (*g)->b_heads[i] != NULL) {		/* skip the bucket if its linked list is empty */

			b_prev = (*g)->b_heads[i];	/* pointers for the traversal of the bucket's linked list */
			b_cur = b_prev->b_next;

			/* before we delete the node b_prev we must first delete its name and its adj.list */
			while(b_cur != NULL) {

				/* delete b_prev's adj.list */
				if(b_prev->head != NULL)
					delete_list(&b_prev->head);

				/* after that we can delete b_prev */
				free(b_prev->name);
				free(b_prev);
				b_prev = b_cur;
				b_cur = b_cur->b_next;
			}

			/* we are about to delete the last node in the bucket's linked list */
			/* first delete its adj.list */
			if(b_prev->head != NULL) 
				delete_list(&b_prev->head);
			/* now we can delete the last bucket node of this bucket */
			free(b_prev->name);
			free(b_prev);
			(*g)->b_heads[i] = NULL;
		}
	}
	free( (*g) );
	(*g) = NULL;

	printf("\nexit program\n");
}


void Exit_out(Graph **g, char *filename)
{
	int i;
	bucket_node *b_prev, *b_cur;
	adj_node *a_prev, *a_cur;

	FILE *f_handle = fopen(filename, "w");
	if(f_handle == NULL) {
		printf("Couldn't open file!\n");
		exit (-1);
	}    

	for(i = 0; i < TABLESIZE; ++i) {		/* run this loop for every bucket */

		if( (*g)->b_heads[i] != NULL) {		/* skip the bucket if its linked list is already empty */

			b_prev = (*g)->b_heads[i];	/* pointers for the traversal of the bucket's linked list */
			b_cur = b_prev->b_next;
			fprintf(f_handle, "\n|%s|" , b_prev->name); 

			/* before we delete the node b_prev we must first delete its name and its adj.list */
			while(b_cur != NULL) {
				
				/* delete b_prev's adj.list */
				if(b_prev->head != NULL) {

					a_prev = b_prev->head;
					a_cur = a_prev->next;
					while(a_cur != NULL) {
						
						fprintf(f_handle, "\n  -%d->|%s|" , a_prev->weight, a_prev->name);						
						free(a_prev->name);
						free(a_prev);
						a_prev = a_cur;
						a_cur = a_cur->next;
					}
					fprintf(f_handle, "\n  -%d->|%s|" , a_prev->weight, a_prev->name);						
					free(a_prev->name);
					free(a_prev);

				}
				fprintf(f_handle, "\n");

				/* after that we can delete b_prev */
				free(b_prev->name);
				free(b_prev);
				b_prev = b_cur;
				b_cur = b_cur->b_next;
				fprintf(f_handle, "\n|%s|" , b_prev->name); 
			}

			/* we are about to delete the last node in the bucket's linked list */
			/* first delete its adj.list */
			if(b_prev->head != NULL) {

				a_prev = b_prev->head;
				a_cur = a_prev->next;
				while(a_cur != NULL) {
					fprintf(f_handle, "\n  -%d->|%s|" , a_prev->weight, a_prev->name);						
					free(a_prev->name);
					free(a_prev);
					a_prev = a_cur;
					a_cur = a_cur->next;
				}
				fprintf(f_handle, "\n  -%d->|%s|" , a_prev->weight, a_prev->name);						
				free(a_prev->name);
				free(a_prev);
			}
			/* now we can delete the last bucket node */
			free(b_prev->name);
			free(b_prev);
			(*g)->b_heads[i] = NULL;
		}
	}
	fclose(f_handle);
	free( (*g) );
	(*g) = NULL;

	printf("\nexit program\n");
}

