/*
 * openal-info: Display information about ALC and AL.
 *
 * Idea based on glxinfo for OpenGL.
 * Initial OpenAL version by Erik Hofman <erik@ehofman.com>.
 * Further hacked by Sven Panne <sven.panne@aedion.de>.
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <AL/alut.h>

#if HAVE___ATTRIBUTE__
#define ATTRIBUTE_NORETURN __attribute__((noreturn))
#else
#define ATTRIBUTE_NORETURN
#endif

static const int indentation = 4;
static const int maxmimumWidth = 79;

static void
printChar (int c, int *width)
{
  putchar (c);
  *width = (c == '\n') ? 0 : (*width + 1);
}

static void
indent (int *width)
{
  int i;
  for (i = 0; i < indentation; i++)
    {
      printChar (' ', width);
    }
}

static void
printExtensions (const char *header, char separator, const char *extensions)
{
  int width = 0, start = 0, end = 0;

  printf ("%s:\n", header);
  if (extensions == NULL || extensions[0] == '\0')
    {
      return;
    }

  indent (&width);
  while (1)
    {
      if (extensions[end] == separator || extensions[end] == '\0')
        {
          if (width + end - start + 2 > maxmimumWidth)
            {
              printChar ('\n', &width);
              indent (&width);
            }
          while (start < end)
            {
              printChar (extensions[start], &width);
              start++;
            }
          if (extensions[end] == '\0')
            {
              break;
            }
          start++;
          end++;
          if (extensions[end] == '\0')
            {
              break;
            }
          printChar (',', &width);
          printChar (' ', &width);
        }
      end++;
    }
  printChar ('\n', &width);
}

ATTRIBUTE_NORETURN static void
die (const char *kind, const char *description)
{
  fprintf (stderr, "%s error %s occured\n", kind, description);
  exit (EXIT_FAILURE);
}

static void
checkForErrors (void)
{
  {
    ALenum error = alutGetError ();
    if (error != ALUT_ERROR_NO_ERROR)
      {
        die ("ALUT", alutGetErrorString (error));
      }
  }
  {
    ALCdevice *device = alcGetContextsDevice (alcGetCurrentContext ());
    ALCenum error = alcGetError (device);
    if (error != ALC_NO_ERROR)
      {
        die ("ALC", (const char *) alcGetString (device, error));
      }
  }
  {
    ALenum error = alGetError ();
    if (error != AL_NO_ERROR)
      {
        die ("AL", (const char *) alGetString (error));
      }
  }
}

static const char *
getStringALC (ALCdevice *device, ALCenum param)
{
  const char *s = (const char *) alcGetString (device, param);
  checkForErrors ();
  return s;
}

static const char *
getStringAL (ALenum param)
{
  const char *s = (const char *) alGetString (param);
  checkForErrors ();
  return s;
}

static void
printALUTInfo (void)
{
  ALint major, minor;
  const char *s;

  major = alutGetMajorVersion ();
  minor = alutGetMinorVersion ();
  checkForErrors ();
  printf ("ALUT version: %d.%d\n", (int) major, (int) minor);

  s = alutGetMIMETypes (ALUT_LOADER_BUFFER);
  checkForErrors ();
  printExtensions ("ALUT buffer loaders", ',', s);

  s = alutGetMIMETypes (ALUT_LOADER_MEMORY);
  checkForErrors ();
  printExtensions ("ALUT memory loaders", ',', s);
}

static void
printDevices (ALCenum which, const char *kind)
{
  const char *s = getStringALC (NULL, which);
  printf ("available %sdevices:\n", kind);
  while (*s != '\0')
    {
      printf ("    %s\n", s);
      while (*s++ != '\0')
        ;
    }
}

static void
printALCInfo (void)
{
  ALCint major, minor;
  ALCdevice *device;

  if (alcIsExtensionPresent (NULL, (const ALCchar *) "ALC_ENUMERATION_EXT") ==
      AL_TRUE)
    {
      printDevices (ALC_DEVICE_SPECIFIER, "");
      printDevices (ALC_CAPTURE_DEVICE_SPECIFIER, "capture ");
    }
  else
    {
      printf ("no device enumeration available\n");
    }

  device = alcGetContextsDevice (alcGetCurrentContext ());
  checkForErrors ();

  printf ("default device: %s\n",
          getStringALC (device, ALC_DEFAULT_DEVICE_SPECIFIER));

  printf ("default capture device: %s\n",
          getStringALC (device, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER));

  alcGetIntegerv (device, ALC_MAJOR_VERSION, 1, &major);
  alcGetIntegerv (device, ALC_MAJOR_VERSION, 1, &minor);
  checkForErrors ();
  printf ("ALC version: %d.%d\n", (int) major, (int) minor);

  printExtensions ("ALC extensions", ' ',
                   getStringALC (device, ALC_EXTENSIONS));
}

static void
printALInfo (void)
{
  printf ("OpenAL vendor string: %s\n", getStringAL (AL_VENDOR));
  printf ("OpenAL renderer string: %s\n", getStringAL (AL_RENDERER));
  printf ("OpenAL version string: %s\n", getStringAL (AL_VERSION));
  printExtensions ("OpenAL extensions", ' ', getStringAL (AL_EXTENSIONS));
}

int
main (int argc, char *argv[])
{
  alutInit (&argc, argv);
  checkForErrors ();

  printALUTInfo ();
  printALCInfo ();
  printALInfo ();
  checkForErrors ();

  alutExit ();

  return EXIT_SUCCESS;
}
