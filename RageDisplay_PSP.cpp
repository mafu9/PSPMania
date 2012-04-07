/*
 * The depth test of PSP seems to be a little different from OpenGL. Probably inverse?
 */

#include "global.h"
#include "RageDisplay.h"
#include "RageDisplay_PSP.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageTimer.h"
#include "RageException.h"
#include "RageTexture.h"
#include "RageTextureManager.h"
#include "RageMath.h"
#include "RageTypes.h"
#include "RageFile.h"
#include "GameConstantsAndTypes.h"
#include "StepMania.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"

#include "archutils/PSP/GuList.h"
#include <pspdisplay.h>
#include <pspgu.h>

#define SCR_WIDTH						480
#define SCR_HEIGHT						272

#define VRAM_NO_CASHE_ADDR				0x44000000
#define VRAM_WIDTH						512
#define VRAM_BUF_SIZE_16				(VRAM_WIDTH * SCR_HEIGHT * 2)
#define VRAM_BUF_SIZE_32				(VRAM_WIDTH * SCR_HEIGHT * 4)
#define VRAM_BUF_ADDR_16(n)				((void*)(VRAM_BUF_SIZE_16 * (n)))
#define VRAM_BUF_ADDR_32(n)				((void*)(VRAM_BUF_SIZE_32 * (n)))

#define ENABLE_GU_STATUS(s)				{ if( sceGuGetStatus(s) == 0 ) sceGuEnable(s); }
#define DISABLE_GU_STATUS(s)			{ if( sceGuGetStatus(s) != 0 ) sceGuDisable(s); }

#define RAGE_SPRITE_VERTEX_FORMAT		(GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF)
#define RAGE_MODEL_VERTEX2_FORMAT		(GU_TEXTURE_32BITF|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT)

struct RageModelVertex2
{
    RageVector3		p;	// position
    RageVector3		n;	// normal
	RageVector2		t;	// texture coordinates
};

struct PspTexture
{
	int psm;
	uint32_t w, h;

	uint32_t *palette;

	uint8_t *pixels;
	uint32_t pixelSize;

	int swizzle;

	PspTexture()
	{
		swizzle = 0;
		pixels = NULL;
		palette = NULL;
	}
	~PspTexture()
	{
		free( palette );
		free( pixels );
	}
};

static void CheckGuList( int limit = 0 );

static bool g_bZWrite;
static RageDisplay::VideoModeParams g_CurrentParams;
static int g_iCurrentDisplayBufferIndex;
static const PspTexture *g_ActiveTexture = NULL;

static const RageDisplay::PixelFormatDesc PIXEL_FORMAT_DESC[RageDisplay::NUM_PIX_FORMATS] = {
	{
		/* A8B8G8R8 */
		32,
		{ 0x000000FF,
		  0x0000FF00,
		  0x00FF0000,
		  0xFF000000 }
	}, {
		/* A4R4G4B4 */
		16,
		{ 0x000F,
		  0x00F0,
		  0x0F00,
		  0xF000 },
	}, {
		/* A1B5G5R5 */
		16,
		{ 0x001F,
		  0x03E0,
		  0x7C00,
		  0x8000 },
	}, {
		/* X1R5G5R5 */
		16,
		{ 0x001F,
		  0x03E0,
		  0x7C00,
		  0x0000 },
	}, {
		/* B8G8R8 */
		24,
		{ 0x0000FF,
		  0x00FF00,
		  0xFF0000,
		  0x000000 }
	}, {
		/* Paletted */
		8,
		{ 0,0,0,0 } /* N/A */
	}, {
		/* BGR (N/A; OpenGL only) */
		0, { 0,0,0,0 }
	}, {
		/* ABGR (N/A; OpenGL only) */
		0, { 0,0,0,0 }
	}
};

static const int PSP_GU_FORMATS[RageDisplay::NUM_PIX_FORMATS] = 
{
	GU_PSM_8888,
	GU_PSM_4444,
	GU_PSM_5551,
	-1, /* no RGB */
	-1, /* no RGB */
	GU_PSM_T8,
	-1, /* no BGR */
	-1  /* no ABGR */
};

static uint32_t ARGB8888fromARGB1555( uint16_t src )
{
	uint32_t c = ((src & 0x001F) << 3) | ((src & 0x03E0) << 6) | ((src & 0x7C00) << 9) | ((src >> 15) << 24);
	c |= (c >> 5) & 0x00070707;
	return c;
}

static uint32_t ARGB8888fromARGB4444( uint16_t src )
{
	uint32_t c = ((src & 0x000F) << 4) | ((src & 0x00F0) << 8) | ((src & 0x0F00) << 12) | ((src & 0xF000) << 16);
	c |= (c >> 4) & 0x0F0F0F0F;
	return c;
}

