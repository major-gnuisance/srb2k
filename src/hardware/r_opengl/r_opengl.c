// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief OpenGL API for Sonic Robo Blast 2

#if defined (_WIN32)
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif
#undef GETTEXT
#ifdef __GNUC__
#include <unistd.h>
#endif

#include <stdarg.h>
#include <math.h>
#include "r_opengl.h"
#include "r_vbo.h"

#if defined (HWRENDER) && !defined (NOROPENGL)

struct GLRGBAFloat
{
	GLfloat red;
	GLfloat green;
	GLfloat blue;
	GLfloat alpha;
};
typedef struct GLRGBAFloat GLRGBAFloat;
static const GLubyte white[4] = { 255, 255, 255, 255 };

// ==========================================================================
//                                                                  CONSTANTS
// ==========================================================================

// With OpenGL 1.1+, the first texture should be 1
#define NOTEXTURE_NUM     0
#define FIRST_TEX_AVAIL   (NOTEXTURE_NUM + 1)

#define      N_PI_DEMI               (M_PIl/2.0f) //(1.5707963268f)

#define      ASPECT_RATIO            (1.0f)  //(320.0f/200.0f)
#define      FAR_CLIPPING_PLANE      32768.0f // Draw further! Tails 01-21-2001
static float NEAR_CLIPPING_PLANE =   NZCLIP_PLANE;

// **************************************************************************
//                                                                    GLOBALS
// **************************************************************************


static  GLuint      NextTexAvail    = FIRST_TEX_AVAIL;
static  GLuint      tex_downloaded  = 0;
static  GLfloat     fov             = 90.0f;
static  FBITFIELD   CurrentPolyFlags;

static  FTextureInfo*  gr_cachetail = NULL;
static  FTextureInfo*  gr_cachehead = NULL;

RGBA_t  myPaletteData[256];
GLint   screen_width    = 0;               // used by Draw2DLine()
GLint   screen_height   = 0;
GLbyte  screen_depth    = 0;
GLint   textureformatGL = 0;
GLint maximumAnisotropy = 0;
static GLboolean MipMap = GL_FALSE;
static GLint min_filter = GL_LINEAR;
static GLint mag_filter = GL_LINEAR;
static GLint anisotropic_filter = 0;
static FTransform  md2_transform;

const GLubyte *gl_version = NULL;
const GLubyte *gl_renderer = NULL;
const GLubyte *gl_extensions = NULL;

//Hurdler: 04/10/2000: added for the kick ass coronas as Boris wanted;-)
static GLfloat modelMatrix[16];
static GLfloat projMatrix[16];
static GLint viewport[4];

#ifdef USE_PALETTED_TEXTURE
	PFNGLCOLORTABLEEXTPROC  glColorTableEXT = NULL;
	GLubyte                 palette_tex[256*3];
#endif

// Yay for arbitrary  numbers! NextTexAvail is buggy for some reason.
// Sryder:	NextTexAvail is broken for these because palette changes or changes to the texture filter or antialiasing
//			flush all of the stored textures, leaving them unavailable at times such as between levels
//			These need to start at 0 and be set to their number, and be reset to 0 when deleted so that Intel GPUs
//			can know when the textures aren't there, as textures are always considered resident in their virtual memory
// TODO:	Store them in a more normal way
#define SCRTEX_SCREENTEXTURE 4294967295U
#define SCRTEX_STARTSCREENWIPE 4294967294U
#define SCRTEX_ENDSCREENWIPE 4294967293U
#define SCRTEX_FINALSCREENTEXTURE 4294967292U
static GLuint screentexture = 0;
static GLuint startScreenWipe = 0;
static GLuint endScreenWipe = 0;
static GLuint finalScreenTexture = 0;

// shortcut for ((float)1/i)
static const GLfloat byte2float[256] = {
	0.000000f, 0.003922f, 0.007843f, 0.011765f, 0.015686f, 0.019608f, 0.023529f, 0.027451f,
	0.031373f, 0.035294f, 0.039216f, 0.043137f, 0.047059f, 0.050980f, 0.054902f, 0.058824f,
	0.062745f, 0.066667f, 0.070588f, 0.074510f, 0.078431f, 0.082353f, 0.086275f, 0.090196f,
	0.094118f, 0.098039f, 0.101961f, 0.105882f, 0.109804f, 0.113725f, 0.117647f, 0.121569f,
	0.125490f, 0.129412f, 0.133333f, 0.137255f, 0.141176f, 0.145098f, 0.149020f, 0.152941f,
	0.156863f, 0.160784f, 0.164706f, 0.168627f, 0.172549f, 0.176471f, 0.180392f, 0.184314f,
	0.188235f, 0.192157f, 0.196078f, 0.200000f, 0.203922f, 0.207843f, 0.211765f, 0.215686f,
	0.219608f, 0.223529f, 0.227451f, 0.231373f, 0.235294f, 0.239216f, 0.243137f, 0.247059f,
	0.250980f, 0.254902f, 0.258824f, 0.262745f, 0.266667f, 0.270588f, 0.274510f, 0.278431f,
	0.282353f, 0.286275f, 0.290196f, 0.294118f, 0.298039f, 0.301961f, 0.305882f, 0.309804f,
	0.313726f, 0.317647f, 0.321569f, 0.325490f, 0.329412f, 0.333333f, 0.337255f, 0.341176f,
	0.345098f, 0.349020f, 0.352941f, 0.356863f, 0.360784f, 0.364706f, 0.368627f, 0.372549f,
	0.376471f, 0.380392f, 0.384314f, 0.388235f, 0.392157f, 0.396078f, 0.400000f, 0.403922f,
	0.407843f, 0.411765f, 0.415686f, 0.419608f, 0.423529f, 0.427451f, 0.431373f, 0.435294f,
	0.439216f, 0.443137f, 0.447059f, 0.450980f, 0.454902f, 0.458824f, 0.462745f, 0.466667f,
	0.470588f, 0.474510f, 0.478431f, 0.482353f, 0.486275f, 0.490196f, 0.494118f, 0.498039f,
	0.501961f, 0.505882f, 0.509804f, 0.513726f, 0.517647f, 0.521569f, 0.525490f, 0.529412f,
	0.533333f, 0.537255f, 0.541177f, 0.545098f, 0.549020f, 0.552941f, 0.556863f, 0.560784f,
	0.564706f, 0.568627f, 0.572549f, 0.576471f, 0.580392f, 0.584314f, 0.588235f, 0.592157f,
	0.596078f, 0.600000f, 0.603922f, 0.607843f, 0.611765f, 0.615686f, 0.619608f, 0.623529f,
	0.627451f, 0.631373f, 0.635294f, 0.639216f, 0.643137f, 0.647059f, 0.650980f, 0.654902f,
	0.658824f, 0.662745f, 0.666667f, 0.670588f, 0.674510f, 0.678431f, 0.682353f, 0.686275f,
	0.690196f, 0.694118f, 0.698039f, 0.701961f, 0.705882f, 0.709804f, 0.713726f, 0.717647f,
	0.721569f, 0.725490f, 0.729412f, 0.733333f, 0.737255f, 0.741177f, 0.745098f, 0.749020f,
	0.752941f, 0.756863f, 0.760784f, 0.764706f, 0.768627f, 0.772549f, 0.776471f, 0.780392f,
	0.784314f, 0.788235f, 0.792157f, 0.796078f, 0.800000f, 0.803922f, 0.807843f, 0.811765f,
	0.815686f, 0.819608f, 0.823529f, 0.827451f, 0.831373f, 0.835294f, 0.839216f, 0.843137f,
	0.847059f, 0.850980f, 0.854902f, 0.858824f, 0.862745f, 0.866667f, 0.870588f, 0.874510f,
	0.878431f, 0.882353f, 0.886275f, 0.890196f, 0.894118f, 0.898039f, 0.901961f, 0.905882f,
	0.909804f, 0.913726f, 0.917647f, 0.921569f, 0.925490f, 0.929412f, 0.933333f, 0.937255f,
	0.941177f, 0.945098f, 0.949020f, 0.952941f, 0.956863f, 0.960784f, 0.964706f, 0.968628f,
	0.972549f, 0.976471f, 0.980392f, 0.984314f, 0.988235f, 0.992157f, 0.996078f, 1.000000f
};

// -----------------+
// GL_DBG_Printf    : Output debug messages to debug log if DEBUG_TO_FILE is defined,
//                  : else do nothing
// Returns          :
// -----------------+

// dont save ogllog.txt, it crashes on this machine for some reason
//#undef DEBUG_TO_FILE

#ifdef DEBUG_TO_FILE
FILE *gllogstream;
#endif

FUNCPRINTF void GL_DBG_Printf(const char *format, ...)
{
#ifdef DEBUG_TO_FILE
	char str[4096] = "";
	va_list arglist;

	if (!gllogstream)
		gllogstream = fopen("ogllog.txt", "w");

	va_start(arglist, format);
	vsnprintf(str, 4096, format, arglist);
	va_end(arglist);

	fwrite(str, strlen(str), 1, gllogstream);
#else
	(void)format;
#endif
}

#ifdef STATIC_OPENGL
/* 1.0 functions */
/* Miscellaneous */
#define pglClearColor glClearColor
//glClear
#define pglColorMask glColorMask
#define pglAlphaFunc glAlphaFunc
#define pglBlendFunc glBlendFunc
#define pglCullFace glCullFace
#define pglPolygonOffset glPolygonOffset
#define pglScissor glScissor
#define pglEnable glEnable
#define pglDisable glDisable
#define pglGetFloatv glGetFloatv

/* Depth Buffer */
#define pglClearDepth glClearDepth
#define pglDepthFunc glDepthFunc
#define pglDepthMask glDepthMask
#define pglDepthRange glDepthRange

/* Transformation */
#define pglMatrixMode glMatrixMode
#define pglViewport glViewport
#define pglPushMatrix glPushMatrix
#define pglPopMatrix glPopMatrix
#define pglLoadIdentity glLoadIdentity
#define pglMultMatrixd glMultMatrixd
#define pglRotatef glRotatef
#define pglScalef glScalef
#define pglTranslatef glTranslatef

/* Drawing Functions */
#define pglColor4ubv glColor4ubv
#define pglVertexPointer glVertexPointer
#define pglNormalPointer glNormalPointer
#define pglTexCoordPointer glTexCoordPointer
#define pglDrawArrays glDrawArrays
#define pglDrawElements glDrawElements
#define pglEnableClientState glEnableClientState
#define pglDisableClientState glDisableClientState
#define pglClientActiveTexture glClientActiveTexture
#define pglGenBuffers glGenBuffers
#define pglBindBuffer glBindBuffer
#define pglBufferData glBufferData
#define pglDeleteBuffers glDeleteBuffers

/* Lighting */
#define pglShadeModel glShadeModel
#define pglLightfv glLightfv
#define pglLightModelfv glLightModelfv
#define pglMaterialfv glMaterialfv
#define pglMateriali glMateriali

/* Raster functions */
#define pglPixelStorei glPixelStorei
#define pglReadPixels glReadPixels

/* Texture mapping */
#define pglTexEnvi glTexEnvi
#define pglTexParameteri glTexParameteri
#define pglTexImage2D glTexImage2D

/* Fog */
#define pglFogf glFogf
#define pglFogfv glFogfv

/* 1.1 functions */
/* texture objects */ //GL_EXT_texture_object
#define pglGenTextures glGenTextures
#define pglDeleteTextures glDeleteTextures
#define pglBindTexture glBindTexture
/* texture mapping */ //GL_EXT_copy_texture
#define pglCopyTexImage2D glCopyTexImage2D
#define pglCopyTexSubImage2D glCopyTexSubImage2D

#else //!STATIC_OPENGL

