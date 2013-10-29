/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2007 Peter Mackay and Chris Swindle.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_main.c

extern "C"
{
#include "../quakedef.h"
float TraceLine (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal);
}

#include "video_hardware_hlmdl.h"

#include <pspgu.h>
#include <pspgum.h>

#include "clipping.hpp"

using namespace quake;

entity_t	r_worldentity;

qboolean	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys, c_md3_polys;

qboolean	envmap;				// qtrue during envmap command capture 

int			currenttexture = -1;		// to avoid unnecessary texture sets

int			cnttextures[2] = {-1, -1};     // cached

int			particletexture;	// little dot for particles
int			playertextures;		// up to 16 color translated skins

int			mirrortexturenum;	// quake texturenum, not gltexturenum
qboolean	mirror;
mplane_t	*mirror_plane;

void R_RenderDecals (void);
bool    fixlight;
bool    alphafunc2;
bool    alphafunc;
//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

ScePspFMatrix4	r_world_matrix;
ScePspFMatrix4	r_base_world_matrix;
ScePspFMatrix4  md3mult;
//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves (void);
cvar_t	r_partalpha           = {"r_partalpha",             "0.8",qtrue};
cvar_t	r_norefresh           = {"r_norefresh",               "0"};
cvar_t	r_drawentities        = {"r_drawentities",            "1"};
cvar_t	r_drawviewmodel       = {"r_drawviewmodel",           "1"};
cvar_t	r_speeds              = {"r_speeds",                  "0"};
cvar_t	r_fullbright          = {"r_fullbright",              "0"};
cvar_t	r_lightmap            = {"r_lightmap",                "0"};
cvar_t	r_shadows             = {"r_shadows",                 "0"};
cvar_t	r_mirroralpha         = {"r_mirroralpha",             "1",qtrue};
cvar_t	r_wateralpha          = {"r_wateralpha",            "0.6",qtrue};
cvar_t	r_vsync               = {"r_vsync",                   "0",qtrue};
cvar_t	r_farclip	          = {"r_farclip",              "4096"};        //far cliping for q3 models
cvar_t  r_loadq3models        = {"r_loadq3models",            "0",qtrue}; //replace player model to q3 player
cvar_t  cl_loadmapcfg         = {"cl_loadmapcfg",             "0",qtrue}; //Load individual cfg for map
cvar_t	r_restexf             = {"r_restexf",                 "0",qtrue}; //texture resampler setup
cvar_t  r_texcompr            = {"r_texcompr",                "5",qtrue}; //texture compression setup (default DXT5) warning DXT1 conflicted with switches palettes
cvar_t  r_maxrange            = {"r_maxrange",             "4096"}; //render distance
cvar_t	r_skydis              = {"r_skydis",               "2560",qtrue};
cvar_t	r_skyvflip            = {"r_skyvflip",                "1",qtrue};
cvar_t  r_skyfog              = {"r_skyfog",                  "1",qtrue};
cvar_t	r_caustics            = {"r_caustics",                "1",qtrue};
cvar_t	r_detail              = {"r_detail",                  "1",qtrue};
cvar_t	r_detail_mipmaps      = {"r_detail_mipmaps",          "1",qtrue};
cvar_t	r_detail_mipmaps_func = {"r_detail_mipmaps_func",     "2",qtrue};
cvar_t	r_detail_mipmaps_bias = {"r_detail_mipmaps_bias",    "-6",qtrue};
cvar_t  r_asynch              = {"r_asynch",                  "0"};
cvar_t  r_ipolations          = {"r_ipolations",              "0"};
cvar_t  r_i_model_animation   = {"r_i_model_animation",       "1",qtrue}; // Toggle smooth model animation
cvar_t  r_i_model_transform   = {"r_i_model_transform",       "1",qtrue}; // Toggle smooth model movement
cvar_t	r_mipmaps             = {"r_mipmaps",                 "1",qtrue};
cvar_t  r_mipmaps_func        = {"r_mipmaps_func",            "2",qtrue};          // Adjust mip map calculations
cvar_t  r_mipmaps_bias        = {"r_mipmaps_bias",           "-7",qtrue};         // Adjust mip map bias level
cvar_t	r_dynamic             = {"r_dynamic",                 "1"};
cvar_t	r_novis               = {"r_novis",                   "0"};
cvar_t	r_tex_scale_down      = {"r_tex_scale_down",          "1",qtrue};
cvar_t	r_particles_simple    = {"r_particles_simple",        "0",qtrue};
cvar_t	gl_keeptjunctions     = {"gl_keeptjunctions",         "0"};
cvar_t	r_waterripple         = {"r_waterripple",             "2",qtrue};
cvar_t	r_waterwarp           = {"r_waterwarp",               "1",qtrue};
cvar_t	r_fastsky             = {"r_fastsky",                 "0",qtrue};
cvar_t  r_skycolor            = {"r_skycolor",         "64 64 70",qtrue};
cvar_t  r_showbboxes          = {"r_showbboxes",              "0"};
cvar_t  r_showbboxes_full     = {"r_showbboxes_full",         "0",qtrue};
cvar_t	r_showtris            = {"r_showtris",                "0"};
cvar_t	r_showtris_full       = {"r_showtris_full",           "0",qtrue};
cvar_t	r_polyblend	          = {"r_polyblend",               "1",qtrue};

//QMB
cvar_t  r_explosiontype     = {"r_explosiontype",    "0",qtrue};
cvar_t	r_laserpoint		= {"r_laserpoint",       "0",qtrue};
cvar_t	r_part_explosions	= {"r_part_explosions",  "1",qtrue};
cvar_t	r_part_trails		= {"r_part_trails",      "1",qtrue};
cvar_t	r_part_sparks		= {"r_part_sparks",      "1",qtrue};
cvar_t	r_part_spikes		= {"r_part_spikes",      "1",qtrue};
cvar_t	r_part_gunshots	    = {"r_part_gunshots",    "1",qtrue};
cvar_t	r_part_blood		= {"r_part_blood",       "1",qtrue};
cvar_t	r_part_telesplash	= {"r_part_telesplash",  "1",qtrue};
cvar_t	r_part_blobs		= {"r_part_blobs",       "1",qtrue};
cvar_t	r_part_lavasplash	= {"r_part_lavasplash",  "1",qtrue};
cvar_t	r_part_flames		= {"r_part_flames",      "1",qtrue};
cvar_t	r_part_lightning	= {"r_part_lightning",   "1",qtrue};
cvar_t	r_part_flies		= {"r_part_flies",       "1",qtrue};
cvar_t	r_part_muzzleflash  = {"r_part_muzzleflash", "1",qtrue};
cvar_t	r_flametype	        = {"r_flametype",        "2",qtrue};


void 	VID_SetPaletteH2();
void 	VID_SetPaletteTX();

void Fog_EnableGFog  (void);
void Fog_DisableGFog (void);

/*
================
ConvertMatrix 
By Crow_bar for MD3
================
*/
void ConvertMatrix(float *a, float *b)
{
	for (int i = 0; i < 16; i++)
	     a[i] = b[i];
}

/*
=============
R_RotateForTagEntity
=============
*/
void R_RotateForTagEntity (tagentity_t *tagent, md3tag_t *tag, float *m)
{
	int	i;
	float	lerpfrac, timepassed;

	// positional interpolation
	timepassed = cl.time - tagent->tag_translate_start_time;

	if (tagent->tag_translate_start_time == 0 || timepassed > 1)
	{
		tagent->tag_translate_start_time = cl.time;
		VectorCopy (tag->pos, tagent->tag_pos1);
		VectorCopy (tag->pos, tagent->tag_pos2);
	}

	if (!VectorCompare(tag->pos, tagent->tag_pos2))
	{
		tagent->tag_translate_start_time = cl.time;
		VectorCopy (tagent->tag_pos2, tagent->tag_pos1);
		VectorCopy (tag->pos, tagent->tag_pos2);
		lerpfrac = 0;
	}
	else
	{
		lerpfrac = timepassed / 0.1;
		if (cl.paused || lerpfrac > 1)
			lerpfrac = 1;
	}

	VectorInterpolate (tagent->tag_pos1, lerpfrac, tagent->tag_pos2, m + 12);
	m[15] = 1;

	for (i=0 ; i<3 ; i++)
	{
		// orientation interpolation (Euler angles, yuck!)
		timepassed = cl.time - tagent->tag_rotate_start_time[i];

		if (tagent->tag_rotate_start_time[i] == 0 || timepassed > 1)
		{
			tagent->tag_rotate_start_time[i] = cl.time;
			VectorCopy (tag->rot[i], tagent->tag_rot1[i]);
			VectorCopy (tag->rot[i], tagent->tag_rot2[i]);
		}

		if (!VectorCompare(tag->rot[i], tagent->tag_rot2[i]))
		{
			tagent->tag_rotate_start_time[i] = cl.time;
			VectorCopy (tagent->tag_rot2[i], tagent->tag_rot1[i]);
			VectorCopy (tag->rot[i], tagent->tag_rot2[i]);
			lerpfrac = 0;
		}
		else
		{
			lerpfrac = timepassed / 0.1;
			if (cl.paused || lerpfrac > 1)
				lerpfrac = 1;
		}

		VectorInterpolate (tagent->tag_rot1[i], lerpfrac, tagent->tag_rot2[i], m + i*4);
		m[i*4+3] = 0;
	}
}

/*
=============
R_RotateForViewEntity
=============
*/
void R_RotateForViewEntity (entity_t *ent)
{
	// Translate.
	const ScePspFVector3 translation =
	{
		ent->origin[0], ent->origin[1], ent->origin[2]
	};
	sceGumTranslate(&translation);

	// Rotate.
	const ScePspFVector3 rotation =
	{
		ent->angles[ROLL] * (GU_PI / 180.0f),
		-ent->angles[PITCH] * (GU_PI / 180.0f),
		ent->angles[YAW] * (GU_PI / 180.0f)
	};
	sceGumRotateZYX(&rotation);
}

/*
=================
R_CullBox

Returns qtrue if the box is completely outside the frustom
=================

qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i=0 ; i<4 ; i++)
		if (BoxOnPlaneSide (mins, maxs, &frustum[i]) == 2)
			return qtrue;
	return qfalse;
}
*/

/*
=================
R_CullBox -- replaced with new function from lordhavoc

Returns true if the box is completely outside the frustum
=================
*/
qboolean R_CullBox (vec3_t emins, vec3_t emaxs)
{
	int i;
	mplane_t *p;
	for (i = 0;i < 4;i++)
	{
		p = frustum + i;
		switch(p->signbits)
		{
		default:
		case 0:
			if (p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2] < p->dist)
				return qtrue;
			break;
		case 1:
			if (p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2] < p->dist)
				return qtrue;
			break;
		case 2:
			if (p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2] < p->dist)
				return qtrue;
			break;
		case 3:
			if (p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2] < p->dist)
				return qtrue;
			break;
		case 4:
			if (p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2] < p->dist)
				return qtrue;
			break;
		case 5:
			if (p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2] < p->dist)
				return qtrue;
			break;
		case 6:
			if (p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2] < p->dist)
				return qtrue;
			break;
		case 7:
			if (p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2] < p->dist)
				return qtrue;
			break;
		}
	}
	return qfalse;
}

/*
=================
R_CullSphere

Returns true if the sphere is completely outside the frustum
=================
*/
qboolean R_CullSphere (vec3_t centre, float radius)
{
	int		i;
	mplane_t	*p;

	for (i=0, p=frustum ; i<4 ; i++, p++)
	{
		if (PlaneDiff(centre, p) <= -radius)
			return qtrue;
	}

	return qfalse;
}

/*
=============
R_RotateForEntity
=============
*/
void R_RotateForEntity (entity_t *e, int shadow)
{
	// Translate.
	const ScePspFVector3 translation =
	{
		e->origin[0], e->origin[1], e->origin[2]
	};
	sceGumTranslate(&translation);
/*
	// Scale.
    const ScePspFVector3 scale =
	{
	e->scale, e->scale, e->scale
	};
	sceGumScale(&scale);
*/

	// Rotate.
    sceGumRotateZ(e->angles[YAW] * (GU_PI / 180.0f));
	if (shadow == 0)
	{
		sceGumRotateY (-e->angles[PITCH] * (GU_PI / 180.0f));
		sceGumRotateX (e->angles[ROLL] * (GU_PI / 180.0f));
	}

	sceGumUpdateMatrix();
}

