/*
 * PSP has a VFPU(Vector Floating Point Unit) that can calculate so fast.
 * I am not good at math, so i have no confidence.
 *
 * MEMO: VFPU Functions List
 *   http://wiki.fx-world.org/doku.php?id=general:cycles
 *   http://forums.ps2dev.org/viewtopic.php?t=6929
 */

#include "global.h"
#include "StepMania.h"
#include "RageMath.h"
#include "RageTypes.h"
#include "RageUtil.h"
#include "RageDisplay.h"
#include "RageLog.h"
#include "arch/Dialog/Dialog.h"

#ifdef PSP
void RageVec3Roundf( RageVector3* pOut, const RageVector3* pV )
{
#if 1
	__asm__ volatile (
		"lv.s		S000, 0 + %1\n"
		"lv.s		S010, 4 + %1\n"
		"lv.s		S020, 8 + %1\n"
		"vf2in.t	R000, R000, 0\n"
		"vi2f.t		R000, R000, 0\n"
		"sv.s		S000, 0 + %0\n"
		"sv.s		S010, 4 + %0\n"
		"sv.s		S020, 8 + %0\n"
		: "=m"(*pOut)
		: "m"(*pV)
	);
#else
	pOut->x = roundf( pV->x );
	pOut->y = roundf( pV->y );
	pOut->z = roundf( pV->z );
#endif
}
#endif

void RageVec3ClearBounds( RageVector3 &mins, RageVector3 &maxs )
{
	mins = RageVector3( 999999, 999999, 999999 );
	maxs = mins * -1;
}

void RageVec3AddToBounds( const RageVector3 &p, RageVector3 &mins, RageVector3 &maxs )
{
	mins.x = min( mins.x, p.x );
	mins.y = min( mins.y, p.y );
	mins.z = min( mins.z, p.z );
	maxs.x = max( maxs.x, p.x );
	maxs.y = max( maxs.y, p.y );
	maxs.z = max( maxs.z, p.z );
}

void RageVec2Normalize( RageVector2* pOut, const RageVector2* pV )
{
	float scale = 1.0f / sqrtf( pV->x*pV->x + pV->y*pV->y );
	pOut->x = pV->x * scale;
	pOut->y = pV->y * scale;
}

void RageVec3Normalize( RageVector3* pOut, const RageVector3* pV )
{
#ifdef PSP
	__asm__ volatile (
		"lv.s		S000, 0 + %1\n"
		"lv.s		S010, 4 + %1\n"
		"lv.s		S020, 8 + %1\n"
		"vdot.t		S001, R000, R000\n"
		"vrsq.s		S001, S001\n"
		"vscl.t		R000, R000, S001\n"
		"sv.s		S000, 0 + %0\n"
		"sv.s		S010, 4 + %0\n"
		"sv.s		S020, 8 + %0\n"
		: "=m"(*pOut)
		: "m"(*pV)
	);
#else
	float scale = 1.0f / sqrtf( pV->x*pV->x + pV->y*pV->y + pV->z*pV->z );
	pOut->x = pV->x * scale;
	pOut->y = pV->y * scale;
	pOut->z = pV->z * scale;
#endif
}

void RageVec3TransformCoord( RageVector3* pOut, const RageVector3* pV, const RageMatrix* pM )
{
	RageVector4 temp( pV->x, pV->y, pV->z, 1.0f );	// translate
	RageVec4TransformCoord( &temp, &temp, pM );
	*pOut = RageVector3( temp.x/temp.w, temp.y/temp.w, temp.z/temp.w );
}

void RageVec3TransformNormal( RageVector3* pOut, const RageVector3* pV, const RageMatrix* pM )
{
	RageVector4 temp( pV->x, pV->y, pV->z, 0.0f );	// don't translate
	RageVec4TransformCoord( &temp, &temp, pM );
	*pOut = RageVector3( temp.x, temp.y, temp.z );
}

#define m00 m[0][0]
#define m01 m[0][1]
#define m02 m[0][2]
#define m03 m[0][3]
#define m10 m[1][0]
#define m11 m[1][1]
#define m12 m[1][2]
#define m13 m[1][3]
#define m20 m[2][0]
#define m21 m[2][1]
#define m22 m[2][2]
#define m23 m[2][3]
#define m30 m[3][0]
#define m31 m[3][1]
#define m32 m[3][2]
#define m33 m[3][3]

void RageVec4TransformCoord( RageVector4* pOut, const RageVector4* pV, const RageMatrix* pM )
{
#ifdef PSP
	__asm__ volatile (
		"ulv.q		R000,  0 + %1\n"
		"ulv.q		R001, 16 + %1\n"
		"ulv.q		R002, 32 + %1\n"
		"ulv.q		R003, 48 + %1\n"
		"ulv.q		R100, %2\n"
		"vtfm4.q	R101, M000, R100\n"
		"usv.q		R101, %0\n"
		: "=m"(*pOut)
		: "m"(*pM), "m"(*pV)
	);
#else
    const RageMatrix &a = *pM;
    const RageVector4 &v = *pV;
	*pOut = RageVector4(
		a.m00*v.x+a.m10*v.y+a.m20*v.z+a.m30*v.w,
		a.m01*v.x+a.m11*v.y+a.m21*v.z+a.m31*v.w,
		a.m02*v.x+a.m12*v.y+a.m22*v.z+a.m32*v.w,
		a.m03*v.x+a.m13*v.y+a.m23*v.z+a.m33*v.w );
#endif
}

