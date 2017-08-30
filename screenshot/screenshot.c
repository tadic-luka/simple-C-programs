#include "screenshot.h"

static XImage *
blur_image(XImage *image, int radius)
{
	XImage *blurred = malloc(sizeof(XImage));
	CHECK_ERROR(blurred, "Unable to allocate memory for new image!\n");
	memcpy(blurred, image, sizeof(XImage));
	unsigned long bytes_to_cpy = sizeof(char) * image->bytes_per_line * image->height;
	blurred->data = malloc(bytes_to_cpy);
	CHECK_ERROR(blurred->data, "Unable to allocate memory for image data\n");
	memcpy(blurred->data, image->data, bytes_to_cpy);
	stackblur(blurred, 0, 0, image->width, image->height, radius, CPU_THREADS);
	return blurred;
}

static int write_jpeg(XImage *img, const char* filename)
{
	FILE* outfile;
	unsigned long pixel;
	int x, y;
	char* buffer;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	JSAMPROW row_pointer;

	outfile = fopen(filename, "wb");
	if (!outfile) {
		fprintf(stderr, "Failed to open output file %s", filename);
		return 1;
	}

	/* collect separate RGB values to a buffer */
	buffer = malloc(sizeof(char)*3*img->width*img->height);
	CHECK_ERROR(buffer, "no memory");
	for (y = 0; y < img->height; y++) {
		for (x = 0; x < img->width; x++) {
			pixel = XGetPixel(img,x,y);
			buffer[y*img->width*3+x*3+0] = (char)(pixel>>16);
			buffer[y*img->width*3+x*3+1] = (char)((pixel&0x00ff00)>>8);
			buffer[y*img->width*3+x*3+2] = (char)(pixel&0x0000ff);
		}
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = img->width;
	cinfo.image_height = img->height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	cinfo.optimize_coding = TRUE;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 100, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline
			*(img->depth>>3)*img->width];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	free(buffer);
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(outfile);

	return 0;
}

static void capture_rectangle_image(Display *display,XRectangle rect, const char *filename)
{
	XImage *image;
	image = XGetImage(display, RootWindow(display, DefaultScreen(display)), rect.x, rect.y, rect.width, rect.height,
			AllPlanes, ZPixmap);	
	write_jpeg(image, filename);
	XFree(image);
	image = NULL;
}

//rectangle is defined with upper left point and width, height
static XRectangle get_rectangle_from_points(int xpoint1, int ypoint1, int xpoint2, int ypoint2)
{
	XRectangle rect;
	rect.x = (xpoint1 < xpoint2 ? xpoint1 : xpoint2);
	rect.y = (ypoint1 < ypoint2 ? ypoint1 : ypoint2);
	rect.width = abs(xpoint1 - xpoint2);
	rect.height = abs(ypoint1 - ypoint2);
	return rect;
}
static void
draw_rect_on_window(Display *dpy, Window win, XImage *image, GC gc, XRectangle rec)
{
	XImage *sub = XSubImage(image, rec.x, rec.y, rec.width, rec.height);
	XPutImage(dpy, win, gc, sub, 0, 0, rec.x, rec.y, rec.width, rec.height);
	XDrawRectangle(dpy, win, gc, rec.x, rec.y, rec.width, rec.height);
}

