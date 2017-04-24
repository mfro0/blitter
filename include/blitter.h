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
    UWORD halftone[16];                 // the blitter halftone RAM

    WORD src_xinc;                      // source X increment
    WORD src_yinc;                      // source Y increment
    UWORD *src_addr;                    // address of source block

    UWORD endmask1;                     // left endmask
    UWORD endmask2;                     // middle mask
    UWORD endmask3;                     // right endmask

    WORD dst_xinc;                      // destination X increment
    WORD dst_yinc;                      // destination Y increment
    UWORD *dst_addr;                    // address of destination area

    UWORD x_count;                      // number of words in destination line (0 = 65536)
    UWORD y_count;                      // number of words in destination line (0 = 65536)

    union
    {
        struct
        {
            UBYTE resvd0  : 6;
            UBYTE hop     : 2;          // halftone operation. See "HOP register constants" above
            UBYTE resvd1  : 4;
            UBYTE op      : 4;          // logic op. See "operation modes" above
        };
        struct                          // same as bytes if one prefers ...
        {
            UBYTE hop8;
            UBYTE op8;
        };
        UWORD hop_op16;                 // ... or do you rather want words?
    };
    union
    {
        struct
        {
            UBYTE busy        : 1;      // blitter busy bit. 1 = active
            UBYTE hog         : 1;      // "hog bus" bit: 0 = allow CPU to bus every 64 cycles
            UBYTE smudge      : 1;      // special effects...
            UBYTE resvd2      : 1;
            UBYTE line_num    : 4;      // index into halftone mask
            UBYTE fxsr        : 1;      // force extra source read (first word)
            UBYTE nfsr        : 1;      // no final source read (last word)
            UBYTE rsv0        : 2;
            UBYTE skew        : 4;      // number of bits to shift right source data
        };
        struct                          // same as bytes if one prefers
        {
            UBYTE line_num8;
            UBYTE skew8;
        };
        UWORD lno_skew16;
    };
};

#endif /* _BLITTER_H_ */
