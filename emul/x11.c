/* X11 implementation of the graphics API.
   Copyright (C) 2021 bellrise */

#include "emul_graphics.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <malloc.h>


/* Global last error instance. */
static int glob_err = 0;

/* X11 definition of the eg_window struct. */
struct eg_window
{
    int     width;
    int     height;

    Display *dis;
    int     scr;
    Window  win;
    GC      gcontext;
};

/* Error names */
const char *glob_strerrs[] = {
    "Everything is fine",
    "General failure",
    "Invalid parameter",
    "OS failure",
    "Window server failure"
};

const int n_glob_strerrs = sizeof(glob_strerrs) / sizeof(char *);


struct eg_window *eg_create_window(int width, int height)
{
    struct eg_window *win;
    unsigned long white, black;

    /* Create and intialize the X11 window. */

    win = malloc(sizeof(struct eg_window));
    if (!(win->dis = XOpenDisplay(NULL))) {
        glob_err = EGE_WINSRV;
        return NULL;
    }

    win->scr = DefaultScreen(win->dis);

    white = WhitePixel(win->dis, win->scr);
    black = BlackPixel(win->dis, win->scr);

    win->win = XCreateSimpleWindow(win->dis, DefaultRootWindow(win->dis), 0, 0,
            width, height, 5, white, black);
    XSelectInput(win->dis, win->win, ExposureMask | KeyPressMask);

    /* Create a graphics context. */

    win->gcontext = XCreateGC(win->dis, win->win, 0, NULL);

    XSetBackground(win->dis, win->gcontext, black);
    XSetForeground(win->dis, win->gcontext, white);

    XClearWindow(win->dis, win->win);
    XMapRaised(win->dis, win->win);

    XSync(win->dis, False);

    return win;
}

int eg_close_window(struct eg_window *win)
{
    if (!win)
        return EGE_PARAM;

    XFreeGC(win->dis, win->gcontext);
    XDestroyWindow(win->dis, win->win);
    XCloseDisplay(win->dis);

    free(win);
    return EGE_OK;
}

int eg_draw_text(struct eg_window *win, char *str, unsigned clr, int x,
        int y)
{
    XDrawString(win->dis, win->win, win->gcontext, x, y, str, strlen(str));

    return EGE_OK;
}

int eg_draw_pixel(struct eg_window *win, unsigned clr, int x, int y)
{
    return EGE_OK;
}

int eg_get_last_error()
{
    return glob_err;
}

const char *eg_strerror(int err)
{
    if (err < 0)
        err = -err;

    if (err >= n_glob_strerrs)
        return NULL;

    return glob_strerrs[err];
}

int eg_get_height(struct eg_window *win)
{
    return win->height;
}

int eg_get_width(struct eg_window *win)
{
    return win->width;
}