static uint32_t ARGB8888fromRGB565( uint16_t src )
{
	uint32_t c = ((src & 0x001F) << 3) | ((src & 0x07E0) << 5) | ((src & 0xF800) << 8) | 0xFF000000;
	c |= ((c >> 5) & 0x00070007) | ((c >> 6) & 0x00000300);
	return c;
}

static uint16_t ARGB8888toARGB1555( uint32_t src )
{
	const uint8_t *p = (const uint8_t*)&src;
	uint16_t c = (p[3] & 0x80) << 8;
    c |= (((p[2] + 0x04) * 0x1F24) & 0xFFFF0000) >> 6;
    c |= (((p[1] + 0x04) * 0x1F24) & 0xFFFF0000) >> 11;
    c |= ((p[0] + 0x04) * 0x1F24) >> 16;
    return c;
}

static uint16_t ARGB8888toARGB4444( uint32_t src )
{
	const uint8_t *p = (const uint8_t*)&src;
	uint16_t c = (((p[3] + 0x08) * 0x0F10) & 0xFFFF0000) >> 4;
	c |= (((p[2] + 0x08) * 0x0F10) & 0xFFFF0000) >> 8;
	c |= (((p[1] + 0x08) * 0x0F10) & 0xFFFF0000) >> 12;
	c |= ((p[0] + 0x08) * 0x0F10) >> 16;
    return c;
}

static uint16_t ARGB1555toARGB4444( uint16_t src )
{
	return ARGB8888toARGB4444( ARGB8888fromARGB1555(src) );
}

static uint16_t ARGB4444toARGB1555( uint16_t src )
{
	return ARGB8888toARGB1555( ARGB8888fromARGB4444(src) );
}

static uint32_t GetTextureSize( int psm, uint32_t width, uint32_t height )
{
	switch( psm )
	{
		case GU_PSM_T8:
			return width * height;

		case GU_PSM_4444:
		case GU_PSM_5551:
		case GU_PSM_5650:
		case GU_PSM_T16:
			return (width * height) * 2;

		case GU_PSM_8888:
		case GU_PSM_T32:
			return (width * height) * 4;

		case GU_PSM_T4:
			return (width * height) / 2;

		default:
			return 0;
	}
}

