#include "gl.h"
#include "glu.h"
#include "glut.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "common.h"
#ifdef MACOS
#include <Events.h>
#include <Timer.h>
#endif
#ifdef MAC_OS_X
#include <sys/time.h>
#endif

// Globals
AVEnvironment g_Env;

AudioEnvPtr Object::audioObject = NULL; // static pointer to an audio object used by Object::Move

// Constants
const float STRIDE = 0.9f;
const float PI = 3.1415926535f;

float TimeDiff()
{
#ifdef MACOS
    float newVal;
    static UnsignedWide lastTC;
    UnsignedWide oldTC;
    
    oldTC = lastTC;
    Microseconds(&lastTC);
    
    if (oldTC.hi == lastTC.hi)
    {
        newVal = ((float)lastTC.lo - (float)oldTC.lo)/1000000;
    } else
    {
        newVal = (((float)lastTC.hi-(float)oldTC.hi)*0xffffffff+((float)lastTC.lo - (float)oldTC.lo))/1000000;
    }
    return newVal;
#endif
#ifdef MAC_OS_X
   static double oldTime = 0;
   double newTime;
   timeval newTimeVal;
   float returnTimeDiff;
   bool returnZero = false;

   if (oldTime == 0) { returnZero = true; } else { returnZero = false; }

   gettimeofday(&newTimeVal, NULL);
   newTime = (double) newTimeVal.tv_sec + ((double) newTimeVal.tv_usec / 1000000);

   returnTimeDiff =  (float) (newTime - oldTime);
   oldTime = newTime;

   if (returnZero == false) {
      return returnTimeDiff;
   } else {
      return 0.0f;
   }
#endif
}

void display(void)
{
    static int numCalls = 10;
    static float lastTD = 0.02;
    
    if (numCalls++ >= 10) // calculate new TimeDiff value every ten frames (smoother on MacOS)
    {
    	float td = TimeDiff() / 10;
    	lastTD = td;
    	numCalls = 0;
    }
    g_Env.DrawBuffer(lastTD);
    if (lastTD != 0) { g_Env.SetFPS(1/lastTD); }
    g_Env.PlaceCamera();
    glutSwapBuffers();
}

void myinit (void)
{
	g_Env.Init();
}

void parsekey(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 27:  	exit(0); break;
		case 'f':   g_Env.ToggleFPS(); break;
		case 'e':   g_Env.IncrementEnv(); break;
		case 'z': 	g_Env.Step((-PI/2), STRIDE); break;
		case 'x': 	g_Env.Step((PI/2), STRIDE); break;
	}
}

void parsekey_special(int key, int x, int y)
{
	switch (key)
	{
		case GLUT_KEY_UP:			g_Env.Step(0, STRIDE); break;
		case GLUT_KEY_DOWN:			g_Env.Step(3.14f, STRIDE); break;
		case GLUT_KEY_RIGHT:		g_Env.Turn(0.15f * STRIDE); break;
		case GLUT_KEY_LEFT:			g_Env.Turn(-0.15f * STRIDE); break;
	}
}

void SetCamera()
{
    g_Env.PlaceCamera();
}

void Animate(void)
{
    SetCamera();
    glutPostRedisplay();
}

void myReshape(int w, int h)
{
    SetCamera();
    g_Env.ChangeView(w,h);
}

int main(int argc, char *argv[])
{
	int sz;
	
	if (argc > 1)
		sz = atoi(argv[1]);
	else
		sz = 200;

        glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(200, 100);
	glutInitWindowSize(sz*1.33*3, sz*3);
	glutCreateWindow("XL Demo");
	glutDisplayFunc(display);
	glutKeyboardFunc(parsekey);
	glutSpecialFunc(parsekey_special);
	glutReshapeFunc(myReshape);
	glutIdleFunc(Animate);
	
	myinit();
	glutSwapBuffers();
	glutMainLoop();
}
