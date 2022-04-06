/* Main emulator function, accessable from the outside.
   Copyright (C) 2021-2022 bellrise */

#include "emul_graphics.h"
#include <setjmp.h>
#include <string.h>
#include "emul.h"


static void load_file(void *addr, size_t max, size_t off, const char *path);
static int text_page_handler(struct page_info *self, ir_word vaddr);
static void fail(struct emulator_ctx *ctx, int err, const char *msg);
static void handle_cpucall(struct emulator_ctx *ctx);
static ir_half *get_raddr(struct emulator_ctx *ctx, ir_half id);
static void set_register8(struct emulator_ctx *ctx, ir_half id, ir_half val);
static void set_register16(struct emulator_ctx *ctx, ir_half id, ir_word val);
static inline void stack_push8(struct emulator_ctx *ctx, ir_half val);
static inline void stack_push16(struct emulator_ctx *ctx, ir_word val);
static inline ir_half stack_pop8(struct emulator_ctx *ctx);
static inline ir_word stack_pop16(struct emulator_ctx *ctx);


#define PUSH8(VAL)      stack_push8(&context, VAL)
#define PUSH16(VAL)     stack_push16(&context, VAL)
#define POP8()          stack_pop8(&context)
#define POP16()         stack_pop16(&context)


int irid_emulate(const char *binfile, size_t load_offt, int opts, void *_win)
{
	struct emulator_ctx context = {0};
	struct page_info *text_pages[2];
	struct page_info *cpu_page;
	int crash_val;

	context.win  = _win;
	context.regs = calloc(sizeof(struct irid_reg), 1);
	context.mem  = calloc(sizeof(struct memory_bank), 1);
	context.is_silent = opts & IRID_EMUL_QUIET;

	/*
	 * Setup the exception jump buffer. If the CPU fails, it will return here.
	 * This is done using setjmp/longjmp, because this function needs to return
	 * the error code back to the program without killing the whole program.
	 */
	crash_val = setjmp(context.crash_ret);
	if (crash_val)
		goto end;

	bank_alloc(context.mem, IRID_MAX_PAGES, 0);
	load_file(context.mem->base_ptr, IRID_MAX_ADDR, load_offt, binfile);

	/*
	 * Mark the last page as read-only, for cpucalls. The third and second last
	 * pages are initiallly mapped to 80*25 screen I/O.
	 */

	cpu_page = &context.mem->page_info[context.mem->pages - 1];
	text_pages[0] = &context.mem->page_info[context.mem->pages - 3];
	text_pages[1] = &context.mem->page_info[context.mem->pages - 2];

	cpu_page->flags = PAGE_READ | PAGE_CPU;
	text_pages[0]->flags |= PAGE_TEXT;
	text_pages[1]->flags |= PAGE_TEXT;
	text_pages[0]->write_handler = text_page_handler;
	text_pages[1]->write_handler = text_page_handler;

	/*
	 * Start executing from address 0x0000. We don't have to worry about ending
	 * this loop, because if the instruction pointer goes above IRID_MAX_ADDR,
	 * it will just roll over.
	 */

	ir_half instruction, half_temp;
	ir_word temp;
	ir_half *here;

	while (1) {
		instruction = context.mem->base_ptr[context.regs->ip];
		here        = &((ir_half *) context.mem->base_ptr)[context.regs->ip];

		switch (instruction) {
			/* Push a value from a register onto the stack. */
			case I_PUSH:
				temp = * (ir_word *) get_raddr(&context, here[1]);
				if (here[1] <= R_R7 || here[1] >= R_IP)
					PUSH16(temp);
				else
					PUSH8(*get_raddr(&context, here[1]));
				context.regs->ip += 2;
				break;

			/* Push an immediate value onto the stack. */
			case I_PUSH8:
				PUSH8(here[1]);
				context.regs->ip += 2;
				break;

			/* Push an immediate value onto the stack. */
			case I_PUSH16:
				PUSH16(* (ir_word *) (here + 1));
				context.regs->ip += 3;
				break;

			/* Pop a value off the stack into a register. */
			case I_POP:
				if (here[1] <= R_R7 || here[1] >= R_IP) {
					temp = POP16();
					info("pop16=%x", temp);
					set_register16(&context, here[1], temp);
				} else {
					half_temp = POP8();
					info("pop8=%x", half_temp);
					set_register8(&context, here[1], half_temp);
				}
				context.regs->ip += 2;
				break;

			/* Move a value from a register to another register. */
			case I_MOV:
				temp = * (ir_word *) get_raddr(&context, here[2]);
				if (here[1] <= R_R7 || here[1] >= R_IP)
					set_register16(&context, here[1], temp);
				else
					set_register8(&context, here[1], temp);
				context.regs->ip += 3;
				break;

			/* Copy an 8-bit value to a register. */
			case I_MOV8:
				set_register8(&context, here[1], here[2]);
				context.regs->ip += 3;
				break;

			/* Copy a 16-bit value into a register. */
			case I_MOV16:
				set_register16(&context, here[1], * ((ir_word *)(here + 2)));
				context.regs->ip += 4;
				break;

			/* Load contents of memory pointed by [rx] to [rx/hx]. */
			case I_LOAD:
				temp = *get_raddr(&context, here[2]);
				if (here[1] <= R_R7 || here[1] >= R_IP) {
					temp = read16(context.mem, temp);
					set_register16(&context, here[1], temp);
				} else {
					half_temp = read8(context.mem, temp);
					set_register8(&context, here[1], half_temp);
				}
				context.regs->ip += 3;
				break;

			/* Store contents of [rx/hx] to the address pointed by [rx]. */
			case I_STORE:
				temp = *get_raddr(&context, here[2]);
				if (here[1] <= R_R7 || here[1] >= R_IP) {
					write16(context.mem, temp, * (ir_word *) get_raddr(
								&context, here[1]));
				} else {
					write8(context.mem, temp, *get_raddr(&context, here[1]));
				}
				context.regs->ip += 3;
				break;

			/* Reset a register. */
			case I_NULL:
				if (here[1] <= R_R7 || here[1] >= R_IP)
					set_register16(&context, here[1], 0);
				else
					set_register8(&context, here[1], 0);
				context.regs->ip += 2;
				break;

			/* Execute a CPU function. */
			case I_CPUCALL:
				handle_cpucall(&context);
				context.regs->ip += 2;
				break;

			default:
				fail(&context, CPUFAULT_INS, "illegal instruction");
		}

		if (context.regs->sf)
			break;
	}

end:
	bank_free(context.mem);
	free(context.regs);
	free(context.mem);
	return crash_val;
}