/* 1.0 functions */
/* Miscellaneous */
typedef void (APIENTRY * PFNglClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static PFNglClearColor pglClearColor;
typedef void (APIENTRY * PFNglColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static PFNglColorMask pglColorMask;
typedef void (APIENTRY * PFNglAlphaFunc) (GLenum func, GLclampf ref);
static PFNglAlphaFunc pglAlphaFunc;
typedef void (APIENTRY * PFNglBlendFunc) (GLenum sfactor, GLenum dfactor);
static PFNglBlendFunc pglBlendFunc;
typedef void (APIENTRY * PFNglCullFace) (GLenum mode);
static PFNglCullFace pglCullFace;
typedef void (APIENTRY * PFNglPolygonOffset) (GLfloat factor, GLfloat units);
static PFNglPolygonOffset pglPolygonOffset;
typedef void (APIENTRY * PFNglScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
static PFNglScissor pglScissor;
typedef void (APIENTRY * PFNglEnable) (GLenum cap);
static PFNglEnable pglEnable;
typedef void (APIENTRY * PFNglDisable) (GLenum cap);
static PFNglDisable pglDisable;
typedef void (APIENTRY * PFNglGetFloatv) (GLenum pname, GLfloat *params);
static PFNglGetFloatv pglGetFloatv;

/* Depth Buffer */
typedef void (APIENTRY * PFNglClearDepth) (GLclampd depth);
static PFNglClearDepth pglClearDepth;
typedef void (APIENTRY * PFNglDepthFunc) (GLenum func);
static PFNglDepthFunc pglDepthFunc;
typedef void (APIENTRY * PFNglDepthMask) (GLboolean flag);
static PFNglDepthMask pglDepthMask;
typedef void (APIENTRY * PFNglDepthRange) (GLclampd near_val, GLclampd far_val);
static PFNglDepthRange pglDepthRange;

/* Transformation */
typedef void (APIENTRY * PFNglMatrixMode) (GLenum mode);
static PFNglMatrixMode pglMatrixMode;
typedef void (APIENTRY * PFNglViewport) (GLint x, GLint y, GLsizei width, GLsizei height);
static PFNglViewport pglViewport;
typedef void (APIENTRY * PFNglPushMatrix) (void);
static PFNglPushMatrix pglPushMatrix;
typedef void (APIENTRY * PFNglPopMatrix) (void);
static PFNglPopMatrix pglPopMatrix;
typedef void (APIENTRY * PFNglLoadIdentity) (void);
static PFNglLoadIdentity pglLoadIdentity;
typedef void (APIENTRY * PFNglMultMatrixf) (const GLfloat *m);
static PFNglMultMatrixf pglMultMatrixf;
typedef void (APIENTRY * PFNglRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static PFNglRotatef pglRotatef;
typedef void (APIENTRY * PFNglScalef) (GLfloat x, GLfloat y, GLfloat z);
static PFNglScalef pglScalef;
typedef void (APIENTRY * PFNglTranslatef) (GLfloat x, GLfloat y, GLfloat z);
static PFNglTranslatef pglTranslatef;

/* Drawing Functions */
typedef void (APIENTRY * PFNglColor4ubv) (const GLubyte *v);
static PFNglColor4ubv pglColor4ubv;
typedef void (APIENTRY * PFNglVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static PFNglVertexPointer pglVertexPointer;
typedef void (APIENTRY * PFNglNormalPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
static PFNglNormalPointer pglNormalPointer;
typedef void (APIENTRY * PFNglTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static PFNglTexCoordPointer pglTexCoordPointer;
typedef void (APIENTRY * PFNglDrawArrays) (GLenum mode, GLint first, GLsizei count);
static PFNglDrawArrays pglDrawArrays;
typedef void (APIENTRY * PFNglDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static PFNglDrawElements pglDrawElements;
typedef void (APIENTRY * PFNglEnableClientState) (GLenum cap);
static PFNglEnableClientState pglEnableClientState;
typedef void (APIENTRY * PFNglDisableClientState) (GLenum cap);
static PFNglDisableClientState pglDisableClientState;
typedef void (APIENTRY * PFNglGenBuffers) (GLsizei n, GLuint *buffers);
static PFNglGenBuffers pglGenBuffers;
typedef void (APIENTRY * PFNglBindBuffer) (GLenum target, GLuint buffer);
static PFNglBindBuffer pglBindBuffer;
typedef void (APIENTRY * PFNglBufferData) (GLenum target, GLsizei size, const GLvoid *data, GLenum usage);
static PFNglBufferData pglBufferData;
typedef void (APIENTRY * PFNglDeleteBuffers) (GLsizei n, const GLuint *buffers);
static PFNglDeleteBuffers pglDeleteBuffers;

/* Lighting */
typedef void (APIENTRY * PFNglShadeModel) (GLenum mode);
static PFNglShadeModel pglShadeModel;
typedef void (APIENTRY * PFNglLightfv) (GLenum light, GLenum pname, GLfloat *params);
static PFNglLightfv pglLightfv;
typedef void (APIENTRY * PFNglLightModelfv) (GLenum pname, GLfloat *params);
static PFNglLightModelfv pglLightModelfv;
typedef void (APIENTRY * PFNglMaterialfv) (GLint face, GLenum pname, GLfloat *params);
static PFNglMaterialfv pglMaterialfv;
typedef void (APIENTRY * PFNglMateriali) (GLint face, GLenum pname, GLint param);
static PFNglMateriali pglMateriali;

/* Raster functions */
typedef void (APIENTRY * PFNglPixelStorei) (GLenum pname, GLint param);
static PFNglPixelStorei pglPixelStorei;
typedef void (APIENTRY  * PFNglReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static PFNglReadPixels pglReadPixels;

/* Texture mapping */
typedef void (APIENTRY * PFNglTexEnvi) (GLenum target, GLenum pname, GLint param);
static PFNglTexEnvi pglTexEnvi;
typedef void (APIENTRY * PFNglTexParameteri) (GLenum target, GLenum pname, GLint param);
static PFNglTexParameteri pglTexParameteri;
typedef void (APIENTRY * PFNglTexImage2D) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static PFNglTexImage2D pglTexImage2D;

/* Fog */
typedef void (APIENTRY * PFNglFogf) (GLenum pname, GLfloat param);
static PFNglFogf pglFogf;
typedef void (APIENTRY * PFNglFogfv) (GLenum pname, const GLfloat *params);
static PFNglFogfv pglFogfv;

/* 1.1 functions */
/* texture objects */ //GL_EXT_texture_object
typedef void (APIENTRY * PFNglGenTextures) (GLsizei n, const GLuint *textures);
static PFNglGenTextures pglGenTextures;
typedef void (APIENTRY * PFNglDeleteTextures) (GLsizei n, const GLuint *textures);
static PFNglDeleteTextures pglDeleteTextures;
typedef void (APIENTRY * PFNglBindTexture) (GLenum target, GLuint texture);
static PFNglBindTexture pglBindTexture;
/* texture mapping */ //GL_EXT_copy_texture
typedef void (APIENTRY * PFNglCopyTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
static PFNglCopyTexImage2D pglCopyTexImage2D;
typedef void (APIENTRY * PFNglCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static PFNglCopyTexSubImage2D pglCopyTexSubImage2D;
#endif
/* GLU functions */
typedef GLint (APIENTRY * PFNgluBuild2DMipmaps) (GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data);
static PFNgluBuild2DMipmaps pgluBuild2DMipmaps;

/* 1.3 functions for multitexturing */
typedef void (APIENTRY *PFNglActiveTexture) (GLenum);
static PFNglActiveTexture pglActiveTexture;
typedef void (APIENTRY *PFNglMultiTexCoord2f) (GLenum, GLfloat, GLfloat);
static PFNglMultiTexCoord2f pglMultiTexCoord2f;
typedef void (APIENTRY *PFNglMultiTexCoord2fv) (GLenum target, const GLfloat *v);
static PFNglMultiTexCoord2fv pglMultiTexCoord2fv;
typedef void (APIENTRY *PFNglClientActiveTexture) (GLenum);
static PFNglClientActiveTexture pglClientActiveTexture;

/* 1.2 Parms */
/* GL_CLAMP_TO_EDGE_EXT */
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_MIN_LOD
#define GL_TEXTURE_MIN_LOD 0x813A
#endif
#ifndef GL_TEXTURE_MAX_LOD
#define GL_TEXTURE_MAX_LOD 0x813B
#endif

/* 1.3 GL_TEXTUREi */
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif

boolean SetupGLfunc(void)
{
#ifndef STATIC_OPENGL
#define GETOPENGLFUNC(func, proc) \
	func = GetGLFunc(#proc); \
	if (!func) \
	{ \
		GL_DBG_Printf("failed to get OpenGL function: %s\n", #proc); \
	} \

	GETOPENGLFUNC(pglClearColor, glClearColor)

	GETOPENGLFUNC(pglClear, glClear)
	GETOPENGLFUNC(pglColorMask, glColorMask)
	GETOPENGLFUNC(pglAlphaFunc, glAlphaFunc)
	GETOPENGLFUNC(pglBlendFunc, glBlendFunc)
	GETOPENGLFUNC(pglCullFace, glCullFace)
	GETOPENGLFUNC(pglPolygonOffset, glPolygonOffset)
	GETOPENGLFUNC(pglScissor, glScissor)
	GETOPENGLFUNC(pglEnable, glEnable)
	GETOPENGLFUNC(pglDisable, glDisable)
	GETOPENGLFUNC(pglGetFloatv, glGetFloatv)
	GETOPENGLFUNC(pglGetIntegerv, glGetIntegerv)
	GETOPENGLFUNC(pglGetString, glGetString)

	GETOPENGLFUNC(pglClearDepth, glClearDepth)
	GETOPENGLFUNC(pglDepthFunc, glDepthFunc)
	GETOPENGLFUNC(pglDepthMask, glDepthMask)
	GETOPENGLFUNC(pglDepthRange, glDepthRange)

	GETOPENGLFUNC(pglMatrixMode, glMatrixMode)
	GETOPENGLFUNC(pglViewport, glViewport)
	GETOPENGLFUNC(pglPushMatrix, glPushMatrix)
	GETOPENGLFUNC(pglPopMatrix, glPopMatrix)
	GETOPENGLFUNC(pglLoadIdentity, glLoadIdentity)
	GETOPENGLFUNC(pglMultMatrixf, glMultMatrixf)
	GETOPENGLFUNC(pglRotatef, glRotatef)
	GETOPENGLFUNC(pglScalef, glScalef)
	GETOPENGLFUNC(pglTranslatef, glTranslatef)

	GETOPENGLFUNC(pglColor4ubv, glColor4ubv)

	GETOPENGLFUNC(pglVertexPointer, glVertexPointer)
	GETOPENGLFUNC(pglNormalPointer, glNormalPointer)
	GETOPENGLFUNC(pglTexCoordPointer, glTexCoordPointer)
	GETOPENGLFUNC(pglDrawArrays, glDrawArrays)
	GETOPENGLFUNC(pglDrawElements, glDrawElements)
	GETOPENGLFUNC(pglEnableClientState, glEnableClientState)
	GETOPENGLFUNC(pglDisableClientState, glDisableClientState)

	GETOPENGLFUNC(pglShadeModel, glShadeModel)
	GETOPENGLFUNC(pglLightfv, glLightfv)
	GETOPENGLFUNC(pglLightModelfv, glLightModelfv)
	GETOPENGLFUNC(pglMaterialfv, glMaterialfv)
	GETOPENGLFUNC(pglMateriali, glMateriali)

	GETOPENGLFUNC(pglPixelStorei, glPixelStorei)
	GETOPENGLFUNC(pglReadPixels, glReadPixels)

	GETOPENGLFUNC(pglTexEnvi, glTexEnvi)
	GETOPENGLFUNC(pglTexParameteri, glTexParameteri)
	GETOPENGLFUNC(pglTexImage2D, glTexImage2D)

	GETOPENGLFUNC(pglFogf, glFogf)
	GETOPENGLFUNC(pglFogfv, glFogfv)

	GETOPENGLFUNC(pglGenTextures, glGenTextures)
	GETOPENGLFUNC(pglDeleteTextures, glDeleteTextures)
	GETOPENGLFUNC(pglBindTexture, glBindTexture)

	GETOPENGLFUNC(pglCopyTexImage2D, glCopyTexImage2D)
	GETOPENGLFUNC(pglCopyTexSubImage2D, glCopyTexSubImage2D)

#undef GETOPENGLFUNC
#endif
	return true;
}

static INT32 glstate_fog_mode = 0;
static float glstate_fog_density = 0;

INT32 gl_leveltime = 0;

// for wireframe mode, not sure where and how and what and why to put this
typedef void    (APIENTRY *PFNglPolygonMode)                (GLenum, GLenum);
static PFNglPolygonMode pglPolygonMode;

#ifdef GL_SHADERS
typedef GLuint 	(APIENTRY *PFNglCreateShader)		(GLenum);
typedef void 	(APIENTRY *PFNglShaderSource)		(GLuint, GLsizei, const GLchar**, GLint*);
typedef void 	(APIENTRY *PFNglCompileShader)		(GLuint);
typedef void 	(APIENTRY *PFNglGetShaderiv)		(GLuint, GLenum, GLint*);
typedef void 	(APIENTRY *PFNglGetShaderInfoLog)	(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void 	(APIENTRY *PFNglDeleteShader)		(GLuint);
typedef GLuint 	(APIENTRY *PFNglCreateProgram)		(void);
typedef void 	(APIENTRY *PFNglAttachShader)		(GLuint, GLuint);
typedef void 	(APIENTRY *PFNglLinkProgram)		(GLuint);
typedef void 	(APIENTRY *PFNglGetProgramiv)		(GLuint, GLenum, GLint*);
typedef void 	(APIENTRY *PFNglUseProgram)			(GLuint);
typedef void 	(APIENTRY *PFNglUniform1i)			(GLint, GLint);
typedef void 	(APIENTRY *PFNglUniform1f)			(GLint, GLfloat);
typedef void 	(APIENTRY *PFNglUniform2f)			(GLint, GLfloat, GLfloat);
typedef void 	(APIENTRY *PFNglUniform3f)			(GLint, GLfloat, GLfloat, GLfloat);
typedef void 	(APIENTRY *PFNglUniform4f)			(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void 	(APIENTRY *PFNglUniform1fv)			(GLint, GLsizei, const GLfloat*);
typedef void 	(APIENTRY *PFNglUniform2fv)			(GLint, GLsizei, const GLfloat*);
typedef void 	(APIENTRY *PFNglUniform3fv)			(GLint, GLsizei, const GLfloat*);
typedef GLint 	(APIENTRY *PFNglGetUniformLocation)	(GLuint, const GLchar*);

static PFNglCreateShader pglCreateShader;
static PFNglShaderSource pglShaderSource;
static PFNglCompileShader pglCompileShader;
static PFNglGetShaderiv pglGetShaderiv;
static PFNglGetShaderInfoLog pglGetShaderInfoLog;
static PFNglDeleteShader pglDeleteShader;
static PFNglCreateProgram pglCreateProgram;
static PFNglAttachShader pglAttachShader;
static PFNglLinkProgram pglLinkProgram;
static PFNglGetProgramiv pglGetProgramiv;
static PFNglUseProgram pglUseProgram;
static PFNglUniform1i pglUniform1i;
static PFNglUniform1f pglUniform1f;
static PFNglUniform2f pglUniform2f;
static PFNglUniform3f pglUniform3f;
static PFNglUniform4f pglUniform4f;
static PFNglUniform1fv pglUniform1fv;
static PFNglUniform2fv pglUniform2fv;
static PFNglUniform3fv pglUniform3fv;
static PFNglGetUniformLocation pglGetUniformLocation;

#define MAXSHADERS 16
#define MAXSHADERPROGRAMS 16

// 18032019
static char *gl_customvertexshaders[MAXSHADERS];
static char *gl_customfragmentshaders[MAXSHADERS];

static boolean gl_allowshaders = false;
static boolean gl_shadersenabled = false;
static GLuint gl_currentshaderprogram = 0;

// trying to improve performance by removing redundant glUseProgram calls
static boolean gl_shaderprogramchanged = true;

// adding some test stuff here
// this var makes DrawPolygon do nothing, purpose is to study opengl calls' effect on performance
static INT32 gl_test_disable_something = 0;

static boolean gl_batching = false;// are we currently collecting batches?

// 13062019
typedef enum
{
	// lighting
	gluniform_mix_color,
	gluniform_fade_color,
	gluniform_lighting,

	// fog
	gluniform_fog_mode,
	gluniform_fog_density,

	// misc. (custom shaders)
	gluniform_leveltime,

	gluniform_max,
} gluniform_t;

typedef struct gl_shaderprogram_s
{
	GLuint program;
	boolean custom;
	GLint uniforms[gluniform_max+1];
} gl_shaderprogram_t;
static gl_shaderprogram_t gl_shaderprograms[MAXSHADERPROGRAMS];

// ========================
//  Fragment shader macros
// ========================

//
// GLSL Software fragment shader
//

#define GLSL_INTERNAL_FOG_FUNCTION \
	"float fog(const float dist, const float density,  const float globaldensity) {\n" \
		"const float LOG2 = -1.442695;\n" \
		"float d = density * dist;\n" \
		"return 1.0 - clamp(exp2(d * sqrt(d) * globaldensity * LOG2), 0.0, 1.0);\n" \
	"}\n"

// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_gpu_shader_fp64.txt
#define GLSL_INTERNAL_FOG_MIX \
	"float fog_distance = gl_FragCoord.z / gl_FragCoord.w;\n" \
	"float fog_attenuation = floor(fog(fog_distance, 0.0001 * ((256.0-lighting)/24.0), fog_density)*10.0)/10.0;\n" \
	"vec4 fog_color = vec4(fade_color[0], fade_color[1], fade_color[2], 1.0);\n" \
	"vec4 mixed_color = texel * mix_color;\n" \
	"vec4 fog_mix = mix(mixed_color, fog_color, fog_attenuation);\n" \
	"vec4 final_color = mix(fog_mix, fog_color, ((256.0-lighting)/256.0));\n" \
	"final_color[3] = mixed_color[3];\n"

#define GLSL_SOFTWARE_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 mix_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform int fog_mode;\n" \
	"uniform float fog_density;\n" \
	GLSL_INTERNAL_FOG_FUNCTION \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		GLSL_INTERNAL_FOG_MIX \
		"gl_FragColor = final_color;\n" \
	"}\0"

//
// GLSL generic fragment shader
//

#define GLSL_DEFAULT_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 mix_color;\n" \
	"void main(void) {\n" \
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st) * mix_color;\n" \
	"}\0"

static const char *fragment_shaders[] = {
	// Default fragment shader
	GLSL_DEFAULT_FRAGMENT_SHADER,

	// Floor fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Wall fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Sprite fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Model fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Water fragment shader
	GLSL_SOFTWARE_FRAGMENT_SHADER,

	// Fog fragment shader
	"void main(void) {\n"
		"gl_FragColor = gl_Color;\n"
	"}\0",

	// Sky fragment shader
	"uniform sampler2D tex;\n"
	"void main(void) {\n"
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n" \
	"}\0",

	NULL,
};

// ======================
//  Vertex shader macros
// ======================

//
// GLSL generic vertex shader
//

#define GLSL_DEFAULT_VERTEX_SHADER \
	"void main()\n" \
	"{\n" \
		"gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n" \
		"gl_FrontColor = gl_Color;\n" \
		"gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;\n" \
		"gl_ClipVertex = gl_ModelViewMatrix*gl_Vertex;\n" \
	"}\0"

static const char *vertex_shaders[] = {
	// Default vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Floor vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Wall vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Sprite vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Model vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Water vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Fog vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	// Sky vertex shader
	GLSL_DEFAULT_VERTEX_SHADER,

	NULL,
};

#endif	// GL_SHADERS

void SetupGLFunc4(void)
{
	pglActiveTexture = GetGLFunc("glActiveTexture");
	pglMultiTexCoord2f = GetGLFunc("glMultiTexCoord2f");
	pglClientActiveTexture = GetGLFunc("glClientActiveTexture");
	pglMultiTexCoord2fv = GetGLFunc("glMultiTexCoord2fv");
	pglGenBuffers = GetGLFunc("glGenBuffers");
	pglBindBuffer = GetGLFunc("glBindBuffer");
	pglBufferData = GetGLFunc("glBufferData");
	pglDeleteBuffers = GetGLFunc("glDeleteBuffers");

	// wireframe
	pglPolygonMode = GetGLFunc("glPolygonMode");

#ifdef GL_SHADERS
	pglCreateShader = GetGLFunc("glCreateShader");
	pglShaderSource = GetGLFunc("glShaderSource");
	pglCompileShader = GetGLFunc("glCompileShader");
	pglGetShaderiv = GetGLFunc("glGetShaderiv");
	pglGetShaderInfoLog = GetGLFunc("glGetShaderInfoLog");
	pglDeleteShader = GetGLFunc("glDeleteShader");
	pglCreateProgram = GetGLFunc("glCreateProgram");
	pglAttachShader = GetGLFunc("glAttachShader");
	pglLinkProgram = GetGLFunc("glLinkProgram");
	pglGetProgramiv = GetGLFunc("glGetProgramiv");
	pglUseProgram = GetGLFunc("glUseProgram");
	pglUniform1i = GetGLFunc("glUniform1i");
	pglUniform1f = GetGLFunc("glUniform1f");
	pglUniform2f = GetGLFunc("glUniform2f");
	pglUniform3f = GetGLFunc("glUniform3f");
	pglUniform4f = GetGLFunc("glUniform4f");
	pglUniform1fv = GetGLFunc("glUniform1fv");
	pglUniform2fv = GetGLFunc("glUniform2fv");
	pglUniform3fv = GetGLFunc("glUniform3fv");
	pglGetUniformLocation = GetGLFunc("glGetUniformLocation");
#endif

	// GLU
	pgluBuild2DMipmaps = GetGLFunc("gluBuild2DMipmaps");
}

// jimita
EXPORT void HWRAPI(LoadShaders) (void)
{
#ifdef GL_SHADERS
	GLuint gl_vertShader, gl_fragShader;
	GLint i, result;

	gl_customvertexshaders[0] = NULL;
	gl_customfragmentshaders[0] = NULL;

	for (i = 0; vertex_shaders[i] && fragment_shaders[i]; i++)
	{
		gl_shaderprogram_t *shader;
		const GLchar* vert_shader = vertex_shaders[i];
		const GLchar* frag_shader = fragment_shaders[i];
		boolean custom = ((gl_customvertexshaders[i] || gl_customfragmentshaders[i]) && (i > 0));

		// 18032019
		if (gl_customvertexshaders[i])
			vert_shader = gl_customvertexshaders[i];
		if (gl_customfragmentshaders[i])
			frag_shader = gl_customfragmentshaders[i];

		if (i >= MAXSHADERS)
			break;
		if (i >= MAXSHADERPROGRAMS)
			break;

		//
		// Load and compile vertex shader
		//
		gl_vertShader = pglCreateShader(GL_VERTEX_SHADER);
		if (!gl_vertShader)
			I_Error("Hardware driver: Error creating vertex shader %d", i);

		pglShaderSource(gl_vertShader, 1, &vert_shader, NULL);
		pglCompileShader(gl_vertShader);

		// check for compile errors
		pglGetShaderiv(gl_vertShader, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			GLchar* infoLog;
			GLint logLength;

			pglGetShaderiv(gl_vertShader, GL_INFO_LOG_LENGTH, &logLength);

			infoLog = malloc(logLength);
			pglGetShaderInfoLog(gl_vertShader, logLength, NULL, infoLog);

			I_Error("Hardware driver: Error compiling vertex shader %d\n%s", i, infoLog);
		}

		//
		// Load and compile fragment shader
		//
		gl_fragShader = pglCreateShader(GL_FRAGMENT_SHADER);
		if (!gl_fragShader)
			I_Error("Hardware driver: Error creating fragment shader %d", i);

		pglShaderSource(gl_fragShader, 1, &frag_shader, NULL);
		pglCompileShader(gl_fragShader);

		// check for compile errors
		pglGetShaderiv(gl_fragShader, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			GLchar* infoLog;
			GLint logLength;

			pglGetShaderiv(gl_fragShader, GL_INFO_LOG_LENGTH, &logLength);

			infoLog = malloc(logLength);
			pglGetShaderInfoLog(gl_fragShader, logLength, NULL, infoLog);

			I_Error("Hardware driver: Error compiling fragment shader %d\n%s", i, infoLog);
		}

		shader = &gl_shaderprograms[i];
		shader->program = pglCreateProgram();
		shader->custom = custom;
		pglAttachShader(shader->program, gl_vertShader);
		pglAttachShader(shader->program, gl_fragShader);
		pglLinkProgram(shader->program);

		// check link status
		pglGetProgramiv(shader->program, GL_LINK_STATUS, &result);
		if (result != GL_TRUE)
			I_Error("Hardware driver: Error linking shader program %d", i);

		// delete the shader objects
		pglDeleteShader(gl_vertShader);
		pglDeleteShader(gl_fragShader);

		// 13062019
#define GETUNI(uniform) pglGetUniformLocation(shader->program, uniform);

		// lighting
		shader->uniforms[gluniform_mix_color] = GETUNI("mix_color");
		shader->uniforms[gluniform_fade_color] = GETUNI("fade_color");
		shader->uniforms[gluniform_lighting] = GETUNI("lighting");

		// fog
		shader->uniforms[gluniform_fog_mode] = GETUNI("fog_mode");
		shader->uniforms[gluniform_fog_density] = GETUNI("fog_density");

		// misc. (custom shaders)
		shader->uniforms[gluniform_leveltime] = GETUNI("leveltime");

#undef GETUNI
	}
#endif
}

EXPORT void HWRAPI(LoadCustomShader) (int number, char *shader, size_t size, boolean fragment)
{
#ifdef GL_SHADERS
	if (number < 1 || number > MAXSHADERS)
		I_Error("LoadCustomShader(): cannot load shader %d (max %d)", number, MAXSHADERS);

	if (fragment)
	{
		gl_customfragmentshaders[number] = malloc(size+1);
		strncpy(gl_customfragmentshaders[number], shader, size);
		gl_customfragmentshaders[number][size] = 0;
	}
	else
	{
		gl_customvertexshaders[number] = malloc(size+1);
		strncpy(gl_customvertexshaders[number], shader, size);
		gl_customvertexshaders[number][size] = 0;
	}
#endif
}

EXPORT void HWRAPI(InitCustomShaders) (void)
{
#ifdef GL_SHADERS
	KillShaders();
	LoadShaders();
#endif
}

EXPORT void HWRAPI(SetShader) (int shader)
{
#ifdef GL_SHADERS
	if (gl_allowshaders)
	{
		gl_shadersenabled = true;
		if (shader != gl_currentshaderprogram)
		{
			gl_currentshaderprogram = shader;
			gl_shaderprogramchanged = true;
		}
	}
	else
#endif
		gl_shadersenabled = false;
}

EXPORT void HWRAPI(UnSetShader) (void)
{
#ifdef GL_SHADERS
	gl_shadersenabled = false;
	gl_currentshaderprogram = 0;
	gl_shaderprogramchanged = true;// not sure if this is needed
	pglUseProgram(0);
#endif
}

EXPORT void HWRAPI(KillShaders) (void)
{
	// unused.........................
}

// -----------------+
// SetNoTexture     : Disable texture
// -----------------+
static void SetNoTexture(void)
{
	// Disable texture.
	if (tex_downloaded != NOTEXTURE_NUM && !gl_batching)
	{
		pglBindTexture(GL_TEXTURE_2D, NOTEXTURE_NUM);
		tex_downloaded = NOTEXTURE_NUM;
	}
}

static void GLPerspective(GLfloat fovy, GLfloat aspect)
{
	GLfloat m[4][4] =
	{
		{ 1.0f, 0.0f, 0.0f, 0.0f},
		{ 0.0f, 1.0f, 0.0f, 0.0f},
		{ 0.0f, 0.0f, 1.0f,-1.0f},
		{ 0.0f, 0.0f, 0.0f, 0.0f},
	};
	const GLfloat zNear = NEAR_CLIPPING_PLANE;
	const GLfloat zFar = FAR_CLIPPING_PLANE;
	const GLfloat radians = (GLfloat)(fovy / 2.0f * M_PIl / 180.0f);
	const GLfloat sine = sin(radians);
	const GLfloat deltaZ = zFar - zNear;
	GLfloat cotangent;

	if ((fabsf((float)deltaZ) < 1.0E-36f) || fpclassify(sine) == FP_ZERO || fpclassify(aspect) == FP_ZERO)
	{
		return;
	}
	cotangent = cosf(radians) / sine;

	m[0][0] = cotangent / aspect;
	m[1][1] = cotangent;
	m[2][2] = -(zFar + zNear) / deltaZ;
	m[3][2] = -2.0f * zNear * zFar / deltaZ;
	pglMultMatrixf(&m[0][0]);
}

// -----------------+
// SetModelView     :
// -----------------+
void SetModelView(GLint w, GLint h)
{
	//GL_DBG_Printf("SetModelView(): %dx%d\n", (int)w, (int)h);

	// The screen textures need to be flushed if the width or height change so that they be remade for the correct size
	if (screen_width != w || screen_height != h)
		FlushScreenTextures();

	screen_width = w;
	screen_height = h;

	pglViewport(0, 0, w, h);
#ifdef GL_ACCUM_BUFFER_BIT
	pglClear(GL_ACCUM_BUFFER_BIT);
#endif

	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();

	pglMatrixMode(GL_MODELVIEW);
	pglLoadIdentity();

	GLPerspective(fov, ASPECT_RATIO);

	// added for new coronas' code (without depth buffer)
	pglGetIntegerv(GL_VIEWPORT, viewport);
	pglGetFloatv(GL_PROJECTION_MATRIX, projMatrix);
}

// -----------------+
// SetStates        : Set permanent states
// -----------------+
void SetStates(void)
{
	pglEnable(GL_TEXTURE_2D);      // two-dimensional texturing
	pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	pglAlphaFunc(GL_NOTEQUAL, 0.0f);
	pglEnable(GL_BLEND);           // enable color blending

	pglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	pglEnable(GL_DEPTH_TEST);    // check the depth buffer
	pglDepthMask(GL_TRUE);             // enable writing to depth buffer
	pglClearDepth(1.0f);
	pglDepthRange(0.0f, 1.0f);
	pglDepthFunc(GL_LEQUAL);

	// this set CurrentPolyFlags to the actual configuration
	CurrentPolyFlags = 0xffffffff;
	SetBlend(0);

	tex_downloaded = (GLuint)-1;
	SetNoTexture();

	pglPolygonOffset(-1.0f, -1.0f);

	// bp : when no t&l :)
	pglLoadIdentity();
	pglScalef(1.0f, 1.0f, -1.0f);
	pglGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix); // added for new coronas' code (without depth buffer)
}

// -----------------+
// Flush            : flush OpenGL textures
//                  : Clear list of downloaded mipmaps
// -----------------+
void Flush(void)
{
	//GL_DBG_Printf("HWR_Flush()\n");

	while (gr_cachehead)
	{
		// ceci n'est pas du tout necessaire vu que tu les a charger normalement et
		// donc il sont dans ta liste !
		pglDeleteTextures(1, (GLuint *)&gr_cachehead->downloaded);
		gr_cachehead->downloaded = 0;
		gr_cachehead = gr_cachehead->nextmipmap;
	}
	gr_cachetail = gr_cachehead = NULL; //Hurdler: well, gr_cachehead is already NULL
	NextTexAvail = FIRST_TEX_AVAIL;

	tex_downloaded = 0;
}


// -----------------+
// isExtAvailable   : Look if an OpenGL extension is available
// Returns          : true if extension available
// -----------------+
INT32 isExtAvailable(const char *extension, const GLubyte *start)
{
	GLubyte         *where, *terminator;

	if (!extension || !start) return 0;
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return 1;
		start = terminator;
	}
	return 0;
}


// -----------------+
// Init             : Initialise the OpenGL interface API
// Returns          :
// -----------------+
EXPORT boolean HWRAPI(Init) (void)
{
	return LoadGL();
}


// -----------------+
// ClearMipMapCache : Flush OpenGL textures from memory
// -----------------+
EXPORT void HWRAPI(ClearMipMapCache) (void)
{
	Flush();
}


// -----------------+
// ReadRect         : Read a rectangle region of the truecolor framebuffer
//                  : store pixels as 16bit 565 RGB
// Returns          : 16bit 565 RGB pixel array stored in dst_data
// -----------------+
EXPORT void HWRAPI(ReadRect) (INT32 x, INT32 y, INT32 width, INT32 height,
                                INT32 dst_stride, UINT16 * dst_data)
{
	INT32 i;
	//GL_DBG_Printf("ReadRect()\n");
	if (dst_stride == width*3)
	{
		GLubyte*top = (GLvoid*)dst_data, *bottom = top + dst_stride * (height - 1);
		GLubyte *row = malloc(dst_stride);
		if (!row) return;
		pglPixelStorei(GL_PACK_ALIGNMENT, 1);
		pglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, dst_data);
		pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		for(i = 0; i < height/2; i++)
		{
			memcpy(row, top, dst_stride);
			memcpy(top, bottom, dst_stride);
			memcpy(bottom, row, dst_stride);
			top += dst_stride;
			bottom -= dst_stride;
		}
		free(row);
	}
	else
	{
		INT32 j;
		GLubyte *image = malloc(width*height*3*sizeof (*image));
		if (!image) return;
		pglPixelStorei(GL_PACK_ALIGNMENT, 1);
		pglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
		pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		for (i = height-1; i >= 0; i--)
		{
			for (j = 0; j < width; j++)
			{
				dst_data[(height-1-i)*width+j] =
				(UINT16)(
				                 ((image[(i*width+j)*3]>>3)<<11) |
				                 ((image[(i*width+j)*3+1]>>2)<<5) |
				                 ((image[(i*width+j)*3+2]>>3)));
			}
		}
		free(image);
	}
}


// -----------------+
// GClipRect        : Defines the 2D hardware clipping window
// -----------------+
EXPORT void HWRAPI(GClipRect) (INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip)
{
	//GL_DBG_Printf("GClipRect(%d, %d, %d, %d)\n", minx, miny, maxx, maxy);

	pglViewport(minx, screen_height-maxy, maxx-minx, maxy-miny);
	NEAR_CLIPPING_PLANE = nearclip;

	//pglScissor(minx, screen_height-maxy, maxx-minx, maxy-miny);
	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();
	GLPerspective(fov, ASPECT_RATIO);
	pglMatrixMode(GL_MODELVIEW);

	// added for new coronas' code (without depth buffer)
	pglGetIntegerv(GL_VIEWPORT, viewport);
	pglGetFloatv(GL_PROJECTION_MATRIX, projMatrix);
}


// -----------------+
// ClearBuffer      : Clear the color/alpha/depth buffer(s)
// -----------------+
EXPORT void HWRAPI(ClearBuffer) (FBOOLEAN ColorMask,
                                    FBOOLEAN DepthMask,
                                    FRGBAFloat * ClearColor)
{
	//GL_DBG_Printf("ClearBuffer(%d)\n", alpha);
	GLbitfield ClearMask = 0;

	if (ColorMask)
	{
		if (ClearColor)
			pglClearColor(ClearColor->red,
			              ClearColor->green,
			              ClearColor->blue,
			              ClearColor->alpha);
		ClearMask |= GL_COLOR_BUFFER_BIT;
	}
	if (DepthMask)
	{
		pglClearDepth(1.0f);     //Hurdler: all that are permanen states
		pglDepthRange(0.0f, 1.0f);
		pglDepthFunc(GL_LEQUAL);
		ClearMask |= GL_DEPTH_BUFFER_BIT;
	}

	SetBlend(DepthMask ? PF_Occlude | CurrentPolyFlags : CurrentPolyFlags&~PF_Occlude);

	pglClear(ClearMask);
	pglEnableClientState(GL_VERTEX_ARRAY); // We always use this one
	pglEnableClientState(GL_TEXTURE_COORD_ARRAY); // And mostly this one, too
}


// -----------------+
// HWRAPI Draw2DLine: Render a 2D line
// -----------------+
EXPORT void HWRAPI(Draw2DLine) (F2DCoord * v1,
                                   F2DCoord * v2,
                                   RGBA_t Color)
{
	//GL_DBG_Printf("DrawLine(): %f %f, %f %f\n", v1->x, v1->y, v2->x, v2->y);
	GLfloat p[12];
	GLfloat dx, dy;
	GLfloat angle;

	// BP: we should reflect the new state in our variable
	//SetBlend(PF_Modulated|PF_NoTexture);

	pglDisable(GL_TEXTURE_2D);

	// This is the preferred, 'modern' way of rendering lines -- creating a polygon.
	if (fabsf(v2->x - v1->x) > FLT_EPSILON)
		angle = (float)atan((v2->y-v1->y)/(v2->x-v1->x));
	else
		angle = (float)N_PI_DEMI;
	dx = (float)sin(angle) / (float)screen_width;
	dy = (float)cos(angle) / (float)screen_height;

	p[0] = v1->x - dx;  p[1] = -(v1->y + dy); p[2] = 1;
	p[3] = v2->x - dx;  p[4] = -(v2->y + dy); p[5] = 1;
	p[6] = v2->x + dx;  p[7] = -(v2->y - dy); p[8] = 1;
	p[9] = v1->x + dx;  p[10] = -(v1->y - dy); p[11] = 1;

	pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	pglColor4ubv((GLubyte*)&Color.s);
	pglVertexPointer(3, GL_FLOAT, 0, p);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	pglEnable(GL_TEXTURE_2D);
}

static void Clamp2D(GLenum pname)
{
	pglTexParameteri(GL_TEXTURE_2D, pname, GL_CLAMP); // fallback clamp
#ifdef GL_CLAMP_TO_EDGE
	pglTexParameteri(GL_TEXTURE_2D, pname, GL_CLAMP_TO_EDGE);
#endif
}


// -----------------+
// SetBlend         : Set render mode
// -----------------+
// PF_Masked - we could use an ALPHA_TEST of GL_EQUAL, and alpha ref of 0,
//             is it faster when pixels are discarded ?
EXPORT void HWRAPI(SetBlend) (FBITFIELD PolyFlags)
{
	FBITFIELD Xor;
	Xor = CurrentPolyFlags^PolyFlags;
	if (Xor & (PF_Blending|PF_RemoveYWrap|PF_ForceWrapX|PF_ForceWrapY|PF_Occlude|PF_NoTexture|PF_Modulated|PF_NoDepthTest|PF_Decal|PF_Invisible|PF_NoAlphaTest))
	{
		if (Xor&(PF_Blending)) // if blending mode must be changed
		{
			switch (PolyFlags & PF_Blending) {
				case PF_Translucent & PF_Blending:
					pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha = level of transparency
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
					break;
				case PF_Masked & PF_Blending:
					// Hurdler: does that mean lighting is only made by alpha src?
					// it sounds ok, but not for polygonsmooth
					pglBlendFunc(GL_SRC_ALPHA, GL_ZERO);                // 0 alpha = holes in texture
					pglAlphaFunc(GL_GREATER, 0.5f);
					break;
				case PF_Additive & PF_Blending:
					pglBlendFunc(GL_SRC_ALPHA, GL_ONE);                 // src * alpha + dest
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
					break;
				case PF_Environment & PF_Blending:
					pglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
					break;
				case PF_Substractive & PF_Blending:
					// good for shadow
					// not really but what else ?
					pglBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
					break;
				case PF_Fog & PF_Fog:
					// Sryder: Fog
					// multiplies input colour by input alpha, and destination colour by input colour, then adds them
					pglBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
					break;
				default : // must be 0, otherwise it's an error
					// No blending
					pglBlendFunc(GL_ONE, GL_ZERO);   // the same as no blending
					pglAlphaFunc(GL_GREATER, 0.5f);
					break;
			}
		}
		if (Xor & PF_NoAlphaTest)
		{
			if (PolyFlags & PF_NoAlphaTest)
				pglDisable(GL_ALPHA_TEST);
			else
				pglEnable(GL_ALPHA_TEST);      // discard 0 alpha pixels (holes in texture)
		}

		if (Xor & PF_Decal)
		{
			if (PolyFlags & PF_Decal)
				pglEnable(GL_POLYGON_OFFSET_FILL);
			else
				pglDisable(GL_POLYGON_OFFSET_FILL);
		}
		if (Xor&PF_NoDepthTest)
		{
			if (PolyFlags & PF_NoDepthTest)
				pglDepthFunc(GL_ALWAYS); //pglDisable(GL_DEPTH_TEST);
			else
				pglDepthFunc(GL_LEQUAL); //pglEnable(GL_DEPTH_TEST);
		}

		if (Xor&PF_RemoveYWrap)
		{
			if (PolyFlags & PF_RemoveYWrap)
				Clamp2D(GL_TEXTURE_WRAP_T);
		}

		if (Xor&PF_ForceWrapX)
		{
			if (PolyFlags & PF_ForceWrapX)
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}

		if (Xor&PF_ForceWrapY)
		{
			if (PolyFlags & PF_ForceWrapY)
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		if (Xor&PF_Modulated)
		{
#if defined (__unix__) || defined (UNIXCOMMON)
			if (oglflags & GLF_NOTEXENV)
			{
				if (!(PolyFlags & PF_Modulated))
					pglColor4ubv(white);
			}
			else
#endif

			// mix texture colour with Surface->PolyColor
			if (PolyFlags & PF_Modulated)
				pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			// colour from texture is unchanged before blending
			else
				pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}

		if (Xor & PF_Occlude) // depth test but (no) depth write
		{
			if (PolyFlags&PF_Occlude)
				pglDepthMask(1);
			else
				pglDepthMask(0);
		}
		////Hurdler: not used if we don't define POLYSKY
		if (Xor & PF_Invisible)
		{
			if (PolyFlags&PF_Invisible)
				pglBlendFunc(GL_ZERO, GL_ONE);         // transparent blending
			else
			{   // big hack: (TODO: manage that better)
				// we test only for PF_Masked because PF_Invisible is only used
				// (for now) with it (yeah, that's crappy, sorry)
				if ((PolyFlags&PF_Blending)==PF_Masked)
					pglBlendFunc(GL_SRC_ALPHA, GL_ZERO);
			}
		}
		if (PolyFlags & PF_NoTexture)
			SetNoTexture();
	}
	CurrentPolyFlags = PolyFlags;
}


// -----------------+
// SetTexture       : The mipmap becomes the current texture source
// -----------------+
EXPORT void HWRAPI(SetTexture) (FTextureInfo *pTexInfo)
{
	if (gl_test_disable_something) return;
	if (!pTexInfo)
	{
		SetNoTexture();
		return;
	}
	else if (pTexInfo->downloaded)
	{
		if (pTexInfo->downloaded != tex_downloaded)
		{
			if (!gl_batching)
				pglBindTexture(GL_TEXTURE_2D, pTexInfo->downloaded);
			tex_downloaded = pTexInfo->downloaded;
		}
	}
	else
	{
		// Download a mipmap
		static RGBA_t   tex[2048*2048];
		const GLvoid   *ptex = tex;
		INT32             w, h;

		//GL_DBG_Printf("DownloadMipmap %d\n", NextTexAvail, pTexInfo->grInfo.data);

		w = pTexInfo->width;
		h = pTexInfo->height;

#ifdef USE_PALETTED_TEXTURE
		if (glColorTableEXT &&
			(pTexInfo->grInfo.format == GR_TEXFMT_P_8) &&
			!(pTexInfo->flags & TF_CHROMAKEYED))
		{
			// do nothing here.
			// Not a problem with MiniGL since we don't use paletted texture
		}
		else
#endif

		if ((pTexInfo->grInfo.format == GR_TEXFMT_P_8) ||
			(pTexInfo->grInfo.format == GR_TEXFMT_AP_88))
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					if ((*pImgData == HWR_PATCHES_CHROMAKEY_COLORINDEX) &&
					    (pTexInfo->flags & TF_CHROMAKEYED))
					{
						tex[w*j+i].s.red   = 0;
						tex[w*j+i].s.green = 0;
						tex[w*j+i].s.blue  = 0;
						tex[w*j+i].s.alpha = 0;
						pTexInfo->flags |= TF_TRANSPARENT; // there is a hole in it
					}
					else
					{
						tex[w*j+i].s.red   = myPaletteData[*pImgData].s.red;
						tex[w*j+i].s.green = myPaletteData[*pImgData].s.green;
						tex[w*j+i].s.blue  = myPaletteData[*pImgData].s.blue;
						tex[w*j+i].s.alpha = myPaletteData[*pImgData].s.alpha;
					}

					pImgData++;

					if (pTexInfo->grInfo.format == GR_TEXFMT_AP_88)
					{
						if (!(pTexInfo->flags & TF_CHROMAKEYED))
							tex[w*j+i].s.alpha = *pImgData;
						pImgData++;
					}

				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_RGBA)
		{
			// corona test : passed as ARGB 8888, which is not in glide formats
			// Hurdler: not used for coronas anymore, just for dynamic lighting
			ptex = pTexInfo->grInfo.data;
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					tex[w*j+i].s.red   = *pImgData;
					tex[w*j+i].s.green = *pImgData;
					tex[w*j+i].s.blue  = *pImgData;
					pImgData++;
					tex[w*j+i].s.alpha = *pImgData;
					pImgData++;
				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_8) // Used for fade masks
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					tex[w*j+i].s.red   = 255; // 255 because the fade mask is modulated with the screen texture, so alpha affects it while the colours don't
					tex[w*j+i].s.green = 255;
					tex[w*j+i].s.blue  = 255;
					tex[w*j+i].s.alpha = *pImgData;
					pImgData++;
				}
			}
		}
		else
			GL_DBG_Printf("SetTexture(bad format) %ld\n", pTexInfo->grInfo.format);

		pTexInfo->downloaded = NextTexAvail++;
		tex_downloaded = pTexInfo->downloaded;
		pglBindTexture(GL_TEXTURE_2D, pTexInfo->downloaded);

		// disable texture filtering on any texture that has holes so there's no dumb borders or blending issues
		if (pTexInfo->flags & TF_TRANSPARENT)
		{
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		else
		{
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		}

#ifdef USE_PALETTED_TEXTURE
			//Hurdler: not really supported and not tested recently
		if (glColorTableEXT &&
			(pTexInfo->grInfo.format == GR_TEXFMT_P_8) &&
			!(pTexInfo->flags & TF_CHROMAKEYED))
		{
			glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, palette_tex);
			pglTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, w, h, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, pTexInfo->grInfo.data);
		}
		else
#endif
		if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		{
			//pglTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			if (MipMap)
			{
				pgluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
#ifdef GL_TEXTURE_MIN_LOD
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
#endif
#ifdef GL_TEXTURE_MAX_LOD
				if (pTexInfo->flags & TF_TRANSPARENT)
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0); // No mippmaps on transparent stuff
				else
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4);
#endif
				//pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
				pglTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_8)
		{
			//pglTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			if (MipMap)
			{
				pgluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
#ifdef GL_TEXTURE_MIN_LOD
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
#endif
#ifdef GL_TEXTURE_MAX_LOD
				if (pTexInfo->flags & TF_TRANSPARENT)
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0); // No mippmaps on transparent stuff
				else
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4);
#endif
				//pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
				pglTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		}
		else
		{
			if (MipMap)
			{
				pgluBuild2DMipmaps(GL_TEXTURE_2D, textureformatGL, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
				// Control the mipmap level of detail
#ifdef GL_TEXTURE_MIN_LOD
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0); // the lower the number, the higer the detail
#endif
#ifdef GL_TEXTURE_MAX_LOD
				if (pTexInfo->flags & TF_TRANSPARENT)
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0); // No mippmaps on transparent stuff
				else
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 5);
#endif
			}
			else
				pglTexImage2D(GL_TEXTURE_2D, 0, textureformatGL, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		}

		if (pTexInfo->flags & TF_WRAPX)
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		else
			Clamp2D(GL_TEXTURE_WRAP_S);

		if (pTexInfo->flags & TF_WRAPY)
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		else
			Clamp2D(GL_TEXTURE_WRAP_T);

		if (maximumAnisotropy)
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropic_filter);

		pTexInfo->nextmipmap = NULL;
		if (gr_cachetail)
		{ // insertion en fin de liste
			gr_cachetail->nextmipmap = pTexInfo;
			gr_cachetail = pTexInfo;
		}
		else // initialisation de la liste
			gr_cachetail = gr_cachehead =  pTexInfo;
	}
}

static void load_shaders(FSurfaceInfo *Surface, GLRGBAFloat *mix, GLRGBAFloat *fade)
{
#ifdef GL_SHADERS
	if (gl_shadersenabled)
	{
		//gl_shaderprogramchanged = true;// test for comparing with/without optimization
		gl_shaderprogram_t *shader = &gl_shaderprograms[gl_currentshaderprogram];
		if (shader->program)
		{
			boolean custom = (gl_shaderprograms[gl_currentshaderprogram].custom);
			// 13062019
			// Check for fog
			//if (changed)
			{
				if (!custom)
				{
					if (glstate_fog_mode == 0)	// disabled
					{
						// Nevermind!
						pglUseProgram(0);
						return;
					}
					else	// enabled
					{
						if (gl_shaderprogramchanged)
						{
							pglUseProgram(gl_shaderprograms[gl_currentshaderprogram].program);
							gl_shaderprogramchanged = false;
						}
					}
				}
				else	// always load custom shaders
				{
					if (gl_shaderprogramchanged)
					{
						pglUseProgram(gl_shaderprograms[gl_currentshaderprogram].program);
						gl_shaderprogramchanged = false;
					}
				}
			}

			// set uniforms
			{
				#define UNIFORM_1(uniform, a, function) \
					if (uniform != -1) \
						function (uniform, a);

				#define UNIFORM_2(uniform, a, b, function) \
					if (uniform != -1) \
						function (uniform, a, b);

				#define UNIFORM_3(uniform, a, b, c, function) \
					if (uniform != -1) \
						function (uniform, a, b, c);

				#define UNIFORM_4(uniform, a, b, c, d, function) \
					if (uniform != -1) \
						function (uniform, a, b, c, d);

				// polygon
				UNIFORM_4(shader->uniforms[gluniform_mix_color], mix->red, mix->green, mix->blue, mix->alpha, pglUniform4f);

				// 13062019
				// Check for fog
				if (glstate_fog_mode == 1)
				{
					// glstate
					UNIFORM_1(shader->uniforms[gluniform_fog_density], glstate_fog_density, pglUniform1f);

					// polygon
					UNIFORM_4(shader->uniforms[gluniform_fade_color], fade->red, fade->green, fade->blue, fade->alpha, pglUniform4f);
					UNIFORM_1(shader->uniforms[gluniform_lighting], Surface->LightInfo.light_level, pglUniform1f);

					// Custom shader uniforms
					if (custom)
					{
						UNIFORM_1(shader->uniforms[gluniform_fog_mode], glstate_fog_mode, pglUniform1i);
						UNIFORM_1(shader->uniforms[gluniform_leveltime], (float)gl_leveltime, pglUniform1f);
					}
				}

				#undef UNIFORM_1
				#undef UNIFORM_2
				#undef UNIFORM_3
				#undef UNIFORM_4
			}
		}
		else
			pglUseProgram(0);
	}
	else
#endif
		pglUseProgram(0);
}

// unfinished draw call batching

// Note: could use realloc in the array reallocation code

typedef struct 
{
	FSurfaceInfo surf;// surf also has its own polyflags for some reason, but it seems unused
	unsigned int vertsIndex;// location of verts in unsortedVertexArray
	FUINT numVerts;
	FBITFIELD polyFlags;
	GLuint texNum;
	GLuint shader;
} PolygonArrayEntry;

FOutVector* finalVertexArray = NULL;// contains subset of sorted vertices and texture coordinates to be sent to gpu
UINT32* finalVertexIndexArray = NULL;// contains indexes for glDrawElements, taking into account fan->triangles conversion
//     NOTE have this alloced as 3x finalVertexArray size
int finalVertexArrayAllocSize = 65536;
//GLubyte* colorArray = NULL;// contains color data to be sent to gpu, if needed
//int colorArrayAllocSize = 65536;
// not gonna use this for now, just sort by color and change state when it changes
// later maybe when using vertex attributes if it's needed

PolygonArrayEntry* polygonArray = NULL;// contains the polygon data from DrawPolygon, waiting to be processed
int polygonArraySize = 0;
unsigned int* polygonIndexArray = NULL;// contains sorting pointers for polygonArray
int polygonArrayAllocSize = 65536;

FOutVector* unsortedVertexArray = NULL;// contains unsorted vertices and texture coordinates from DrawPolygon
int unsortedVertexArraySize = 0;
int unsortedVertexArrayAllocSize = 65536;

EXPORT void HWRAPI(StartBatching) (void)
{
	//CONS_Printf("StartBatching begin\n");
	// init arrays if that has not been done yet
	if (!finalVertexArray)
	{
		finalVertexArray = malloc(finalVertexArrayAllocSize * sizeof(FOutVector));
		finalVertexIndexArray = malloc(finalVertexArrayAllocSize * 3 * sizeof(UINT32));
		polygonArray = malloc(polygonArrayAllocSize * sizeof(PolygonArrayEntry));
		polygonIndexArray = malloc(polygonArrayAllocSize * sizeof(unsigned int));
		unsortedVertexArray = malloc(unsortedVertexArrayAllocSize * sizeof(FOutVector));
	}
	// drawing functions will now collect the drawing data instead of passing it to opengl
	gl_batching = true;
	//CONS_Printf("StartBatching end\n");
}

static int comparePolygons(const void *p1, const void *p2)
{
	PolygonArrayEntry* poly1 = &polygonArray[*(unsigned int*)p1];
	PolygonArrayEntry* poly2 = &polygonArray[*(unsigned int*)p2];
	int diff;
	INT64 diff64;
	
	int shader1 = poly1->shader;
	int shader2 = poly2->shader;
	// make skywalls first in order
	if (poly1->polyFlags & PF_NoTexture)
		shader1 = -1;
	if (poly2->polyFlags & PF_NoTexture)
		shader2 = -1;
	diff = shader1 - shader2;
	if (diff != 0) return diff;
	
	diff = poly1->texNum - poly2->texNum;
	if (diff != 0) return diff;
	
	diff = poly1->polyFlags - poly2->polyFlags;
	if (diff != 0) return diff;
	
	diff64 = poly1->surf.PolyColor.rgba - poly2->surf.PolyColor.rgba;
	if (diff64 < 0) return -1; else if (diff64 > 0) return 1;
	diff64 = poly1->surf.FadeColor.rgba - poly2->surf.FadeColor.rgba;
	if (diff64 < 0) return -1; else if (diff64 > 0) return 1;
	
	diff = poly1->surf.LightInfo.light_level - poly2->surf.LightInfo.light_level;
	return diff;
}

static int comparePolygonsNoShaders(const void *p1, const void *p2)
{
	PolygonArrayEntry* poly1 = &polygonArray[*(unsigned int*)p1];
	PolygonArrayEntry* poly2 = &polygonArray[*(unsigned int*)p2];
	int diff;
	INT64 diff64;
	
	GLuint texNum1 = poly1->texNum;
	GLuint texNum2 = poly2->texNum;
	if (poly1->polyFlags & PF_NoTexture)
		texNum1 = 0;
	if (poly2->polyFlags & PF_NoTexture)
		texNum2 = 0;
	diff = texNum1 - texNum2;
	if (diff != 0) return diff;
	
	diff = poly1->polyFlags - poly2->polyFlags;
	if (diff != 0) return diff;
	
	diff64 = poly1->surf.PolyColor.rgba - poly2->surf.PolyColor.rgba;
	if (diff64 < 0) return -1; else if (diff64 > 0) return 1;

	return 0;
}


// the parameters for this functions (numPolys etc.) are used to return rendering stats
EXPORT void HWRAPI(RenderBatches) (int *sNumPolys, int *sNumCalls, int *sNumShaders, int *sNumTextures, int *sNumPolyFlags, int *sNumColors, int *sSortTime, int *sDrawTime)
{
	int i;
	//CONS_Printf("RenderBatches\n");
	gl_batching = false;// no longer collecting batches
	if (!polygonArraySize)
	{
		*sNumPolys = *sNumCalls = *sNumShaders = *sNumTextures = *sNumPolyFlags = *sNumColors = 0;
		return;// nothing to draw
	}
	// init stats vars
	*sNumPolys = polygonArraySize;
	*sNumCalls = 0;
	*sNumShaders = *sNumTextures = *sNumPolyFlags = *sNumColors = 1;
	// init polygonIndexArray
	for (i = 0; i < polygonArraySize; i++)
	{
		polygonIndexArray[i] = i;
	}

	// sort polygons
	//CONS_Printf("qsort polys\n");
	*sSortTime = I_GetTimeMillis();// Using this function gives a compiler warning but works anyway...
	if (gl_allowshaders)
		qsort(polygonIndexArray, polygonArraySize, sizeof(unsigned int), comparePolygons);
	else
		qsort(polygonIndexArray, polygonArraySize, sizeof(unsigned int), comparePolygonsNoShaders);
	*sSortTime = I_GetTimeMillis() - *sSortTime;
	//CONS_Printf("sort done\n");
	// sort order
	// 1. shader
	// 2. texture
	// 3. polyflags
	// 4. colors + light level
	// not sure about order of last 2, or if it even matters

	*sDrawTime = I_GetTimeMillis();

	int finalVertexWritePos = 0;// position in finalVertexArray
	int finalIndexWritePos = 0;// position in finalVertexIndexArray

	int polygonReadPos = 0;// position in polygonIndexArray

	GLuint currentShader = polygonArray[polygonIndexArray[0]].shader;
	GLuint currentTexture = polygonArray[polygonIndexArray[0]].texNum;
	FBITFIELD currentPolyFlags = polygonArray[polygonIndexArray[0]].polyFlags;
	FSurfaceInfo currentSurfaceInfo = polygonArray[polygonIndexArray[0]].surf;
	// For now, will sort and track the colors. Vertex attributes could be used instead of uniforms
	// and a color array could replace the color calls.
	
	// set state for first batch
	//CONS_Printf("set first state\n");
	gl_currentshaderprogram = currentShader;
	gl_shaderprogramchanged = true;
	GLRGBAFloat firstMix = {0,0,0,0};
	GLRGBAFloat firstFade = {0,0,0,0};
	if (currentPolyFlags & PF_Modulated)
	{
		// Mix color
		firstMix.red    = byte2float[currentSurfaceInfo.PolyColor.s.red];
		firstMix.green  = byte2float[currentSurfaceInfo.PolyColor.s.green];
		firstMix.blue   = byte2float[currentSurfaceInfo.PolyColor.s.blue];
		firstMix.alpha  = byte2float[currentSurfaceInfo.PolyColor.s.alpha];
		pglColor4ubv((GLubyte*)&currentSurfaceInfo.PolyColor.s);
	}
	// Fade color
	firstFade.red   = byte2float[currentSurfaceInfo.FadeColor.s.red];
	firstFade.green = byte2float[currentSurfaceInfo.FadeColor.s.green];
	firstFade.blue  = byte2float[currentSurfaceInfo.FadeColor.s.blue];
	firstFade.alpha = byte2float[currentSurfaceInfo.FadeColor.s.alpha];
	
	if (gl_allowshaders)
		load_shaders(&currentSurfaceInfo, &firstMix, &firstFade);
	
	if (currentPolyFlags & PF_NoTexture)
		currentTexture = 0;
	pglBindTexture(GL_TEXTURE_2D, currentTexture);
	tex_downloaded = currentTexture;
	
	SetBlend(currentPolyFlags);
	
	boolean needRebind = false;
	//CONS_Printf("first pointers to ogl\n");
	pglVertexPointer(3, GL_FLOAT, sizeof(FOutVector), &finalVertexArray[0].x);
	pglTexCoordPointer(2, GL_FLOAT, sizeof(FOutVector), &finalVertexArray[0].s);
	
	while (1)// note: remember handling notexture polyflag as having texture number 0 (also in comparePolygons)
	{
		//CONS_Printf("loop iter start\n");
		// new try:
		// write vertices
		// check for changes or end, otherwise go back to writing
			// changes will affect the next vars and the change bools
			// end could set flag for stopping
		// execute draw call
		// could check ending flag here
		// change states according to next vars and change bools, updating the current vars and reseting the bools
		// reset write pos
		// repeat loop
		
		int index = polygonIndexArray[polygonReadPos++];
		FUINT numVerts = polygonArray[index].numVerts;
		// before writing, check if there is enough room
		// using 'while' instead of 'if' here makes sure that there will *always* be enough room.
		// probably never will this loop run more than once though
		while (finalVertexWritePos + numVerts > finalVertexArrayAllocSize)
		{
			//CONS_Printf("final vert realloc\n");
			finalVertexArrayAllocSize *= 2;
			FOutVector* new_array = malloc(finalVertexArrayAllocSize * sizeof(FOutVector));
			memcpy(new_array, finalVertexArray, finalVertexWritePos * sizeof(FOutVector));
			free(finalVertexArray);
			finalVertexArray = new_array;
			// also increase size of index array, 3x of vertex array since
			// going from fans to triangles increases vertex count to 3x
			unsigned int* new_index_array = malloc(finalVertexArrayAllocSize * 3 * sizeof(UINT32));
			memcpy(new_index_array, finalVertexIndexArray, finalIndexWritePos * sizeof(UINT32));
			free(finalVertexIndexArray);
			finalVertexIndexArray = new_index_array;
			// if vertex buffers are reallocated then opengl needs to know too
			needRebind = true;
		}
		//CONS_Printf("write verts to final\n");
		// write the vertices of the polygon
		memcpy(&finalVertexArray[finalVertexWritePos], &unsortedVertexArray[polygonArray[index].vertsIndex],
			numVerts * sizeof(FOutVector));
		// write the indexes, pointing to the fan vertexes but in triangles format
		int firstIndex = finalVertexWritePos;
		int lastIndex = finalVertexWritePos + numVerts;
		finalVertexWritePos += 2;
		//CONS_Printf("write final vert indices\n");
		while (finalVertexWritePos < lastIndex)
		{
			finalVertexIndexArray[finalIndexWritePos++] = firstIndex;
			finalVertexIndexArray[finalIndexWritePos++] = finalVertexWritePos - 1;
			finalVertexIndexArray[finalIndexWritePos++] = finalVertexWritePos++;
		}
		
		boolean stopFlag = false;
		boolean changeState = false;
		boolean changeShader = false;
		GLuint nextShader;
		boolean changeTexture = false;
		GLuint nextTexture;
		boolean changePolyFlags = false;
		FBITFIELD nextPolyFlags;
		boolean changeSurfaceInfo = false;
		FSurfaceInfo nextSurfaceInfo;
		if (polygonReadPos >= polygonArraySize)
		{
			stopFlag = true;
		}
		else
		{
			//CONS_Printf("state change check\n");
			// check if a state change is required, set the change bools and next vars
			int nextIndex = polygonIndexArray[polygonReadPos];
			nextShader = polygonArray[nextIndex].shader;
			nextTexture = polygonArray[nextIndex].texNum;
			nextPolyFlags = polygonArray[nextIndex].polyFlags;
			nextSurfaceInfo = polygonArray[nextIndex].surf;
			if (nextPolyFlags & PF_NoTexture)
				nextTexture = 0;
			if (currentShader != nextShader)
			{
				changeState = true;
				changeShader = true;
			}
			if (currentTexture != nextTexture)
			{
				changeState = true;
				changeTexture = true;
			}
			if (currentPolyFlags != nextPolyFlags)
			{
				changeState = true;
				changePolyFlags = true;
			}
			if (gl_allowshaders)
			{
				if (currentSurfaceInfo.PolyColor.rgba != nextSurfaceInfo.PolyColor.rgba ||
					currentSurfaceInfo.FadeColor.rgba != nextSurfaceInfo.FadeColor.rgba ||
					currentSurfaceInfo.LightInfo.light_level != nextSurfaceInfo.LightInfo.light_level)
				{
					changeState = true;
					changeSurfaceInfo = true;
				}
			}
			else
			{
				if (currentSurfaceInfo.PolyColor.rgba != nextSurfaceInfo.PolyColor.rgba)
				{
					changeState = true;
					changeSurfaceInfo = true;
				}
			}
		}
		
		if (changeState || stopFlag)
		{
			if (needRebind)
			{
				//CONS_Printf("rebind\n");
				pglVertexPointer(3, GL_FLOAT, sizeof(FOutVector), &finalVertexArray[0].x);
				pglTexCoordPointer(2, GL_FLOAT, sizeof(FOutVector), &finalVertexArray[0].s);
				needRebind = false;
			}
			//CONS_Printf("exec draw call\n");
			// execute draw call
			pglDrawElements(GL_TRIANGLES, finalIndexWritePos, GL_UNSIGNED_INT, finalVertexIndexArray);
			//CONS_Printf("draw call done\n");
			// reset write positions
			finalVertexWritePos = 0;
			finalIndexWritePos = 0;
			// update stats
			(*sNumCalls)++;
		}
		else continue;
		
		// if we're here then either its time to stop or time to change state
		if (stopFlag) break;
		
		//CONS_Printf("state change\n");
		
		// change state according to change bools and next vars, update current vars and reset bools
		if (changeShader)
		{
			gl_currentshaderprogram = nextShader;
			gl_shaderprogramchanged = true;
			GLRGBAFloat mix = {0,0,0,0};
			GLRGBAFloat fade = {0,0,0,0};
			if (nextPolyFlags & PF_Modulated)
			{
				// Mix color
				mix.red    = byte2float[nextSurfaceInfo.PolyColor.s.red];
				mix.green  = byte2float[nextSurfaceInfo.PolyColor.s.green];
				mix.blue   = byte2float[nextSurfaceInfo.PolyColor.s.blue];
				mix.alpha  = byte2float[nextSurfaceInfo.PolyColor.s.alpha];
			}
			// Fade color
			fade.red   = byte2float[nextSurfaceInfo.FadeColor.s.red];
			fade.green = byte2float[nextSurfaceInfo.FadeColor.s.green];
			fade.blue  = byte2float[nextSurfaceInfo.FadeColor.s.blue];
			fade.alpha = byte2float[nextSurfaceInfo.FadeColor.s.alpha];
			
			load_shaders(&nextSurfaceInfo, &mix, &fade);
			currentShader = nextShader;
			changeShader = false;

			(*sNumShaders)++;
		}
		if (changeTexture)
		{
			// texture should be already set up by calls to SetTexture during batch collection
			pglBindTexture(GL_TEXTURE_2D, nextTexture);
			tex_downloaded = nextTexture;
			currentTexture = nextTexture;
			changeTexture = false;

			(*sNumTextures)++;
		}
		if (changePolyFlags)
		{
			SetBlend(nextPolyFlags);
			currentPolyFlags = nextPolyFlags;
			changePolyFlags = false;

			(*sNumPolyFlags)++;
		}
		if (changeSurfaceInfo)
		{
			gl_shaderprogramchanged = false;
			GLRGBAFloat mix = {0,0,0,0};
			GLRGBAFloat fade = {0,0,0,0};
			if (nextPolyFlags & PF_Modulated)
			{
				// Mix color
				mix.red    = byte2float[nextSurfaceInfo.PolyColor.s.red];
				mix.green  = byte2float[nextSurfaceInfo.PolyColor.s.green];
				mix.blue   = byte2float[nextSurfaceInfo.PolyColor.s.blue];
				mix.alpha  = byte2float[nextSurfaceInfo.PolyColor.s.alpha];
				pglColor4ubv((GLubyte*)&nextSurfaceInfo.PolyColor.s);
			}
			if (gl_allowshaders)
			{
				// Fade color
				fade.red   = byte2float[nextSurfaceInfo.FadeColor.s.red];
				fade.green = byte2float[nextSurfaceInfo.FadeColor.s.green];
				fade.blue  = byte2float[nextSurfaceInfo.FadeColor.s.blue];
				fade.alpha = byte2float[nextSurfaceInfo.FadeColor.s.alpha];
				
				load_shaders(&nextSurfaceInfo, &mix, &fade);
			}
			currentSurfaceInfo = nextSurfaceInfo;
			changeSurfaceInfo = false;

			(*sNumColors)++;
		}
		// and that should be it?
	}
	// reset the arrays (set sizes to 0)
	polygonArraySize = 0;
	unsortedVertexArraySize = 0;
	
	*sDrawTime = I_GetTimeMillis() - *sDrawTime;
}

// -----------------+
// DrawPolygon      : Render a polygon, set the texture, set render mode
// -----------------+
EXPORT void HWRAPI(DrawPolygon) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags)
{
	if (gl_batching)
	{
		//CONS_Printf("Batched DrawPolygon\n");
		if (!pSurf)
			I_Error("Got a null FSurfaceInfo in batching");// nulls should only come in sky background pic drawing
		if (polygonArraySize == polygonArrayAllocSize)
		{
			// ran out of space, make new array double the size
			polygonArrayAllocSize *= 2;
			PolygonArrayEntry* new_array = malloc(polygonArrayAllocSize * sizeof(PolygonArrayEntry));
			memcpy(new_array, polygonArray, polygonArraySize * sizeof(PolygonArrayEntry));
			free(polygonArray);
			polygonArray = new_array;
			// also need to redo the index array, dont need to copy it though
			free(polygonIndexArray);
			polygonIndexArray = malloc(polygonArrayAllocSize * sizeof(unsigned int));
		}

		while (unsortedVertexArraySize + iNumPts > unsortedVertexArrayAllocSize)
		{
			// need more space for vertices in unsortedVertexArray
			unsortedVertexArrayAllocSize *= 2;
			FOutVector* new_array = malloc(unsortedVertexArrayAllocSize * sizeof(FOutVector));
			memcpy(new_array, unsortedVertexArray, unsortedVertexArraySize * sizeof(FOutVector));
			free(unsortedVertexArray);
			unsortedVertexArray = new_array;
		}	

		// add the polygon data to the arrays

		polygonArray[polygonArraySize].surf = *pSurf;
		polygonArray[polygonArraySize].vertsIndex = unsortedVertexArraySize;
		polygonArray[polygonArraySize].numVerts = iNumPts;
		polygonArray[polygonArraySize].polyFlags = PolyFlags;
		polygonArray[polygonArraySize].texNum = tex_downloaded;
		polygonArray[polygonArraySize].shader = gl_currentshaderprogram;
		polygonArraySize++;

		memcpy(&unsortedVertexArray[unsortedVertexArraySize], pOutVerts, iNumPts * sizeof(FOutVector));
		unsortedVertexArraySize += iNumPts;
	}
	else
	{
		if (gl_test_disable_something) return;
		
		static GLRGBAFloat mix = {0,0,0,0};
		static GLRGBAFloat fade = {0,0,0,0};

		SetBlend(PolyFlags);    //TODO: inline (#pragma..)

		// PolyColor
		if (pSurf)
		{
			// If Modulated, mix the surface colour to the texture
			if (CurrentPolyFlags & PF_Modulated)
			{
				// Mix color
				mix.red    = byte2float[pSurf->PolyColor.s.red];
				mix.green  = byte2float[pSurf->PolyColor.s.green];
				mix.blue   = byte2float[pSurf->PolyColor.s.blue];
				mix.alpha  = byte2float[pSurf->PolyColor.s.alpha];

				pglColor4ubv((GLubyte*)&pSurf->PolyColor.s);
			}

			// Fade color
			fade.red   = byte2float[pSurf->FadeColor.s.red];
			fade.green = byte2float[pSurf->FadeColor.s.green];
			fade.blue  = byte2float[pSurf->FadeColor.s.blue];
			fade.alpha = byte2float[pSurf->FadeColor.s.alpha];
		}

		load_shaders(pSurf, &mix, &fade);

		pglVertexPointer(3, GL_FLOAT, sizeof(FOutVector), &pOutVerts[0].x);
		pglTexCoordPointer(2, GL_FLOAT, sizeof(FOutVector), &pOutVerts[0].s);
		pglDrawArrays(GL_TRIANGLE_FAN, 0, iNumPts);
	/*
		if (PolyFlags & PF_RemoveYWrap)
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (PolyFlags & PF_ForceWrapX)
			Clamp2D(GL_TEXTURE_WRAP_S);

		if (PolyFlags & PF_ForceWrapY)
			Clamp2D(GL_TEXTURE_WRAP_T);*/// test: remove these and see if they are important

	#ifdef GL_SHADERS
		//pglUseProgram(0);// test: removed this, trying to improve performance
	#endif
	}
}

