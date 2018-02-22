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

#include <windows.h>
#include <stdio.h>
#include <xmmintrin.h>
#include <process.h>
#include <float.h>
#include "avisynth.h"

#define VERSION "2.5"
#define DATE "01/25/2009"

#define MAGIC_NUMBER 0xdeadbeef
#define COLORIMETRY 0x0000001C
#define COLORIMETRY_SHIFT 2
#define CTS(n) n == 1 ? "Rec.709" : \
				n == 4 ? "FCC" : \
				n == 5 ? "Rec.601" : \
				n == 6 ? "Rec.601" : \
				n == 7 ? "SMPTE 240M" : \
				n == -1 ? "no hint found" : \
				"unknown"
#define CTS2(n) n == 1 ? "Rec.709->FCC" : \
				n == 2 ? "Rec.709->Rec.601" : \
				n == 3 ? "Rec.709->SMPTE 240M" : \
				n == 4 ? "FCC->Rec.709" : \
				n == 6 ? "FCC->Rec.601" : \
				n == 7 ? "FCC->SMPTE 240M" : \
				n == 8 ? "Rec.601->Rec.709" : \
				n == 9 ? "Rec.601->FCC" : \
				n == 11 ? "Rec.601->SMPTE 240M" : \
				n == 12 ? "SMPTE 240M->Rec.709" : \
				n == 13 ? "SMPTE 240M->FCC" : \
				n == 14 ? "SMPTE 240M->Rec.601" : \
				"unknown"
#define ns(n) n < 0 ? int(n*65536.0-0.5+DBL_EPSILON) : int(n*65536.0+0.5)
#define CB(n) max(min((n),255),0)
#define simd_scale(n) n >= 65536 ? (n+2)>>2 : n >= 32768 ? (n+1)>>1 : n;

static double yuv_coeffs_luma[4][3] =
{ 
  +0.7152, +0.0722, +0.2126, // Rec.709 (0)
  +0.5900, +0.1100, +0.3000, // FCC (1)
  +0.5870, +0.1140, +0.2990, // Rec.601 (ITU-R BT.470-2/SMPTE 170M) (2)
  +0.7010, +0.0870, +0.2120, // SMPTE 240M (3)
};

__declspec(align(16)) const __int64 Q32[2] = { 0x0020002000200020, 0x0020002000200020 };
__declspec(align(16)) const __int64 Q64[2] = { 0x0040004000400040, 0x0040004000400040 };
__declspec(align(16)) const __int64 Q128[2] = { 0x0080008000800080, 0x0080008000800080 };
__declspec(align(16)) const __int64 Q8224[2] = { 0x2020202020202020, 0x2020202020202020 };
__declspec(align(16)) const __int64 Q8 = 0x0000000000000008;

struct CFS {
	int c1, c2, c3, c4;
	int c5, c6, c7, c8;
	int n, modef;
	long cpu;
	bool debug;
};

struct PS_INFO {
	int ylut[256], uvlut[256];
	const unsigned char *srcp, *srcpn;
	const unsigned char *srcpU, *srcpV;
	int src_pitch, src_pitchR, src_pitchUV;
	int height, width, widtha;
	unsigned char *dstp, *dstpn;
	unsigned char *dstpU, *dstpV;
	int dst_pitch, dst_pitchR, dst_pitchUV;
	CFS *cs;
	HANDLE nextJob, jobFinished;
	bool finished;
};

int num_processors();
unsigned __stdcall processFrame_YUY2(void *ps);
unsigned __stdcall processFrame_YV12(void *ps);
void (*find_YV12_SIMD(int modef, bool sse2))(void *ps);
void conv1_YV12_MMX(void *ps);
void conv2_YV12_MMX(void *ps);
void conv3_YV12_MMX(void *ps);
void conv4_YV12_MMX(void *ps);
void conv1_YV12_SSE2(void *ps);
void conv2_YV12_SSE2(void *ps);
void conv3_YV12_SSE2(void *ps);
void conv4_YV12_SSE2(void *ps);

class ColorMatrix : public GenericVideoFilter 
{
private:
	char buf[256];
	int yuv_convert[16][3][3];
	const char *mode, *d2v;
	unsigned char *d2vArray;
	bool hints, interlaced, debug;
	bool inputFR, outputFR;
	int source, dest, modei, clamp;
	int opt, threads, thrdmthd;
	PClip hintClip;
	CFS css;
	unsigned *tids;
	HANDLE *thds;
	PS_INFO **pssInfo;
	void ColorMatrix::getHint(const unsigned char *srcp, int &color);
	void ColorMatrix::checkMode(const char *md, IScriptEnvironment *env);
	int ColorMatrix::findMode(int color);
	int ColorMatrix::parseD2V(const char *d2v);
	void ColorMatrix::inverse3x3(double im[3][3], double m[3][3]);
	void ColorMatrix::solve_coefficients(double cm[3][3], double rgb[3][3], double yuv[3][3],
		double yiscale, double uviscale, double yoscale, double uvoscale);
	void ColorMatrix::calc_coefficients(IScriptEnvironment *env);
	static int ColorMatrix::get_num_processors();

public:
	ColorMatrix::ColorMatrix(PClip _child, const char* _mode, int _source, int _dest, 
		int _clamp, bool _interlaced, bool _inputFR, bool _outputFR, bool _hints, 
		const char* _d2v, bool _debug, int _threads, int _thrdmthd, int _opt, 
		IScriptEnvironment* env);
	ColorMatrix::~ColorMatrix();
	PVideoFrame __stdcall ColorMatrix::GetFrame(int n, IScriptEnvironment* env);
};