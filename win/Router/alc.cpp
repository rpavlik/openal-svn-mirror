/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */




#include <stdlib.h>
#include <memory.h>
#define _OPENAL32LIB
#include <al/alc.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>

#include <stddef.h>
#include <windows.h>
#include <crtdbg.h>
#include <atlconv.h>

#include "OpenAL32.h"


//*****************************************************************************
//*****************************************************************************
//
// Defines
//
//*****************************************************************************
//*****************************************************************************

typedef struct ALCextension_struct
{

    char* ename;

} ALCextension;

typedef struct
{
    char*    ename;
    ALenum   value;

} ALCRouterEnum;

typedef struct ALCfunction_struct
{

    char* fname;
    ALvoid*  address;

} ALCfunction;



//*****************************************************************************
//*****************************************************************************
//
// Global Vars
//
//*****************************************************************************
//*****************************************************************************

ALlist* alContextList = 0;
ALCcontext* alCurrentContext = 0;



//*****************************************************************************
//*****************************************************************************
//
// Local Vars
//
//*****************************************************************************
//*****************************************************************************

//
// The values of the enums supported by OpenAL.
//
static ALCRouterEnum alcEnums[] =
{
    // Types
    {"ALC_INVALID",                     ALC_INVALID},

    // ALC Properties
    {"ALC_MAJOR_VERSION",               ALC_MAJOR_VERSION},
    {"ALC_MINOR_VERSION",               ALC_MINOR_VERSION},
    {"ALC_ATTRIBUTES_SIZE",             ALC_ATTRIBUTES_SIZE},
    {"ALC_ALL_ATTRIBUTES",              ALC_ALL_ATTRIBUTES},
    {"ALC_DEFAULT_DEVICE_SPECIFIER",    ALC_DEFAULT_DEVICE_SPECIFIER},
    {"ALC_DEVICE_SPECIFIER",            ALC_DEVICE_SPECIFIER},
    {"ALC_EXTENSIONS",                  ALC_EXTENSIONS},
    {"ALC_FREQUENCY",                   ALC_FREQUENCY},
    {"ALC_REFRESH",                     ALC_REFRESH},
    {"ALC_SYNC",                        ALC_SYNC},

    // ALC Error Message
    {"ALC_NO_ERROR",                    ALC_NO_ERROR},
    {"ALC_INVALID_DEVICE",              ALC_INVALID_DEVICE},
    {"ALC_INVALID_CONTEXT",             ALC_INVALID_CONTEXT},
    {"ALC_INVALID_ENUM",                ALC_INVALID_ENUM},
    {"ALC_INVALID_VALUE",               ALC_INVALID_VALUE},
    {"ALC_OUT_OF_MEMORY",               ALC_OUT_OF_MEMORY},

    // Default
    {0,                                 (ALenum)0}
};

//
// Our function pointers.
//
static ALCfunction alcFunctions[] =
{
    {"alcCloseDevice",                  (ALvoid*)alcCloseDevice},
    {"alcCreateContext",                (ALvoid*)alcCreateContext},
    {"alcDestroyContext",               (ALvoid*)alcDestroyContext},
    {"alcGetContextsDevice",            (ALvoid*)alcGetContextsDevice},
    {"alcGetCurrentContext",            (ALvoid*)alcGetCurrentContext},
    {"alcGetEnumValue",                 (ALvoid*)alcGetEnumValue},
    {"alcGetError",                     (ALvoid*)alcGetError},
    {"alcGetIntegerv",                  (ALvoid*)alcGetIntegerv},
    {"alcGetProcAddress",               (ALvoid*)alcGetProcAddress},
    {"alcGetString",                    (ALvoid*)alcGetString},
    {"alcIsExtensionPresent",           (ALvoid*)alcIsExtensionPresent},
    {"alcMakeContextCurrent",           (ALvoid*)alcMakeContextCurrent},
    {"alcOpenDevice",                   (ALvoid*)alcOpenDevice},
    {"alcProcessContext",               (ALvoid*)alcProcessContext},
    {"alcSuspendContext",               (ALvoid*)alcSuspendContext},

    {0,                                 (ALvoid*)0}
};

//
// Our extensions.
//
static ALCextension alcExtensions[] =
{
    "ALC_ENUMERATION_EXT",
	0
};


// Error strings
static ALenum  LastError = AL_NO_ERROR;
static ALubyte alcNoError[] = "No Error";
static ALubyte alcErrInvalidDevice[] = "Invalid Device";
static ALubyte alcErrInvalidContext[] = "Invalid Context";
static ALubyte alcErrInvalidEnum[] = "Invalid Enum";
static ALubyte alcErrInvalidValue[] = "Invalid Value";

// Context strings
static ALubyte alcDefaultDeviceSpecifier[MAX_PATH] = "\0";
static ALubyte alcDeviceSpecifierList[MAX_PATH] = "\0";
static ALubyte alcExtensionList[] = "";

static ALint alcMajorVersion = 1;
static ALint alcMinorVersion = 0;



//*****************************************************************************
//*****************************************************************************
//
// Local Functions
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
// NewSpecifierCheck
//*****************************************************************************
//
ALboolean NewSpecifierCheck(ALubyte* specifier)
{
	ALubyte* list = alcDeviceSpecifierList;

	while (*list != 0) {
		if (strcmp((char *)list, (char *)specifier) == 0) {
			return AL_FALSE;
		}
		list += strlen((char *)list) + 1;
	}
	
	return AL_TRUE;
}