// ==========================================================================
//
// ==========================================================================
EXPORT void HWRAPI(SetSpecialState) (hwdspecialstate_t IdState, INT32 Value)
{
	switch (IdState)
	{
		case HWD_SET_SHADERS:
			switch (Value)
			{
				case 1:
					gl_allowshaders = true;
					break;
				default:
					gl_allowshaders = false;
					break;
			}
			break;
		case HWD_SET_WIREFRAME:
			pglPolygonMode(GL_FRONT_AND_BACK, Value ? GL_LINE : GL_FILL);
			break;
		
		case HWD_SET_TEST_DISABLE_SOMETHING:
			gl_test_disable_something = Value;
			break;

		case HWD_SET_FOG_MODE:
			glstate_fog_mode = Value;
			break;

		case HWD_SET_FOG_DENSITY:
			glstate_fog_density = FIXED_TO_FLOAT(Value);
			break;

		case HWD_SET_TEXTUREFILTERMODE:
			switch (Value)
			{
				case HWD_SET_TEXTUREFILTER_TRILINEAR:
					min_filter = GL_LINEAR_MIPMAP_LINEAR;
					mag_filter = GL_LINEAR;
					MipMap = GL_TRUE;
					break;
				case HWD_SET_TEXTUREFILTER_BILINEAR:
					min_filter = mag_filter = GL_LINEAR;
					MipMap = GL_FALSE;
					break;
				case HWD_SET_TEXTUREFILTER_POINTSAMPLED:
					min_filter = mag_filter = GL_NEAREST;
					MipMap = GL_FALSE;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED1:
					min_filter = GL_NEAREST;
					mag_filter = GL_LINEAR;
					MipMap = GL_FALSE;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED2:
					min_filter = GL_LINEAR;
					mag_filter = GL_NEAREST;
					MipMap = GL_FALSE;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED3:
					min_filter = GL_LINEAR_MIPMAP_LINEAR;
					mag_filter = GL_NEAREST;
					MipMap = GL_TRUE;
					break;
				default:
					mag_filter = GL_LINEAR;
					min_filter = GL_NEAREST;
			}
			if (!pgluBuild2DMipmaps)
			{
				MipMap = GL_FALSE;
				min_filter = GL_LINEAR;
			}
			Flush(); //??? if we want to change filter mode by texture, remove this
			break;

		case HWD_SET_TEXTUREANISOTROPICMODE:
			anisotropic_filter = min(Value,maximumAnisotropy);
			if (maximumAnisotropy)
				Flush(); //??? if we want to change filter mode by texture, remove this
			break;

		default:
			break;
	}
}