void SwizzleFast( uint8_t *out, const uint8_t *in, uint32_t width, uint32_t height )
{
	uint32_t width_blocks = width / 16;
	uint32_t height_blocks = height / 8;
 
	uint32_t src_pitch = (width - 16) / 4;
	uint32_t src_row = width * 8;
 
	const uint8_t *ysrc = in;
	uint32_t *dst = (uint32_t*)out;
 
	for( uint32_t blocky = 0; blocky < height_blocks; ++blocky )
	{
		const uint8_t *xsrc = ysrc;
		for( uint32_t blockx = 0; blockx < width_blocks; ++blockx )
		{
			const uint32_t *src = (uint32_t*)xsrc;
			for( uint32_t j = 0; j < 8; ++j )
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
}

// optimize texture as possible
int SwizzleTexture( PspTexture *texture )
{
	if( texture->swizzle )
		return 0;

	SetMemoryAssert( false );
	uint8_t *buf = (uint8_t*)memalign( 16, texture->pixelSize );
	SetMemoryAssert( true );
	if( buf == NULL )
		return -1;

	uint32_t pitch = GetTextureSize( texture->psm, texture->w, 1 );

	SwizzleFast( buf, texture->pixels, pitch, texture->h );
	memcpy( texture->pixels, buf, texture->pixelSize );

	free( buf );
	texture->swizzle = 1;
	return 0;
}

void CopyImageToTexture( PspTexture *texture, const RageSurface *img, int img_psm )
{
	ASSERT( texture->pixels );

	if( texture->psm == img_psm )
	{
		memcpy( texture->pixels, img->pixels, texture->pixelSize );
	}
	else
	{
		const uint8_t *src = img->pixels;
		uint8_t *dest = texture->pixels;

		uint32_t tex_pitch = GetTextureSize( texture->psm, texture->w, 1 );

		for( int i = 0; i < img->h; ++i )
		{
			for( int j = 0; j < img->w; ++j )
			{
				switch( texture->psm )
				{
				case GU_PSM_8888:
					if( img_psm == GU_PSM_4444 )
						((uint32_t*)dest)[j] = ARGB8888fromARGB4444( ((const uint16_t*)src)[j] );
					else // GU_PSM_5551
						((uint32_t*)dest)[j] = ARGB8888fromARGB1555( ((const uint16_t*)src)[j] );

					break;
				
				case GU_PSM_4444:
					if( img_psm == GU_PSM_8888 )
						((uint16_t*)dest)[j] = ARGB8888toARGB4444( ((const uint32_t*)src)[j] );
					else // GU_PSM_5551
						((uint16_t*)dest)[j] = ARGB1555toARGB4444( ((const uint16_t*)src)[j] );

					break;

				case GU_PSM_5551:
					if( img_psm == GU_PSM_8888 )
						((uint16_t*)dest)[j] = ARGB8888toARGB1555( ((const uint32_t*)src)[j] );
					else // GU_PSM_4444
						((uint16_t*)dest)[j] = ARGB4444toARGB1555( ((const uint16_t*)src)[j] );

					break;
				}
			}

			src += img->pitch;
			dest += tex_pitch;
		}
	}
}

RageDisplay_PSP::RageDisplay_PSP( VideoModeParams p )
{
	try {
		LOG->Trace( "RageDisplay_PSP::RageDisplay_PSP()" );
		LOG->MapLog( "renderer", "Current renderer: PSP-GU" );

		g_bZWrite = false;
		g_ActiveTexture = NULL;

		sceGuInit();
		StartGuList();
		sceGuOffset( 2048 - (p.width / 2), 2048 - (p.height / 2) );
		sceGuScissor( 0, 0, p.width, p.height );
		sceGuEnable( GU_SCISSOR_TEST );
		sceGuTexWrap( GU_CLAMP, GU_CLAMP );
		sceGuTexScale( 1.0f, 1.0f );
		sceGuTexOffset( 0.0f, 0.0f );
		sceGuTexFilter( GU_LINEAR, GU_LINEAR );
		sceGuTexFunc( GU_TFX_MODULATE, GU_TCC_RGBA );
		sceGuShadeModel( GU_FLAT );
		sceGuEnable( GU_BLEND );
		sceGuEnable( GU_ALPHA_TEST );
		sceGuAlphaFunc( GU_NOTEQUAL, 0, 0xFF );
		sceGuDisable( GU_DEPTH_TEST );
		sceGuDepthMask( GU_TRUE );
		sceGuDepthRange( 0xFFFF, 0x0000 );
		sceGuClearColor( 0x00000000 );
		sceGuClearDepth( 0x0000 );
		sceGuColor( 0xFFFFFFFF );
		SetVideoMode( p );
		sceDisplayWaitVblankStart();
		sceGuDisplay( GU_TRUE );
	} catch( ... ) {
		FinishGuList();
		sceGuSync( GU_SYNC_FINISH, GU_SYNC_WHAT_DONE );
		sceGuTerm();
		throw;
	};
}

void RageDisplay_PSP::Update( float fDeltaTime )
{
}

RageDisplay_PSP::~RageDisplay_PSP()
{
	LOG->Trace( "RageDisplay_PSP::~RageDisplay()" );

	FinishGuList();
	sceGuSync( GU_SYNC_FINISH, GU_SYNC_WHAT_DONE );
	sceGuTerm();
}

static int FindBackBufferType( int iBPP )
{
	if( iBPP == 16 )
		return GU_PSM_5650;

	if( iBPP == 32 )
		return GU_PSM_8888;

	LOG->Warn( "Invalid BPP '%d' specified", iBPP );
	LOG->Warn( "Couldn't find an appropriate back buffer format." );
	return -1;
}

static void CheckGuList( int limit )
{
	ASSERT( 0 <= limit && limit < NUM_GU_LIST );

	if( sceGuCheckList() < (0.8 * NUM_GU_LIST) )
		return;

	FinishGuList();
	sceGuSync( GU_SYNC_FINISH, GU_SYNC_WHAT_DONE );
	StartGuList();
}

void RageDisplay_PSP::ResolutionChanged()
{
	SetViewport( 0, 0 );

	FinishGuList();
	sceGuSync( GU_SYNC_FINISH, GU_SYNC_WHAT_DONE );
	StartGuList();
}

CString RageDisplay_PSP::TryVideoMode( VideoModeParams p, bool &bNewDeviceOut )
{
	g_CurrentParams = p;

	int psm = FindBackBufferType( p.bpp );
	if( psm < 0 )	// no possible back buffer formats
		return ssprintf( "FindBackBufferType(%i) failed", p.bpp );	// failed to set mode

	if( p.bpp == 16 )
	{
		sceGuDrawBuffer( psm, VRAM_BUF_ADDR_16(0), VRAM_WIDTH );
		sceGuDispBuffer( p.width, p.height, VRAM_BUF_ADDR_16(1), VRAM_WIDTH );
		sceGuDepthBuffer( VRAM_BUF_ADDR_16(2), VRAM_WIDTH );
	}
	else // 32
	{
		sceGuDrawBuffer( psm, VRAM_BUF_ADDR_32(0), VRAM_WIDTH );
		sceGuDispBuffer( p.width, p.height, VRAM_BUF_ADDR_32(1), VRAM_WIDTH );
		sceGuDepthBuffer( VRAM_BUF_ADDR_32(2), VRAM_WIDTH );
	}

	g_iCurrentDisplayBufferIndex = 1;

	bNewDeviceOut = true;

	this->SetDefaultRenderStates();

	ResolutionChanged();

	return "";	// successfully set mode
}

void RageDisplay_PSP::SetViewport( int shift_left, int shift_down )
{
	/* left and down are on a 0..SCREEN_WIDTH, 0..SCREEN_HEIGHT scale.
	 * Scale them to the actual viewport range. */
	shift_left = int( shift_left * float(g_CurrentParams.width) / SCREEN_WIDTH );
	shift_down = int( shift_down * float(g_CurrentParams.height) / SCREEN_HEIGHT );

	sceGuViewport( 2048 + shift_left, 2048 - shift_down, g_CurrentParams.width, g_CurrentParams.height );
}

int RageDisplay_PSP::GetMaxTextureSize() const
{
	return 512;
}

bool RageDisplay_PSP::BeginFrame()
{
	sceGuClear( GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT );
	return true;
}

void RageDisplay_PSP::EndFrame()
{
	FinishGuList();
	sceGuSync( GU_SYNC_FINISH, GU_SYNC_WHAT_DONE );

	if( g_CurrentParams.vsync )
	{
		static uint32_t lastVcount;

		lastVcount++;

		uint32_t nowVcount = sceDisplayGetVcount();
		if( lastVcount == nowVcount )
		{
			if( !sceDisplayIsVblank() )
				sceDisplayWaitVblankStart();
		}
		else
			lastVcount = nowVcount;
	}

	sceGuSwapBuffers();
	g_iCurrentDisplayBufferIndex ^= 1;

	ProcessStatsOnFlip();
	StartGuList();
}

RageSurface* RageDisplay_PSP::CreateScreenshot()
{
	const uint8_t *displayBuffer;
	if( g_CurrentParams.bpp == 16 )
		displayBuffer = (const uint8_t*)((uint32_t)VRAM_BUF_ADDR_16(g_iCurrentDisplayBufferIndex) | VRAM_NO_CASHE_ADDR);
	else if ( g_CurrentParams.bpp == 32 )
		displayBuffer = (const uint8_t*)((uint32_t)VRAM_BUF_ADDR_32(g_iCurrentDisplayBufferIndex) | VRAM_NO_CASHE_ADDR);
	else
		return NULL;

	int width = g_CurrentParams.width;
	int height = g_CurrentParams.height;
	int psm = FindBackBufferType( g_CurrentParams.bpp );

	const PixelFormatDesc &desc = PIXEL_FORMAT_DESC[FMT_RGBA8];

	RageSurface *image = CreateSurface( width, height, desc.bpp,
		desc.masks[0], desc.masks[1], desc.masks[2], 0 );

	if( psm == GU_PSM_8888 )
	{
		for( int i = 0; i < height; ++i )
			memcpy( &image->pixels[image->pitch * i], &displayBuffer[VRAM_WIDTH * 4 * i], width * 4 );
	}
	else if( psm == GU_PSM_5650 )
	{
		const uint16_t *src = (const uint16_t*)displayBuffer;
		uint8_t *dest = (uint8_t*)image->pixels;
		for( int i = 0; i < height; ++i )
		{
			for( int j = 0; j < width; ++j )
				((uint32_t*)dest)[j] = ARGB8888fromRGB565( src[j] );

			src += VRAM_WIDTH;
			dest += image->pitch;
		}
	}
	else
	{
		delete image;
		image = NULL;
	}

	return image;
}

RageDisplay::VideoModeParams RageDisplay_PSP::GetVideoModeParams() const
{
	return g_CurrentParams;
}

void RageDisplay_PSP::SendCurrentMatrices()
{
	CheckGuList();

	if( UpdateProjectionMatrix() )
		sceGuSetMatrix( GU_PROJECTION, (const ScePspFMatrix4*)GetProjectionTop() );

	if( (UpdateViewMatrix() | UpdateCenteringMatrix()) )
	{
		RageMatrix m;
		RageMatrixMultiply( &m, GetCentering(), GetViewTop() );
		sceGuSetMatrix( GU_VIEW, (const ScePspFMatrix4*)&m );
	}

	if( UpdateWorldMatrix() )
		sceGuSetMatrix( GU_MODEL, (const ScePspFMatrix4*)GetWorldTop() );

	if( UpdateTextureMatrix() )
		sceGuSetMatrix( GU_TEXTURE, (const ScePspFMatrix4*)GetTextureTop() );
}

class RageCompiledGeometryPSP : public RageCompiledGeometry
{
#if 0 // not work well
public:
	void Allocate( const vector<msMesh> &vMeshes )
	{
		m_vVertex.resize( GetTotalVertices() );
		m_vTriangles.resize( GetTotalTriangles() );
	}

	void Change( const vector<msMesh> &vMeshes )
	{
		for( unsigned i=0; i<vMeshes.size(); i++ )
		{
			const MeshInfo& meshInfo = m_vMeshInfo[i];
			const vector<RageModelVertex> &Vertices = vMeshes[i].Vertices;
			const vector<msTriangle> &Triangles = vMeshes[i].Triangles;
			unsigned j, k;

			for( j=0; j<Vertices.size(); j++ )
				memcpy( &m_vVertex[meshInfo.iVertexStart+j], &Vertices[j], sizeof(RageModelVertex2) );

			for( j=0; j<Triangles.size(); j++ )
				for( k=0; k<3; k++ )
					m_vTriangles[meshInfo.iTriangleStart+j].nVertexIndices[k] = meshInfo.iVertexStart + Triangles[j].nVertexIndices[k];
		}
	}

	void Draw( int iMeshIndex ) const
	{
		const MeshInfo &meshInfo = m_vMeshInfo[iMeshIndex];

		sceGuDrawArray(
			GU_TRIANGLES,
			RAGE_MODEL_VERTEX2_FORMAT|GU_TRANSFORM_3D,
			meshInfo.iTriangleCount * 3,
			&m_vTriangles[meshInfo.iTriangleStart],
			&m_vVertex[0]
		);
	}

protected:
	vector<RageModelVertex2>	m_vVertex;
	vector<msTriangle>			m_vTriangles;

#else

public:
	void Allocate( const vector<msMesh> &vMeshes ) {}
	void Change( const vector<msMesh> &vMeshes ) {}
	void Draw( int iMeshIndex ) const {}

#endif
};

RageCompiledGeometry* RageDisplay_PSP::CreateCompiledGeometry()
{
	return new RageCompiledGeometryPSP;
}

void RageDisplay_PSP::DeleteCompiledGeometry( RageCompiledGeometry* p )
{
	delete p;
}

static void DrawVertices( const RageSpriteVertex v[], int iNumVerts, int prim )
{
	const int size = iNumVerts * sizeof(RageSpriteVertex);
	CheckGuList( size );

	RageSpriteVertex *vertices = (RageSpriteVertex*)sceGuGetMemory( size );
	memcpy( vertices, v, size );

	sceGuDrawArray( prim, RAGE_SPRITE_VERTEX_FORMAT|GU_TRANSFORM_3D, iNumVerts, NULL, vertices );
	CheckGuList();
}

void RageDisplay_PSP::DrawQuadsInternal( const RageSpriteVertex v[], int iNumVerts )
{
	SendCurrentMatrices();

	const int size = iNumVerts * sizeof(RageSpriteVertex);
	CheckGuList( size );

	RageSpriteVertex *vertices = (RageSpriteVertex*)sceGuGetMemory( size );
	memcpy( vertices, v, size );

	sceGuDrawArrayN( GU_TRIANGLE_FAN, RAGE_SPRITE_VERTEX_FORMAT|GU_TRANSFORM_3D, 4, iNumVerts/4, NULL, vertices );
	CheckGuList();
}

void RageDisplay_PSP::DrawQuadStripInternal( const RageSpriteVertex v[], int iNumVerts )
{
	SendCurrentMatrices();
	DrawVertices( v, iNumVerts, GU_TRIANGLE_STRIP );
}

void RageDisplay_PSP::DrawFanInternal( const RageSpriteVertex v[], int iNumVerts )
{
	SendCurrentMatrices();
	DrawVertices( v, iNumVerts, GU_TRIANGLE_FAN );
}

void RageDisplay_PSP::DrawStripInternal( const RageSpriteVertex v[], int iNumVerts )
{
	SendCurrentMatrices();
	DrawVertices( v, iNumVerts, GU_TRIANGLE_STRIP );
}

void RageDisplay_PSP::DrawTrianglesInternal( const RageSpriteVertex v[], int iNumVerts )
{
	SendCurrentMatrices();
	DrawVertices( v, iNumVerts, GU_TRIANGLES );
}

#define SLICE_SIZE 64

// draw with optimized way as possible
void RageDisplay_PSP::DrawRectAngle( const RageSpriteVertex v[4] )
{
	RageVColor color = v[0].c;

	const float u_min = v[0].t.x, u_max = v[3].t.x;
	const float v_min = v[0].t.y, v_max = v[1].t.y;

	if( !sceGuGetStatus( GU_TEXTURE_2D ) ||
		v[1].c != color || v[2].c != color || v[3].c != color ||
		(u_max - u_min <= SLICE_SIZE && v_max - v_min <= SLICE_SIZE) )
	{
		DrawQuad( v );
		return;
	}

	SendCurrentMatrices();

	const float x_min = v[0].p.x, x_max = v[3].p.x;
	const float y_min = v[0].p.y, y_max = v[1].p.y;

	const float sw = SLICE_SIZE * ((x_max - x_min) / (u_max - u_min));
	const float sh = SLICE_SIZE * ((y_max - y_min) / (v_max - v_min));

	const int u_count = ((int)ceilf(u_max - u_min) + SLICE_SIZE - 1) / SLICE_SIZE;
	const int v_count = (((int)ceilf(v_max - v_min) + SLICE_SIZE - 1) / SLICE_SIZE) * 2 + 2;

	const int size = u_count * v_count * sizeof(RageSpriteVertex);
	CheckGuList( size );

	RageSpriteVertex *vertices = (RageSpriteVertex*)sceGuGetMemory( size );
	const RageSpriteVertex *v_top = vertices;

	float u0, u1, v0, v1;
	float x1, x2, y1, y2;

	bool bBreak1 = false;
	for( u0 = u_min, x1 = x_min; !bBreak1; u0 = u1, x1 = x2 )
	{
		u1 = u0 + SLICE_SIZE;
		x2 = x1 + sw;
		if( u1 >= u_max )
		{
			u1 = u_max;
			x2 = x_max;
			bBreak1 = true;
		}

		v0 = v_min;
		y1 = y_min;

		vertices->t.x = u0;
		vertices->t.y = v0;
		vertices->c = color;
		vertices->p.x = x1;
		vertices->p.y = y1;
		vertices->p.z = 0.0f;
		vertices++;

		vertices->t.x = u1;
		vertices->t.y = v0;
		vertices->c = color;
		vertices->p.x = x2;
		vertices->p.y = y1;
		vertices->p.z = 0.0f;
		vertices++;

		bool bBreak2 = false;
		for( ; !bBreak2; v0 = v1, y1 = y2 )
		{
			v1 = v0 + SLICE_SIZE;
			y2 = y1 + sh;
			if( v1 >= v_max )
			{
				v1 = v_max;
				y2 = y_max;
				bBreak2 = true;
			}

			vertices->t.x = u0;
			vertices->t.y = v1;
			vertices->c = color;
			vertices->p.x = x1;
			vertices->p.y = y2;
			vertices->p.z = 0.0f;
			vertices++;

			vertices->t.x = u1;
			vertices->t.y = v1;
			vertices->c = color;
			vertices->p.x = x2;
			vertices->p.y = y2;
			vertices->p.z = 0.0f;
			vertices++;
		}
	}

	sceGuDrawArrayN( GU_TRIANGLE_STRIP, RAGE_SPRITE_VERTEX_FORMAT|GU_TRANSFORM_3D, v_count, u_count, NULL, v_top );
	CheckGuList();
}

void RageDisplay_PSP::DrawCompiledGeometryInternal( const RageCompiledGeometry *p, int iMeshIndex )
{
	SendCurrentMatrices();
	p->Draw( iMeshIndex );
	CheckGuList();
}

void RageDisplay_PSP::ClearAllTextures()
{
	DISABLE_GU_STATUS( GU_TEXTURE_2D );
//	g_ActiveTexture = NULL;
}

void RageDisplay_PSP::SetTexture( int iTextureUnitIndex, RageTexture* pTexture )
{
	CheckGuList();

	if( pTexture )
	{
		ENABLE_GU_STATUS( GU_TEXTURE_2D );

		PspTexture *texture = (PspTexture*)pTexture->GetTexHandle();

		if( g_ActiveTexture != texture )
		{
			if( texture->palette )
			{
				sceGuClutMode( GU_PSM_8888, 0, 0xFF, 0 );
				sceGuClutLoad( 32, texture->palette );
			}

			sceGuTexMode( texture->psm, 0, 0, texture->swizzle );
			sceGuTexImage( 0, texture->w, texture->h, texture->w, texture->pixels );

			g_ActiveTexture = texture;
		}
	}
	else
	{
		DISABLE_GU_STATUS( GU_TEXTURE_2D );
	}
}

void RageDisplay_PSP::SetTextureModeModulate()
{
	sceGuTexFunc( GU_TFX_MODULATE, GU_TCC_RGBA );
}

void RageDisplay_PSP::SetTextureModeGlow( GlowMode m )
{
	sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF );
}

void RageDisplay_PSP::SetTextureModeAdd()
{
	sceGuTexFunc( GU_TFX_ADD, GU_TCC_RGBA );
}

void RageDisplay_PSP::SetTextureFiltering( bool b )
{
}

// depth offset doesnt work well...
void RageDisplay_PSP::SetBlendMode( BlendMode mode )
{
//	sceGuDepthOffset( 3000 );
	sceGuDepthOffset( 1000 );
	switch( mode )
	{
	case BLEND_NORMAL:
		sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0 );
		break;

	case BLEND_ADD:
		sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF );
		break;

	case BLEND_NO_EFFECT:
		sceGuBlendFunc( GU_ADD, GU_FIX, GU_FIX, 0xFFFFFFFF, 0 );
		/* This is almost exclusively used to draw masks to the Z-buffer.  Make sure
		 * masks always win the depth test when drawn at the same position. */
//		sceGuDepthOffset( 1000 ); // this bias is bigger than it needs to be to handle the coplanar case
		sceGuDepthOffset( 3000 );
		break;

	default:
		ASSERT(0);
	}
}

