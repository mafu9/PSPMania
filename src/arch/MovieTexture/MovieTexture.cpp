#include "global.h"
#include "MovieTexture.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "RageFile.h"

#include "MovieTexture_PSPMpeg.h"

RageMovieTexture *MakeRageMovieTexture( RageTextureID ID )
{
	return new MovieTexture_PSPMpeg( ID );
}
