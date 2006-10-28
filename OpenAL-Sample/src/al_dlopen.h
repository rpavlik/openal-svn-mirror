#ifndef AL_AL_DLOPEN_H_
#define AL_AL_DLOPEN_H_

typedef void *AL_DLDataPtr;
typedef void (*AL_DLFunPtr) (void);
typedef void *AL_DLHandle;

#ifdef USE_DLOPEN

#include <dlfcn.h>

static __inline int alDLInit_ (void) {return 0;}

static __inline int alDLExit_ (void) {return 0;}

static __inline AL_DLHandle
alDLOpen_ (const char *file)
{
	return dlopen (file, RTLD_LAZY | RTLD_GLOBAL);
}

static __inline int
alDLClose_ (AL_DLHandle handle)
{
	return dlclose (handle);
}

/* not inlined to have warning only at one place */
extern AL_DLDataPtr
alDLDataSym_ (AL_DLHandle handle, const char *name);

/* not inlined to have warning only at one place */
extern AL_DLFunPtr
alDLFunSym_ (AL_DLHandle handle, const char *name);

static __inline const char *
alDLError_ (void)
{
	return dlerror ();
}

#else /* USE_DLOPEN */

extern const char alDLErrorMessage[];


static __inline int alDLInit_ (void) {return 1;}

static __inline int alDLExit_ (void) {return 1;}

static __inline AL_DLHandle
alDLOpen_ (UNUSED (const char *file))
{
	return (AL_DLHandle) 0;
}

static __inline int
alDLClose_ (UNUSED (AL_DLHandle handle))
{
	return 1;
}

static __inline AL_DLDataPtr
alDLDataSym_ (UNUSED (AL_DLHandle handle), UNUSED (const char *name))
{
	return (AL_DLDataPtr) 0;
}

static __inline AL_DLFunPtr
alDLFunSym_ (UNUSED (AL_DLHandle handle), UNUSED (const char *name))
{
	return (AL_DLFunPtr) 0;
}

static __inline const char *
alDLError_ (void)
{
	return alDLErrorMessage;
}
#endif /* USE_DLOPEN */

#endif /* not AL_AL_DLOPEN_H_ */