//*****************************************************************************
// BuildDeviceSpecifierList
//*****************************************************************************
//
ALvoid BuildDeviceSpecifierList()
{
    WIN32_FIND_DATA findData;
    HANDLE searchHandle = INVALID_HANDLE_VALUE;
    TCHAR searchName[MAX_PATH + 1];
	TCHAR module[MAX_PATH + 1];
    TCHAR fileDrive[MAX_PATH + 1];
    TCHAR fileDir[MAX_PATH + 1];
    TCHAR fileName[MAX_PATH + 1];
    TCHAR fileExt[MAX_PATH + 1];
	TCHAR systemAL[MAX_PATH + 1];
    BOOL found = FALSE;
    ALubyte* specifier = 0;
    ALuint specifierSize = 0;
    ALubyte* list = alcDeviceSpecifierList;
    ALuint listSize = 0;
	ALCdevice *device;
	void *context;


	if (list[0] == 0) { // don't re-build list if it's already been done once...

		//
		// Directory[0] is the current directory
		// Directory[1] is the current OpenAL32.dll directory
		// Directory[2] is the system directory.
		//
		TCHAR dir[3][MAX_PATH + 1];
		DWORD dirSize = 0;
		int i;
		HINSTANCE dll = 0;
		ALCAPI_GET_STRING alcGetStringFxn = 0;
		ALCAPI_IS_EXTENSION_PRESENT alcIsExtensionPresentFxn = 0;
		ALCAPI_OPEN_DEVICE alcOpenDeviceFxn = 0;
		ALCAPI_CREATE_CONTEXT alcCreateContextFxn = 0;
		ALCAPI_MAKE_CONTEXT_CURRENT alcMakeContextCurrentFxn = 0;
		ALCAPI_DESTROY_CONTEXT alcDestroyContextFxn = 0;
		ALCAPI_CLOSE_DEVICE alcCloseDeviceFxn = 0;

		//
		// Construct our search paths.  We will search the current directory, the app directory, and then
		// the system directory.
		//
		dirSize = GetCurrentDirectory(MAX_PATH, dir[0]);
		_tcscat(dir[0], _T("\\"));

		GetModuleFileName(0, module, MAX_PATH);
		_splitpath(module, fileDrive, fileDir, fileName, fileExt);
		_tcscpy(dir[1], fileDrive);
		_tcscat(dir[1], fileDir);
		_tcscat(fileName, fileExt);

		dirSize = GetSystemDirectory(dir[2], MAX_PATH);
		_tcscat(dir[2], _T("\\"));
		_tcscpy(systemAL, dir[2]);
		_tcscat(systemAL, _T("OpenAL32.dll"));

		//
		// Begin searching for additional OpenAL implementations.  Start with the current directory, but check if
		// the current directory is the same as the application directory so we don't search it twice.
		//
		for(i = _tcsicmp(dir[0], dir[1]) != 0 ? 0 : 1; i < 3; i++)
		{
			_tcscpy(searchName, dir[i]);
			_tcscat(searchName, _T("nvopenal.dll"));
			searchHandle = FindFirstFile(searchName, &findData);
			if(searchHandle != INVALID_HANDLE_VALUE)
			{
				while(TRUE)
				{
					//
					// if this is an OpenAL32.dll, skip it -- it's probably a router and shouldn't be enumerated regardless
					//
					_tcscpy(searchName, dir[i]);
					_tcscat(searchName, findData.cFileName);
					TCHAR cmpName[MAX_PATH];
					_tcscpy(cmpName, searchName);
					_tcsupr(cmpName);
					if (strstr(cmpName, "OPENAL32.DLL") == 0)
					{
						//
						// Its not, so load it and see if the name matches.
						//
						dll = LoadLibrary(searchName);
						if(dll)
						{	
							alcOpenDeviceFxn = (ALCAPI_OPEN_DEVICE)GetProcAddress(dll, "alcOpenDevice");
							alcCreateContextFxn = (ALCAPI_CREATE_CONTEXT)GetProcAddress(dll, "alcCreateContext");
							alcMakeContextCurrentFxn = (ALCAPI_MAKE_CONTEXT_CURRENT)GetProcAddress(dll, "alcMakeContextCurrent");
							alcGetStringFxn = (ALCAPI_GET_STRING)GetProcAddress(dll, "alcGetString");
							alcDestroyContextFxn = (ALCAPI_DESTROY_CONTEXT)GetProcAddress(dll, "alcDestroyContext");
							alcCloseDeviceFxn = (ALCAPI_CLOSE_DEVICE)GetProcAddress(dll, "alcCloseDevice");
							alcIsExtensionPresentFxn = (ALCAPI_IS_EXTENSION_PRESENT)GetProcAddress(dll, "alcIsExtensionPresent");

							if ((alcOpenDeviceFxn != 0) && 
								(alcCreateContextFxn != 0) &&
								(alcMakeContextCurrentFxn != 0) &&
								(alcGetStringFxn != 0) &&
								(alcDestroyContextFxn != 0) &&
								(alcCloseDeviceFxn != 0) &&
								(alcIsExtensionPresentFxn != 0)) {

								if (alcIsExtensionPresentFxn(NULL, (ALubyte *)"ALC_ENUMERATION_EXT")) {
									// this DLL can enumerate devices -- so add complete list of devices
									specifier = alcGetStringFxn(0, ALC_DEVICE_SPECIFIER);
									do {
										specifierSize = strlen((char*)specifier);

										if (NewSpecifierCheck(specifier)) { // make sure we're not creating a duplicate device
											if(specifierSize + listSize + 1 < MAX_PATH - 1)
											{
												strcpy((char*)list, (char*)specifier);
												list += specifierSize + 1;
											}
										}
										specifier += strlen((char *)specifier) + 1;
									} while (strlen((char *)specifier) > 0);
								} else {
									// no enumeration ability, -- so just add default device to the list								
									device = alcOpenDeviceFxn(NULL);
									if (device != NULL) {
										context = alcCreateContextFxn(device, NULL);
										alcMakeContextCurrentFxn((ALCcontext *)context);
										if (context != NULL) {
											specifier = alcGetStringFxn(device, ALC_DEVICE_SPECIFIER);
											specifierSize = strlen((char*)specifier);

											if (NewSpecifierCheck(specifier)) { // make sure we're not creating a duplicate device
												if(specifierSize + listSize + 1 < MAX_PATH - 1)
												{
													strcpy((char*)list, (char*)specifier);
													list += specifierSize + 1;
												}
											}
											alcMakeContextCurrentFxn((ALCcontext *)NULL);
											alcDestroyContextFxn((ALCcontext *)context);
											alcCloseDeviceFxn(device);
										}
									}
								}
							} else {
								// handle legacy Creative DLLs which don't have alcIsExtensionPresentFxn exported (original Audigy, Live)
								if ((alcOpenDeviceFxn != 0) && 
									(alcCreateContextFxn != 0) &&
									(alcMakeContextCurrentFxn != 0) &&
									(alcGetStringFxn != 0) &&
									(alcDestroyContextFxn != 0) &&
									(alcCloseDeviceFxn != 0)) {

									device = alcOpenDeviceFxn(NULL);
									if (device != NULL) {
										context = alcCreateContextFxn(device, NULL);
										alcMakeContextCurrentFxn((ALCcontext *)context);
										if (context != NULL) {
											specifier = alcGetStringFxn(device, ALC_DEVICE_SPECIFIER);
											specifierSize = strlen((char*)specifier);

											if (NewSpecifierCheck(specifier)) { // make sure we're not creating a duplicate device
												if(specifierSize + listSize + 1 < MAX_PATH - 1)
												{
													strcpy((char*)list, (char*)specifier);
													list += specifierSize + 1;
												}
											}
											alcMakeContextCurrentFxn((ALCcontext *)NULL);
											alcDestroyContextFxn((ALCcontext *)context);
											alcCloseDeviceFxn(device);
										}
									}

								}
							}

							FreeLibrary(dll);
							dll = 0;
						}
					}

					if(!FindNextFile(searchHandle, &findData))
					{
						if(GetLastError() == ERROR_NO_MORE_FILES)
						{
							break;
						}
					}
				}

				FindClose(searchHandle);
				searchHandle = INVALID_HANDLE_VALUE;
			}
		}

		//
		// And again for the other DLL pattern match.
		//
		for(i = _tcsicmp(dir[0], dir[1]) != 0 ? 0 : 1; i < 3; i++)
		{
			_tcscpy(searchName, dir[i]);
			_tcscat(searchName, _T("*oal.dll"));
			searchHandle = FindFirstFile(searchName, &findData);
			if(searchHandle != INVALID_HANDLE_VALUE)
			{
				while(TRUE)
				{
					//
					// if this is an OpenAL32.dll, skip it -- it's probably a router and shouldn't be enumerated regardless
					//
					_tcscpy(searchName, dir[i]);
					_tcscat(searchName, findData.cFileName);
					TCHAR cmpName[MAX_PATH];
					_tcscpy(cmpName, searchName);
					_tcsupr(cmpName);
					if (strstr(cmpName, "OPENAL32.DLL") == 0)
					{
						//
						// Its not, so load it and see if the name matches.
						//
						dll = LoadLibrary(searchName);
						if(dll)
						{
							alcOpenDeviceFxn = (ALCAPI_OPEN_DEVICE)GetProcAddress(dll, "alcOpenDevice");
							alcCreateContextFxn = (ALCAPI_CREATE_CONTEXT)GetProcAddress(dll, "alcCreateContext");
							alcMakeContextCurrentFxn = (ALCAPI_MAKE_CONTEXT_CURRENT)GetProcAddress(dll, "alcMakeContextCurrent");
							alcGetStringFxn = (ALCAPI_GET_STRING)GetProcAddress(dll, "alcGetString");
							alcDestroyContextFxn = (ALCAPI_DESTROY_CONTEXT)GetProcAddress(dll, "alcDestroyContext");
							alcCloseDeviceFxn = (ALCAPI_CLOSE_DEVICE)GetProcAddress(dll, "alcCloseDevice");
							alcIsExtensionPresentFxn = (ALCAPI_IS_EXTENSION_PRESENT)GetProcAddress(dll, "alcIsExtensionPresent");

							if ((alcOpenDeviceFxn != 0) && 
								(alcCreateContextFxn != 0) &&
								(alcMakeContextCurrentFxn != 0) &&
								(alcGetStringFxn != 0) &&
								(alcDestroyContextFxn != 0) &&
								(alcCloseDeviceFxn != 0) &&
								(alcIsExtensionPresentFxn != 0)) {

								if (alcIsExtensionPresentFxn(NULL, (ALubyte *)"ALC_ENUMERATION_EXT")) {
									// this DLL can enumerate devices -- so add complete list of devices
									specifier = alcGetStringFxn(0, ALC_DEVICE_SPECIFIER);
									do {
										specifierSize = strlen((char*)specifier);

										if (NewSpecifierCheck(specifier)) { // make sure we're not creating a duplicate device
											if(specifierSize + listSize + 1 < MAX_PATH - 1)
											{
												strcpy((char*)list, (char*)specifier);
												list += specifierSize + 1;
											}
										}
										specifier += strlen((char *)specifier) + 1;
									} while (strlen((char *)specifier) > 0);
								} else {
									// no enumeration ability, -- so just add default device to the list								
									device = alcOpenDeviceFxn(NULL);
									if (device != NULL) {
										context = alcCreateContextFxn(device, NULL);
										alcMakeContextCurrentFxn((ALCcontext *)context);
										if (context != NULL) {
											specifier = alcGetStringFxn(device, ALC_DEVICE_SPECIFIER);
											specifierSize = strlen((char*)specifier);

											if (NewSpecifierCheck(specifier)) { // make sure we're not creating a duplicate device
												if(specifierSize + listSize + 1 < MAX_PATH - 1)
												{
													strcpy((char*)list, (char*)specifier);
													list += specifierSize + 1;
												}
											}
											alcMakeContextCurrentFxn((ALCcontext *)NULL);
											alcDestroyContextFxn((ALCcontext *)context);
											alcCloseDeviceFxn(device);
										}
									}
								}
							} else {
								// handle legacy Creative DLLs which don't have alcIsExtensionPresentFxn exported (original Audigy, Live)
								if ((alcOpenDeviceFxn != 0) && 
									(alcCreateContextFxn != 0) &&
									(alcMakeContextCurrentFxn != 0) &&
									(alcGetStringFxn != 0) &&
									(alcDestroyContextFxn != 0) &&
									(alcCloseDeviceFxn != 0)) {

									device = alcOpenDeviceFxn(NULL);
									if (device != NULL) {
										context = alcCreateContextFxn(device, NULL);
										alcMakeContextCurrentFxn((ALCcontext *)context);
										if (context != NULL) {
											specifier = alcGetStringFxn(device, ALC_DEVICE_SPECIFIER);
											specifierSize = strlen((char*)specifier);

											if (NewSpecifierCheck(specifier)) { // make sure we're not creating a duplicate device
												if(specifierSize + listSize + 1 < MAX_PATH - 1)
												{
													strcpy((char*)list, (char*)specifier);
													list += specifierSize + 1;
												}
											}
											alcMakeContextCurrentFxn((ALCcontext *)NULL);
											alcDestroyContextFxn((ALCcontext *)context);
											alcCloseDeviceFxn(device);
										}
									}

								}
							}

							FreeLibrary(dll);
							dll = 0;
						}
					}

					if(!FindNextFile(searchHandle, &findData))
					{
						if(GetLastError() == ERROR_NO_MORE_FILES)
						{
							break;
						}
					}
				}

				FindClose(searchHandle);
				searchHandle = INVALID_HANDLE_VALUE;
			}
		}

		//
		// Put a terminating NULL on.
		//
		strcpy((char*)list, "\0");
	}

    return;
}