bool RageDisplay_PSP::IsZWriteEnabled() const
{
	return g_bZWrite;
}

bool RageDisplay_PSP::IsZTestEnabled() const
{
	return sceGuGetStatus( GU_DEPTH_TEST ) != 0;
}

void RageDisplay_PSP::ClearZBuffer()
{
	sceGuClear( GU_DEPTH_BUFFER_BIT );
}

void RageDisplay_PSP::SetZWrite( bool b )
{
	sceGuDepthMask( b ? GU_FALSE : GU_TRUE );
	g_bZWrite = b;
}

void RageDisplay_PSP::SetZTestMode( ZTestMode mode )
{
	switch( mode )
	{
	case ZTEST_OFF:
		DISABLE_GU_STATUS( GU_DEPTH_TEST );
		break;

	case ZTEST_WRITE_ON_PASS:
		ENABLE_GU_STATUS( GU_DEPTH_TEST );
		sceGuDepthFunc( GU_GEQUAL );
		break;

	case ZTEST_WRITE_ON_FAIL:
		ENABLE_GU_STATUS( GU_DEPTH_TEST );
		sceGuDepthFunc( GU_LESS );
		break;

	default:
		ASSERT(0);
	}
}

void RageDisplay_PSP::SetTextureWrapping( bool b )
{
	int mode = b ? GU_REPEAT : GU_CLAMP;
	sceGuTexWrap( mode, mode );
}

