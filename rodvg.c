/*
* Copyright (c) 2012, Freescale Semiconductor, Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this list
* of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this
* list of conditions and the following disclaimer in the documentation and/or
* other materials provided with the distribution.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** \addtogroup Main
 *  @{
 ****************************************************************************/
/**  \file      main.c
 *   \author    Hugo Osornio
 *   \date      \$Date: 2012/11/05 $
 *
 *    language  C
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include "VG/openvg.h"
#include "VG/vgu.h"

#include "EGL/egl.h"
#include "FSL/fsl_egl.h"

#include <sys/time.h>
//-------------------------------------------------------------------
struct timeval now;
float time0,time1,dt;

extern const VGint     pathCount;
extern const VGint     commandCounts[];
extern const VGubyte*  commandArrays[];
extern const VGfloat*  dataArrays[];
extern const VGfloat*  styleArrays[];

int currentFrame;

VGPath *tigerPaths = NULL;
VGPaint tigerStroke;
VGPaint tigerFill;

VGfloat sx=1.0f, sy=1.0f;
VGfloat tx=0.0f, ty=0.0f;
VGfloat ang = 0.0f;
int animate = 1;
char mode = 'z';

VGfloat startX = 0.0f;
VGfloat startY = 0.0f;
VGfloat clickX = 0.0f;
VGfloat clickY = 0.0f;

const char commands[] =
  "Click & drag mouse to change\n"
  "value for current mode\n\n"
  "H - this help\n"
  "Z - zoom mode\n"
  "P - pan mode\n"
  "SPACE - animation pause\\play\n";
/*****************************************************************************
 *  local defines
 ****************************************************************************/

#define XRES width
#define YRES height

/*****************************************************************************
 *  local types
 ****************************************************************************/

struct appAttribs
{
	int winWidth;
	int winHeight;
	int redSize;
	int greenSize;
	int blueSize;
	int alphaSize;
	int depthSize;
	int stencilSize;
	int samples;
	int openvgbit;
	int frameCount;
	int save;
	int fps;
};

/*****************************************************************************
 *  local prototypes
 ****************************************************************************/

/*****************************************************************************
 *  exported variables
 ****************************************************************************/

EGLint width;
EGLint height;


/*****************************************************************************
 *  local variables
 ****************************************************************************/

/* VG EGL variable*/
EGLDisplay			egldisplay;
EGLConfig			eglconfig;
EGLConfig			eglconfigs[64];
EGLSurface			eglsurface;
EGLContext			eglcontext;
EGLNativeWindowType eglNativeWindow;
EGLNativeDisplayType eglNativeDisplayType;
volatile sig_atomic_t quit = 0;

static struct appAttribs const cmdAttrib = {1280,480,8,8,8,8,-1,-1,-1,2,0,0,0};

//Define a triangle and a Circle figure!
/*OpenVG Primitives are:
	PATHS: Integrated by Vertices, Segments and points.
	IMAGES: Bitmaps.
The triangle that we will draw will consist of 3 vertices and 4 segments
*/

VGPath vgTriangle, vgQuadCurve, vgCubicCurve, vgArc;
VGPaint vgTrianglePaint, vgQuadCurvePaint, vgCubicCurvePaint, vgArcPaint, strokePaint;

/*
	The segments of a path are described as a series of commands stored in
	an array of bytes, each command will consume a series of points (defined next)
*/
VGbyte vgTriangleSegments[] =
{
	VG_MOVE_TO_ABS,
	VG_LINE_TO_ABS,
	VG_LINE_TO_ABS,
	VG_CLOSE_PATH,
};

VGbyte vgQuadCurveSegments[] =
{
	VG_MOVE_TO_ABS,
	VG_LINE_TO_ABS,
	VG_QUAD_TO_ABS,
	VG_CLOSE_PATH,
};

VGbyte vgCubicCurveSegments[] =
{
	VG_MOVE_TO_ABS,
	VG_LINE_TO_ABS,
	VG_CUBIC_TO_ABS,
	VG_LINE_TO_ABS,
	VG_CLOSE_PATH,
};

VGbyte vgArcSegments[] =
{
	VG_MOVE_TO_ABS,
	VG_LINE_TO_ABS,
	VG_SCCWARC_TO_ABS,
	VG_CLOSE_PATH,
};



