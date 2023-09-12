/* Console device implementation
   Copyright (c) 2023 bellrise */

#include "emul.h"

#include <ctype.h>
#include <poll.h>
#include <queue>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct console_state
{
    int in;
    int out;
    bool control_mode;
    std::queue<u8> readbuffer;
};

static u8 console_read(device&);
static void console_write(device&, u8);
static bool console_poll(device&);
static void console_close(device&);

static inline console_state *state(device& self)
{
    return static_cast<console_state *>(self.state);
}

device console_create(int in, int out)
{
    device console = {0x1000, "console"};

    console.state = new console_state;

    state(console)->in = in;
    state(console)->out = out;

    console.read = console_read;
    console.write = console_write;
    console.poll = console_poll;
    console.close = console_close;

    return console;
}

static u8 console_read(device& self)
{
    u8 c;

    /* First empty our own readbuffer. */
    if (state(self)->readbuffer.size()) {
        c = state(self)->readbuffer.front();
        state(self)->readbuffer.pop();
        return c;
    }

    /* Make this non-blocking by returning a NUL if stdin is empty. */
    if (!console_poll(self))
        return 0;

    if (read(state(self)->in, &c, 1) <= 0)
        return 0;
    return c;
}

static void queue_write_word(std::queue<u8>& buf, u16 word)
{
    buf.push(word & 0xff);
    buf.push(word >> 8);
}

static void cctl_size(device& self)
{
    u16 w = 0;
    u16 h = 0;

    if (isatty(state(self)->out)) {
        struct winsize siz;
        ioctl(state(self)->out, TIOCGWINSZ, &siz);

        w = siz.ws_col;
        h = siz.ws_row;
    }

    queue_write_word(state(self)->readbuffer, w);
    queue_write_word(state(self)->readbuffer, h);
}

static void control(device& self, u8 code)
{
    if (code == CCTL_SIZE)
        cctl_size(self);
}

static void console_write(device& self, u8 byte)
{
    /* If we are in control mode, fetch the next byte. */

    if (state(self)->control_mode) {
        control(self, byte);
        state(self)->control_mode = false;
        return;
    }

    /* Writing to the console will only let printable characters through
       to the console, while "firewalling" escape & control codes. */

    if (isprint(byte) || byte == ' ' || byte == '\n' || byte == '\r'
        || byte == 0x7F) {
        write(state(self)->out, &byte, 1);
        return;
    }

    /* Clear screen command. */

    if (byte == '\f') {
        write(state(self)->out, "\033[2J\033[1;1H", 10);
        return;
    }

    /* If the user sends the control marker (17d / 11h), the console
       will drop into a special control mode. */

    if (byte == CONSOLE_CTRL)
        state(self)->control_mode = true;
}

static bool console_poll(device& self)
{
    /* First check our own readbuffer. */
    if (state(self)->readbuffer.size())
        return true;

    struct pollfd poll_rq;
    poll_rq.fd = state(self)->in;
    poll_rq.events = POLLIN;

    if (poll(&poll_rq, 1, 0) == -1)
        return false;

    return poll_rq.revents & POLLIN;
}

static void console_close(device& self)
{
    delete static_cast<console_state *>(self.state);
}