//*****************************************************************************
// FillOutAlcFunctions
//*****************************************************************************
//
ALboolean FillOutAlcFunctions(ALCdevice* device)
{
    ALboolean alcFxns = FALSE;
    ALCAPI_FXN_TABLE* alcApi = &device->AlcApi;

    //
    // Get the OpenAL 1.0 Entry points.
    //
    alcApi->alcGetProcAddress     = (ALCAPI_GET_PROC_ADDRESS)GetProcAddress(device->Dll, "alcGetProcAddress");

    alcApi->alcGetString          = (ALCAPI_GET_STRING)GetProcAddress(device->Dll, "alcGetString");
    alcApi->alcGetIntegerv        = (ALCAPI_GET_INTEGERV)GetProcAddress(device->Dll, "alcGetIntegerv");

    alcApi->alcOpenDevice         = (ALCAPI_OPEN_DEVICE)GetProcAddress(device->Dll, "alcOpenDevice");
    alcApi->alcCloseDevice        = (ALCAPI_CLOSE_DEVICE)GetProcAddress(device->Dll, "alcCloseDevice");

    alcApi->alcCreateContext      = (ALCAPI_CREATE_CONTEXT)GetProcAddress(device->Dll, "alcCreateContext");
    alcApi->alcMakeContextCurrent = (ALCAPI_MAKE_CONTEXT_CURRENT)GetProcAddress(device->Dll, "alcMakeContextCurrent");
    alcApi->alcProcessContext     = (ALCAPI_PROCESS_CONTEXT)GetProcAddress(device->Dll, "alcProcessContext");
    alcApi->alcGetCurrentContext  = (ALCAPI_GET_CURRENT_CONTEXT)GetProcAddress(device->Dll, "alcGetCurrentContext");
    alcApi->alcGetContextsDevice  = (ALCAPI_GET_CONTEXTS_DEVICE)GetProcAddress(device->Dll, "alcGetContextsDevice");
    alcApi->alcSuspendContext     = (ALCAPI_SUSPEND_CONTEXT)GetProcAddress(device->Dll, "alcSuspendContext");
    alcApi->alcDestroyContext     = (ALCAPI_DESTROY_CONTEXT)GetProcAddress(device->Dll, "alcDestroyContext");

    alcApi->alcGetError           = (ALCAPI_GET_ERROR)GetProcAddress(device->Dll, "alcGetError");

    alcApi->alcIsExtensionPresent = (ALCAPI_IS_EXTENSION_PRESENT)GetProcAddress(device->Dll, "alcIsExtensionPresent");
    alcApi->alcGetEnumValue       = (ALCAPI_GET_ENUM_VALUE)GetProcAddress(device->Dll, "alcGetEnumValue");

	// handle legacy issue with old Creative DLLs which may not have alcGetProcAddress, alcIsExtensionPresent, alcGetEnumValue
	if (alcApi->alcGetProcAddress == NULL) {
		alcApi->alcGetProcAddress = (ALCAPI_GET_PROC_ADDRESS)alcGetProcAddress;
	}
	if (alcApi->alcIsExtensionPresent == NULL) {
		alcApi->alcIsExtensionPresent = (ALCAPI_IS_EXTENSION_PRESENT)alcIsExtensionPresent;
	}
	if (alcApi->alcGetEnumValue == NULL) {
		alcApi->alcGetEnumValue = (ALCAPI_GET_ENUM_VALUE)alcGetEnumValue;
	}


    alcFxns = (alcApi->alcGetString          &&
               alcApi->alcGetIntegerv        &&
               alcApi->alcOpenDevice         &&
               alcApi->alcCloseDevice        &&

               alcApi->alcCreateContext      &&
               alcApi->alcMakeContextCurrent &&
               alcApi->alcProcessContext     &&
               alcApi->alcGetCurrentContext  &&
               alcApi->alcGetContextsDevice  &&
               alcApi->alcSuspendContext     &&
               alcApi->alcDestroyContext     &&

               alcApi->alcGetError           &&

               alcApi->alcIsExtensionPresent &&
               alcApi->alcGetProcAddress     &&
               alcApi->alcGetEnumValue);

    return alcFxns;
}


