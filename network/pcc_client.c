#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <stdint.h>


//--------------functions declration-----------------
void check_argument_amount(int argc);
int socket_handler();
void initialize_serv_addr(struct sockaddr_in* serv_addr_ptr, uint32_t server_port, char* server_ip);
int connection_handel(struct sockaddr_in* serv_addr);
void read_from_file(char* file_path, char** buffer, uint32_t* stream_length);
void write_stream_length(int sockfd, uint32_t stream_length);
void write_buffer(int sockfd, uint32_t stream_length, char* sent_buffer);
uint32_t read_from_server(int sockfd);

//--------end of functions declration-----------------

void check_argument_amount(int argc)
{
	if (argc < 4)
	{
		perror("not enough arguments");
		exit(1);
	}
}

int socket_handler()
{
	int sockfd = 0;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("\n Error : Could not create socket \n");
		exit(1);
	}
	return sockfd;
}

void initialize_serv_addr(struct sockaddr_in* serv_addr_ptr, uint32_t server_port, char *server_ip)
{
	memset(serv_addr_ptr, '0', sizeof(*serv_addr_ptr));
	(*serv_addr_ptr).sin_family = AF_INET;
	(*serv_addr_ptr).sin_port = htons(server_port);
	if (inet_aton(server_ip, &(*serv_addr_ptr).sin_addr) == 0)
	{
		perror("invalid address");
		exit(1);
	}
}

int connection_handel(struct sockaddr_in *serv_addr)
{
	int sockfd = 0;
	sockfd = socket_handler();
	if (connect(sockfd, (struct sockaddr*) serv_addr, sizeof(*serv_addr)) < 0)
	{
		perror("Error : Connect Failed \n");
		exit(1);
	}
	return sockfd;
}

void read_from_file(char *file_path, char **buffer, uint32_t *stream_length)
{
	FILE* fptr;
	if ((fptr = fopen(file_path, "rb")) == NULL)
	{
		perror("Error while trying opening file");
		exit(1);
	}
	fseek(fptr, 0, SEEK_END);
	*stream_length = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);
	*buffer = malloc(*stream_length + 1);
	if (!*buffer)
	{
		perror("malloc error");
		exit(1);
	}
	fread(*buffer, 1, *stream_length, fptr);
	(*buffer)[*stream_length] = 0;
	fclose(fptr);
}

void write_stream_length(int sockfd, uint32_t stream_length)
{
	uint32_t net_stream_length = htonl(stream_length);
	if (write(sockfd, &net_stream_length, sizeof(net_stream_length)) < 0)
	{
		perror("client write error \n");
		exit(1);
	}
}

void write_buffer(int sockfd, uint32_t stream_length, char* sent_buffer)
{
	uint32_t totalsent = 0; //how much we've written so far
	uint32_t notwritten = stream_length; //how much we have left to write
	uint32_t nsent = 0; //how much we've written in last write() call
	while (notwritten > 0) // keep looping until nothing left to write
	{
		nsent = write(sockfd, sent_buffer + totalsent, notwritten);
		if (nsent < 0)
		{
			perror("client write error \n");
			exit(1);
		}
		totalsent += nsent;
		notwritten -= nsent;
	}
}

uint32_t read_from_server(int sockfd)
{
	uint32_t pcc = 0;
	if (read(sockfd, &pcc, sizeof(pcc)) < 0)
	{
		perror("client read error \n");
		exit(1);
	}
	pcc = ntohl(pcc);
	return pcc;
}

int main(int argc, char* argv[])
{
	int sockfd = 0;
	struct sockaddr_in serv_addr;
	uint32_t server_port, pcc, stream_length;
	char server_ip[20], file_path[1000], *sent_buffer = 0;
	check_argument_amount(argc);
	strcpy(server_ip, argv[1]);
	server_port = atoi(argv[2]);
	strcpy(file_path, argv[3]);
	initialize_serv_addr(&serv_addr, server_port, server_ip);
	sockfd = connection_handel(&serv_addr);
	read_from_file(file_path , &sent_buffer, &stream_length);
	printf("length:  %d\n", stream_length);
	write_stream_length(sockfd, stream_length);
	write_buffer(sockfd, stream_length, sent_buffer);
	pcc = read_from_server(sockfd);
	printf("# of printable characters: %u\n", pcc);
	free(sent_buffer);
	close(sockfd);
	exit(0);
}