RageMatrix::RageMatrix( float v00, float v01, float v02, float v03,
						float v10, float v11, float v12, float v13,
						float v20, float v21, float v22, float v23,
						float v30, float v31, float v32, float v33 )
{
	m00=v00; m01=v01; m02=v02; m03=v03;
	m10=v10; m11=v11; m12=v12; m13=v13;
	m20=v20; m21=v21; m22=v22; m23=v23;
	m30=v30; m31=v31; m32=v32; m33=v33;
}

void RageMatrixIdentity( RageMatrix* pOut )
{
#ifdef PSP
	/* this function is also called by "SceModmgrStart" thread *
	 * that forbid to use VFPU before the main function.       */
	if( g_argv != NULL )
	{
		__asm__ volatile (
			"vmidt.q	M000\n"
			"usv.q		R000,  0 + %0\n"
			"usv.q		R001, 16 + %0\n"
			"usv.q		R002, 32 + %0\n"
			"usv.q		R003, 48 + %0\n"
			: "=m"(*pOut)
		);
		return;
	}
#endif
	*pOut = RageMatrix(
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1 );
}

void RageMatrix::GetTranspose( RageMatrix *pOut ) const
{
#ifdef PSP
	__asm__ volatile (
		"ulv.q		R000,  0 + %1\n"
		"ulv.q		R001, 16 + %1\n"
		"ulv.q		R002, 32 + %1\n"
		"ulv.q		R003, 48 + %1\n"
		"usv.q		C000,  0 + %0\n"
		"usv.q		C010, 16 + %0\n"
		"usv.q		C020, 32 + %0\n"
		"usv.q		C030, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(*this)
	);
#else
	*pOut = RageMatrix(m00,m10,m20,m30,m01,m11,m21,m31,m02,m12,m22,m32,m03,m13,m23,m33);
#endif
}

void RageMatrixMultiply( RageMatrix* pOut, const RageMatrix* pA, const RageMatrix* pB )
{
#ifdef PSP
	__asm__ volatile (
		"ulv.q		R000,  0 + %1\n"
		"ulv.q		R001, 16 + %1\n"
		"ulv.q		R002, 32 + %1\n"
		"ulv.q		R003, 48 + %1\n"
		"ulv.q		R100,  0 + %2\n"
		"ulv.q		R101, 16 + %2\n"
		"ulv.q		R102, 32 + %2\n"
		"ulv.q		R103, 48 + %2\n"
		"vmmul.q	M200, M100, M000\n"
		"usv.q		R200,  0 + %0\n"
		"usv.q		R201, 16 + %0\n"
		"usv.q		R202, 32 + %0\n"
		"usv.q		R203, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(*pA), "m"(*pB)
	);
#else
    const RageMatrix &a = *pA;
    const RageMatrix &b = *pB;

	*pOut = RageMatrix(
		b.m00*a.m00+b.m01*a.m10+b.m02*a.m20+b.m03*a.m30,
		b.m00*a.m01+b.m01*a.m11+b.m02*a.m21+b.m03*a.m31,
		b.m00*a.m02+b.m01*a.m12+b.m02*a.m22+b.m03*a.m32,
		b.m00*a.m03+b.m01*a.m13+b.m02*a.m23+b.m03*a.m33,
		b.m10*a.m00+b.m11*a.m10+b.m12*a.m20+b.m13*a.m30,
		b.m10*a.m01+b.m11*a.m11+b.m12*a.m21+b.m13*a.m31,
		b.m10*a.m02+b.m11*a.m12+b.m12*a.m22+b.m13*a.m32,
		b.m10*a.m03+b.m11*a.m13+b.m12*a.m23+b.m13*a.m33,
		b.m20*a.m00+b.m21*a.m10+b.m22*a.m20+b.m23*a.m30,
		b.m20*a.m01+b.m21*a.m11+b.m22*a.m21+b.m23*a.m31,
		b.m20*a.m02+b.m21*a.m12+b.m22*a.m22+b.m23*a.m32,
		b.m20*a.m03+b.m21*a.m13+b.m22*a.m23+b.m23*a.m33,
		b.m30*a.m00+b.m31*a.m10+b.m32*a.m20+b.m33*a.m30,
		b.m30*a.m01+b.m31*a.m11+b.m32*a.m21+b.m33*a.m31,
		b.m30*a.m02+b.m31*a.m12+b.m32*a.m22+b.m33*a.m32,
		b.m30*a.m03+b.m31*a.m13+b.m32*a.m23+b.m33*a.m33 
	);
#endif
}

