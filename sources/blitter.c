#include <stdio.h>
#include <string.h>
#include <mint/osbind.h>
#include <mint/linea.h>

#include "blitter.h"
#include "pattern.h"


#define NOP()   __asm__ __volatile__("nop");

static volatile struct blitter_regs *blitter = (volatile struct blitter_regs *) 0xffff8a00;
__LINEA *__aline;
__FONT  **__fonts;
short  (**__funcs) (void);

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

    blitter->hog = 0;
    blitter->busy = 1;
    __asm__ __volatile__(
        "       lea     %[line_num],a0              \t\n"
        "restart%=:                                 \t\n"
        "       bset.b  %[busybit],(a0)             \t\n"
        "       jbne    restart%=                   \t\n"
        :
        : [line_num] "m" (blitter->line_num8), [busybit] "i" (BUSYBIT)
        : "a0", "memory", "cc"
    );
}


static unsigned short l_endmask[] =
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

static unsigned short *r_endmask = &l_endmask[1];

void blitter_init(void)
{
    memset((void *) blitter, 0, sizeof(struct blitter_regs));
}

static inline void blitter_set_fill_pattern(const unsigned short pattern[], unsigned short mask)
{
    int i;


    for (i = 0; i < sizeof(blitter->halftone) / sizeof(unsigned short); i++)
    {
        blitter->halftone[i] = pattern[i & mask];
    }
}

#define BITS_PER(a)     (sizeof(a) * 8)
#define NUM_PLANES      __aline->_VPLANES
#define SCREEN_WIDTH    (V_X_MAX + 1)
#define SCREEN_HEIGHT   (V_Y_MAX + 1)
#define SCR_WDWIDTH     ((SCREEN_WIDTH / BITS_PER(short)) * NUM_PLANES)

void blit_area(short mode, void *src_addr, short src_x, short src_y, short dst_x, short dst_y, short w, short h, short hop)
{
    int plane;
    short x0 = dst_x;
    short x1 = dst_x + w - 1;
    short xcount = (x1 / BITS_PER(short) - x0 / BITS_PER(short));
    unsigned short *scr = src_addr;



    for (plane = 0; plane < NUM_PLANES; plane++)
    {
        blitter->op = mode;
        blitter->skew = 0; // dst_x & 15;
        blitter->fxsr = 0;
        blitter->nfsr = 0;
        blitter->smudge = 0;
        blitter->hop = hop;
        blitter->line_num = dst_y & 15;
        /*
         * determine endmasks
         */
        blitter->endmask1 = l_endmask[x0 & 15];
        blitter->endmask2 = ~0;
        blitter->endmask3 = ~r_endmask[x1 & 15];

        blitter->dst_xinc = NUM_PLANES * sizeof(short);      // offset to the next word in the same line and plane (in bytes)
        blitter->dst_yinc = (SCR_WDWIDTH - xcount * NUM_PLANES) * 2;         // offset (in bytes) from the last word in current line to the first in next one

        blitter->src_xinc = NUM_PLANES * sizeof(short);
        blitter->src_yinc = (dst_x + w) / 16 - dst_x / 16;

        blitter->src_addr = scr;

        blitter->x_count = xcount + 1;
        blitter->y_count = h;

        blitter->dst_addr = scr +                                       // start address
                            dst_y * SCR_WDWIDTH +                       // + y * number of words/line
                            x0 / BITS_PER(short) * NUM_PLANES +          // + x *
                            plane;

        blitter_start();
    }
}


