#include <stdio.h>
#include<string.h>
#include<stdlib.h>
#include <X11/Xutil.h>
#include <math.h>
#include <unistd.h>
#include <jpeglib.h>
#include "stackblur.h"

#define BLUR_RADIUS 4
#define CPU_THREADS 4


#define CHECK_ERROR(ptr, err_msg) if(!ptr) { \
	fprintf(stderr, "%s\n", err_msg); \
	exit(1); \
}

static int write_jpeg(XImage *img, const char* filename);

static XImage* blur_image(XImage *image, int radius);

static void capture_rectangle_image(Display *display,XRectangle rect, const char *filename);

static XRectangle get_rectangle_from_points(int xpoint1, int ypoint1, int xpoint2, int ypoint2);

static void draw_rect_on_window(Display *dpy, Window win, XImage *image, GC gc, XRectangle rec);

static void loop(Display *display, GC gc, const char *filename, Window win);

int scrshot(char whole_scr, const char *filename);
