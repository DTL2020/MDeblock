// MDeblock.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

//#define AVS_LINKAGE_DLLIMPORT

#define LOGO		"MDeblock 0.3\n"
// An Avisynth plugin for removing blockiness
//
// By Rainer Wittmann <gorw@gmx.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// To get a copy of the GNU General Public License write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

//#define FDEBUG
#define	MAXMODE	4

#define VC_EXTRALEAN 
#include <Windows.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "include\avisynth.h"

IScriptEnvironment	*AVSenvironment;


void	debug_printf(const char *format, ...)
{
	char	buffer[200];
	va_list	args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	OutputDebugString(buffer);
}

static void	nodeblock(BYTE *p, int pitch, int dist, int hblocks, int height, int offset)
{
}

#define	ONETHIRDS	23 // we could take the more accurate 24 here, but choose 23 to be on the save side
#define	ONETHIRDM	(((1u << ONETHIRDS) + 0)/3)
#define	ONEFOURTHS	2
#define	ONEFOURTHM	1
#define	ONEFIFTHS	22
#define	ONEFIFTHM	(((1u << ONEFIFTHS) + 3)/5)

#define	deblock1(x1, x2, x3, x4)\
{\
	int	d1, d2, d3;\
	if( (d1 = (int)x1 - (int)x2) < 0 ) d1 = -d1;\
	if( (d3 = (int)x4 - (int)x3) < 0 ) d3 = -d3;\
	if( (d2 = (int)x2 - (int)x3) > 0 ) \
	{\
		int	a2;\
		if( (a2 = d2 + d3 - 2*d1) < 0 )\
		{\
			if( (d2 -= d3) > 0 )\
			{\
				x3 += d2 / 2;\
			}\
		}\
		else\
		{\
			int a3;\
			if( (a3 = d2 + d1 - 2*d3) < 0 )\
			{\
				if( (d2 -= d1) > 0 )\
				{\
					x2 -= d2 / 2;\
				}\
			}\
			else\
			{\
				x3 += (BYTE)(((unsigned)a3 * ONETHIRDM) >> ONETHIRDS);\
				x2 -= (BYTE)(((unsigned)a2 * ONETHIRDM) >> ONETHIRDS);\
			}\
		}\
	}\
	else /* d2 <= 0 */ \
	{\
		d2 = -d2;\
		int	a2;\
		if( (a2 = d2 + d3 - 2*d1) < 0 )\
		{\
			if( (d2 -= d3) > 0 )\
			{\
				x3 -= d2 / 2;\
			}\
		}\
		else\
		{\
			int a3;\
			if( (a3 = d2 + d1 - 2*d3) < 0 )\
			{\
				if( (d2 -= d1) > 0 )\
				{\
					x2 += d2 / 2;\
				}\
			}\
			else\
			{\
				x3 -= (BYTE)(((unsigned)a3 * ONETHIRDM) >> ONETHIRDS);\
				x2 += (BYTE)(((unsigned)a2 * ONETHIRDM) >> ONETHIRDS);\
			}\
		}\
	}\
}