/*
=============
R_BlendedRotateForEntity

fenix@io.com: model transform interpolation
=============
*/
void R_BlendedRotateForEntity (entity_t *e, int shadow)	// Tomaz - New Shadow
{
    float timepassed;
    float blend;
    vec3_t d;
    int i;

    // positional interpolation

    timepassed = realtime - e->translate_start_time;

    if (e->translate_start_time == 0 || timepassed > 1)
    {
        e->translate_start_time = realtime;
        VectorCopy (e->origin, e->origin1);
        VectorCopy (e->origin, e->origin2);
    }

    if (!VectorCompare (e->origin, e->origin2))
    {
        e->translate_start_time = realtime;
        VectorCopy (e->origin2, e->origin1);
        VectorCopy (e->origin,  e->origin2);
        blend = 0;
    }
    else
    {
        blend =  timepassed / 0.1;
        if (cl.paused || blend > 1)
            blend = 0;
    }

    VectorSubtract (e->origin2, e->origin1, d);

    // Translate.
    const ScePspFVector3 translation = {
    e->origin[0] + (blend * d[0]),
    e->origin[1] + (blend * d[1]),
    e->origin[2] + (blend * d[2])
    };
    sceGumTranslate(&translation);
/*
    // Scale.
    const ScePspFVector3 scale = {
    e->scale + (blend * d[0]),
    e->scale + (blend * d[1]),
    e->scale + (blend * d[2]
    };
    sceGumScale(&scale);
*/
    // orientation interpolation (Euler angles, yuck!)
    timepassed = realtime - e->rotate_start_time;

    if (e->rotate_start_time == 0 || timepassed > 1)
    {
        e->rotate_start_time = realtime;
        VectorCopy (e->angles, e->angles1);
        VectorCopy (e->angles, e->angles2);
    }

    if (!VectorCompare (e->angles, e->angles2))
    {
        e->rotate_start_time = realtime;
        VectorCopy (e->angles2, e->angles1);
        VectorCopy (e->angles,  e->angles2);
        blend = 0;
    }
    else
    {
        blend = timepassed / 0.1;
        if (cl.paused || blend > 1)
            blend = 1;
    }

    VectorSubtract (e->angles2, e->angles1, d);

    // always interpolate along the shortest path
    for (i = 0; i < 3; i++)
    {
        if (d[i] > 180)
        {
            d[i] -= 360;
        }
        else if (d[i] < -180)
        {
            d[i] += 360;
        }
    }

	// Rotate.
    sceGumRotateZ((e->angles1[YAW] + ( blend * d[YAW])) * (GU_PI / 180.0f));
	if (shadow == 0)
	{
		sceGumRotateY ((-e->angles1[PITCH] + (-blend * d[PITCH])) * (GU_PI / 180.0f));
		sceGumRotateX ((e->angles1[ROLL] + ( blend * d[ROLL])) * (GU_PI / 180.0f));
	}

	sceGumUpdateMatrix();
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

extern vec3_t lightcolor; // LordHavoc: .lit support

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *currententity)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = static_cast<msprite_t*>(currententity->model->cache.data);
	frame = currententity->frame;

	
	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}

/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	vec3_t			point, v_forward, v_right, v_up;
	msprite_t		*psprite;
	mspriteframe_t	*frame;
	float			*s_up, *s_right;
	float			angle, sr, cr;
	bool additive = false;
	bool filter = false;

	// don't even bother culling, because it's just a single polygon without a surface cache
	frame = R_GetSpriteFrame (e);
	psprite = static_cast<msprite_t*>(currententity->model->cache.data);

	switch(psprite->type)
	{
	case SPR_VP_PARALLEL_UPRIGHT: //faces view plane, up is towards the heavens
		v_up[0] = 0;
		v_up[1] = 0;
		v_up[2] = 1;
		s_up = v_up;
		s_right = vright;
		break;
	case SPR_FACING_UPRIGHT: //faces camera origin, up is towards the heavens
		VectorSubtract(currententity->origin, r_origin, v_forward);
		v_forward[2] = 0;
		VectorNormalizeFast(v_forward);
		v_right[0] = v_forward[1];
		v_right[1] = -v_forward[0];
		v_right[2] = 0;
		v_up[0] = 0;
		v_up[1] = 0;
		v_up[2] = 1;
		s_up = v_up;
		s_right = v_right;
		break;
	case SPR_VP_PARALLEL: //faces view plane, up is towards the top of the screen
		s_up = vup;
		s_right = vright;
		break;
	case SPR_ORIENTED: //pitch yaw roll are independent of camera
		AngleVectors (currententity->angles, v_forward, v_right, v_up);
		s_up = v_up;
		s_right = v_right;
		break;
	case SPR_VP_PARALLEL_ORIENTED: //faces view plane, but obeys roll value
		angle = currententity->angles[ROLL] * M_PI_DIV_180;
		sr = sin(angle);
		cr = cos(angle);
		v_right[0] = vright[0] * cr + vup[0] * sr;
		v_up[0] = vright[0] * -sr + vup[0] * cr;
		v_right[1] = vright[1] * cr + vup[1] * sr;
		v_up[1] = vright[1] * -sr + vup[1] * cr;
		v_right[2] = vright[2] * cr + vup[2] * sr;
		v_up[2] = vright[2] * -sr + vup[2] * cr;
		s_up = v_up;
		s_right = v_right;
		break;
	default:
		return;
	}

	if (psprite->beamlength == 10) // we use the beam length of sprites, since they are unused by quake anyway.
		additive = true;

	if (psprite->beamlength == 20)
		filter = true;

	// Bind the texture.
	GL_Bind(frame->gl_texturenum);

	sceGuEnable(GU_BLEND);
	sceGuDepthMask(GU_TRUE);

	Fog_DisableGFog ();
	
    if (additive)
	{
		sceGuDepthMask(GU_TRUE);
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);
		sceGuTexFunc(GU_TFX_MODULATE , GU_TCC_RGB);
	}
    else if (filter)
	{
		sceGuDepthMask(GU_TRUE);
		sceGuBlendFunc(GU_ADD, GU_FIX, GU_SRC_COLOR, 0, 0);
        sceGuTexFunc(GU_TFX_MODULATE , GU_TCC_RGB);
	}
	else
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	// Allocate memory for this polygon.
	glvert_t* const	vertices =
		static_cast<glvert_t*>(sceGuGetMemory(sizeof(glvert_t) * 4));

	VectorMA (e->origin, frame->down, s_up, point);
	VectorMA (point, frame->left, s_right, point);

	vertices[0].st[0]	= 0.0f;
	vertices[0].st[1]	= 1.0f;
	vertices[0].xyz[0]	= point[0];
	vertices[0].xyz[1]	= point[1];
	vertices[0].xyz[2]	= point[2];

	VectorMA (e->origin, frame->up, s_up, point);
	VectorMA (point, frame->left, s_right, point);

	vertices[1].st[0]	= 0.0f;
	vertices[1].st[1]	= 0.0f;
	vertices[1].xyz[0]	= point[0];
	vertices[1].xyz[1]	= point[1];
	vertices[1].xyz[2]	= point[2];

	VectorMA (e->origin, frame->up, s_up, point);
	VectorMA (point, frame->right, s_right, point);

	vertices[2].st[0]	= 1.0f;
	vertices[2].st[1]	= 0.0f;
	vertices[2].xyz[0]	= point[0];
	vertices[2].xyz[1]	= point[1];
	vertices[2].xyz[2]	= point[2];

	VectorMA (e->origin, frame->down, s_up, point);
	VectorMA (point, frame->right, s_right, point);

	vertices[3].st[0]	= 1.0f;
	vertices[3].st[1]	= 1.0f;
	vertices[3].xyz[0]	= point[0];
	vertices[3].xyz[1]	= point[1];
	vertices[3].xyz[2]	= point[2];

	// Draw the clipped vertices.
	sceGuDrawArray(
		GU_TRIANGLE_FAN,
		GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
		4, 0, vertices);

	sceGuDepthMask(GU_FALSE);
	sceGuDisable(GU_BLEND);

	Fog_EnableGFog ();

	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162

#define INTERP_WEAP_MAXNUM		24
#define INTERP_WEAP_MINDIST		5000
#define INTERP_WEAP_MAXDIST		95000
#define	INTERP_MINDIST			70
#define	INTERP_MAXDIST			300

extern "C" float	r_avertexnormals[NUMVERTEXNORMALS][3];
float r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "../anorms.h"
};

vec3_t	shadevector;
float	shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "../anorm_dots.h"
;

float	*shadedots = r_avertexnormal_dots[0];

// fenix@io.com: model animation interpolation
int lastposenum0;
//

int	lastposenum;

// fenix@io.com: model transform interpolation
float old_i_model_transform;
//

/*
=============
GL_DrawAliasFrame
=============
*/
void GL_DrawAliasFrame (aliashdr_t *paliashdr, int posenum, float apitch, float ayaw)
{
	float 	l;
	trivertx_t	*verts;
	int		*order;
	int		count;

    lastposenum = posenum;

	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		int prim;
		if (count < 0)
		{
			count = -count;
			prim = GU_TRIANGLE_FAN;
		}
		else
		{
			prim = GU_TRIANGLE_STRIP;
		}

		// Allocate the vertices.
		struct vertex
		{
			float u, v;
			unsigned int color; 
			float x, y, z;
		};

		vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * count));

		for (int vertex_index = 0; vertex_index < count; ++vertex_index)
		{
            // texture coordinates come from the draw list
			out[vertex_index].u = ((float *)order)[0];
			out[vertex_index].v = ((float *)order)[1];
			order += 2;

            l = shadedots[verts->lightnormalindex];
            float       r,g,b;
			
			r = l * lightcolor[0];
            g = l * lightcolor[1];
            b = l * lightcolor[2];

			 if(r > 1)
				r = 1;
			 if(g > 1)
				g = 1;
			 if(b > 1)
				b = 1;

			out[vertex_index].x = verts->v[0];
			out[vertex_index].y = verts->v[1];
			out[vertex_index].z = verts->v[2];
            out[vertex_index].color = GU_COLOR(r, g, b, 1.0f);
            
           	++verts;
		}
		if(r_showtris.value)
		{
		   sceGuDisable(GU_TEXTURE_2D);
		}
		sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : prim, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_COLOR_8888, count, 0, out);
        if(r_showtris.value)
		{
		   sceGuEnable(GU_TEXTURE_2D);
		}
	}
	sceGuColor(0xffffffff);
}

/*
=============
GL_DrawAliasBlendedFrame

fenix@io.com: model animation interpolation
=============
*/
void GL_DrawAliasBlendedFrame (aliashdr_t *paliashdr, int pose1, int pose2, float blend, float apitch, float ayaw)
{
	float       l;
    trivertx_t* verts1;
	trivertx_t* verts2;
	int*        order;
	int         count, brightness;
	vec3_t      d;
	vec3_t       point;

	lastposenum0 = pose1;
	lastposenum  = pose2;

	verts1 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts2 = verts1;

	verts1 += pose1 * paliashdr->poseverts;
	verts2 += pose2 * paliashdr->poseverts;

	order = (int *)((byte *)paliashdr + paliashdr->commands);
	
	while (1)
	{
       // get the vertex count and primitive type
		int prim;
		count = *order++;

		if (!count) break;

		if (count < 0)
        {
			count = -count;
            prim = GU_TRIANGLE_FAN;
		}
		else
		{
		    prim = GU_TRIANGLE_STRIP;;
		}

		// Allocate the vertices.
		struct vertex
		{
			float u, v;
			unsigned int color;
			float x, y, z;
		};

		vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * count));

		for (int vertex_index = 0; vertex_index < count; ++vertex_index)
		{
            
			// texture coordinates come from the draw list
			out[vertex_index].u = ((float *)order)[0];
			out[vertex_index].v = ((float *)order)[1];

			order += 2;
			d[0] = shadedots[verts2->lightnormalindex] - shadedots[verts1->lightnormalindex];
            l = shadedots[verts1->lightnormalindex] + (blend * d[0]);

            float       r,g,b;

			r = l * lightcolor[0];
            g = l * lightcolor[1];
            b = l * lightcolor[2];

             if(r > 1)
				r = 1;
			 if(g > 1)
				g = 1;
			 if(b > 1)
				b = 1;

			VectorSubtract(verts2->v, verts1->v, d);

			// blend the vertex positions from each frame together
			point[0] = verts1->v[0] + (blend * d[0]);
			point[1] = verts1->v[1] + (blend * d[1]);
			point[2] = verts1->v[2] + (blend * d[2]);
			
		    out[vertex_index].x = point[0];
            out[vertex_index].y = point[1];
            out[vertex_index].z = point[2];
            out[vertex_index].color = GU_COLOR(r, g, b, 1.0f);
            
            verts1++;
            verts2++;
		}
        if(r_showtris.value)
		{
		   sceGuDisable(GU_TEXTURE_2D);
		}
		sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : prim, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_COLOR_8888, count, 0, out);
        if(r_showtris.value)
		{
		   sceGuEnable(GU_TEXTURE_2D);
		}
	}
	sceGuColor(0xffffffff);
}

/*
=============
GL_DrawAliasInterpolatedFrame
=============
*/
void GL_DrawAliasInterpolatedFrame (aliashdr_t *paliashdr, int posenum, int oldposenum, int interp)
{
   float s, t;
   float  l;
   float interpolations;
   int  i, j;
   int  index;
   trivertx_t *v, *verts, *oldverts;
   int  list;
   int  *order;
   vec3_t point;
   float *normal;
   int  count;

   lastposenum = posenum;
   interpolations = interp/r_ipolations.value;
   verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
   oldverts = verts;
   verts += posenum * paliashdr->poseverts;
   if (oldposenum >= 0)
      oldverts += oldposenum * paliashdr->poseverts;
   else
      oldverts += posenum * paliashdr->poseverts;

   order = (int *)((byte *)paliashdr + paliashdr->commands);

   while (1)
      {
        // get the vertex count and primitive type
        int prim;
		count = *order++;

		if (!count)
		break;

		if (count < 0)
        {
			count = -count;
            prim = GU_TRIANGLE_FAN;
		}
		else
		{
		    prim = GU_TRIANGLE_STRIP;;
		}

		// Allocate the vertices.
		struct vertex
		{
			float u, v;
			unsigned int color;
			float x, y, z;
		};

		vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * count));

		for (int vertex_index = 0; vertex_index < count; ++vertex_index)
        {
         // texture coordinates come from the draw list
     	 out[vertex_index].u = ((float *)order)[0];
		 out[vertex_index].v = ((float *)order)[1];
         order += 2;
         // normals and vertexes come from the frame list
         l = shadedots[verts->lightnormalindex];
         float       r,g,b;

			r = l * lightcolor[0];
            g = l * lightcolor[1];
            b = l * lightcolor[2];
             if(r > 1)
				r = 1;
			 if(g > 1)
				g = 1;
			 if(b > 1)
				b = 1;

		 out[vertex_index].x = oldverts->v[0] + ((verts->v[0] - oldverts->v[0])*interpolations);
         out[vertex_index].y = oldverts->v[1] + ((verts->v[1] - oldverts->v[1])*interpolations);
         out[vertex_index].z = oldverts->v[2] + ((verts->v[2] - oldverts->v[2])*interpolations);

         out[vertex_index].color = GU_COLOR(r, g, b, 1.0f);

		 verts++;
         oldverts++;
		 }
         if(r_showtris.value)
		 {
		   sceGuDisable(GU_TEXTURE_2D);
		 }
		 sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : prim, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_COLOR_8888, count, 0, out);
         if(r_showtris.value)
		 {
		   sceGuEnable(GU_TEXTURE_2D);
		 }
	  }
	  sceGuColor(0xffffffff);
}


