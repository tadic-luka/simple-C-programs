#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>

#define BUFSIZE 4096

typedef struct {
	int sfd;
	char err;
	char *msg;
	int msg_size;
}redis_cli;
void usage()
{
	fprintf(stderr, "Usage: prog [-h serv_addr] [-p serv_port]\n");
}
redis_cli make_connection(char *serv_addr, char *serv_port)
{
	redis_cli connection = {
		.err = 0,
		.sfd = -1,
		.msg = NULL,
		.msg_size = 0
	};
	int sfd;
	struct addrinfo hints, *addr, *r;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int res;
	if((res = getaddrinfo(serv_addr, serv_port, &hints, &addr)) == -1) {
		connection.err = 1;
		connection.msg = strdup(gai_strerror(res));
		connection.msg_size = strlen(connection.msg);
		return connection;
	}
	for(r = addr; r; r = r->ai_next) {
		if((sfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1) {
			continue;
		}
		if(connect(sfd, r->ai_addr, r->ai_addrlen) != -1) {
			break;
		}
		close(sfd);
	}
	if(!r) {
		connection.err = 1;	
		connection.msg = strdup(strerror(errno));	
		connection.msg_size = strlen(connection.msg);	
	}
	connection.sfd = sfd;
	return connection;
}
void free_redis_cli(redis_cli cli)
{
	if(cli.msg != NULL) 
		free(cli.msg);
	close(cli.sfd);
}
void cli_error(redis_cli *cli, char *msg)
{
	cli->err = 1;
	cli->msg_size = strlen(msg);
	cli->msg = strdup(msg);
}
void error_message(redis_cli *cli)
{
	char buf[BUFSIZE];
	char *ptr = buf;
	ssize_t rcvd;
	ssize_t max = sizeof buf;
	while(1) {
		rcvd = recv(cli->sfd, buf, max, 0);
		switch(rcvd) {
			case -1:
				cli_error(cli, strerror(errno));
				return;
			case 0:
				cli_error(cli, "Connection closed by server.");
				return;
			default:
				buf[rcvd - 2] = 0;
				cli->err = 1;
				cli->msg = strdup(buf + 1);
				cli->msg_size = rcvd - 2;
				return;
		}
	}
}
void simple_string(redis_cli *cli)
{
	char buf[BUFSIZE];
	char *ptr = buf;
	ssize_t rcvd;
	int max = sizeof buf;
	while(1) {
		rcvd = recv(cli->sfd, buf, max, 0);
		switch(rcvd) {
			case -1:
				cli_error(cli, strerror(errno));
				return;
			case 0:
				cli_error(cli, "Connection closed by server.");
				return;
			default:
				buf[rcvd-2] = 0;
				cli->msg = strdup(buf);
				cli->msg_size = rcvd - 2;
				return;
		}
	}

}
void bulk_string(redis_cli *cli)
{
	char buf[BUFSIZE];
	char *ptr;
	ssize_t rcvd;
	ssize_t max = sizeof buf;
	while(1) {
		rcvd = recv(cli->sfd, buf, max, 0);
		switch(rcvd) {
			case -1:
				cli_error(cli, strerror(errno));
				return;
			case 0:
				cli_error(cli, "Connection closed by server.");
				return;
			default:
				buf[rcvd - 2] = 0;
				int num;
				sscanf(buf, "%d", &num);
				if(num == -1) {
					return;
				}
				ptr = memchr(buf, '\n', rcvd);
				if(ptr == NULL) {
					cli_error(cli, "Error with message");
					return;
				}
				++ptr;
				cli->msg = malloc(num + 1);
				memcpy(cli->msg, ptr, num);
				cli->msg_size = num;
				cli->msg[num] = 0;
				return;
		}
	}
}
void read_message(redis_cli *cli)
{
	ssize_t rcvd;
	char ch;
	rcvd = recv(cli->sfd, &ch, 1, 0);
	switch(ch) {
		case '+':
			simple_string(cli);	
			break;
		case '-':
			error_message(cli);	
			break;
		case '$':
			bulk_string(cli);
			break;
		case ':':
			break;

	}
}
void send_message(redis_cli *cli, char *msg)
{
	send(cli->sfd, msg, strlen(msg), 0);
	send(cli->sfd, "\r\n", 2, 0);

	free(cli->msg);
	cli->msg = NULL;
	cli->msg_size = -1;

	read_message(cli);
}
int main(int argc, char *argv[])
{
	char *serv_port = "6379";
	char *serv_addr = "127.0.0.1";
	char bufin[4096];
	char ch;
	while((ch = getopt(argc, argv, "h:p:")) != -1) {
		switch(ch) {
			case 'h':
				serv_addr = optarg;
				break;
			case 'p':
				serv_port = optarg;
				break;
			default:
				usage();
				exit(1);
		}
	}
	if(optind != argc) {
		usage();
		return 1;
	}
	redis_cli cli;
	cli = make_connection(serv_addr, serv_port);
	if(cli.err) {
		fprintf(stderr, "%s\n", cli.msg);
		return 1;
	}
	while(1) {
		printf("%s:%s>", serv_addr, serv_port);
		fflush(stdout);
		ssize_t rcvd = read(0, bufin, sizeof bufin);
		if(rcvd == -1) {
			perror("stdin");
			continue;
		} else if(rcvd == 0) {
			break;
		} else if(rcvd == 1) 
			continue;
		bufin[rcvd-1] = 0;
		if(strcmp(bufin, "quit") == 0)
			break;
		send_message(&cli, bufin);				
		if(cli.err) {
			fprintf(stderr, "Error: %s\n", cli.msg);
		} else if(cli.msg_size > 0) {
			printf("%s\n", cli.msg);
		} else {
			printf("(nil)\n");
		}
	}
	free_redis_cli(cli);
	return 0;
}