#define	deblock2(x1, x2, x3, x4)\
{\
	int	d2;\
	if( (d2 = (int)x2 - (int)x3) > 0 ) \
	{\
		int m;\
		if( (m = (int)x1 - (int)x4) > 0 )\
		{\
			int d1, d3;\
			if( (d1 = (int)x1 - (int)x2) < 0 ) d1 = -d1;\
			if( (d3 = (int)x3 - (int)x4) < 0 ) d3 = -d3;\
			unsigned	a2;\
			if( (int)(a2 = d2 + d3 - 2*d1) < 0 )\
			{\
				if( (d2 -= d3) > 0 )\
				{\
					if( (d2 /= 2) > m ) d2 = m;\
					x3 += d2;\
				}\
			}\
			else\
			{\
				unsigned a3;\
				if( (int)(a3 = d2 + d1 - 2*d3) < 0 )\
				{\
					if( (d2 -= d1) > 0 )\
					{\
						if( (d2 /= 2) > m ) d2 = m;\
						x2 -= d2;\
					}\
				}\
				else\
				{\
					if( (a3 = (a3 * ONETHIRDM) >> ONETHIRDS) > (unsigned) m ) a3 = m;\
					x3 += (BYTE) a3;\
					if( (a2 = (a2 * ONETHIRDM) >> ONETHIRDS) > (unsigned) m ) a2 = m;\
					x2 -= (BYTE) a2;\
				}\
			}\
		}\
	}\
	else /* d2 <= 0 */ \
	{\
		d2 = -d2;\
		int m;\
		if( (m = (int)x4 - (int)x1) > 0 )\
		{\
			int d1, d3;\
			if( (d1 = (int)x1 - (int)x2) < 0 ) d1 = -d1;\
			if( (d3 = (int)x3 - (int)x4) < 0 ) d3 = -d3;\
			unsigned	a2;\
			if( (int)(a2 = d2 + d3 - 2*d1) < 0 )\
			{\
				if( (d2 -= d3) > 0 )\
				{\
					if( (d2 /= 2) > m ) d2 = m;\
					x3 -= d2;\
				}\
			}\
			else\
			{\
				unsigned a3;\
				if( (int)(a3 = d2 + d1 - 2*d3) < 0 )\
				{\
					if( (d2 -= d1) > 0 )\
					{\
						if( (d2 /= 2) > m ) d2 = m;\
						x2 += d2;\
					}\
				}\
				else\
				{\
					if( (a3 = (a3 * ONETHIRDM) >> ONETHIRDS) > (unsigned) m ) a3 = m;\
					x3 -= (BYTE) a3;\
					if( (a2 = (a2 * ONETHIRDM) >> ONETHIRDS) > (unsigned) m ) a2 = m;\
					x2 += (BYTE) a2;\
				}\
			}\
		}\
	}\
}

#define	deblock3(x1, x2, x3, x4)\
{\
	int	d2;\
	if( (d2 = (int)x2 - (int)x3) > 0 ) \
	{\
		int d1, d3;\
		if( (d1 = (int)x1 - (int)x2) < 0 ) d1 = -d1;\
		if( (d3 = (int)x3 - (int)x4) < 0 ) d3 = -d3;\
		unsigned	a2;\
		if( (int)(a2 = d2 + d3 - 2*d1) <= 0 )\
		{\
			if( (d2 -= d1) > 0 )\
			{\
				x3 += d2;\
			}\
		}\
		else\
		{\
			unsigned a3;\
			if( (int)(a3 = d2 + d1 - 2*d3) <= 0 )\
			{\
				if( (d2 -= d3) > 0 )\
				{\
					x2 -= d2;\
				}\
			}\
			else\
			{\
				x3 += (BYTE) ((a3 * ONETHIRDM) >> ONETHIRDS);\
				x2 -= (BYTE) ((a2 * ONETHIRDM) >> ONETHIRDS);\
			}\
		}\
	}\
	else /* d2 <= 0 */ \
	{\
		d2 = -d2;\
		int d1, d3;\
		if( (d1 = (int)x1 - (int)x2) < 0 ) d1 = -d1;\
		if( (d3 = (int)x3 - (int)x4) < 0 ) d3 = -d3;\
		unsigned	a2;\
		if( (int)(a2 = d2 + d3 - 2*d1) <= 0 )\
		{\
			if( (d2 -= d1) > 0 )\
			{\
				x3 -= d2;\
			}\
		}\
		else\
		{\
			unsigned a3;\
			if( (int)(a3 = d2 + d1 - 2*d3) <= 0 )\
			{\
				if( (d2 -= d3) > 0 )\
				{\
					x2 += d2;\
				}\
			}\
			else\
			{\
				x3 -= (BYTE) ((a3 * ONETHIRDM) >> ONETHIRDS);\
				x2 += (BYTE) ((a2 * ONETHIRDM) >> ONETHIRDS);\
			}\
		}\
	}\
}

