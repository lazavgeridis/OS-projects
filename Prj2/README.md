
## Description
This is the 2nd project of the Operating Systems course. In this project, an initial(root) process creates new children processes using the **_fork()_** system call. The children processes produce an hierarchy of processes similar to a **complete binary tree**. 

The purpose of this project was to execute queries on binary files of fixed length records and to sort the results afterwards. Leaf nodes(**searcher processes**) are given the task of searching in specific parts of the binary file, whereas internal nodes(**splitter/merger processes**) merge the results retrieved from their children and "push" these results to their parents. After the search is concluded, the *sort()* system program performs, as its name indicates, the final sorting of the search results.

![Screenshot](BinaryTree.png)

1. The **initial-root process** forks off the first splitter/merger process(**_sm0_**) passing to it the necessary arguments: height of the remaining tree, the binary datafile, the requested pattern of the search etc. When the query has been executed and the root has gathered all the search results, it uses the *sort()* system program to sort these results. In addition, the root process prints out time statistics related to the execution time of the query.
2. A **splitter/merger** process receives, as mentioned above, the following arguments among others: height of the remaining tree(e.g if the height of the binary tree is 3, sm0 receives height = 3 as its argument, but both sm1 and sm2 will receive h = 2 as their argument), the binary file of records, the pattern of the search, and the range of the search. If the height argument *is not 1*, the splitter/merger process forks off 2 new children processes that will run the **splitter/merger executable**. If the height argument *is 1*, the splitter/merger process forks off 2 new children process that will run the **searcher/leaf node executable**. Before terminating, a splitter/merger process receives the search results "pushed" by its children(with the help of the 2 **_named pipes-FIFOs_**), merges these results, and similarly "pushes" them to its parent process. These search results also include time stats related to the execution time of the children processes.
3. A **leaf/searcher** process receives among others arguments, the range of the search(e.g if the search range is [1000, 1500], it should start at the 1000th record and end at the 1500th record) and the pattern of the search. The results of the search in combination with time stats related to the execution time of the searcher process, will be "pushed" to the parent process via a **_named pipe (FIFO)_**. Before terminating, a searcher process sends a **SIGUSR2** signal to the root process.
4. In general, new processes in the hierarchy are created using **_fork()_** and their address space(image) is replaced with either the splitter/merger binary or the searcher binary using the **_exec*()_** system calls. The inter-process communication is achieved via *named pipes-FIFOs* and specifically with the **_mkfifo()_** system call. 




 
