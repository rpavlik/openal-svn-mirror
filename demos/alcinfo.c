/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alcinfo.x
 *
 * alcinfo display info about a ALC extension and OpenAL renderer
 *
 * This file is in the Public Domain and comes with no warranty.
 * Erik Hofman <erik@ehofman.com>
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __APPLE__
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
# include <AL/alext.h>
#endif

#ifndef AL_VERSION_1_1
# ifdef __APPLE__
#  include <OpenAL/altypes.h>
#  include <OpenAL/alctypes.h>
#else
#  include <AL/altypes.h>
#  include <AL/alctypes.h>
# endif
#endif

#define MAX_DATA	16

static void _printe(const char *t, const char *e, unsigned int sln)
{
   unsigned int i;
   char *b, *p, *s;

   b = strdup(e);
   printf(t);
   if (b && (strlen(b) > sln))
   {
      char newline = 1;
      s = b+strlen(b)-1;
      if (*s == ' ') *s = 0;

      s = b + sln;
      p = strchr(s, ' ');
      i = (p - s) + sln + 1;
      while (p != 0)
      {
         *p = 0;
         if (newline)
         {
            printf("\n    %s,", s);
            newline = 0;
         }
         else printf(" %s,", s);

         s = p + sln + 1;
         p = strchr(s, ' ');
         if (p) i += (p - s) + 2;
         else i += strlen(s) + 2;

         if (i > 75)
         {
            i = sln+2;
            newline = 1;
         }
      }
      if (newline) printf("\n    %s.\n", s);
      else printf(" %s.\n", s);
      free(b);
   } else
      printf(" none.\n");
}

int main()
{
   ALCint data[MAX_DATA];
   ALCdevice *device = NULL;
   ALCcontext *context = NULL;
   ALenum error;
   char *s;

   if (alcIsExtensionPresent(NULL, (unsigned char *)"ALC_enumeration_EXT") == AL_TRUE)
   {
      ALCchar *ptr;
      s = (char *)alcGetString(NULL, ALC_DEVICE_SPECIFIER);
      ptr = (ALCchar *)s;
      printf("available devices: ");
      if (!s)
         printf("none\n");
      else
         do {
            printf("%s ", ptr);
         } while (*(ptr += strlen(ptr)+1) != 0);
   }

   device = alcOpenDevice(NULL);
   if (device == NULL)
   {
      printf("\nNo default audio device available.\n");
      return -1;
   }

   context = alcCreateContext(device, NULL);
   if (context == NULL)
   {
      printf("\nUnable to create a valid context.\n");
      return -2;
   }
   alcMakeContextCurrent(context);

   if ((error = alcGetError(device)) != ALC_NO_ERROR)
      printf("\nAn ALC Error occurred: #%x\n", error);

   s = (char *)alcGetString(device, ALC_DEFAULT_DEVICE_SPECIFIER);
   printf("\nrenderer: %s\n", s);

   printf("client alc version: ");
   alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, data);
   printf("%i.", *data);
   alcGetIntegerv(device, ALC_MINOR_VERSION, 1, data);
   printf("%i\n", *data);

   if ((error = alcGetError(device)) != ALC_NO_ERROR)
      printf("\nAn ALC Error occurred: #%x\n", error);

   s = (char *)alcGetString(device, ALC_EXTENSIONS);
   _printe("client alc extensions (ALC_):", s, strlen("ALC_"));
  
   if ((error = alcGetError(device)) != ALC_NO_ERROR)
      printf("\nAn ALC Error occurred: #%x\n", error);

   s = (char *)alGetString(AL_VENDOR);
   printf("OpenAL verndor string: %s\n", s);
   if ((error = alGetError()) != AL_NO_ERROR)
      printf("An OpenAL Error #%x occured\n", error);

   s = (char *)alGetString(AL_RENDERER);
   printf("OpenAL renderer string %s\n", s);
   if ((error = alGetError()) != AL_NO_ERROR)
      printf("An OpenAL Error occurred: #%x\n", error);

   s = (char *)alGetString(AL_VERSION);
   printf("OpenAL version string: %s\n", s);
   if ((error = alGetError()) != AL_NO_ERROR)
      printf("An OpenAL Error occurred: #%x\n", error);

   s = (char *)alGetString(AL_EXTENSIONS);
   _printe("OpenAL extensions (AL_):", s, strlen("AL_"));

   if ((error = alGetError()) != AL_NO_ERROR)
      printf("An OpenAL Error occurred: #%x\n", error);

   alcCloseDevice(device);

   return 0;
}
