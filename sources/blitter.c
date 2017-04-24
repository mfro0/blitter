#include <stdio.h>
#include <string.h>
#include <mint/osbind.h>

#include "portab.h"
#include "blitter.h"
#include "pattern.h"

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   400

#define NOP()   __asm__ __volatile__("nop");

static volatile struct blitter_regs *blitter = (volatile struct blitter_regs *) 0xffff8a00;

static inline void blitter_start(void)
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


static UWORD l_endmask[] =
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

static UWORD *r_endmask = &l_endmask[1];

static inline void blitter_set_fill_pattern(const UWORD pattern[], UWORD mask)
{
    int i;


    for (i = 0; i < sizeof(blitter->halftone) / sizeof(UWORD); i++)
    {
        blitter->halftone[i] = pattern[i & mask];
    }
}

#define BITS_PER(a)     (sizeof(a) * 8)
#define NUM_PLANES      8


inline static void blit_area(WORD mode, void *src_addr, WORD src_x, WORD src_y, WORD dst_x, WORD dst_y, WORD w, WORD h, WORD hop)
{
    int plane;
    WORD x2 = dst_x + w - 1;
    UWORD *scr = src_addr;
    WORD start_word = dst_x / BITS_PER(WORD);
    WORD end_word = x2 / BITS_PER(WORD);
    WORD width_words = (end_word - start_word) + 1;

    blitter->op = mode;
    blitter->dst_xinc = NUM_PLANES * sizeof(WORD);
    blitter->dst_yinc = (SCREEN_WIDTH / BITS_PER(WORD) - width_words + 1) * sizeof(UWORD) * NUM_PLANES;
    blitter->src_xinc = NUM_PLANES * BITS_PER(WORD);
    blitter->src_yinc = (dst_x + w) / 16 - dst_x / 16 + 1;
    blitter->skew = 0; // dst_x & 15;
    blitter->fxsr = 0;
    blitter->nfsr = 1;
    blitter->smudge = 0;
    blitter->hop = hop;

    /*
     * determine endmasks
     */
    blitter->endmask1 = l_endmask[dst_x & 15];
    blitter->endmask2 = ~0;
    blitter->endmask3 = ~r_endmask[x2 & 15];

    for (plane = 0; plane < NUM_PLANES; plane++)
    {
        blitter->x_count = width_words;
        blitter->y_count = h;

        blitter->src_addr = scr;

        blitter->dst_addr = scr +
                            dst_y * (SCREEN_WIDTH / BITS_PER(UWORD) * NUM_PLANES) +
                            start_word + plane;


        blitter_start();
    }
}


void pump(void)
{
    int i;
    int j;
    void *start_addr = Physbase();

    blitter_set_fill_pattern(HATCH1, HAT_1_MSK);
    for (i = 0; i < 10; i++)
    {
        /*
         * grow
         */
        for (j = 15; j < 640 - 2; j++)
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

            blitter->line_num = dst_y & 15;

            blit_area(OP_SRC, start_addr, src_x, src_y, dst_x, dst_y, w, h, HOP_HALFTONE_ONLY);
            Vsync();
            blit_area(OP_SRC, start_addr, src_x, src_y, dst_x, dst_y, w, h, HOP_HALFTONE_ONLY);
            Vsync();
        }

        /*
         * shrink
         */
        for (j = 640 - 2; j >= 15; j--)
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

            blitter->line_num = dst_y & 15;

            blit_area(OP_SRC, start_addr, src_x, src_y, dst_x, dst_y, w, h, HOP_HALFTONE_ONLY);
            Vsync();
            blit_area(OP_SRC, start_addr, src_x, src_y, dst_x, dst_y, w, h, HOP_HALFTONE_ONLY);
            Vsync();
        }
    }
}

void flicker(void)
{
    int i;
    void *start_addr = Physbase();

    for (i = 0; i < 100; i++)
    {
        blit_area(OP_ONE, start_addr, 0, 0, 1, 1, 400 - i, 400 - 2 * 16, HOP_ALL_ONE);
        Vsync();
        blit_area(OP_ZERO, start_addr, 0, 0, 1, 1, 400 - i, 400 - 2 * 16, HOP_ALL_ONE);
        Vsync();
    }
}

void fill(void)
{
    int i, j;
    void *start_addr = Physbase();

    /*
     * FIXME: for some reason, the blitter crashes in Hatari if we don't clear the line_num
     * and skew registers before a write to the halftone RAM ???
     */
    blitter->lno_skew16 = 0;

    for (j = 0; j < sizeof(OEMPAT) / sizeof(UWORD) / (OEMMSKPAT + 1); j++)
    {
        blitter_set_fill_pattern(OEMPAT + j * (OEMMSKPAT + 1), OEMMSKPAT);
        blitter->line_num = 15;
        for (i = 22; i < 36; i++)
        {
            blitter->skew = 0;
            blit_area(OP_SRC, start_addr, 0, 0, i, i, 200, 200, HOP_HALFTONE_ONLY);
            Vsync();
            Cconin();
        }
    }


    for (j = 0; j < sizeof(DITHER) / sizeof(UWORD) / (DITHRMSK + 1); j++)
    {
        blitter_set_fill_pattern(DITHER + j * (DITHRMSK + 1), DITHRMSK);
        blitter->line_num = 15;
        for (i = 13; i < 27; i++)
        {
            blitter->skew = 0;
            blit_area(OP_SRC, start_addr, 0, 0, i, i, 200, 200, HOP_HALFTONE_ONLY);
            Vsync();
            Cconin();
        }
    }

    for (j = 0; j < sizeof(HATCH0) / sizeof(UWORD) / (HAT_0_MSK + 1); j++)
    {
        blitter_set_fill_pattern(HATCH0 + j * (HAT_0_MSK + 1), HAT_0_MSK);
        blitter->line_num = 15;
        for (i = 13; i < 27; i++)
        {
            blitter->skew = 0;
            blit_area(OP_SRC, start_addr, 0, 0, i, i, 200, 200, HOP_HALFTONE_ONLY);
            Vsync();
            Cconin();
        }
    }

    for (j = 0; j < sizeof(HATCH1) / sizeof(UWORD) / (HAT_1_MSK + 1); j++)
    {
        blitter_set_fill_pattern(HATCH1 + j * (HAT_1_MSK + 1), HAT_1_MSK);
        blitter->line_num = 15;
        for (i = 13; i < 27; i++)
        {
            blitter->skew = 0;
            blit_area(OP_SRC, start_addr, 0, 0, i, i, 200, 200, HOP_HALFTONE_ONLY);
            Vsync();
            Cconin();
        }
    }
}


int main(int argc, char *argv[])
{
    // Supexec(flicker);
    // Supexec(pump);
    Supexec(fill);

    return 0;
}