/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;
void GL_DrawAliasShadow (aliashdr_t *paliashdr, int posenum)
{
	float	s, t, l;
	int		i, j;
	int		index;
	trivertx_t	*v, *verts;
	int		list;
	int		*order;
	vec3_t	point;
	float	*normal;
	float	height, lheight;
	int		count;

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;
	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		int prim;
		count = *order++;

		if (!count)
			break;		// done

		if (count < 0)
		{
			count = -count;
			prim =  GU_TRIANGLE_FAN;
		}
		else
			prim =  GU_TRIANGLE_STRIP;

		// Allocate the vertices.
		struct vertex
		{
			float x, y, z;
		};

		vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * count));

		for (int vertex_index = 0; vertex_index < count; ++vertex_index)
		{
			// texture coordinates come from the draw list
			// (skipped for shadows) glTexCoord2fv ((float *)order);
			order += 2;

			// normals and vertexes come from the frame list
			point[0] = verts->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point[1] = verts->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point[2] = verts->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;

			out[vertex_index].x = point[0];
            out[vertex_index].y = point[1];
            out[vertex_index].z = point[2];
			verts++;
		}
        if(r_showtris.value)
		{
		   sceGuDisable(GU_TEXTURE_2D);
		}

		sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : prim, GU_VERTEX_32BITF, count, 0, out);

		if(r_showtris.value)
		{
		   sceGuEnable(GU_TEXTURE_2D);
		}
	}
}

/*
=============
GL_DrawAliasBlendedShadow

fenix@io.com: model animation interpolation
=============
*/
void GL_DrawAliasBlendedShadow (aliashdr_t *paliashdr, int pose1, int pose2, entity_t* e)
{
    trivertx_t* verts1;
    trivertx_t* verts2;
    int*        order;
    vec3_t      point1;
    vec3_t      point2;
    vec3_t      d;
    float       height;
    float       lheight;
    int         count;
    float       blend;

	// Tomaz - New Shadow Begin
	trace_t		downtrace;
	vec3_t		downmove;
	float		s1,c1;
	// Tomaz - New Shadow End

    blend = (realtime - e->frame_start_time) / e->frame_interval;

    if (blend > 1) blend = 1;

    lheight = e->origin[2] - lightspot[2];
    height  = -lheight; // Tomaz - New Shadow

	// Tomaz - New Shadow Begin
	VectorCopy (e->origin, downmove);
	downmove[2] = downmove[2] - 4096;
	memset (&downtrace, 0, sizeof(downtrace));
	SV_RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, e->origin, downmove, &downtrace);

	s1 = sin( e->angles[1]/180*M_PI);
	c1 = cos( e->angles[1]/180*M_PI);
	// Tomaz - New Shadow End

    verts1 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
    verts2 = verts1;

    verts1 += pose1 * paliashdr->poseverts;
    verts2 += pose2 * paliashdr->poseverts;

    order = (int *)((byte *)paliashdr + paliashdr->commands);

    for (;;)
    {
        // get the vertex count and primitive type
        count = *order++;

        if (!count)
		break;

        int prim;
		if (count < 0)
		{
		 count = -count;
		 prim = GU_TRIANGLE_FAN;
		}
		else
		{
		 prim = GU_TRIANGLE_STRIP;
		}

		// Allocate the vertices.
		struct vertex
		{
			float x, y, z;
		};

		vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * count));

		for (int vertex_index = 0; vertex_index < count; ++vertex_index)
		{
                order += 2;

                point1[0] = verts1->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
                point1[1] = verts1->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
                point1[2] = verts1->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

                point1[0] -= shadevector[0]*(point1[2]+lheight);
                point1[1] -= shadevector[1]*(point1[2]+lheight);

                point2[0] = verts2->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
                point2[1] = verts2->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
                point2[2] = verts2->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

                point2[0] -= shadevector[0]*(point2[2]+lheight);
                point2[1] -= shadevector[1]*(point2[2]+lheight);

                VectorSubtract(point2, point1, d);

				// Tomaz - New shadow Begin
				point1[0] = point1[0] + (blend * d[0]);
				point1[1] = point1[1] + (blend * d[1]);
				point1[2] = point1[2] + (blend * d[2]);

				point1[2] =  - (e->origin[2] - downtrace.endpos[2]);

				point1[2] += ((point1[1] * (s1 * downtrace.plane.normal[0])) -
							  (point1[0] * (c1 * downtrace.plane.normal[0])) -
							  (point1[0] * (s1 * downtrace.plane.normal[1])) -
							  (point1[1] * (c1 * downtrace.plane.normal[1]))) +
							  ((1.0 - downtrace.plane.normal[2])*20) + 0.2 ;

				out[vertex_index].x = point1[0];
                out[vertex_index].y = point1[1] ;
                out[vertex_index].z = point1[2];
				// Tomaz - New shadow Begin

                verts1++;
                verts2++;
        }
        if(r_showtris.value)
		{
		   sceGuDisable(GU_TEXTURE_2D);
		}
		sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : prim,GU_VERTEX_32BITF, count, 0, out);
        if(r_showtris.value)
		{
		   sceGuEnable(GU_TEXTURE_2D);
		}
    }
}



/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame (int frame, aliashdr_t *paliashdr, float apitch, float ayaw)
{
	int				pose, numposes;
	float			interval;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / interval) % numposes;
	}

	GL_DrawAliasFrame (paliashdr, pose, apitch, ayaw);
}

/*
=================
R_SetupAliasBlendedFrame

fenix@io.com: model animation interpolation
=================
*/
void R_SetupAliasBlendedFrame (int frame, aliashdr_t *paliashdr, entity_t* e, float apitch, float ayaw)
{
	int   pose;
	int   numposes;
	float blend;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
  		e->frame_interval = paliashdr->frames[frame].interval;
  		pose += (int)(cl.time / e->frame_interval) % numposes;
  	}
    else
    {
		/* One tenth of a second is a good for most Quake animations.
		If the nextthink is longer then the animation is usually meant to pause
		(e.g. check out the shambler magic animation in shambler.qc).  If its
		shorter then things will still be smoothed partly, and the jumps will be
		less noticable because of the shorter time.  So, this is probably a good
		assumption. */
		e->frame_interval = 0.1;
	}

	if (e->pose2 != pose)
	{
		e->frame_start_time = realtime;
		e->pose1 = e->pose2;
		e->pose2 = pose;
		blend = 0;
	}
	else
	{
		blend = (realtime - e->frame_start_time) / e->frame_interval;
	}

	// wierd things start happening if blend passes 1
	if (cl.paused || blend > 1) blend = 1;

	if (blend == 1)
        GL_DrawAliasFrame (paliashdr, pose, apitch, ayaw);
    else
        GL_DrawAliasBlendedFrame (paliashdr, e->pose1, e->pose2, blend, apitch, ayaw);
}

/*
=================
R_SetupAliasInterpolatedFrame
=================
*/
void R_SetupAliasInterpolatedFrame (int frame, int lastframe, float interp, aliashdr_t *paliashdr)
{
   int   pose, numposes, oldpose;
   float   interval;

	if ((frame >= paliashdr->numframes) || (frame < 0))
    {
      Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
      frame = 0;
    }

	if ((lastframe >= paliashdr->numframes) || (lastframe < 0))
    {
      Con_DPrintf ("R_AliasSetupFrame: no such last frame %d\n", lastframe);
      lastframe = 0;
    }

	pose = paliashdr->frames[frame].firstpose;
    numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
    {
      interval = paliashdr->frames[frame].interval;
      pose += (int)(cl.time / interval) % numposes;
    }

    oldpose = paliashdr->frames[lastframe].firstpose;
    numposes = paliashdr->frames[lastframe].numposes;

	if (numposes > 1)
    {
      interval = paliashdr->frames[lastframe].interval;
      oldpose += (int)(cl.time / interval) % numposes;
    }

   GL_DrawAliasInterpolatedFrame (paliashdr, pose, oldpose, interp);
}

/*
=============
GL_DrawQ2AliasFrame
=============
*/
void GL_DrawQ2AliasFrame (entity_t *e, md2_t *pheader, int lastpose, int pose, float lerp)
{
	float	ilerp, l;
	int		*order, count;
	md2trivertx_t	*verts1, *verts2;
	vec3_t	scale1, translate1, scale2, translate2;
	md2frame_t *frame1, *frame2;

	sceGuShadeModel(GU_SMOOTH);

	ilerp = 1.0f - lerp;

	//new version by muff - fixes bug, easier to read, faster (well slightly)
	frame1 = (md2frame_t *)((int) pheader + pheader->ofs_frames + (pheader->framesize * lastpose));
	frame2 = (md2frame_t *)((int) pheader + pheader->ofs_frames + (pheader->framesize * pose));

	VectorCopy(frame1->scale, scale1);
	VectorCopy(frame1->translate, translate1);
	VectorCopy(frame2->scale, scale2);
	VectorCopy(frame2->translate, translate2);
	verts1 = &frame1->verts[0];
	verts2 = &frame2->verts[0];
	order = (int *)((int)pheader + pheader->ofs_glcmds);

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		int prim;
		if (count < 0)
		{
			count = -count;
		    prim = GU_TRIANGLE_FAN;
		}
		else
		{
			prim = GU_TRIANGLE_STRIP;
		}

		// Allocate the vertices.
		struct vertex
		{
			float u, v;
            unsigned int color; 
			float x, y, z;
       	};

		vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * count));

		for (int vertex_index = 0; vertex_index < count; ++vertex_index)
		{
			// texture coordinates come from the draw list
			out[vertex_index].u = ((float *)order)[0];
			out[vertex_index].v = ((float *)order)[1];
            l = shadedots[verts1->lightnormalindex];
 
			float       r,g,b;

			r = l * lightcolor[0];
            g = l * lightcolor[1];
            b = l * lightcolor[2];

            if(r > 1)
			   r = 1;
			if(g > 1)
			   g = 1;
			if(b > 1)
			   b = 1;

			out[vertex_index].x =
		(verts1[order[2]].v[0]*scale1[0]+translate1[0])*ilerp+
		(verts2[order[2]].v[0]*scale2[0]+translate2[0])*lerp;


			out[vertex_index].y =                              
		(verts1[order[2]].v[1]*scale1[1]+translate1[1])*ilerp+
        (verts2[order[2]].v[1]*scale2[1]+translate2[1])*lerp;

			out[vertex_index].z =
		(verts1[order[2]].v[2]*scale1[2]+translate1[2])*ilerp+
		(verts2[order[2]].v[2]*scale2[2]+translate2[2])*lerp;

			out[vertex_index].color =
		GU_COLOR(r, g, b, 1.0f);

		   order+=3;
		}

		if(r_showtris.value)
		{
		   sceGuDisable(GU_TEXTURE_2D);
		}
		sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : prim, GU_TEXTURE_32BITF | GU_VERTEX_32BITF  | GU_COLOR_8888, count, 0, out);
        if(r_showtris.value)
		{
		   sceGuEnable(GU_TEXTURE_2D);
		}
	}
	sceGuColor(0xffffffff);
}

/*
=============
GL_DrawQ2AliasShadow
=============
*/
void GL_DrawQ2AliasShadow (entity_t *e, md2_t *pheader, int lastpose, int pose, float lerp)
{
	float	ilerp, height, lheight;
	int		*order, count;
	md2trivertx_t	*verts1, *verts2;
	vec3_t	scale1, translate1, scale2, translate2, point;
	md2frame_t *frame1, *frame2;

	// Tomaz - New Shadow Begin
	trace_t		downtrace;
	vec3_t		downmove;
	float		s1,c1;
	// Tomaz - New Shadow End

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;

	ilerp = 1.0 - lerp;

	// Tomaz - New Shadow Begin
	VectorCopy (e->origin, downmove);
	downmove[2] = downmove[2] - 4096;
	memset (&downtrace, 0, sizeof(downtrace));
	SV_RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, e->origin, downmove, &downtrace);

	s1 = sin( e->angles[1]/180*M_PI);
	c1 = cos( e->angles[1]/180*M_PI);
	// Tomaz - New Shadow End

	//new version by muff - fixes bug, easier to read, faster (well slightly)
	frame1 = (md2frame_t *)((int) pheader + pheader->ofs_frames + (pheader->framesize * lastpose));
	frame2 = (md2frame_t *)((int) pheader + pheader->ofs_frames + (pheader->framesize * pose));

	VectorCopy(frame1->scale, scale1);
	VectorCopy(frame1->translate, translate1);
	VectorCopy(frame2->scale, scale2);
	VectorCopy(frame2->translate, translate2);
	verts1 = &frame1->verts[0];
	verts2 = &frame2->verts[0];
	order = (int *)((int) pheader + pheader->ofs_glcmds);

	height = -lheight + 1.0;
    
    while (1)
    {
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		int prim;
		if (count < 0)
		{
			count = -count;
		    prim = GU_TRIANGLE_FAN;
		}
		else
		{
			prim = GU_TRIANGLE_STRIP;
		}

		// Allocate the vertices.
		struct vertex
		{
			float x, y, z;
		};

		vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * count));

		for (int vertex_index = 0; vertex_index < count; ++vertex_index)
		{
            point[0] =
		(verts1[order[2]].v[0]*scale1[0]+translate1[0])*ilerp+(verts2[order[2]].v[0]*scale2[0]+translate2[0])*lerp;

			point[1] =
		(verts1[order[2]].v[1]*scale1[1]+translate1[1])*ilerp+(verts2[order[2]].v[1]*scale2[1]+translate2[1])*lerp;

			point[2] =
		(verts1[order[2]].v[2]*scale1[2]+translate1[2])*ilerp+(verts2[order[2]].v[2]*scale2[2]+translate2[2])*lerp;

			// Tomaz - New shadow Begin
			point[2] =  - (e->origin[2] - downtrace.endpos[2]) ;

			point[2] += ((point[1] * (s1 * downtrace.plane.normal[0])) -
						  (point[0] * (c1 * downtrace.plane.normal[0])) -
						  (point[0] * (s1 * downtrace.plane.normal[1])) -
						  (point[1] * (c1 * downtrace.plane.normal[1]))) +
						  ((1.0 - downtrace.plane.normal[2])*20) + 0.2 ;

			out[vertex_index].x = point[0];
		    out[vertex_index].y = point[1];
		    out[vertex_index].z = point[2];
			// Tomaz - New shadow Begin

			order+=3;
		 }
	     if(r_showtris.value)
		 {
			sceGuDisable(GU_TEXTURE_2D);
		 }
		 sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : prim,GU_VERTEX_32BITF, count, 0, out);
	     if(r_showtris.value)
		 {
			sceGuEnable(GU_TEXTURE_2D);
		 }
	}
}