/*
	The path needs a list of points, a command/segment can consume from 0 to 6 values depending on the type of command
*/
VGfloat vgTrianglePoints[] = 
{
	//VG_MOVE_TO_ABS consumes 2
	100.0f,
	100.0f,
	//VG_LINE_TO_ABS consumes 2
	300.0f,
	300.0f,
	//VG_LINE_TO_ABS consumes 2
    500.0f,
	100.0f
};

VGfloat vgQuadCurvePoints[] = 
{
	//VG_MOVE_TO_ABS consumes 2
	100.0f,
	400.0f,
	//VG_LINE_TO_ABS consumes 2
	100.0f,
	600.0f,
	//VG_QUAD_TO_ABS consumes 4
	//Control Point
	300.0f,
	700.0f,
	//End Point
	500.0f,
	600.0f,
};

VGfloat vgCubicCurvePoints[] = 
{
	//VG_MOVE_TO_ABS consumes 2
	620.0f,
	400.0f,
	//VG_LINE_TO_ABS consumes 2
	620.0f,
	500.0f,
	//VG_QUAD_TO_ABS consumes 6
	//Control Point 1
	680.0f,
	700.0f,
	//Control Point 2
	700.0f,
	200.0f,
	//End point
	800.0f,
	500.0f,
	//VG_LINE_TO_ABS consumes 2
	800.0f,
	400.0f,

};

VGfloat vgArcPoints[] = 
{
	//VG_MOVE_TO_ABS consumes 2
	620.0f,
	200.0f,
	//VG_LINE_TO_ABS consumes 2
	620.0f,
	100.0f,
	//ARC TO consumes 5
	//H
	10.0f,
	//V
	10.0f,
	//Rotate
	0.0f,
	//End Point
	700.0f,
	150.0f,
};


/*****************************************************************************
 *  local functions
 ****************************************************************************/

void sighandler(int signal)
{
	printf("Caught signal %d, setting flaq to quit.\n", signal);
	quit = 1;
}


void deinit(void)
{
	printf("Cleaning up...\n");
	eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	assert(eglGetError() == EGL_SUCCESS);
	eglDestroyContext(egldisplay, eglcontext);
	eglDestroySurface(egldisplay, eglsurface);	
	fsl_destroywindow(eglNativeWindow, eglNativeDisplayType);
	eglTerminate(egldisplay);
	assert(eglGetError() == EGL_SUCCESS);
	eglReleaseThread();	
}


void OvgApp_Init(void)
{
    VGfloat afClearColour[] = { 0.6f, 0.8f, 1.0f, 1.0f };
	vgSetfv(VG_CLEAR_COLOR, 4, afClearColour);

	//Create the paths
	vgTriangle = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 4, 6, VG_PATH_CAPABILITY_APPEND_TO);
	vgQuadCurve = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 4, 8, VG_PATH_CAPABILITY_APPEND_TO);
	vgCubicCurve = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 5, 12, VG_PATH_CAPABILITY_APPEND_TO);
	vgArc = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 4, 9, VG_PATH_CAPABILITY_APPEND_TO);
	
	//Fill the path data with the commands, the points are used automatically
	vgAppendPathData(vgTriangle, 4, (const VGubyte *)vgTriangleSegments, vgTrianglePoints);
	vgAppendPathData(vgQuadCurve, 4, (const VGubyte *)vgQuadCurveSegments, vgQuadCurvePoints);
	vgAppendPathData(vgCubicCurve, 5, (const VGubyte *)vgCubicCurveSegments, vgCubicCurvePoints);
	vgAppendPathData(vgArc, 4, (const VGubyte *)vgArcSegments, vgArcPoints);
}

void OvgApp_Draw(void)
{
  vgClear(0, 0, 1280, 1080);
	
	//Set transformation matrix mode
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	//Loads identity matrix
	vgLoadIdentity();
	vgDrawPath(vgTriangle, VG_STROKE_PATH);
	vgDrawPath(vgQuadCurve, VG_STROKE_PATH);
	vgDrawPath(vgCubicCurve, VG_STROKE_PATH);
	vgDrawPath(vgArc, VG_STROKE_PATH);
	vgFinish();
}

