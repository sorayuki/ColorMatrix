/*
**                 ColorMatrix v2.5 for Avisynth 2.5.x
**
**   ColorMatrix 2.0 is based on the original ColorMatrix filter by Wilbert 
**   Dijkhof.  It adds the ability to convert between any of: Rec.709, FCC, 
**   Rec.601, and SMPTE 240M. It also makes pre and post clipping optional,
**   adds range expansion/contraction, and more...
**
**   Copyright (C) 2006-2009 Kevin Stone
**
**   ColorMatrix 1.x is Copyright (C) Wilbert Dijkhof
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**   GNU General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program; if not, write to the Free Software
**   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ColorMatrix.h"

#pragma warning(disable : 4311 4312)

#define GETPTRS() \
	const PS_INFO *pss = (PS_INFO*)ps; \
	const unsigned char *srcpY = pss->srcp; \
	const unsigned char *srcpU = pss->srcpU; \
	const unsigned char *srcpV = pss->srcpV; \
	const int src_pitchY = pss->src_pitch; \
	const int src_pitchR = pss->src_pitchR; \
	const int src_pitchY2 = src_pitchY*2; \
	const int src_pitchUV = pss->src_pitchUV; \
	unsigned char *dstpY = pss->dstp; \
	unsigned char *dstpU = pss->dstpU; \
	unsigned char *dstpV = pss->dstpV; \
	const int dst_pitchY = pss->dst_pitch; \
	const int dst_pitchR = pss->dst_pitchR; \
	const int dst_pitchY2 = dst_pitchY*2; \
	const int dst_pitchUV = pss->dst_pitchUV; \
	const int width = pss->widtha; \
	int height = pss->height; \

#define GETMMXVS() \
	int loopctr = width>>3; \
	__int64 fact_YU, fact_YV, fact_UU; \
	__int64 fact_UV, fact_VV, fact_VU; \
	getmmxv(fact_YU, pss->cs->c2); \
	getmmxv(fact_YV, pss->cs->c3); \
	getmmxv(fact_UU, pss->cs->c4); \
	getmmxv(fact_UV, pss->cs->c5); \
	getmmxv(fact_VU, pss->cs->c6); \
	getmmxv(fact_VV, pss->cs->c7); \

void getmmxv(__int64 &v, int c)
{
	c = abs(c);
	c = simd_scale(c);
	v = (c<<16)+c;
	v += v<<32;
}

#define GETSSE2VS() \
	int loopctr = width>>4; \
	__m128 fact_YU, fact_YV, fact_UU; \
	__m128 fact_UV, fact_VV, fact_VU; \
	getsse2v(&fact_YU, pss->cs->c2); \
	getsse2v(&fact_YV, pss->cs->c3); \
	getsse2v(&fact_UU, pss->cs->c4); \
	getsse2v(&fact_UV, pss->cs->c5); \
	getsse2v(&fact_VU, pss->cs->c6); \
	getsse2v(&fact_VV, pss->cs->c7); \

void getsse2v(__m128 *v, int c)
{
	__int64 t[2];
	c = abs(c);
	c = simd_scale(c);
	t[0] = (c<<16)+c;
	t[0] += t[0]<<32;
	t[1] = t[0];
	__asm
	{
		mov eax,v
		movdqu xmm0,t
		movdqa [eax],xmm0
	}
}

void conv1_YV12_MMX(void *ps)
{
	GETPTRS();
	GETMMXVS();
	__asm
	{
		mov esi, srcpY
		mov edi, dstpY
		mov eax, srcpU
		mov ecx, srcpV
		mov edx, dstpV
		mov ebx, dstpU
		pxor mm7, mm7
		movq mm6, Q64
		align 16
xloop:
		movd mm0, [eax]
		movd mm1, [ecx]
		punpcklbw mm0, mm7
		punpcklbw mm1, mm7
		psubw mm0, Q128
		psubw mm1, Q128
		pmullw mm0, mm6
		pmullw mm1, mm6
		movq mm2, mm0
		movq mm3, mm1
		pmulhw mm2, fact_YU
		pmulhw mm3, fact_YV
		paddw mm2, mm3
		movq mm3, mm2
		movq mm4, [esi]
		punpcklwd mm2, mm2
		punpckhwd mm3, mm3
		movq mm5, mm4
		punpcklbw mm4, mm7
		punpckhbw mm5, mm7
		pmullw mm4, mm6
		pmullw mm5, mm6
		paddw mm4, mm2
		paddw mm5, mm3
		paddw mm4, Q32
		paddw mm5, Q32
		psraw mm4, 6
		psraw mm5, 6
		packuswb mm4, mm5
		movq [edi], mm4
		add esi, src_pitchR
		add	edi, dst_pitchR
		movq mm4, [esi]
		movq mm5, mm4
		punpcklbw mm4, mm7
		punpckhbw mm5, mm7
		pmullw mm4, mm6
		pmullw mm5, mm6
		paddw mm4, mm2
		paddw mm5, mm3
		paddw mm4, Q32
		paddw mm5, Q32
		psraw mm4, 6
		psraw mm5, 6
		packuswb mm4, mm5
		movq [edi], mm4
		sub	esi, src_pitchR
		sub	edi, dst_pitchR
		movq mm2, mm0
		movq mm3, mm1
		paddw mm2, mm2
		pmulhw mm2, fact_UU
		pmulhw mm3, fact_UV
		psubsw mm2, mm3
		paddw mm2, Q8224
		psraw mm2, 6
		packuswb mm2, mm7
		movd [ebx], mm2
		movq mm2, mm0
		movq mm3, mm1
		paddw mm3, mm3
		pmulhw mm3, fact_VV
		pmulhw mm2, fact_VU
		psubsw mm3, mm2
		paddw mm3, Q8224
		psraw mm3, 6
		packuswb mm3, mm7
		movd [edx], mm3
		add esi, 8
		add edi, 8
		add eax, 4
		add ebx, 4
		add ecx, 4
		add edx, 4
		dec loopctr
		jnz xloop
		sub height, 2
		jz end
		mov eax, width
		mov esi, srcpY
		shr eax, 3
		mov edi, dstpY
		mov loopctr, eax
		mov ebx, dstpU
		mov eax, srcpU
		mov edx, dstpV
		mov ecx, srcpV
		add esi, src_pitchY2
		add edi, dst_pitchY2
		add eax, src_pitchUV
		add ebx, dst_pitchUV
		add ecx, src_pitchUV
		add edx, dst_pitchUV
		mov srcpY, esi
		mov dstpY, edi
		mov dstpU, ebx
		mov dstpV, edx
		mov srcpU, eax
		mov srcpV, ecx
		jmp xloop
end:
		emms
	}
}

void conv2_YV12_MMX(void *ps)
{
	GETPTRS();
	GETMMXVS();
	__asm
	{
		mov esi, srcpY
		mov edi, dstpY
		mov eax, srcpU
		mov ebx, dstpU
		mov ecx, srcpV
		mov edx, dstpV
		pxor mm7, mm7			// all 0's
		movq mm6, Q64			// 64's words
		align 16
xloop:
		movd mm0, [eax]			// move 1st 4 U bytes
		movd mm1, [ecx]			// move 1st 4 V bytes
		punpcklbw mm0, mm7		// unpack to words, 0U0U0U0U
		punpcklbw mm1, mm7		// unpack to words, 0V0V0V0V
		psubw mm0, Q128			// adj for 128 chroma offset
		psubw mm1, Q128			// adj for 128 chroma offset
		pmullw mm0, mm6			// *64, for rounding later			
		pmullw mm1, mm6			// *64, for rounding later
		movq mm2, mm0			// copy so mm0 is stored for later
		movq mm3, mm1			// copy so mm1 is stored for later
		pmulhw mm2, fact_YU		// YU factor (U term in adjusted Y)
		pmulhw mm3, fact_YV		// YV factor (V term in adjusted Y)
		paddw mm2, mm3			// total adjusted amount to add to Y
		movq mm3, mm2			// make copy
		movq mm4, [esi]			// 8 Y bytes from curr line
		punpcklwd mm2, mm2		// words <1,1,0,0>
		punpckhwd mm3, mm3		// words <3,3,2,2>
		movq mm5, mm4			// make copy of it	
		punpcklbw mm4, mm7      // 0Y0Y0Y0Y
		punpckhbw mm5, mm7		// 0Y0Y0Y0Y
		pmullw mm4, mm6			// *64
		pmullw mm5, mm6			// *64
		psubsw mm4, mm2			// subtract uv adjustment (both coefficients are negative)
		psubsw mm5, mm3			// subtract uv adjustment (both coefficients are negative)
		paddw mm4, Q32			// bump up 32 for rounding
		paddw mm5, Q32			// bump up 32 for rounding
		psraw mm4, 6			// /64
		psraw mm5, 6			// /64
		packuswb mm4, mm5		// pack back to 8 bytes, saturate to 0-255
		movq [edi], mm4			// store new curr line Y bytes
		add esi, src_pitchR		// bump to next line
		add	edi, dst_pitchR		// bump to next line
		movq mm4, [esi]			// 8 Y bytes from next line
		movq mm5, mm4			// make copy of it
		punpcklbw mm4, mm7		// 0Y0Y0Y0Y
		punpckhbw mm5, mm7		// 0Y0Y0Y0Y
		pmullw mm4, mm6			// *64
		pmullw mm5, mm6			// *64
		psubsw mm4, mm2			// add uv adjustment
		psubsw mm5, mm3			// add uv adjustment
		paddw mm4, Q32			// bump up 32 for rounding
		paddw mm5, Q32			// bump up 32 for rounding
		psraw mm4, 6			// /64
		psraw mm5, 6			// /64
		packuswb mm4, mm5		// pack back to 8 bytes, saturate to 0-255
		movq [edi], mm4			// store new next line Y bytes
		sub	esi, src_pitchR		// restore curr line
		sub	edi, dst_pitchR		// restore curr line
		movq mm2, mm0			// mov back stored U words
		movq mm3, mm1			// mov back stored V words
		paddw mm2, mm2			// fact_UU is scaled down by 4 so adjust
		paddw mm2, mm2			// fact_UU is scaled down by 4 so adjust
		pmulhw mm2, fact_UU		// UU factor (U term in adjusted U)
		pmulhw mm3, fact_UV		// UV factor (V term in adjusted U)
		paddw mm2, mm3			// this is new U
		paddw mm2, Q8224		// bias up by 64*128 + 32
		psraw mm2, 6			// /64
		packuswb mm2, mm7		// back to 4 bytes 
		movd [ebx], mm2			// store adjusted U
		movq mm2, mm0			// mov back stored U words
		movq mm3, mm1			// mov back stored V words
		paddw mm3, mm3			// fact_VV is scaled down by 4 so adjust
		paddw mm3, mm3			// fact_VV is scaled down by 4 so adjust
		pmulhw mm2, fact_VU		// VU factor (U term in adjusted V)
		pmulhw mm3, fact_VV		// VV factor (V term in adjusted V)
		paddw mm3, mm2			// 1st term negative, this is new V 
		paddw mm3, Q8224		// bias up by 64*128 + 32
		psraw mm3, 6			// /64
		packuswb mm3, mm7		// back to 4 bytes 
		movd [edx], mm3			// store adjusted V
		add esi, 8				// bump ptrs
		add edi, 8
		add eax, 4
		add ebx, 4
		add ecx, 4
		add edx, 4
		dec loopctr				// decrease counter
		jnz xloop				// loop
		sub height, 2
		jz end
		mov eax, width
		mov esi, srcpY
		shr eax, 3
		mov edi, dstpY
		mov loopctr, eax
		mov ebx, dstpU
		mov eax, srcpU
		mov edx, dstpV
		mov ecx, srcpV
		add esi, src_pitchY2
		add edi, dst_pitchY2
		add eax, src_pitchUV
		add ebx, dst_pitchUV
		add ecx, src_pitchUV
		add edx, dst_pitchUV
		mov srcpY, esi
		mov dstpY, edi
		mov dstpU, ebx
		mov dstpV, edx
		mov srcpU, eax
		mov srcpV, ecx
		jmp xloop
end:
		emms
	}
}

void conv3_YV12_MMX(void *ps)
{
	GETPTRS();
	GETMMXVS();
	__asm
	{
		mov esi, srcpY
		mov edi, dstpY
		mov eax, srcpU
		mov ecx, srcpV
		mov edx, dstpV
		mov ebx, dstpU
		pxor mm7, mm7			// all 0's
		movq mm6, Q64			// 64's words
		align 16
xloop:
		movd mm0, [eax]			// move 1st 4 U bytes
		movd mm1, [ecx]			// move 1st 4 V bytes
		punpcklbw mm0, mm7		// unpack to words, 0U0U0U0U
		punpcklbw mm1, mm7		// unpack to words, 0V0V0V0V
		psubw mm0, Q128			// adj for 128 chroma offset
		psubw mm1, Q128			// adj for 128 chroma offset
		pmullw mm0, mm6			// *64, for rounding later
		pmullw mm1, mm6			// *64, for rounding later
		movq mm2, mm0			// copy so mm0 is stored for later
		movq mm3, mm1			// copy so mm1 is stored for later
		pmulhw mm2, fact_YU		// YU factor (U term in adjusted Y)
		pmulhw mm3, fact_YV		// YV factor (V term in adjusted Y)
		paddw mm2, mm3			// (total adjusted amount)*64 to add to Y
		movq mm3, mm2			// make copy
		movq mm4, [esi]			// 8 Y bytes from curr line
		punpcklwd mm2, mm2		// words <1,1,0,0>
		punpckhwd mm3, mm3		// words <3,3,2,2>
		movq mm5, mm4			// make copy of it	
		punpcklbw mm4, mm7      // 0Y0Y0Y0Y
		punpckhbw mm5, mm7		// 0Y0Y0Y0Y
		pmullw mm4, mm6			// *64
		pmullw mm5, mm6			// *64
		paddw mm4, mm2			// add uv adjustment
		paddw mm5, mm3			// add uv adjustment
		paddw mm4, Q32			// bump up 32 for rounding
		paddw mm5, Q32			// bump up 32 for rounding
		psraw mm4, 6			// /64
		psraw mm5, 6			// /64
		packuswb mm4, mm5		// pack back to 8 bytes, saturate to 0-255
		movq [edi], mm4			// store new curr line Y bytes
		add esi, src_pitchR		// bump to next line
		add	edi, dst_pitchR		// bump to next line
		movq mm4, [esi]			// 8 Y bytes from next line
		movq mm5, mm4			// make copy of it
		punpcklbw mm4, mm7		// 0Y0Y0Y0Y
		punpckhbw mm5, mm7		// 0Y0Y0Y0Y
		pmullw mm4, mm6			// *64
		pmullw mm5, mm6			// *64
		paddw mm4, mm2			// add uv adjustment
		paddw mm5, mm3			// add uv adjustment
		paddw mm4, Q32			// bump up 32 for rounding
		paddw mm5, Q32			// bump up 32 for rounding
		psraw mm4, 6			// /64
		psraw mm5, 6			// /64
		packuswb mm4, mm5		// pack back to 8 bytes, saturate to 0-255
		movq [edi], mm4			// store new next line Y bytes
		sub	esi, src_pitchR		// restore curr line
		sub	edi, dst_pitchR		// restore curr line
		movq mm2, mm0			// mov back stored U words
		movq mm3, mm1			// mov back stored V words
		paddw mm2, mm2			// adjust for /2 scale in fact_UU
		pmulhw mm2, fact_UU		// UU factor (U term in adjusted U)
		pmulhw mm3, fact_UV		// UV factor (V term in adjusted U)
		paddw mm2, mm3			// add
		paddw mm2, Q8224		// bias up by 64*128 + 32
		psraw mm2, 6			// /64
		packuswb mm2, mm7		// back to 4 bytes 
		movd [ebx], mm2			// store adjusted U
		movq mm2, mm0			// mov back stored U words
		movq mm3, mm1			// mov back stored V words
		paddw mm3, mm3			// adjust for /2 scale in fact_VV
		pmulhw mm3, fact_VV		// VV factor (V term in adjusted V)
		pmulhw mm2, fact_VU		// VU factor (U term in adjusted V)
		psubsw mm3, mm2			// 1st term negative, this is new V 
		paddw mm3, Q8224		// bias up by 64*128 + 64
		psraw mm3, 6			// /64
		packuswb mm3, mm7		// back to 4 bytes 
		movd [edx], mm3			// store adjusted V
		add esi, 8				// bump ptrs
		add edi, 8
		add eax, 4
		add ebx, 4
		add ecx, 4
		add edx, 4
		dec loopctr				// decrease counter
		jnz xloop				// loop
		sub height, 2
		jz end
		mov eax, width
		mov esi, srcpY
		shr eax, 3
		mov edi, dstpY
		mov loopctr, eax
		mov ebx, dstpU
		mov eax, srcpU
		mov edx, dstpV
		mov ecx, srcpV
		add esi, src_pitchY2
		add edi, dst_pitchY2
		add eax, src_pitchUV
		add ebx, dst_pitchUV
		add ecx, src_pitchUV
		add edx, dst_pitchUV
		mov srcpY, esi
		mov dstpY, edi
		mov dstpU, ebx
		mov dstpV, edx
		mov srcpU, eax
		mov srcpV, ecx
		jmp xloop
end:
		emms
	}
}

void conv4_YV12_MMX(void *ps)
{
	GETPTRS();
	GETMMXVS();
	__asm
	{
		mov esi, srcpY
		mov edi, dstpY
		mov eax, srcpU
		mov ebx, dstpU
		mov ecx, srcpV
		mov edx, dstpV
		pxor mm7, mm7			// all 0's
		movq mm6, Q64			// 64's words
		align 16
xloop:
		movd mm0, [eax]			// move 1st 4 U bytes
		movd mm1, [ecx]			// move 1st 4 V bytes
		punpcklbw mm0, mm7		// unpack to words, 0U0U0U0U
		punpcklbw mm1, mm7		// unpack to words, 0V0V0V0V
		psubw mm0, Q128			// adj for 128 chroma offset
		psubw mm1, Q128			// adj for 128 chroma offset
		pmullw mm0, mm6			// *64, for rounding later			
		pmullw mm1, mm6			// *64, for rounding later
		movq mm2, mm0			// copy so mm0 is stored for later
		movq mm3, mm1			// copy so mm1 is stored for later
		pmulhw mm2, fact_YU		// YU factor (U term in adjusted Y)
		pmulhw mm3, fact_YV		// YV factor (V term in adjusted Y)
		paddw mm2, mm3			// total adjusted amount to add to Y
		movq mm3, mm2			// make copy
		movq mm4, [esi]			// 8 Y bytes from curr line
		punpcklwd mm2, mm2		// words <1,1,0,0>
		punpckhwd mm3, mm3		// words <3,3,2,2>
		movq mm5, mm4			// make copy of it	
		punpcklbw mm4, mm7      // 0Y0Y0Y0Y
		punpckhbw mm5, mm7		// 0Y0Y0Y0Y
		pmullw mm4, mm6			// *64
		pmullw mm5, mm6			// *64
		psubsw mm4, mm2			// subtract uv adjustment (both coefficients are negative)
		psubsw mm5, mm3			// subtract uv adjustment (both coefficients are negative)
		paddw mm4, Q32			// bump up 32 for rounding
		paddw mm5, Q32			// bump up 32 for rounding
		psraw mm4, 6			// /64
		psraw mm5, 6			// /64
		packuswb mm4, mm5		// pack back to 8 bytes, saturate to 0-255
		movq [edi], mm4			// store new curr line Y bytes
		add esi, src_pitchR		// bump to next line
		add	edi, dst_pitchR		// bump to next line
		movq mm4, [esi]			// 8 Y bytes from next line
		movq mm5, mm4			// make copy of it
		punpcklbw mm4, mm7		// 0Y0Y0Y0Y
		punpckhbw mm5, mm7		// 0Y0Y0Y0Y
		pmullw mm4, mm6			// *64
		pmullw mm5, mm6			// *64
		psubsw mm4, mm2			// add uv adjustment
		psubsw mm5, mm3			// add uv adjustment
		paddw mm4, Q32			// bump up 32 for rounding
		paddw mm5, Q32			// bump up 32 for rounding
		psraw mm4, 6			// /64
		psraw mm5, 6			// /64
		packuswb mm4, mm5		// pack back to 8 bytes, saturate to 0-255
		movq [edi], mm4			// store new next line Y bytes
		sub	esi, src_pitchR		// restore curr line
		sub	edi, dst_pitchR		// restore curr line
		movq mm2, mm0			// mov back stored U words
		movq mm3, mm1			// mov back stored V words
		paddw mm2, mm2			// fact_UU is scaled down by 4 so adjust
		paddw mm2, mm2			// fact_UU is scaled down by 4 so adjust
		pmulhw mm2, fact_UU		// UU factor (U term in adjusted U)
		pmulhw mm3, fact_UV		// UV factor (V term in adjusted U)
		psubsw mm2, mm3			// this is new U
		paddw mm2, Q8224		// bias up by 64*128 + 32
		psraw mm2, 6			// /64
		packuswb mm2, mm7		// back to 4 bytes 
		movd [ebx], mm2			// store adjusted U
		movq mm2, mm0			// mov back stored U words
		movq mm3, mm1			// mov back stored V words
		paddw mm3, mm3			// fact_VV is scaled down by 4 so adjust
		paddw mm3, mm3			// fact_VV is scaled down by 4 so adjust
		pmulhw mm2, fact_VU		// VU factor (U term in adjusted V)
		pmulhw mm3, fact_VV		// VV factor (V term in adjusted V)
		paddw mm3, mm2			// 1st term negative, this is new V 
		paddw mm3, Q8224		// bias up by 64*128 + 32
		psraw mm3, 6			// /64
		packuswb mm3, mm7		// back to 4 bytes 
		movd [edx], mm3			// store adjusted V
		add esi, 8				// bump ptrs
		add edi, 8
		add eax, 4
		add ebx, 4
		add ecx, 4
		add edx, 4
		dec loopctr				// decrease counter
		jnz xloop				// loop
		sub height, 2
		jz end
		mov eax, width
		mov esi, srcpY
		shr eax, 3
		mov edi, dstpY
		mov loopctr, eax
		mov ebx, dstpU
		mov eax, srcpU
		mov edx, dstpV
		mov ecx, srcpV
		add esi, src_pitchY2
		add edi, dst_pitchY2
		add eax, src_pitchUV
		add ebx, dst_pitchUV
		add ecx, src_pitchUV
		add edx, dst_pitchUV
		mov srcpY, esi
		mov dstpY, edi
		mov dstpU, ebx
		mov dstpV, edx
		mov srcpU, eax
		mov srcpV, ecx
		jmp xloop
end:
		emms
	}
}

// These sse2 routines use mm0/mm1 to stash the dstpU pointer since
// ebx is needed to track the aligned variables on the stack.

void conv1_YV12_SSE2(void *ps)
{
	GETPTRS();
	GETSSE2VS();
	__asm
	{
		mov esi, srcpY
		mov edi, dstpY
		mov eax, srcpU
		movd mm0, dstpU
		mov ecx, srcpV
		mov edx, dstpV
		pxor xmm7, xmm7				// all 0's
		movdqa xmm6, Q64			// 64's words
		align 16
xloop:
		movq xmm0, qword ptr[eax]
		movq xmm1, qword ptr[ecx]
		punpcklbw xmm0, xmm7		// unpack to words, 0U0U0U0U
		punpcklbw xmm1, xmm7		// unpack to words, 0V0V0V0V
		psubw xmm0, Q128			// adj for 128 chroma offset
		psubw xmm1, Q128			// adj for 128 chroma offset
		pmullw xmm0, xmm6			// *64, for rounding later			
		pmullw xmm1, xmm6			// *64, for rounding later
		movdqa xmm2, xmm0			// copy so mm0 is stored for later
		movdqa xmm3, xmm1			// copy so mm1 is stored for later
		pmulhw xmm2, fact_YU		// YU factor (U term in adjusted Y)
		pmulhw xmm3, fact_YV		// YV factor (V term in adjusted Y)
		paddw xmm2, xmm3			// total adjusted amount to add to Y
		movdqa xmm4, [esi]
		movdqa xmm3, xmm2			// make copy
		punpcklwd xmm2, xmm2		// words <1,1,0,0>
		punpckhwd xmm3, xmm3		// words <3,3,2,2>
		movdqa xmm5, xmm4			// make copy of it
		punpcklbw xmm4, xmm7		// 0Y0Y0Y0Y
		punpckhbw xmm5, xmm7		// 0Y0Y0Y0Y
		pmullw xmm4, xmm6			// *64
		pmullw xmm5, xmm6			// *64
		paddw xmm4, xmm2			// add uv adjustment
		paddw xmm5, xmm3			// add uv adjustment
		paddw xmm4, Q32				// bump up 32 for rounding
		paddw xmm5, Q32				// bump up 32 for rounding
		psraw xmm4, 6				// /64
		psraw xmm5, 6				// /64
		packuswb xmm4, xmm5			// pack back to 8 bytes, saturate to 0-255
		movdqa [edi], xmm4
		add esi, src_pitchR
		add	edi, dst_pitchR
		movdqa xmm4, [esi]
		movdqa xmm5, xmm4			// make copy of it
		punpcklbw xmm4, xmm7		// 0Y0Y0Y0Y
		punpckhbw xmm5, xmm7		// 0Y0Y0Y0Y
		pmullw xmm4, xmm6			// *64
		pmullw xmm5, xmm6			// *64
		paddw xmm4, xmm2			// add uv adjustment
		paddw xmm5, xmm3			// add uv adjustment
		paddw xmm4, Q32				// bump up 32 for rounding
		paddw xmm5, Q32				// bump up 32 for rounding
		psraw xmm4, 6				// /64
		psraw xmm5, 6				// /64
		packuswb xmm4, xmm5			// pack back to 8 bytes, saturate to 0-255
		movdqa [edi], xmm4
		sub	esi, src_pitchR			// restore curr line
		sub	edi, dst_pitchR			// restore curr line
		movdqa xmm2, xmm0			// mov back stored U words
		movdqa xmm3, xmm1			// mov back stored V words
		paddw xmm2, xmm2			// adjust for /2 scale in fact_UU
		pmulhw xmm2, fact_UU		// UU factor (U term in adjusted U)
		pmulhw xmm3, fact_UV		// UV factor (V term in adjusted U)
		psubsw xmm2, xmm3			// this is new U
		movd mm1, eax
		paddw xmm2, Q8224			// bias up by 64*128 + 32
		psraw xmm2, 6				// /64
		movd eax, mm0
		packuswb xmm2, xmm7			// back to 4 bytes 
		movq qword ptr[eax], xmm2	// store adjusted U
		movdqa xmm2, xmm0			// mov back stored U words
		movdqa xmm3, xmm1			// mov back stored V words
		movd eax, mm1
		paddw xmm3, xmm3			// adjust for /2 scale in fact_VV
		pmulhw xmm2, fact_VU		// VU factor (U term in adjusted V)
		pmulhw xmm3, fact_VV		// VV factor (V term in adjusted V)
		psubsw xmm3, xmm2			// 1st term negative, this is new V 
		paddw xmm3, Q8224			// bias up by 64*128 + 32
		psraw xmm3, 6				// /64
		packuswb xmm3, xmm7			// pack to 4 bytes 
		movq qword ptr[edx], xmm3	// store adjusted V
		add esi, 16					// bump ptrs
		add edi, 16
		add eax, 8
		add ecx, 8
		paddd mm0, Q8
		add edx, 8
		dec loopctr					// decrease counter
		jnz xloop					// loop
		sub height, 2
		jz end
		mov eax, width
		mov esi, srcpY
		shr eax, 4
		mov edi, dstpY
		mov loopctr, eax
		mov eax, srcpU
		mov edx, dstpV
		mov ecx, srcpV
		add esi, src_pitchY2
		add edi, dst_pitchY2
		add eax, src_pitchUV
		add ecx, src_pitchUV
		add edx, dst_pitchUV
		mov srcpY, esi
		mov dstpY, edi
		mov dstpV, edx
		mov srcpU, eax
		mov srcpV, ecx
		movd mm1, eax
		mov eax, dstpU
		add eax, dst_pitchUV
		mov dstpU, eax
		movd mm0, eax
		movd eax, mm1
		jmp xloop
end:
		emms
	}
}

void conv2_YV12_SSE2(void *ps)
{
	GETPTRS();
	GETSSE2VS();
	__asm
	{
		mov esi, srcpY
		mov edi, dstpY
		mov eax, srcpU
		movd mm0, dstpU
		mov ecx, srcpV
		mov edx, dstpV
		pxor xmm7, xmm7				// all 0's
		movdqa xmm6, Q64			// 64's words
		align 16
xloop:
		movq xmm0, qword ptr[eax]
		movq xmm1, qword ptr[ecx]
		punpcklbw xmm0, xmm7		// unpack to words, 0U0U0U0U
		punpcklbw xmm1, xmm7		// unpack to words, 0V0V0V0V
		psubw xmm0, Q128			// adj for 128 chroma offset
		psubw xmm1, Q128			// adj for 128 chroma offset
		pmullw xmm0, xmm6			// *64, for rounding later			
		pmullw xmm1, xmm6			// *64, for rounding later
		movdqa xmm2, xmm0			// copy so xmm0 is stored for later
		movdqa xmm3, xmm1			// copy so xmm1 is stored for later
		pmulhw xmm2, fact_YU		// YU factor (U term in adjusted Y)
		pmulhw xmm3, fact_YV		// YV factor (V term in adjusted Y)
		paddw xmm2, xmm3			// total adjusted amount to add to Y
		movdqa xmm4, [esi]
		movdqa xmm3, xmm2			// make copy
		punpcklwd xmm2, xmm2		// words <1,1,0,0>
		punpckhwd xmm3, xmm3		// words <3,3,2,2>
		movdqa xmm5, xmm4			// make copy of it
		punpcklbw xmm4, xmm7		// 0Y0Y0Y0Y
		punpckhbw xmm5, xmm7		// 0Y0Y0Y0Y
		pmullw xmm4, xmm6			// *64
		pmullw xmm5, xmm6			// *64
		psubsw xmm4, xmm2			// subtract uv adjustment (both coefficients are negative)
		psubsw xmm5, xmm3			// subtract uv adjustment (both coefficients are negative)
		paddw xmm4, Q32				// bump up 32 for rounding
		paddw xmm5, Q32				// bump up 32 for rounding
		psraw xmm4, 6				// /64
		psraw xmm5, 6				// /64
		packuswb xmm4, xmm5			// pack back to 8 bytes, saturate to 0-255
		movdqa [edi], xmm4
		add esi, src_pitchR
		add	edi, dst_pitchR
		movdqa xmm4, [esi]
		movdqa xmm5, xmm4			// make copy of it
		punpcklbw xmm4, xmm7		// 0Y0Y0Y0Y
		punpckhbw xmm5, xmm7		// 0Y0Y0Y0Y
		pmullw xmm4, xmm6			// *64
		pmullw xmm5, xmm6			// *64
		psubsw xmm4, xmm2			// add uv adjustment
		psubsw xmm5, xmm3			// add uv adjustment
		paddw xmm4, Q32				// bump up 32 for rounding
		paddw xmm5, Q32				// bump up 32 for rounding
		psraw xmm4, 6				// /64
		psraw xmm5, 6				// /64
		packuswb xmm4, xmm5			// pack back to 8 bytes, saturate to 0-255
		movdqa [edi], xmm4
		sub	esi, src_pitchR			// restore curr line
		sub	edi, dst_pitchR			// restore curr line
		movdqa xmm2, xmm0			// mov back stored U words
		movdqa xmm3, xmm1			// mov back stored V words
		paddw xmm2, xmm2			// fact_UU is scaled down by 4 so adjust
		paddw xmm2, xmm2			// fact_UU is scaled down by 4 so adjust
		pmulhw xmm2, fact_UU		// UU factor (U term in adjusted U)
		pmulhw xmm3, fact_UV		// UV factor (V term in adjusted U)
		paddw xmm2, xmm3			// this is new U
		movd mm1, eax
		paddw xmm2, Q8224			// bias up by 64*128 + 32
		psraw xmm2, 6				// /64
		movd eax, mm0
		packuswb xmm2, xmm7			// pack to 4 bytes
		movq qword ptr[eax], xmm2	// store adjusted U
		movdqa xmm2, xmm0			// mov back stored U words
		movdqa xmm3, xmm1			// mov back stored V words
		paddw xmm3, xmm3			// fact_VV is scaled down by 4 so adjust
		movd eax, mm1
		paddw xmm3, xmm3			// fact_VV is scaled down by 4 so adjust
		pmulhw xmm2, fact_VU		// VU factor (U term in adjusted V)
		pmulhw xmm3, fact_VV		// VV factor (V term in adjusted V)
		paddw xmm3, xmm2			// 1st term negative, this is new V 
		paddw xmm3, Q8224			// bias up by 64*128 + 32
		psraw xmm3, 6				// /64
		packuswb xmm3, xmm7			// pack to 4 bytes 
		movq qword ptr[edx], xmm3	// store adjusted V
		add esi, 16					// bump ptrs
		add edi, 16
		add eax, 8
		paddd mm0, Q8
		add ecx, 8
		add edx, 8
		dec loopctr					// decrease counter
		jnz xloop					// loop
		sub height, 2
		jz end
		mov eax, width
		mov esi, srcpY
		shr eax, 4
		mov edi, dstpY
		mov loopctr, eax
		mov eax, srcpU
		mov edx, dstpV
		mov ecx, srcpV
		add esi, src_pitchY2
		add edi, dst_pitchY2
		add eax, src_pitchUV
		add ecx, src_pitchUV
		add edx, dst_pitchUV
		mov srcpY, esi
		mov dstpY, edi
		mov dstpV, edx
		mov srcpU, eax
		mov srcpV, ecx
		movd mm1, eax
		mov eax, dstpU
		add eax, dst_pitchUV
		mov dstpU, eax
		movd mm0, eax
		movd eax, mm1
		jmp xloop
end:
		emms
	}
}

void conv3_YV12_SSE2(void *ps)
{
	GETPTRS();
	GETSSE2VS();
	__asm
	{
		mov esi, srcpY
		mov edi, dstpY
		mov eax, srcpU
		movd mm0, dstpU
		mov ecx, srcpV
		mov edx, dstpV
		pxor xmm7, xmm7				// all 0's
		movdqa xmm6, Q64			// 64's words
		align 16
xloop:
		movq xmm0, qword ptr[eax]
		movq xmm1, qword ptr[ecx]
		punpcklbw xmm0, xmm7		// unpack to words, 0U0U0U0U
		punpcklbw xmm1, xmm7		// unpack to words, 0V0V0V0V
		psubw xmm0, Q128			// adj for 128 chroma offset
		psubw xmm1, Q128			// adj for 128 chroma offset
		pmullw xmm0, xmm6			// *64, for rounding later			
		pmullw xmm1, xmm6			// *64, for rounding later
		movdqa xmm2, xmm0			// copy so mm0 is stored for later
		movdqa xmm3, xmm1			// copy so mm1 is stored for later
		pmulhw xmm2, fact_YU		// YU factor (U term in adjusted Y)
		pmulhw xmm3, fact_YV		// YV factor (V term in adjusted Y)
		paddw xmm2, xmm3			// total adjusted amount to add to Y
		movdqa xmm4, [esi]
		movdqa xmm3, xmm2			// make copy
		punpcklwd xmm2, xmm2		// words <1,1,0,0>
		punpckhwd xmm3, xmm3		// words <3,3,2,2>
		movdqa xmm5, xmm4			// make copy of it
		punpcklbw xmm4, xmm7		// 0Y0Y0Y0Y
		punpckhbw xmm5, xmm7		// 0Y0Y0Y0Y
		pmullw xmm4, xmm6			// *64
		pmullw xmm5, xmm6			// *64
		paddw xmm4, xmm2			// add uv adjustment
		paddw xmm5, xmm3			// add uv adjustment
		paddw xmm4, Q32				// bump up 32 for rounding
		paddw xmm5, Q32				// bump up 32 for rounding
		psraw xmm4, 6				// /64
		psraw xmm5, 6				// /64
		packuswb xmm4, xmm5			// pack back to 8 bytes, saturate to 0-255
		movdqa [edi], xmm4
		add esi, src_pitchR
		add	edi, dst_pitchR
		movdqa xmm4, [esi]
		movdqa xmm5, xmm4			// make copy of it
		punpcklbw xmm4, xmm7		// 0Y0Y0Y0Y
		punpckhbw xmm5, xmm7		// 0Y0Y0Y0Y
		pmullw xmm4, xmm6			// *64
		pmullw xmm5, xmm6			// *64
		paddw xmm4, xmm2			// add uv adjustment
		paddw xmm5, xmm3			// add uv adjustment
		paddw xmm4, Q32				// bump up 32 for rounding
		paddw xmm5, Q32				// bump up 32 for rounding
		psraw xmm4, 6				// /64
		psraw xmm5, 6				// /64
		packuswb xmm4, xmm5			// pack back to 8 bytes, saturate to 0-255
		movdqa [edi], xmm4
		sub	esi, src_pitchR			// restore curr line
		sub	edi, dst_pitchR			// restore curr line
		movdqa xmm2, xmm0			// mov back stored U words
		movdqa xmm3, xmm1			// mov back stored V words
		paddw xmm2, xmm2			// adjust for /2 scale in fact_UU
		pmulhw xmm2, fact_UU		// UU factor (U term in adjusted U)
		pmulhw xmm3, fact_UV		// UV factor (V term in adjusted U)
		paddw xmm2, xmm3			// add
		movd mm1, eax
		paddw xmm2, Q8224			// bias up by 64*128 + 32
		psraw xmm2, 6				// /64
		movd eax, mm0
		packuswb xmm2, xmm7			// back to 4 bytes 
		movq qword ptr[eax], xmm2	// store adjusted U
		movdqa xmm2, xmm0			// mov back stored U words
		movdqa xmm3, xmm1			// mov back stored V words
		movd eax, mm1
		paddw xmm3, xmm3			// adjust for /2 scale in fact_VV
		pmulhw xmm2, fact_VU		// VU factor (U term in adjusted V)
		pmulhw xmm3, fact_VV		// VV factor (V term in adjusted V)
		psubsw xmm3, xmm2			// 1st term negative, this is new V 
		paddw xmm3, Q8224			// bias up by 64*128 + 32
		psraw xmm3, 6				// /64
		packuswb xmm3, xmm7			// pack to 4 bytes 
		movq qword ptr[edx], xmm3	// store adjusted V
		add esi, 16					// bump ptrs
		add edi, 16
		add eax, 8
		paddd mm0, Q8
		add ecx, 8
		add edx, 8
		dec loopctr					// decrease counter
		jnz xloop					// loop
		sub height, 2
		jz end
		mov eax, width
		mov esi, srcpY
		shr eax, 4
		mov edi, dstpY
		mov loopctr, eax
		mov eax, srcpU
		mov edx, dstpV
		mov ecx, srcpV
		add esi, src_pitchY2
		add edi, dst_pitchY2
		add eax, src_pitchUV
		add ecx, src_pitchUV
		add edx, dst_pitchUV
		mov srcpY, esi
		mov dstpY, edi
		mov dstpV, edx
		mov srcpU, eax
		mov srcpV, ecx
		movd mm1, eax
		mov eax, dstpU
		add eax, dst_pitchUV
		mov dstpU, eax
		movd mm0, eax
		movd eax, mm1
		jmp xloop
end:
		emms
	}
}

void conv4_YV12_SSE2(void *ps)
{
	GETPTRS();
	GETSSE2VS();
	__asm
	{
		mov esi, srcpY
		mov edi, dstpY
		mov eax, srcpU
		movd mm0, dstpU
		mov ecx, srcpV
		mov edx, dstpV
		pxor xmm7, xmm7				// all 0's
		movdqa xmm6, Q64			// 64's words
		align 16
xloop:
		movq xmm0, qword ptr[eax]
		movq xmm1, qword ptr[ecx]
		punpcklbw xmm0, xmm7		// unpack to words, 0U0U0U0U
		punpcklbw xmm1, xmm7		// unpack to words, 0V0V0V0V
		psubw xmm0, Q128			// adj for 128 chroma offset
		psubw xmm1, Q128			// adj for 128 chroma offset
		pmullw xmm0, xmm6			// *64, for rounding later			
		pmullw xmm1, xmm6			// *64, for rounding later
		movdqa xmm2, xmm0			// copy so xmm0 is stored for later
		movdqa xmm3, xmm1			// copy so xmm1 is stored for later
		pmulhw xmm2, fact_YU		// YU factor (U term in adjusted Y)
		pmulhw xmm3, fact_YV		// YV factor (V term in adjusted Y)
		paddw xmm2, xmm3			// total adjusted amount to add to Y
		movdqa xmm4, [esi]
		movdqa xmm3, xmm2			// make copy
		punpcklwd xmm2, xmm2		// words <1,1,0,0>
		punpckhwd xmm3, xmm3		// words <3,3,2,2>
		movdqa xmm5, xmm4			// make copy of it
		punpcklbw xmm4, xmm7		// 0Y0Y0Y0Y
		punpckhbw xmm5, xmm7		// 0Y0Y0Y0Y
		pmullw xmm4, xmm6			// *64
		pmullw xmm5, xmm6			// *64
		psubsw xmm4, xmm2			// subtract uv adjustment (both coefficients are negative)
		psubsw xmm5, xmm3			// subtract uv adjustment (both coefficients are negative)
		paddw xmm4, Q32				// bump up 32 for rounding
		paddw xmm5, Q32				// bump up 32 for rounding
		psraw xmm4, 6				// /64
		psraw xmm5, 6				// /64
		packuswb xmm4, xmm5			// pack back to 8 bytes, saturate to 0-255
		movdqa [edi], xmm4
		add esi, src_pitchR
		add	edi, dst_pitchR
		movdqa xmm4, [esi]
		movdqa xmm5, xmm4			// make copy of it
		punpcklbw xmm4, xmm7		// 0Y0Y0Y0Y
		punpckhbw xmm5, xmm7		// 0Y0Y0Y0Y
		pmullw xmm4, xmm6			// *64
		pmullw xmm5, xmm6			// *64
		psubsw xmm4, xmm2			// add uv adjustment
		psubsw xmm5, xmm3			// add uv adjustment
		paddw xmm4, Q32				// bump up 32 for rounding
		paddw xmm5, Q32				// bump up 32 for rounding
		psraw xmm4, 6				// /64
		psraw xmm5, 6				// /64
		packuswb xmm4, xmm5			// pack back to 8 bytes, saturate to 0-255
		movdqa [edi], xmm4
		sub	esi, src_pitchR			// restore curr line
		sub	edi, dst_pitchR			// restore curr line
		movdqa xmm2, xmm0			// mov back stored U words
		movdqa xmm3, xmm1			// mov back stored V words
		paddw xmm2, xmm2			// fact_UU is scaled down by 4 so adjust
		paddw xmm2, xmm2			// fact_UU is scaled down by 4 so adjust
		pmulhw xmm2, fact_UU		// UU factor (U term in adjusted U)
		pmulhw xmm3, fact_UV		// UV factor (V term in adjusted U)
		psubsw xmm2, xmm3			// this is new U
		movd mm1, eax
		paddw xmm2, Q8224			// bias up by 64*128 + 32
		psraw xmm2, 6				// /64
		movd eax, mm0
		packuswb xmm2, xmm7			// pack to 4 bytes
		movq qword ptr[eax], xmm2	// store adjusted U
		movdqa xmm2, xmm0			// mov back stored U words
		movdqa xmm3, xmm1			// mov back stored V words
		movd eax, mm1
		paddw xmm3, xmm3			// fact_VV is scaled down by 4 so adjust
		paddw xmm3, xmm3			// fact_VV is scaled down by 4 so adjust
		pmulhw xmm2, fact_VU		// VU factor (U term in adjusted V)
		pmulhw xmm3, fact_VV		// VV factor (V term in adjusted V)
		paddw xmm3, xmm2			// 1st term negative, this is new V 
		paddw xmm3, Q8224			// bias up by 64*128 + 32
		psraw xmm3, 6				// /64
		packuswb xmm3, xmm7			// pack to 4 bytes 
		movq qword ptr[edx], xmm3	// store adjusted V
		add esi, 16					// bump ptrs
		add edi, 16
		add eax, 8
		paddd mm0, Q8
		add ecx, 8
		add edx, 8
		dec loopctr					// decrease counter
		jnz xloop					// loop
		sub height, 2
		jz end
		mov eax, width
		mov esi, srcpY
		shr eax, 4
		mov edi, dstpY
		mov loopctr, eax
		mov eax, srcpU
		mov edx, dstpV
		mov ecx, srcpV
		add esi, src_pitchY2
		add edi, dst_pitchY2
		add eax, src_pitchUV
		add ecx, src_pitchUV
		add edx, dst_pitchUV
		mov srcpY, esi
		mov dstpY, edi
		mov dstpV, edx
		mov srcpU, eax
		mov srcpV, ecx
		movd mm1, eax
		mov eax, dstpU
		add eax, dst_pitchUV
		mov dstpU, eax
		movd mm0, eax
		movd eax, mm1
		jmp xloop
end:
		emms
	}
}