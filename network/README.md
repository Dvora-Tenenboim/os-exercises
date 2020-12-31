# Network Assignment

## Introduction
The goal of this assignment is to gain experience with sockets and network programming.
In this assignment, I implemented a toy client/server architecture:
a printable characters counting (PCC) server.
Clients connect to the server and send it a stream of bytes.
The server counts how many of the bytes are printable and returns that number to the client.
The server also maintains overall statistics on the distribution of printable characters it has received from all clients.
When the server terminates, it prints these statistics to standard output.

## Client speciﬁcation
Implemented in a ﬁle named pcc_client.c.

##### Command line arguments:
• argv[1]: server’s IP address (assume a valid IP address).
• argv[2]: server’s port (assume a 16-bit unsigned integer).
• argv[3]: path of the ﬁle to send.

## Server speciﬁcation
Implemented in a ﬁle named pcc_server.c.

##### Command line arguments:
• argv[1]: server’s port (assume a 16-bit unsigned integer).