//*****************************************************************************
// FillOutAlFunctions
//*****************************************************************************
//
ALboolean FillOutAlFunctions(ALCcontext* context)
{
    ALboolean  alFxns = FALSE;
    ALAPI_FXN_TABLE*   alApi = &context->AlApi;

    //
    // Get the OpenAL 1.0 Entry points.
    //
    alApi->alGetProcAddress       = (ALAPI_GET_PROC_ADDRESS)GetProcAddress(context->Device->Dll, "alGetProcAddress");

    // AL:
    alApi->alEnable               = (ALAPI_ENABLE)GetProcAddress(context->Device->Dll, "alEnable");
    alApi->alDisable              = (ALAPI_DISABLE)GetProcAddress(context->Device->Dll, "alDisable");
    alApi->alIsEnabled            = (ALAPI_IS_ENABLED)GetProcAddress(context->Device->Dll, "alIsEnabled");

    alApi->alGetBoolean           = (ALAPI_GET_BOOLEAN)GetProcAddress(context->Device->Dll, "alGetBoolean");
    alApi->alGetInteger           = (ALAPI_GET_INTEGER)GetProcAddress(context->Device->Dll, "alGetInteger");
    alApi->alGetFloat             = (ALAPI_GET_FLOAT)GetProcAddress(context->Device->Dll, "alGetFloat");
    alApi->alGetDouble            = (ALAPI_GET_DOUBLE)GetProcAddress(context->Device->Dll, "alGetDouble");
    alApi->alGetBooleanv          = (ALAPI_GET_BOOLEANV)GetProcAddress(context->Device->Dll, "alGetBooleanv");
    alApi->alGetIntegerv          = (ALAPI_GET_INTEGERV)GetProcAddress(context->Device->Dll, "alGetIntegerv");
    alApi->alGetFloatv            = (ALAPI_GET_FLOATV)GetProcAddress(context->Device->Dll, "alGetFloatv");
    alApi->alGetDoublev           = (ALAPI_GET_DOUBLEV)GetProcAddress(context->Device->Dll, "alGetDoublev");
    alApi->alGetString            = (ALAPI_GET_STRING)GetProcAddress(context->Device->Dll, "alGetString");

    alApi->alGetError             = (ALAPI_GET_ERROR)GetProcAddress(context->Device->Dll, "alGetError");

    alApi->alIsExtensionPresent   = (ALAPI_IS_EXTENSION_PRESENT)GetProcAddress(context->Device->Dll, "alIsExtensionPresent");
    alApi->alGetEnumValue         = (ALAPI_GET_ENUM_VALUE)GetProcAddress(context->Device->Dll, "alGetEnumValue");

    alApi->alListeneri            = (ALAPI_LISTENERI)GetProcAddress(context->Device->Dll, "alListeneri");
    alApi->alListenerf            = (ALAPI_LISTENERF)GetProcAddress(context->Device->Dll, "alListenerf");
    alApi->alListener3f           = (ALAPI_LISTENER3F)GetProcAddress(context->Device->Dll, "alListener3f");
    alApi->alListenerfv           = (ALAPI_LISTENERFV)GetProcAddress(context->Device->Dll, "alListenerfv");
    alApi->alGetListeneri         = (ALAPI_GET_LISTENERI)GetProcAddress(context->Device->Dll, "alGetListeneri");
    alApi->alGetListenerf         = (ALAPI_GET_LISTENERF)GetProcAddress(context->Device->Dll, "alGetListenerf");
    alApi->alGetListener3f        = (ALAPI_GET_LISTENER3F)GetProcAddress(context->Device->Dll, "alGetListener3f");
    alApi->alGetListenerfv        = (ALAPI_GET_LISTENERFV)GetProcAddress(context->Device->Dll, "alGetListenerfv");

    alApi->alGenSources           = (ALAPI_GEN_SOURCES)GetProcAddress(context->Device->Dll, "alGenSources");
    alApi->alDeleteSources        = (ALAPI_DELETE_SOURCES)GetProcAddress(context->Device->Dll, "alDeleteSources");
    alApi->alIsSource             = (ALAPI_IS_SOURCE)GetProcAddress(context->Device->Dll, "alIsSource");
    alApi->alSourcei              = (ALAPI_SOURCEI)GetProcAddress(context->Device->Dll, "alSourcei");
    alApi->alSourcef              = (ALAPI_SOURCEF)GetProcAddress(context->Device->Dll, "alSourcef");
    alApi->alSource3f             = (ALAPI_SOURCE3F)GetProcAddress(context->Device->Dll, "alSource3f");
    alApi->alSourcefv             = (ALAPI_SOURCEFV)GetProcAddress(context->Device->Dll, "alSourcefv");
    alApi->alGetSourcei           = (ALAPI_GET_SOURCEI)GetProcAddress(context->Device->Dll, "alGetSourcei");
    alApi->alGetSourcef           = (ALAPI_GET_SOURCEF)GetProcAddress(context->Device->Dll, "alGetSourcef");
    alApi->alGetSource3f          = (ALAPI_GET_SOURCE3F)GetProcAddress(context->Device->Dll, "alGetSource3f");
    alApi->alGetSourcefv          = (ALAPI_GET_SOURCEFV)GetProcAddress(context->Device->Dll, "alGetSourcefv");
    alApi->alSourcePlayv          = (ALAPI_SOURCE_PLAYV)GetProcAddress(context->Device->Dll, "alSourcePlayv");
    alApi->alSourcePausev         = (ALAPI_SOURCE_PAUSEV)GetProcAddress(context->Device->Dll, "alSourcePausev");
    alApi->alSourceStopv          = (ALAPI_SOURCE_STOPV)GetProcAddress(context->Device->Dll, "alSourceStopv");
    alApi->alSourceRewindv        = (ALAPI_SOURCE_REWINDV)GetProcAddress(context->Device->Dll, "alSourceRewindv");
    alApi->alSourcePlay           = (ALAPI_SOURCE_PLAY)GetProcAddress(context->Device->Dll, "alSourcePlay");
    alApi->alSourcePause          = (ALAPI_SOURCE_PAUSE)GetProcAddress(context->Device->Dll, "alSourcePause");
    alApi->alSourceStop           = (ALAPI_SOURCE_STOP)GetProcAddress(context->Device->Dll, "alSourceStop");
    alApi->alSourceRewind         = (ALAPI_SOURCE_STOP)GetProcAddress(context->Device->Dll, "alSourceRewind");

    alApi->alGenBuffers           = (ALAPI_GEN_BUFFERS)GetProcAddress(context->Device->Dll, "alGenBuffers");
    alApi->alDeleteBuffers        = (ALAPI_DELETE_BUFFERS)GetProcAddress(context->Device->Dll, "alDeleteBuffers");
    alApi->alIsBuffer             = (ALAPI_IS_BUFFER)GetProcAddress(context->Device->Dll, "alIsBuffer");
    alApi->alBufferData           = (ALAPI_BUFFER_DATA)GetProcAddress(context->Device->Dll, "alBufferData");
    alApi->alGetBufferi           = (ALAPI_GET_BUFFERI)GetProcAddress(context->Device->Dll, "alGetBufferi");
    alApi->alGetBufferf           = (ALAPI_GET_BUFFERF)GetProcAddress(context->Device->Dll, "alGetBufferf");

    alApi->alSourceQueueBuffers   = (ALAPI_SOURCE_QUEUE_BUFFERS)GetProcAddress(context->Device->Dll, "alSourceQueueBuffers");
    alApi->alSourceUnqueueBuffers = (ALAPI_SOURCE_UNQUEUE_BUFFERS)GetProcAddress(context->Device->Dll, "alSourceUnqueueBuffers");

    alApi->alDistanceModel        = (ALAPI_DISTANCE_MODEL)GetProcAddress(context->Device->Dll, "alDistanceModel");
    alApi->alDopplerFactor        = (ALAPI_DOPPLER_FACTOR)GetProcAddress(context->Device->Dll, "alDopplerFactor");
    alApi->alDopplerVelocity      = (ALAPI_DOPPLER_VELOCITY)GetProcAddress(context->Device->Dll, "alDopplerVelocity");


    alFxns = (alApi->alEnable               &&
              alApi->alDisable              &&
              alApi->alIsEnabled            &&

              alApi->alGetBoolean           &&
              alApi->alGetInteger           &&
              alApi->alGetFloat             &&
              alApi->alGetDouble            &&
              alApi->alGetBooleanv          &&
              alApi->alGetIntegerv          &&
              alApi->alGetFloatv            &&
              alApi->alGetDoublev           &&
              alApi->alGetString            &&

              alApi->alGetError             &&

              alApi->alIsExtensionPresent   &&
              alApi->alGetProcAddress       &&
              alApi->alGetEnumValue         &&

              alApi->alListeneri            &&
              alApi->alListenerf            &&
              alApi->alListener3f           &&
              alApi->alListenerfv           &&
              alApi->alGetListeneri         &&
              alApi->alGetListenerf         &&
              alApi->alGetListener3f        &&
              alApi->alGetListenerfv        &&

              alApi->alGenSources           &&
              alApi->alDeleteSources        &&
              alApi->alIsSource             &&
              alApi->alSourcei              &&
              alApi->alSourcef              &&
              alApi->alSource3f             &&
              alApi->alSourcefv             &&
              alApi->alGetSourcei           &&
              alApi->alGetSourcef           &&
              alApi->alGetSource3f          &&
              alApi->alGetSourcefv          &&
              alApi->alSourcePlayv          &&
              alApi->alSourcePausev         &&
              alApi->alSourceStopv          &&
              alApi->alSourceRewindv        &&
              alApi->alSourcePlay           &&
              alApi->alSourcePause          &&
              alApi->alSourceStop           &&
              alApi->alSourceRewind         &&

              alApi->alGenBuffers           &&
              alApi->alDeleteBuffers        &&
              alApi->alIsBuffer             &&
              alApi->alBufferData           &&
              alApi->alGetBufferi           &&
              alApi->alGetBufferf           &&

              alApi->alSourceQueueBuffers   &&
              alApi->alSourceUnqueueBuffers &&

              alApi->alDistanceModel        &&
              alApi->alDopplerFactor        &&
              alApi->alDopplerVelocity);

    return alFxns;
}


