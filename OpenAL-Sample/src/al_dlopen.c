#include "al_siteconfig.h"
#include "al_dlopen.h"

#ifdef USE_DLOPEN

AL_DLDataPtr
alDLDataSym_ (AL_DLHandle handle, const char *name)
{
	return (AL_DLDataPtr) dlsym (handle, name);
}

AL_DLFunPtr
alDLFunSym_ (AL_DLHandle handle, const char *name)
{
	return (AL_DLFunPtr) dlsym (handle, name);
}

#else /* USE_DLOPEN */

const char alDLErrorMessage[] =
	"Dynamic linking on this platform is not supported.";

#endif /* USE_DLOPEN */
