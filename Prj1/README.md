
This is the introductory project of the Operating Systems course. In this project, we create a directed multi-graph data 
structure which depicts bank account transactions between users.

The graph is implemented using a hash table of 26 buckets, one bucket for each letter in the english alphabet. We use the 
seperate chaining approach to resolve collisions, and therefore keys, which hash to the same bucket form a linked list. When 
we wish to insert a new vertex/"bucket" node in the graph, we use its bank account name, and more specifically, its first
letter to decide in which bucket it should be added. Obviously, bank account names starting with the same letter hash to the 
same bucket. In adittion, each vertex/"bucket" node has a list of transactions with other bank accounts. These transactions 
are implemented with adjacency list nodes and each adjacency list node also has, among others, a field "weight", which 
resembles the transaction amount between the 2 nodes. Basically, these transactions are the outgoing edges of each vertex. 
There could be one or more transactions between the same 2 nodes with either the same or different weight. Of course, the
size of the graph can change dynamically.  

###### examples
"Bob"->1500->"Liz"(1): transaction between bucket node "Bob" and adjacent node "Liz" with weight 1500  
"Bob"->700->"Liz" could also exist and even  
"Bob"->1500->"Liz"(2) is valid  

Here is the list of the prompt interface commands:

- to insert a new node "Ni" in the graph:  
  `i Ni`

- to insert a new edge between "Ni" and "Nj" with weight "weight"("Ni"->weight->"Nj"):  
  `n Ni Nj weight`

- to delete a node "Ni" and all its incoming/outcoming edges:  
  `d Ni`

- to delete an edge between "Ni" and "Nj" with weight "weight":  
  `l Ni Nj weight`

- to modify an edge's weight between "Ni" and "Nj":  
  `m Ni Nj weight newweight`

- to print out all the receiving/incoming edges of "Ni":  
  `r Ni`

- to find if "Ni" is involved in *simple cycles* + print out these simple cycles:  
  `c Ni`  
   i.e   **_Ni_->w1->A->w2->B->w3->_Ni_**

- to find if "Ni" is involved in *cyclic transactions* with other accounts:  
  `f Ni k`  
  k is the minimum weight each outgoing edge can have  
  i.e   (min.weight 100)  **_Ni_->200->A->150->B->707->Ni->360->C->102->_Ni_**
  
- to find if a transaction starts at "Ni" and ends at "Nj":  
  `t Ni Nj l`  
  l is the maximum distance allowed measured in number of edges
  
  ## Execution  
  ```
  $ make
  $ ./mygraph -i TestData-Prj1-F18/InputFile-Small.csv -o mysmall ( < workloads/Small/workload-Small > mysmall_out )  
  ```
  where after the -i flag is the input file for the graph creation, after the -o flag is the output file where the final 
  state of the graph will be printed out. The part inside the parentheses is optional. 
