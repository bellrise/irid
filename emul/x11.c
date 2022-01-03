/* X11 implementation of the graphics API.
   Copyright (C) 2021-2022 bellrise */

#include "emul_graphics.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>


/* X11 definition of the eg_window struct. */
struct eg_window
{
    int         width;
    int         height;
    Display     *dis;
    int         scr;
    Window      win;
    GC          gcontext;
    XFontStruct *font;
    int         glyph_w;
    int         glyph_h;
};


struct eg_window *eg_create_window(int width, int height)
{
    struct eg_window *win;
    unsigned long white, black;

    /* Create and intialize the X11 window. */

    win = calloc(sizeof(struct eg_window), 1);
    if (!(win->dis = XOpenDisplay(NULL))) {
        errno = EPROTO;
        return NULL;
    }

    win->scr = DefaultScreen(win->dis);

    white = WhitePixel(win->dis, win->scr);
    black = BlackPixel(win->dis, win->scr);

    win->win = XCreateSimpleWindow(win->dis, DefaultRootWindow(win->dis), 0, 0,
            width, height, 5, white, black);
    XSelectInput(win->dis, win->win, StructureNotifyMask | ExposureMask
            | KeyPressMask);

    /* Create a graphics context. */

    win->gcontext = XCreateGC(win->dis, win->win, 0, NULL);

    XSetBackground(win->dis, win->gcontext, black);
    XSetForeground(win->dis, win->gcontext, white);

    /* Load a font. */

    win->font = XLoadQueryFont(win->dis, "*-fixed-*-*-*-18-*");
    if (!win->font) {
        errno = ENOENT;
        return NULL;
    }

    XSetFont(win->dis, win->gcontext, win->font->fid);
    win->glyph_w = win->font->max_bounds.rbearing
        - win->font->min_bounds.lbearing;
    win->glyph_h = win->font->max_bounds.ascent
        + win->font->max_bounds.descent;

    XClearWindow(win->dis, win->win);
    XMapRaised(win->dis, win->win);

    XSync(win->dis, False);

    /* Wait for the map notify event, so we can actually draw stuff. */
    while (1) {
        XEvent ev;
        XNextEvent(win->dis, &ev);

        if (ev.type == MapNotify)
            break;
    }

    return win;
}

int eg_close_window(struct eg_window *win)
{
    if (!win)
        return EINVAL;

    XFreeGC(win->dis, win->gcontext);
    XFreeFont(win->dis, win->font);
    XDestroyWindow(win->dis, win->win);
    XCloseDisplay(win->dis);

    free(win);
    return 0;
}

int eg_draw_text(struct eg_window *win, char *str, int row, int col)
{
    int x, y;

    x = win->glyph_w * col;
    y = win->glyph_h * (row + 1);

    XDrawString(win->dis, win->win, win->gcontext, x, y, str, strlen(str));
    return 0;
}

int eg_get_height(struct eg_window *win)
{
    return win->height;
}

int eg_get_width(struct eg_window *win)
{
    return win->width;
}
