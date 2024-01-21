/* Device
   Copyright (c) 2023-2024 bellrise */

#include "emul.h"

device::device(u16 id, const std::string& name)
    : id(id)
    , interrupt_ptr(0)
    , name(name)
{ }
