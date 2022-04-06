/* Common info/error/warn utilities.
   Copyright (C) 2021-2022 bellrise */

#include <irid/common.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>


int _die_impl(const char *prog, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "%s: \033[1;31merror: \033[1;39m", prog);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\033[0m\n");

	va_end(args);

	exit(1);
	return 0;
}

int _warn_impl(const char *prog, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "%s: \033[1;35mwarning: \033[1;39m", prog);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\033[0m\n");

	va_end(args);
	return 0;
}

int _info_impl(const char *prog, const char *func, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "%s: \033[1;36minfo: \033[0m(%s)\033[1;39m ", prog, func);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\033[0m\n");

	va_end(args);
	return 0;
}

void dbytes(void *addr, size_t amount)
{
	/*
	 * To make this look nice, bytes are grouped to 16 per line, simmilar to
	 * what xxd or hexdump does. To get the amount of 16 byte groups, we only
	 * have to move the amount by 4 bits right. Any bytes left are printed on
	 * the last line.
	 */
	size_t lines, rest;

	lines = amount >> 4;
	for (size_t i = 0; i < lines; i++) {
		for (size_t j = 0; j < 16; j++) {
			printf("%02hhx", ((char *) addr)[i*16+j]);
			if (j % 2 != 0)
				putc(' ', stdout);
		}
		printf("\n");
	}

	rest = (lines << 4) ^ amount;
	for (size_t i = 0; i < rest; i++) {
		printf("%02hhx", ((char *) addr)[(lines << 4) + i]);
		if (i % 2 != 0)
			putc(' ', stdout);
	}

	if (rest)
		putc('\n', stdout);
}