//*****************************************************************************
// FindDllWithMatchingSpecifier
//*****************************************************************************
//
HINSTANCE FindDllWithMatchingSpecifier(TCHAR* dllSearchPattern, char* specifier, bool partialName = false, char *actualName = NULL)
{
    WIN32_FIND_DATA findData;
    HANDLE searchHandle = INVALID_HANDLE_VALUE;
    TCHAR searchName[MAX_PATH + 1];
    TCHAR module[MAX_PATH + 1];
    TCHAR fileDrive[MAX_PATH + 1];
    TCHAR fileDir[MAX_PATH + 1];
    TCHAR fileName[MAX_PATH + 1];
    TCHAR fileExt[MAX_PATH + 1];
	TCHAR systemAL[MAX_PATH + 1];
    BOOL found = FALSE;
    ALubyte* deviceSpecifier = 0;
	ALCdevice *device;
	void *context;

    //
    // Directory[0] is the current directory
    // Directory[1] is the current OpenAL32.dll directory
    // Directory[2] is the system directory.
    //
    TCHAR dir[3][MAX_PATH + 1];
    DWORD dirSize = 0;
    int i;
    HINSTANCE dll = 0;
    ALCAPI_GET_STRING alcGetStringFxn = 0;
	ALCAPI_IS_EXTENSION_PRESENT alcIsExtensionPresentFxn = 0;
	ALCAPI_OPEN_DEVICE alcOpenDeviceFxn = 0;
	ALCAPI_CREATE_CONTEXT alcCreateContextFxn = 0;
	ALCAPI_MAKE_CONTEXT_CURRENT alcMakeContextCurrentFxn = 0;
	ALCAPI_DESTROY_CONTEXT alcDestroyContextFxn = 0;
	ALCAPI_CLOSE_DEVICE alcCloseDeviceFxn = 0;

    //
    // Construct our search paths.  We will search the current directory, the app directory, and then
    // the system directory.
    //
    dirSize = GetCurrentDirectory(MAX_PATH, dir[0]);
    _tcscat(dir[0], _T("\\"));

    GetModuleFileName(0, module, MAX_PATH);
    _splitpath(module, fileDrive, fileDir, fileName, fileExt);
    _tcscpy(dir[1], fileDrive);
    _tcscat(dir[1], fileDir);
    _tcscat(fileName, fileExt);

    dirSize = GetSystemDirectory(dir[2], MAX_PATH);
    _tcscat(dir[2], _T("\\"));
	_tcscpy(systemAL, dir[2]);
	_tcscat(systemAL, _T("OpenAL32.dll"));

    //
    // Begin searching for additional OpenAL implementations.  Start with the current directory, but check if
    // the current directory is the same as the application directory so we don't search it twice.
    //
    for(i = _tcsicmp(dir[0], dir[1]) != 0 ? 0 : 1; i < 3 && !found; i++)
    {
        _tcscpy(searchName, dir[i]);
        _tcscat(searchName, dllSearchPattern);
        searchHandle = FindFirstFile(searchName, &findData);
        if(searchHandle != INVALID_HANDLE_VALUE)
        {
            while(TRUE)
            {
                //
                // if this is an OpenAL32.dll, skip it -- it's probably a router and shouldn't be enumerated regardless
                //
				_tcscpy(searchName, dir[i]);
                _tcscat(searchName, findData.cFileName);
				TCHAR cmpName[MAX_PATH];
				_tcscpy(cmpName, searchName);
				_tcsupr(cmpName);
				if (strstr(cmpName, "OPENAL32.DLL") == 0)
                {
                    //
                    // Its not, so load it and see if the name matches.
                    //
                    dll = LoadLibrary(searchName);
                    if(dll)
                    {
						alcOpenDeviceFxn = (ALCAPI_OPEN_DEVICE)GetProcAddress(dll, "alcOpenDevice");
						alcCreateContextFxn = (ALCAPI_CREATE_CONTEXT)GetProcAddress(dll, "alcCreateContext");
						alcMakeContextCurrentFxn = (ALCAPI_MAKE_CONTEXT_CURRENT)GetProcAddress(dll, "alcMakeContextCurrent");
						alcGetStringFxn = (ALCAPI_GET_STRING)GetProcAddress(dll, "alcGetString");
						alcDestroyContextFxn = (ALCAPI_DESTROY_CONTEXT)GetProcAddress(dll, "alcDestroyContext");
						alcCloseDeviceFxn = (ALCAPI_CLOSE_DEVICE)GetProcAddress(dll, "alcCloseDevice");
						alcIsExtensionPresentFxn = (ALCAPI_IS_EXTENSION_PRESENT)GetProcAddress(dll, "alcIsExtensionPresent");

						if ((alcOpenDeviceFxn != 0) && 
							(alcCreateContextFxn != 0) &&
							(alcMakeContextCurrentFxn != 0) &&
							(alcGetStringFxn != 0) &&
							(alcDestroyContextFxn != 0) &&
							(alcCloseDeviceFxn != 0) &&
							(alcIsExtensionPresentFxn != 0)) {

							alcGetStringFxn = (ALCAPI_GET_STRING)GetProcAddress(dll, "alcGetString");
							if(alcGetStringFxn)
							{			
								if (alcIsExtensionPresentFxn(0, (ALubyte *)"ALC_ENUMERATION_EXT")) {
									// have an enumeratable DLL here, so check all available devices
									deviceSpecifier = alcGetStringFxn(0, ALC_DEVICE_SPECIFIER);
									do {
										if (deviceSpecifier != NULL) {
											if (partialName == false) {
												found = strcmp((char*)deviceSpecifier, specifier) == 0;
											} else {
												found = strstr((char*)deviceSpecifier, specifier) != 0;
												strcpy(actualName, (char *)deviceSpecifier);
											}
										} else {
											found = false;
										}
										deviceSpecifier += strlen((char *)deviceSpecifier) + 1;
									} while (!found && (strlen((char *)deviceSpecifier) > 0));

								} else {
									// no enumeration ability
									device = alcOpenDeviceFxn(NULL);
									if (device != NULL) {
										context = alcCreateContextFxn(device, NULL);
										alcMakeContextCurrentFxn((ALCcontext *)context);
										if (context != NULL) {
											deviceSpecifier = alcGetStringFxn(device, ALC_DEFAULT_DEVICE_SPECIFIER);
											if (deviceSpecifier != NULL) {
												if (partialName == false) {
													found = strcmp((char*)deviceSpecifier, specifier) == 0;
												} else {
													found = strstr((char*)deviceSpecifier, specifier) != 0;
													strcpy(actualName, (char *)deviceSpecifier);
												}
											} else {
												found = false;
											}

											alcMakeContextCurrentFxn((ALCcontext *)NULL);
											alcDestroyContextFxn((ALCcontext *)context);
											alcCloseDeviceFxn(device);
										}
									}
								}
							}
						} else {
							// handle legacy Creative DLLs which don't have alcIsExtensionPresentFxn exported (original Audigy, Live)
							device = alcOpenDeviceFxn(NULL);
							if (device != NULL) {
								context = alcCreateContextFxn(device, NULL);
								alcMakeContextCurrentFxn((ALCcontext *)context);
								if (context != NULL) {
									deviceSpecifier = alcGetStringFxn(device, ALC_DEFAULT_DEVICE_SPECIFIER);
									if (deviceSpecifier != NULL) {
										if (partialName == false) {
											found = strcmp((char*)deviceSpecifier, specifier) == 0;
										} else {
											found = strstr((char*)deviceSpecifier, specifier) != 0;
											strcpy(actualName, (char *)deviceSpecifier);
										}
									} else {
										found = false;
									}

									alcMakeContextCurrentFxn((ALCcontext *)NULL);
									alcDestroyContextFxn((ALCcontext *)context);
									alcCloseDeviceFxn(device);
								}
							}
						}

                        if(found)
                        {
                            break;
                        }

                        else
                        {
                            FreeLibrary(dll);
                            dll = 0;
                        }
                    }
                }

                if(!FindNextFile(searchHandle, &findData))
                {
                    if(GetLastError() == ERROR_NO_MORE_FILES)
                    {
                        break;
                    }
                }
            }

            FindClose(searchHandle);
            searchHandle = INVALID_HANDLE_VALUE;
        }
    }

    return dll;
}