static float *vertBuffer = NULL;
static float *normBuffer = NULL;
static size_t lerpBufferSize = 0;
static short *vertTinyBuffer = NULL;
static char *normTinyBuffer = NULL;
static size_t lerpTinyBufferSize = 0;

// Static temporary buffer for doing frame interpolation
// 'size' is the vertex size
static void AllocLerpBuffer(size_t size)
{
	if (lerpBufferSize >= size)
		return;

	if (vertBuffer != NULL)
		free(vertBuffer);

	if (normBuffer != NULL)
		free(normBuffer);

	lerpBufferSize = size;
	vertBuffer = malloc(lerpBufferSize);
	normBuffer = malloc(lerpBufferSize);
}

// Static temporary buffer for doing frame interpolation
// 'size' is the vertex size
static void AllocLerpTinyBuffer(size_t size)
{
	if (lerpTinyBufferSize >= size)
		return;

	if (vertTinyBuffer != NULL)
		free(vertTinyBuffer);

	if (normTinyBuffer != NULL)
		free(normTinyBuffer);

	lerpTinyBufferSize = size;
	vertTinyBuffer = malloc(lerpTinyBufferSize);
	normTinyBuffer = malloc(lerpTinyBufferSize / 2);
}

#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif

static void CreateModelVBO(mesh_t *mesh, mdlframe_t *frame)
{
	int bufferSize = sizeof(vbo64_t)*mesh->numTriangles * 3;
	vbo64_t *buffer = (vbo64_t*)malloc(bufferSize);
	vbo64_t *bufPtr = buffer;

	float *vertPtr = frame->vertices;
	float *normPtr = frame->normals;
	float *tanPtr = frame->tangents;
	float *uvPtr = mesh->uvs;
	float *lightPtr = mesh->lightuvs;
	char *colorPtr = frame->colors;

	int i;
	for (i = 0; i < mesh->numTriangles * 3; i++)
	{
		bufPtr->x = *vertPtr++;
		bufPtr->y = *vertPtr++;
		bufPtr->z = *vertPtr++;

		bufPtr->nx = *normPtr++;
		bufPtr->ny = *normPtr++;
		bufPtr->nz = *normPtr++;

		bufPtr->s0 = *uvPtr++;
		bufPtr->t0 = *uvPtr++;

		if (tanPtr != NULL)
		{
			bufPtr->tan0 = *tanPtr++;
			bufPtr->tan1 = *tanPtr++;
			bufPtr->tan2 = *tanPtr++;
		}

		if (lightPtr != NULL)
		{
			bufPtr->s1 = *lightPtr++;
			bufPtr->t1 = *lightPtr++;
		}

		if (colorPtr)
		{
			bufPtr->r = *colorPtr++;
			bufPtr->g = *colorPtr++;
			bufPtr->b = *colorPtr++;
			bufPtr->a = *colorPtr++;
		}
		else
		{
			bufPtr->r = 255;
			bufPtr->g = 255;
			bufPtr->b = 255;
			bufPtr->a = 255;
		}

		bufPtr++;
	}

	pglGenBuffers(1, &frame->vboID);
	pglBindBuffer(GL_ARRAY_BUFFER, frame->vboID);
	pglBufferData(GL_ARRAY_BUFFER, bufferSize, buffer, GL_STATIC_DRAW);
	free(buffer);

	// Don't leave the array buffer bound to the model,
	// since this is called mid-frame
	pglBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void CreateModelVBOTiny(mesh_t *mesh, tinyframe_t *frame)
{
	int bufferSize = sizeof(vbotiny_t)*mesh->numTriangles * 3;
	vbotiny_t *buffer = (vbotiny_t*)malloc(bufferSize);
	vbotiny_t *bufPtr = buffer;

	short *vertPtr = frame->vertices;
	char *normPtr = frame->normals;
	float *uvPtr = mesh->uvs;
	char *tanPtr = frame->tangents;

	int i;
	for (i = 0; i < mesh->numVertices; i++)
	{
		bufPtr->x = *vertPtr++;
		bufPtr->y = *vertPtr++;
		bufPtr->z = *vertPtr++;

		bufPtr->nx = *normPtr++;
		bufPtr->ny = *normPtr++;
		bufPtr->nz = *normPtr++;

		bufPtr->s0 = *uvPtr++;
		bufPtr->t0 = *uvPtr++;

		if (tanPtr)
		{
			bufPtr->tanx = *tanPtr++;
			bufPtr->tany = *tanPtr++;
			bufPtr->tanz = *tanPtr++;
		}

		bufPtr++;
	}

	pglGenBuffers(1, &frame->vboID);
	pglBindBuffer(GL_ARRAY_BUFFER, frame->vboID);
	pglBufferData(GL_ARRAY_BUFFER, bufferSize, buffer, GL_STATIC_DRAW);
	free(buffer);

	// Don't leave the array buffer bound to the model,
	// since this is called mid-frame
	pglBindBuffer(GL_ARRAY_BUFFER, 0);
}

EXPORT void HWRAPI(CreateModelVBOs) (model_t *model)
{
	int i;
	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (mesh->frames)
		{
			int j;
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				mdlframe_t *frame = &mesh->frames[j];
				if (frame->vboID)
					pglDeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
				CreateModelVBO(mesh, frame);
			}
		}
		else if (mesh->tinyframes)
		{
			int j;
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				tinyframe_t *frame = &mesh->tinyframes[j];
				if (frame->vboID)
					pglDeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
				CreateModelVBOTiny(mesh, frame);
			}
		}
	}
}