#define	deblock4(x1, x2, x3, x4)\
{\
	int	d2;\
	if( (d2 = (int)x2 - (int)x3) > 0 ) \
	{\
		int m;\
		if( (m = (int)x1 - (int)x4) > 0 )\
		{\
			int d1, d3;\
			if( (d1 = (int)x1 - (int)x2) < 0 ) d1 = -d1;\
			if( (d3 = (int)x3 - (int)x4) < 0 ) d3 = -d3;\
			unsigned	a2;\
			if( (int)(a2 = d2 + d3 - 2*d1) < 0 )\
			{\
				if( (d2 -= d1) > 0 )\
				{\
					if( d2 > m ) d2 = m;\
					x3 += d2;\
				}\
			}\
			else\
			{\
				unsigned a3;\
				if( (int)(a3 = d2 + d1 - 2*d3) < 0 )\
				{\
					if( (d2 -= d3) > 0 )\
					{\
						if( d2 > m ) d2 = m;\
						x2 -= d2;\
					}\
				}\
				else\
				{\
					if( (a3 = (a3 * ONETHIRDM) >> ONETHIRDS) > (unsigned) m ) a3 = m;\
					x3 += (BYTE) a3;\
					if( (a2 = (a2 * ONETHIRDM) >> ONETHIRDS) > (unsigned) m ) a2 = m;\
					x2 -= (BYTE) a2;\
				}\
			}\
		}\
	}\
	else /* d2 <= 0 */ \
	{\
		d2 = -d2;\
		int m;\
		if( (m = (int)x4 - (int)x1) > 0 )\
		{\
			int d1, d3;\
			if( (d1 = (int)x1 - (int)x2) < 0 ) d1 = -d1;\
			if( (d3 = (int)x3 - (int)x4) < 0 ) d3 = -d3;\
			unsigned	a2;\
			if( (int)(a2 = d2 + d3 - 2*d1) < 0 )\
			{\
				if( (d2 -= d1) > 0 )\
				{\
					if( d2 > m ) d2 = m;\
					x3 -= d2;\
				}\
			}\
			else\
			{\
				unsigned a3;\
				if( (int)(a3 = d2 + d1 - 2*d3) < 0 )\
				{\
					if( (d2 -= d3) > 0 )\
					{\
						if( d2 > m ) d2 = m;\
						x2 += d2;\
					}\
				}\
				else\
				{\
					if( (a3 = (a3 * ONETHIRDM) >> ONETHIRDS) > (unsigned) m ) a3 = m;\
					x3 -= (BYTE) a3;\
					if( (a2 = (a2 * ONETHIRDM) >> ONETHIRDS) > (unsigned) m ) a2 = m;\
					x2 += (BYTE) a2;\
				}\
			}\
		}\
	}\
}


static void	deblock1h(BYTE *p, int pitch, int dist, int hblocks, int height, int offset)
{
	p += offset;
	pitch -= (dist * hblocks);
	do
	{
		int	i = hblocks;
		do
		{
#ifdef		FDEBUG
			int s1 = p[0] - p[1], s2 = p[1] - p[2], s3 = p[2] - p[3];
			if (s1 < 0) s1 = -s1;
			if (s2 < 0) s2 = -s2;
			if (s3 < 0) s3 = -s3;
#endif	// FDEBUG
			deblock1(p[0], p[1], p[2], p[3])
#ifdef		FDEBUG
				int u1 = p[0] - p[1], u2 = p[1] - p[2], u3 = p[2] - p[3];
			if (u1 < 0) u1 = -u1;
			if (u2 < 0) u2 = -u2;
			if (u3 < 0) u3 = -u3;
			if ((s1 != u1) && (u3 < u2 - 1)) debug_printf("u2 - u3 = %i\n", u2 - u3);
			// if( ((s1 != u1) || (s3 != u3)) & ((u1 > u2) || (u2 > s2) || (u3 > u2)) ) debug_printf("fluctuation increase\n");
#endif	// FDEBUG
			p += dist;
		} while (--i);
		p += pitch;
	} while (--height);
}