void RageMatrixTranslation( RageMatrix* pOut, float x, float y, float z )
{
	RageMatrixIdentity(pOut);
	pOut->m[3][0] = x;
	pOut->m[3][1] = y;
	pOut->m[3][2] = z;
}

void RageMatrixScaling( RageMatrix* pOut, float x, float y, float z )
{
	RageMatrixIdentity(pOut);
	pOut->m[0][0] = x;
	pOut->m[1][1] = y;
	pOut->m[2][2] = z;
}

void RageMatrixRotationX( RageMatrix* pOut, float theta )
{
#ifdef PSP
	__asm__ volatile (
		"lv.s		S000, %1\n"
		"vfim.s		S001, 0.01111\n" // 1/90
		"vmul.s		S000, S000, S001\n"
		"vidt.q		R000\n"
		"vrot.q		R101, S000, [0, c,s,0]\n"
		"vrot.q		R102, S000, [0,-s,c,0]\n"
		"vidt.q		R103\n"
		"usv.q		R100,  0 + %0\n"
		"usv.q		R101, 16 + %0\n"
		"usv.q		R102, 32 + %0\n"
		"usv.q		R103, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(theta)
	);
#else
	theta *= PI/180;

	RageMatrixIdentity(pOut);
	pOut->m[1][1] = cosf(theta);
	pOut->m[2][2] = pOut->m[1][1];

	pOut->m[2][1] = sinf(theta);
	pOut->m[1][2] = -pOut->m[2][1];
#endif
}

void RageMatrixRotationY( RageMatrix* pOut, float theta )
{
#ifdef PSP
	__asm__ volatile (
		"lv.s		S000, %1\n"
		"vfim.s		S001, 0.01111\n" // 1/90
		"vmul.s		S000, S000, S001\n"
		"vrot.q		R100, S000, [c,0,-s,0]\n"
		"vidt.q		R101\n"
		"vrot.q		R102, S000, [s,0, c,0]\n"
		"vidt.q		R103\n"
		"usv.q		R100,  0 + %0\n"
		"usv.q		R101, 16 + %0\n"
		"usv.q		R102, 32 + %0\n"
		"usv.q		R103, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(theta)
	);
#else
	theta *= PI/180;

	RageMatrixIdentity(pOut);
	pOut->m[0][0] = cosf(theta);
	pOut->m[2][2] = pOut->m[0][0];

	pOut->m[0][2] = sinf(theta);
	pOut->m[2][0] = -pOut->m[0][2];
#endif
}

void RageMatrixRotationZ( RageMatrix* pOut, float theta )
{
#ifdef PSP
	__asm__ volatile (
		"lv.s		S000, %1\n"
		"vfim.s		S001, 0.01111\n" // 1/90
		"vmul.s		S000, S000, S001\n"
		"vrot.q		R100, S000, [ c,s,0,0]\n"
		"vrot.q		R101, S000, [-s,c,0,0]\n"
		"vidt.q		R102\n"
		"vidt.q		R103\n"
		"usv.q		R100,  0 + %0\n"
		"usv.q		R101, 16 + %0\n"
		"usv.q		R102, 32 + %0\n"
		"usv.q		R103, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(theta)
	);
#else
	theta *= PI/180;

	RageMatrixIdentity(pOut);
	pOut->m[0][0] = cosf(theta);
	pOut->m[1][1] = pOut->m[0][0];

	pOut->m[0][1] = sinf(theta);
	pOut->m[1][0] = -pOut->m[0][1];
#endif
}

void RageMatrixScale( RageMatrix* pOut, const RageVector3* pV )
{
#ifdef PSP
	__asm__ volatile (
		"lv.q		R000,  0 + %0\n"
		"lv.q		R001, 16 + %0\n"
		"lv.q		R002, 32 + %0\n"
		"lv.q		R003, 48 + %0\n"
		"lv.s		S100, 0 + %1\n"
		"lv.s		S110, 4 + %1\n"
		"lv.s		S120, 8 + %1\n"
		"vscl.t		R000, R000, S100\n"
		"vscl.t		R001, R001, S110\n"
		"vscl.t		R002, R002, S120\n"
		"sv.q		R000,  0 + %0\n"
		"sv.q		R001, 16 + %0\n"
		"sv.q		R002, 32 + %0\n"
		"sv.q		R003, 48 + %0\n"
        : "+m"(*pOut)
		: "m"(*pV)
	);
#else
	for( int i=0; i < 4; ++i )
	{
		pOut->m[i][0] *= pV->x;
		pOut->m[i][1] *= pV->y;
		pOut->m[i][2] *= pV->z;
	}
#endif
}