#define BUFFER_OFFSET(i) ((char*)(i))

static void DrawModelEx(model_t *model, INT32 frameIndex, INT32 duration, INT32 tics, INT32 nextFrameIndex, FTransform *pos, float scale, UINT8 flipped, FSurfaceInfo *Surface)
{
	static GLRGBAFloat mix = {0,0,0,0};
	static GLRGBAFloat fade = {0,0,0,0};

	float pol = 0.0f;
	float scalex, scaley, scalez;

	boolean useTinyFrames;

	int i;

	// Affect input model scaling
	scale *= 0.5f;
	scalex = scale;
	scaley = scale;
	scalez = scale;

	if (duration != 0 && duration != -1 && tics != -1) // don't interpolate if instantaneous or infinite in length
	{
		UINT32 newtime = (duration - tics); // + 1;

		pol = (newtime)/(float)duration;

		if (pol > 1.0f)
			pol = 1.0f;

		if (pol < 0.0f)
			pol = 0.0f;
	}

	mix.red    = byte2float[Surface->PolyColor.s.red];
	mix.green  = byte2float[Surface->PolyColor.s.green];
	mix.blue   = byte2float[Surface->PolyColor.s.blue];
	mix.alpha  = byte2float[Surface->PolyColor.s.alpha];

	if (mix.alpha < 1)
		SetBlend(PF_Translucent|PF_Modulated);
	else
		SetBlend(PF_Masked|PF_Modulated|PF_Occlude);

	pglColor4ubv((GLubyte*)&Surface->PolyColor.s);

	fade.red   = byte2float[Surface->FadeColor.s.red];
	fade.green = byte2float[Surface->FadeColor.s.green];
	fade.blue  = byte2float[Surface->FadeColor.s.blue];
	fade.alpha = byte2float[Surface->FadeColor.s.alpha];

	load_shaders(Surface, &mix, &fade);

	pglEnable(GL_CULL_FACE);
	pglEnable(GL_NORMALIZE);

#ifdef USE_FTRANSFORM_MIRROR
	// flipped is if the object is flipped
	// pos->flip is if the screen is flipped vertically
	// pos->mirror is if the screen is flipped horizontally
	// XOR all the flips together to figure out what culling to use!
	{
		boolean reversecull = (flipped ^ pos->flip ^ pos->mirror);
		if (reversecull)
			pglCullFace(GL_FRONT);
		else
			pglCullFace(GL_BACK);
	}
#else
	// pos->flip is if the screen is flipped too
	if (flipped != pos->flip) // If either are active, but not both, invert the model's culling
		pglCullFace(GL_FRONT);
	else
		pglCullFace(GL_BACK);
#endif

	pglPushMatrix(); // should be the same as glLoadIdentity
	pglTranslatef(pos->x, pos->z, pos->y);
	if (flipped)
		scaley = -scaley;
#ifdef USE_FTRANSFORM_ANGLEZ
	pglRotatef(pos->anglez, 0.0f, 0.0f, -1.0f); // rotate by slope from Kart
#endif
	pglRotatef(pos->anglex, -1.0f, 0.0f, 0.0f);
	pglRotatef(pos->angley, 0.0f, -1.0f, 0.0f);

	pglScalef(scalex, scaley, scalez);

	useTinyFrames = model->meshes[0].tinyframes != NULL;

	if (useTinyFrames)
		pglScalef(1 / 64.0f, 1 / 64.0f, 1 / 64.0f);

	pglEnableClientState(GL_NORMAL_ARRAY);

	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (useTinyFrames)
		{
			tinyframe_t *frame = &mesh->tinyframes[frameIndex % mesh->numFrames];
			tinyframe_t *nextframe = NULL;

			if (nextFrameIndex != -1)
				nextframe = &mesh->tinyframes[nextFrameIndex % mesh->numFrames];

			if (!nextframe || fpclassify(pol) == FP_ZERO)
			{
				pglBindBuffer(GL_ARRAY_BUFFER, frame->vboID);
				pglVertexPointer(3, GL_SHORT, sizeof(vbotiny_t), BUFFER_OFFSET(0));
				pglNormalPointer(GL_BYTE, sizeof(vbotiny_t), BUFFER_OFFSET(sizeof(short)*3));
				pglTexCoordPointer(2, GL_FLOAT, sizeof(vbotiny_t), BUFFER_OFFSET(sizeof(short) * 3 + sizeof(char) * 6));
				pglDrawElements(GL_TRIANGLES, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->indices);
				pglBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				short *vertPtr;
				char *normPtr;
				int j = 0;

				// Dangit, I soooo want to do this in a GLSL shader...
				AllocLerpTinyBuffer(mesh->numVertices * sizeof(short) * 3);
				vertPtr = vertTinyBuffer;
				normPtr = normTinyBuffer;

				for (j = 0; j < mesh->numVertices * 3; j++)
				{
					// Interpolate
					*vertPtr++ = (short)(frame->vertices[j] + (pol * (nextframe->vertices[j] - frame->vertices[j])));
					*normPtr++ = (char)(frame->normals[j] + (pol * (nextframe->normals[j] - frame->normals[j])));
				}

				pglVertexPointer(3, GL_SHORT, 0, vertTinyBuffer);
				pglNormalPointer(GL_BYTE, 0, normTinyBuffer);
				pglTexCoordPointer(2, GL_FLOAT, 0, mesh->uvs);
				pglDrawElements(GL_TRIANGLES, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->indices);
			}
		}
		else
		{
			mdlframe_t *frame = &mesh->frames[frameIndex % mesh->numFrames];
			mdlframe_t *nextframe = NULL;

			if (nextFrameIndex != -1)
				nextframe = &mesh->frames[nextFrameIndex % mesh->numFrames];

			if (!nextframe || fpclassify(pol) == FP_ZERO)
			{
				// Zoom! Take advantage of just shoving the entire arrays to the GPU.
				pglBindBuffer(GL_ARRAY_BUFFER, frame->vboID);
				pglVertexPointer(3, GL_FLOAT, sizeof(vbo64_t), BUFFER_OFFSET(0));
				pglNormalPointer(GL_FLOAT, sizeof(vbo64_t), BUFFER_OFFSET(sizeof(float) * 3));
				pglTexCoordPointer(2, GL_FLOAT, sizeof(vbo64_t), BUFFER_OFFSET(sizeof(float) * 6));

				pglDrawArrays(GL_TRIANGLES, 0, mesh->numTriangles * 3);
				pglBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				float *vertPtr;
				float *normPtr;
				int j = 0;

				// Dangit, I soooo want to do this in a GLSL shader...
				AllocLerpBuffer(mesh->numVertices * sizeof(float) * 3);
				vertPtr = vertBuffer;
				normPtr = normBuffer;

				for (j = 0; j < mesh->numVertices * 3; j++)
				{
					// Interpolate
					*vertPtr++ = frame->vertices[j] + (pol * (nextframe->vertices[j] - frame->vertices[j]));
					*normPtr++ = frame->normals[j] + (pol * (nextframe->normals[j] - frame->normals[j]));
				}

				pglVertexPointer(3, GL_FLOAT, 0, vertBuffer);
				pglNormalPointer(GL_FLOAT, 0, normBuffer);
				pglTexCoordPointer(2, GL_FLOAT, 0, mesh->uvs);
				pglDrawArrays(GL_TRIANGLES, 0, mesh->numVertices);
			}
		}
	}

	pglDisableClientState(GL_NORMAL_ARRAY);

	pglPopMatrix(); // should be the same as glLoadIdentity
	pglDisable(GL_CULL_FACE);
	pglDisable(GL_NORMALIZE);

