/*
 * RageTypes - vector and matrix types
 */

#ifndef RAGETYPES_H
#define RAGETYPES_H

enum GlowMode { GLOW_BRIGHTEN=0, GLOW_WHITEN };
enum BlendMode { BLEND_NORMAL=0, BLEND_ADD, BLEND_NO_EFFECT, BLEND_INVALID };
enum CullMode { CULL_BACK=0, CULL_FRONT, CULL_NONE };
enum ZTestMode { ZTEST_OFF=0, ZTEST_WRITE_ON_PASS, ZTEST_WRITE_ON_FAIL };
enum PolygonMode { POLYGON_FILL=0, POLYGON_LINE };


struct RageVector2
{
public:
    RageVector2() {}
    RageVector2( const float * f )								{ x=f[0]; y=f[1]; }
	RageVector2( float x1, float y1 )							{ x=x1; y=y1; }

    // casting
	operator float* ()											{ return &x; }
    operator const float* () const								{ return &x; }

    // assignment operators
	RageVector2& operator += ( const RageVector2& other )		{ x+=other.x; y+=other.y; return *this; }
    RageVector2& operator -= ( const RageVector2& other )		{ x-=other.x; y-=other.y; return *this; }
    RageVector2& operator *= ( float f )						{ x*=f; y*=f; return *this; }
    RageVector2& operator /= ( float f )						{ x/=f; y/=f; return *this; }

    // binary operators
    RageVector2 operator + ( const RageVector2& other ) const	{ return RageVector2( x+other.x, y+other.y ); }
    RageVector2 operator - ( const RageVector2& other ) const	{ return RageVector2( x-other.x, y-other.y ); }
    RageVector2 operator * ( float f ) const					{ return RageVector2( x*f, y*f ); }
    RageVector2 operator / ( float f ) const					{ return RageVector2( x/f, y/f ); }

	friend RageVector2 operator * ( float f, const RageVector2& other )	{ return other*f; }
 
    float x, y;
};


struct RageVector3
{
public:
	RageVector3() {}
	RageVector3( const float * f )								{ x=f[0]; y=f[1]; z=f[2]; }
	RageVector3( float x1, float y1, float z1 )					{ x=x1; y=y1; z=z1; }

	// casting
	operator float* ()											{ return &x; }
	operator const float* () const								{ return &x; }

	// assignment operators
	RageVector3& operator += ( const RageVector3& other )		{ x+=other.x; y+=other.y; z+=other.z; return *this; }
	RageVector3& operator -= ( const RageVector3& other )		{ x-=other.x; y-=other.y; z-=other.z; return *this; }
#ifdef PSP
	RageVector3& operator *= ( float f )
	{
		__asm__ volatile (
			"mtv		%1,   S010\n"
			"lv.s		S000, 0 + %0\n"
			"lv.s		S001, 4 + %0\n"
			"lv.s		S002, 8 + %0\n"
			"vscl.t		C000, C000, S010\n"
			"sv.s		S000, 0 + %0\n"
			"sv.s		S001, 4 + %0\n"
			"sv.s		S002, 8 + %0\n"
			: "+m"(*this)
			: "r"(f)
		);
		return *this;
	}
	RageVector3& operator /= ( float f )
	{
		__asm__ volatile (
			"mtv		%1,   S010\n"
			"vrcp.s		S010, S010\n"
			"lv.s		S000, 0 + %0\n"
			"lv.s		S001, 4 + %0\n"
			"lv.s		S002, 8 + %0\n"
			"vscl.t		C000, C000, S010\n"
			"sv.s		S000, 0 + %0\n"
			"sv.s		S001, 4 + %0\n"
			"sv.s		S002, 8 + %0\n"
			: "+m"(*this)
			: "r"(f)
		);
		return *this;
	}
#else
    RageVector3& operator *= ( float f )						{ x*=f; y*=f; z*=f; return *this; }
    RageVector3& operator /= ( float f )						{ x/=f; y/=f; z/=f; return *this; }
#endif