void RageMatrixScaleLocal( RageMatrix* pOut, const RageVector3* pV )
{
#ifdef PSP
	__asm__ volatile (
		"lv.q		R000,  0 + %0\n"
		"lv.q		R001, 16 + %0\n"
		"lv.q		R002, 32 + %0\n"
		"lv.q		R003, 48 + %0\n"
		"lv.s		S100, 0 + %1\n"
		"lv.s		S101, 4 + %1\n"
		"lv.s		S102, 8 + %1\n"
		"vscl.t		C000, C000, S100\n"
		"vscl.t		C010, C010, S101\n"
		"vscl.t		C020, C020, S102\n"
		"sv.q		R000,  0 + %0\n"
		"sv.q		R001, 16 + %0\n"
		"sv.q		R002, 32 + %0\n"
		"sv.q		R003, 48 + %0\n"
        : "+m"(*pOut)
		: "m"(*pV)
	);
#else
	for( int i=0; i < 4; ++i )
	{
		pOut->m[0][i] *= pV->x;
		pOut->m[1][i] *= pV->y;
		pOut->m[2][i] *= pV->z;
	}
#endif
}

void RageMatrixTranslate( RageMatrix* pOut, const RageVector3* pV )
{
	RageMatrix m;
	RageMatrixTranslation( &m, pV->x, pV->y, pV->z );
	RageMatrixMultiply( pOut, &m, pOut );
}

void RageMatrixTranslateLocal( RageMatrix* pOut, const RageVector3* pV )
{
#ifdef PSP
	// from ps2dev forum
	/* This might be a little faster, since the vmmul is a
		pretty long-latency instruction, compared to simple
		scales and adds. Also uses fewer registers (M000 &
		M100) */
	__asm__ volatile (
		"lv.q		R000,  0 + %0\n"
		"lv.q		R001, 16 + %0\n"
		"lv.q		R002, 32 + %0\n"
		"lv.q		R003, 48 + %0\n"
		"lv.s		S103, 0 + %1\n"
		"lv.s		S113, 4 + %1\n"
		"lv.s		S123, 8 + %1\n"
		"vscl.q		R100, R000, S103\n"
		"vscl.q		R101, R001, S113\n"
		"vscl.q		R102, R002, S123\n"
		"vadd.q		R003, R003, R100\n"
		"vadd.q		R003, R003, R101\n"
		"vadd.q		R003, R003, R102\n"
		"sv.q		R003, 48 + %0\n" // only R003 has changed
		: "+m"(*pOut)
		: "m"(*pV)
	);
#else
	for( int i=0; i < 4; ++i )
		pOut->m[3][i] += pOut->m[0][i] * pV->x + pOut->m[1][i] * pV->y + pOut->m[2][i] * pV->z;
#endif
}


/* This is similar in style to Actor::Command.  However, Actors don't store
 * matrix stacks; they only store offsets and scales, and compound them into
 * a single transformations at once.  This makes some things easy, but it's not
 * convenient for generic 3d transforms.  For that, we have this, which has the
 * small subset of the actor commands that applies to raw matrices, and we apply
 * commands in the order given.  "scale,2;x,1;" is very different from
 * "x,1;scale,2;". */
static CString GetParam( const CStringArray& sParams, int iIndex, int& iMaxIndexAccessed )
{
	iMaxIndexAccessed = max( iIndex, iMaxIndexAccessed );
	if( iIndex < int(sParams.size()) )
		return sParams[iIndex];
	else
		return "";
}

void RageMatrixCommand( CString sCommandString, RageMatrix &mat )
{
	CStringArray asCommands;
	split( sCommandString, ";", asCommands, true );
	
	for( unsigned c=0; c<asCommands.size(); c++ )
	{
		CStringArray asTokens;
		split( asCommands[c], ",", asTokens, true );

		int iMaxIndexAccessed = 0;

#define sParam(i) (GetParam(asTokens,i,iMaxIndexAccessed))
#define fParam(i) (strtof(sParam(i),NULL))
#define iParam(i) (atoi(sParam(i)))
#define bParam(i) (iParam(i)!=0)

		CString& sName = asTokens[0];
		sName.MakeLower();

		RageMatrix b;
		// Act on command
		if( sName=="x" )					RageMatrixTranslation( &b, fParam(1),0,0 );
		else if( sName=="y" )				RageMatrixTranslation( &b, 0,fParam(1),0 );
		else if( sName=="z" )				RageMatrixTranslation( &b, 0,0,fParam(1) );
		else if( sName=="zoomx" )			RageMatrixScaling(&b, fParam(1),1,1 );
		else if( sName=="zoomy" )			RageMatrixScaling(&b, 1,fParam(1),1 );
		else if( sName=="zoomz" )			RageMatrixScaling(&b, 1,1,fParam(1) );
		else if( sName=="rotationx" )		RageMatrixRotationX( &b, fParam(1) );
		else if( sName=="rotationy" )		RageMatrixRotationY( &b, fParam(1) );
		else if( sName=="rotationz" )		RageMatrixRotationZ( &b, fParam(1) );
		else
		{
			CString sError = ssprintf( "MatrixCommand:  Unrecognized matrix command name '%s' in command string '%s'.", sName.c_str(), sCommandString.c_str() );
			LOG->Warn( sError );
			Dialog::OK( sError );
			continue;
		}


		if( iMaxIndexAccessed != (int)asTokens.size()-1 )
		{
			CString sError = ssprintf( "MatrixCommand:  Wrong number of parameters in command '%s'.  Expected %d but there are %d.", join(",",asTokens).c_str(), iMaxIndexAccessed+1, (int)asTokens.size() );
			LOG->Warn( sError );
			Dialog::OK( sError );
			continue;
		}

#ifdef PSP
		RageMatrixMultiply(&mat, &mat, &b);
#else
		RageMatrix a(mat);
		RageMatrixMultiply(&mat, &a, &b);
#endif
	}
}