#ifdef GL_SHADERS
	pglUseProgram(0);
#endif
}

// -----------------+
// HWRAPI DrawModel : Draw a model
// -----------------+
EXPORT void HWRAPI(DrawModel) (model_t *model, INT32 frameIndex, INT32 duration, INT32 tics, INT32 nextFrameIndex, FTransform *pos, float scale, UINT8 flipped, FSurfaceInfo *Surface)
{
	DrawModelEx(model, frameIndex, duration, tics, nextFrameIndex, pos, scale, flipped, Surface);
}

// -----------------+
// SetTransform     :
// -----------------+
EXPORT void HWRAPI(SetTransform) (FTransform *stransform)
{
	static boolean special_splitscreen;
	GLdouble used_fov;
	boolean shearing = false;
	pglLoadIdentity();
	if (stransform)
	{
		used_fov = stransform->fovxangle;
		shearing = stransform->shearing;
		// keep a trace of the transformation for md2
		memcpy(&md2_transform, stransform, sizeof (md2_transform));

#ifdef USE_FTRANSFORM_MIRROR
		// mirroring from Kart
		if (stransform->mirror)
			pglScalef(-stransform->scalex, stransform->scaley, -stransform->scalez);
		else
#endif
		if (stransform->flip)
			pglScalef(stransform->scalex, -stransform->scaley, -stransform->scalez);
		else
			pglScalef(stransform->scalex, stransform->scaley, -stransform->scalez);

		pglMatrixMode(GL_MODELVIEW);
		pglRotatef(stransform->anglex, 1.0f, 0.0f, 0.0f);
		pglRotatef(stransform->angley+270.0f, 0.0f, 1.0f, 0.0f);
		pglTranslatef(-stransform->x, -stransform->z, -stransform->y);

		special_splitscreen = (stransform->splitscreen == 1);
	}
	else
	{
		//Hurdler: is "fov" correct?
		used_fov = fov;
		pglScalef(1.0f, 1.0f, -1.0f);
	}

	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();

	// jimita 14042019
	// Simulate Software's y-shearing
	// https://zdoom.org/wiki/Y-shearing
	if (shearing)
	{
		float dy = FIXED_TO_FLOAT(AIMINGTODY(stransform->viewaiming)) * 2; //screen_width/BASEVIDWIDTH;
		pglTranslatef(0.0f, -dy/BASEVIDHEIGHT, 0.0f);
	}

	if (special_splitscreen)
	{
		used_fov = atan(tan(used_fov*M_PIl/360)*0.8)*360/M_PIl;
		GLPerspective((GLfloat)used_fov, 2*ASPECT_RATIO);
	}
	else
		GLPerspective((GLfloat)used_fov, ASPECT_RATIO);
	pglGetFloatv(GL_PROJECTION_MATRIX, projMatrix); // added for new coronas' code (without depth buffer)
	pglMatrixMode(GL_MODELVIEW);

	pglGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix); // added for new coronas' code (without depth buffer)
}

