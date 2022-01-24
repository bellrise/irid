# Copyright (C) 2021-2022 bellrise
# A single buildfile for the whole project.

# If no target is defined, build the whole project.
@__default  build _setup && build -f emul/buildfile && build -f asm/buildfile

@clean      rm -rf build
@install    sh scripts/install

# Private targets
@_setup     mkdir -p build
