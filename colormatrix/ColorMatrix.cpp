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

ColorMatrix::ColorMatrix(PClip _child, const char* _mode, int _source, int _dest, int _clamp, 
	bool _interlaced, bool _inputFR, bool _outputFR, bool _hints, const char* _d2v, bool _debug, 
	int _threads, int _thrdmthd, int _opt, IScriptEnvironment* env) : GenericVideoFilter(_child), 
	mode(_mode), source(_source), dest(_dest), clamp(_clamp), interlaced(_interlaced), 
	inputFR(_inputFR), outputFR(_outputFR), hints(_hints), d2v(_d2v), debug(_debug), 
	threads(_threads), thrdmthd(_thrdmthd), opt(_opt)
{
	d2vArray = NULL;
	tids = NULL;
	thds = NULL;
	pssInfo = NULL;
	if (*d2v && hints)
		env->ThrowError("ColorMatrix:  hints and d2v input cannot be used at the same time!");
	if (!vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("ColorMatrix:  input to filter must be YV12 or YUY2!");
	if (clamp < 0 || clamp > 3)
		env->ThrowError("ColorMatrix:  clamp must be set to 0, 1, 2, or 3!");
	if (opt < 0 || opt > 3)
		env->ThrowError("ColorMatrix:  opt must be set to 0, 1, 2, or 3!");
	if (threads < 0)
		env->ThrowError("ColorMatrix:  threads must greater than or equal to 0!");
	if ((vi.IsYUY2() && threads > vi.height) || (vi.IsYV12() && threads > vi.height/2))
		env->ThrowError("ColorMatrix:  cannot use more than %d threads on this clip!", 
			vi.IsYUY2() ? vi.height : vi.height/2);
	if (thrdmthd < 0 || thrdmthd > 1)
		env->ThrowError("ColorMatrix:  thrdmthd must be set to 0 or 1!");
	long cpu = env->GetCPUFlags();
	if (opt != 3)
	{
		if (opt == 0) cpu &= ~0x2C;
		else if (opt == 1) { cpu &= ~0x28; cpu |= 0x04; }
		else if (opt == 2) cpu |= 0x2C;
	}

	css.cpu = cpu;
	css.debug = debug;
	if (*mode) 
		checkMode(mode, env);
	else
	{
        if (source < 0 || source >= YUV_COEFFS_LUMA_COUNT)
			env->ThrowError("ColorMatrix:  source must be set to 0, 1, 2, 3 or 4!");
        if (dest < 0 || dest >= YUV_COEFFS_LUMA_COUNT)
			env->ThrowError("ColorMatrix:  dest must be set to 0, 1, 2, 3 or 4!");
	}
	if (source == dest && inputFR == outputFR && !(*d2v) && !hints)
		env->ThrowError("ColorMatrix:  source and dest or inputFR and outputFR must " \
			"have different values!");

    // disable simd for Rec.2020 for I don't implement it
    if (source == 4 || dest == 4)
        css.cpu = 0;

	modei = source == dest ? -2 : source*4+dest;
	if (debug)
	{
		sprintf(buf,"ColorMatrix:%u:  version %s (%s)\n", 
			GetCurrentThreadId(), VERSION, DATE);
		OutputDebugString(buf);
	}
	if (hints)
	{
		int temp;
		child->SetCacheHints(CACHE_RANGE, 1);
		hintClip = child;
		PVideoFrame pv = child->GetFrame(0, env);
		getHint(pv->GetReadPtr(PLANAR_Y), temp);
		if (temp == -1)
			env->ThrowError("ColorMatrix:  no hints detected in stream with hints=true!");
	}
	else child->SetCacheHints(CACHE_NOTHING, 0);
	if (clamp&1) // clip input to 16-235/16-240 range
	{
		try
		{
			child = env->Invoke("Limiter", child).AsClip();
		}
		catch (IScriptEnvironment::NotFound)
		{
			env->ThrowError("ColorMatrix:  error invoking Limiter (not found)!");
		}
		catch (AvisynthError e)
		{
			env->ThrowError("ColorMatrix:  avisynth error invoking Limiter (%s)!", e.msg);
		}
	}
	if (interlaced)
	{
		if (child->GetVideoInfo().IsFieldBased())
			env->ThrowError("ColorMatrix:  input must be framebased for interlaced=true " \
				"processing (use AssumeFrameBased())!");
		try 
		{
			child = env->Invoke("InternalCache", child).AsClip();
			child->SetCacheHints(CACHE_RANGE, 1);
			child = env->Invoke("SeparateFields", child).AsClip();
		}
		catch (IScriptEnvironment::NotFound)
		{
			env->ThrowError("ColorMatrix:  error invoking InternalCache" \
				" and SeparateFields (not found)!");
		}
		catch (AvisynthError e)
		{
			env->ThrowError("ColorMatrix:  avisynth error invoking" \
				" InternalCache and SeparateFields (%s)!", e.msg);
		}
		vi.num_frames *= 2;
		vi.fps_numerator *= 2;
		vi.height >>= 1;
		vi.SetFieldBased(true);
	}
	if (*d2v) 
	{
		int temp = parseD2V(d2v);
		if (temp == 0) env->ThrowError("ColorMatrix:  unknown coefficient type detected in d2v file!");
		else if (temp == -1) env->ThrowError("ColorMatrix:  could not open specified d2v project file!");
		else if (temp == -2) env->ThrowError("ColorMatrix:  d2v file is not a dgindex project file!");
		else if (temp == -3) env->ThrowError("ColorMatrix:  d2v file is from an old version of dgindex and does not contain colorimetry info!");
		else if (temp == -4) env->ThrowError("ColorMatrix:  multiple colorimetry types for a single frame detected in d2v file!");
		else if (temp == -5) env->ThrowError("ColorMatrix:  no colorimetry info detected in d2v file!");
		else if (temp == -6) env->ThrowError("ColorMatrix:  malloc failure (carray)!");
		else if (temp == -7) env->ThrowError("ColorMatrix:  d2v and filter frame counts do not match!");
		else if (temp == -8) env->ThrowError("ColorMatrix:  malloc failure (d2varray)!");
		else if (temp == -9) env->ThrowError("ColorMatrix:  not all frames had valid values after d2v parsing!");
	}
	calc_coefficients(env);
	if (threads == 0)
	{
		threads = get_num_processors();
		if (debug)
		{
			sprintf(buf,"ColorMatrix:%u:  number of detected processors = %d\n", 
				GetCurrentThreadId(), threads);
			OutputDebugString(buf);
		}
	}
	tids = (unsigned*)malloc(threads*sizeof(unsigned));
	thds = (HANDLE*)malloc(threads*sizeof(HANDLE));
	pssInfo = (PS_INFO**)malloc(threads*sizeof(PS_INFO*));
	if (!tids || !thds || !pssInfo)
		env->ThrowError("ColorMatrix:  malloc failure (thread storage)!");
	double c0y, c1y, c0uv, c1uv;
	if (inputFR)
	{
		c0y = 219.0/255.0;
		c1y = 16.0+0.5;
		c0uv = 224.0/255.0;
	}
	else
	{
		c0y = 255.0/219.0;
		c1y = -16.0*255.0/219.0+0.5;
		c0uv = 255.0/224.0;
	}
	c1uv = -128.0*c0uv+128.0+0.5;
	for (int i=0; i<threads; ++i)
	{
		pssInfo[i] = (PS_INFO*)malloc(sizeof(PS_INFO));
		pssInfo[i]->cs = &css;
		pssInfo[i]->finished = 0;
		for (int j=0; j<256; ++j)
		{
			pssInfo[i]->ylut[j] = CB((int)(j*c0y+c1y));
			pssInfo[i]->uvlut[j] = CB((int)(j*c0uv+c1uv));
		}
		pssInfo[i]->jobFinished = CreateEvent(NULL, TRUE, TRUE, NULL);
		pssInfo[i]->nextJob = CreateEvent(NULL, TRUE, FALSE, NULL);
		thds[i] = vi.IsYUY2() ? 
			(HANDLE)_beginthreadex(0,0,&processFrame_YUY2,(void*)(pssInfo[i]),0,&tids[i]) :
			(HANDLE)_beginthreadex(0,0,&processFrame_YV12,(void*)(pssInfo[i]),0,&tids[i]);
	}
}

ColorMatrix::~ColorMatrix() 
{
	for (int i=0; i<threads; ++i)
	{
		pssInfo[i]->finished = 1;
		SetEvent(pssInfo[i]->nextJob);
	}
	WaitForMultipleObjects(threads,thds,TRUE,INFINITE);
	for (int i=0; i<threads; ++i)
		CloseHandle(thds[i]);
	if (tids) free(tids);
	if (thds) free(thds);
	if (pssInfo)
	{
		for (int i=0; i<threads; ++i)
		{
			CloseHandle(pssInfo[i]->jobFinished);
			CloseHandle(pssInfo[i]->nextJob);
			free(pssInfo[i]);
		}
		free(pssInfo);
	}
	if (d2vArray) free(d2vArray);
}

int num_processors()
{
	int pcount = 0;
	DWORD p_aff, s_aff;
	GetProcessAffinityMask(GetCurrentProcess(), &p_aff, &s_aff);
	for(; p_aff != 0; p_aff>>=1) 
		pcount += (p_aff&1);
	return pcount;
}

int ColorMatrix::get_num_processors() 
{
	static const int pcount = num_processors();
	return pcount;
}

unsigned __stdcall processFrame_YUY2(void *ps)
{
	const PS_INFO *pss = (PS_INFO*)ps;
	const bool debug = pss->cs->debug;
	while (true)
	{
		WaitForSingleObject(pss->nextJob,INFINITE);
		if (pss->finished)
			return 0;
		const unsigned char *srcp = pss->srcp;
		const int src_pitch = pss->src_pitch;
		const int height = pss->height;
		const int width = pss->width;
		unsigned char *dstp = pss->dstp;
		const int dst_pitch = pss->dst_pitch;
		if (pss->cs->modef == -2)
		{
			if (debug)
			{
				char buf[256];
				sprintf(buf,"ColorMatrix:%u:  frame %d:  YUY2 range conversion only.\n", 
					GetCurrentThreadId(), pss->cs->n);
				OutputDebugString(buf);
			}
			const int *ylut = pss->ylut;
			const int *uvlut = pss->uvlut;
			for (int h=0; h<height; ++h)
			{
				for (int x=0; x<width; x+=4)
				{
					dstp[x] = ylut[srcp[x]];
					dstp[x+1] = uvlut[srcp[x+1]];
					dstp[x+2] = ylut[srcp[x+2]];
					dstp[x+3] = uvlut[srcp[x+3]];
				}
				srcp += src_pitch;
				dstp += dst_pitch;
			}
		}
		else
		{
			if (debug)
			{
				char buf[256];
				sprintf(buf,"ColorMatrix:%u:  frame %d:  using YUY2 %s conversion.\n", 
					GetCurrentThreadId(), pss->cs->n, CTS2(pss->cs->modef));
				OutputDebugString(buf);
			}
			const int c1 = pss->cs->c1;
			const int c2 = pss->cs->c2;
			const int c3 = pss->cs->c3; 
			const int c4 = pss->cs->c4;
			const int c5 = pss->cs->c5;
			const int c6 = pss->cs->c6;
			const int c7 = pss->cs->c7;
			const int c8 = pss->cs->c8;
			for (int h=0; h<height; ++h) 
			{
				for (int x=0; x<width; x+=4)
				{
					const int u = srcp[x+1]-128;
					const int v = srcp[x+3]-128;
					const int uvval = c2*u + c3*v + c8;
					dstp[x] = CB((c1*srcp[x] + uvval) >> 16);
					dstp[x+1] = CB((c4*u + c5*v + 8421376) >> 16);
					dstp[x+2] = CB((c1*srcp[x+2] + uvval) >> 16);
					dstp[x+3] = CB((c6*u + c7*v + 8421376) >> 16);
				}
				srcp += src_pitch;
				dstp += dst_pitch;
			}
		}
		ResetEvent(pss->nextJob);
		SetEvent(pss->jobFinished);
	}
}

unsigned _stdcall processFrame_YV12(void *ps)
{
	const PS_INFO *pss = (PS_INFO*)ps;
	const long cpu = pss->cs->cpu;
	const bool debug = pss->cs->debug;
	while (true)
	{
		WaitForSingleObject(pss->nextJob,INFINITE);
		if (pss->finished)
			return 0;
		if (pss->cs->modef == -2)
		{
			if (debug)
			{
				char buf[256];
				sprintf(buf,"ColorMatrix:%u:  frame %d:  YV12 range conversion only.\n", 
					GetCurrentThreadId(), pss->cs->n);
				OutputDebugString(buf);
			}
			for (int b=0; b<3; ++b)
			{
				const unsigned char *srcp = b == 0 ? pss->srcp :
					b == 1 ? pss->srcpU : pss->srcpV;
				unsigned char *dstp = b == 0 ? pss->dstp :
					b == 1 ? pss->dstpU : pss->dstpV;
				int src_pitch, dst_pitch, width, height;
				if (b == 0)
				{
					src_pitch = pss->src_pitch;
					dst_pitch = pss->dst_pitch;
					width = pss->width;
					height = pss->height;
				}
				else
				{
					src_pitch = pss->src_pitchUV;
					dst_pitch = pss->dst_pitchUV;
					width = pss->width>>1;
					height = pss->height>>1;
				}
				const int *plut = b == 0 ? pss->ylut : pss->uvlut;
				for (int h=0; h<height; ++h)
				{
					for (int x=0; x<width; ++x)
					{
						dstp[x] = plut[srcp[x]];
					}
					srcp += src_pitch;
					dstp += dst_pitch;
				}
			}
		}
		else
		{
			const unsigned char *srcp = pss->srcp;
			unsigned char *dstp = pss->dstp;
			const int widtha = pss->widtha;
			const int src_pitch = pss->src_pitch;
			const int dst_pitch = pss->dst_pitch;
			const int c1 = pss->cs->c1;
			if (c1 == 65536 && (cpu&CPUF_SSE2) && 
				!((int(srcp)|int(dstp)|widtha|dst_pitch|src_pitch)&15)) 
			{
				if (debug)
				{
					char buf[256];
					sprintf(buf,"ColorMatrix:%u:  frame %d:  using YV12 %s conversion (SSE2).\n", 
						GetCurrentThreadId(), pss->cs->n, CTS2(pss->cs->modef));
					OutputDebugString(buf);
				}
				(find_YV12_SIMD(pss->cs->modef, true))(ps);
			}
			else if (c1 == 65536 && (cpu&CPUF_MMX) && !(widtha&7))
			{
				if (debug)
				{
					char buf[256];
					sprintf(buf,"ColorMatrix:%u:  frame %d:  using YV12 %s conversion (MMX).\n", 
						GetCurrentThreadId(), pss->cs->n, CTS2(pss->cs->modef));
					OutputDebugString(buf);
				}
				(find_YV12_SIMD(pss->cs->modef, false))(ps);
			}
			else
			{
				if (debug)
				{
					char buf[256];
					sprintf(buf,"ColorMatrix:%u:  frame %d:  using YV12 %s conversion (C).\n", 
						GetCurrentThreadId(), pss->cs->n, CTS2(pss->cs->modef));
					OutputDebugString(buf);
				}
				const unsigned char *srcpU = pss->srcpU;
				const unsigned char *srcpV = pss->srcpV;
				const unsigned char *srcpn = pss->srcpn;
				const int src_pitchUV = pss->src_pitchUV;
				const int height = pss->height;
				const int width = pss->width;
				unsigned char *dstpU = pss->dstpU;
				unsigned char *dstpV = pss->dstpV;
				unsigned char *dstpn = pss->dstpn;
				const int dst_pitchUV = pss->dst_pitchUV;
				const int c2 = pss->cs->c2;
				const int c3 = pss->cs->c3; 
				const int c4 = pss->cs->c4;
				const int c5 = pss->cs->c5;
				const int c6 = pss->cs->c6;
				const int c7 = pss->cs->c7;
				const int c8 = pss->cs->c8;
				for (int h=0; h<height; h+=2)
				{
					for (int x=0; x<width; x+=2)
					{
						const int u = srcpU[x>>1]-128;
						const int v = srcpV[x>>1]-128;
						const int uvval = c2*u + c3*v + c8;
						dstp[x] = CB((c1*srcp[x] + uvval) >> 16);
						dstp[x+1] = CB((c1*srcp[x+1] + uvval) >> 16);
						dstpn[x] = CB((c1*srcpn[x] + uvval) >> 16);
						dstpn[x+1] = CB((c1*srcpn[x+1] + uvval) >> 16);
						dstpU[x>>1] = CB((c4*u + c5*v + 8421376) >> 16);
						dstpV[x>>1] = CB((c6*u + c7*v + 8421376) >> 16);
					}
					srcp += src_pitch<<1;
					srcpn += src_pitch<<1;
					dstp += dst_pitch<<1;
					dstpn += dst_pitch<<1;
					srcpU += src_pitchUV;
					srcpV += src_pitchUV;
					dstpU += dst_pitchUV;
					dstpV += dst_pitchUV;
				}
			}
		}
		ResetEvent(pss->nextJob);
		SetEvent(pss->jobFinished);
	}
}

PVideoFrame __stdcall ColorMatrix::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame src = child->GetFrame(n, env);
	int modef = modei;
	if (d2vArray)
	{
		int temp = d2vArray[interlaced?(n>>1):n];
		if (debug)
		{
			sprintf(buf,"ColorMatrix:%u:  frame %d:  detected colorimetry from d2v = %d (%s)\n", 
				GetCurrentThreadId(), n, temp, CTS(temp));
			OutputDebugString(buf);
		}
		modef = findMode(temp);
		if (modef == -1) 
		{
			if (debug)
			{
				sprintf(buf,"ColorMatrix:%u:  frame %d:  returning src frame... no conversion " \
					"required (d2v)\n", GetCurrentThreadId(), n);
				OutputDebugString(buf);
			}
			return src;
		}
	}
	else if (hints)
	{
		int temp = -1;
		PVideoFrame hintf;
		if (interlaced) hintf = hintClip->GetFrame(n>>1, env);
		else hintf = hintClip->GetFrame(n, env);	
		getHint(hintf->GetReadPtr(PLANAR_Y), temp);
		if (temp == -1) 
			env->ThrowError("ColorMatrix:  no hints detected in stream with hints=true!");
		if (debug)
		{
			sprintf(buf,"ColorMatrix:%u:  frame %d:  detected hint = %d (%s)\n", 
				GetCurrentThreadId(), n, temp, CTS(temp));
			OutputDebugString(buf);
		}
		modef = findMode(temp);
		if (modef == -1) 
		{
			if (debug)
			{
				sprintf(buf,"ColorMatrix:%u:  frame %d:  returning src frame... no conversion " \
					"required (hints)\n", GetCurrentThreadId(), n);
				OutputDebugString(buf);
			}
			return src;
		}
	}
	PVideoFrame dst = env->NewVideoFrame(vi);
	const int src_pitch = src->GetPitch();
	const int src_width = src->GetRowSize();
	const int src_height = src->GetHeight();
	const int dst_pitch = dst->GetPitch();
	const int dst_width = dst->GetRowSize();
	const int dst_height = dst->GetHeight();
	if (modef >= 0)
	{
		css.c1 = yuv_convert[modef][0][0];
		css.c2 = yuv_convert[modef][0][1];
		css.c3 = yuv_convert[modef][0][2]; 
		css.c4 = yuv_convert[modef][1][1];
		css.c5 = yuv_convert[modef][1][2];
		css.c6 = yuv_convert[modef][2][1];
		css.c7 = yuv_convert[modef][2][2];
		css.c8 = 32768;
		if (!inputFR)
			css.c8 -= 16*yuv_convert[modef][0][0];
		if (!outputFR)
			css.c8 += 16*65536;
	}
	css.modef = modef;
	css.n = n;
	if (vi.IsYUY2())
	{
		const unsigned char* srcp = src->GetReadPtr();
		unsigned char* dstp = dst->GetWritePtr();
		const int hslice = src_height/threads;
		const int hremain = src_height%threads;
		for (int tc=0; tc<threads; ++tc)
		{
			pssInfo[tc]->width = src_width;
			if (thrdmthd == 1)
			{
				pssInfo[tc]->dst_pitch = dst_pitch*threads;
				pssInfo[tc]->src_pitch = src_pitch*threads;
				pssInfo[tc]->dstp = dstp+tc*dst_pitch;
				pssInfo[tc]->srcp = srcp+tc*src_pitch;
				pssInfo[tc]->height = tc < hremain ? hslice+1 : hslice;
			}
			else
			{
				pssInfo[tc]->dst_pitch = dst_pitch;
				pssInfo[tc]->src_pitch = src_pitch;
				pssInfo[tc]->dstp = dstp+hslice*tc*dst_pitch;
				pssInfo[tc]->srcp = srcp+hslice*tc*src_pitch;
				pssInfo[tc]->height = tc == threads-1 ? hslice+hremain : hslice;
			}
			ResetEvent(pssInfo[tc]->jobFinished);
			SetEvent(pssInfo[tc]->nextJob);
		}
		for (int tc=0; tc<threads; ++tc)
			WaitForSingleObject(pssInfo[tc]->jobFinished,INFINITE);
	}
	if (vi.IsYV12())
	{
		const unsigned char* srcp = src->GetReadPtr(PLANAR_Y);
		const unsigned char* srcpV = src->GetReadPtr(PLANAR_V);
		const unsigned char* srcpU = src->GetReadPtr(PLANAR_U);
		const int src_widtha = src->GetRowSize(PLANAR_Y_ALIGNED);
		const int src_pitchUV = src->GetPitch(PLANAR_U);
		const int src_widthUV = src->GetRowSize(PLANAR_U);
		const int src_heightUV = src->GetHeight(PLANAR_U);
		unsigned char* dstp = dst->GetWritePtr(PLANAR_Y);
		unsigned char* dstpV = dst->GetWritePtr(PLANAR_V);
		unsigned char* dstpU = dst->GetWritePtr(PLANAR_U);
		const int dst_pitchUV = dst->GetPitch(PLANAR_U);
		const int hslice = (src_height>>1)/threads;
		const int hremain = (src_height>>1)%threads;
		for (int tc=0; tc<threads; ++tc)
		{
			pssInfo[tc]->width = src_width;
			pssInfo[tc]->widtha = src_widtha;
			if (thrdmthd == 1)
			{
				pssInfo[tc]->dst_pitch = dst_pitch*threads;
				pssInfo[tc]->dst_pitchR = dst_pitch;
				pssInfo[tc]->dst_pitchUV = dst_pitchUV*threads;
				pssInfo[tc]->src_pitch = src_pitch*threads;
				pssInfo[tc]->src_pitchR = src_pitch;
				pssInfo[tc]->src_pitchUV = src_pitchUV*threads;
				pssInfo[tc]->dstp = dstp+tc*dst_pitch*2;
				pssInfo[tc]->dstpn = pssInfo[tc]->dstp+dst_pitch;
				pssInfo[tc]->dstpU = dstpU+tc*dst_pitchUV;
				pssInfo[tc]->dstpV = dstpV+tc*dst_pitchUV;
				pssInfo[tc]->srcp = srcp+tc*src_pitch*2;
				pssInfo[tc]->srcpn = pssInfo[tc]->srcp+src_pitch;
				pssInfo[tc]->srcpU = srcpU+tc*src_pitchUV;
				pssInfo[tc]->srcpV = srcpV+tc*src_pitchUV;
				pssInfo[tc]->height = tc < hremain ? (hslice+1)*2 : hslice*2;
			}
			else
			{
				pssInfo[tc]->dst_pitch = dst_pitch;
				pssInfo[tc]->dst_pitchR = dst_pitch;
				pssInfo[tc]->dst_pitchUV = dst_pitchUV;
				pssInfo[tc]->src_pitch = src_pitch;
				pssInfo[tc]->src_pitchR = src_pitch;
				pssInfo[tc]->src_pitchUV = src_pitchUV;
				pssInfo[tc]->dstp = dstp+hslice*tc*dst_pitch*2;
				pssInfo[tc]->dstpn = pssInfo[tc]->dstp+dst_pitch;
				pssInfo[tc]->dstpU = dstpU+hslice*tc*dst_pitchUV;
				pssInfo[tc]->dstpV = dstpV+hslice*tc*dst_pitchUV;
				pssInfo[tc]->srcp = srcp+hslice*tc*src_pitch*2;
				pssInfo[tc]->srcpn = pssInfo[tc]->srcp+src_pitch;
				pssInfo[tc]->srcpU = srcpU+hslice*tc*src_pitchUV;
				pssInfo[tc]->srcpV = srcpV+hslice*tc*src_pitchUV;
				pssInfo[tc]->height = tc == threads-1 ? (hslice+hremain)*2 : hslice*2;
			}
			ResetEvent(pssInfo[tc]->jobFinished);
			SetEvent(pssInfo[tc]->nextJob);
		}
		for (int tc=0; tc<threads; ++tc)
			WaitForSingleObject(pssInfo[tc]->jobFinished,INFINITE);
	}
	return dst;
}

