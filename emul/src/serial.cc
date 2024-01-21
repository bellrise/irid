/* Serial device implementation
   Copyright (c) 2023-2024 bellrise */

#include "emul.h"

#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <queue>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct serial_state
{
    int fd;
};

static u8 serial_read(device&);
static void serial_write(device&, u8);
static bool serial_poll(device&);

static inline serial_state *state(device& self)
{
    return static_cast<serial_state *>(self.state);
}

device serial_create(u16 id, const std::string& name, const std::string& file)
{
    device serial = {id, name};
    serial_state *state;

    state = new serial_state;
    serial.state = state;

    state->fd = open(file.c_str(), O_RDWR);
    if (state->fd == -1)
        die("failed to open serial device %s @ %s", name.c_str(), file.c_str());

    serial.read = serial_read;
    serial.write = serial_write;
    serial.poll = serial_poll;
    serial.close = nullptr;

    return serial;
}

static u8 serial_read(device& self)
{
    u8 c;

    /* Make this non-blocking by returning a NUL if stdin is empty. */
    if (!serial_poll(self))
        return 0;

    if (read(state(self)->fd, &c, 1) <= 0)
        return 0;

    return c;
}

static void serial_write(device& self, u8 byte)
{
    write(state(self)->fd, &byte, 1);
}

static bool serial_poll(device& self)
{
    struct pollfd poll_rq;
    poll_rq.fd = state(self)->fd;
    poll_rq.events = POLLIN;

    if (poll(&poll_rq, 1, 0) == -1)
        return false;

    return poll_rq.revents & POLLIN;
}
