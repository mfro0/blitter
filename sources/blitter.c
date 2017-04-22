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
        "restart%=:                                 \t\n"   // contrary to the surrounding comments
        "       bset.b  %[busybit],(a0)             \t\n"
        "       nop                                 \t\n"
        "       jbne    restart%=                   \t\n"
        :
        : [line_num] "m" (blitter->line_num8), [hogbit] "i" (HOGBIT), [busybit] "i" (BUSYBIT)
        : "a0", "cc"
    );
}

#define BITS_PER(a)     (sizeof(a) * 8)

static uint16_t l_endmask[] =
{
    0xffff,
    0x7fff,
    0x3fff,
    0x1fff,
    0x0fff,
    0x07ff,
    0x03ff,
    0x01ff,
    0x00ff,
    0x007f,
    0x003f,
    0x001f,
    0x000f,
    0x0007,
    0x0003,
    0x0001,
    0x0000
};

uint16_t *r_endmask = &l_endmask[1];

static inline void blit_area(volatile struct blitter_regs *blitter, int mode, void *start_addr, int x, int y, int w, int h)
{
    uint16_t *start = start_addr;
    int x1 = x;
    int y1 = y;
    int x2 = x + w - 1;
    int y2 = y + h - 1;

    blitter->op = mode;
    blitter->endmask1 = r_endmask[x1 & 15];
    blitter->endmask2 = ~0;
    blitter->endmask3 = ~l_endmask[x2 & 15];
    blitter->x_count = (w + 15) / BITS_PER(uint16_t);
    blitter->y_count = h;
    blitter->dst_xinc = 2;          /* monochrome only, for now */
    blitter->dst_yinc = (SCREEN_WIDTH - w) / BITS_PER(uint8_t);
    blitter->dst_addr = start +
                        x / (sizeof(uint16_t) * 8) +
                        y * (SCREEN_WIDTH / BITS_PER(uint16_t));
    blitter->skew = x & 15;
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

void pump(void)
{
    int i;
    void *start_addr = Physbase();

    for (i = 0; i < 100; i++)
    {
        blit_area(blitter, OP_ZERO, start_addr, 16 + i, 16 + i, 640 - 2 * (16 + i), 400 - 2 * (16 + i));
        Vsync();
        blit_area(blitter, OP_ONE, start_addr, 16 + i, 16 + i, 640 - 2 * (16 + i), 400 - 2 * (16 + i));
        Vsync();
    }
}

int main(int argc, char *argv[])
{
    // Supexec(flicker);
    Supexec(pump);
    return 0;
}