void ColorMatrix::getHint(const unsigned char *srcp, int &color)
{
	color = -1;
	unsigned int i, hint = 0, magic_number = 0;
	for (i=0; i<32; ++i)
	{
		magic_number |= ((*srcp++ & 1) << i);
	}
	if (magic_number != MAGIC_NUMBER) return;
	for (i=0; i<32; ++i)
	{
		hint |= ((*srcp++ & 1) << i);
	}
	if (hint&0xFFFFFFE0) return; // this will need to be changed if more hint fields are added
	color = (hint&COLORIMETRY)>>COLORIMETRY_SHIFT;
	if (color != 1 && (color < 4 || color > 7))
		color = -1;
}

void ColorMatrix::checkMode(const char *md, IScriptEnvironment *env)
{
	source = dest = -1;
	if (lstrcmpi(md, "Rec.709->Rec.709") == 0) { source = 0; dest = 0; }
	if (lstrcmpi(md, "Rec.709->FCC") == 0) { source = 0; dest = 1; }
	if (lstrcmpi(md, "Rec.709->Rec.601") == 0) { source = 0; dest = 2; }
	if (lstrcmpi(md, "Rec.709->SMPTE 240M") == 0) { source = 0; dest = 3; }
    if (lstrcmpi(md, "Rec.709->Rec.2020") == 0) { source = 0; dest = 4; }
	if (lstrcmpi(md, "FCC->Rec.709") == 0) { source = 1; dest = 0; }
	if (lstrcmpi(md, "FCC->FCC") == 0) { source = 1; dest = 1; }
	if (lstrcmpi(md, "FCC->Rec.601") == 0) { source = 1; dest = 2; }
	if (lstrcmpi(md, "FCC->SMPTE 240M") == 0) { source = 1; dest = 3; }
    if (lstrcmpi(md, "FCC->Rec.2020") == 0) { source = 1; dest = 4; }
	if (lstrcmpi(md, "Rec.601->Rec.709") == 0) { source = 2; dest = 0; }
	if (lstrcmpi(md, "Rec.601->FCC") == 0) { source = 2; dest = 1; }
	if (lstrcmpi(md, "Rec.601->Rec.601") == 0) { source = 2; dest = 2; }
	if (lstrcmpi(md, "Rec.601->SMPTE 240M") == 0) { source = 2; dest = 3; }
    if (lstrcmpi(md, "Rec.601->Rec.2020") == 0) { source = 2; dest = 4; }
	if (lstrcmpi(md, "SMPTE 240M->Rec.709") == 0) { source = 3; dest = 0; }
	if (lstrcmpi(md, "SMPTE 240M->FCC") == 0) { source = 3; dest = 1; }
	if (lstrcmpi(md, "SMPTE 240M->Rec.601") == 0) { source = 3; dest = 2; }
	if (lstrcmpi(md, "SMPTE 240M->SMPTE 240M") == 0) { source = 3; dest = 3; }
    if (lstrcmpi(md, "SMPTE 240M->Rec.2020") == 0) { source = 3; dest = 4; }
    if (lstrcmpi(md, "Rec.2020->Rec.709") == 0) { source = 4; dest = 0; }
    if (lstrcmpi(md, "Rec.2020->FCC") == 0) { source = 4; dest = 1; }
    if (lstrcmpi(md, "Rec.2020->Rec.601") == 0) { source = 4; dest = 2; }
    if (lstrcmpi(md, "Rec.2020->SMPTE 240M") == 0) { source = 4; dest = 3; }
    if (lstrcmpi(md, "Rec.2020->Rec.2020") == 0) { source = 4; dest = 4; }
	if (source == -1 || dest == -1)
		env->ThrowError("ColorMatrix:  invalid mode string!");
}

