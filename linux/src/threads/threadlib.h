/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * threadlib.h
 *
 * Header which sorts out which thread package we're using.
 */
#ifndef THREADLIB_H_
#define THREADLIB_H_

#include "al_siteconfig.h"

#if defined(USE_POSIXTHREADS)

#include "posixthreads.h"

#elif defined(USE_WINDOWSTHREADS)

#include "windowsthreads.h"

#elif defined(USE_MORPHOSTHREADS)

#include "morphosthreads.h"

#else

#error "No thread package"

#endif

#endif /* THREADLIB_H_ */