//*****************************************************************************
//*****************************************************************************
//
// ALC API Entry Points
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
// alcCloseDevice
//*****************************************************************************
//
ALCAPI ALvoid ALCAPIENTRY alcCloseDevice(ALCdevice* device)
{
    if(!device)
    {
        return;
    }

    if(IsBadReadPtr(device, sizeof(ALCdevice)))
    {
        return;
    }

    //
    // Check if its linked to a context.
    //
    if(device->InUse)
    {
        ALCcontext* context = 0;
        ALlistEntry* entry = 0;

        //
        // Not all of the contexts using the device have been destroyed.
        //
        assert(0);

        //
        // Loop through the context list and free and contexts linked to the device.
        // Go back to the beginning each time in case some one changed the context
        // list iterator.
        //
        alListAcquireLock(alContextList);
        entry = alListIteratorReset(alContextList);
        while(entry)
        {
            context = (ALCcontext*)alListGetData(entry);
            if(context->Device == device)
            {
                alListReleaseLock(alContextList);
                alcDestroyContext((ALCcontext *)context);
                alListAcquireLock(alContextList);
                entry = alListIteratorReset(alContextList);
            }

            else
            {
                entry = alListIteratorNext(alContextList);
            }
        }

        alListReleaseLock(alContextList);
        assert(!device->InUse);
    }

    device->AlcApi.alcCloseDevice(device->DllDevice);
    FreeLibrary(device->Dll);
    free(device);
}


//*****************************************************************************
// alcCreateContext
//*****************************************************************************
//
ALCAPI ALCcontext* ALCAPIENTRY alcCreateContext(ALCdevice* device, ALint* attrList)
{
    ALCcontext* context = 0;

    if(!device)
    {
        LastError = ALC_INVALID_DEVICE;
        return 0;
    }

    if(IsBadReadPtr(device, sizeof(ALCdevice)))
    {
        LastError = ALC_INVALID_DEVICE;
        return 0;
    }

    if(IsBadWritePtr(device, sizeof(ALCdevice)))
    {
        LastError = ALC_INVALID_DEVICE;
        return 0;
    }


    //
    // Allocate the context.
    //
    context = (ALCcontext*)malloc(sizeof(ALCcontext));
    if(!context)
    {
        return 0;
    }

    memset(context, 0, sizeof(ALCcontext));
    context->Device = device;
    context->Suspended = FALSE;
    context->LastError = AL_NO_ERROR;
    InitializeCriticalSection(&context->Lock);

    //
    // We don't fill out the AL functions in case they are context specific.
    //

    context->DllContext = device->AlcApi.alcCreateContext(device->DllDevice, attrList);
    if(!context->DllContext)
    {
        DeleteCriticalSection(&context->Lock);
        free(context);
        context = 0;
        return 0;
    }

    device->InUse++;

    //
    // Add it to the context list.
    //
    alListInitializeEntry(&context->ListEntry, context);
    alListAcquireLock(alContextList);
    alListAddEntry(alContextList, &context->ListEntry);
    alListReleaseLock(alContextList);
    return context;
}


//*****************************************************************************
// alcDestroyContext
//*****************************************************************************
//
ALCAPI ALvoid ALCAPIENTRY alcDestroyContext(ALCcontext* context)
{
    ALCcontext* listData = 0;

    if(!context)
    {
        return;
    }

    //
    // Remove the entry from the context list.
    //
    alListAcquireLock(alContextList);
    listData = (ALCcontext*)alListRemoveEntry(alContextList, &context->ListEntry);
    if(!listData)
    {
        alListReleaseLock(alContextList);
        return;
    }

    if(context == alCurrentContext)
    {
        alCurrentContext = 0;
    }

    EnterCriticalSection(&context->Lock);
    alListReleaseLock(alContextList);

    context->Device->InUse--;

    // Clean up the context.
    if(context->DllContext)
    {
        context->Device->AlcApi.alcDestroyContext(context->DllContext);
    }

    LeaveCriticalSection(&context->Lock);
    DeleteCriticalSection(&context->Lock);
    free(context);
}


