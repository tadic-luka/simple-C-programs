#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	int flags = O_CREAT | O_WRONLY;
	if(argc < 2) {
		fprintf(stderr, "Usage: simple_tee [-a] file");
	}
	while(*argv) {
		if(memcmp(*argv, "-a", 2) == 0) {
			/*flags &= ~O_CREAT;*/
			flags |= O_APPEND;
			++argv;
			continue;
		}
		int fd = open(*argv, flags, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
		if(fd == -1) {
			err(1, "open %s", *argv);
		}
		++argv;
		tee(fd);
		close(fd);
	}
	return 0;
}

