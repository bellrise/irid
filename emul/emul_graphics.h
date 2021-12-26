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

/* Defined in the implementation. */
struct eg_window;

/*
 * Create a window and return a pointer to a handle. May return NULL if the
 * window creation failed. If that happens, errno is set.
 *
 * @param   height      height of the window
 */
struct eg_window *eg_create_window(int width, int height);

/*
 * Close the window & free any allocated resources. Note that any access to
 * the window pointer will result in a segfault or heap violation. Returns
 * an error code.
 *
 * @param   win         window to close
 */
int eg_close_window(struct eg_window *win);

/*
 * Draw some text at the given row & column. Note that the first coordinate is
 * the row (the line) of the string. Newlines do not move to the next line.
 *
 * @param   str         string to print
 * @param   row         row to print (line num, starts from 0)
 * @param   col         column to print (offset, starts from 0)
 */
int eg_draw_text(struct eg_window *win, char *str, int row, int col);

/*
 * Return the height or the width of the window. The result may be a negative
 * value, being the negated errno code.
 *
 * @param   win         pointer to the window struct
 */
int eg_get_height(struct eg_window *win);
int eg_get_width(struct eg_window *win);


#endif /* EMUL_GRAPHICS_H */
