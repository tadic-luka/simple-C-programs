#include <stdio.h>
#include <string.h>
#include <err.h>
#include <stdlib.h>
#include <hiredis/hiredis.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#define BUFSIZE 1024
#define SET_ELEMENT(c, ex, fmt, ...) \
	do{ \
		redisReply *p = redisCommand(c, fmt, __VA_ARGS__); \
		if(!p) { \
			fprintf(stderr, "No memory for redisReply\n");\
		if(ex) {\
			exit(ex);	\
		}\
		} else if(p->type == REDIS_REPLY_ERROR) {\
			if(ex) {\
				err(1, "set_element: %s", p->str);\
			} else {\
				perror("");\
					fprintf(stderr, "set_element: %s\n", p->str);\
			}\
		}\
		freeReplyObject(p);\
	} while(0)

void rtrim(char *str)
{
	char *end = str + strlen(str) - 1;
	while((str != end) && (*end == ' ')) {
		*end = 0;
		--end;
	}
}

void ltrim(char *str)
{
	int i = 0;
	char *start = str;
	while(*str == ' ') {
		++i;
		++str;
	}
	while(*start) {
		*start = start[i];
		++start;
	}

}
			
void trim(char *str)
{
	ltrim(str);
	rtrim(str);
}
redisContext* connect_redis(const char *hostname,int port)
{
	redisContext *c = redisConnect(hostname, port);
	if(c == NULL || c->err) {
		if(c) {
			fprintf(stderr, "Connection error: %s\n", c->errstr);
			redisFree(c);
			exit(1);
		} else {
			fprintf(stderr, "Connection error: can't allocate\n");
			exit(1);
		}
	}
	return c;
}
void list_dir(const char *name, int hidden, int recursive)
{
	DIR *dir = opendir(name);
	if(dir == NULL) {
		perror("opendir");
		return;
	}
	struct dirent *d;
	int i = 0;
	while((d = readdir(dir))) {
		if(d->d_name[0] == '.' && !hidden) {
			continue;
		}
		if(recursive && d->d_type == DT_DIR) {
			chdir(name);
			list_dir(d->d_name, hidden, recursive);
			chdir("../");
		}
		printf("%s\n", d->d_name);
	}
}


void parse_command(redisContext *c, char *command, char *args)
{
	SET_ELEMENT(c, 0, "INCR %s", command);
	if(memcmp(command, "ls", 2) == 0) {
		char *arg = strsep(&args, " ");
		if(!arg) {
			exec
			list_dir(".", 0, 0);
		} else {
			do {
				list_dir(arg, 0, 0);	
			}while((arg = strsep(&args, " ")));
		}
	}
}
int main(int argc, char *argv[])
{
	redisContext *c;
	redisReply *reply;
	const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
	int port = (argc > 2) ? atoi(argv[2]) : 6379;
	char a[100] = "  luka je kralj  ";
	c = connect_redis(hostname, port);

	char input[BUFSIZE];
	do {
		int ret = read(0, input, BUFSIZE);
		if(ret == -1) err(1, "read");
		input[ret-1] = 0;
		trim(input);
		char *in = input;
		char *command = strsep(&in, " ");
		if(command == NULL) {
			fprintf(stderr, "Invalid input\n");
			break;
		}
		if(!memcmp(command, "quit", 4)) {
			write(1, "Thank you for using this shell\n", 34);
			break;
		}
		parse_command(c, command, in);
	}while(1);



	/*reply = redisCommand(c,"GET html");*/
	/*printf("GET html: %s\n", reply->str);*/
	/*freeReplyObject(reply);*/

	/*  Disconnects and frees the context */
	redisFree(c);
	return 0;
}