#undef PUSH8
#undef PUSH16
#undef POP8
#undef POP16

static void handle_cpucall(struct emulator_ctx *ctx)
{
	switch (ctx->mem->base_ptr[ctx->regs->ip+1]) {
		case CPUCALL_SHUTDOWN:
			ctx->regs->sf = 1;
			break;

		case CPUCALL_RESTART:
			/*
			 * In order to restart the whole emulator, we only need to set
			 * all registers to 0. Because the instruction pointer is also
			 * set to 0, the next iteration of the emulator loop will
			 * automatically move to the start of memory.
			 */
			memset(ctx->regs, 0, sizeof(struct irid_reg));
			break;

		case CPUCALL_FAULT:
			fail(ctx, CPUFAULT_USER, "intentional cpufault");
			break;

		default:
			fail(ctx, CPUFAULT_CPUCALL, "unknown cpucall");
	}
}

static ir_half *get_raddr(struct emulator_ctx *ctx, ir_half id)
{
	/*
	 * Register IDs 0x00-0x07 are generic 16 bit registers. For a quick trick,
	 * we can use the fact that these register IDs are also the offset into
	 * the irid_reg struct.
	 */

	if (id <= R_R7) {
		return ((ir_half *) ctx->regs) + (id << 1);
	} else if (id >= R_IP && id <= R_BP) {
		ir_half *s = (ir_half *) (&ctx->regs->ip) + ((id - 0x70) << 1);
		return s;
	} else if (id >= R_H0 && id <= R_L3) {
		return ((ir_half *) ctx->regs) + id - 0x10;
	} else {
		fail(ctx, CPUFAULT_REG, "unknown register ID");
	}

	return NULL;
}