    // binary operators
    RageVector3 operator + ( const RageVector3& other ) const	{ return RageVector3( x+other.x, y+other.y, z+other.z ); }
    RageVector3 operator - ( const RageVector3& other ) const	{ return RageVector3( x-other.x, y-other.y, z-other.z ); }



#ifdef PSP
	RageVector3 operator * ( const RageVector3& other ) const
	{
		RageVector3 v;
		__asm__ volatile (
			"lv.s		S000, 0 + %1\n"
			"lv.s		S001, 4 + %1\n"
			"lv.s		S002, 8 + %1\n"
			"lv.s		S010, 0 + %2\n"
			"lv.s		S011, 4 + %2\n"
			"lv.s		S012, 8 + %2\n"
			"vmul.t		C000, C000, C010\n"
			"sv.s		S000, 0 + %0\n"
			"sv.s		S001, 4 + %0\n"
			"sv.s		S002, 8 + %0\n"
			: "=m"(v)
			: "m"(*this), "m"(other)
		);
		return v;
	}
	RageVector3 operator * ( float f ) const
	{
		RageVector3 v;
		__asm__ volatile (
			"mtv		%2,   S010\n"
			"lv.s		S000, 0 + %1\n"
			"lv.s		S001, 4 + %1\n"
			"lv.s		S002, 8 + %1\n"
			"vscl.t		C000, C000, S010\n"
			"sv.s		S000, 0 + %0\n"
			"sv.s		S001, 4 + %0\n"
			"sv.s		S002, 8 + %0\n"
			: "=m"(v)
			: "m"(*this), "r"(f)
		);
		return v;
	}
	RageVector3 operator / ( float f ) const
	{
		RageVector3 v;
		__asm__ volatile (
			"mtv		%2,   S010\n"
			"vrcp.s		S010, S010\n"
			"lv.s		S000, 0 + %1\n"
			"lv.s		S001, 4 + %1\n"
			"lv.s		S002, 8 + %1\n"
			"vscl.t		C000, C000, S010\n"
			"sv.s		S000, 0 + %0\n"
			"sv.s		S001, 4 + %0\n"
			"sv.s		S002, 8 + %0\n"
			: "=m"(v)
			: "m"(*this), "r"(f)
		);
		return v;
	}
#else
	RageVector3 operator * ( const RageVector3& other ) const	{ return RageVector3( x*other.x, y*other.y, z*other.z ); }
	RageVector3 operator * ( float f ) const					{ return RageVector3( x*f, y*f, z*f ); }
	RageVector3 operator / ( float f ) const					{ return RageVector3( x/f, y/f, z/f ); }
#endif

	friend RageVector3 operator * ( float f, const RageVector3& other )	{ return other*f; }

	float x, y, z;
};


struct RageVector4
{
public:
    RageVector4() {}
    RageVector4( const float * f )								{ memcpy( this, f, sizeof(RageVector4) ); }
	RageVector4( float x1, float y1, float z1, float w1 )		{ x=x1; y=y1; z=z1; w=w1; }

    // casting
	operator float* ()											{ return &x; }
    operator const float* () const								{ return &x; }

#ifdef PSP
	RageVector4& operator += ( const RageVector4& other )
	{
		__asm__ volatile (
			"ulv.q 		C010, %0\n"
			"ulv.q		C020, %1\n"
			"vadd.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "+m"(*this)
			: "m"(other)
		);
		return *this;
	}
	
	RageVector4& operator -= ( const RageVector4& other )
	{
		__asm__ volatile (
			"ulv.q 		C010, %0\n"
			"ulv.q		C020, %1\n"
			"vsub.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "+m"(*this)
			: "m"(other)
		);
		return *this;
	}

	RageVector4& operator *= ( float f )
	{
		__asm__ volatile (
			"mtv		%1,   S010\n"
			"ulv.q		C000, %0\n"
			"vscl.q		C000, C000, S010\n"
			"usv.q		C000, %0\n"
			: "+m"(*this)
			: "r"(f)
		);
		return *this;
	}
	
	RageVector4& operator /= ( float f )
	{
		__asm__ volatile (
			"mtv		%1,   S010\n"
			"vrcp.s		S010, S010\n"
			"ulv.q		C000, %0\n"
			"vscl.q		C000, C000, S010\n"
			"usv.q		C000, %0\n"
			: "+m"(*this)
			: "r"(f)
		);
		return *this;
	}

	RageVector4 operator + ( const RageVector4& other ) const
	{
		RageVector4 v;
		__asm__ volatile (
			"ulv.q 		C010, %1\n"
			"ulv.q		C020, %2\n"
			"vadd.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "=m"(v)
			: "m"(*this), "m"(other)
		);
		return v;
	}

	RageVector4 operator - ( const RageVector4& other ) const
	{
		RageVector4 v;
		__asm__ volatile (
			"ulv.q 		C010, %1\n"
			"ulv.q		C020, %2\n"
			"vsub.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "=m"(v)
			: "m"(*this), "m"(other)
		);
		return v;
	}

