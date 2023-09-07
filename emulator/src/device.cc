/* Device
   Copyright (c) 2023 bellrise */

#include "emul.h"

device::device(u16 id, const char *name)
    : id(id)
    , handlerptr(0)
    , name(name)
{ }
