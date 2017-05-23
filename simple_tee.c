#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <err.h>

void tee(int fd)
{
	char *buf = NULL;
	struct stat fd_stat;
	if(fstat(fd, &fd_stat) == -1) {
		err(1, "fstat");
	}
	if((buf = malloc(fd_stat.st_blksize)) == NULL) {
		err(1, "malloc");
	}
	ssize_t ret;
	while((ret = read(0, buf, fd_stat.st_blksize))) {
		if(ret == -1) {
			err(1, "read");
		}
		write(2, buf, ret);
		write(fd, buf, ret);
	}
	free(buf);
}

int main(int argc, char *argv[])
{
	++argv;
	if(argc < 2) {
		fprintf(stderr, "Usage: simple_tee [-a] file");
	}
	while(*argv) {
		int fd = open(*argv, O_CREAT | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH);
		if(fd == -1) {
			err(1, "open %s", *argv);
		}
		++argv;
		tee(fd);
		close(fd);
	}
	return 0;
}