void RageDisplay_PSP::SetMaterial( 
	const RageColor &emissive,
	const RageColor &ambient,
	const RageColor &diffuse,
	const RageColor &specular,
	float shininess
	)
{
	// TRICKY:  If lighting is off, then setting the material 
	// will have no effect.  Even if lighting is off, we still
	// want Models to have basic color and transparency.
	// We can do this fake lighting by setting the vertex color.
	if( sceGuGetStatus( GU_LIGHTING ) )
	{
		RageVColor e(emissive), a(ambient), d(diffuse), s(specular);
		sceGuModelColor( e, a, d, s );
		sceGuSpecular( shininess );
	}
	else
	{
		RageVColor c = RageVColor( emissive + ambient + diffuse );
		sceGuColor( c );
	}
}

void RageDisplay_PSP::SetLighting( bool b )
{
	if( b )
		sceGuEnable( GU_LIGHTING );
	else
		sceGuDisable( GU_LIGHTING );
}

void RageDisplay_PSP::SetLightOff()
{
	sceGuDisable( GU_LIGHT0 );
	sceGuDisable( GU_LIGHT1 );
}

void RageDisplay_PSP::SetLightDirectional(
	const RageColor &ambient,
	const RageColor &diffuse,
	const RageColor &specular,
	const RageVector3 &dir )
{
	CheckGuList();

	sceGuEnable( GU_LIGHT0 );
	sceGuEnable( GU_LIGHT1 );

	sceGuLight( 0, GU_DIRECTIONAL, GU_AMBIENT_AND_DIFFUSE, (const ScePspFVector3*)&dir );
	sceGuLight( 1, GU_DIRECTIONAL, GU_DIFFUSE_AND_SPECULAR, (const ScePspFVector3*)&dir );

	RageVColor a(ambient), d(diffuse), s(specular);
	sceGuLightColor( 0, GU_AMBIENT, a );
	sceGuLightColor( 0, GU_DIFFUSE, d );
	sceGuLightColor( 1, GU_DIFFUSE, d );
	sceGuLightColor( 1, GU_SPECULAR, s );

	sceGuLightAtt( 0, 1.0f, 0.0f, 0.0f );
	sceGuLightAtt( 1, 1.0f, 0.0f, 0.0f );
}

