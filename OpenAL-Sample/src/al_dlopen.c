#include "al_siteconfig.h"
#include "al_dlopen.h"

#ifdef USE_DLOPEN

#include <dlfcn.h>

int
alDLInit_ (void)
{
  return 0;
}

int
alDLExit_ (void)
{
  return 0;
}

AL_DLHandle
alDLOpen_ (const char *file)
{
  return dlopen (file, RTLD_LAZY | RTLD_GLOBAL);
}

int
alDLClose_ (AL_DLHandle handle)
{
  return dlclose (handle);
}

AL_DLDataPtr
alDLDataSym_ (AL_DLHandle handle, const char *name)
{
  return (AL_DLDataPtr) dlsym (handle, name);
}

AL_DLFunPtr
alDLFunSym_ (AL_DLHandle handle, const char *name)
{
  /*
   * The dlsym type sucks, there should be different API entries for
   * data/function pointers
   */
  return alDLDataPtrAsFunPtr_ (dlsym (handle, name));
}

const char *
alDLError_ (void)
{
  return dlerror ();
}

#else

int
alDLInit_ (void)
{
  return 1;
}

int
alDLExit_ (void)
{
  return 1;
}

AL_DLHandle
alDLOpen_ (UNUSED (const char *file))
{
  return (AL_DLHandle) 0;
}

int
alDLClose_ (UNUSED (AL_DLHandle handle))
{
  return 1;
}

AL_DLDataPtr
alDLDataSym_ (UNUSED (AL_DLHandle handle), UNUSED (const char *name))
{
  return (AL_DLDataPtr) 0;
}

AL_DLFunPtr
alDLFunSym_ (UNUSED (AL_DLHandle handle), UNUSED (const char *name))
{
  return (AL_DLFunPtr) 0;
}

const char *
alDLError_ (void)
{
  return "Dynamic linking on this platform is not supported.";
}

#endif /* USE_DLOPEN */

AL_DLFunPtr
alDLDataPtrAsFunPtr_ (AL_DLDataPtr p)
{
  return (void *) p; /* NOTE: The cast is not valid ISO C! */
}

AL_DLDataPtr
alDLFunPtrAsDataPtr_ (AL_DLFunPtr p)
{
  return (void *) p; /* NOTE: The cast is not valid ISO C! */
}