EXPORT INT32  HWRAPI(GetTextureUsed) (void)
{
	FTextureInfo*   tmp = gr_cachehead;
	INT32             res = 0;

	while (tmp)
	{
		res += tmp->height*tmp->width*(screen_depth/8);
		tmp = tmp->nextmipmap;
	}
	return res;
}

EXPORT void HWRAPI(PostImgRedraw) (float points[SCREENVERTS][SCREENVERTS][2])
{
	INT32 x, y;
	float float_x, float_y, float_nextx, float_nexty;
	float xfix, yfix;
	//INT32 texsize = 2048;
	INT32 texsize = 512;

	const float blackBack[16] =
	{
		-16.0f, -16.0f, 6.0f,
		-16.0f, 16.0f, 6.0f,
		16.0f, 16.0f, 6.0f,
		16.0f, -16.0f, 6.0f
	};
/*
	// Use a power of two texture
	if(screen_width <= 2048)
		texsize = 2048;
	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;
*/	
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// X/Y stretch fix for all resolutions(!)
	xfix = (float)(texsize)/((float)((screen_width)/(float)(SCREENVERTS-1)));
	yfix = (float)(texsize)/((float)((screen_height)/(float)(SCREENVERTS-1)));

	pglDisable(GL_DEPTH_TEST);
	pglDisable(GL_BLEND);

	// Draw a black square behind the screen texture,
	// so nothing shows through the edges
	pglColor4ubv(white);

	pglVertexPointer(3, GL_FLOAT, 0, blackBack);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	for(x=0;x<SCREENVERTS-1;x++)
	{
		for(y=0;y<SCREENVERTS-1;y++)
		{
			float stCoords[8];
			float vertCoords[12];

			// Used for texture coordinates
			// Annoying magic numbers to scale the square texture to
			// a non-square screen..
			float_x = (float)(x/(xfix));
			float_y = (float)(y/(yfix));
			float_nextx = (float)(x+1)/(xfix);
			float_nexty = (float)(y+1)/(yfix);

			stCoords[0] = float_x;
			stCoords[1] = float_y;
			stCoords[2] = float_x;
			stCoords[3] = float_nexty;
			stCoords[4] = float_nextx;
			stCoords[5] = float_nexty;
			stCoords[6] = float_nextx;
			stCoords[7] = float_y;

			pglTexCoordPointer(2, GL_FLOAT, 0, stCoords);

			vertCoords[0] = points[x][y][0];
			vertCoords[1] = points[x][y][1];
			vertCoords[2] = 4.4f;
			vertCoords[3] = points[x][y + 1][0];
			vertCoords[4] = points[x][y + 1][1];
			vertCoords[5] = 4.4f;
			vertCoords[6] = points[x + 1][y + 1][0];
			vertCoords[7] = points[x + 1][y + 1][1];
			vertCoords[8] = 4.4f;
			vertCoords[9] = points[x + 1][y][0];
			vertCoords[10] = points[x + 1][y][1];
			vertCoords[11] = 4.4f;

			pglVertexPointer(3, GL_FLOAT, 0, vertCoords);

			pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	}

	pglEnable(GL_DEPTH_TEST);
	pglEnable(GL_BLEND);
}

// Sryder:	This needs to be called whenever the screen changes resolution in order to reset the screen textures to use
//			a new size
EXPORT void HWRAPI(FlushScreenTextures) (void)
{
	pglDeleteTextures(1, &screentexture);
	pglDeleteTextures(1, &startScreenWipe);
	pglDeleteTextures(1, &endScreenWipe);
	pglDeleteTextures(1, &finalScreenTexture);

	screentexture = 0;
	startScreenWipe = 0;
	endScreenWipe = 0;
	finalScreenTexture = 0;
}

// Create Screen to fade from
EXPORT void HWRAPI(StartScreenWipe) (void)
{
	//INT32 texsize = 2048;
	INT32 texsize = 512;
	boolean firstTime = (startScreenWipe == 0);
/*
	// Use a power of two texture, dammit
	if(screen_width <= 512)
		texsize = 512;
	else if(screen_width <= 1024)
		texsize = 1024;
	else if(screen_width <= 2048)
		texsize = 2048;
*/
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// Create screen texture
	if (firstTime)
		startScreenWipe = SCRTEX_STARTSCREENWIPE;
	pglBindTexture(GL_TEXTURE_2D, startScreenWipe);

	if (firstTime)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		Clamp2D(GL_TEXTURE_WRAP_S);
		Clamp2D(GL_TEXTURE_WRAP_T);
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
	}
	else
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);

	tex_downloaded = startScreenWipe;
}

