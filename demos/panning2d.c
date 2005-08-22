#include <AL/al.h>
#include <AL/alut.h>

#include <GL/glut.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define WAVE "boom.wav"

static void display( void );
static void keyboard( unsigned char key, int x, int y );
static void reshape( int w, int h );
static void init( void );

static ALuint left_sid = 0;
static ALuint right_sid = 0;
static ALfloat left_pos[3] = { -4.0, 0.0, 4.0 };
static ALfloat right_pos[3] = { 4.0, 0.0, 4.0 };

static void display( void )
{
    static time_t then = 0;
    static ALboolean left = AL_FALSE;
    time_t now;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity(  );

    gluLookAt( 0.0, 4.0, 16.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 );

    /* Inverse, because we invert below. */
    if( left == AL_FALSE ) {
	glColor3f( 1.0, 0.0, 0.0 );
    } else {
	glColor3f( 1.0, 1.0, 1.0 );
    }

    /* Draw radiation cones. */
    glPushMatrix(  );
    glTranslatef( left_pos[0], left_pos[1], left_pos[2] );
    glRotatef( 180, 0.0, 1.0, 0.0 );
    glutWireCone( 1.0, 2.0, 20, 20 );
    glPopMatrix(  );

    if( left == AL_FALSE ) {
	glColor3f( 1.0, 1.0, 1.0 );
    } else {
	glColor3f( 1.0, 0.0, 0.0 );
    }

    glPushMatrix(  );
    glTranslatef( right_pos[0], right_pos[1], right_pos[2] );
    glRotatef( 180, 0.0, 1.0, 0.0 );
    glutWireCone( 1.0, 2.0, 20, 20 );
    glPopMatrix(  );

    /* Let's draw some text. */
    glMatrixMode( GL_PROJECTION );
    glPushMatrix(  );
    glLoadIdentity(  );
    glOrtho( 0, 640, 0, 480, -1.0, 1.0 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity(  );
    glTranslatef( 10.0, 10.0, 0.0 );
    glScalef( 0.2, 0.2, 0.2 );
    glColor3f( 1.0, 0.0, 0.0 );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'R' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'e' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'd' );
    glColor3f( 1.0, 1.0, 1.0 );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, ' ' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'i' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 's' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, ' ' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'A' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'c' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 't' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'i' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'v' );
    glutStrokeCharacter( GLUT_STROKE_ROMAN, 'e' );

    glMatrixMode( GL_PROJECTION );
    glPopMatrix(  );

    now = time( NULL );

    /* Switch between left and right boom every two seconds. */
    if( now - then > 1 ) {
	then = now;

	if( left == AL_TRUE ) {
	    alSourcePlay( left_sid );
	    left = AL_FALSE;
	} else {
	    alSourcePlay( right_sid );
	    left = AL_TRUE;
	}

    }

    glutSwapBuffers(  );
    glutPostRedisplay(  );
}

static void keyboard( unsigned char key, int x, int y )
{

    switch ( key ) {
    case 27:
	exit( EXIT_SUCCESS );
	break;
    default:
	break;
    }

}

static void reshape( int w, int h )
{
    glViewport( 0, 0, w, h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity(  );
    gluPerspective( 60.0f, ( float ) w / ( float ) h, 0.1f, 1024.0f );
}

static void init( void )
{
    ALuint sources[2];
    ALuint boom;
    void *wave = NULL;
    ALsizei size;
    ALsizei freq;
    ALsizei format;
    ALboolean loop;

    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glShadeModel( GL_SMOOTH );

    alGetError(  );
    alGenBuffers( 1, &boom );
    if( alGetError(  ) != AL_NO_ERROR ) {
	fprintf( stderr, "aldemo: couldn't generate samples\n" );
	exit( EXIT_FAILURE );
    }

    alutLoadWAVFile( ( ALbyte * ) WAVE, &format, &wave, &size, &freq, &loop );
    if( wave == NULL ) {
	fprintf( stderr, "aldemo: couldn't load %s\n", WAVE );
	exit( EXIT_FAILURE );
    }

    alBufferData( boom, format, wave, size, freq );

    alGetError(  );
    alGenSources( 2, sources );

    if( alGetError(  ) != AL_NO_ERROR ) {
	fprintf( stderr, "aldemo: couldn't generate sources\n" );
	exit( EXIT_FAILURE );
    }

    left_sid = sources[0];
    right_sid = sources[1];

    alSourcefv( left_sid, AL_POSITION, left_pos );
    alSourcei( left_sid, AL_BUFFER, boom );

    alSourcefv( right_sid, AL_POSITION, right_pos );
    alSourcei( right_sid, AL_BUFFER, boom );
}

int main( int argc, char *argv[] )
{
    /* Initialize GLUT. */
    glutInit( &argc, argv );

    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    glutInitWindowSize( 640, 480 );
    glutInitWindowPosition( 0, 0 );
    glutCreateWindow( argv[0] );

    glutReshapeFunc( reshape );
    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );

    /* Initialize ALUT. */
    alutInit( &argc, argv );

    init(  );

    glutMainLoop(  );

    return EXIT_SUCCESS;
}
