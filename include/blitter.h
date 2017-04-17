#ifndef _BLITTER_H_
#define _BLITTER_H_

#include <stdint.h>

/*
 * blitter operation modes
 */
#define OP_ZERO                 0   /* destination = 0 */
#define OP_SRC_AND_DST          1   /* destination = source and destination */
#define OP_SRC_AND_NOT_DST      2   /* destination = source and not destination */
#define OP_SRC                  3   /* destination = source */
#define OP_NOT_SRC_AND_DST      4   /* destination = not source and destination */
#define OP_DST                  5   /* destination = destination */
#define OP_SRC_XOR_DST          6   /* destination = source xor destination */
#define OP_SRC_OR_DST           7   /* destination = source or destination */
#define OP_NOT_SRC_AND_NOT_DST  8   /* destination = not source and not destination */
#define OP_NOT_SRC_XOR_DST      9   /* destination = not source xor destination */
#define OP_NOT_DST              10  /* destination = not destination */
#define OP_SRC_OR_NOT_DST       11  /* destination = source or not destination */
#define OP_NOT_SRC              12  /* destination = not source */
#define OP_NOT_SRC_OR_DST       13  /* destination = not source or destination */
#define OP_NOT_SRC_OR_NOT_DST   14  /* destination = not source or not destination */
#define OP_ONE                  15  /* destination = 1 */

/*
 * HOP register constants
 */
#define HOP_ALL_ONE             0
#define HOP_HALFTONE_ONLY       1
#define HOP_SOURCE_ONLY         2
#define HOP_SOURCE_AND_HALFTONE 3

/*
 * blitter register addresses
 */
struct blitter_regs
{
    uint16_t halftone_ram[16];

    int16_t src_xinc;
    int16_t src_yinc;
    uint16_t *src_addr;

    uint16_t endmask1;
    uint16_t endmask2;
    uint16_t endmask3;

    int16_t dst_xinc;
    int16_t dst_yinc;
    uint16_t *dst_addr;

    uint16_t x_count;
    uint16_t y_count;

    union
    {
        struct
        {
            uint8_t resvd0  : 6;
            uint8_t hop     : 2;
            uint8_t resvd1  : 4;
            uint8_t op      : 4;
        };
        struct
        {
            uint8_t hop8;
            uint8_t op8;
        };
    };
    union
    {
        struct
        {
            uint8_t busy        : 1;
            uint8_t hog         : 1;
            uint8_t smudge      : 1;
            uint8_t resvd2      : 1;
            uint8_t line_num    : 4;
            uint8_t fxsr        : 1;
            uint8_t nfsr        : 1;
            uint8_t rsv0        : 2;
            uint8_t skew        : 4;
        };
        struct
        {
            uint8_t line_num8;
            uint8_t skew8;
        };
        uint16_t lno_skew16;
    };
};

#endif /* _BLITTER_H_ */