// Create Screen to fade to
EXPORT void HWRAPI(EndScreenWipe)(void)
{
	//INT32 texsize = 2048;
	INT32 texsize = 512;
	boolean firstTime = (endScreenWipe == 0);
/*
	// Use a power of two texture, dammit
	if(screen_width <= 512)
		texsize = 512;
	else if(screen_width <= 1024)
		texsize = 1024;
	else if(screen_width <= 2048)
		texsize = 2048;
*/
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// Create screen texture
	if (firstTime)
		endScreenWipe = SCRTEX_ENDSCREENWIPE;
	pglBindTexture(GL_TEXTURE_2D, endScreenWipe);

	if (firstTime)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		Clamp2D(GL_TEXTURE_WRAP_S);
		Clamp2D(GL_TEXTURE_WRAP_T);
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
	}
	else
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);

	tex_downloaded = endScreenWipe;
}

// Draw the last scene under the intermission
EXPORT void HWRAPI(DrawIntermissionBG)(void)
{
	float xfix, yfix;
	//INT32 texsize = 2048;
	INT32 texsize = 512;

	const float screenVerts[12] =
	{
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f
	};

	float fix[8];
/*
	if(screen_width <= 2048)
		texsize = 2048;
	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;
*/
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	fix[0] = 0.0f;
	fix[1] = 0.0f;
	fix[2] = 0.0f;
	fix[3] = yfix;
	fix[4] = xfix;
	fix[5] = yfix;
	fix[6] = xfix;
	fix[7] = 0.0f;

	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	pglBindTexture(GL_TEXTURE_2D, screentexture);
	pglColor4ubv(white);
	pglTexCoordPointer(2, GL_FLOAT, 0, fix);
	pglVertexPointer(3, GL_FLOAT, 0, screenVerts);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	tex_downloaded = screentexture;
}

// Do screen fades!
EXPORT void HWRAPI(DoScreenWipe)(void)
{
	//INT32 texsize = 2048;
	INT32 texsize = 512;
	float xfix, yfix;

	INT32 fademaskdownloaded = tex_downloaded; // the fade mask that has been set

	const float screenVerts[12] =
	{
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f
	};

	float fix[8];

	const float defaultST[8] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
/*
	// Use a power of two texture, dammit
	if(screen_width <= 2048)
		texsize = 2048;
	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;
*/
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;


	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	fix[0] = 0.0f;
	fix[1] = 0.0f;
	fix[2] = 0.0f;
	fix[3] = yfix;
	fix[4] = xfix;
	fix[5] = yfix;
	fix[6] = xfix;
	fix[7] = 0.0f;

	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	SetBlend(PF_Modulated|PF_NoDepthTest);
	pglEnable(GL_TEXTURE_2D);

	// Draw the original screen
	pglBindTexture(GL_TEXTURE_2D, startScreenWipe);
	pglColor4ubv(white);
	pglTexCoordPointer(2, GL_FLOAT, 0, fix);
	pglVertexPointer(3, GL_FLOAT, 0, screenVerts);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	SetBlend(PF_Modulated|PF_Translucent|PF_NoDepthTest);

	// Draw the end screen that fades in
	pglActiveTexture(GL_TEXTURE0);
	pglEnable(GL_TEXTURE_2D);
	pglBindTexture(GL_TEXTURE_2D, endScreenWipe);
	pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	pglActiveTexture(GL_TEXTURE1);
	pglEnable(GL_TEXTURE_2D);
	pglBindTexture(GL_TEXTURE_2D, fademaskdownloaded);

	pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	pglClientActiveTexture(GL_TEXTURE0);
	pglTexCoordPointer(2, GL_FLOAT, 0, fix);
	pglVertexPointer(3, GL_FLOAT, 0, screenVerts);
	pglClientActiveTexture(GL_TEXTURE1);
	pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	pglTexCoordPointer(2, GL_FLOAT, 0, defaultST);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	pglDisable(GL_TEXTURE_2D); // disable the texture in the 2nd texture unit
	pglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	pglActiveTexture(GL_TEXTURE0);
	pglClientActiveTexture(GL_TEXTURE0);
	tex_downloaded = endScreenWipe;
}

// Create a texture from the screen.
EXPORT void HWRAPI(MakeScreenTexture) (void)
{
	//INT32 texsize = 2048;
	INT32 texsize = 512;
	boolean firstTime = (screentexture == 0);
/*
	// Use a power of two texture, dammit
	if(screen_width <= 512)
		texsize = 512;
	else if(screen_width <= 1024)
		texsize = 1024;
	else if(screen_width <= 2048)
		texsize = 2048;
*/
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// Create screen texture
	if (firstTime)
		screentexture = SCRTEX_SCREENTEXTURE;
	pglBindTexture(GL_TEXTURE_2D, screentexture);

	if (firstTime)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		Clamp2D(GL_TEXTURE_WRAP_S);
		Clamp2D(GL_TEXTURE_WRAP_T);
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
	}
	else
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);

	tex_downloaded = screentexture;
}

EXPORT void HWRAPI(RenderVhsEffect) (INT16 upbary, INT16 downbary, UINT8 updistort, UINT8 downdistort, UINT8 barsize)
{
	//INT32 texsize = 2048;
	INT32 texsize = 512;
	float xfix, yfix;
	float fix[8];
	GLubyte color[4] = {255, 255, 255, 255};
	float i;

	float screenVerts[12] =
	{
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f
	};
/*
	if(screen_width <= 2048)
		texsize = 2048;
	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;
*/
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;


	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	// Slight fuzziness
	MakeScreenTexture();
	SetBlend(PF_Modulated|PF_Translucent|PF_NoDepthTest);
	pglBindTexture(GL_TEXTURE_2D, screentexture);

	fix[2] = (float)(rand() / 255) / -22000 * xfix;

	for (i = 0; i < 1; i += 0.01)
	{
		color[3] = (float)(rand() * 70 / 256) + 40;
		fix[0] = fix[2];
		fix[2] = (float)(rand() / 255) / -22000 * xfix;
		fix[6] = fix[0] + xfix;
		fix[4] = fix[2] + xfix;
		fix[1] = fix[7] = i*yfix;
		fix[3] = fix[5] = (i+0.015)*yfix;

		screenVerts[1] = screenVerts[10] = 2*i - 1.0f;
		screenVerts[4] = screenVerts[7] = screenVerts[1] + 0.03;

		pglColor4ubv(color);

		pglTexCoordPointer(2, GL_FLOAT, 0, fix);
		pglVertexPointer(3, GL_FLOAT, 0, screenVerts);
		pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	// Upward bar
	MakeScreenTexture();
	pglBindTexture(GL_TEXTURE_2D, screentexture);
	color[0] = color[1] = color[2] = 190;
	color[3] = 250;
	pglColor4ubv(color);

	fix[0] = 0.0f;
	fix[6] = xfix;
	fix[2] = (float)updistort / screen_width * xfix;
	fix[4] = fix[2] + fix[6];

	screenVerts[1] = screenVerts[10] = 2.0f*upbary/screen_height - 1.0f;
	screenVerts[4] = screenVerts[7] = screenVerts[1] + (float)barsize/screen_height;

	fix[1] = fix[7] = (float)upbary/screen_height * yfix;
	fix[3] = fix[5] = fix[1] + (float)barsize/2/screen_height * yfix;

	pglTexCoordPointer(2, GL_FLOAT, 0, fix);
	pglVertexPointer(3, GL_FLOAT, 0, screenVerts);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	fix[1] = fix[7] += (fix[3] - fix[7])*2;
	screenVerts[1] = screenVerts[10] += (screenVerts[4] - screenVerts[1])*2;
	pglTexCoordPointer(2, GL_FLOAT, 0, fix);
	pglVertexPointer(3, GL_FLOAT, 0, screenVerts);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// Downward bar
	MakeScreenTexture();
	pglBindTexture(GL_TEXTURE_2D, screentexture);

	fix[0] = 0.0f;
	fix[6] = xfix;
	fix[2] = (float)downdistort / screen_width * -xfix;
	fix[4] = fix[2] + fix[6];

	screenVerts[1] = screenVerts[10] = 2.0f*downbary/screen_height - 1.0f;
	screenVerts[4] = screenVerts[7] = screenVerts[1] + (float)barsize/screen_height;

	fix[1] = fix[7] = (float)downbary/screen_height * yfix;
	fix[3] = fix[5] = fix[1] + (float)barsize/2/screen_height * yfix;

	pglTexCoordPointer(2, GL_FLOAT, 0, fix);
	pglVertexPointer(3, GL_FLOAT, 0, screenVerts);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	fix[1] = fix[7] += (fix[3] - fix[7])*2;
	screenVerts[1] = screenVerts[10] += (screenVerts[4] - screenVerts[1])*2;
	pglTexCoordPointer(2, GL_FLOAT, 0, fix);
	pglVertexPointer(3, GL_FLOAT, 0, screenVerts);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

EXPORT void HWRAPI(MakeScreenFinalTexture) (void)
{
	//INT32 texsize = 2048;
	INT32 texsize = 512;
	boolean firstTime = (finalScreenTexture == 0);
/*
	// Use a power of two texture, dammit
	if(screen_width <= 512)
		texsize = 512;
	else if(screen_width <= 1024)
		texsize = 1024;
	else if(screen_width <= 2048)
		texsize = 2048;
*/
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	// Create screen texture
	if (firstTime)
		finalScreenTexture = SCRTEX_FINALSCREENTEXTURE;
	pglBindTexture(GL_TEXTURE_2D, finalScreenTexture);

	if (firstTime)
	{
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		Clamp2D(GL_TEXTURE_WRAP_S);
		Clamp2D(GL_TEXTURE_WRAP_T);
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
	}
	else
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);

	tex_downloaded = finalScreenTexture;
}

EXPORT void HWRAPI(DrawScreenFinalTexture)(int width, int height)
{
	float xfix, yfix;
	float origaspect, newaspect;
	float xoff = 1, yoff = 1; // xoffset and yoffset for the polygon to have black bars around the screen
	FRGBAFloat clearColour;
	//INT32 texsize = 2048;
	INT32 texsize = 512;

	float off[12];
	float fix[8];
/*
	if(screen_width <= 2048)
		texsize = 2048;
	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;
*/
	// look for power of two that is large enough for the screen
	while (texsize < screen_width || texsize < screen_height)
		texsize <<= 1;

	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	origaspect = (float)screen_width / screen_height;
	newaspect = (float)width / height;
	if (origaspect < newaspect)
	{
		xoff = origaspect / newaspect;
		yoff = 1;
	}
	else if (origaspect > newaspect)
	{
		xoff = 1;
		yoff = newaspect / origaspect;
	}

	off[0] = -xoff;
	off[1] = -yoff;
	off[2] = 1.0f;
	off[3] = -xoff;
	off[4] = yoff;
	off[5] = 1.0f;
	off[6] = xoff;
	off[7] = yoff;
	off[8] = 1.0f;
	off[9] = xoff;
	off[10] = -yoff;
	off[11] = 1.0f;


	fix[0] = 0.0f;
	fix[1] = 0.0f;
	fix[2] = 0.0f;
	fix[3] = yfix;
	fix[4] = xfix;
	fix[5] = yfix;
	fix[6] = xfix;
	fix[7] = 0.0f;

	pglViewport(0, 0, width, height);

	clearColour.red = clearColour.green = clearColour.blue = 0;
	clearColour.alpha = 1;
	ClearBuffer(true, false, &clearColour);
	pglBindTexture(GL_TEXTURE_2D, finalScreenTexture);

	pglColor4ubv(white);
	pglTexCoordPointer(2, GL_FLOAT, 0, fix);
	pglVertexPointer(3, GL_FLOAT, 0, off);

	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	tex_downloaded = finalScreenTexture;
}

#endif //HWRENDER