	RageVector4 operator * ( float f ) const
	{
		RageVector4 v;
		__asm__ volatile (
			"mtv		%2,   S010\n"
			"ulv.q		C000, %1\n"
			"vscl.q		C000, C000, S010\n"
			"usv.q		C000, %0\n"
			: "=m"(v)
			: "m"(*this), "r"(f)
		);
		return v;
	}

	RageVector4 operator / ( float f ) const
	{
		RageVector4 v;
		__asm__ volatile (
			"mtv		%2,   S010\n"
			"vrcp.s		S010, S010\n"
			"ulv.q		C000, %1\n"
			"vscl.q		C000, C000, S010\n"
			"usv.q		C000, %0\n"
			: "=m"(v)
			: "m"(*this), "r"(f)
		);
		return v;
	}
#else
    // assignment operators
	RageVector4& operator += ( const RageVector4& other )		{ x+=other.x; y+=other.y; z+=other.z; w+=other.w; return *this; }
    RageVector4& operator -= ( const RageVector4& other )		{ x-=other.x; y-=other.y; z-=other.z; w-=other.w; return *this; }
    RageVector4& operator *= ( float f )						{ x*=f; y*=f; z*=f; w*=f; return *this; }
    RageVector4& operator /= ( float f )						{ x/=f; y/=f; z/=f; w/=f; return *this; }

    // binary operators
    RageVector4 operator + ( const RageVector4& other ) const	{ return RageVector4( x+other.x, y+other.y, z+other.z, w+other.w ); }
    RageVector4 operator - ( const RageVector4& other ) const	{ return RageVector4( x-other.x, y-other.y, z-other.z, w-other.w ); }
    RageVector4 operator * ( float f ) const					{ return RageVector4( x*f, y*f, z*f, w*f ); }
    RageVector4 operator / ( float f ) const					{ return RageVector4( x/f, y/f, z/f, w/f ); }
#endif

	friend RageVector4 operator * ( float f, const RageVector4& other )	{ return other*f; }

	float x, y, z, w;
};

struct RageColor
{
public:
    RageColor() {}
    RageColor( const float * f )							{ memcpy( this, f, sizeof(RageColor) ); }
	RageColor( float r1, float g1, float b1, float a1 )		{ r=r1; g=g1; b=b1; a=a1; }

    // casting
	operator float* ()										{ return &r; }
    operator const float* () const							{ return &r; }

#ifdef PSP
	RageColor& operator += ( const RageColor& other )
	{
		__asm__ volatile (
			"ulv.q 		C010, %0\n"
			"ulv.q		C020, %1\n"
			"vadd.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "+m"(*this)
			: "m"(other)
		);
		return *this;
	}

	RageColor& operator -= ( const RageColor& other )
	{
		__asm__ volatile (
			"ulv.q 		C010, %0\n"
			"ulv.q		C020, %1\n"
			"vsub.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "+m"(*this)
			: "m"(other)
		);
		return *this;
	}

	RageColor& operator *= ( const RageColor& other )
	{
		__asm__ volatile (
			"ulv.q 		C010, %0\n"
			"ulv.q		C020, %1\n"
			"vmul.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "+m"(*this)
			: "m"(other)
		);
		return *this;
	}

	RageColor& operator *= ( float f )
	{
		__asm__ volatile (
			"mtv		%1,   S010\n"
			"ulv.q		C000, %0\n"
			"vscl.q		C000, C000, S010\n"
			"usv.q		C000, %0\n"
			: "+m"(*this)
			: "r"(f)
		);
		return *this;
	}

	RageColor operator + ( const RageColor& other ) const
	{
		RageColor c;
		__asm__ volatile (
			"ulv.q 		C010, %1\n"
			"ulv.q		C020, %2\n"
			"vadd.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "=m"(c)
			: "m"(*this), "m"(other)
		);
		return c;
	}

	RageColor operator - ( const RageColor& other ) const
	{
		RageColor c;
		__asm__ volatile (
			"ulv.q 		C010, %1\n"
			"ulv.q		C020, %2\n"
			"vsub.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "=m"(c)
			: "m"(*this), "m"(other)
		);
		return c;
	}

	RageColor operator * ( const RageColor& other ) const
	{
		RageColor c;
		__asm__ volatile (
			"ulv.q 		C010, %1\n"
			"ulv.q		C020, %2\n"
			"vmul.q		C000, C010, C020\n"
			"usv.q		C000, %0\n"
			: "=m"(c)
			: "m"(*this), "m"(other)
		);
		return c;
	}

