#ifndef AL_AL_DLOPEN_H_
#define AL_AL_DLOPEN_H_

typedef void *AL_DLDataPtr;
typedef void (*AL_DLFunPtr) (void);

typedef void *AL_DLHandle;

extern int alDLInit_ (void);

extern int alDLExit_ (void);

extern AL_DLHandle alDLOpen_ (const char *file);

extern int alDLClose_ (AL_DLHandle handle);

extern AL_DLDataPtr alDLDataSym_ (AL_DLHandle handle, const char *name);

extern AL_DLFunPtr alDLFunSym_ (AL_DLHandle handle, const char *name);

extern const char *alDLError_ (void);

extern AL_DLFunPtr alDLDataPtrAsFunPtr_ (AL_DLDataPtr);

extern AL_DLDataPtr alDLFunPtrAsDataPtr_ (AL_DLFunPtr);

#endif /* not AL_AL_DLOPEN_H_ */
