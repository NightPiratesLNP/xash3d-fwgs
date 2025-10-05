#define APIENTRY_LINKAGE
#include "gl_local.h"
#include "gl_export.h"
#include "cvar.h"

#if XASH_GL4ES
#include "gl4es/include/gl4esinit.h"
#endif

ref_api_t      gEngfuncs;
ref_globals_t *gpGlobals;
ref_client_t  *gp_cl;
ref_host_t    *gp_host;

/* Global state for optional internal scaling render target (FBO) */
static GLuint g_scale_fbo = 0;
static GLuint g_scale_tex = 0;
static int    g_scale_rt_width = 0;
static int    g_scale_rt_height = 0;
static float  g_scale_x = 1.0f;
static float  g_scale_y = 1.0f;

/*
========================
R_CreateScaleRenderTarget
========================
*/
qboolean R_CreateScaleRenderTarget( int width, int height ) // static kaldırıldı
{
	GLenum status;
	qboolean fbo_supported = true; // değişkeni blok başına taşıdık

	// Clean existing if any
	if( g_scale_fbo || g_scale_tex )
	{
		if( g_scale_fbo )
		{
			pglDeleteFramebuffers( 1, &g_scale_fbo );
			g_scale_fbo = 0;
		}
		if( g_scale_tex )
		{
			pglDeleteTextures( 1, &g_scale_tex );
			g_scale_tex = 0;
		}
		g_scale_rt_width = g_scale_rt_height = 0;
		g_scale_x = g_scale_y = 1.0f;
	}

	if( width <= 0 || height <= 0 )
		return false;

	// Create texture for render target
	pglGenTextures( 1, &g_scale_tex );
	if( !g_scale_tex )
		return false;

	pglBindTexture( GL_TEXTURE_2D, g_scale_tex );
	pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	// allocate texture storage (RGBA8)
	pglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	pglBindTexture( GL_TEXTURE_2D, 0 );

	// Create framebuffer and attach texture
	pglGenFramebuffers( 1, &g_scale_fbo );
	if( !g_scale_fbo )
	{
		pglDeleteTextures( 1, &g_scale_tex );
		g_scale_tex = 0;
		return false;
	}

	pglBindFramebuffer( GL_FRAMEBUFFER, g_scale_fbo );
	pglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_scale_tex, 0 );

#ifdef _WIN32
	fbo_supported = (g_scale_fbo != 0 && g_scale_tex != 0);
#else
	// Diğer platformlar için FBO status kontrolü
	status = GL_FRAMEBUFFER_COMPLETE; // varsayılan olarak başarılı
	
	// pglCheckFramebufferStatus fonksiyon pointer'ını kontrol et
	if( pglCheckFramebufferStatus )
	{
		status = pglCheckFramebufferStatus( GL_FRAMEBUFFER );
	}
	
	fbo_supported = (status == GL_FRAMEBUFFER_COMPLETE);
#endif

	pglBindFramebuffer( GL_FRAMEBUFFER, 0 );

	if( !fbo_supported )
	{
		// cleanup on failure
		if( g_scale_fbo ) { pglDeleteFramebuffers( 1, &g_scale_fbo ); g_scale_fbo = 0; }
		if( g_scale_tex ) { pglDeleteTextures( 1, &g_scale_tex ); g_scale_tex = 0; }
		return false;
	}

	g_scale_rt_width = width;
	g_scale_rt_height = height;
	return true;
}

/*
========================
R_DestroyScaleRenderTarget
========================
*/
void R_DestroyScaleRenderTarget( void ) // static kaldırıldı
{
	if( g_scale_fbo )
	{
		pglDeleteFramebuffers( 1, &g_scale_fbo );
		g_scale_fbo = 0;
	}
	if( g_scale_tex )
	{
		pglDeleteTextures( 1, &g_scale_tex );
		g_scale_tex = 0;
	}
	g_scale_rt_width = g_scale_rt_height = 0;
	g_scale_x = g_scale_y = 1.0f;
}