void RageQuatMultiply( RageVector4* pOut, const RageVector4 &pA, const RageVector4 &pB )
{
#ifdef PSP
	__asm__ volatile (
		"ulv.q		R001, %1\n"
		"ulv.q		R002, %2\n"
		"vqmul.q	R003, R001, R002\n"
		"vdot.q		S000, R003, R003\n"
		"vrsq.s		S000, S000\n"
		"vscl.q		R003, R003, S000\n"
		"usv.q		R003, %0\n"
		: "=m"(*pOut)
		: "m"(pA), "m"(pB)
	);
#else
	RageVector4 out;
	out.x = pA.w * pB.x + pA.x * pB.w + pA.y * pB.z - pA.z * pB.y;
	out.y = pA.w * pB.y + pA.y * pB.w + pA.z * pB.x - pA.x * pB.z;
	out.z = pA.w * pB.z + pA.z * pB.w + pA.x * pB.y - pA.y * pB.x;
	out.w = pA.w * pB.w - pA.x * pB.x - pA.y * pB.y - pA.z * pB.z;

    float dist, square;

	square = out.x * out.x + out.y * out.y + out.z * out.z + out.w * out.w;
	
	if (square > 0.0)
		dist = 1.0f / sqrtf(square);
	else dist = 1;

    out.x *= dist;
    out.y *= dist;
    out.z *= dist;
    out.w *= dist;

	*pOut = out;
#endif
}

RageVector4 RageQuatFromH(float theta )
{
	theta *= PI/180.0f;
	theta /= 2.0f;
	theta *= -1;

	float c, s;
	sincosf( theta, &s, &c );

	return RageVector4(0, s, 0, c);
}

RageVector4 RageQuatFromP(float theta )
{
	theta *= PI/180.0f;
	theta /= 2.0f;
	theta *= -1;

	float c, s;
	sincosf( theta, &s, &c );

	return RageVector4(s, 0, 0, c);
}

RageVector4 RageQuatFromR(float theta )
{
	theta *= PI/180.0f;
	theta /= 2.0f;
	theta *= -1;

	float c, s;
	sincosf( theta, &s, &c );

	return RageVector4(0, 0, s, c);
}


/* Math from http://www.gamasutra.com/features/19980703/quaternions_01.htm . */

/* prh.xyz -> heading, pitch, roll */
#ifdef PSP
void RageQuatFromHPR(RageVector4* pOut, const RageVector3 &hpr )
{
	__asm__ volatile (
		"lv.s		S000, 0 + %1\n"
		"lv.s		S010, 0 + %1\n"
		"lv.s		S020, 0 + %1\n"
		"vfim.s		S030, 0.00556\n" // 1/180
		"vscl.t		R000, R000, S030\n"
		"vcos.t		R001, R000\n"
		"vsin.t		R000, R000\n"
		"vcrs.t		R002, R001, R001\n"
		"vcrs.t		R003, R000, R000\n"
		"vmul.s		S030, S002, S001\n"
		"vmul.s		S031, S003, S000\n"
		"vmul.t		R002, R002, R000\n"
		"vmul.t		R003, R003, R001\n"
		"vsub.s		S030, S030, S031\n"
		"vadd.t		R000, R002, R003[x,-y,z]\n"
		"usv.q		R000, %0\n"
		: "=m"(*pOut)
		: "m"(hpr)
	);
}
#else
void RageQuatFromHPR(RageVector4* pOut, RageVector3 hpr )
{
	hpr *= PI;
	hpr /= 180.0f;
	hpr /= 2.0f;

	float sX, cX, sY, cY, sZ, cZ;
	sincosf( hpr.x, &sX, &cX );
	sincosf( hpr.y, &sY, &cY );
	sincosf( hpr.z, &sZ, &cZ );

	pOut->w = cX * cY * cZ + sX * sY * sZ;
	pOut->x = sX * cY * cZ - cX * sY * sZ;
	pOut->y = cX * sY * cZ + sX * cY * sZ;
	pOut->z = cX * cY * sZ - sX * sY * cZ;
}
#endif

/*
 * Screen orientatoin:  the "floor" is the XZ plane, and Y is height; in other
 * words, the screen is the XY plane and negative Z goes into it.
 */