static void	deblock1v(BYTE *p, int pitch, int dist, int vblocks, int width, int offset)
{
	p += offset * pitch;
	dist = (dist * pitch) - width;
	do
	{
		int	i = width;
		do
		{
			deblock1(p[0], p[pitch], p[2 * pitch], p[3 * pitch])
				++p;
		} while (--i);
		p += dist;
	} while (--vblocks);
}

static void	deblock2h(BYTE *p, int pitch, int dist, int hblocks, int height, int offset)
{
	p += offset;
	pitch -= (dist * hblocks);
	do
	{
		int	i = hblocks;
		do
		{
			deblock2(p[0], p[1], p[2], p[3])
				p += dist;
		} while (--i);
		p += pitch;
	} while (--height);
}

static void	deblock2v(BYTE *p, int pitch, int dist, int vblocks, int width, int offset)
{
	p += offset * pitch;
	dist = (dist * pitch) - width;
	do
	{
		int	i = width;
		do
		{
			deblock2(p[0], p[pitch], p[2 * pitch], p[3 * pitch])
				++p;
		} while (--i);
		p += dist;
	} while (--vblocks);
}

static void	deblock3h(BYTE *p, int pitch, int dist, int hblocks, int height, int offset)
{
	p += offset;
	pitch -= (dist * hblocks);
	do
	{
		int	i = hblocks;
		do
		{
			deblock3(p[0], p[1], p[2], p[3])
				p += dist;
		} while (--i);
		p += pitch;
	} while (--height);
}

static void	deblock3v(BYTE *p, int pitch, int dist, int vblocks, int width, int offset)
{
	p += offset * pitch;
	dist = (dist * pitch) - width;
	do
	{
		int	i = width;
		do
		{
			deblock3(p[0], p[pitch], p[2 * pitch], p[3 * pitch])
				++p;
		} while (--i);
		p += dist;
	} while (--vblocks);
}

static void	deblock4h(BYTE *p, int pitch, int dist, int hblocks, int height, int offset)
{
	p += offset;
	pitch -= (dist * hblocks);
	do
	{
		int	i = hblocks;
		do
		{
			deblock4(p[0], p[1], p[2], p[3])
				p += dist;
		} while (--i);
		p += pitch;
	} while (--height);
}

static void	deblock4v(BYTE *p, int pitch, int dist, int vblocks, int width, int offset)
{
	p += offset * pitch;
	dist = (dist * pitch) - width;
	do
	{
		int	i = width;
		do
		{
			deblock4(p[0], p[pitch], p[2 * pitch], p[3 * pitch])
				++p;
		} while (--i);
		p += dist;
	} while (--vblocks);
}



#if	0
static void	deblock2h(BYTE *p, int pitch, int dist, int hblocks, int height, int offset)
{
	p += offset;
	pitch -= (dist * hblocks);
	do
	{
		int	i = hblocks;
		do
		{
			deblock2(p[0], p[1], p[2], p[3], p[4], p[5])
				p += dist;
		} while (--i);
		p += pitch;
	} while (--height);
}

static void	deblock2v(BYTE *p, int pitch, int dist, int vblocks, int width, int offset)
{
	p += offset * pitch;
	dist = (dist * pitch) - width;
	do
	{
		int	i = width;
		do
		{
			deblock2(p[0], p[pitch], p[2 * pitch], p[3 * pitch], p[4 * pitch], p[5 * pitch])
				++p;
		} while (--i);
		p += dist;
	} while (--vblocks);
}
#endif


static void(*const hdeblock[])(BYTE *p, int pitch, int dist, int hblocks, int height, int offset)
= { nodeblock, deblock1h, deblock2h, deblock3h, deblock4h };
static void(*const vdeblock[])(BYTE *p, int pitch, int dist, int hblocks, int height, int offset)
= { nodeblock, deblock1v, deblock2v, deblock3v, deblock4v };


static int const plane[3] = { PLANAR_Y, PLANAR_U, PLANAR_V };

