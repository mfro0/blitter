#include <stdio.h>
#include <mint/osbind.h>

#include "blitter.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT   400

#define NOP()   __asm__ __volatile__("nop");

static inline void blitter_start(volatile struct blitter_regs *blitter)
{
    /*
     * this is the "Atari ST Profibuch"-recommended way to start the blitter:
     * NOHOG mode but restart immediately after each iteration until finished
     */

    __asm__ __volatile__(
        "start%=:                                   \t\n"
        "       lea     %[line_num],a0              \t\n"
        "       bset.b  %[hogbit],(a0)              \t\n"
        "restart%=:                                 \t\n"
        "       bset.b  %[busybit],(a0)             \t\n"
        "       nop                                 \t\n"
        "       bne.s   restart%=                   \t\n"
        :
        : [line_num] "m" (blitter->line_num8), [hogbit] "i" (7), [busybit] "i" (6)
        : "a0", "cc"
    );
}

static inline void clear_area(volatile struct blitter_regs *blitter, void *start_addr, int x, int y, int w, int h)
{
    uint16_t *start = start_addr;

    blitter->op = OP_ZERO;
    blitter->endmask1 = ~0 >> (x % 16);         /* */
    blitter->endmask2 = ~0;
    blitter->endmask3 = ~0 << ((x + w) % 16);
    blitter->x_count = (w + 15) / 16;
    blitter->y_count = h;
    blitter->dst_xinc = blitter->src_xinc = 2;          /* monochrome only, for now */
    blitter->dst_yinc = blitter->src_yinc = SCREEN_WIDTH / (sizeof(uint16_t) * 8);
    blitter->src_addr = blitter->dst_addr = start +
                                            x / sizeof(uint16_t) +
                                            y * SCREEN_WIDTH / sizeof(uint16_t);
    blitter->skew = 0;
    blitter->fxsr = 0;
    blitter->nfsr = 0;
    blitter->smudge = 0;
    blitter->hop = HOP_ALL_ONE;
    blitter->hog = 0;

    blitter_start(blitter);
}

inline void set_area(volatile struct blitter_regs *blitter, void *start_addr, int x, int y, int w, int h)
{
    uint16_t *start = start_addr;

    blitter->op = OP_ONE;                       /* write all ones */
    blitter->endmask1 = ~0 >> (x % 16);         /* */
    blitter->endmask2 = ~0;
    blitter->endmask3 = ~0 << ((x + w) % 16);
    blitter->x_count = (w + 15) / 16;
    blitter->y_count = h;
    blitter->dst_xinc = 2;          /* monochrome only, for now */
    blitter->dst_yinc = SCREEN_WIDTH / (sizeof(uint16_t) * 8);
    blitter->src_addr = blitter->dst_addr = start +
                                            x / sizeof(uint16_t) +
                                            y * SCREEN_WIDTH / sizeof(uint16_t);
    blitter->skew = 0;
    blitter->fxsr = 0;
    blitter->nfsr = 0;
    blitter->smudge = 0;
    blitter->hop = HOP_ALL_ONE;
    blitter->hog = 0;

    blitter_start(blitter);
}

static volatile struct blitter_regs *blitter = (volatile struct blitter_regs *) 0xffff8a00;

void flicker(void)
{
    int i;
    void *start_addr = Physbase();

    for (i = 0; i < 100000; i++)
    {
        set_area(blitter, start_addr, 10, 10, 640 - 20, 400 - 20);
        clear_area(blitter, start_addr, 10, 10, 640 - 20, 400 - 20);
    }
}

int main(int argc, char *argv[])
{
    Supexec(flicker);
    return 0;
}