	RageColor operator * ( float f ) const
	{
		RageColor c;
		__asm__ volatile (
			"mtv		%2,   S010\n"
			"ulv.q		C000, %1\n"
			"vscl.q		C000, C000, S010\n"
			"usv.q		C000, %0\n"
			: "=m"(c)
			: "m"(*this), "r"(f)
		);
		return c;
	}
#else
    // assignment operators
	RageColor& operator += ( const RageColor& other )		{ r+=other.r; g+=other.g; b+=other.b; a+=other.a; return *this; }
    RageColor& operator -= ( const RageColor& other )		{ r-=other.r; g-=other.g; b-=other.b; a-=other.a; return *this; }
    RageColor& operator *= ( const RageColor& other )		{ r*=other.r; g*=other.g; b*=other.b; a*=other.a; return *this; }
    RageColor& operator *= ( float f )						{ r*=f; g*=f; b*=f; a*=f; return *this; }
	/* Divide is rarely useful: you can always use multiplication, and you don't have to
	 * worry about div/0. */
//    RageColor& operator /= ( float f )						{ r/=f; g/=f; b/=f; a/=f; return *this; }

    // binary operators
    RageColor operator + ( const RageColor& other ) const	{ return RageColor( r+other.r, g+other.g, b+other.b, a+other.a ); }
    RageColor operator - ( const RageColor& other ) const	{ return RageColor( r-other.r, g-other.g, b-other.b, a-other.a ); }
    RageColor operator * ( const RageColor& other ) const	{ return RageColor( r*other.r, g*other.g, b*other.b, a*other.a ); }
    RageColor operator * ( float f ) const					{ return RageColor( r*f, g*f, b*f, a*f ); }
//    RageColor operator / ( float f ) const					{ return RageColor( r/f, g/f, b/f, a/f ); }
#endif

	friend RageColor operator * ( float f, const RageColor& other )	{ return other*f; }		// What is this for?  Did I add this?  -Chris

	bool FromString( const CString &str )
	{
		int result = sscanf( str, "%f,%f,%f,%f", &r, &g, &b, &a );
		if( result == 3 )
		{
			a = 1;
			return true;
		}
		if( result == 4 )
			return true;

		int ir=255, ib=255, ig=255, ia=255;
		result = sscanf( str, "#%2x%2x%2x%2x", &ir, &ig, &ib, &ia );
		if( result >= 3 )
		{
			r = ir / 255.0f; g = ig / 255.0f; b = ib / 255.0f;
			if( result == 4 )
				a = ia / 255.0f;
			else
				a = 1;
			return true;
		}

		r=1; b=1; g=1; a=1;
		return false;
	}
	float r, g, b, a;
};

#ifndef PSP
/* Convert floating-point 0..1 value to integer 0..255 value. *
 *
 * As a test case,
 *
 * int cnts[1000]; memset(cnts, 0, sizeof(cnts));
 * for( float n = 0; n <= 1.0; n += 0.0001 ) cnts[FTOC(n)]++;
 * for( int i = 0; i < 256; ++i ) printf("%i ", cnts[i]);
 *
 * should output the same value (+-1) 256 times.  If this function is
 * incorrect, the first and/or last values may be biased. */
inline unsigned char FTOC(float a)
{
        /* lfintf is much faster than C casts.  We don't care which way negative values
         * are rounded, since we'll clamp them to zero below.  Be sure to truncate (not
         * round) positive values.  The highest value that should be converted to 1 is
         * roughly (1/256 - 0.00001); if we don't truncate, values up to (1/256 + 0.5)
         * will be converted to 1, which is wrong. */
        int ret = lrintf(a*256.f - 0.5f);

        /* Benchmarking shows that clamping here, as integers, is much faster than clamping
         * before the conversion, as floats. */
        if( ret<0 ) return 0;
        else if( ret>255 ) return 255;
        else return (unsigned char) ret;
}
#endif

/* Color type used only in vertex lists.  OpenGL expects colors in
 * r, g, b, a order, independent of endianness, so storing them this
 * way avoids endianness problems.  Don't try to manipulate this; only
 * manip RageColors. */
/* Perhaps the math in RageColor could be moved to RaveVColor.  We don't need the 
 * precision of a float for our calculations anyway.   -Chris */
struct RageVColor
{
#ifdef PSP
	unsigned char r,g,b,a;	// specific ordering required by PSP

