# Parallel File Find Assignment  


## Introduction
The goal of this assignment is to gain experience
with threads and ﬁlesystem system calls.
In this assignment, I created a program that searches a directory tree
for ﬁles whose name matches some search term.
The program receives a directory D and a search term T,
and ﬁnds every ﬁle in D’s directory tree whose name contains T.
The program parallelizes its work using threads.

##Command line arguments:
argv[1]: search root directory
(search for ﬁles within this directory and its subdirectories).

argv[2]: search term
 (search for ﬁle names that include the search term). 

argv[3]: number of searching threads to be used for the search.


 