int ColorMatrix::findMode(int color)
{
	if (color == 1 && dest != 0) 
		return dest;
	else if (color == 4 && dest != 1)
		return 4+dest;
	else if ((color == 5 || color == 6) && dest != 2)
		return 8+dest;
	else if (color == 7 && dest != 3)
		return 12+dest;
	if (inputFR != outputFR)
		return -2;
	return -1;
}

void (*find_YV12_SIMD(int modef, bool sse2))(void *ps)
{
	if (modef == 1 || modef == 2 || modef == 3 || 
		modef == 13 || modef == 14)
	{
		if (sse2) return &conv1_YV12_SSE2;
		return &conv1_YV12_MMX;
	}
	else if (modef == 4 || modef == 7 || modef == 8 || 
		modef == 11 || modef == 12)
	{
		if (sse2) return &conv2_YV12_SSE2;
		return &conv2_YV12_MMX;
	}
	else if (modef == 6)
	{
		if (sse2) return &conv3_YV12_SSE2;
		return &conv3_YV12_MMX;
	}
	else if (modef == 9)
	{
		if (sse2) return &conv4_YV12_SSE2;
		return &conv4_YV12_MMX;
	}
	return NULL;
}

int ColorMatrix::parseD2V(const char *d2v)
{
	char line[1024], *p;
	bool first, multi, smulti;
	int D2Vformat = 0, color, color_last, pass = 1;
	int cnum = 0, cnum2 = 0, val;
	int *carray = NULL, frames = 0;
	FILE *ind2v = NULL;
pass2_start:
	color = color_last = -1;
	first = true;
	multi = false;
	ind2v = fopen(d2v, "r");
	if (ind2v == NULL) return -1;
	fgets(line, 1024, ind2v);
	if (strncmp(line, "DGIndexProjectFile", 18) != 0)
	{
		fclose(ind2v);
		return -2;
	}
	sscanf(line, "DGIndexProjectFile%d", &D2Vformat);
	if (D2Vformat < 7) 
	{
		fclose(ind2v);
		return -3;
	}
	if (pass == 2)
	{
		carray = (int *)malloc(cnum*2*sizeof(int));
		if (!carray)
		{
			fclose(ind2v);
			return -6;
		}
		memset(carray, 0, cnum*2*sizeof(int));
	}
	while (fgets(line, 1024, ind2v) != 0)
	{
		if (strncmp(line, "Location", 8) == 0) break;
	}
	fgets(line, 1024, ind2v);
	fgets(line, 1024, ind2v);
	do
	{
		smulti = false;
		p = line;
		while (*p++ != ' ');
		sscanf(p, "%d", &color);
		if (color != 1 && (color < 4 || color > 7)) 
		{
			fclose(ind2v);
			if (carray) free(carray);
			return 0; // unknown matrix type
		}
		if (color != color_last && !first && ((color != 5 && color != 6) || 
			(color_last != 5 && color_last != 6))) multi = smulti = true;
		first = false;
		color_last = color;
		if (pass == 1) ++cnum;
		else
		{
			carray[cnum2++] = color;
			if ((frames&1) && smulti)
			{
				fclose(ind2v);
				free(carray);
				return -4;
			}
			carray[cnum2++] = frames>>1;
			while (*p++ != ' ');
			while (*p++ != ' ');
			while (*p++ != ' ');
			while (*p++ != ' ');
			while (*p++ != ' ');
			if (D2Vformat >= 16)
				while (*p++ != ' ');
			while (*p > 47 && *p < 123)
			{
				sscanf(p, "%x", &val);
				if (!(D2Vformat > 7 && val == 0xFF) && !(D2Vformat == 7 && (val&0x40)))
				{
					if (val&1) frames += 3;
					else frames += 2;
				}
				while (*p != ' ' && *p != '\n') p++;
				p++;
			}
		}
	}
	while ((fgets(line, 1024, ind2v) != 0) && line[0] > 47 && line[0] < 123);
	fclose(ind2v);
	ind2v = NULL;
	if (pass == 1) { pass++; goto pass2_start; }
	if (color == -1) 
	{
		if (carray) free(carray);
		return -5;
	}
	if (!multi)
	{
		d2vArray = (unsigned char *)malloc(vi.num_frames*sizeof(unsigned char));
		if (!d2vArray) 
		{
			free(carray);
			return -8;
		}
		memset(d2vArray, color, vi.num_frames*sizeof(unsigned char));
	}
	else
	{
		frames >>= 1;
		const int vnf = interlaced ? (vi.num_frames>>1) : vi.num_frames;
		if (frames != vnf)
		{
			free(carray);
			return -7;
		}
		d2vArray = (unsigned char *)malloc(frames*sizeof(unsigned char));
		if (!d2vArray) 
		{
			free(carray);
			return -8;
		}
		memset(d2vArray, 0, frames*sizeof(unsigned char));
		for (int i=0; i<cnum; ++i)
		{
			const int start = carray[i*2+1];
			const int stop = i == cnum-1 ? frames : carray[i*2+3];
			for (int j=start; j<stop; ++j) 
				d2vArray[j] = carray[i*2];
		}
		for (int i=0; i<vnf; ++i)
		{
			if (d2vArray[i] != 1 && (d2vArray[i] < 4 || d2vArray[i] > 7))
			{
				free(carray);
				free(d2vArray);
				return -9;
			}
		}
	}
	free(carray);
	return 1;
}