/*
========================
R_BindRenderTargetForScene
========================
*/
void R_BindRenderTargetForScene( void ) // static kaldırıldı
{
	if( g_scale_fbo )
		pglBindFramebuffer( GL_FRAMEBUFFER, g_scale_fbo );
	else
		pglBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

/*
========================
R_BlitScaleRenderTargetToScreen
========================
*/
void R_BlitScaleRenderTargetToScreen( int screen_w, int screen_h ) // static kaldırıldı
{
	GLboolean blendEnabled, depthTestEnabled;

	if( !g_scale_fbo || !g_scale_tex )
		return;

	// Bind default framebuffer for drawing to screen
	pglBindFramebuffer( GL_FRAMEBUFFER, 0 );

	// Simple orthographic full-screen blit - keep state changes minimal and try to restore states that are commonly used.
	blendEnabled = pglIsEnabled( GL_BLEND );
	depthTestEnabled = pglIsEnabled( GL_DEPTH_TEST );

	pglDisable( GL_DEPTH_TEST );
	pglDisable( GL_CULL_FACE );
	pglEnable( GL_TEXTURE_2D );

	pglMatrixMode( GL_PROJECTION );
	pglPushMatrix();
	pglLoadIdentity();
	pglOrtho( 0, screen_w, 0, screen_h, -1, 1 );

	pglMatrixMode( GL_MODELVIEW );
	pglPushMatrix();
	pglLoadIdentity();

	pglActiveTexture( XASH_TEXTURE0 );
	pglBindTexture( GL_TEXTURE_2D, g_scale_tex );

	// draw a quad that covers the whole screen
	pglBegin( GL_QUADS );
		pglTexCoord2f( 0.0f, 0.0f ); pglVertex2f( 0.0f, 0.0f );
		pglTexCoord2f( 1.0f, 0.0f ); pglVertex2f( (GLfloat)screen_w, 0.0f );
		pglTexCoord2f( 1.0f, 1.0f ); pglVertex2f( (GLfloat)screen_w, (GLfloat)screen_h );
		pglTexCoord2f( 0.0f, 1.0f ); pglVertex2f( 0.0f, (GLfloat)screen_h );
	pglEnd();

	pglBindTexture( GL_TEXTURE_2D, 0 );

	pglPopMatrix();
	pglMatrixMode( GL_PROJECTION );
	pglPopMatrix();
	pglMatrixMode( GL_MODELVIEW );

	if( depthTestEnabled ) pglEnable( GL_DEPTH_TEST ); else pglDisable( GL_DEPTH_TEST );
	if( blendEnabled ) pglEnable( GL_BLEND ); else pglDisable( GL_BLEND );
}

void _Mem_Free( void *data, const char *filename, int fileline )
{
	gEngfuncs._Mem_Free( data, filename, fileline );
}

void *_Mem_Alloc( poolhandle_t poolptr, size_t size, qboolean clear, const char *filename, int fileline )
{
	return gEngfuncs._Mem_Alloc( poolptr, size, clear, filename, fileline );
}

static void R_ClearScreen( void )
{
	pglClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	pglClear( GL_COLOR_BUFFER_BIT );
}

static const byte *R_GetTextureOriginalBuffer( unsigned int idx )
{
	gl_texture_t *glt = R_GetTexture( idx );

	if( !glt || !glt->original || !glt->original->buffer )
		return NULL;

	return glt->original->buffer;
}

/*
========================
R_SetDisplayTransform
========================
 Provide limited support for scale transforms by creating an internal render target
 (smaller render resolution) and blitting it to the screen. Rotation/offsets still
 log "not supported" as before.
 Returns true if the requested transform is accepted (or partially accepted).
========================
*/
qboolean R_SetDisplayTransform( ref_screen_rotation_t rotate, int offset_x, int offset_y, float scale_x, float scale_y ) // static kaldırıldı
{
	qboolean ret = true;
	int screen_w, screen_h, rt_w, rt_h;

	if( rotate > 0 )
	{
		//gEngfuncs.Con_Printf( "rotation transform not supported\n" );
		ret = false;
	}

	if( offset_x || offset_y )
	{
		//gEngfuncs.Con_Printf( "offset transform not supported\n" );
		ret = false;
	}

	if( scale_x != 1.0f || scale_y != 1.0f )
	{
		screen_w = gpGlobals->width;
		screen_h = gpGlobals->height;

		if( screen_w <= 0 ) screen_w = 640;
		if( screen_h <= 0 ) screen_h = 480;

		rt_w = (int)( screen_w * (1.0f / scale_x) );
		rt_h = (int)( screen_h * (1.0f / scale_y) );

		if( rt_w < 1 ) rt_w = 1;
		if( rt_h < 1 ) rt_h = 1;

		if( R_CreateScaleRenderTarget( rt_w, rt_h ) )
		{
			g_scale_x = scale_x;
			g_scale_y = scale_y;
			//Con_Reportf( S_NOTE "scale transform enabled: internal RT %ix%i -> screen %ix%i\n", rt_w, rt_h, screen_w, screen_h );
		}
		else
		{
			//gEngfuncs.Con_Printf( "scale transform not supported (FBO creation failed)\n" );
			ret = false;
		}
	}
	else
	{
		R_DestroyScaleRenderTarget();
	}

	return ret;
}

/*
=============
CL_FillRGBA

=============
*/
static void CL_FillRGBA( int rendermode, float _x, float _y, float _w, float _h, byte r, byte g, byte b, byte a )
{
	pglDisable( GL_TEXTURE_2D );
	pglEnable( GL_BLEND );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	if( rendermode == kRenderTransAdd )
		pglBlendFunc( GL_SRC_ALPHA, GL_ONE );
	else
		pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	pglColor4ub( r, g, b, a );

	pglBegin( GL_QUADS );
		pglVertex2f( _x, _y );
		pglVertex2f( _x + _w, _y );
		pglVertex2f( _x + _w, _y + _h );
		pglVertex2f( _x, _y + _h );
	pglEnd ();

	pglEnable( GL_TEXTURE_2D );
	pglDisable( GL_BLEND );
}

static qboolean Mod_LooksLikeWaterTexture( const char *name )
{
	if(( name[0] == '*' && Q_stricmp( name, REF_DEFAULT_TEXTURE )) || name[0] == '!' )
		return true;

	if( !ENGINE_GET_PARM( PARM_QUAKE_COMPATIBLE ))
	{
		if( !Q_strncmp( name, "water", 5 ) || !Q_strnicmp( name, "laser", 5 ))
			return true;
	}

	return false;
}

static void Mod_BrushUnloadTextures( model_t *mod )
{
	int i;

	for( i = 0; i < mod->numtextures; i++ )
	{
		texture_t *tx = mod->textures[i];
		if( !tx )
			continue; // free slot

		if( tx->gl_texturenum != tr.defaultTexture )
			GL_FreeTexture( tx->gl_texturenum ); // main texture

		if( !Mod_LooksLikeWaterTexture( tx->name ))
		{
			GL_FreeTexture( tx->fb_texturenum ); // luma texture
			GL_FreeTexture( tx->dt_texturenum ); // detail texture
		}
	}
}

static void Mod_UnloadTextures( model_t *mod )
{
	Assert( mod != NULL );

	switch( mod->type )
	{
	case mod_studio:
		Mod_StudioUnloadTextures( mod->cache.data );
		break;
	case mod_alias:
		Mod_AliasUnloadTextures( mod->cache.data );
		break;
	case mod_brush:
		Mod_BrushUnloadTextures( mod );
		break;
	case mod_sprite:
		Mod_SpriteUnloadTextures( mod->cache.data );
		break;
	default:
		Assert( 0 );
		break;
	}
}

static qboolean Mod_ProcessRenderData( model_t *mod, qboolean create, const byte *buf )
{
	qboolean loaded = false;

	if( !create )
	{
		if( gEngfuncs.drawFuncs->Mod_ProcessUserData )
			gEngfuncs.drawFuncs->Mod_ProcessUserData( mod, false, buf );
		Mod_UnloadTextures( mod );
		return true;
	}

	switch( mod->type )
	{
	case mod_studio:
	case mod_brush:
		loaded = true;
		break;
	case mod_sprite:
		Mod_LoadSpriteModel( mod, buf, &loaded, mod->numtexinfo );
		break;
	case mod_alias:
		Mod_LoadAliasModel( mod, buf, &loaded );
		break;
	default:
		gEngfuncs.Host_Error( "%s: unsupported type %d\n", __func__, mod->type );
		return false;
	}

	if( gEngfuncs.drawFuncs->Mod_ProcessUserData )
		gEngfuncs.drawFuncs->Mod_ProcessUserData( mod, true, buf );

	return loaded;
}

static int GL_RefGetParm( int parm, int arg )
{
	gl_texture_t *glt;

	switch( parm )
	{
	case PARM_TEX_WIDTH:
		glt = R_GetTexture( arg );
		return glt->width;
	case PARM_TEX_HEIGHT:
		glt = R_GetTexture( arg );
		return glt->height;
	case PARM_TEX_SRC_WIDTH:
		glt = R_GetTexture( arg );
		return glt->srcWidth;
	case PARM_TEX_SRC_HEIGHT:
		glt = R_GetTexture( arg );
		return glt->srcHeight;
	case PARM_TEX_GLFORMAT:
		glt = R_GetTexture( arg );
		return glt->format;
	case PARM_TEX_ENCODE:
		glt = R_GetTexture( arg );
		return glt->encode;
	case PARM_TEX_MIPCOUNT:
		glt = R_GetTexture( arg );
		return glt->numMips;
	case PARM_TEX_DEPTH:
		glt = R_GetTexture( arg );
		return glt->depth;
	case PARM_TEX_SKYBOX:
		Assert( arg >= 0 && arg < 6 );
		return tr.skyboxTextures[arg];
	case PARM_TEX_SKYTEXNUM:
		return tr.skytexturenum;
	case PARM_TEX_LIGHTMAP:
		arg = bound( 0, arg, MAX_LIGHTMAPS - 1 );
		return tr.lightmapTextures[arg];
	case PARM_TEX_TARGET:
		glt = R_GetTexture( arg );
		return glt->target;
	case PARM_TEX_TEXNUM:
		glt = R_GetTexture( arg );
		return glt->texnum;
	case PARM_TEX_FLAGS:
		glt = R_GetTexture( arg );
		return glt->flags;
	case PARM_TEX_MEMORY:
		return GL_TexMemory();
	case PARM_ACTIVE_TMU:
		return glState.activeTMU;
	case PARM_LIGHTSTYLEVALUE:
		arg = bound( 0, arg, MAX_LIGHTSTYLES - 1 );
		return tr.lightstylevalue[arg];
	case PARM_MAX_IMAGE_UNITS:
		return GL_MaxTextureUnits();
	case PARM_REBUILD_GAMMA:
		return glConfig.softwareGammaUpdate;
	case PARM_GL_CONTEXT_TYPE:
		return glConfig.context;
	case PARM_GLES_WRAPPER:
		return glConfig.wrapper;
	case PARM_STENCIL_ACTIVE:
		return glState.stencilEnabled;
	case PARM_TEX_FILTERING:
		if( arg < 0 )
			return gl_texture_nearest.value == 0.0f;

		return GL_TextureFilteringEnabled( R_GetTexture( arg ));
	default:
		return ENGINE_GET_PARM_( parm, arg );
	}
	return 0;
}

static void R_GetDetailScaleForTexture( int texture, float *xScale, float *yScale )
{
	gl_texture_t *glt = R_GetTexture( texture );

	if( xScale ) *xScale = glt->xscale;
	if( yScale ) *yScale = glt->yscale;
}

static void R_GetExtraParmsForTexture( int texture, byte *red, byte *green, byte *blue, byte *density )
{
	gl_texture_t *glt = R_GetTexture( texture );

	if( red ) *red = glt->fogParams[0];
	if( green ) *green = glt->fogParams[1];
	if( blue ) *blue = glt->fogParams[2];
	if( density ) *density = glt->fogParams[3];
}


static void R_SetCurrentEntity( cl_entity_t *ent )
{
	RI.currententity = ent;

	// set model also
	if( RI.currententity != NULL )
	{
		RI.currentmodel = RI.currententity->model;
	}
}

static void R_SetCurrentModel( model_t *mod )
{
	RI.currentmodel = mod;
}

static float R_GetFrameTime( void )
{
	return tr.frametime;
}

static const char *GL_TextureName( unsigned int texnum )
{
	return R_GetTexture( texnum )->name;
}

static const byte *GL_TextureData( unsigned int texnum )
{
	rgbdata_t *pic = R_GetTexture( texnum )->original;

	if( pic != NULL )
		return pic->buffer;
	return NULL;
}

static void R_ProcessEntData( qboolean allocate, cl_entity_t *entities, unsigned int max_entities )
{
	if( !allocate )
	{
		tr.draw_list->num_solid_entities = 0;
		tr.draw_list->num_trans_entities = 0;
		tr.draw_list->num_beam_entities = 0;

		tr.max_entities = 0;
		tr.entities = NULL;
	}
	else
	{
		tr.max_entities = max_entities;
		tr.entities = entities;
	}

	if( gEngfuncs.drawFuncs->R_ProcessEntData )
		gEngfuncs.drawFuncs->R_ProcessEntData( allocate );
}

static void GAME_EXPORT R_Flush( unsigned int flags )
{
	// stub
}

/*
=============
R_SetSkyCloudsTextures

Quake sky cloud texture was processed by the engine,
remember them for easier access during rendering
==============
*/
static void GAME_EXPORT R_SetSkyCloudsTextures( int solidskyTexture, int alphaskyTexture )
{
	tr.solidskyTexture = solidskyTexture;
	tr.alphaskyTexture = alphaskyTexture;
}

/*
===============
R_SetupSky
===============
*/
static void GAME_EXPORT R_SetupSky( int *skyboxTextures )
{
	int i;

	R_UnloadSkybox();

	if( !skyboxTextures )
		return;

	for( i = 0; i < SKYBOX_MAX_SIDES; i++ )
		tr.skyboxTextures[i] = skyboxTextures[i];
}

static void GAME_EXPORT VGUI_UploadTextureBlock( int drawX, int drawY, const byte *rgba, int blockWidth, int blockHeight )
{
	pglTexSubImage2D( GL_TEXTURE_2D, 0, drawX, drawY, blockWidth, blockHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgba );
}

static void GAME_EXPORT VGUI_SetupDrawing( qboolean rect )
{
	pglEnable( GL_BLEND );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	if( rect )
	{
		pglDisable( GL_ALPHA_TEST );
	}
	else
	{
		pglEnable( GL_ALPHA_TEST );
		pglAlphaFunc( GL_GREATER, 0.0f );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}
}

static void GAME_EXPORT R_OverrideTextureSourceSize( unsigned int texnum, uint srcWidth, uint srcHeight )
{
	gl_texture_t *tx = R_GetTexture( texnum );

	tx->srcWidth = srcWidth;
	tx->srcHeight = srcHeight;
}

static void* GAME_EXPORT R_GetProcAddress( const char *name )
{
#if XASH_GL4ES
	return gl4es_GetProcAddress( name );
#else // TODO: other wrappers
	return gEngfuncs.GL_GetProcAddress( name );
#endif
}

static const char *R_GetConfigName( void )
{
	return "opengl";
}

static const ref_interface_t gReffuncs =
{
	R_Init,
	R_Shutdown,
	R_GetConfigName,
	R_SetDisplayTransform,

	GL_SetupAttributes,
	GL_InitExtensions,
	GL_ClearExtensions,

	R_GammaChanged,
	R_BeginFrame,
	R_RenderScene,
	R_EndFrame,
	R_PushScene,
	R_PopScene,
	GL_BackendStartFrame,
	GL_BackendEndFrame,

	R_ClearScreen,
	R_AllowFog,
	GL_SetRenderMode,

	R_AddEntity,
	CL_AddCustomBeam,
	R_ProcessEntData,
	R_Flush,

	R_ShowTextures,

	R_GetTextureOriginalBuffer,
	GL_LoadTextureFromBuffer,
	GL_ProcessTexture,
	R_SetupSky,

	R_Set2DMode,
	R_DrawStretchRaw,
	R_DrawStretchPic,
	CL_FillRGBA,
	R_WorldToScreen,

	VID_ScreenShot,
	VID_CubemapShot,

	R_LightPoint,

	R_DecalShoot,
	R_DecalRemoveAll,
	R_CreateDecalList,
	R_ClearAllDecals,

	R_StudioEstimateFrame,
	R_StudioLerpMovement,
	CL_InitStudioAPI,

	R_SetSkyCloudsTextures,
	GL_SubdivideSurface,
	CL_RunLightStyles,

	R_GetSpriteParms,
	R_GetSpriteTexture,

	Mod_ProcessRenderData,
	Mod_StudioLoadTextures,

	CL_DrawParticles,
	CL_DrawTracers,
	CL_DrawBeams,
	R_BeamCull,

	GL_RefGetParm,
	R_GetDetailScaleForTexture,
	R_GetExtraParmsForTexture,
	R_GetFrameTime,

	R_SetCurrentEntity,
	R_SetCurrentModel,

	GL_FindTexture,
	GL_TextureName,
	GL_TextureData,
	GL_LoadTexture,
	GL_CreateTexture,
	GL_LoadTextureArray,
	GL_CreateTextureArray,
	GL_FreeTexture,
	R_OverrideTextureSourceSize,

	DrawSingleDecal,
	R_DecalSetupVerts,
	R_EntityRemoveDecals,

	R_UploadStretchRaw,

	GL_Bind,
	GL_SelectTexture,
	GL_LoadTexMatrixExt,
	GL_LoadIdentityTexMatrix,
	GL_CleanUpTextureUnits,
	GL_TexGen,
	GL_TextureTarget,
	GL_SetTexCoordArrayMode,
	GL_UpdateTexSize,
	NULL,
	NULL,

	CL_DrawParticlesExternal,
	R_LightVec,
	R_StudioGetTexture,

	R_RenderFrame,
	Mod_SetOrthoBounds,
	R_SpeedsMessage,
	Mod_GetCurrentVis,
	R_NewMap,
	R_ClearScene,
	R_GetProcAddress,

	TriRenderMode,
	TriBegin,
	TriEnd,
	_TriColor4f,
	_TriColor4ub,
	TriTexCoord2f,
	TriVertex3fv,
	TriVertex3f,
	TriFog,
	R_ScreenToWorld,
	TriGetMatrix,
	TriFogParams,
	TriCullFace,

	VGUI_SetupDrawing,
	VGUI_UploadTextureBlock,
};

int EXPORT GetRefAPI( int version, ref_interface_t *funcs, ref_api_t *engfuncs, ref_globals_t *globals );
int EXPORT GetRefAPI( int version, ref_interface_t *funcs, ref_api_t *engfuncs, ref_globals_t *globals )
{
	if( version != REF_API_VERSION )
		return 0;

	// fill in our callbacks
	*funcs = gReffuncs;
	gEngfuncs = *engfuncs;
	gpGlobals = globals;

	gp_cl = (ref_client_t *)ENGINE_GET_PARM( PARM_GET_CLIENT_PTR );
	gp_host = (ref_host_t *)ENGINE_GET_PARM( PARM_GET_HOST_PTR );

	return REF_API_VERSION;
}
