#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	ALCcontext *default_context = 0, *custom_context = 0;
	ALCdevice *dev;
	int attrlist[] = { ALC_FREQUENCY, 44100, ALC_SYNC, AL_TRUE, 0 };

	dev = alcOpenDevice( (const ALubyte *) "'((sampling-rate 44100))" );
	if( dev == NULL )
	{
		return 1;
	}

	/* Initialize ALUT. */
	default_context = alcCreateContext(dev, NULL);
	if(default_context == NULL)
	{
		alcCloseDevice( dev );

		return 1;
	}

	free(malloc(4));

	alcMakeContextCurrent(default_context);

	{
		ALint NumFlags = 0;
		ALint *Flags = 0;
		int i;

		printf("default context\n");

		alcGetIntegerv(dev, ALC_ATTRIBUTES_SIZE,
			       sizeof NumFlags, &NumFlags );

		printf("NumFlags %d\n", NumFlags);

		if(NumFlags)
		{
			Flags = malloc(sizeof NumFlags * sizeof *Flags);
			assert(Flags);

			alcGetIntegerv(dev, ALC_ALL_ATTRIBUTES,
				       sizeof NumFlags * sizeof *Flags,
				       Flags );
		}

		for(i = 0; i < NumFlags-1; i += 2)
		{
			printf("key 0x%x : value %d\n",
			       Flags[i], Flags[i+1]);
		}

		/* must be 0 terminated */
		assert(Flags[NumFlags-1] == 0);
	}

	custom_context  = alcCreateContext(dev, attrlist);	
	if(custom_context == NULL)
	{
		alcCloseDevice( dev );

		return 1;
	}
	alcMakeContextCurrent(custom_context);
	
	{
		ALint NumFlags = 0;
		ALint *Flags = 0;
		int i;

		printf("custom context\n");

		alcGetIntegerv(dev, ALC_ATTRIBUTES_SIZE,
			       sizeof NumFlags, &NumFlags );

		printf("NumFlags %d\n", NumFlags);

		if(NumFlags)
		{
			Flags = malloc(sizeof NumFlags * sizeof *Flags);
			assert(Flags);

			alcGetIntegerv(dev, ALC_ALL_ATTRIBUTES,
				       sizeof NumFlags * sizeof *Flags,
				       Flags );
		}

		for(i = 0; i < NumFlags-1; i += 2)
		{
			printf("key 0x%x : value %d\n",
			       Flags[i], Flags[i+1]);
		}

		/* must be 0 terminated */
		assert(Flags[NumFlags-1] == 0);
	}
	
	
 

	fixup_function_pointers();

	alcDestroyContext(default_context);
	alcDestroyContext(custom_context);
	
	return 0;
}
