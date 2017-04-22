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
     * which can't be done with C as elegant.
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

static volatile struct blitter_regs *blitter = (volatile struct blitter_regs *) 0xffff8a00;

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

static uint16_t *r_endmask = &l_endmask[1];

static inline void blit_area(int mode, void *src_addr, int src_x, int src_y, int dst_x, int dst_y, int w, int h)
{
    struct blitter_regs b;
    short x2 = dst_x + w - 1;
    short src_span, dst_span;

    src_span = ((src_x & 15) + (dst_x & 15)) & 15;
    dst_span = ((dst_x & 15) + (dst_x & 15)) & 15;

    if (src_span < dst_span)
        b.fxsr = 1;
    else
        b.fxsr = 0;

    b.op = mode;

    /*
     * determine endmasks
     */
    b.endmask1 = l_endmask[dst_x & 15];
    b.endmask2 = ~0;
    b.endmask3 = ~r_endmask[x2 & 15];


    b.x_count = (w + 15) / BITS_PER(short);
    b.y_count = h;

    b.src_xinc = 2;
    b.src_yinc = w / BITS_PER(uint8_t);
    b.src_addr = src_addr;

    b.dst_xinc = 2;          /* monochrome only, for now */
    b.dst_yinc = (SCREEN_WIDTH - w) / BITS_PER(uint8_t);
    b.dst_addr = src_addr +
                 dst_x / BITS_PER(uint16_t) +
                 dst_y * (SCREEN_WIDTH / BITS_PER(uint16_t));

    b.skew = dst_x & 15;
    b.fxsr = 0;
    b.nfsr = 0;
    b.smudge = 0;
    b.hop = HOP_ALL_ONE;
    *blitter = b;

    blitter_start(blitter);
}


void pump(void)
{
    int i;
    int j;
    void *start_addr = Physbase();

    for (i = 0; i < 10; i++)
    {
        /*
         * grow
         */
        for (j = 5 * 16; j < 640 - 2 * 16; j++)
        {
            int src_x;
            int src_y;
            int dst_x;
            int dst_y;
            int w;
            int h;

            w = j;
            h = w * 400 / 640;

            dst_x = (640 - w) / 2;
            dst_y = (400 - h) / 2;

            blit_area(OP_ZERO, start_addr, src_x, src_y, dst_x, dst_y, w, h);
            blit_area(OP_ONE, start_addr, src_x, src_y, dst_x, dst_y, w, h);
        }
        /*
         * shrink
         */
        for (j = 640 - 2 * 16 -1; j >= 5 * 16; j--)
        {
            int src_x;
            int src_y;
            int dst_x;
            int dst_y;
            int w;
            int h;

            w = j;
            h = w * 400 / 640;

            dst_x = (640 - w) / 2;
            dst_y = (400 - h) / 2;

            blit_area(OP_ONE, start_addr, src_x, src_y, dst_x, dst_y, w, h);
            blit_area(OP_ZERO, start_addr, src_x, src_y, dst_x, dst_y, w, h);
        }
    }
}

void flicker(void)
{
    int i;
    void *start_addr = Physbase();

    for (i = 0; i < 100; i++)
    {
        blit_area(OP_ZERO, start_addr, 0, 0, 16, 16, 640 - 2 * 16, 400 - 2 * 16);
        blit_area(OP_ONE, start_addr, 0, 0, 16, 16, 640 - 2 * 16, 400 - 2 * 16);
    }
}

int main(int argc, char *argv[])
{
    Supexec(flicker);
    Supexec(pump);
    return 0;
}