void RageDisplay_PSP::SetCullMode( CullMode mode )
{
	switch( mode )
	{
	case CULL_BACK:
		ENABLE_GU_STATUS( GU_CULL_FACE );
		sceGuFrontFace( GU_CCW );
		break;

	case CULL_FRONT:
		ENABLE_GU_STATUS( GU_CULL_FACE );
		sceGuFrontFace( GU_CW );
		break;

	case CULL_NONE:
		DISABLE_GU_STATUS( GU_CULL_FACE );
		break;

	default:
		ASSERT(0);
	}
}

const RageDisplay::PixelFormatDesc *RageDisplay_PSP::GetPixelFormatDesc( PixelFormat pf ) const
{
	ASSERT( pf < NUM_PIX_FORMATS );
	return &PIXEL_FORMAT_DESC[pf];
}

void RageDisplay_PSP::DeleteTexture( unsigned uTexHandle )
{
	PspTexture *texture = (PspTexture*)uTexHandle;

	if( g_ActiveTexture == texture )
		g_ActiveTexture = NULL;

	delete texture;
}

RageDisplay::PixelFormat RageDisplay_PSP::GetImgPixelFormat( RageSurface* &img, bool &FreeImg, int width, int height )
{
	PixelFormat pixfmt = FindPixelFormat( img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask );
	
	if( pixfmt == NUM_PIX_FORMATS || !SupportsTextureFormat( pixfmt ) )
	{
		/* The source isn't in a supported, known pixel format.  We need to convert
		 * it ourself.  Just convert it to RGBA8, and let OpenGL convert it back
		 * down to whatever the actual pixel format is.  This is a very slow code
		 * path, which should almost never be used. */
		pixfmt = FMT_RGBA8;
		ASSERT( SupportsTextureFormat(pixfmt) );

		const PixelFormatDesc *pfd = DISPLAY->GetPixelFormatDesc( pixfmt );

		RageSurface *imgconv = CreateSurface( width, height,
			pfd->bpp, pfd->masks[0], pfd->masks[1], pfd->masks[2], pfd->masks[3] );
		RageSurfaceUtils::Blit( img, imgconv, width, height );
		img = imgconv;
		FreeImg = true;
	}
	else
		FreeImg = false;

	return pixfmt;
}