/*
=================
R_SetupQ2AliasFrame

=================
*/
void R_SetupQ2AliasFrame (entity_t *e, md2_t *pheader)
{
	int				frame;
	float			lerp, lerpscale;

	frame = e->frame;

    sceGumPushMatrix ();
	R_RotateForEntity (e, 0);

	if ((frame >= pheader->num_frames) || (frame < 0))
	{
		Con_DPrintf ("R_SetupQ2AliasFrame: no such frame %d\n", frame);
		frame = 0;
	}

	if (e->draw_lastmodel == e->model)
	{
		if (frame != e->draw_pose)
		{
			e->draw_lastpose = e->draw_pose;
			e->draw_pose = frame;
			e->draw_lerpstart = cl.time;
			lerp = 0;
		}
		else
			lerp = (cl.time - e->draw_lerpstart) * 10.0; //dr_mabuse1981: lowered abit, so MD2s will look better
	}
	else // uninitialized
	{
		e->draw_lastmodel = e->model;
		e->draw_lastpose = e->draw_pose = frame;
		e->draw_lerpstart = cl.time;
		lerp = 0;
	}
	if (lerp > 1) lerp = 1;

	GL_DrawQ2AliasFrame (e, pheader, e->draw_lastpose, frame, lerp);

	if (r_shadows.value)
	{
        trace_t		downtrace;
		vec3_t		downmove;
		VectorCopy (e->origin, downmove);

		downmove[2] = downmove[2] - 4096;
		memset (&downtrace, 0, sizeof(downtrace));
		SV_RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, e->origin, downmove, &downtrace);

		sceGuDisable (GU_TEXTURE_2D);
		sceGuEnable (GU_BLEND);
		sceGuDepthMask(GU_TRUE); // disable zbuffer updates
		sceGuColor(GU_COLOR(0,0,0,(1 - ((e->origin[2] + e->model->mins[2]-downtrace.endpos[2])/60))));

		//stencil shadows
		sceGuEnable(GU_STENCIL_TEST);
        sceGuStencilFunc(GU_EQUAL,1,2);
        sceGuStencilOp(GU_KEEP,GU_KEEP,GU_INCR);

		GL_DrawQ2AliasShadow (e, pheader, e->draw_lastpose, frame, lerp);

		sceGuDisable(GU_STENCIL_TEST);

		sceGuDepthMask(GU_FALSE); // enable zbuffer updates
		sceGuEnable (GU_TEXTURE_2D);
		sceGuDisable (GU_BLEND);
		sceGuColor(0xffffffff);
	}

	sceGumPopMatrix();
	sceGumUpdateMatrix();
}


/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i, j;
	int			lnum;
	vec3_t		dist;
	float		add;
	model_t		*clmodel;
	vec3_t		mins, maxs;
	aliashdr_t	*paliashdr;
	trivertx_t	*verts, *v;
	int			index;
	float		s, t, an;
	int			anim;
	bool 		force_fullbright, additive;

	md2_t		*pheader; // LH / muff

#ifdef HL_RENDER
  if(ISADDITIVE(e))
  {
	float deg = e->renderamt;
	float alpha_val  = deg;
    float alpha_val2 = 1 - deg;

	if(deg <= 0.7)
	 sceGuDepthMask(GU_TRUE);

	sceGuEnable (GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX,
	GU_COLOR(alpha_val,alpha_val,alpha_val,alpha_val),
	GU_COLOR(alpha_val2,alpha_val2,alpha_val2,alpha_val2));
  }
  else if(ISGLOW(e))
  {
	sceGuDepthMask(GU_TRUE);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);
	sceGuTexFunc(GU_TFX_MODULATE , GU_TCC_RGBA);
  }
  else if (ISSOLID(e))
  {
	sceGuEnable(GU_ALPHA_TEST);
	float c = (e->renderamt) * 255.0f;
	sceGuAlphaFunc(GU_GREATER, 0x88, c);
  }
#endif
	force_fullbright = false;
    additive = false;
	clmodel = currententity->model;

	VectorAdd (currententity->origin, clmodel->mins, mins);
	VectorAdd (currententity->origin, clmodel->maxs, maxs);

	//if (e->angles[0] || e->angles[1] || e->angles[2])
	//{
	//	if (R_CullSphere(e->origin, clmodel->radius))
	//		return;
	//}
	//else
	//{
		if (R_CullBox(mins, maxs))
			return;
	//}


	VectorCopy (currententity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	//
	// get lighting information
	//

	// LordHavoc: .lit support begin
	//ambientlight = shadelight = R_LightPoint (currententity->origin); // LordHavoc: original code, removed shadelight and ambientlight
	R_LightPoint(currententity->origin); // LordHavoc: lightcolor is all that matters from this
	// LordHavoc: .lit support end

	// always give the gun some light
	// LordHavoc: .lit support begin
	//if (e == &cl.viewent && ambientlight < 24) // LordHavoc: original code
	//	ambientlight = shadelight = 24; // LordHavoc: original code
/*
	if (e == &cl.viewent)
	{
		if (lightcolor[0] < 24)
			lightcolor[0] = 24;
		if (lightcolor[1] < 24)
			lightcolor[1] = 24;
		if (lightcolor[2] < 24)
			lightcolor[2] = 24;
	}
*/
	// LordHavoc: .lit support end

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			VectorSubtract (currententity->origin,
							cl_dlights[lnum].origin,
							dist);
			add = cl_dlights[lnum].radius - Length(dist);

			// LordHavoc: .lit support begin
			/* LordHavoc: original code
			if (add > 0) {
				ambientlight += add;
				//ZOID models should be affected by dlights as well
				shadelight += add;
			}
			*/
			if (add > 0)
			{
				lightcolor[0] += add * cl_dlights[lnum].color[0];
				lightcolor[1] += add * cl_dlights[lnum].color[1];
				lightcolor[2] += add * cl_dlights[lnum].color[2];
			}
			// LordHavoc: .lit support end
		}
	}

	// clamp lighting so it doesn't overbright as much
/*
	if (shadelight > 65)
		shadelight = 65;
	if (ambientlight > 196)
    {
		ambientlight = 196;
		force_fullbright = true;
    }
    else
        force_fullbright = false;
*/

	// ZOID: never allow players to go totally black
    //	i = currententity - cl_entities;
    //	if (i >= 1 && i<=cl.maxclients /*&& !strcmp (currententity->model->name, "progs/player.mdl") */)
	// LordHavoc: .lit support begin
	//	if (ambientlight < 8) // LordHavoc: original code
	//		ambientlight = shadelight = 8; // LordHavoc: original code

	for(int g = 0; g < 3; g++)
	{
      if(lightcolor[g] < 8)
         lightcolor[g] = 8;

	  if(lightcolor[g] > 125)
         lightcolor[g] = 125;
	}
	// HACK HACK HACK -- no fullbright colors, so make torches and projectiles full light
	if (!strcmp (clmodel->name, "progs/flame2.mdl") ||
	    !strcmp (clmodel->name, "progs/flame.mdl") ||
	    !strcmp (clmodel->name, "progs/lavaball.mdl") ||
	    !strcmp (clmodel->name, "progs/bolt.mdl") ||
	    !strcmp (clmodel->name, "progs/bolt2.mdl") ||
	    !strcmp (clmodel->name, "progs/bolt3.mdl") ||
	    !strcmp (clmodel->name, "progs/eyes.mdl") ||
	    !strcmp (clmodel->name, "progs/k_spike.mdl") ||
	    !strcmp (clmodel->name, "progs/s_spike.mdl") ||
	    !strcmp (clmodel->name, "progs/spike.mdl") ||
	    !strcmp (clmodel->name, "progs/laser.mdl")) 
	{
		lightcolor[0] = lightcolor[1] = lightcolor[2] = 256;
		force_fullbright = true;
	}
	//DQ+ note: this is for tranparency on weapons,alphafunc for pink,alphafunc2 for blue
//	if (!strcmp (clmodel->name, "progs/model.mdl"))
//	{
//		alphafunc = true;
//	}
	shadedots = r_avertexnormal_dots[((int)(e->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];

	// LordHavoc: .lit support begin
	//shadelight = shadelight / 200.0; // LordHavoc: original code
	VectorScale(lightcolor, 1.0f / 200.0f, lightcolor);
	// LordHavoc: .lit support end

   	an = e->angles[1]/180*M_PI;
	shadevector[0] = cosf(-an);
	shadevector[1] = sinf(-an);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	//
	// locate the proper data
	//
   	if (clmodel->aliastype == ALIASTYPE_MD2)
	{
		pheader = (md2_t *)Mod_Extradata (currententity->model);
		c_alias_polys += pheader->num_tris;
	}
	else
	{
	    paliashdr = (aliashdr_t *)Mod_Extradata (currententity->model);
	     c_alias_polys += paliashdr->numtris;
	}

	//
	// draw all the triangles
	//
	if (clmodel->aliastype != ALIASTYPE_MD2)
	{
		sceGumPushMatrix();

		if (r_i_model_transform.value)
	        R_BlendedRotateForEntity (e, 0);
		else
			R_RotateForEntity (e, 0);

		const ScePspFVector3 translation =
		{
			paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]
		};
		sceGumTranslate(&translation);

		const ScePspFVector3 scaling =
		{
			paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]
		};
		sceGumScale(&scaling);
	}

	if (clmodel->aliastype == ALIASTYPE_MD2)
	{
			GL_Bind(pheader->gl_texturenum[currententity->skinnum]);
	}
	else
	{
		anim = (int)(cl.time*10) & 3;
	    GL_Bind(paliashdr->gl_texturenum[currententity->skinnum][anim]);
	}

	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (currententity->colormap != vid.colormap && 0 /* && !gl_nocolors.value*/)
	{
		i = currententity - cl_entities;
		if (i >= 1 && i<=cl.maxclients /*&& !strcmp (currententity->model->name, "progs/player.mdl")*/)
		{
		    GL_Bind(playertextures - 1 + i);
		}
	}
//for models(pink transparency)
	if (alphafunc)
	{
		sceGuEnable(GU_ALPHA_TEST);
		sceGuAlphaFunc(GU_GREATER, 0, 0xff);
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
	if (force_fullbright)
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    else
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	//for models (blue transparency)
	 if (alphafunc2)
    {
        sceGuEnable(GU_ALPHA_TEST);
        sceGuAlphaFunc(GU_GREATER, 0xaa, 0xff);
        sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    }

	sceGuShadeModel(GU_SMOOTH);

	if (clmodel->aliastype != ALIASTYPE_MD2)
	{
		sceGumUpdateMatrix();
	}

	if (clmodel->aliastype == ALIASTYPE_MD2)
		R_SetupQ2AliasFrame (currententity, pheader);
	else
	{
	        if (r_i_model_animation.value)
	        {
	        R_SetupAliasBlendedFrame (currententity->frame, paliashdr, currententity, e->angles[0], e->angles[1]);
	        }
	        else
	        {
			    if (r_ipolations.value)
		        {
		           if (r_asynch.value)
		           {
		             if (currententity->interpolation >= r_ipolations.value)
		             {
		             currententity->last_frame = currententity->current_frame;
		             currententity->current_frame = currententity->frame;
		             currententity->interpolation = 1;
		             }
		           }
		           else
		           {
		            if (currententity->frame != currententity->current_frame)
		            {
		             currententity->last_frame = currententity->current_frame;
		             currententity->current_frame = currententity->frame;
		             currententity->interpolation = 1;
		            }
		          }
		          R_SetupAliasInterpolatedFrame (currententity->current_frame,
		                                         currententity->last_frame,
		                                         currententity->interpolation,
		                                         paliashdr);
				}
				else
				 R_SetupAliasFrame (currententity->frame, paliashdr, e->angles[0], e->angles[1]);
	        }
	}
	sceGuAlphaFunc(GU_GREATER, 0, 0xff);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuTexFunc(GU_TFX_REPLACE , GU_TCC_RGBA);
	sceGuShadeModel(GU_FLAT);
	if (clmodel->aliastype != ALIASTYPE_MD2)
	{
		sceGuShadeModel(GU_FLAT);
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		sceGumPopMatrix();
		sceGumUpdateMatrix();

		if (r_shadows.value)
		{
        	// Tomaz - New Shadow Begin
			trace_t		downtrace;
			vec3_t		downmove;
			// Tomaz - New Shadow End

			sceGumPushMatrix();

			if (r_i_model_transform.value)
	            R_BlendedRotateForEntity (e, 1);
		    else
			    R_RotateForEntity (e, 1);

			VectorCopy (e->origin, downmove);

			downmove[2] = downmove[2] - 4096;
			memset (&downtrace, 0, sizeof(downtrace));
			SV_RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, e->origin, downmove, &downtrace);

			sceGuDisable (GU_TEXTURE_2D);
			sceGuEnable (GU_BLEND);
			sceGuDepthMask(GU_TRUE); // disable zbuffer updates
			sceGuColor(GU_COLOR(0,0,0,(1 - ((mins[2]-downtrace.endpos[2])/60))));

			//stencil shadows
			sceGuEnable(GU_STENCIL_TEST);
	        sceGuStencilFunc(GU_EQUAL,1,2);
	        sceGuStencilOp(GU_KEEP,GU_KEEP,GU_INCR);

			if (r_i_model_animation.value)
	           GL_DrawAliasBlendedShadow (paliashdr, lastposenum0, lastposenum, currententity);
	        else
	           GL_DrawAliasShadow (paliashdr, lastposenum);

	        sceGuDisable(GU_STENCIL_TEST);

			sceGumPopMatrix();
			sceGumUpdateMatrix();
	        sceGuColor(0xffffffff);
			sceGuDepthMask(GU_FALSE); // enable zbuffer updates
			sceGuEnable (GU_TEXTURE_2D);
			sceGuDisable (GU_BLEND);
		}
	}
	