/* prh.xyz -> pitch, roll, heading */
#ifdef PSP
void RageQuatFromPRH(RageVector4* pOut, const RageVector3 &prh )
{
	__asm__ volatile (
		"lv.s		S000, 0 + %1\n"
		"lv.s		S010, 0 + %1\n"
		"lv.s		S020, 0 + %1\n"
		"vfim.s		S030, 0.00556\n" // 1/180
		"vscl.t		R000, R000, S030\n"
		"vcos.t		R001, R000\n"
		"vsin.t		R000, R000\n"
		"vcrs.t		R002, R001, R001\n"
		"vcrs.t		R003, R000, R000\n"
		"vmul.s		S030, S002, S001\n"
		"vmul.s		S031, S003, S000\n"
		"vmul.t		R002, R002, R000\n"
		"vmul.t		R003, R003, R001\n"
		"vsub.s		S030, S030, S031\n"
		"vadd.t		R000, R002, R003[-x,y,-z]\n"
		"usv.q		R000, %0\n"
		: "=m"(*pOut)
		: "m"(prh)
	);
}
#else
void RageQuatFromPRH(RageVector4* pOut, RageVector3 prh )
{
	prh *= PI;
	prh /= 180.0f;
	prh /= 2.0f;

	/* Set cX to the cosine of the angle we want to rotate on the X axis,
	 * and so on.  Here, hpr.z (roll) rotates on the Z axis, hpr.x (heading)
	 * on Y, and hpr.y (pitch) on X. */
	float sX, cX, sY, cY, sZ, cZ;
	sincosf( prh.y, &sX, &cX );
	sincosf( prh.x, &sY, &cY );
	sincosf( prh.z, &sZ, &cZ );

	pOut->w = cX * cY * cZ + sX * sY * sZ;
	pOut->x = sX * cY * cZ - cX * sY * sZ;
	pOut->y = cX * sY * cZ + sX * cY * sZ;
	pOut->z = cX * cY * sZ - sX * sY * cZ;
}
#endif

void RageMatrixFromQuat( RageMatrix* pOut, const RageVector4 &q )
{
#ifdef PSP
#if 0
/*
	x2 = SQR(x);
	y2 = SQR(y);
	z2 = SQR(z);
	w2 = SQR(w);

	xy = x * y;
	xz = x * z;
	yz = y * z;

	wx = w * x;
	wy = w * y;
	wz = w * z;

	matrix[0] = 1 - 2*(y2 + z2);
	matrix[1] = 2 * (xy + wz);
	matrix[2] = 2 * (xz - wy);

	matrix[4] = 2 * (xy - wz);
	matrix[5] = 1 - 2*(x2 + z2);
	matrix[6] = 2 * (yz + wx);

	matrix[8] = 2 * (xz + wy);
	matrix[9] = 2 * (yz - wx));
	matrix[10] = 1 - 2*(x2 + y2);
*/

   __asm__ volatile (
       "lv.q      C000, %1\n"                               // C000 = [x,  y,  z,  w ]
       "vmul.q    C010, C000, C000\n"                       // C010 = [x2, y2, z2, w2]
       "vcrs.t    C020, C000, C000\n"                       // C020 = [yz, xz, xy ]
       "vmul.q    C030, C000, C000[w,w,w,0]\n"              // C030 = [wx, wy, wz ]

       "vadd.q    C100, C020[0,z,y,0], C030[0,z,-y,0]\n"    // C100 = [0,     xy+wz, xz-wy]
       "vadd.s    S100, S011, S012\n"                       // C100 = [y2+z2, xy+wz, xz-wy]

       "vadd.q    C110, C020[z,0,x,0], C030[-z,0,x,0]\n"    // C110 = [xy-wz, 0,     yz+wx]
       "vadd.s    S111, S010, S012\n"                       // C110 = [xy-wz, x2+z2, yz+wx]

       "vadd.q    C120, C020[y,x,0,0], C030[y,-x,0,0]\n"    // C120 = [xz+wy, yz-wx, 0    ]
       "vadd.s    S122, S010, S011\n"                       // C120 = [xz+wy, yz-wx, x2+y2]

       "vmov.s    S033, S033[2]\n"
       "vscl.t    C100, C100, S033\n"                       // C100 = [2*(y2+z2), 2*(xy+wz), 2*(xz-wy)]
       "vscl.t    C110, C110, S033\n"                       // C110 = [2*(xy-wz), 2*(x2+z2), 2*(yz+wx)]
       "vscl.t    C120, C120, S033\n"                       // C120 = [2*(xz+wy), 2*(yz-wx), 2*(x2+y2)]

       "vocp.s    S100, S100\n"                             // C100 = [1-2*(y2+z2), 2*(xy+wz),   2*(xz-wy)  ]
       "vocp.s    S111, S111\n"                             // C110 = [2*(xy-wz),   1-2*(x2+z2), 2*(yz+wx)  ]
       "vocp.s    S122, S122\n"                             // C120 = [2*(xz+wy),   2*(yz-wx),   1-2*(x2+y2)]

       "vidt.q    C130\n"                                   // C130 = [0, 0, 0, 1]

       "sv.q      R100, 0  + %0\n"
       "sv.q      R101, 16 + %0\n"
       "sv.q      R102, 32 + %0\n"
       "sv.q      R103, 48 + %0\n"
       : "=m"(*pOut) : "m"(q));
} 
#else
	__asm__ volatile (  // unsure
		"ulv.q		R003, %1\n"
		"vmov.q		R000, R003[ w,  z, -y, -x]\n"
		"vmov.q		R001, R003[-z,  w,  x, -y]\n"
		"vmov.q		R002, R003[ y, -x,  w, -z]\n"
		"vmov.q		R100, R003[ w,  z, -y,  x]\n"
		"vmov.q		R101, R003[-z,  w,  x,  y]\n"
		"vmov.q		R102, R003[ y, -x,  w,  z]\n"
		"vmov.q		R103, R003[-x, -y, -z,  w]\n"
		"vmmul.q	M200, M100, M000\n"
		"usv.q		R200,  0 + %0\n"
		"usv.q		R201, 16 + %0\n"
		"usv.q		R202, 32 + %0\n"
		"usv.q		R203, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(q)
	);
