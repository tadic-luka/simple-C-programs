PROJECT=test_prog
OBJECTS=main.o screenshot.o stackblur.o
SOURCES=main.c ../screenshot/screenshot.c ../screenshot/stackblur.c

CC=gcc
CFLAGS = -Wall -Wextra
LIBS= -levent -lasound -lGL -lX11 -lXfixes -ljpeg 
LIBNOTIFY_LIBS = -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng16 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -lnotify -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -pthread


$(PROJECT): $(OBJECTS)
	$(CC)  -o $(PROJECT) $(CFLAGS) $(LIBS) $(LIBNOTIFY_LIBS) $(OBJECTS)

main.o: main.c
	$(CC) $(LIBNOTIFY_LIBS) -c main.c

screenshot.o: ../screenshot/screenshot.c ../screenshot/screenshot.h
	$(CC) -c ../screenshot/screenshot.c

stackblur.o: ../screenshot/stackblur.c ../screenshot/stackblur.h
	$(CC) -c ../screenshot/stackblur.c

clean:
	-rm -f $(PROJECT) *.o *.core

release:
	$(CC) $(LIBS) $(LIBNOTIFY_LIBS) main.c ../screenshot/screenshot.c ../screenshot/stackblur.c -o keylogger
