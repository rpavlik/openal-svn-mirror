#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <AL/alut.h>
#include <GL/glut.h>

#if HAVE___ATTRIBUTE__
#define ATTRIBUTE_UNUSED(x) x __attribute__((unused))
#define ATTRIBUTE_NORETURN __attribute__((noreturn))
#else
#define ATTRIBUTE_UNUSED(x) x
#define ATTRIBUTE_NORETURN
#endif

static ALuint leftSource = 0;
static ALuint rightSource = 0;
static ALfloat leftPosition[3] = { -4.0, 0.0, 4.0 };
static ALfloat rightPosition[3] = { 4.0, 0.0, 4.0 };

static ALboolean rightIsPlaying = AL_FALSE;

static GLfloat red[3] = { 1.0, 0.0, 0.0 };
static GLfloat white[3] = { 1.0, 1.0, 1.0 };

/* Switch between left and right boom every two seconds. */
static unsigned int msecsDelay = 2000;

ATTRIBUTE_NORETURN static void
safeExit (int status)
{
  alutExit ();
  exit (status);
}

static void
coloredCone (GLfloat *color, GLfloat *position)
{
  glColor3fv (color);
  glPushMatrix ();
  glTranslatef (position[0], position[1], position[2]);
  glRotatef (180, 0.0, 1.0, 0.0);
  glutWireCone (1.0, 2.0, 20, 20);
  glPopMatrix ();
}

static void
coloredStrokeString (GLfloat *color, const char *string)
{
  glColor3fv (color);
  while (*string != '\0')
    {
      glutStrokeCharacter (GLUT_STROKE_ROMAN, (int) (*string++));
    }
}

static void
display (void)
{
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  gluLookAt (0.0, 4.0, 16.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

  /* Draw radiation cones. */
  coloredCone ((rightIsPlaying == AL_FALSE) ? red : white, leftPosition);
  coloredCone ((rightIsPlaying == AL_TRUE) ? red : white, rightPosition);

  /* Let's draw some text. */
  glMatrixMode (GL_PROJECTION);
  glPushMatrix ();
  glLoadIdentity ();
  glOrtho (0, 640, 0, 480, -1.0, 1.0);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (10.0, 10.0, 0.0);
  glScalef (0.2, 0.2, 0.2);
  coloredStrokeString (red, "Red");
  coloredStrokeString (white, " is Active");

  glMatrixMode (GL_PROJECTION);
  glPopMatrix ();

  glutSwapBuffers ();
}

static void
keyboard (unsigned char key, int ATTRIBUTE_UNUSED (x),
          int ATTRIBUTE_UNUSED (y))
{
  switch (key)
    {
    case 27:
      safeExit (EXIT_SUCCESS);
      break;
    default:
      break;
    }
}

static void
reshape (int w, int h)
{
  glViewport (0, 0, w, h);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (60.0f, (float) w / (float) h, 0.1f, 1024.0f);
}

static void
timer (int ATTRIBUTE_UNUSED (value))
{
  if (rightIsPlaying == AL_TRUE)
    {
      alSourcePlay (leftSource);
      rightIsPlaying = AL_FALSE;
    }
  else
    {
      alSourcePlay (rightSource);
      rightIsPlaying = AL_TRUE;
    }
  glutPostRedisplay ();
  glutTimerFunc (msecsDelay, timer, 0);
}

static void
init (const char *fileName)
{
  ALuint boom;
  ALenum error;
  ALuint sources[2];

  glClearColor (0.0, 0.0, 0.0, 0.0);
  glShadeModel (GL_SMOOTH);

  boom = alutCreateBufferFromFile (fileName);
  if (boom == AL_NONE)
    {
      error = alutGetError ();
      fprintf (stderr, "Error loading file %s: '%s'\n",
               fileName, alutGetErrorString (error));
      safeExit (EXIT_FAILURE);
    }

  alGenSources (2, sources);
  if (alGetError () != AL_NO_ERROR)
    {
      fprintf (stderr, "aldemo: couldn't generate sources\n");
      safeExit (EXIT_FAILURE);
    }

  leftSource = sources[0];
  rightSource = sources[1];

  alSourcefv (leftSource, AL_POSITION, leftPosition);
  alSourcei (leftSource, AL_BUFFER, boom);

  alSourcefv (rightSource, AL_POSITION, rightPosition);
  alSourcei (rightSource, AL_BUFFER, boom);

  timer (0);
}

int
main (int argc, char *argv[])
{
  /* Initialize GLUT. */
  glutInit (&argc, argv);

  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize (640, 480);
  glutInitWindowPosition (0, 0);
  glutCreateWindow (argv[0]);

  glutDisplayFunc (display);
  glutKeyboardFunc (keyboard);
  glutReshapeFunc (reshape);

  /* Initialize ALUT. */
  alutInit (&argc, argv);

  init ("sounds/boom.wav");

  glutMainLoop ();

  return EXIT_SUCCESS;
}