#endif
#else
	float xx = q.x * (q.x + q.x);
	float xy = q.x * (q.y + q.y);
	float xz = q.x * (q.z + q.z);

	float wx = q.w * (q.x + q.x);
	float wy = q.w * (q.y + q.y);
	float wz = q.w * (q.z + q.z);

	float yy = q.y * (q.y + q.y);
	float yz = q.y * (q.z + q.z);

	float zz = q.z * (q.z + q.z);
	// careful.  The param order is row-major, which is the 
	// transpose of the order shown in the OpenGL docs.
	*pOut = RageMatrix(
		1-(yy+zz), xy+wz,     xz-wy,     0,
		xy-wz,     1-(xx+zz), yz+wx,     0,
		xz+wy,     yz-wx,     1-(xx+yy), 0,
		0,         0,         0,         1 );
#endif
}

void RageQuatSlerp(RageVector4 *pOut, const RageVector4 &from, const RageVector4 &to, float t)
{
#ifdef PSP
	__asm__ volatile ( // unsure
		".set		push\n"
		".set		noreorder\n"
		"ulv.q		R000, %1\n"
		"ulv.q		R001, %2\n"
		"mtv		%3, S022\n"
		"vdot.q		S002, R000, R001\n"
		"vcmp.s		LT, S002, S002[0]\n"
		"vcmovt.q	R001, R001[-x,-y,-z,-w], 0\n"
		"vfim.s		S032, 0.9999\n"
		"vcmp.s		GE, S002, S032\n"
		"bvtl		0, 1f\n"
		"vocp.s		S032, S022\n"

		"vabs.s		S002, S002\n"
		"vcst.s		S003, VFPU_SQRT1_2\n"
		"vcmp.s		LT, S002, S003\n"
		"vmul.s		S003, S002, S002\n"
		"bvtl		0, 0f\n"

		"vasin.s	S002, S002\n"
		"vocp.s		S003, S003\n"
		"vsqrt.s	S003, S003\n"
		"vasin.s	S003, S003\n"
		"vocp.s		S002, S003\n"

	"0:\n"
		"vocp.s		S012, S002\n"
		"vscl.p		R022, R022, S012\n"
		"vsin.t		R012, R012\n"
		"vscl.p		R022, R022, S012\n"

	"1:\n"
		"vscl.q		R000, R000, S032\n"
		"vscl.q		R001, R001, S022\n"
		"vadd.q		R002, R000, R001\n"
		"usv.q		R002, %0\n"
		".set		pop\n"
		: "=m"(*pOut)
		: "m"(from), "m"(to), "r"(t)
	);
#else
	float to1[4];

	// calc cosine
	float cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;

	// adjust signs (if necessary)
	if ( cosom < 0 )
	{
		cosom = -cosom;
		to1[0] = - to.x;
		to1[1] = - to.y;
		to1[2] = - to.z;
		to1[3] = - to.w;
	} else  {
		to1[0] = to.x;
		to1[1] = to.y;
		to1[2] = to.z;
		to1[3] = to.w;
	}

	// calculate coefficients
	float scale0, scale1;
	if ( cosom < 0.9999f )
	{
		// standard case (slerp)
		float omega = acosf(cosom);
		float sinom = sinf(omega);
		scale0 = sinf((1.0f - t) * omega) / sinom;
		scale1 = sinf(t * omega) / sinom;
	} else {        
		// "from" and "to" quaternions are very close 
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - t;
		scale1 = t;
	}
	// calculate final values
	pOut->x = scale0 * from.x + scale1 * to1[0];
	pOut->y = scale0 * from.y + scale1 * to1[1];
	pOut->z = scale0 * from.z + scale1 * to1[2];
	pOut->w = scale0 * from.w + scale1 * to1[3];
#endif
}

