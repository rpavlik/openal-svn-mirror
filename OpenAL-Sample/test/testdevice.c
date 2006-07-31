#include "testlib.h"

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
