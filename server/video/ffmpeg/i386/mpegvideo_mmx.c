/*
 * The simplest mpeg encoder (well, it was the simplest!)
 * Copyright (c) 2000,2001 Gerard Lantau.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Optimized for ia32 cpus by Nick Kurshev <nickols_k@mail.ru>
 */

#include "../dsputil.h"
#include "../mpegvideo.h"

#if 0

/* XXX: GL: I don't understand why this function needs optimization
   (it is called only once per frame!), so I disabled it */

void MPV_frame_start(MpegEncContext *s)
{
    if (s->pict_type == B_TYPE) {
	__asm __volatile(
	    "movl	(%1), %%eax\n\t"
	    "movl	4(%1), %%edx\n\t"
	    "movl	8(%1), %%ecx\n\t"
	    "movl	%%eax, (%0)\n\t"
	    "movl	%%edx, 4(%0)\n\t"
	    "movl	%%ecx, 8(%0)\n\t"
	    :
	    :"r"(s->current_picture), "r"(s->aux_picture)
	    :"eax","edx","ecx","memory");
    } else {
            /* swap next and last */
	__asm __volatile(
	    "movl	(%1), %%eax\n\t"
	    "movl	4(%1), %%edx\n\t"
	    "movl	8(%1), %%ecx\n\t"
	    "xchgl	(%0), %%eax\n\t"
	    "xchgl	4(%0), %%edx\n\t"
	    "xchgl	8(%0), %%ecx\n\t"
	    "movl	%%eax, (%1)\n\t"
	    "movl	%%edx, 4(%1)\n\t"
	    "movl	%%ecx, 8(%1)\n\t"
	    "movl	%%eax, (%2)\n\t"
	    "movl	%%edx, 4(%2)\n\t"
	    "movl	%%ecx, 8(%2)\n\t"
	    :
	    :"r"(s->last_picture), "r"(s->next_picture), "r"(s->current_picture)
	    :"eax","edx","ecx","memory");
    }
}
#endif

static const unsigned long long int mm_wabs __attribute__ ((aligned(8))) = 0xffffffffffffffffULL;
static const unsigned long long int mm_wone __attribute__ ((aligned(8))) = 0x0001000100010001ULL;