static void loop(Display *display, GC gc, const char *filename, Window win)
{
	XEvent event;
	XRectangle rect;
	int pressed = 0, x, y;
	Window root = RootWindow(display, DefaultScreen(display));
	XWindowAttributes gwa;

    XGetWindowAttributes(display, root, &gwa);
	XImage *original = XGetImage(display, root, 0, 0, gwa.width, gwa.height, AllPlanes, ZPixmap);
	XImage *img = blur_image(original, BLUR_RADIUS);
	XPutImage(display, win, gc, img, 0, 0, 0, 0, gwa.width, gwa.height);
	XMapWindow(display, win);
	XRaiseWindow(display, win);

	do	
	{
		XNextEvent(display, &event);
		switch(event.type) {
			case ButtonPress:
				pressed = 1;
				x = event.xbutton.x;
				y = event.xbutton.y;
				break;
			case ButtonRelease:
				pressed = 0;
				rect = get_rectangle_from_points(x, y, event.xmotion.x, event.xmotion.y);
				draw_rect_on_window(display, win, img, gc, rect);
				/*XPutImage(display, win, gc, img, 0, 0, 0, 0, gwa.width, gwa.height);*/
				if(rect.width && rect.height)
					write_jpeg(XSubImage(original, rect.x, rect.y, rect.width, rect.height), filename);
				break;
			case MotionNotify:
				if(pressed) {
					XRectangle new_rect = get_rectangle_from_points(x, y, event.xmotion.x, event.xmotion.y); 
					if(new_rect.width < rect.width || new_rect.height < rect.height) {
						draw_rect_on_window(display, win, img, gc, rect);
						XPutImage(display, win, gc, img, 0, 0, 0, 0, gwa.width, gwa.height);
					}
					rect = new_rect;
					/*rect = get_rectangle_from_points(x, y, event.xmotion.x, event.xmotion.y);*/
					draw_rect_on_window(display, win, original, gc, rect);
				}
				break;
			case Expose:
				XPutImage(event.xexpose.display, event.xexpose.window, gc, img, 0, 0, 0, 0, gwa.width, gwa.height);
				break;
		}
	}while(event.type != KeyPress || event.xkey.keycode != 9);
	XDestroyImage(img);
	img = XGetImage(display, root, 0, 0, gwa.width, gwa.height, AllPlanes, ZPixmap);
}
int scrshot(char whole_scr, const char *filename)
{

	Display* display = XOpenDisplay(NULL); //display
	CHECK_ERROR(display, "Could not establish a connection to X-server");

	if(whole_scr) {
		XRectangle rect = {0, 0, XDisplayWidth(display, XDefaultScreen(display)), XDisplayHeight(display, XDefaultScreen(display))};
		capture_rectangle_image(display, rect,  filename);
		XCloseDisplay(display);
		return 0;
	}

	// query Visual for "TrueColor" and 24 bits depth (RGBA)
	XVisualInfo visualinfo; 
	if(!XMatchVisualInfo(display, DefaultScreen(display), 24, TrueColor, &visualinfo)) {
		XCloseDisplay(display);
		fprintf(stderr, "No found color display\n");
		return 1;
	}

	// create window
	Window  win;
	Window root;
	GC gc; // graphic context
	XSetWindowAttributes attr;

	attr.colormap = XCreateColormap(display, DefaultRootWindow(display), visualinfo.visual, AllocNone);
	attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask; 
	attr.background_pixmap = None;
	attr.backing_pixel = 0;
	/*attr.background_pixel = 0;*/
	/*attr.override_redirect = True;*/
	attr.border_pixel = 0;

	int screen = DefaultScreen(display);
	root = RootWindow(display, screen);
	XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);
	win = XCreateWindow(display, RootWindow(display, DefaultScreen(display)),
			1, 1, gwa.width, gwa.height, // x,y,width,height
			0, // border width
			visualinfo.depth, // window depth
			InputOutput, //window class, inputoutput, inputonly, or copyfromparent
			visualinfo.visual,
			CWColormap|CWEventMask|CWBackPixmap|CWBorderPixel | CWBackPixel,
			&attr
			);
	// trying out simple window which has black background
	gc = XCreateGC( display, win, 0, 0);
	/*XDrawRectangle(dpy, win, gc, rec.x, rec.y, rec.width, rec.height);*/
	XSetLineAttributes(display, gc,
                     1, LineDoubleDash, CapButt, JoinRound);


	// set title bar name of window
	XStoreName(display, win, "IRG_LAB2");

	/*XSetFillRule(display, gc, WindingRule);*/

	// now let the window appear to the user
	
	loop(display, gc, filename, win);
	XFreeGC(display, gc);
	XDestroyWindow(display, win); 
	XCloseDisplay(display);
	return 0;
}

