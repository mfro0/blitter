#include <stdio.h>
#include <string.h>
#include <mint/osbind.h>

#include "portab.h"
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

static volatile struct blitter_regs *blitter = (volatile struct blitter_regs *) 0xffff8a00;

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

static void blitter_set_fill_pattern(UWORD pattern[], WORD len)
{
    int i;

    for (i = 0; i < sizeof(blitter->halftone); i += len)
    {
        memcpy(&blitter->halftone[i], pattern, len * sizeof(UWORD));
    }
}

#define BITS_PER(a)     (sizeof(a) * 8)
#define NUM_PLANES      1

static UWORD *r_endmask = &l_endmask[1];

inline static void blit_area(WORD mode, void *src_addr, WORD src_x, WORD src_y, WORD dst_x, WORD dst_y, WORD w, WORD h, WORD hop)
{
    WORD x2 = dst_x + w - 1;
    WORD start_word = dst_x / (NUM_PLANES * BITS_PER(WORD));
    WORD end_word = x2 / (NUM_PLANES * BITS_PER(WORD));
    WORD width_words = (end_word - start_word) + 1;
    UWORD *scr = src_addr;

    blitter->op = mode;

    /*
     * determine endmasks
     */
    blitter->endmask1 = l_endmask[dst_x & 15];
    blitter->endmask2 = ~0;
    blitter->endmask3 = ~r_endmask[x2 & 15];


    blitter->x_count = width_words;
    blitter->y_count = h;

    blitter->src_xinc = NUM_PLANES * BITS_PER(WORD);
    blitter->src_yinc = (dst_x + w) / 16 - dst_x / 16 + 1;
    blitter->src_addr = src_addr;

    blitter->dst_xinc = NUM_PLANES * sizeof(WORD);
    blitter->dst_yinc = (SCREEN_WIDTH / BITS_PER(WORD) - width_words + 1) * sizeof(UWORD);
    blitter->dst_addr = scr +
                        start_word +
                        dst_y * (SCREEN_WIDTH / BITS_PER(UWORD) * NUM_PLANES);

    // blitter->skew = 0; // dst_x & 15;
    blitter->fxsr = 0;
    blitter->nfsr = 0;
    blitter->smudge = 0;
    blitter->hop = hop;

    blitter_start(blitter);
}


extern UWORD HAT_1_MSK;
extern UWORD HATCH1[];

void pump(void)
{
    int i;
    int j;
    void *start_addr = Physbase();

    blitter_set_fill_pattern(HATCH1, HAT_1_MSK + 1);
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
    int i;
    void *start_addr = Physbase();

    blitter_set_fill_pattern(HATCH1, HAT_1_MSK + 1);
    for (i = 0; i < 100; i++)
    {
        blitter->line_num = (400 - i) & 15;
        blit_area(OP_SRC, start_addr, 0, 0, i, i, 640 - i, 400 - i, HOP_HALFTONE_ONLY);
        Vsync();
    }
}

int main(int argc, char *argv[])
{
    // Supexec(flicker);
    Supexec(pump);
    Supexec(fill);

    return 0;
}