int init(void)
{
	unsigned char i;
	
	static const EGLint s_configAttribs[] =
	{
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE, 	8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE, 	8,
		EGL_SAMPLES,		0,
		EGL_RENDERABLE_TYPE,    EGL_OPENVG_BIT,
		EGL_NONE
	};

	EGLint numconfigs;

	eglNativeDisplayType = fsl_getNativeDisplay();
	egldisplay = eglGetDisplay(eglNativeDisplayType);
	eglInitialize(egldisplay, NULL, NULL);
	assert(eglGetError() == EGL_SUCCESS);
	printf("\nEGL Display properly Got\n");
	eglBindAPI(EGL_OPENVG_API);

	eglChooseConfig(egldisplay, s_configAttribs, &eglconfigs[0], 1, &numconfigs);
	assert(eglGetError() == EGL_SUCCESS);
	printf("\nChoose configs OK\n");
	assert(numconfigs == 1);
	printf("\nNUM Configs OK\n");
	
	for (i = 0; i < numconfigs; i++)
    {

      int redSize = 0;
      int greenSize = 0;
      int blueSize = 0;
      int alphaSize = 0;
      int depthSize = 0;
      int stencilSize = 0;
      int samples = 0;
      int id = 0;

      int openglbit = 0;
      int surfacebit = 0;

      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_RED_SIZE, &redSize);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_GREEN_SIZE, &greenSize);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_BLUE_SIZE, &blueSize);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_ALPHA_SIZE, &alphaSize);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_DEPTH_SIZE, &depthSize);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_STENCIL_SIZE, &stencilSize);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_SAMPLES, &samples);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_RENDERABLE_TYPE, &openglbit);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_CONFIG_ID, &id);
      eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_SURFACE_TYPE, &surfacebit);

      if (((cmdAttrib.redSize != -1)&&(redSize != cmdAttrib.redSize)) ||
          ((cmdAttrib.greenSize != -1)&&(greenSize != cmdAttrib.greenSize)) ||
          ((cmdAttrib.blueSize != -1)&&(blueSize != cmdAttrib.blueSize)) ||
          ((cmdAttrib.alphaSize != -1)&&(alphaSize != cmdAttrib.alphaSize)) ||
          ((cmdAttrib.depthSize != -1)&&(depthSize != cmdAttrib.depthSize)) ||
          ((cmdAttrib.stencilSize != -1)&&(stencilSize != cmdAttrib.stencilSize)) ||
          ((cmdAttrib.samples != -1)&&(samples != cmdAttrib.samples))
          )
	{
          printf("\nconfig %i no match\n",i);
          printf("id=%d, a,b,g,r=%d,%d,%d,%d, d,s=%d,%d, AA=%d,openglbit=%d, surfacebit=%d\n",
                 id, alphaSize, blueSize, greenSize, redSize, depthSize, stencilSize,
                 samples, openglbit, surfacebit);
		
	}else
	{
          eglconfig = eglconfigs[i];
          printf("%s:use config 0x%x\n", __func__, eglconfig);		

          if (eglconfig != NULL)
            {
              printf("id=%d, a,b,g,r=%d,%d,%d,%d, d,s=%d,%d, AA=%d,openvgbit=%d, surfacebit=%d\n",
                     id, alphaSize, blueSize, greenSize, redSize, depthSize, stencilSize,
                     samples, openglbit, surfacebit);
            }
          break;
	}
    }
	
	eglNativeWindow = fsl_createwindow(egldisplay, eglNativeDisplayType);	
	printf("\nCreated Window OK\n");
	assert(eglNativeWindow);	

	eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, eglNativeWindow, NULL);

	assert(eglGetError() == EGL_SUCCESS);
	printf("\nCreated Surface OK\n");
	EGLint ContextAttribList[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };
	eglcontext = eglCreateContext(egldisplay, eglconfig, 0, (const EGLint*)NULL);
	assert(eglGetError() == EGL_SUCCESS);
	printf("\nCreated Context OK\n");
	eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext);
	assert(eglGetError() == EGL_SUCCESS);
	printf("\nMade current Context OK\n");
    return 1;
}
//-------------------------------------------------------------------
void loadTiger()
{
  int i;
  VGPath temp;
  
//  temp = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 4, 6, VG_PATH_CAPABILITY_APPEND_TO); 
  tigerPaths = (VGPath*)malloc(pathCount * sizeof(VGPath));
  vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
  //vgTranslate(-100,100);
  //vgScale(1,-1);
  
  for (i=0; i<pathCount; ++i) {
    
		tigerPaths[i] = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
			1,0,0,0, VG_PATH_CAPABILITY_ALL);
		vgAppendPathData(tigerPaths[i], commandCounts[i],
			commandArrays[i], dataArrays[i]);
  }
  
	tigerStroke = vgCreatePaint();
	tigerFill = vgCreatePaint();
	vgSetPaint(tigerStroke, VG_STROKE_PATH);
	vgSetPaint(tigerFill, VG_FILL_PATH);

	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgTranslate((VGfloat)(width/2 -50.0), (VGfloat)(height/2 + 100.0));
	vgScale(1.0,-1.0);
	//vgScale(0.5,0.5);

	VGfloat clearColor[] = {1,1,1,1};
	vgSetfv(VG_CLEAR_COLOR, 4, clearColor);