void RageLookAt( RageMatrix *pOut, const RageVector3 &eye, const RageVector3 &center, const RageVector3 &up )
{
#ifdef PSP
	__asm__ volatile ( // from ps2dev forum
		"vmidt.q	M100\n"

		// load eye, center, up vectors
		"lv.s		S000, 0 + %1\n"
		"lv.s		S001, 4 + %1\n"
		"lv.s		S002, 8 + %1\n"
		"lv.s		S010, 0 + %2\n"
		"lv.s		S011, 4 + %2\n"
		"lv.s		S012, 8 + %2\n"
		"lv.s		S020, 0 + %3\n"
		"lv.s		S021, 4 + %3\n"
		"lv.s		S022, 8 + %3\n"

		// forward = center - eye
		"vsub.t		R102, C010, C000\n"

		// normalize forward
		"vdot.t		S033, R102, R102\n"
		"vrsq.s		S033, S033\n"
		"vscl.t		R102, R102, S033\n"

		// side = forward cross up
		"vcrsp.t	R100, R102, C020\n"
		"vdot.t		S033, R100, R100\n"
		"vrsq.s		S033, S033\n"
		"vscl.t		R100, R100, S033\n"

		// lup = side cross forward
		"vcrsp.t	R101, R100, R102\n"

		"vneg.t		R102, R102\n"

		"vmidt.q	M200\n"
		"vneg.t		C230, C000\n"
		"vmmul.q	M000, M100, M200\n"

		"usv.q		C000,  0 + %0\n"
		"usv.q		C010, 16 + %0\n"
		"usv.q		C020, 32 + %0\n"
		"usv.q		C030, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(eye), "m"(center), "m"(up)
	);
#else
	RageVector3 Z(eyex - centerx, eyey - centery, eyez - centerz);
	RageVec3Normalize(&Z, &Z);

	RageVector3 Y(upx, upy, upz);

	RageVector3 X(
		 Y[1] * Z[2] - Y[2] * Z[1],
		-Y[0] * Z[2] + Y[2] * Z[0],
		 Y[0] * Z[1] - Y[1] * Z[0]);

	Y = RageVector3(
		 Z[1] * X[2] - Z[2] * X[1],
        -Z[0] * X[2] + Z[2] * X[0],
         Z[0] * X[1] - Z[1] * X[0] );

	RageVec3Normalize(&X, &X);
	RageVec3Normalize(&Y, &Y);

	RageMatrix mat(
		X[0], Y[0], Z[0], 0,
		X[1], Y[1], Z[1], 0,
		X[2], Y[2], Z[2], 0,
		   0,    0,    0, 1 );

	RageMatrix mat2;
	RageMatrixTranslation(&mat2, -eyex, -eyey, -eyez);

	RageMatrixMultiply(pOut, &mat, &mat2);
#endif
}

void RageMatrixAngles( RageMatrix* pOut, const RageVector3 &angles )
{
#ifdef PSP
	__asm__ volatile (
		"lv.s		S000, 0 + %1\n"
		"lv.s		S010, 4 + %1\n"
		"lv.s		S020, 8 + %1\n"
		"vfim.s		S001, 0.01111\n" // 1/90
		"vscl.t		R000, R000, S001\n"
		"vcos.t		R001, R000\n"
		"vsin.t		R000, R000\n"
		"vmidt.q	M100\n"
		"vmov.t		R101, R000[x,x,x]\n"
		"vmov.t		R102, R001[x,x,x]\n"
		"vscl.t		C100, C100, S031\n"
		"vscl.t		C110, C110, S032\n"
		"usv.q		R100,  0 + %0\n"
		"usv.q		R101, 16 + %0\n"
		"usv.q		R102, 32 + %0\n"
		"usv.q		R103, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(angles)
	);
#else
	const RageVector3 angles_radians( angles * 2*PI / 360 );
	
	const float sy = sinf( angles_radians[2] );
	const float cy = cosf( angles_radians[2] );
	const float sp = sinf( angles_radians[1] );
	const float cp = cosf( angles_radians[1] );
	const float sr = sinf( angles_radians[0] );
	const float cr = cosf( angles_radians[0] );

	RageMatrixIdentity( pOut );


	// matrix = (Z * Y) * X
	pOut->m[0][0] = cp*cy;
	pOut->m[0][1] = cp*sy;
	pOut->m[0][2] = -sp;
	pOut->m[1][0] = sr*sp*cy+cr*-sy;
	pOut->m[1][1] = sr*sp*sy+cr*cy;
	pOut->m[1][2] = sr*cp;
	pOut->m[2][0] = (cr*sp*cy+-sr*-sy);
	pOut->m[2][1] = (cr*sp*sy+-sr*cy);
	pOut->m[2][2] = cr*cp;
#endif
}

void RageMatrixTranspose( RageMatrix* pOut, const RageMatrix* pIn )
{
#ifdef PSP
	__asm__ volatile (
		"ulv.q		R000,  0 + %1\n"
		"ulv.q		R001, 16 + %1\n"
		"ulv.q		R002, 32 + %1\n"
		"ulv.q		R003, 48 + %1\n"
		"usv.q		C000,  0 + %0\n"
		"usv.q		C010, 16 + %0\n"
		"usv.q		C020, 32 + %0\n"
		"usv.q		C030, 48 + %0\n"
		: "=m"(*pOut)
		: "m"(*pIn)
	);
#else
	for( int i=0; i<4; i++)
		for( int j=0; j<4; j++)
			pOut->m[j][i] = pIn->m[i][j];
#endif
}

/*
 * Copyright (c) 2001-2003 Chris Danford
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