//*****************************************************************************
// alcGetContextsDevice
//*****************************************************************************
//
ALCAPI ALCdevice* ALCAPIENTRY alcGetContextsDevice(ALCcontext* context)
{
    ALCdevice* ALCdevice = 0;

    alListAcquireLock(alContextList);
    if(alListMatchData(alContextList, context))
    {
        ALCdevice = context->Device;
    }

    alListReleaseLock(alContextList);

    return ALCdevice;
}


//*****************************************************************************
// alcGetCurrentContext
//*****************************************************************************
//
ALCAPI ALCcontext* ALCAPIENTRY alcGetCurrentContext(ALvoid)
{
    return (ALCcontext *)alCurrentContext;
}


//*****************************************************************************
// alcGetEnumValue
//*****************************************************************************
//
ALCAPI ALenum ALCAPIENTRY alcGetEnumValue(ALCdevice* device, ALubyte* ename)
{
    //
    // Always return the router version of the ALC enum if it exists.
    //
    ALsizei i = 0;
    while(alcEnums[i].ename && strcmp((char*)alcEnums[i].ename, (char*)ename))
    {
        i++;
    }

    if(alcEnums[i].ename)
    {
        return alcEnums[i].value;
    }

    if(device)
    {
        if(IsBadReadPtr(device, sizeof(ALCdevice)))
        {
            LastError = ALC_INVALID_DEVICE;
            return 0;
        }

        return device->AlcApi.alcGetEnumValue(device->DllDevice, ename);
    }

    LastError = ALC_INVALID_ENUM;
    return 0;
}


//*****************************************************************************
// alcGetError
//*****************************************************************************
//
ALCAPI ALenum ALCAPIENTRY alcGetError(ALCdevice* device)
{
    ALenum errorCode = AL_NO_ERROR;

    // Try to get a valid device.
    if(!device)
    {
        errorCode = LastError;
        LastError = AL_NO_ERROR;
        return errorCode;
    }

    if(IsBadReadPtr(device, sizeof(ALCdevice)))
    {
        LastError = ALC_INVALID_DEVICE;
        return 0;
    }

    //
    // Check if its a 3rd party device.
    //
    errorCode = device->AlcApi.alcGetError(device->DllDevice);
    return errorCode;
}


//*****************************************************************************
// alcGetIntegerv
//*****************************************************************************
//
ALCAPI ALvoid ALCAPIENTRY alcGetIntegerv(ALCdevice* device, ALenum param, ALsizei size, ALint* data)
{
    if(device)
    {
        if(IsBadReadPtr(device, sizeof(ALCdevice)))
        {
            LastError = ALC_INVALID_DEVICE;
            return;
        }

        device->AlcApi.alcGetIntegerv(device->DllDevice, param, size, data);
        return;
    }

    switch(param)
    {
        case ALC_MAJOR_VERSION:
        {
            if(size < sizeof(ALint) || IsBadReadPtr(data, sizeof(ALint)))
            {
                LastError = ALC_INVALID;
                return;
            }

            *data = alcMajorVersion;
        }
        break;

        case ALC_MINOR_VERSION:
        {
            if(size < sizeof(ALint) || IsBadReadPtr(data, sizeof(ALint)))
            {
                LastError = ALC_INVALID;
                return;
            }

            *data = alcMinorVersion;
        }
        break;

        default:
        {
            device->LastError = ALC_INVALID_ENUM;
        }
        break;
    }
}


//*****************************************************************************
// alcGetProcAddress
//*****************************************************************************
//
ALCAPI ALvoid* ALCAPIENTRY alcGetProcAddress(ALCdevice* device, ALubyte* fname)
{
    //
    // Always return the router version of the ALC function if it exists.
    //
    ALsizei i = 0;
    while(alcFunctions[i].fname && strcmp((char*)alcFunctions[i].fname, (char*)fname))
    {
        i++;
    }

    if(alcFunctions[i].fname)
    {
        return alcFunctions[i].address;
    }

    if(device)
    {
        if(IsBadReadPtr(device, sizeof(ALCdevice)))
        {
            LastError = ALC_INVALID_DEVICE;
            return 0;
        }

        return device->AlcApi.alcGetProcAddress(device->DllDevice, fname);
    }

    LastError = ALC_INVALID_ENUM;
    return 0;
}


//*****************************************************************************
// alcIsExtensionPresent
//*****************************************************************************
//
ALCAPI ALboolean ALCAPIENTRY alcIsExtensionPresent(ALCdevice* device, ALubyte* ename)
{
    //
    // Check if its a router supported extension first as its a good idea to have
    // ALC calls go through the router if possible.
    //
    ALsizei i = 0;
    while(alcExtensions[i].ename && strcmp((char*)alcExtensions[i].ename, (char*)ename))
    {
        i++;
    }

    if(alcExtensions[i].ename)
    {
        return AL_TRUE;
    }

    //
    // Check the device passed in to see if the extension is supported.
    //
    if(device)
    {
        if(IsBadReadPtr(device, sizeof(ALCdevice)))
        {
            LastError = ALC_INVALID_DEVICE;
            return 0;
        }

        return device->AlcApi.alcIsExtensionPresent(device->DllDevice, ename);
    }

    LastError = ALC_INVALID_ENUM;
    return AL_FALSE;
}


//*****************************************************************************
// alcMakeContextCurrent
//*****************************************************************************
//
ALCAPI ALboolean ALCAPIENTRY alcMakeContextCurrent(ALCcontext* context)
{
    ALboolean contextSwitched = AL_TRUE;

    //
    // Context must be a valid context or 0
    //
    alListAcquireLock(alContextList);
    if(!alListMatchData(alContextList, context) && context != 0)
    {
        alListReleaseLock(alContextList);
        return AL_FALSE;
    }

    //
    // Notify a third party OpenAL implementation of a change in contexts.
    //
    if(alCurrentContext)
    {
        if(context)
        {
            contextSwitched = alCurrentContext->Device->AlcApi.alcMakeContextCurrent(context->DllContext);
        }

        else
        {
            contextSwitched = alCurrentContext->Device->AlcApi.alcMakeContextCurrent(0);
        }
    }

    //
    // Try the new context.  If both are 0, then we really aren't changing the context are we.
    //
    else if(context)
    {
        contextSwitched = context->Device->AlcApi.alcMakeContextCurrent(context->DllContext);

        //
        // If this is the first time the context has been made the current context, fill in the context
        // function pointers.
        //
        if(contextSwitched && !context->AlApi.alGetProcAddress)
        {
            //
            // Don't fill out the functions here in case they are context specific pointers in the device.
            //
            if(!FillOutAlFunctions(context))
            {
                LastError = ALC_INVALID_CONTEXT;
                contextSwitched = AL_FALSE;

                //
                // Something went wrong, restore the old context.
                //
                if(alCurrentContext)
                {
                    alCurrentContext->Device->AlcApi.alcMakeContextCurrent(alCurrentContext->DllContext);
                }

                else
                {
                    alCurrentContext->Device->AlcApi.alcMakeContextCurrent(0);
                }
            }
        }
    }

    //
    // Set the context states if the switch was successful.
    //
    if(contextSwitched)
    {
        alCurrentContext = context;
    }

    alListReleaseLock(alContextList);
    return contextSwitched;
}


