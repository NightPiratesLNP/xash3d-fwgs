/*
gl_draw.c - orthogonal drawing stuff
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "gl_local.h"

/*
=============
R_GetImageParms
=============
*/
void R_GetTextureParms( int *w, int *h, int texnum )
{
	gl_texture_t	*glt;

	glt = R_GetTexture( texnum );
	if( w ) *w = glt->srcWidth;
	if( h ) *h = glt->srcHeight;
}

/*
=============
R_GetSpriteParms

same as GetImageParms but used
for sprite models
=============
*/
void R_GetSpriteParms( int *frameWidth, int *frameHeight, int *numFrames, int currentFrame, const model_t *pSprite )
{
	mspriteframe_t	*pFrame;

	if( !pSprite || pSprite->type != mod_sprite ) return; // bad model ?
	pFrame = R_GetSpriteFrame( pSprite, currentFrame, 0.0f );

	if( frameWidth ) *frameWidth = pFrame->width;
	if( frameHeight ) *frameHeight = pFrame->height;
	if( numFrames ) *numFrames = pSprite->numframes;
}

int R_GetSpriteTexture( const model_t *m_pSpriteModel, int frame )
{
	if( !m_pSpriteModel || m_pSpriteModel->type != mod_sprite || !m_pSpriteModel->cache.data )
		return 0;

	return R_GetSpriteFrame( m_pSpriteModel, frame, 0.0f )->gl_texturenum;
}

/*
=============
R_DrawStretchPic
=============
*/
void R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int texnum )
{
	GL_Bind( XASH_TEXTURE0, texnum );

	pglBegin( GL_QUADS );
		pglTexCoord2f( s1, t1 );
		pglVertex2f( x, y );

		pglTexCoord2f( s2, t1 );
		pglVertex2f( x + w, y );

		pglTexCoord2f( s2, t2 );
		pglVertex2f( x + w, y + h );

		pglTexCoord2f( s1, t2 );
		pglVertex2f( x, y + h );
	pglEnd();
}

/*
=============
R_DrawStretchRaw
=============
*/
void R_DrawStretchRaw( float x, float y, float w, float h, int cols, int rows, const byte *data, qboolean dirty )
{
	byte		*raw = NULL;
	gl_texture_t	*tex;

	if( !GL_Support( GL_ARB_TEXTURE_NPOT_EXT ))
	{
		int	width = 1, height = 1;

		// check the dimensions
		width = NearestPOW( cols, true );
		height = NearestPOW( rows, false );

		if( cols != width || rows != height )
		{
			raw = GL_ResampleTexture( data, cols, rows, width, height, false );
			cols = width;
			rows = height;
		}
	}
	else
	{
		raw = (byte *)data;
	}

	if( cols > glConfig.max_2d_texture_size )
		gEngfuncs.Host_Error( "%s: size %i exceeds hardware limits\n", __func__, cols );
	if( rows > glConfig.max_2d_texture_size )
		gEngfuncs.Host_Error( "%s: size %i exceeds hardware limits\n", __func__, rows );

	pglDisable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	tex = R_GetTexture( tr.cinTexture );
	GL_Bind( XASH_TEXTURE0, tr.cinTexture );

	if( cols == tex->width && rows == tex->height )
	{
		if( dirty )
		{
			pglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_BGRA, GL_UNSIGNED_BYTE, raw );
		}
	}
	else
	{
		tex->size = cols * rows * 4;
		tex->width = cols;
		tex->height = rows;
		if( dirty )
		{
			pglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, cols, rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, raw );
		}
	}

	pglBegin( GL_QUADS );
	pglTexCoord2f( 0, 0 );
	pglVertex2f( x, y );
	pglTexCoord2f( 1, 0 );
	pglVertex2f( x + w, y );
	pglTexCoord2f( 1, 1 );
	pglVertex2f( x + w, y + h );
	pglTexCoord2f( 0, 1 );
	pglVertex2f( x, y + h );
	pglEnd();
}

/*
=============
R_UploadStretchRaw
=============
*/
void R_UploadStretchRaw( int texture, int cols, int rows, int width, int height, const byte *data )
{
	byte		*raw = NULL;
	gl_texture_t	*tex;

	if( !GL_Support( GL_ARB_TEXTURE_NPOT_EXT ))
	{
		// check the dimensions
		width = NearestPOW( width, true );
		height = NearestPOW( height, false );
	}
	else
	{
		width = bound( 128, width, glConfig.max_2d_texture_size );
		height = bound( 128, height, glConfig.max_2d_texture_size );
	}

	if( cols != width || rows != height )
	{
		raw = GL_ResampleTexture( data, cols, rows, width, height, false );
		cols = width;
		rows = height;
	}
	else
	{
		raw = (byte *)data;
	}

	if( cols > glConfig.max_2d_texture_size )
		gEngfuncs.Host_Error( "%s: size %i exceeds hardware limits\n", __func__, cols );
	if( rows > glConfig.max_2d_texture_size )
		gEngfuncs.Host_Error( "%s: size %i exceeds hardware limits\n", __func__, rows );

	tex = R_GetTexture( texture );
	GL_Bind( GL_KEEP_UNIT, texture );
	tex->width = cols;
	tex->height = rows;

	pglTexImage2D( GL_TEXTURE_2D, 0, tex->format, cols, rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, raw );
	GL_ApplyTextureParams( tex );
}

/*
===============
R_Set2DMode
===============
*/
void R_Set2DMode( qboolean enable )
{
    if( enable )
    {
        float scale;
        int native_w, native_h;
        int scaled_w, scaled_h;
        int offset_center_x, offset_center_y;

        scale = gEngfuncs.pfnGetCvarFloat( "vid_scale" );
        if( scale < 1.0f ) scale = 1.0f;

        native_w = gpGlobals->width;
        native_h = gpGlobals->height;

        scaled_w = (int)(native_w / scale);
        scaled_h = (int)(native_h / scale);

        offset_center_x = (native_w - scaled_w) / 2;
        offset_center_y = (native_h - scaled_h) / 2;

        pglViewport(offset_center_x, offset_center_y, scaled_w, scaled_h);
        pglScissor(offset_center_x, offset_center_y, scaled_w, scaled_h);

        pglMatrixMode(GL_PROJECTION);
        pglLoadIdentity();
        pglOrtho(0, gpGlobals->width / scale, gpGlobals->height / scale, 0, -99999, 99999);
        pglMatrixMode(GL_MODELVIEW);
        pglLoadIdentity();

        pglDisable(GL_DEPTH_TEST);
        pglDisable(GL_CULL_FACE);
    }
    else
    {
        pglMatrixMode(GL_PROJECTION);
        pglLoadIdentity();
        pglOrtho(0, gpGlobals->width, gpGlobals->height, 0, -99999, 99999);
        pglMatrixMode(GL_MODELVIEW);
        pglLoadIdentity();

        pglViewport(0, 0, gpGlobals->width, gpGlobals->height);
        pglScissor(0, 0, gpGlobals->width, gpGlobals->height);
    }
}