#ifdef HL_RENDER
   if (ISADDITIVE(e))
   {
        float deg = e->renderamt;

		if(deg <= 0.7)
		   sceGuDepthMask(GU_FALSE);

		//sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        //sceGuDisable (GU_BLEND);
   }
   else if(ISSOLID(e))
   {
	    sceGuAlphaFunc(GU_GREATER, 0, 0xff);
	    sceGuDisable(GU_ALPHA_TEST);
   }
   else if(ISGLOW(e))
    {
        sceGuDepthMask(GU_FALSE);
		//sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        //sceGuDisable (GU_BLEND);
    }
#endif

   sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
   sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
   sceGuDisable(GU_BLEND);

#ifdef GGLOW
	// hacked in from UNOFFICAL GLQUAKE code
	// muff@yakko.globalnet.co.uk
	// added lightning glow code  -  20 Mar 2000
	// added rocket tailglow code -  20 Mar 2000
	// updated to be pastable - 5 May 2000
	if ( (!strncmp (clmodel->name, "progs/flame.mdl",11)) ||
		 (!strncmp (clmodel->name, "progs/bolt",10)) ||
		 (!strcmp (clmodel->name, "progs/missile.mdl")) ||
		 (!strcmp (clmodel->name, "progs/quaddama.mdl")) ||
		 (!strcmp (clmodel->name, "progs/invulner.mdl")) ||
		 (!strcmp (clmodel->name, "progs/laser.mdl"))
		 )
	{
		// Draw torch flares. KH
		// NOTE: It would be better if we batched these up.
		//       All those state changes are not nice. KH

		// This relies on unchanged game code!
		const int TORCH_STYLE = 1; // Flicker.

		vec3_t	lightorigin;	// Origin of torch.
		vec3_t	v;				// Vector to torch.
		float	radius;			// Radius of torch flare.
		float	distance;		// Vector distance to torch.
		float	intensity;		// Intensity of torch flare.

		// NOTE: I don't think this is centered on the model.
		VectorCopy(currententity->origin, lightorigin);

		//set radius based on what model we are doing here
		if( (!strncmp (clmodel->name, "progs/flame.mdl",11))  ||
		    (!strcmp (clmodel->name, "progs/missile.mdl")) )
		 radius = 20.0f;
		else if( (!strncmp (clmodel->name, "progs/bolt",10)) ||
		 (!strcmp (clmodel->name, "progs/laser.mdl")) )
		 radius = 30.0f;
		else if( (!strcmp (clmodel->name, "progs/quaddama.mdl")) ||
				 (!strcmp (clmodel->name, "progs/invulner.mdl"))
			   )
		 radius = 50.0f;

		VectorSubtract(lightorigin, r_origin, v);

		// See if view is outside the light.
		distance = Length(v);
		if (distance > radius)
		{
			sceGuDepthMask (GU_TRUE);
			sceGuDisable (GU_TEXTURE_2D);
			sceGuShadeModel (GU_SMOOTH);
			sceGuEnable (GU_BLEND);
			sceGuBlendFunc (GU_ADD, GU_ONE_MINUS_SRC_COLOR, GU_ONE_MINUS_SRC_COLOR, 0, 0);

			// Translate the glow to coincide with the flame. KH
			// or be at the tail of the missile - muff
			sceGumPushMatrix();

			if (!strncmp (clmodel->name, "progs/flame.mdl",11))
			{
			 /*glTranslatef(0.0f, 0.0f, 8.0f);*/
             ScePspFVector3 translation ={0.0f, 0.0f, 8.0f};
	         sceGumTranslate(&translation);
			}
			else if (!strcmp (clmodel->name, "progs/missile.mdl"))
			{
				/*glTranslatef(cos( e->angles[1]/180*M_PI)*(-20.0f), sin( e->angles[1]/180*M_PI)*(-20.0f), sin( e->angles[0]/180*M_PI)*(-20.0f));*/
            ScePspFVector3 translation =
			{cos( e->angles[1]/180*M_PI)*(-20.0f), sin( e->angles[1]/180*M_PI)*(-20.0f), sin( e->angles[0]/180*M_PI)*(-20.0f)};
	         sceGumTranslate(&translation);
			}
		    else if( (!strcmp (clmodel->name, "progs/quaddama.mdl")) ||
				 (!strcmp (clmodel->name, "progs/invulner.mdl"))
			   )
			{
             /*glTranslatef(0.0f, 0.0f, 20.0f);*/
             ScePspFVector3 translation ={0.0f, 0.0f, 20.0f};
	         sceGumTranslate(&translation);
			}
			else if(!strcmp (clmodel->name, "progs/invulner.mdl"))
			{
			 /*glTranslatef(0.0f, 0.0f, 0.0f);*/
             ScePspFVector3 translation ={0.0f, 0.0f, 0.0f};
	         sceGumTranslate(&translation);
			}

              sceGumUpdateMatrix();

			// Invert (fades as you approach) - if we are a torch
			if (!strncmp (clmodel->name, "progs/flame.mdl",11))
			{
				// Diminish torch flare inversely with distance.
			    intensity = (1024.0f - distance) / 1024.0f;
				intensity = (1.0f - intensity);
			}
			// or fix settings if lightning or missile
			else if( (!strncmp (clmodel->name, "progs/bolt",10)) ||
					 (!strcmp (clmodel->name, "progs/laser.mdl")) )
				intensity = 0.2;
			else if (!strcmp (clmodel->name, "progs/missile.mdl"))
				intensity = 0.5;
		    else if( (!strcmp (clmodel->name, "progs/quaddama.mdl")) ||
				 (!strcmp (clmodel->name, "progs/invulner.mdl")) )
			    intensity = 0.3;

			// Clamp, but don't let the flare disappear.
			if (intensity > 1.0) intensity = 1.0;
			if (intensity < 0.0) intensity = 0.0;

			// Now modulate with flicker.
			i = (int)(cl.time*10);
			if (!cl_lightstyle[TORCH_STYLE].length)
			{
				j = 256;
			}
			else
			{
				j = i % cl_lightstyle[TORCH_STYLE].length;
				j = cl_lightstyle[TORCH_STYLE].map[j] - 'a';
				j = j*22;
			}
			intensity *= ((float)j / 255.0);

			// Set yellow intensity
            float c[4];
			// set the colour of the glow - muff
			if ((!strncmp (clmodel->name, "progs/flame.mdl",11))  || (!strcmp (clmodel->name, "progs/missile.mdl"))  )
			{
			 c[0]= (0.8f*intensity) * 255.0f;
             c[1]= (0.4f*intensity) * 255.0f;
             c[2]= (0.1f*intensity) * 255.0f;
             c[3]= 1.0f * 255.0f;
             sceGuColor(GU_RGBA(
			 static_cast<unsigned int>(c[0]),
			 static_cast<unsigned int>(c[1]),
			 static_cast<unsigned int>(c[2]),
			 static_cast<unsigned int>(c[3]))
			 );

			}
			else if	(!strncmp (clmodel->name, "progs/bolt",10))
			{
             c[0]= (0.2f*intensity) * 255.0f;
             c[1]= (0.2f*intensity) * 255.0f;
             c[2]= (0.8f*intensity) * 255.0f;
             c[3]= 1.0f * 255.0f;

			 sceGuColor(GU_RGBA(
			 static_cast<unsigned int>(c[0]),
			 static_cast<unsigned int>(c[1]),
			 static_cast<unsigned int>(c[2]),
			 static_cast<unsigned int>(c[3]))
			 );
			}
			else if (!strcmp (clmodel->name, "progs/quaddama.mdl"))
			{
             c[0]= (0.1f*intensity) * 255.0f;
             c[1]= (0.1f*intensity) * 255.0f;
             c[2]= (0.8f*intensity) * 255.0f;
             c[3]= 1.0f * 255.0f;

			 sceGuColor(GU_RGBA(
			 static_cast<unsigned int>(c[0]),
			 static_cast<unsigned int>(c[1]),
			 static_cast<unsigned int>(c[2]),
			 static_cast<unsigned int>(c[3]))
			 );
			}
			else if (!strcmp (clmodel->name, "progs/invulner.mdl"))
			{
             c[0]= (0.8f*intensity) * 255.0f;
             c[1]= (0.1f*intensity) * 255.0f;
             c[2]= (0.1f*intensity) * 255.0f;
             c[3]= 1.0f * 255.0f;

			 sceGuColor(GU_RGBA(
			 static_cast<unsigned int>(c[0]),
			 static_cast<unsigned int>(c[1]),
			 static_cast<unsigned int>(c[2]),
			 static_cast<unsigned int>(c[3]))
			 );
			}
			else if (!strcmp (clmodel->name, "progs/laser.mdl"))
			{
             c[0]= (0.8f*intensity) * 255.0f;
             c[1]= (0.4f*intensity) * 255.0f;
             c[2]= (0.1f*intensity) * 255.0f;
             c[3]= 1.0f * 255.0f;

			 sceGuColor(GU_RGBA(
			 static_cast<unsigned int>(c[0]),
			 static_cast<unsigned int>(c[1]),
			 static_cast<unsigned int>(c[2]),
			 static_cast<unsigned int>(c[3]))
			 );
			}
			else
			{
			 c[0]= (0.1f*intensity) * 255.0f;
             c[1]= (0.1f*intensity) * 255.0f;
             c[2]= (0.1f*intensity) * 255.0f;
             c[3]= 1.0f * 255.0f;

			 sceGuColor(GU_RGBA(
			 static_cast<unsigned int>(c[0]),
			 static_cast<unsigned int>(c[1]),
			 static_cast<unsigned int>(c[2]),
			 static_cast<unsigned int>(c[3]))
			 );
			}

			 

			// Allocate the vertices.
			struct vertex
		    {
			float x, y, z;
		    };

		   vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 18));

			out[0].x =
			lightorigin[0] - vpn[0]*radius;
            out[0].y =
			lightorigin[1] - vpn[1]*radius;
            out[0].z =
			lightorigin[2] - vpn[2]*radius;

			int vd = 17;
			int vi = 0;
			
			while(vd != 0)
			{
              vd--;
              vi++;
              
			  out[vi].x =
			lightorigin[0] + vright[0]*cos(vd/16.0f * M_PI*2)*radius + vup[0]*sin(vd/16.0f * M_PI*2)*radius;
              out[vi].y =
			lightorigin[1] + vright[1]*cos(vd/16.0f * M_PI*2)*radius + vup[1]*sin(vd/16.0f * M_PI*2)*radius;
              out[vi].z =
			lightorigin[2] + vright[2]*cos(vd/16.0f * M_PI*2)*radius + vup[2]*sin(vd/16.0f * M_PI*2)*radius;
			}
			
			sceGuDrawArray(GU_TRIANGLE_FAN , GU_VERTEX_32BITF, 18, 0, out);


			// Restore previous matrix! KH
			sceGuColor(0xffffffff));
            sceGumPopMatrix();
			sceGumUpdateMatrix();
			sceGuDisable (GU_BLEND);
			sceGuEnable (GU_TEXTURE_2D);
			sceGuBlendFunc (GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
			sceGuDepthMask (GU_FALSE);
		}
	}
#endif

}

//==================================================================================
//=================================Q3 Models========================================
//==================================================================================

int		bodyframe = 0, legsframe = 0;
animtype_t	bodyanim, legsanim;

//ScePspFMatrix4	matrix;

void R_ReplaceQ3Frame (int frame)
{
	animdata_t		*currbodyanim, *currlegsanim;
	static	animtype_t	oldbodyanim, oldlegsanim;
	static	float		bodyanimtime, legsanimtime;
	static	qboolean	deathanim = qfalse;

	if (deathanim)
	{
		bodyanim = oldbodyanim;
		legsanim = oldlegsanim;
	}

	if (frame < 41 || frame > 102)
		deathanim = qfalse;

	if (frame >= 0 && frame <= 5)		// axrun
	{
		bodyanim = torso_stand2;
		legsanim = legs_run;
	}
	else if (frame >= 6 && frame <= 11)	// rockrun
	{
		bodyanim = torso_stand;
		legsanim = legs_run;
	}
	else if ((frame >= 12 && frame <= 16) || (frame >= 35 && frame <= 40))	// stand, pain
	{
		bodyanim = torso_stand;
		legsanim = legs_idle;
	}
	else if ((frame >= 17 && frame <= 28) || (frame >= 29 && frame <= 34))	// axstand, axpain
	{
		bodyanim = torso_stand2;
		legsanim = legs_idle;
	}
	else if (frame >= 41 && frame <= 102 && !deathanim)	// axdeath, deatha, b, c, d, e
	{
		bodyanim = legsanim = both_death1;
		deathanim = qtrue;
	}
	else if (frame > 103 && frame <= 118)	// gun attacks
	{
		bodyanim = torso_attack;
	}
	else if (frame >= 119)			// axe attacks
	{
		bodyanim = torso_attack2;
	}

	currbodyanim = &anims[bodyanim];
	currlegsanim = &anims[legsanim];

	if (bodyanim == oldbodyanim)
	{
		if (cl.time >= bodyanimtime + currbodyanim->interval)
		{
			if (currbodyanim->loop_frames && bodyframe + 1 >= currbodyanim->offset + currbodyanim->loop_frames)
				bodyframe = currbodyanim->offset;
			else if (bodyframe + 1 < currbodyanim->offset + currbodyanim->num_frames)
				bodyframe++;
			bodyanimtime = cl.time;
		}
	}
	else
	{
		bodyframe = currbodyanim->offset;
		bodyanimtime = cl.time;
	}

	if (legsanim == oldlegsanim)
	{
		if (cl.time >= legsanimtime + currlegsanim->interval)
		{
			if (currlegsanim->loop_frames && legsframe + 1 >= currlegsanim->offset + currlegsanim->loop_frames)
				legsframe = currlegsanim->offset;
			else if (legsframe + 1 < currlegsanim->offset + currlegsanim->num_frames)
				legsframe++;
			legsanimtime = cl.time;
		}
	}
	else
	{
		legsframe = currlegsanim->offset;
		legsanimtime = cl.time;
	}

	oldbodyanim = bodyanim;
	oldlegsanim = legsanim;
}

