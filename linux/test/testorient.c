#include "../src/al_main.h"
#include "../src/alc/alc_speaker.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI_2
#define M_PI_2	1.57079632679489661923	/* pi/2 */
#endif

#ifndef M_PI
#define M_PI	(2 * M_PI_2)
#endif

static ALCcontext *context = NULL;	/* our context */
static void setposition( ALfloat x, ALfloat y, ALfloat z );
static void setorientation( ALfloat ax, ALfloat ay, ALfloat az,
			    ALfloat ux, ALfloat uy, ALfloat uz );

int main( void )
{
	ALCdevice *device;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	context = alcCreateContext( device, NULL );
	if( context == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	setposition( -10.0, 10, 10 );
	setposition( 10.0, 10, 10 );
	setposition( 0.0, 10, 10 );

	setposition( 0.0, 0, 0 );
	setorientation( 0.0, 0, -1, 0, 1, 0 );
	setorientation( 0.0, 0, 1, 0, 1, 0 );
	setorientation( 0.0, 0, 1, 0, 1, 0 );

	setposition( 2, 2, 2 );
	setorientation( 0.0, 0, 1, 0, 1, 0 );

	setposition( 0, 0, 0 );
	setorientation( 0.0, 0, 1, 0, 1, 0 );

	setposition( 0, -10, 0 );
	setorientation( 1, 1, -1, 0, 1, 0 );

	setposition( 0, 0, 0 );
	setorientation( 0.0, 0, 1, 0, 1, 0 );

	alcDestroyContext( context );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}

static void setposition( ALfloat x, ALfloat y, ALfloat z )
{
	ALfloat pos[3];

	pos[0] = x;
	pos[1] = y;
	pos[2] = z;

	fprintf( stderr, "POSITION: [%f, %f, %f]\n", x, y, z );

	alListenerfv( AL_POSITION, pos );

	fprintf( stderr, "--------------------------------\n" );
}

static void
setorientation( ALfloat ax, ALfloat ay, ALfloat az,
		ALfloat ux, ALfloat uy, ALfloat uz )
{
	ALfloat fv[6];

	fv[0] = ax;
	fv[1] = ay;
	fv[2] = az;
	fv[3] = ux;
	fv[4] = uy;
	fv[5] = uz;

	fprintf( stderr, "ORIENTATION: [%f, %f, %f] [%f, %f, %f]\n",
		 ax, ay, az, ux, uy, uz );
	alListenerfv( AL_ORIENTATION, fv );

	fprintf( stderr, "--------------------------------\n" );
}