class Deblock : public GenericVideoFilter
{
	void(*proch[3])(BYTE *p, int pitch, int dist, int hblocks, int height, int offset);
	void(*procv[3])(BYTE *p, int pitch, int dist, int vblocks, int width, int offset);

	int	disth[3];
	int	distv[3];
	int	hblocks[3];
	int	vblocks[3];
	int	width[3];
	int	height[3];
	int	offseth[3];
	int	offsetv[3];

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		PVideoFrame frame = child->GetFrame(n, env);
		env->MakeWritable(&frame);

		int	i = 2;
		do
		{
			int	pitch = frame->GetPitch(plane[i]);
			BYTE *p = frame->GetWritePtr(plane[i]);
			proch[i](p, pitch, disth[i], hblocks[i], height[i], offseth[i]);
			procv[i](p, pitch, distv[i], vblocks[i], width[i], offsetv[i]);
		} while (--i >= 0);

		return frame;
	};

public:
	Deblock(PClip child, const char *mode, int x, int y, int _disth, int _distv, int cdisth, int cdistv) : GenericVideoFilter(child)
	{
		if (!vi.IsYV12())
		{
			AVSenvironment->ThrowError("MDeblock: only YV12 clips are supported");
		negativ_offset:
			AVSenvironment->ThrowError("MDeblock: offset variables must be >= 0");
		illegal_mode:
			AVSenvironment->ThrowError("MDeblock: incorrect mode string");
		too_small:
			AVSenvironment->ThrowError("MDeblock: block width or height too small");
		}
		if (strlen(mode) < 6) goto illegal_mode;
		width[2] = width[1] = (width[0] = vi.width) / 2;
		height[2] = height[1] = (height[0] = vi.height) / 2;
		disth[0] = _disth;
		disth[2] = disth[1] = cdisth;
		distv[0] = _distv;
		distv[2] = distv[1] = cdistv;
		if ((x < 0) || (y < 0)) goto negativ_offset;
		offseth[2] = offseth[1] = (offseth[0] = x) / 2;
		offsetv[2] = offsetv[1] = (offsetv[0] = y) / 2;
		int	i = 2;
		do
		{
			int	hmode = mode[0] - '0', vmode = mode[1] - '0'; mode += 2;
			if (((unsigned)hmode > MAXMODE) || ((unsigned)vmode > MAXMODE)) goto illegal_mode;
			proch[i] = hdeblock[hmode];
			procv[i] = vdeblock[vmode];

			if (disth[i] < 2 * hmode + 1) goto too_small;
			if ((offseth[i] = disth[i] - hmode - 1 - offseth[i] % disth[i]) < 0) offseth[i] += disth[i];
			if ((hblocks[i] = (width[i] - offseth[i] - 2 * (hmode + 1)) / disth[i]) <= 0) proch[i] = nodeblock;
			hblocks[i]++;

			if (distv[i] < 2 * vmode + 1) goto too_small;
			if ((offsetv[i] = distv[i] - vmode - 1 - offsetv[i] % distv[i]) < 0) offsetv[i] += distv[i];
			if ((vblocks[i] = (height[i] - offsetv[i] - 2 * (vmode + 1)) / distv[i]) <= 0) procv[i] = nodeblock;
			vblocks[i]++;
		} while (--i >= 0);
	}
};


AVSValue __cdecl CreateDeblock(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, MODE, DISTH, DISTV, CDISTH, CDISTV, OFFSETH, OFFSETV };
	return new Deblock(args[CLIP].AsClip(), args[MODE].AsString("222222"), args[OFFSETH].AsInt(0), args[OFFSETV].AsInt(0)
		, args[DISTH].AsInt(8), args[DISTV].AsInt(8)
		, args[CDISTH].AsInt(8), args[CDISTV].AsInt(8));
}

const AVS_Linkage *AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
	AVS_linkage = vectors;
	AVSenvironment = env;
	env->AddFunction("MDeblock", "c[mode]s[width]i[height]i[cwidth]i[cheight]i[leftcrop]i[topcrop]i", CreateDeblock, 0);
	//debug_printf(LOGO);
	return 0;
}