#define ma m[0][0]
#define mb m[0][1]
#define mc m[0][2]
#define md m[1][0]
#define me m[1][1]
#define mf m[1][2]
#define mg m[2][0]
#define mh m[2][1]
#define mi m[2][2]

#define ima im[0][0]
#define imb im[0][1]
#define imc im[0][2]
#define imd im[1][0]
#define ime im[1][1]
#define imf im[1][2]
#define img im[2][0]
#define imh im[2][1]
#define imi im[2][2]

void ColorMatrix::inverse3x3(double im[3][3], double m[3][3])
{
	double det = ma*(me*mi-mf*mh)-mb*(md*mi-mf*mg)+mc*(md*mh-me*mg);
	det = 1.0/det;
	ima = det*(me*mi-mf*mh);
	imb = det*(mc*mh-mb*mi);
	imc = det*(mb*mf-mc*me);
	imd = det*(mf*mg-md*mi);
	ime = det*(ma*mi-mc*mg);
	imf = det*(mc*md-ma*mf);
	img = det*(md*mh-me*mg);
	imh = det*(mb*mg-ma*mh);
	imi = det*(ma*me-mb*md);
}

void ColorMatrix::solve_coefficients(double cm[3][3], double rgb[3][3], double yuv[3][3],
	double yiscale, double uviscale, double yoscale, double uvoscale)
{
	for (int i=0; i<3; ++i)
	{
		double toscale = i == 0 ? yoscale : uvoscale;
		for (int j=0; j<3; ++j)
		{
			double tiscale = j == 0 ? yiscale : uviscale;
			cm[i][j] = (yuv[i][0]*rgb[0][j]+yuv[i][1]*rgb[1][j]+
				yuv[i][2]*rgb[2][j])*tiscale*toscale;
		}
	}
}

