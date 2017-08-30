PROJECT = prog
HEADERS = rate_xch.h
SOURCE = rate_xch.c

CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
LDFLAGS =
LDLIBS = -lcurl
OBJECTS = ${SOURCE:.c=.o}

$(PROJECT): $(OBJECTS)
		$(CC) $(OBJECTS) -o $(PROJECT) $(LDLIBS)

$(OBJECTS): $(HEADERS)

release:
		$(CC) $(CFLAGS) -O2 $(SOURCE) -o $(PROJECT) $(LDLIBS)
clean:
	-rm -f *~ *.o *.core $(PROJECT)
