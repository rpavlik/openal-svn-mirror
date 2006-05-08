#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	const ALCchar *initstr =
	    ( const ALCchar * ) "'( ( devices '( native null ) ) )";

	device = alcOpenDevice( initstr );

	sleep( 1 );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