unsigned RageDisplay_PSP::CreateTexture( 
	PixelFormat pixfmt,
	RageSurface* img,
	bool bGenerateMipMaps )
{
	ASSERT( img->w == power_of_two(img->w) && img->h == power_of_two(img->h) );

	/* Find the pixel format of the image we've been given. */
	bool freeImg;
	PixelFormat imgpixfmt = GetImgPixelFormat( img, freeImg, img->w, img->h );

	PspTexture *texture = new PspTexture;
	texture->psm = PSP_GU_FORMATS[pixfmt];
	texture->w = img->w;
	texture->h = img->h;
	texture->pixelSize = img->pitch * img->h;
	texture->pixels = (uint8_t*)memalign( 16, texture->pixelSize );

	if( pixfmt == FMT_PAL )
	{
		texture->palette = (uint32_t*)memalign( 16, 256*4 );
		memcpy( texture->palette, img->format->palette->colors, 256*4 );
	}

	CopyImageToTexture( texture, img, PSP_GU_FORMATS[imgpixfmt] );

	if( freeImg )
		delete img;

	SwizzleTexture( texture );

	sceKernelDcacheWritebackAll();
	return (unsigned)texture;
}

void RageDisplay_PSP::UpdateTexture(
	unsigned uTexHandle,
	RageSurface* img,
	int width, int height )
{
	PspTexture *texture = (PspTexture*)uTexHandle;

	texture->w = power_of_two( width );
	texture->h = power_of_two( height );

	uint32_t texSize = GetTextureSize( texture->psm, texture->w, texture->h );

	if( texture->pixelSize < texSize )
	{
		free( texture->pixels );
		texture->pixels = (uint8_t*)memalign( 16, texSize );
		texture->pixelSize = texSize;
	}

	texture->swizzle = 0;

	bool freeImg;
	PixelFormat imgpixfmt = GetImgPixelFormat( img, freeImg, img->w, img->h );

	CopyImageToTexture( texture, img, PSP_GU_FORMATS[imgpixfmt] );

	if( freeImg )
		delete img;

	if( g_ActiveTexture == texture )
		g_ActiveTexture = NULL;

	sceKernelDcacheWritebackAll();
}