/*
  NK:
  Note: looking at PARANOID:
  "enable all paranoid tests for rounding, overflows, etc..."

#ifdef PARANOID
                if (level < -2048 || level > 2047)
                    fprintf(stderr, "unquant error %d %d\n", i, level);
#endif
  We can suppose that result of two multiplications can't be greate of 0xFFFF
  i.e. is 16-bit, so we use here only PMULLW instruction and can avoid
  a complex multiplication.
=====================================================
 Full formula for multiplication of 2 integer numbers
 which are represent as high:low words:
 input: value1 = high1:low1
        value2 = high2:low2
 output: value3 = value1*value2
 value3=high3:low3 (on overflow: modulus 2^32 wrap-around)
 this mean that for 0x123456 * 0x123456 correct result is 0x766cb0ce4
 but this algorithm will compute only 0x66cb0ce4
 this limited by 16-bit size of operands
 ---------------------------------
 tlow1 = high1*low2
 tlow2 = high2*low1
 tlow1 = tlow1 + tlow2
 high3:low3 = low1*low2
 high3 += tlow1
*/
static void dct_unquantize_mpeg1_mmx(MpegEncContext *s,
                                     DCTELEM *block, int n, int qscale)
{
    int i, level;
    const UINT16 *quant_matrix;
    if (s->mb_intra) {
        if (n < 4) 
            block[0] = block[0] * s->y_dc_scale;
        else
            block[0] = block[0] * s->c_dc_scale;
        if (s->out_format == FMT_H263) {
            i = 1;
            goto unquant_even;
        }
        /* XXX: only mpeg1 */
        quant_matrix = s->intra_matrix;
	i=1;
	/* Align on 4 elements boundary */
	while(i&3)
	{
            level = block[i];
            if (level) {
                if (level < 0) level = -level;
                    level = (int)(level * qscale * quant_matrix[i]) >> 3;
                    level = (level - 1) | 1;
                if (block[i] < 0) level = -level;
                block[i] = level;
            }
	    i++;
	}
	__asm __volatile(
	"movd	%0, %%mm6\n\t"       /* mm6 = qscale | 0  */
	"punpckldq %%mm6, %%mm6\n\t" /* mm6 = qscale | qscale */
	"movq	%2, %%mm4\n\t"
	"movq	%%mm6, %%mm7\n\t"
	"movq	%1, %%mm5\n\t"
	"packssdw %%mm6, %%mm7\n\t" /* mm7 = qscale | qscale | qscale | qscale */
	"pxor	%%mm6, %%mm6\n\t"
	::"g"(qscale),"m"(mm_wone),"m"(mm_wabs):"memory");
        for(;i<64;i+=4) {
		__asm __volatile(
			"movq	%1, %%mm0\n\t"
			"movq	%%mm7, %%mm1\n\t"
			"movq	%%mm0, %%mm2\n\t"
			"movq	%%mm0, %%mm3\n\t"
			"pcmpgtw %%mm6, %%mm2\n\t"
			"pmullw	%2, %%mm1\n\t"
			"pandn	%%mm4, %%mm2\n\t"
			"por	%%mm5, %%mm2\n\t"  
			"pmullw	%%mm2, %%mm0\n\t" /* mm0 = abs(block[i]). */
			
			"pcmpeqw %%mm6, %%mm3\n\t"
			"pmullw	%%mm0, %%mm1\n\t"
			"psraw	$3, %%mm1\n\t"
			"psubw	%%mm5, %%mm1\n\t"   /* block[i] --; */
			"pandn	%%mm4, %%mm3\n\t"  /* fake of pcmpneqw : mm0 != 0 then mm1 = -1 */
			"por	%%mm5, %%mm1\n\t"   /* block[i] |= 1 */
			"pmullw %%mm2, %%mm1\n\t"   /* change signs again */

			"pand	%%mm3, %%mm1\n\t" /* nullify if was zero */
			"movq	%%mm1, %0"
			:"=m"(block[i])
			:"m"(block[i]), "m"(quant_matrix[i])
			:"memory");
        }
    } else {
        i = 0;
    unquant_even:
        quant_matrix = s->non_intra_matrix;
	/* Align on 4 elements boundary */
	while(i&3)
	{
	    level = block[i];
            if (level) {
                if (level < 0) level = -level;
                    level = (((level << 1) + 1) * qscale *
                             ((int) quant_matrix[i])) >> 4;
                    level = (level - 1) | 1;
                if(block[i] < 0) level = -level;
                block[i] = level;
	    }
	    i++;
	}
	__asm __volatile(
	"movd	%0, %%mm6\n\t"       /* mm6 = qscale | 0  */
	"punpckldq %%mm6, %%mm6\n\t" /* mm6 = qscale | qscale */
	"movq	%2, %%mm4\n\t"
	"movq	%%mm6, %%mm7\n\t"
	"movq	%1, %%mm5\n\t"
	"packssdw %%mm6, %%mm7\n\t" /* mm7 = qscale | qscale | qscale | qscale */
	"pxor	%%mm6, %%mm6\n\t"
	::"g"(qscale),"m"(mm_wone),"m"(mm_wabs):"memory");
        for(;i<64;i+=4) {
		__asm __volatile(
			"movq	%1, %%mm0\n\t"
			"movq	%%mm7, %%mm1\n\t"
			"movq	%%mm0, %%mm2\n\t"
			"movq	%%mm0, %%mm3\n\t"
			"pcmpgtw %%mm6, %%mm2\n\t"
			"pmullw	%2, %%mm1\n\t"
			"pandn	%%mm4, %%mm2\n\t"
			"por	%%mm5, %%mm2\n\t"  
			"pmullw	%%mm2, %%mm0\n\t" /* mm0 = abs(block[i]). */
			"psllw	$1, %%mm0\n\t" /* block[i] <<= 1 */
			"paddw	%%mm5, %%mm0\n\t" /* block[i] ++ */
			
			"pmullw	%%mm0, %%mm1\n\t"
			"psraw	$4, %%mm1\n\t"
			"pcmpeqw %%mm6, %%mm3\n\t"
			"psubw	%%mm5, %%mm1\n\t"   /* block[i] --; */
			"pandn	%%mm4, %%mm3\n\t"  /* fake of pcmpneqw : mm0 != 0 then mm1 = -1 */
			"por	%%mm5, %%mm1\n\t"   /* block[i] |= 1 */
			"pmullw %%mm2, %%mm1\n\t"   /* change signs again */

			"pand	%%mm3, %%mm1\n\t" /* nullify if was zero */
			"movq	%%mm1, %0"
			:"=m"(block[i])
			:"m"(block[i]), "m"(quant_matrix[i])
			:"memory");
        }
    }
}

void MPV_common_init_mmx(MpegEncContext *s)
{
    if (mm_flags & MM_MMX) {
        /* XXX: should include h263 optimization too. It would go even
           faster! */
        s->dct_unquantize = dct_unquantize_mpeg1_mmx;
    }
}