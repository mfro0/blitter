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
    uint16_t halftone[16];              // the blitter halftone RAM

    int16_t src_xinc;                   // source X increment
    int16_t src_yinc;                   // source Y increment
    uint16_t *src_addr;                 // address of source block

    uint16_t endmask1;                  // left endmask
    uint16_t endmask2;                  // middle mask
    uint16_t endmask3;                  // right endmask

    int16_t dst_xinc;                   // destination X increment
    int16_t dst_yinc;                   // destination Y increment
    uint16_t *dst_addr;                 // address of destination area

    uint16_t x_count;                   // number of words in destination line (0 = 65536)
    uint16_t y_count;                   // number of words in destination line (0 = 65536)

    union
    {
        struct
        {
            uint8_t resvd0  : 6;
            uint8_t hop     : 2;        // halftone operation. See "HOP register constants" above
            uint8_t resvd1  : 4;
            uint8_t op      : 4;        // logic op. See "operation modes" above
        };
        struct                          // same as bytes if one prefers ...
        {
            uint8_t hop8;
            uint8_t op8;
        };
        uint16_t hop_op16;              // ... or do you rather want words?
    };
    union
    {
        struct
        {
            uint8_t busy        : 1;    // blitter busy bit. 1 = active
            uint8_t hog         : 1;    // "hog bus" bit: 0 = allow CPU to bus every 64 cycles
            uint8_t smudge      : 1;    // special effects...
            uint8_t resvd2      : 1;
            uint8_t line_num    : 4;    // index into halftone mask
            uint8_t fxsr        : 1;    // force extra source read (first word)
            uint8_t nfsr        : 1;    // no final source read (last word)
            uint8_t rsv0        : 2;
            uint8_t skew        : 4;    // number of bits to shift right source data
        };
        struct                          // same as bytes if one prefers
        {
            uint8_t line_num8;
            uint8_t skew8;
        };
        uint16_t lno_skew16;
    };
};

#endif /* _BLITTER_H_ */