int		multimodel_level;
bool	surface_transparent;

/*
=================
R_DrawQ3Frame
=================
*/
void R_DrawQ3Frame (int frame, md3header_t *pmd3hdr, md3surface_t *pmd3surf, entity_t *ent, int distance)
{
	int		i, j, numtris, pose, pose1, pose2;
	float		l, lerpfrac;
	vec3_t		lightvec, interpolated_verts;
	unsigned int	*tris;
	md3tc_t		*tc;
	md3vert_mem_t	*verts, *v1, *v2;
	model_t		*clmodel = ent->model;

	if ((frame >= pmd3hdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_DrawQ3Frame: no such frame %d\n", frame);
		frame = 0;
	}

	if (ent->pose1 >= pmd3hdr->numframes)
		ent->pose1 = 0;

	pose = frame;

	if (!strcmp(clmodel->name, "progs/player/lower.md3"))
		ent->frame_interval = anims[legsanim].interval;
	else if (!strcmp(clmodel->name, "progs/player/upper.md3"))
		ent->frame_interval = anims[bodyanim].interval;
	else
		ent->frame_interval = 0.1;

	if (ent->pose2 != pose)
	{
		ent->frame_start_time = cl.time;
		ent->pose1 = ent->pose2;
		ent->pose2 = pose;
		ent->framelerp = 0;
	}
	else
	{
		ent->framelerp = (cl.time - ent->frame_start_time) / ent->frame_interval;
	}

	// weird things start happening if blend passes 1
	if (cl.paused || ent->framelerp > 1)
		ent->framelerp = 1;

	verts = (md3vert_mem_t *)((byte *)pmd3hdr + pmd3surf->ofsverts);
	tc = (md3tc_t *)((byte *)pmd3surf + pmd3surf->ofstc);
	tris = (unsigned int *)((byte *)pmd3surf + pmd3surf->ofstris);
	numtris = pmd3surf->numtris * 3;
	pose1 = ent->pose1 * pmd3surf->numverts;
	pose2 = ent->pose2 * pmd3surf->numverts;

	if (surface_transparent)
	{
		sceGuEnable (GU_BLEND);
		//sceGuBlendFunc (GL_ONE, GL_ONE);
		sceGuDepthMask (GU_TRUE);
		sceGuDisable (GU_CULL_FACE);
	}
	else
	if (ISADDITIVE(ent))
		sceGuEnable (GU_BLEND);

	// Allocate the vertices.
	struct vertex
	{
		float u, v;
		unsigned int color;
		float x, y, z;
	};

	vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * numtris));

	for (i=0 ; i<numtris ; i++)
	{
		float	s, t;

		v1 = verts + *tris + pose1;
		v2 = verts + *tris + pose2;

		s = tc[*tris].s, t = tc[*tris].t;

        out[i].u = s;
		out[i].v = t;

		lerpfrac = VectorL2Compare(v1->vec, v2->vec, distance) ? ent->framelerp : 1;


		l = FloatInterpolate (shadedots[v1->oldnormal>>8], lerpfrac, shadedots[v2->oldnormal>>8]);
		l = l / 256;
		l = fmin(l, 1);

		VectorInterpolate (v1->vec, lerpfrac, v2->vec, interpolated_verts);

		out[i].x = interpolated_verts[0];
        out[i].y = interpolated_verts[1];
        out[i].z = interpolated_verts[2];

		for (j = 0 ; j < 3 ; j++)
			lightvec[j] = lightcolor[j] /1.0f + l;

	   	out[i].color = GU_COLOR(lightvec[0], lightvec[1], lightvec[2], 1.0f);
		*tris++;
	}

    if(r_showtris.value)
	{
	  sceGuDisable(GU_TEXTURE_2D);
	}

	sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : GU_TRIANGLES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_COLOR_8888, numtris, 0, out);

    if(r_showtris.value)
	{
	  sceGuEnable(GU_TEXTURE_2D);
	}

	if (surface_transparent)
	{
		sceGuDisable (GU_BLEND);
		sceGuBlendFunc (GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuDepthMask (GU_FALSE);
		sceGuEnable (GU_CULL_FACE);
	}
	else
	if (ISADDITIVE(ent))
		sceGuDisable (GU_BLEND);
}

/*
=================
R_DrawQ3Shadow
=================
*/
void R_DrawQ3Shadow (entity_t *ent, float lheight, float s1, float c1, trace_t downtrace)
{
	int		i, j, numtris, pose1, pose2;
	vec3_t		point1, point2, interpolated;
	md3header_t	*pmd3hdr;
	md3surface_t	*pmd3surf;
	unsigned int	*tris;
	md3vert_mem_t	*verts;
	model_t		*clmodel = ent->model;
#if 0
	float		m[16];
	md3tag_t	*tag;
	tagentity_t	*tagent;
#endif

	pmd3hdr = (md3header_t *)Mod_Extradata (clmodel);

	pmd3surf = (md3surface_t *)((byte *)pmd3hdr + pmd3hdr->ofssurfs);
	for (i=0 ; i<pmd3hdr->numsurfs ; i++)
	{
		verts = (md3vert_mem_t *)((byte *)pmd3hdr + pmd3surf->ofsverts);
		tris = (unsigned int *)((byte *)pmd3surf + pmd3surf->ofstris);
		numtris = pmd3surf->numtris * 3;
		pose1 = ent->pose1 * pmd3surf->numverts;
		pose2 = ent->pose2 * pmd3surf->numverts;

		// Allocate the vertices.
	    struct vertex
	    {
		 float x, y, z;
	    };

	    vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * numtris));

		for (j=0 ; j<numtris ; j++)
		{
			// normals and vertexes come from the frame list
			VectorCopy (verts[*tris+pose1].vec, point1);

			point1[0] -= shadevector[0] * (point1[2] + lheight);
			point1[1] -= shadevector[1] * (point1[2] + lheight);

			VectorCopy (verts[*tris+pose2].vec, point2);

			point2[0] -= shadevector[0] * (point2[2] + lheight);
			point2[1] -= shadevector[1] * (point2[2] + lheight);

			VectorInterpolate (point1, ent->framelerp, point2, interpolated);

			interpolated[2] = -(ent->origin[2] - downtrace.endpos[2]);

			interpolated[2] += ((interpolated[1] * (s1 * downtrace.plane.normal[0])) -
					    (interpolated[0] * (c1 * downtrace.plane.normal[0])) -
					    (interpolated[0] * (s1 * downtrace.plane.normal[1])) -
					    (interpolated[1] * (c1 * downtrace.plane.normal[1]))) +
					    ((1 - downtrace.plane.normal[2]) * 20) + 0.2;

			out[j].x = interpolated[0];
            out[j].y = interpolated[1];
            out[j].z = interpolated[2];
			*tris++;
		}
        if(r_showtris.value)
		{
		   sceGuDisable(GU_TEXTURE_2D);
		}
		sceGuDrawArray(r_showtris.value ? GU_LINE_STRIP : GU_TRIANGLES,GU_VERTEX_32BITF, numtris, 0, out);
        if(r_showtris.value)
		{
		   sceGuEnable(GU_TEXTURE_2D);
		}

		pmd3surf = (md3surface_t *)((byte *)pmd3surf + pmd3surf->ofsend);
	}

	if (!pmd3hdr->numtags)	// single model, done
		return;

// no multimodel shadow support yet
#if 0
	tag = (md3tag_t *)((byte *)pmd3hdr + pmd3hdr->ofstags);
	tag += ent->pose2 * pmd3hdr->numtags;
	for (i=0 ; i<pmd3hdr->numtags ; i++, tag++)
	{
		if (multimodel_level == 0 && !strcmp(tag->name, "tag_torso"))
		{
			tagent = &q3player_body;
			ent = &q3player_body.ent;
			multimodel_level++;
		}
		else if (multimodel_level == 1 && !strcmp(tag->name, "tag_head"))
		{
			tagent = &q3player_head;
			ent = &q3player_head.ent;
			multimodel_level++;
		}
		else
		{
			continue;
		}

		glPushMatrix ();
		R_RotateForTagEntity (tagent, tag, m);
		glMultMatrixf (m);
		R_DrawQ3Shadow (ent, lheight, s1, c1, downtrace);
		glPopMatrix ();
	}
#endif
}

/*
=================
R_SetupQ3Frame
=================
*/
void R_SetupQ3Frame (entity_t *ent)
{
	int		i, j, frame, shadernum, texture;
	float		m[16];
 md3header_t	*pmd3hdr;
	md3surface_t	*pmd3surf;
	md3tag_t	*tag;
	model_t		*clmodel = ent->model;
	tagentity_t	*tagent;

	if (!strcmp(clmodel->name, "progs/player/lower.md3"))
		frame = legsframe;
	else if (!strcmp(clmodel->name, "progs/player/upper.md3"))
		frame = bodyframe;
	else
		frame = ent->frame;

	// locate the proper data
	pmd3hdr = (md3header_t *)Mod_Extradata (clmodel);

	// draw all the triangles

	// draw non-transparent surfaces first, then the transparent ones
	for (i=0 ; i<2 ; i++)
	{
		pmd3surf = (md3surface_t *)((byte *)pmd3hdr + pmd3hdr->ofssurfs);
		for (j=0 ; j<pmd3hdr->numsurfs ; j++)
		{
			md3shader_mem_t	*shader;

 surface_transparent =  (strstr(pmd3surf->name, "energy") ||
						strstr(pmd3surf->name, "f_") ||
						strstr(pmd3surf->name, "flare") ||
						strstr(pmd3surf->name, "flash") ||
						strstr(pmd3surf->name, "Sphere") ||
						strstr(pmd3surf->name, "telep"));

			if ((!i && surface_transparent) || (i && !surface_transparent))
			{
				pmd3surf = (md3surface_t *)((byte *)pmd3surf + pmd3surf->ofsend);
				continue;
			}

			c_md3_polys += pmd3surf->numtris;

			shadernum = ent->skinnum;
			if ((shadernum >= pmd3surf->numshaders) || (shadernum < 0))
			{
				Con_DPrintf ("R_SetupQ3Frame: no such skin # %d\n", shadernum);
				shadernum = 0;
			}

			shader = (md3shader_mem_t *)((byte *)pmd3hdr + pmd3surf->ofsshaders);

			texture = shader[shadernum].gl_texnum;

			//glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			GL_Bind (texture);

			R_DrawQ3Frame (frame, pmd3hdr, pmd3surf, ent, INTERP_MAXDIST);

			pmd3surf = (md3surface_t *)((byte *)pmd3surf + pmd3surf->ofsend);
		}
	}

	if (!pmd3hdr->numtags)	// single model, done
		return;

	tag = (md3tag_t *)((byte *)pmd3hdr + pmd3hdr->ofstags);
	tag += frame * pmd3hdr->numtags;
	for (i=0 ; i<pmd3hdr->numtags ; i++, tag++)
	{
		if (multimodel_level == 0 && !strcmp(tag->name, "tag_torso"))
		{
			tagent = &q3player_body;
			ent = &q3player_body.ent;
			multimodel_level++;
		}
		else if (multimodel_level == 1 && !strcmp(tag->name, "tag_head"))
		{
			tagent = &q3player_head;
			ent = &q3player_head.ent;
			multimodel_level++;
		}
		else
		{
			continue;
		}

		sceGumPushMatrix ();
        R_RotateForTagEntity (tagent, tag, m);
        ConvertMatrix((float*)&md3mult, m);
		sceGumMultMatrix(&md3mult);
        sceGumUpdateMatrix ();
		R_SetupQ3Frame (ent);
		sceGumPopMatrix ();
	}
}

extern cvar_t	scr_fov;

/*
=================
R_DrawQ3Model
=================
*/
void R_DrawQ3Model (entity_t *ent)
{
	vec3_t		mins, maxs, md3_scale_origin = {0, 0, 0};
	model_t		*clmodel = ent->model;
	float scale;
    int			lnum;
	vec3_t		dist;
	float		add, an;

	VectorAdd (ent->origin, clmodel->mins, mins);
	VectorAdd (ent->origin, clmodel->maxs, maxs);
#ifdef HL_RENDER
    if(ISADDITIVE(ent))
    {
	  float deg = ent->renderamt;
	  float alpha_val  = deg;
      float alpha_val2 = 1 - deg;
	  if(deg <= 0.7)
	    sceGuDepthMask(GU_TRUE);

	  sceGuEnable (GU_BLEND);
	  sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX,
	  GU_COLOR(alpha_val,alpha_val,alpha_val,alpha_val),
	  GU_COLOR(alpha_val2,alpha_val2,alpha_val2,alpha_val2));
    }
    else if(ISGLOW(ent))
    {
	  sceGuDepthMask(GU_TRUE);
	  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);
	  sceGuTexFunc(GU_TFX_MODULATE , GU_TCC_RGB);
    }
    else if (ISSOLID(ent))
    {
	  sceGuEnable(GU_ALPHA_TEST);
	  float c = (ent->renderamt) * 255.0f;
	  sceGuAlphaFunc(GU_GREATER, 0x88, c);
    }
