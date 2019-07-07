
This is the 2nd project of the operatings system course. In this project, an initial(root) process creates new children processes using the **_fork()_** system call. The children processes produce an hierarchy of processes similar to a **binary tree**. 

The purpose of this project was to execute queries on binary files of fixed length records and to sort the results afterwards. Leaf nodes(**searchers processes**) are given the task of searching in specific parts of the binary file, whereas internal nodes(**splitter/merger processes**) merge the results retrieved from their children and "push" these results to their parents. After the search is concluded, the *sort()* system program performs, as its name indicates, the final sorting of the search results.

![image](/home/lazaros/Pictures/Screenshot from 2019-07-07 19-08-13.png)