	RageVColor() {}
	RageVColor( const RageVColor& other ) { *this = other; }
	RageVColor( const RageColor& rc ) { *this = rc; }

	RageVColor& operator= ( const RageVColor& rvc )
	{
		*(uint32_t*)this = *(uint32_t*)&rvc;
		return *this;
	}

	RageVColor& operator= ( const RageColor& rc )
	{
		__asm__ volatile (
			"ulv.q		C000, %1\n"
			"vsat0.q	C000, C000\n" // rc = max(min(rc, 1), 0)
			"viim.s		S010, 255\n"
			"vscl.q		C000, C000, S010\n"
			"vf2in.q	C000, C000, 23\n"
			"vi2uc.q	S000, C000\n"
			"usv.s		S000, %0\n"
			: "=m"(*this)
			: "m"(rc)
		);
		return *this;
	}
#else
	unsigned char b,g,r,a;	// specific ordering required by Direct3D

	RageVColor(): b(0), g(0), r(0), a(0) { }
	RageVColor(const RageColor &rc) { *this = rc; }
	RageVColor &operator= (const RageColor &rc) {
		r = FTOC(rc.r); g = FTOC(rc.g); b = FTOC(rc.b); a = FTOC(rc.a);
		return *this;
	}
#endif
	// casting
	operator uint32_t () const { return *(uint32_t*)this; }

	bool operator==( const RageVColor& other ) const
	{
		return *(uint32_t*)this == *(uint32_t*)&other;
	}
};


template <class T>
class Rect
{
public:
	Rect()						{}
	Rect(T l, T t, T r, T b)	{ left = l, top = t, right = r, bottom = b; }

	T GetWidth() const		{ return right-left; }
	T GetHeight() const		{ return bottom-top;  }
	T GetCenterX() const	{ return (left+right)/2; }
	T GetCenterY() const	{ return (top+bottom)/2; }

    T    left, top, right, bottom;
};

typedef Rect<int> RectI;
typedef Rect<float> RectF;

struct RageSpriteVertex	// has color
{
	/* Note that these data structes have the same layout that PSP expects. */
	RageSpriteVertex() {}
	RageSpriteVertex( const RageSpriteVertex& other )	{ memcpy( this, &other, sizeof(RageSpriteVertex) ); }
	RageSpriteVertex& operator= ( const RageSpriteVertex& other )
	{
		memcpy( this, &other, sizeof(RageSpriteVertex) );
		return *this;
	}

	RageVector2		t;	// texture coordinates
	RageVColor		c;	// diffuse color
//	RageVector3		n;	// normal
	RageVector3		p;	// position
};


struct RageModelVertex	// doesn't have color.  Relies on material color
{
	/* Zero out by default. */
	RageModelVertex():
		p(0,0,0),
		n(0,0,0),
		t(0,0)
		{ }
    RageVector3		p;	// position
    RageVector3		n;	// normal
	RageVector2		t;	// texture coordinates
	int8_t			bone;
};


// RageMatrix elements are specified in row-major order.  This
// means that the translate terms are located in the fourth row and the
// projection terms in the fourth column.  This is consistent with the way
// MAX, Direct3D, and OpenGL all handle matrices.  Even though the OpenGL
// documentation is in column-major form, the OpenGL code is designed to
// handle matrix operations in row-major form.
struct RageMatrix
{
public:
    RageMatrix() {};
	RageMatrix( const float *f )			{ memcpy( m, f, sizeof(m) ); }
	RageMatrix( const RageMatrix& other )	{ memcpy( m, &other.m, sizeof(m) ); }
    RageMatrix( float v00, float v01, float v02, float v03,
                float v10, float v11, float v12, float v13,
                float v20, float v21, float v22, float v23,
                float v30, float v31, float v32, float v33 );

    // access grants
	float& operator () ( int iRow, int iCol )		{ return m[iCol][iRow]; }
    float  operator () ( int iRow, int iCol ) const { return m[iCol][iRow]; }

    // casting operators
    operator float* ()								{ return m[0]; }
    operator const float* () const					{ return m[0]; }

	RageMatrix& operator= ( const RageMatrix& other )
	{
		memcpy( m, &other.m, sizeof(m) );
		return *this;
	}

    void GetTranspose( RageMatrix *pOut ) const;

	float m[4][4];
};

RageColor scale( float x, float l1, float h1, const RageColor &a, const RageColor &b );

#endif

/*
 * Copyright (c) 2001-2002 Chris Danford
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
