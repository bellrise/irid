/* Main emulator function, accessable from the outside.
   Copyright (C) 2021 bellrise */

#include <string.h>
#include "emul.h"


static void load_file(void *addr, size_t max, size_t off, const char *path);
static int text_page_handler(struct page_info *self, ir_word vaddr);
static void fail(struct irid_reg *regs, const char *msg);
static void handle_cpucall(struct memory_bank *mem, struct irid_reg *regs);
static ir_half *get_raddr(struct irid_reg *regs, ir_half id);
static void set_register8(struct irid_reg *regs, ir_half id, ir_half val);
static void set_register16(struct irid_reg *regs, ir_half id, ir_word val);
static inline void stack_push8(struct memory_bank *mem, struct irid_reg *regs,
        ir_half val);
static inline void stack_push16(struct memory_bank *mem, struct irid_reg *regs,
        ir_word val);
static inline ir_half stack_pop8(struct memory_bank *mem,
        struct irid_reg *regs);
static inline ir_word stack_pop16(struct memory_bank *mem,
        struct irid_reg *regs);


#define PUSH8(VAL)      stack_push8(&memory, &regs, VAL)
#define PUSH16(VAL)     stack_push16(&memory, &regs, VAL)
#define POP8()          stack_pop8(&memory, &regs)
#define POP16()         stack_pop16(&memory, &regs)


int irid_emulate(const char *binfile, size_t load_offt, int _Unused opts)
{
    struct memory_bank memory;
    struct irid_reg regs = {0};
    struct page_info *text_pages[2];
    struct page_info *cpu_page;

    bank_alloc(&memory, IRID_MAX_PAGES, 0);
    load_file(memory.base_ptr, IRID_MAX_ADDR, load_offt, binfile);

    /*
     * Mark the last page as read-only, for cpucalls. The third and second last
     * pages are initiallly mapped to 80*25 screen I/O.
     */

    cpu_page = &memory.page_info[memory.pages - 1];
    text_pages[0] = &memory.page_info[memory.pages - 3];
    text_pages[1] = &memory.page_info[memory.pages - 2];

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
    ir_word temp, temp2;
    ir_half *here;

    while (1) {
        instruction = memory.base_ptr[regs.ip];
        here        = &((ir_half *) memory.base_ptr)[regs.ip];

        switch (instruction) {
            /* Push a value from a register onto the stack. */
            case I_PUSH:
                temp = * (ir_word *) get_raddr(&regs, here[1]);
                if (here[1] <= R_R7 || here[1] >= R_IP)
                    PUSH16(temp);
                else
                    PUSH8(*get_raddr(&regs, here[1]));
                regs.ip += 2;
                break;

            /* Push an immediate value onto the stack. */
            case I_PUSH8:
                PUSH8(here[1]);
                regs.ip += 2;
                break;

            /* Push an immediate value onto the stack. */
            case I_PUSH16:
                PUSH16(* (ir_word *) (here + 1));
                regs.ip += 3;
                break;

            /* Pop a value off the stack into a register. */
            case I_POP:
                if (here[1] <= R_R7 || here[1] >= R_IP) {
                    temp = POP16();
                    info("pop16=%x", temp);
                    set_register16(&regs, here[1], temp);
                } else {
                    half_temp = POP8();
                    info("pop8=%x", half_temp);
                    set_register8(&regs, here[1], half_temp);
                }
                regs.ip += 2;
                break;

            /* Move a value from a register to another register. */
            case I_MOV:
                temp = * (ir_word *) get_raddr(&regs, here[2]);
                if (here[1] <= R_R7 || here[1] >= R_IP)
                    set_register16(&regs, here[1], temp);
                else
                    set_register8(&regs, here[1], temp);
                regs.ip += 3;
                break;

            /* Copy an 8-bit value to a register. */
            case I_MOV8:
                set_register8(&regs, here[1], here[2]);
                regs.ip += 3;
                break;

            /* Copy a 16-bit value into a register. */
            case I_MOV16:
                set_register16(&regs, here[1], * ((ir_word *)(here + 2)));
                regs.ip += 4;
                break;

            /* Load a value from memory pointed by [addr] to a register. */
            case I_LOAD:
                temp = * (ir_word *) (here + 2);
                if (here[1] <= R_R7 || here[1] >= R_IP)
                    set_register16(&regs, here[1], read16(&memory, temp));
                else
                    set_register8(&regs, here[1], read8(&memory, temp));
                regs.ip += 4;
                break;

            /* Store a value from a register into memory pointed by [addr]. */
            case I_STORE:
                temp  = * (ir_word *) (here + 2);
                temp2 = * (ir_word *) get_raddr(&regs, here[1]);
                if (here[1] <= R_R7 || here[1] >= R_IP)
                    write16(&memory, temp, temp2);
                else
                    write8(&memory, temp, *get_raddr(&regs, here[1]));
                regs.ip += 4;
                break;

            /* Reset a register. */
            case I_NULL:
                if (here[1] <= R_R7 || here[1] >= R_IP)
                    set_register16(&regs, here[1], 0);
                else
                    set_register8(&regs, here[1], 0);
                regs.ip += 2;
                break;

            /* Execute a CPU function. */
            case I_CPUCALL:
                handle_cpucall(&memory, &regs);
                regs.ip += 2;
                break;

            default:
                fail(&regs, "invalid instruction");
        }

        if (regs.sf)
            break;
    }

    bank_free(&memory);
    return 0;
}

