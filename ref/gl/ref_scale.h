// ref_scale.h - Scale render target functions for both GL and Software renderers
#ifndef REF_SCALE_H
#define REF_SCALE_H

#include "ref_api.h"

// Scale render target functions
qboolean R_CreateScaleRenderTarget( int width, int height );
void R_DestroyScaleRenderTarget( void );
void R_BindRenderTargetForScene( void );
void R_BlitScaleRenderTargetToScreen( int screen_w, int screen_h );
qboolean R_SetDisplayTransform( ref_screen_rotation_t rotate, int offset_x, int offset_y, float scale_x, float scale_y );

#endif // REF_SCALE_H
