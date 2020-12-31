#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <stdint.h>
#include <ctype.h>
#include <signal.h>

static int PRINTABLE_CHARACTERS_NUM = 93, FIRST_PRINTABLE_CHARACTER = 32;
static uint32_t pcc_total[93];


//--------------functions declration-----------------

void my_signal_handler(int signum);
void register_signal_handling();
void initialize_pcc_total();
void check_argument_amount(int argc);
void handel_SO_REUSEADDR(int listenfd);
int socket_handel();
void initialize_serv_addr(struct sockaddr_in* serv_addr_ptr, uint32_t server_port);
void bind_handel(struct sockaddr_in* serv_addr_ptr, int listenfd);
void listen_handel(int listenfd);
int accept_handler(int listenfd);
void read_stream_length(uint32_t* stream_length_ptr, int connfd, int* read_write_error);
char* create_buffer(uint32_t length);
void read_to_buffer(char* recv_buffer, uint32_t stream_length, int connfd, int* read_write_error);
uint32_t count_pcc(char* buffer, uint32_t length);
void write_pcc(uint32_t pcc, int connfd, int* read_write_error);
void update_pcc_total(char* characters_buffer, uint32_t length, int* read_write_error);
void pcc_hadler(int connfd);
void connections_handel(uint32_t server_port);
//--------end of functions declration-----------------


void my_signal_handler(int signum)
{
	int j;
	for (j = 0; j < PRINTABLE_CHARACTERS_NUM; j++)
	{
		printf("char ’%c’  : %u times\n", j + FIRST_PRINTABLE_CHARACTER, pcc_total[j]);
	}
	exit(0);
}

void register_signal_handling()
{
	struct sigaction new_action;
	memset(&new_action, 0, sizeof(new_action));
	new_action.sa_handler = my_signal_handler;
	// Overwrite default behavior for ctrl+c
	if (sigaction(SIGINT, &new_action, NULL) == -1)
	{
		perror("Signal handle registration failed");
		exit(1);
	}
}

void initialize_pcc_total()
{
	memset(pcc_total, 0, sizeof(pcc_total));
}

void check_argument_amount(int argc)
{
	if (argc < 2)
	{
		perror("not enough arguments");
		exit(1);
	}
}

void handel_SO_REUSEADDR(int listenfd)
{
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR) failed");
	}
}

int socket_handel()
{
	int listenfd = 0;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0)
	{
		perror("error opening socket");
		exit(1);
	}
	handel_SO_REUSEADDR(listenfd);
	return listenfd;
}

void initialize_serv_addr(struct sockaddr_in *serv_addr_ptr, uint32_t server_port)
{
	memset(serv_addr_ptr, '0', sizeof(*serv_addr_ptr));
	(*serv_addr_ptr).sin_family = AF_INET;
	(*serv_addr_ptr).sin_addr.s_addr = htonl(INADDR_ANY);
	(*serv_addr_ptr).sin_port = htons(server_port);

}

void bind_handel(struct sockaddr_in *serv_addr_ptr, int listenfd)
{
	if (bind(listenfd, (struct sockaddr*) serv_addr_ptr, sizeof(*serv_addr_ptr)))
	{
		perror("Error : Bind Failed\n");
		exit(1);
	}
}

void listen_handel(int listenfd)
{
	if (0 != listen(listenfd, 10))
	{
		perror("Error : Listen Failed\n");
		exit(1);
	}
}

int accept_handler(int listenfd)
{
	int connfd = 0;
	connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
	if (connfd < 0)
	{
		perror("Error : Accept Failed\n");
		exit(1);
	}
	return connfd;
}

void read_stream_length(uint32_t *stream_length_ptr, int connfd, int *read_write_error)
{
	if (read(connfd, stream_length_ptr, sizeof(*stream_length_ptr)) < 0)
	{
		perror("server read error \n");
		*read_write_error = 1;
	}
	*stream_length_ptr = ntohl(*stream_length_ptr);
}

char* create_buffer(uint32_t length)
{
	char* buffer = malloc(length);
	if (!buffer)
	{
		perror("malloc error");
		exit(1);
	}
	return buffer;
}

void read_to_buffer(char *recv_buffer, uint32_t stream_length, int connfd, int *read_write_error)
{
	uint32_t totalread = 0;   //how much we've readen so far
	uint32_t notreaden = stream_length;   //how much we have left to read
	uint32_t nread = 0;   //how much we've readen in last write() call
	while (notreaden > 0) // keep looping until nothing left to read
	{
		nread = read(connfd, recv_buffer + totalread, notreaden);
		if (nread < 0)
		{
			perror("server read error \n");
			*read_write_error = 1;
		}
		totalread += nread;
		notreaden -= nread;
	}
	recv_buffer[stream_length] = 0;
}

uint32_t count_pcc(char* buffer, uint32_t length)
{
	int i;
	uint32_t curr_pcc = 0;
	for (i = 0; i < length; i++)
	{
		if (isprint((int)(buffer[i])))
		{
			curr_pcc++;
		}
	}
	return curr_pcc;
}

void write_pcc(uint32_t pcc, int connfd, int *read_write_error)
{
	pcc = htonl(pcc);
	if (write(connfd, &pcc, sizeof(pcc)) < 0)
	{
		perror("server write error \n");
		*read_write_error = 1;
	}
}

void update_pcc_total(char *characters_buffer, uint32_t length, int *read_write_error)
{
	if (!*read_write_error)
	{
		int i;
		for (i = 0; i < length; i++)
		{
			if (isprint((int)(characters_buffer[i])))
			{
				pcc_total[(int)(characters_buffer[i]) - FIRST_PRINTABLE_CHARACTER] ++;
			}
		}
	}
}
void pcc_hadler(int connfd)
{
	uint32_t stream_length, pcc_num;
	char* characters_buffer;
	int read_write_error = 0;
	read_stream_length(&stream_length, connfd,  &read_write_error);
	characters_buffer = create_buffer(stream_length + 1);
	read_to_buffer(characters_buffer, stream_length, connfd, &read_write_error);
	pcc_num = count_pcc(characters_buffer, stream_length);
	write_pcc(pcc_num, connfd , &read_write_error);
	update_pcc_total(characters_buffer, stream_length, &read_write_error);
	free(characters_buffer);
	close(connfd);
}

void connections_handel(uint32_t server_port)
{
	struct sockaddr_in serv_addr;
	int listenfd = 0, connfd = 0;
	initialize_serv_addr(&serv_addr, server_port);
	listenfd = socket_handel();
	bind_handel(&serv_addr, listenfd);
	listen_handel(listenfd);
	while (1)
	{
		connfd = accept_handler(listenfd);
		pcc_hadler(connfd);
	}
}

int main(int argc, char* argv[])
{
	register_signal_handling();
	uint32_t server_port;
	check_argument_amount(argc);
	server_port = atoi(argv[1]);
	initialize_pcc_total();
	connections_handel(server_port);
}