//////////////////////////////////////


//////////////////////////////////////

/*
//Create the paths
vgTriangle   = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 4, 6, VG_PATH_CAPABILITY_APPEND_TO);
vgQuadCurve  = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 4, 8, VG_PATH_CAPABILITY_APPEND_TO);
vgCubicCurve = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 5,12, VG_PATH_CAPABILITY_APPEND_TO);
vgArc        = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 4, 9, VG_PATH_CAPABILITY_APPEND_TO);
//Fill the path data with the commands, the points are used automatically
vgAppendPathData(vgTriangle,   4, (const VGubyte *)vgTriangleSegments,   vgTrianglePoints);
vgAppendPathData(vgQuadCurve,  4, (const VGubyte *)vgQuadCurveSegments,  vgQuadCurvePoints);
vgAppendPathData(vgCubicCurve, 5, (const VGubyte *)vgCubicCurveSegments, vgCubicCurvePoints);
vgAppendPathData(vgArc,        4, (const VGubyte *)vgArcSegments,        vgArcPoints);
*/

}
//-------------------------------------------------------------------
void displayTiger(float interval)
{
  int i;
  const VGfloat *style;
  VGfloat clearColor[] = {1,0,0,0};
  
  if (animate) {
    ang += interval * 360 * 0.1f;
    if (ang > 360) ang -= 360;
  }
  
  vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
  vgClear(0,0,width,height);
  
  vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
  vgLoadIdentity();
  vgTranslate(width/2 + tx,height/2 + ty);
  vgScale(sx, sy);
  vgRotate(ang);

//vgDrawPath(vgTriangle,  VG_STROKE_PATH);
//vgDrawPath(vgQuadCurve, VG_STROKE_PATH);
  
  for (i=0; i<pathCount; ++i) {
    
    style = styleArrays[i];
    vgSetParameterfv(tigerStroke, VG_PAINT_COLOR, 4, &style[0]);
    vgSetParameterfv(tigerFill, VG_PAINT_COLOR, 4, &style[4]);
    vgSetf(VG_STROKE_LINE_WIDTH, style[8]);
    vgDrawPath(tigerPaths[i], (VGint)style[9]); // Bingo!!, Draw it!! 
  }
	vgFinish();
  vgFlush();


}
/*****************************************************************************
 *  exported functions
 ****************************************************************************/
int main(int argc, char *argv[])
{
      
//DOES NOT WORK YET!!!
  
  width=1024;
  height=768;
  
  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);
  
  assert( init() );
  //OvgApp_Init();
  
  // clear both the front and back buffers
  vgClear(0, 0, XRES,YRES);
  eglSwapBuffers(egldisplay, eglsurface);
  vgClear(0, 0, XRES,YRES);
  eglSwapBuffers(egldisplay, eglsurface);
  vgClear(0, 0, XRES,YRES);
  eglSwapBuffers(egldisplay, eglsurface);
  vgClear(0, 0, XRES,YRES);
  eglSwapBuffers(egldisplay, eglsurface);

  loadTiger();
  printf("w=%d  h=%d  animate=%d  pathCount=%d\n",width,height,animate,pathCount);
    
  while (!quit)
    {
    //OvgApp_Draw();
    displayTiger(0.5);
    eglSwapBuffers(egldisplay, eglsurface);
    currentFrame++;
    if ((currentFrame%500)==0) 
      {
      gettimeofday(&now,NULL); time1=now.tv_sec+now.tv_usec/1e6; dt=time1-time0; time0=time1;
      printf("Frame=%d   FPS=%6.3f\n",currentFrame,500/dt);
      }
    }
  deinit();
  return 0;
}

void Cleanup()
{

}