void pump(void)
{
    int i;
    int j;
    void *start_addr = Physbase();

    blitter_set_fill_pattern(HATCH1, HAT_1_MSK);
    for (i = 0; i < 3; i++)
    {
        /*
         * grow
         */
        for (j = 15; j < V_X_MAX - 2; j += 16)
        {
            int src_x = 0;
            int src_y = 0;
            int dst_x;
            int dst_y;
            int w;
            int h;

            w = j;
            h = w * V_Y_MAX / V_X_MAX;

            dst_x = (V_X_MAX - w) / 2;
            dst_y = (V_Y_MAX - h) / 2;

            blit_area(OP_SRC, start_addr, src_x, src_y, dst_x, dst_y, w, h, HOP_HALFTONE_ONLY);
            blit_area(OP_SRC, start_addr, src_x, src_y, dst_x, dst_y, w, h, HOP_HALFTONE_ONLY);
        }

        /*
         * shrink
         */
        for (j = V_X_MAX - 2; j >= 15; j -= 16)
        {
            int src_x = 0;
            int src_y = 0;
            int dst_x;
            int dst_y;
            int w;
            int h;

            w = j;
            h = w * V_Y_MAX / V_X_MAX;

            dst_x = (V_X_MAX - w) / 2;
            dst_y = (V_Y_MAX - h) / 2;

            blit_area(OP_SRC, start_addr, src_x, src_y, dst_x, dst_y, w, h, HOP_HALFTONE_ONLY);
            blit_area(OP_SRC, start_addr, src_x, src_y, dst_x, dst_y, w, h, HOP_HALFTONE_ONLY);
        }
    }
}

void flicker(void)
{
    int i;
    unsigned short *start_addr = Physbase();

    for (i = 1; i < 10; i++)
    {
        blit_area(OP_ONE, start_addr, 0, 0, 10, 10, V_X_MAX - 10 - 10, V_Y_MAX - 10 - 10, HOP_ALL_ONE);
        blit_area(OP_ZERO, start_addr, 0, 0, 10, 10, V_X_MAX - 10 - 10, V_Y_MAX - 10 - 10, HOP_ALL_ONE);
    }
}

void fill(void)
{
    int i, j;
    void *start_addr = Physbase();

    for (j = 0; j < sizeof(OEMPAT) / sizeof(unsigned short) / (OEMMSKPAT + 1); j++)
    {
        blitter_set_fill_pattern(OEMPAT + j * (OEMMSKPAT + 1), OEMMSKPAT);
        for (i = 22; i < 36; i++)
        {
            blit_area(OP_SRC, start_addr, 0, 0, 20, 20, V_X_MAX - 40, V_Y_MAX - 80, HOP_HALFTONE_ONLY);
        }
    }


    for (j = 0; j < sizeof(DITHER) / sizeof(unsigned short) / (DITHRMSK + 1); j++)
    {
        blitter_set_fill_pattern(DITHER + j * (DITHRMSK + 1), DITHRMSK);
        for (i = 13; i < 27; i++)
        {
            blit_area(OP_SRC, start_addr, 0, 0, i, i, V_X_MAX - 40, V_Y_MAX - 80, HOP_HALFTONE_ONLY);
        }
    }

    for (j = 0; j < sizeof(HATCH0) / sizeof(unsigned short) / (HAT_0_MSK + 1); j++)
    {
        blitter_set_fill_pattern(HATCH0 + j * (HAT_0_MSK + 1), HAT_0_MSK);
        for (i = 13; i < 27; i++)
        {
            blit_area(OP_SRC, start_addr, 0, 0, i, i, V_X_MAX - 40, V_Y_MAX - 80, HOP_HALFTONE_ONLY);
        }
    }

    for (j = 0; j < sizeof(HATCH1) / sizeof(unsigned short) / (HAT_1_MSK + 1); j++)
    {
        blitter_set_fill_pattern(HATCH1 + j * (HAT_1_MSK + 1), HAT_1_MSK);
        for (i = 13; i < 27; i++)
        {
            blit_area(OP_SRC, start_addr, 0, 0, i, i, V_X_MAX - 40, V_Y_MAX - 80, HOP_HALFTONE_ONLY);
        }
    }
}


int main(int argc, char *argv[])
{

    linea0();

    (void) *__aline;
    (void) *__fonts;
    (void) *__funcs;

    Supexec(blitter_init);
    Supexec(flicker);
    Supexec(pump);
    Supexec(fill);

    return 0;
}