void RageDisplay_PSP::SetAlphaTest( bool b )
{
	if( b )
	{
		ENABLE_GU_STATUS( GU_ALPHA_TEST );
	}
	else
	{
		DISABLE_GU_STATUS( GU_ALPHA_TEST );
	}
}

void RageDisplay_PSP::GetOrthoMatrix( RageMatrix* pOut, float l, float r, float b, float t, float zn, float zf )
{
#if 1
	__asm__ volatile (
		"vmidt.q	M100\n"
		"mtv		%2, S000\n"
		"mtv		%4, S010\n"
		"mtv		%6, S020\n"
		"mtv		%1, S001\n"
		"mtv		%3, S011\n"
		"mtv		%5, S021\n"
		"vsub.t		R002, R000, R001\n"
		"vrcp.t		R002, R002\n"
		"vmul.s		S100, S100[2], S002\n"
		"vmul.s		S111, S111[2], S012\n"
		"vmul.s		S122, S122[2], S022[-x]\n"
		"vsub.t		R103, R000[-x,-y,-z], R001\n"
		"vmul.t		R103, R103, R002\n"
		"usv.q		R100,  0 + %0\n"
		"usv.q		R101, 16 + %0\n"
		"usv.q		R102, 32 + %0\n"
		"usv.q		R103, 48 + %0\n"
		: "=m"(*pOut)
		: "r"(l), "r"(r), "r"(b), "r"(t), "r"(zn), "r"(zf)
	);
#else
	*pOut = RageMatrix(
		2/(r-l),      0,            0,           0,
		0,            2/(t-b),      0,           0,
		0,            0,            -2/(zf-zn),   0,
		-(r+l)/(r-l), -(t+b)/(t-b), -(zf+zn)/(zf-zn),  1 );
#endif
}

bool RageDisplay_PSP::SupportsTextureFormat( PixelFormat pixfmt, bool realtime )
{
	return PSP_GU_FORMATS[pixfmt] >= 0;
}

void RageDisplay_PSP::SetSphereEnironmentMapping( bool b )
{
}