//*****************************************************************************
// alcOpenDevice
//*****************************************************************************
//
ALCAPI ALCdevice* ALCAPIENTRY alcOpenDevice(ALubyte* deviceName)
{
    HINSTANCE dll = 0;
    ALCdevice* device = 0;


    //
    // Initialize the OpenAL device structure
    //
    device = (ALCdevice*)malloc(sizeof(ALCdevice));
    if(!device)
    {
        return 0;
    }

    memset(device, 0, sizeof(ALCdevice));
    device->LastError = AL_NO_ERROR;
    device->InUse = 0;

    //
    // Make sure we atleast have some device name.
    //
    if ((!deviceName) || (strcmp((char *)deviceName, "DirectSound3D") == 0))
    {
        deviceName = alcGetString(0, ALC_DEFAULT_DEVICE_SPECIFIER);
    }

	//
    // Find the device to open.
    //
    dll = FindDllWithMatchingSpecifier(_T("nvopenal.dll"), (char*)deviceName);
    if(!dll)
    {
        dll = FindDllWithMatchingSpecifier(_T("*oal.dll"), (char*)deviceName);
        if(!dll)
        {
            //
            // If we still don't have a match for these default names, try the default string.
            //
            if(!dll &&
               (strcmp((char*)deviceName, "DirectSound3D") == 0 ||
                strcmp((char*)deviceName, "DirectSound")   == 0 ||
                strcmp((char*)deviceName, "MMSYSTEM")      == 0))
            {
                deviceName = alcGetString(0, ALC_DEFAULT_DEVICE_SPECIFIER);
                dll = FindDllWithMatchingSpecifier(_T("nvopenal.dll"), (char*)deviceName);
                if(!dll)
                {
                    dll = FindDllWithMatchingSpecifier(_T("*oal.dll"), (char*)deviceName);
                    if(!dll)
                    {
                        goto NoDll;
                    }
                }
            }
        }
    }

    device->Dll = dll;
    if(!FillOutAlcFunctions(device))
    {
        goto OpenDeviceFailed;
    }

    device->DllDevice = device->AlcApi.alcOpenDevice(deviceName);
    if(!device->DllDevice)
    {
        goto OpenDeviceFailed;
    }

    goto NoError;


    //
    // Clean up if we encountered an error.
    //
OpenDeviceFailed:
    FreeLibrary(dll);
    dll = 0;

NoDll:
    free(device);
    device = 0;
    LastError = ALC_INVALID_DEVICE;

NoError:
    return (ALCdevice *)device;
}


//*****************************************************************************
// alcProcessContext
//*****************************************************************************
//
ALCAPI ALvoid ALCAPIENTRY alcProcessContext(ALCcontext* context)
{
    alListAcquireLock(alContextList);
    if(!context && !alCurrentContext)
    {
        alListReleaseLock(alContextList);
        return;
    }

    if(!context)
    {
        context = alCurrentContext;
    }

    EnterCriticalSection(&context->Lock);
    alListReleaseLock(alContextList);

    if(context->DllContext)
    {
        context->Device->AlcApi.alcProcessContext(context->DllContext);
    }

    context->Suspended = FALSE;

    LeaveCriticalSection(&context->Lock);
    return;
}


//*****************************************************************************
// alcSuspendContext
//*****************************************************************************
//
ALCAPI ALCvoid ALCAPIENTRY alcSuspendContext(ALCcontext* context)
{
    alListAcquireLock(alContextList);
    if(!context && !alCurrentContext)
    {
        alListReleaseLock(alContextList);
        return;
    }

    if(!context)
    {
        context = (ALCcontext *)alCurrentContext;
    }

    EnterCriticalSection(&context->Lock);
    alListReleaseLock(alContextList);

    context->Suspended = TRUE;

    if(context->DllContext)
    {
        context->Device->AlcApi.alcSuspendContext(context->DllContext);
    }

    LeaveCriticalSection(&context->Lock);
    return;
}











//*****************************************************************************
// alcGetString
//*****************************************************************************
//
ALCAPI ALubyte* ALCAPIENTRY alcGetString(ALCdevice* device, ALenum param)
{
    ALubyte* value = 0;

    if(device)
    {
        return device->AlcApi.alcGetString(device->DllDevice, param);
    }

    switch(param)
    {
        case ALC_NO_ERROR:
        {
            value = alcNoError;
        }
        break;

        case ALC_INVALID_ENUM:
        {
            value = alcErrInvalidEnum;
        }
        break;

        case ALC_INVALID_VALUE:
        {
            value = alcErrInvalidValue;
        }
        break;

        case ALC_INVALID_DEVICE:
        {
            value = alcErrInvalidDevice;
        }
        break;

        case ALC_INVALID_CONTEXT:
        {
            value = alcErrInvalidContext;
        }
        break;

        case ALC_DEFAULT_DEVICE_SPECIFIER:
        {
            while(TRUE)
            {
                //
                // See if we can find a native implementation for the user's current device.
                //
                char* specifier = 0;
                HINSTANCE dll = 0;
				char mixerDevice[32];
				bool acceptPartial = false;
				char actualName[32];
				UINT uDeviceID;
				DWORD dwFlags=1;
				WAVEOUTCAPS info;

				// two issues here --
				// 1) whatever device is in the control panel as the preferred device should be accepted
				// 2) if the preferred device is an Audigy, then the name won't match the hardware DLL, so 
				//    allow a partial match in this case
				__asm pusha; // workaround for register destruction caused by these wavOutMessage calls (weird but true)
				waveOutMessage((HWAVEOUT)WAVE_MAPPER,0x2000+0x0015,(LPARAM)&uDeviceID,(WPARAM)&dwFlags);
				waveOutGetDevCaps(uDeviceID,&info,sizeof(info));
				__asm popa;
				strcpy(mixerDevice, T2A(info.szPname));
				if (strstr(mixerDevice, "Audigy") != NULL) {
					acceptPartial = true;
					strcpy(mixerDevice, "Audigy");
				}
                dll = FindDllWithMatchingSpecifier(_T("nvopenal.dll"), mixerDevice, acceptPartial, actualName);
                if(!dll)
                {
                    dll = FindDllWithMatchingSpecifier(_T("*oal.dll"), mixerDevice, acceptPartial, actualName);
                }

                if(dll)
                {
					if (acceptPartial == true) { // legacy Audigy issue again...
						strcpy(mixerDevice, actualName);
					}
                    strcpy((char*)alcDefaultDeviceSpecifier, mixerDevice);
                    FreeLibrary(dll);
                    break;
                }

                //
                // Try to find a default version.
                //
                dll = FindDllWithMatchingSpecifier(_T("nvopenal.dll"), "DirectSound3D");
                if(!dll)
                {
                    dll = FindDllWithMatchingSpecifier(_T("*oal.dll"), "DirectSound3D");
                }

                if(dll)
                {
                    strcpy((char*)alcDefaultDeviceSpecifier, "DirectSound3D");
                    FreeLibrary(dll);
                    break;
                }

                dll = FindDllWithMatchingSpecifier(_T("nvopenal.dll"), "DirectSound");
                if(!dll)
                {
                    dll = FindDllWithMatchingSpecifier(_T("*oal.dll"), "DirectSound");
                }

                if(dll)
                {
                    strcpy((char*)alcDefaultDeviceSpecifier, "DirectSound");
                    FreeLibrary(dll);
                    break;
                }

                dll = FindDllWithMatchingSpecifier(_T("nvopenal.dll"), "MMSYSTEM");
                if(!dll)
                {
                    dll = FindDllWithMatchingSpecifier(_T("*oal.dll"), "MMSYSTEM");
                }

                if(dll)
                {
                    strcpy((char*)alcDefaultDeviceSpecifier, "MMSYSTEM");
                    FreeLibrary(dll);
                    break;
                }

                memset((char*)alcDefaultDeviceSpecifier, 0, MAX_PATH * sizeof(char));
                break;
            }

            return alcDefaultDeviceSpecifier;
        }
        break;

        case ALC_DEVICE_SPECIFIER:
        {
            BuildDeviceSpecifierList();
            return alcDeviceSpecifierList;
        }
        break;

        default:
            LastError = ALC_INVALID_ENUM;
            break;
    }

    return value;
}