static void set_register8(struct emulator_ctx *ctx, ir_half id, ir_half val)
{
	ir_half *addr = get_raddr(ctx, id);
	*addr = val;
}

static void set_register16(struct emulator_ctx *ctx, ir_half id, ir_word val)
{
	ir_word *addr = (ir_word *) get_raddr(ctx, id);
	*addr = val;
}

static inline void stack_push8(struct emulator_ctx *ctx, ir_half val)
{
	if (ctx->regs->sp > ctx->regs->bp)
		fail(ctx, CPUFAULT_STACK, "stack overflow");
	ctx->regs->sp -= 1;
	ctx->mem->base_ptr[ctx->regs->sp] = val;
}

static inline void stack_push16(struct emulator_ctx *ctx, ir_word val)
{
	if (ctx->regs->sp > ctx->regs->bp)
		fail(ctx, CPUFAULT_STACK, "stack overflow");
	ctx->regs->sp -= 2;
	* (ir_word *) (ctx->mem->base_ptr + ctx->regs->sp) = val;
}

static inline ir_half stack_pop8(struct emulator_ctx *ctx)
{
	if (ctx->regs->sp >= ctx->regs->bp)
		fail(ctx, CPUFAULT_STACK, "stack corruption, read above bp");
	ctx->regs->sp += 1;
	return ctx->mem->base_ptr[ctx->regs->sp - 1];
}

static inline ir_word stack_pop16(struct emulator_ctx *ctx)
{
	if (ctx->regs->sp + 1 >= ctx->regs->bp)
		fail(ctx, CPUFAULT_STACK, "stack corruption, read above bp");
	ctx->regs->sp += 2;
	return * (ir_word *) (ctx->mem->base_ptr + ctx->regs->sp - 2);
}

static void load_file(void *addr, size_t max, size_t off, const char *path)
{
	size_t index = off;
	FILE *fp;
	int c;

	if (!(fp = fopen(path, "rb")))
		die("could not open file '%s' to move into memory", path);

	while ((c = fgetc(fp)) != EOF) {
		if (index >= max) {
			warn("could not fit file into memory");
			break;
		}
		((unsigned char *) addr)[index++] = (unsigned char) c;
	}
}

static int text_page_handler(struct page_info *_Unused page, ir_word vaddr)
{
	return vaddr;
}

static void fail(struct emulator_ctx *ctx, int err, const char *msg)
{
	struct irid_reg *regs = ctx->regs;

	if (ctx->is_silent)
		goto crash;

	printf("\n\033[1;31mCPU fault:\033[1;39m %s\033[0m\n\n", msg);

	printf(
		"Registers:\n"
		"  r0=0x%04x r1=0x%04x r2=0x%04x r3=0x%04x\n"
		"  r4=0x%04x r5=0x%04x r6=0x%04x r7=0x%04x\n"
		"  ip=0x%04x sp=0x%04x bp=0x%04x\n"
		"  cf=%d      zf=%d      of=%d      sf=%d\n\n",
		regs->r0, regs->r1, regs->r2, regs->r3,
		regs->r4, regs->r5, regs->r6, regs->r7,
		regs->ip, regs->sp, regs->bp,
		regs->cf, regs->zf, regs->of, regs->sf
	);

crash:
	/*
	 * Return to the irid_emulate function by using longjmp. Set the crash_val
	 * to the error number, so the emulator can return it.
	 */
	longjmp(ctx->crash_ret, err);
}
