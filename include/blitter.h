#ifndef _BLITTER_H_
#define _BLITTER_H_


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
    unsigned short halftone[16];                 // the blitter halftone RAM

    short src_xinc;                      // source X increment
    short src_yinc;                      // source Y increment
    unsigned short *src_addr;                    // address of source block

    unsigned short endmask1;                     // left endmask
    unsigned short endmask2;                     // middle mask
    unsigned short endmask3;                     // right endmask

    short dst_xinc;                      // destination X increment
    short dst_yinc;                      // destination Y increment
    unsigned short *dst_addr;                    // address of destination area

    unsigned short x_count;                      // number of words in destination line (0 = 65536)
    unsigned short y_count;                      // number of words in destination line (0 = 65536)

    union
    {
        struct
        {
            unsigned char resvd0  : 6;
            unsigned char hop     : 2;          // halftone operation. See "HOP register constants" above
            unsigned char resvd1  : 4;
            unsigned char op      : 4;          // logic op. See "operation modes" above
        };
        struct                          // same as bytes if one prefers ...
        {
            unsigned char hop8;
            unsigned char op8;
        };
        unsigned short hop_op16;                 // ... or do you rather want words?
    };
    union
    {
        struct
        {
            unsigned char busy        : 1;      // blitter busy bit. 1 = active
            unsigned char hog         : 1;      // "hog bus" bit: 0 = allow CPU to bus every 64 cycles
            unsigned char smudge      : 1;      // special effects...
            unsigned char resvd2      : 1;
            unsigned char line_num    : 4;      // index into halftone mask
            unsigned char fxsr        : 1;      // force extra source read (first word)
            unsigned char nfsr        : 1;      // no final source read (last word)
            unsigned char rsv0        : 2;
            unsigned char skew        : 4;      // number of bits to shift right source data
        };
        struct                          // same as bytes if one prefers
        {
            unsigned char line_num8;
            unsigned char skew8;
        };
        unsigned short lno_skew16;
    };
};

#endif /* _BLITTER_H_ */
