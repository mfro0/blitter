#include <stdio.h>
#include <mint/osbind.h>

#include "blitter.h"

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   400

#define NOP()   __asm__ __volatile__("nop");

static inline void blitter_start(volatile struct blitter_regs *blitter)
{
#define HOGBIT  6
#define BUSYBIT 7
    /*
     * this is the "Atari ST Profibuch"-recommended way to start the blitter:
     * NOHOG mode but restart immediately after each iteration until finished.
     *
     * This is done in inline assembler since bset.b tests _and_ sets the corresponding bit in one single instruction
     * which can't be done reliably with pure C.
     */

    __asm__ __volatile__(
        "start%=:                                   \t\n"
        "       lea     %[line_num],a0              \t\n"
        "       bclr.b  %[hogbit],(a0)              \t\n"   // clear HOG bit - apparently wrong in Profibuch where the bit is set instead
        "restart%=:                                 \t\n"
        "       bset.b  %[busybit],(a0)             \t\n"
        "       nop                                 \t\n"
        "       bne.s   restart%=                   \t\n"
        :
        : [line_num] "m" (blitter->line_num8), [hogbit] "i" (HOGBIT), [busybit] "i" (BUSYBIT)
        : "a0", "cc"
    );
}

#define BITS_PER(a)     (sizeof(a) * 8)

static inline void blit_area(volatile struct blitter_regs *blitter, int mode, void *start_addr, int x, int y, int w, int h)
{
    uint16_t *start = start_addr;

    blitter->op = mode;
    blitter->endmask1 = ~0 >> (x % BITS_PER(uint16_t));
    blitter->endmask2 = ~0;
    blitter->endmask3 = ~0 << ((x + w) % BITS_PER(uint16_t));
    blitter->x_count = (w + 15) / BITS_PER(uint16_t);
    blitter->y_count = h;
    blitter->dst_xinc = 2;          /* monochrome only, for now */
    blitter->dst_yinc = blitter->src_yinc = (SCREEN_WIDTH - w + x) / BITS_PER(uint8_t);
    blitter->src_addr = blitter->dst_addr = start +
                                            x / (sizeof(uint16_t) * 8) +
                                            y * (SCREEN_WIDTH / BITS_PER(uint16_t));
    blitter->skew = 0;
    blitter->fxsr = 0;
    blitter->nfsr = 0;
    blitter->smudge = 0;
    blitter->hop = HOP_ALL_ONE;

    blitter_start(blitter);
}

static volatile struct blitter_regs *blitter = (volatile struct blitter_regs *) 0xffff8a00;

void flicker(void)
{
    int i;
    void *start_addr = Physbase();

    for (i = 0; i < 100; i++)
    {
        blit_area(blitter, OP_ZERO, start_addr, 16, 16, 640 - 2 * 16, 400 - 2 * 16);
        Vsync();
        blit_area(blitter, OP_ONE, start_addr, 16, 16, 640 - 2 * 16, 400 - 2 * 16);
        Vsync();
    }
}

int main(int argc, char *argv[])
{
    Supexec(flicker);

    return 0;
}
