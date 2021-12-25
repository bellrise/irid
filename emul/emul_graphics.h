/* Irid emulator graphics API.
   Copyright (C) 2021 bellrise */

#ifndef EMUL_GRAPHICS_H
#define EMUL_GRAPHICS_H

/*
 * This is only the graphics API, which has its custom implementations for each
 * system. For example, the Linux version uses X11 and Windows uses its GDI
 * library for drawing pixels. Note that none of these functions are designed
 * to be thread-safe, so the whole rendering process should be done on a single
 * thread.
 */

/* Error codes */
#define EGE_OK          0       /* Everything is fine */
#define EGE_FAIL        1       /* General failure */
#define EGE_PARAM       2       /* Invalid parameter */
#define EGE_OS          3       /* OS failure */
#define EGE_WINSRV      4       /* Window server failure */

/* Defined in the implementation. */
struct eg_window;

/*
 * Create a window and return a pointer to a handle. May return NULL if the
 * window creation failed. If that happens, try using eg_get_last_error to
 * understand what went wrong.
 *
 * @param   height      height of the window
 */
struct eg_window *eg_create_window(int width, int height);

/*
 * Close the window & free any allocated resources. Note that any access to
 * the window pointer will result in a segfault or heap violation. Returns
 * a error code.
 *
 * @param   win         window to close
 */
int eg_close_window(struct eg_window *win);

int eg_draw_text(struct eg_window *win, char *str, unsigned clr, int x, int y);
int eg_draw_pixel(struct eg_window *win, unsigned clr, int x, int y);

/*
 * Returns the last error that the program generated. If no error is found,
 * it returns a 0 (EGE_OK). Remember that this function is NOT thread safe.
 */
int eg_get_last_error();

/*
 * Return the string representation of the error, for debugging or nicer error
 * messages. Flips negative values by default.
 *
 * @param   err         err code (one of EGE_*)
 */
const char *eg_strerror(int err);

/*
 * Return the height or the width of the window. The result may be a negative
 * value, being the negated EGE_* error code.
 *
 * @param   win         pointer to the window struct
 */
int eg_get_height(struct eg_window *win);
int eg_get_width(struct eg_window *win);


#endif /* EMUL_GRAPHICS_H */