void ColorMatrix::calc_coefficients(IScriptEnvironment *env)
{
    double yuv_coeff[YUV_COEFFS_LUMA_COUNT][3][3];
    for (int i = 0; i<YUV_COEFFS_LUMA_COUNT; ++i)
	{
		yuv_coeff[i][0][0] = yuv_coeffs_luma[i][0];
		yuv_coeff[i][0][1] = yuv_coeffs_luma[i][1];
		yuv_coeff[i][0][2] = yuv_coeffs_luma[i][2];
		const double bscale = .5/(1.0-yuv_coeff[i][0][1]);
		const double rscale = .5/(1.0-yuv_coeff[i][0][2]);
		yuv_coeff[i][1][0] = -yuv_coeff[i][0][0]*bscale;
		yuv_coeff[i][1][1] = (1.0-yuv_coeff[i][0][1])*bscale;
		yuv_coeff[i][1][2] = -yuv_coeff[i][0][2]*bscale;
		yuv_coeff[i][2][0] = -yuv_coeff[i][0][0]*rscale;
		yuv_coeff[i][2][1] = -yuv_coeff[i][0][1]*rscale;
		yuv_coeff[i][2][2] = (1.0-yuv_coeff[i][0][2])*rscale;
	}
    double rgb_coeffd[YUV_COEFFS_LUMA_COUNT][3][3];
    double yuv_convertd[YUV_COEFFS_LUMA_COUNT * YUV_COEFFS_LUMA_COUNT][3][3];
    for (int i = 0; i<YUV_COEFFS_LUMA_COUNT; ++i)
		inverse3x3(rgb_coeffd[i], yuv_coeff[i]);
	double yiscale = 1.0/255.0, uviscale = 1.0/255.0;
	double yoscale = 255.0, uvoscale = 255.0;
	if (!inputFR)
	{
		yiscale = 1.0/219.0;
		uviscale = 1.0/224.0;
	}
	if (!outputFR)
	{
		yoscale = 219.0;
		uvoscale = 224.0;
	}
	int v = 0;
    for (int i = 0; i<YUV_COEFFS_LUMA_COUNT; ++i)
	{
        for (int j = 0; j<YUV_COEFFS_LUMA_COUNT; ++j)
		{
			solve_coefficients(yuv_convertd[v], rgb_coeffd[i], yuv_coeff[j],
				yiscale, uviscale, yoscale, uvoscale);
			for (int k=0; k<3; ++k)
			{
				yuv_convert[v][k][0] = ns(yuv_convertd[v][k][0]);
				yuv_convert[v][k][1] = ns(yuv_convertd[v][k][1]);
				yuv_convert[v][k][2] = ns(yuv_convertd[v][k][2]);
			}

#if 1
            // hack it for Rec.2020
            if (i == 4 || j == 4)
            {
                if (abs((signed long)yuv_convert[v][0][0] - 65536) < (65536 / 200)) yuv_convert[v][0][0] = 65536;
                if (abs(yuv_convert[v][1][0]) < 65536 / 400) yuv_convert[v][1][0] = 0;
                if (abs(yuv_convert[v][2][0]) < 65536 / 400) yuv_convert[v][2][0] = 0;
            }
#endif

            if ((yuv_convert[v][0][0] != 65536 && inputFR == outputFR) || 
                yuv_convert[v][1][0] != 0 || yuv_convert[v][2][0] != 0)
            {

                env->ThrowError("ColorMatrix:  error calculating conversion coefficients!");
            }
			++v;
		}
	}
}