#endif
	if (ent->angles[0] || ent->angles[1] || ent->angles[2])
	{
		if (R_CullSphere(ent->origin, clmodel->radius))
			return;
	}
	else
	{
		if (R_CullBox(mins, maxs))
			return;
	}

	//==========================================================================

	VectorCopy (ent->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	//
	// get lighting information
	//
	R_LightPoint(currententity->origin); // LordHavoc: lightcolor is all that matters from this
	
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			VectorSubtract (currententity->origin,
							cl_dlights[lnum].origin,
							dist);
			add = cl_dlights[lnum].radius - Length(dist);
			if (add > 0)
			{
				lightcolor[0] += add * cl_dlights[lnum].color[0];
				lightcolor[1] += add * cl_dlights[lnum].color[1];
				lightcolor[2] += add * cl_dlights[lnum].color[2];
			}
		}
	}

	// clamp lighting so it doesn't overbright as much
    //ColorClamp(lightcolor[0], lightcolor[1], lightcolor[2], 0, 125, 8);

    for(int g = 0; g < 3; g++)
	{
      if(lightcolor[g] < 8)
         lightcolor[g] = 8;

	  if(lightcolor[g] > 125)
         lightcolor[g] = 125;
	}
	
	shadedots = r_avertexnormal_dots[((int)(ent->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];

	VectorScale(lightcolor, 1.0f / 200.0f, lightcolor);


   	an = ent->angles[1]/180*M_PI;
	shadevector[0] = cosf(-an);
	shadevector[1] = sinf(-an);
	shadevector[2] = 1;
	VectorNormalize (shadevector);


	//==========================================================================

	sceGumPushMatrix ();


	if (ent == &cl.viewent)
		R_RotateForViewEntity (ent);
	else
		R_RotateForEntity(ent, 0);

    if ((ent == &cl.viewent) && (scr_fov.value != 0))
	{
		if (scr_fov.value <= 90)
			scale = 1.0f;
		else
			scale = 1.0f / tan( DEG2RAD(scr_fov.value/2));

         const ScePspFVector3 translation =
	     {
		  md3_scale_origin[0]*scale, md3_scale_origin[1], md3_scale_origin[2]
	     };
	     sceGumTranslate(&translation);

         const ScePspFVector3 GUscale =
	     {
	      scale, 1, 1
	     };
	     sceGumScale(&GUscale);
	}
	else
	{
	     const ScePspFVector3 translation =
	     {
		  md3_scale_origin[0], md3_scale_origin[1], md3_scale_origin[2]
	     };
	     sceGumTranslate(&translation);
	}


	sceGuShadeModel (GU_SMOOTH);

	sceGumUpdateMatrix ();

    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);

   //==========================================================================

	if ((!strcmp(ent->model->name, "progs/player/lower.md3"))||(!strcmp(ent->model->name, "progs/player/upper.md3")))
	{
		//q3player_body.ent.renderamt = q3player_head.ent.renderamt = cl_entities[cl.viewentity].renderamt;
		R_ReplaceQ3Frame (ent->frame);
		ent->noshadow = qtrue;
	}

	multimodel_level = 0;
	R_SetupQ3Frame (ent);

	sceGuShadeModel (GU_FLAT);

	sceGuTexFunc(GU_TFX_REPLACE , GU_TCC_RGBA);

	sceGumPopMatrix ();
    sceGumUpdateMatrix ();

	if (r_shadows.value && !ent->noshadow)
	{
		int		farclip;
		float		theta, lheight, s1, c1;
		vec3_t		downmove;
		trace_t		downtrace;
		static	float	shadescale = 0;

		farclip = fmax((int)r_farclip.value, 4096);

		if (!shadescale)
			shadescale = 1 / sqrt(2);
		theta = -ent->angles[1] / 180 * M_PI;

		VectorSet (shadevector, cos(theta) * shadescale, sin(theta) * shadescale, shadescale);

		sceGumPushMatrix ();

		R_RotateForEntity (ent, 0);

		VectorCopy (ent->origin, downmove);
		downmove[2] -= farclip;
		memset (&downtrace, 0, sizeof(downtrace));
		SV_RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, ent->origin, downmove, &downtrace);

		lheight = ent->origin[2] - lightspot[2];

		s1 = sin(ent->angles[1] / 180 * M_PI);
		c1 = cos(ent->angles[1] / 180 * M_PI);

		sceGuDepthMask (GU_TRUE);
		sceGuDisable (GU_TEXTURE_2D);
		sceGuEnable (GU_BLEND);

		sceGuColor(GU_RGBA(
	       static_cast<unsigned int>(0.0f * 255.0f),
	       static_cast<unsigned int>(0.0f * 255.0f),
	       static_cast<unsigned int>(0.0f * 255.0f),
	       static_cast<unsigned int>(((ambientlight - (mins[2] - downtrace.endpos[2]))*r_shadows.value)*0.0066 * 255.0f)));

		multimodel_level = 0;

		sceGuEnable(GU_STENCIL_TEST);
		sceGuStencilFunc(GU_EQUAL,1,2);
		sceGuStencilOp(GU_KEEP,GU_KEEP,GU_INCR);

		R_DrawQ3Shadow (ent, lheight, s1, c1, downtrace);

		sceGuDisable(GU_STENCIL_TEST);

		sceGuDepthMask (GU_FALSE);
		sceGuEnable    (GU_TEXTURE_2D);
		sceGuDisable   (GU_BLEND);
        sceGuColor(GU_RGBA(0xff,0xff,0xff,0xff)); //return to normal color

		sceGumPopMatrix ();
        sceGumUpdateMatrix ();
	}
#ifdef HL_RENDER
   if (ISADDITIVE(ent))
   {
        float deg = ent->renderamt;

		if(deg <= 0.7)
		   sceGuDepthMask(GU_FALSE);

		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        sceGuDisable (GU_BLEND);
   }
   else if(ISSOLID(ent))
   {
	    sceGuAlphaFunc(GU_GREATER, 0, 0xff);
	    sceGuDisable(GU_ALPHA_TEST);
   }
   else if(ISGLOW(ent))
   {
        sceGuDepthMask(GU_FALSE);
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        sceGuDisable (GU_BLEND);
   }
#endif
}

/*
=============
R_DrawNullModel
From pspq2
=============
*/
void R_DrawNullModel(void)
{
	R_LightPoint(currententity->origin);
	sceGumPushMatrix();
	sceGuDisable(GU_TEXTURE_2D);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuShadeModel (GU_SMOOTH);
	R_RotateForEntity(currententity, 0);
	typedef struct VERT_t
	{
		float x, y, z;
	} VERT;
	VERT* v;
	sceGuColor(0x0099FF);
	v = (VERT*)sceGuGetMemory(sizeof(VERT) * 6);
	v[0].x =  0.0f; v[0].y =  0.0f; v[0].z =  9.0f;
	v[1].x =  9.0f; v[1].y =  0.0f; v[1].z =  0.0f;
	v[2].x =  0.0f; v[2].y = -9.0f; v[2].z =  0.0f;
	v[3].x = -9.0f; v[3].y =  0.0f; v[3].z =  0.0f;
	v[4].x =  0.0f; v[4].y =  9.0f; v[4].z =  0.0f;
	v[5].x =  9.0f; v[5].y =  0.0f; v[5].z =  0.0f;
	sceGumDrawArray(r_showtris.value ? GU_LINE_STRIP : GU_TRIANGLE_FAN, GU_VERTEX_32BITF | GU_TRANSFORM_3D, 6, 0, v);
	sceGuColor(0x0000FF);
	v = (VERT*)sceGuGetMemory(sizeof(VERT) * 6);
	v[0].x =  0.0f; v[0].y =  0.0f; v[0].z = -9.0f;
	v[1].x =  9.0f; v[1].y =  0.0f; v[1].z =  0.0f;
	v[2].x =  0.0f; v[2].y =  9.0f; v[2].z =  0.0f;
	v[3].x = -9.0f; v[3].y =  0.0f; v[3].z =  0.0f;
	v[4].x =  0.0f; v[4].y = -9.0f; v[4].z =  0.0f;
	v[5].x =  9.0f; v[5].y =  0.0f; v[5].z =  0.0f;
	sceGumDrawArray(r_showtris.value ? GU_LINE_STRIP : GU_TRIANGLE_FAN, GU_VERTEX_32BITF | GU_TRANSFORM_3D, 6, 0, v);
	sceGuTexFunc(GU_TFX_REPLACE , GU_TCC_RGBA);
	sceGuColor(0xFFFFFF);
	sceGuEnable(GU_TEXTURE_2D);
	sceGumPopMatrix();
}
/*
================
R_EmitWireBox -- johnfitz -- draws one axis aligned bounding box
================
*/
void R_EmitWireBox (vec3_t mins, vec3_t maxs, qboolean line_strip)
{
	// Allocate the vertices.
	struct vertex
	{
	   float x, y, z;
	};
	vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 10));
	out[0].x = mins[0]; out[0].y = mins[1]; out[0].z = mins[2];
	out[1].x = mins[0]; out[1].y = mins[1]; out[1].z = maxs[2];
	out[2].x = maxs[0]; out[2].y = mins[1]; out[2].z = mins[2];
	out[3].x = maxs[0]; out[3].y = mins[1]; out[3].z = maxs[2];
	out[4].x = maxs[0]; out[4].y = maxs[1]; out[4].z = mins[2];
	out[5].x = maxs[0]; out[5].y = maxs[1]; out[5].z = maxs[2];
	out[6].x = mins[0]; out[6].y = maxs[1]; out[6].z = mins[2];
	out[7].x = mins[0]; out[7].y = maxs[1]; out[7].z = maxs[2];
	out[8].x = mins[0]; out[8].y = mins[1]; out[8].z = mins[2];
	out[9].x = mins[0]; out[9].y = mins[1]; out[9].z = maxs[2];
	sceGuDrawArray(line_strip ? GU_LINE_STRIP : GU_TRIANGLE_STRIP,GU_VERTEX_32BITF, 10, 0, out);
}

/*
================
R_ShowBoundingBoxes -- johnfitz

draw bounding boxes -- the server-side boxes, not the renderer cullboxes
================
*/
void R_ShowBoundingBoxes (void)
{
	extern		edict_t *sv_player;
	vec3_t		mins,maxs;
	edict_t		*ed;
	int			i;

	if (!r_showbboxes.value || cl.maxclients > 1 || !r_drawentities.value || !sv.active)
		return;

	if(r_showbboxes_full.value)
	   sceGuDisable (GU_DEPTH_TEST);

	sceGuDisable (GU_TEXTURE_2D);
	sceGuDisable (GU_CULL_FACE);
	sceGuColor(GU_COLOR(0,1,0,1));
	for (i=0, ed = NEXT_EDICT(sv.edicts) ; i < sv.num_edicts ; i++, ed = NEXT_EDICT(ed))
	{
		if (ed == sv_player)
			continue; //don't draw player's own bbox

		R_EmitWireBox (ed->v.absmin, ed->v.absmax, (r_showbboxes.value >= 2) ? qtrue : qfalse);
	}
	sceGuColor(GU_COLOR(1,1,1,1));
	sceGuEnable (GU_TEXTURE_2D);
	sceGuEnable (GU_CULL_FACE);

	if(r_showbboxes_full.value)
	   sceGuEnable (GU_DEPTH_TEST);
	Sbar_Changed (); //so we don't get dots collecting on the statusbar
}

//==================================================================================
int SetFlameModelState (void)
{
	if (!r_part_flames.value && !strcmp(currententity->model->name, "progs/flame0.mdl"))
	{
		currententity->model = cl.model_precache[cl_modelindex[mi_flame1]];
	}
	else if (r_part_flames.value)
	{
		vec3_t	liteorg;

		VectorCopy (currententity->origin, liteorg);
		if (currententity->baseline.modelindex == cl_modelindex[mi_flame0])
		{
			if (r_part_flames.value == 2)
			{
				liteorg[2] += 14;
				QMB_Q3TorchFlame (liteorg, 15);
			}
			else
			{
				liteorg[2] += 5.5;

				if(r_flametype.value == 2)
				  QMB_FlameGt (liteorg, 7, 0.8);
				else
				  QMB_TorchFlame(liteorg);
			}
		}
		else if (currententity->baseline.modelindex == cl_modelindex[mi_flame1])
		{
			if (r_part_flames.value == 2)
			{
				liteorg[2] += 14;
				QMB_Q3TorchFlame (liteorg, 15);
			}
			else
			{
				liteorg[2] += 5.5;

				if(r_flametype.value > 1)
				  QMB_FlameGt (liteorg, 7, 0.8);
				else
			      QMB_TorchFlame(liteorg);

			}
			currententity->model = cl.model_precache[cl_modelindex[mi_flame0]];
		}
		else if (currententity->baseline.modelindex == cl_modelindex[mi_flame2])
		{
			if (r_part_flames.value == 2)
            {
				liteorg[2] += 14;
				QMB_Q3TorchFlame (liteorg, 32);
            }
			else 
			{
                liteorg[2] -= 1;

				if(r_flametype.value > 1)
				  QMB_FlameGt (liteorg, 12, 1);
				else
			      QMB_BigTorchFlame(liteorg);
			}
			return -1;	//continue;
		}
        else if (!strcmp(currententity->model->name, "progs/wyvflame.mdl"))
		{
			liteorg[2] -= 1;

			if(r_flametype.value > 1)
			  QMB_FlameGt (liteorg, 12, 1);
			else
			  QMB_BigTorchFlame(liteorg);

			return -1;	//continue;
		}
	}

	return 0;
}


