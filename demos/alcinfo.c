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

int main()
{
   ALCint data[MAX_DATA];
   ALCdevice *device = NULL;
   ALCcontext *context = NULL;
   unsigned int i;
   ALenum error;
   char *b, *p, *s;

   if (alcIsExtensionPresent(NULL, (unsigned char *)"ALC_EXT_enumeration") == AL_TRUE)
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

   b = s = strdup((char *)alcGetString(device, ALC_EXTENSIONS));
   printf("client alc extensions (ALC_):");
   if (b && (strlen(b) > 4))
   {
      char last = 0;
      s = b+strlen(b)-1;
      if (*s == ' ') *s = 0;

      i = 4;
      s = b+3;
      printf("\n   ");
      p = strchr(s, ' ');
      while (p || !last)
      {
         if (p) *p = 0;
         if (i > 4) printf(",");

         i += strlen(s);
         if (i > 76)
         {
            printf("\n   ");
            i = 5;
         }

         printf(" %s", s);
         if (p)
         {
            s = p+5;
            p = strchr(s, ' ');
         } else
            last = 1;
      }
      printf(".\n");
      free(b);
   } else
      printf(" none.\n");
  
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

   b = strdup((char *)alGetString(AL_EXTENSIONS));
   printf("OpenAL extensions (AL_):");
   if (b && (strlen(b) > 4))
   {
      char last = 0;
      s = b+strlen(b)-1;
      if (*s == ' ') *s = 0;

      i = 3;
      s = b+3;
      printf("\n   ");
      p = strchr(s, ' ');
      while (p || !last)
      {
         if (p) *p = 0;
         if (i > 3) printf(",");

         i += strlen(s);
         if (i > 76)
         {
            printf("\n   ");
            i = 4;
         }
 
         printf(" %s", s);
         if (p)
         {
            s = p+4;
            p = strchr(s, ' ');
         } else
            last = 1;
      }
      printf(".\n");
      free(b);
   } else
      printf(" none.\n");

   if ((error = alGetError()) != AL_NO_ERROR)
      printf("An OpenAL Error occurred: #%x\n", error);

   alcCloseDevice(device);

   return 0;
}