static void handle_cpucall(struct memory_bank *mem, struct irid_reg *regs)
{
    switch (mem->base_ptr[regs->ip+1]) {
        case CPUCALL_SHUTDOWN:
            regs->sf = 1;
            break;

        case CPUCALL_RESTART:
            /*
             * In order to restart the whole emulator, we only need to set
             * all registers to 0. Because the instruction pointer is also
             * set to 0, the next iteration of the emulator loop will
             * automatically move to the start of memory.
             */
            memset(regs, 0, sizeof(struct irid_reg));
            break;

        case CPUCALL_FAULT:
            fail(regs, "intentional cpufault");
            break;

        default:
            fail(regs, "unknown cpucall");
    }
}

static ir_half *get_raddr(struct irid_reg *regs, ir_half id)
{
    /*
     * Register IDs 0x00-0x07 are generic 16 bit registers. For a quick trick,
     * we can use the fact that these register IDs are also the offset into
     * the irid_reg struct.
     */

    if (id <= R_R7) {
        return ((ir_half *) regs) + (id << 1);
    }
    else if (id >= 0x70) {
        ir_half *s = (ir_half *) (&regs->ip) + ((id - 0x70) << 1);
        info("%p, %p", regs, s);
        return s;
    }
    else if (id >= 0x10) {
        return ((ir_half *) regs) + id - 0x10;
    }
    else
        fail(regs, "unknown register ID");

    return NULL;
}

static void set_register8(struct irid_reg *regs, ir_half id, ir_half val)
{
    ir_half *addr = get_raddr(regs, id);
    *addr = val;
}

static void set_register16(struct irid_reg *regs, ir_half id, ir_word val)
{
    ir_word *addr = (ir_word *) get_raddr(regs, id);
    *addr = val;
}

static inline void stack_push8(struct memory_bank *mem, struct irid_reg *regs,
        ir_half val)
{
    if (regs->sp > regs->bp)
        fail(regs, "stack overflow");
    regs->sp -= 1;
    mem->base_ptr[regs->sp] = val;
}

static inline void stack_push16(struct memory_bank *mem, struct irid_reg *regs,
        ir_word val)
{
    if (regs->sp > regs->bp)
        fail(regs, "stack overflow");
    regs->sp -= 2;
    * (ir_word *) (mem->base_ptr + regs->sp) = val;
}

static inline ir_half stack_pop8(struct memory_bank *mem,
        struct irid_reg *regs)
{
    if (regs->sp >= regs->bp)
        fail(regs, "stack corruption, read above bp");
    regs->sp += 1;
    return mem->base_ptr[regs->sp - 1];
}

static inline ir_word stack_pop16(struct memory_bank *mem,
        struct irid_reg *regs)
{
    if (regs->sp + 1 >= regs->bp)
        fail(regs, "stack corruption, read above bp");
    regs->sp += 2;
    return * (ir_word *) (mem->base_ptr + regs->sp - 2);
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

static int text_page_handler(struct page_info *_Unused self, ir_word _Unused
        vaddr)
{
    return 0;
}

static void fail(struct irid_reg *regs, const char *msg)
{
    printf("\n\033[1;31mCPU fault:\033[1;39m %s\033[0m\n\n", msg);

    printf(
        "Registers:\n"
        "  r0=0x%04x r1=0x%04x r2=0x%04x r3=0x%04x\n"
        "  r4=0x%04x r5=0x%04x r6=0x%04x r7=0x%04x\n"
        "  ip=0x%04x sp=0x%04x bp=0x%04x\n"
        "  cf=%d      zf=%d      of=%d     sf=%d\n\n",
        regs->r0, regs->r1, regs->r2, regs->r3,
        regs->r4, regs->r5, regs->r6, regs->r7,
        regs->ip, regs->sp, regs->bp,
        regs->cf, regs->zf, regs->of, regs->sf
    );

    die("CPU fault");
}