/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities.value)
		return;

   	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		if (currententity == &cl_entities[cl.viewentity])
	       currententity->angles[0] *= 0.3;

        //currentmodel = currententity->model;
		if(!(currententity->model))
		{
			R_DrawNullModel();
			continue;
		}

		switch (currententity->model->type)
		{
		case mod_alias:

            if (qmb_initialized && SetFlameModelState() == -1)
					continue;

			R_DrawAliasModel (currententity);
			break;

		case mod_md3:
			R_DrawQ3Model (currententity);
			break;

		case mod_halflife:
			R_DrawHLModel (currententity);
			break;

		case mod_brush:
			R_DrawBrushModel (currententity);
			break;

		default:
			break;
		}
	}

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];
        
		switch (currententity->model->type)
		{
		case mod_sprite:
			R_DrawSpriteModel (currententity);
			break;
		}
	}

}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	/*
	float		ambient[4], diffuse[4];
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;
	int			ambientlight, shadelight;
*/
    // fenix@io.com: model transform interpolation
    float old_i_model_transform;


	if (!r_drawviewmodel.value)
		return;

	if (chase_active.value)
		return;

	if (envmap)
		return;

	if (!r_drawentities.value)
		return;

	if (cl.items & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent;
    
	if (!currententity->model)
		return;

	// Tomaz - QC Alpha Scale Begin
#ifdef HL_RENDER
	currententity->renderamt  = cl_entities[cl.viewentity].renderamt;
    currententity->rendermode = cl_entities[cl.viewentity].rendermode;
    currententity->rendercolor[0] = cl_entities[cl.viewentity].rendercolor[0];
    currententity->rendercolor[1] = cl_entities[cl.viewentity].rendercolor[1];
    currententity->rendercolor[2] = cl_entities[cl.viewentity].rendercolor[2];
#endif
    // Tomaz - QC Alpha Scale End
    

/*
	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - Length(dist);
		if (add > 0)
			ambientlight += add;
	}
*/
/*
	ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
	diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;
*/

	// hack the depth range to prevent view model from poking into walls
	sceGuDepthRange(0, 19660);
    switch (currententity->model->type)
	{
	case mod_alias:
		// fenix@io.com: model transform interpolation
        old_i_model_transform = r_i_model_transform.value;
        r_i_model_transform.value = false;
        R_DrawAliasModel (currententity);
        r_i_model_transform.value = old_i_model_transform;
		break;
    case mod_md3:
		R_DrawQ3Model (currententity);
		break;
    case mod_halflife:
		R_DrawHLModel (currententity);
		break;
	default:
		Con_Printf("Not drawing view model of type %i\n", currententity->model->type);
		break;
	}
	sceGuDepthRange(0, 65535);

	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    sceGuDisable(GU_BLEND);
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!r_polyblend.value)
		 return;

	if (!v_blend[3])
		return;

	sceGuEnable  (GU_BLEND);
	sceGuDisable (GU_TEXTURE_2D);
	sceGuTexFunc (GU_TFX_MODULATE, GU_TCC_RGBA);

    sceGumLoadIdentity();

	sceGuColor(GU_COLOR(v_blend[0], v_blend[1], v_blend[2], v_blend[3]));
    struct vertex
	{
	   short x, y, z;
	};
	vertex* const out = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	out[0].x = 0;
	out[0].y = 0;
	out[0].z = 0;

	out[1].x = 480;
	out[1].y = 272;
	out[1].z = 0;

	sceGuDrawArray(GU_SPRITES, GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, out);
	sceGuColor(0xffffffff);

	sceGuDisable (GU_BLEND);
	sceGuEnable  (GU_TEXTURE_2D);
	sceGuTexFunc (GU_TFX_REPLACE, GU_TCC_RGBA);
}


static int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	// disabled as well.
	if (r_refdef.fov_x == 90)
	{
		// front side is visible

		VectorAdd (vpn, vright, frustum[0].normal);
		VectorSubtract (vpn, vright, frustum[1].normal);

		VectorAdd (vpn, vup, frustum[2].normal);
		VectorSubtract (vpn, vup, frustum[3].normal);
	}
	else
	{
		RotatePointAroundVector( frustum[0].normal, vup, vpn,    -( 90 - r_refdef.fov_x * 0.5 ) );
		RotatePointAroundVector( frustum[1].normal, vup, vpn,     ( 90 - r_refdef.fov_x * 0.5 ) );
		RotatePointAroundVector( frustum[2].normal, vright, vpn,  ( 90 - r_refdef.fov_y * 0.5 ) );
		RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_refdef.fov_y * 0.5 ) );
	}

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}


void Fog_SetupFrame (void);
extern "C" void V_CalcBlend (void);
/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
// don't allow cheats in multiplayer

    Fog_SetupFrame ();

	if (cl.maxclients > 1)
		Cvar_Set ("r_fullbright", "0");

	R_AnimateLight ();

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	V_SetContentsColor (r_viewleaf->contents);

	V_CalcBlend ();

	r_cache_thrash = qfalse;

	c_brush_polys = 0;
	c_alias_polys = 0;
    c_md3_polys = 0;
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h;
    float fovx, fovy; //johnfitz
	int contents; //johnfitz

	//
	// set up viewpoint
	//
	/*
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	*/
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();

	x = r_refdef.vrect.x * glwidth/vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * glwidth/vid.width;
	y = (vid.height-r_refdef.vrect.y) * glheight/vid.height;
	y2 = (vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * glheight/vid.height;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glwidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glheight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	sceGuViewport(
		glx,
		gly + (glheight / 2) - y2 - (h / 2),
		w,
		h);
	sceGuScissor(x, glheight - y2 - h, x + w, glheight - y2);

    screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;

	 //johnfitz -- warp view for underwater
	fovx = screenaspect;
	fovy = r_refdef.fov_y;
	if (r_waterwarp.value)
	{
		contents = Mod_PointInLeaf (r_origin, cl.worldmodel)->contents;
		if (contents == CONTENTS_WATER ||
			contents == CONTENTS_SLIME ||
			contents == CONTENTS_LAVA)
		{
			//variance should be a percentage of width, where width = 2 * tan(fov / 2)
			//otherwise the effect is too dramatic at high FOV and too subtle at low FOV
			//what a mess!
			//fovx = atan(tan(DEG2RAD(r_refdef.fov_x) / 2) * (0.97 + sin(cl.time * 1) * 0.04)) * 2 / M_PI_DIV_180;
			fovy = atan(tan(DEG2RAD(r_refdef.fov_y) / 2) * (1.03 - sin(cl.time * 2) * 0.04)) * 2 / M_PI_DIV_180;
		}
	}

	sceGumPerspective(fovy, fovx, 4, r_maxrange.value);

	if (mirror)
	{
		if (mirror_plane->normal[2])
		{
			const ScePspFVector3 scaling = {1, -1, 1};
	        sceGumScale(&scaling);
		}
		else
		{
			const ScePspFVector3 scaling = {-1, 1, 1};
	        sceGumScale(&scaling);
		}
	}
	else
	{
	}
	sceGumUpdateMatrix();

	/*glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();*/
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

    /*glRotatef (-90,  1, 0, 0);	    // put Z going up*/
	sceGumRotateX(-90 * (GU_PI / 180.0f));

    /*glRotatef (90,  0, 0, 1);	    // put Z going up*/
	sceGumRotateZ(90 * (GU_PI / 180.0f));

    /*glRotatef (-r_refdef.viewangles[2],  1, 0, 0);*/
	sceGumRotateX(-r_refdef.viewangles[2] * (GU_PI / 180.0f));

    /*glRotatef (-r_refdef.viewangles[0],  0, 1, 0);*/
	sceGumRotateY(-r_refdef.viewangles[0] * (GU_PI / 180.0f));

    /*glRotatef (-r_refdef.viewangles[1],  0, 0, 1);*/
	sceGumRotateZ(-r_refdef.viewangles[1] * (GU_PI / 180.0f));

    /*glTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);*/
	const ScePspFVector3 translation = {
		-r_refdef.vieworg[0],
		-r_refdef.vieworg[1],
		-r_refdef.vieworg[2]
	};
	sceGumTranslate(&translation);

	/*glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);*/
	sceGumStoreMatrix(&r_world_matrix);
	sceGumUpdateMatrix();

	sceGumMatrixMode(GU_MODEL);

	clipping::begin_frame();

	//
	// set drawing parms
	//
	/*
	if (gl_cull.value)
	{
		glEnable(GL_CULL_FACE);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);*/
	sceGuDisable(GU_BLEND);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
}

void Fog_EnableGFog (void); 
void Fog_DisableGFog (void);
void R_DrawDecals (void);
void QMB_LetItRain(void);
void QMB_LaserSight (void);

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/

void R_RenderScene (void)
{
    R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

    Fog_EnableGFog (); //johnfitz

	R_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList ();

	/*GL_DisableMultitexture();*/

	R_RenderDlights ();

//	R_RenderDecals ();

	R_DrawDecals();

	//QMB_LetItRain();

	//QMB_LaserSight ();

	R_DrawParticles ();

	Fog_DisableGFog (); //johnfitz

	R_ShowBoundingBoxes ();
	
#ifdef GLTEST
	Test_Draw ();
#endif

}


/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
    if(r_refdef.fog_end > 0 && r_skyfog.value)
    {
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
        sceGuClearColor(GU_COLOR(r_refdef.fog_red * 0.01f,r_refdef.fog_green * 0.01f,r_refdef.fog_blue * 0.01f,1.0f));
    }
    else
    {
        sceGuClear(GU_DEPTH_BUFFER_BIT);
//      sceGuClearColor(GU_COLOR(0.25f,0.25f,0.25f,0.5f));
    }

	if(r_shadows.value)
	{
	  sceGuClearStencil(GU_TRUE);
	  sceGuClear(GU_STENCIL_BUFFER_BIT);
	}

	if (r_mirroralpha.value != 1.0)
	{
		/*
		gldepthmin = 0;
		gldepthmax = 0.5;
		glDepthFunc (GL_LEQUAL);
		*/
	}
	else
	{
		/*
		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc (GL_LEQUAL);*/
	}

	/*glDepthRange (gldepthmin, gldepthmax);*/
}

/*
=============
R_Mirror
=============
*/
void R_Mirror (void)
{
	float		d;
	msurface_t	*s;
	entity_t	*ent;

	if (!mirror)
		return;

	r_base_world_matrix = r_world_matrix;

	d = DotProduct (r_refdef.vieworg, mirror_plane->normal) - mirror_plane->dist;
	VectorMA (r_refdef.vieworg, -2*d, mirror_plane->normal, r_refdef.vieworg);

	d = DotProduct (vpn, mirror_plane->normal);
	VectorMA (vpn, -2*d, mirror_plane->normal, vpn);

	r_refdef.viewangles[0] = -asinf(vpn[2])/M_PI*180;
	r_refdef.viewangles[1] = atan2f(vpn[1], vpn[0])/M_PI*180;
	r_refdef.viewangles[2] = -r_refdef.viewangles[2];

	ent = &cl_entities[cl.viewentity];
	if (cl_numvisedicts < MAX_VISEDICTS)
	{
		cl_visedicts[cl_numvisedicts] = ent;
		cl_numvisedicts++;
	}
/*
	gldepthmin = 0.5;
	gldepthmax = 1;
	glDepthRange (gldepthmin, gldepthmax);
	glDepthFunc (GL_LEQUAL);
*/
	R_RenderScene ();
	R_DrawWaterSurfaces ();
/*
	gldepthmin = 0;
	gldepthmax = 0.5;
	glDepthRange (gldepthmin, gldepthmax);
	glDepthFunc (GL_LEQUAL);
*/
	// blend on top
	sceGuEnable (GU_BLEND);

    sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	
	if (mirror_plane->normal[2])
	{
	    const ScePspFVector3 scaling = {1, -1, 1};
		sceGumScale(&scaling);
  	}
	else
	{
		const ScePspFVector3 scaling = {-1, 1, 1};
		sceGumScale(&scaling);
	}

    //sceGuFrontFace(GU_CW);

    sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

	sceGumMatrixMode(GU_MODEL);

	sceGumStoreMatrix(&r_base_world_matrix);
	sceGumUpdateMatrix();

	sceGuColor(GU_COLOR(1,1,1,r_mirroralpha.value));

	s = cl.worldmodel->textures[mirrortexturenum]->texturechain;
	for ( ; s ; s=s->texturechain)
		R_RenderBrushPoly (s);
	cl.worldmodel->textures[mirrortexturenum]->texturechain = NULL;

	sceGuDisable (GU_BLEND);
	sceGuColor (0xffffffff);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1, time2;

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
		/*glFinish ();*/
		time1 = Sys_FloatTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
        c_md3_polys = 0;
	}

	mirror = qfalse;
/*
	if (gl_finish.value)
		glFinish ();*/

	R_Clear ();

	// render normal view
	R_RenderScene ();
/*
	sceGumMatrixMode(GU_VIEW);
	ScePspFMatrix4 view_matrix;
	sceGumStoreMatrix(&view_matrix);

	sceGumLoadIdentity();

	ScePspFVector3 translation =
	{
		view_matrix.x.w, view_matrix.y.w, view_matrix.z.w
	};
	sceGumTranslate(&translation);
	sceGumUpdateMatrix();

	sceGumMatrixMode(GU_MODEL);

	sceGumLoadIdentity();
	sceGumUpdateMatrix();
*/

	R_DrawViewModel ();

	R_DrawWaterSurfaces ();

	// render mirror view
	R_Mirror ();

	R_PolyBlend ();
    //Crow_bar fixed
	if (r_speeds.value)
	{
		time2 = Sys_FloatTime ();
		Con_Printf ("%3i ms\n",(int)((time2-time1)*1000));
        Con_Printf ("%4i wpoly\n",  c_brush_polys);
        Con_Printf ("%4i epoly\n",  c_alias_polys);
        Con_Printf ("%4i md3poly\n",c_md3_polys);
	}
}
