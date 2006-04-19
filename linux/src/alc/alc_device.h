/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alc_device.h
 *
 * Prototypes, defines etc for device aquisition and management.
 */
#ifndef AL_ALC_ALC_DEVICE_H_
#define AL_ALC_ALC_DEVICE_H_

#include "al_siteconfig.h"
#include <AL/al.h>
#include "al_types.h"

void alcDevicePause_( AL_device *dev );

void alcDeviceResume_( AL_device *dev );

/*
 * Sets the attributes for the device from the settings in the device. The
 * library is free to change the parameters associated with the device, but
 * until alcDeviceSet_ is called, none of the changes are important.
 *
 * Sets ALC_INVALID_DEVICE if the setting operation failed.  After a call to
 * this function, the caller should check the members in dev is see what the
 * actual values set where.
 */
void alcDeviceSet_( AL_device *dev );

void alcDeviceWrite_( AL_device *dev, ALvoid *dataptr, ALuint bytes_to_write );

ALsizei alcDeviceRead_( AL_device *dev, ALvoid *dataptr, ALuint bytes_to_read );

ALfloat alcDeviceGetAudioChannel_( AL_device *dev, ALuint channel);

void alcDeviceSetAudioChannel_( AL_device *dev, ALuint channel, ALfloat volume);

#endif /* not AL_ALC_ALC_DEVICE_H_ */