AVSValue __cdecl Create_ColorMatrix(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	PClip return_clip = args[0].AsClip();
	bool interlaced = false;
	int clamp = args[4].AsInt(3);
	if (args[5].IsBool() && args[5].AsBool() &&
		return_clip->GetVideoInfo().IsYV12()) // only need special handling for YV12 interlaced
		interlaced = true;
	return_clip = new ColorMatrix(return_clip,args[1].AsString(""),args[2].AsInt(0),
		args[3].AsInt(2),clamp,interlaced,args[6].AsBool(false),args[7].AsBool(false),
		args[8].AsBool(false),args[9].AsString(""),args[10].AsBool(false),args[11].AsInt(1),
		args[12].AsInt(0),args[13].AsInt(3),env);
	if (interlaced) // interlaced
	{
		try
		{
			return_clip = env->Invoke("Weave", return_clip).AsClip();
		}
		catch (IScriptEnvironment::NotFound)
		{
			env->ThrowError("ColorMatrix:  error invoking Weave (not found)!");
		}
		catch (AvisynthError e) 
		{
			env->ThrowError("ColorMatrix:  avisynth error invoking Weave (%s)!", e.msg);
		}
	}
	if (clamp>1) // clip output to 16-235/16-240 range
	{
		try
		{
			return_clip = env->Invoke("Limiter", return_clip).AsClip();
		}
		catch (IScriptEnvironment::NotFound)
		{
			env->ThrowError("ColorMatrix:  error invoking Limiter (not found)!");
		}
		catch (AvisynthError e)
		{
			env->ThrowError("ColorMatrix:  avisynth error invoking Limiter (%s)!", e.msg);
		}
	}
	return return_clip;
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) 
{
	env->AddFunction("ColorMatrix", "c[mode]s[source]i[dest]i[clamp]i[interlaced]b" \
		"[inputFR]b[outputFR]b[hints]b[d2v]s[debug]b[threads]i[thrdmthd]i[opt]i", 
		Create_ColorMatrix, 0);
    return 0;
}