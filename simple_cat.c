#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>

void read_fd(int fd)
{
	static char *buf = NULL;
	struct stat fd_stat;
	if(fstat(fd, &fd_stat) == -1) {
		err(1, "fstat");
	}
	if(!buf && (buf = malloc(sizeof(fd_stat.st_blksize))) == NULL) {
		err(1, "malloc");	
	}
	int ret;
	printf("%d\n", fd_stat.st_blksize);
	while((ret = read(fd, buf, fd_stat.st_blksize))) {
		if(ret == -1) {
			err(1, "read");
		}
		fwrite(buf, 1, ret, stdout);
	}

}
void new_handler(int sig)
{
	puts("primio signal");
}
int main(int argc, char *argv[])
{
	argv++;
	signal(SIGINT, new_handler);	
	
	struct stat fd_stat;
	while(*argv) {
		printf("%s\n", *argv);
		if(!memcmp(*argv, "-", 1)) {
			read_fd(0);
		} else {
			int fd = open(*argv, 0);
			if(fd == -1) {
				err(1, "open %s\n", *argv);
			}
			read_fd(fd);
		}
		argv++;
	}
	return 0;
}
