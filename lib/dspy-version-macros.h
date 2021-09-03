/* dspy-version-macros.h
 *
 * Copyright 2021 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <glib.h>

#include "dspy-version.h"

#ifndef _DSPY_EXTERN
#define _DSPY_EXTERN extern
#endif

#ifdef DSPY_DISABLE_DEPRECATION_WARNINGS
# define DSPY_DEPRECATED _DSPY_EXTERN
# define DSPY_DEPRECATED_FOR(f) _DSPY_EXTERN
# define DSPY_UNAVAILABLE(maj,min) _DSPY_EXTERN
#else
# define DSPY_DEPRECATED G_DEPRECATED _DSPY_EXTERN
# define DSPY_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _DSPY_EXTERN
# define DSPY_UNAVAILABLE(maj,min) G_UNAVAILABLE(maj,min) _DSPY_EXTERN
#endif

#define DSPY_VERSION_42_0 (G_ENCODE_VERSION (42, 0))

#if (DSPY_MINOR_VERSION == 99)
# define DSPY_VERSION_CUR_STABLE (G_ENCODE_VERSION (DSPY_MAJOR_VERSION + 1, 0))
#elif (DSPY_MINOR_VERSION % 2)
# define DSPY_VERSION_CUR_STABLE (G_ENCODE_VERSION (DSPY_MAJOR_VERSION, DSPY_MINOR_VERSION + 1))
#else
# define DSPY_VERSION_CUR_STABLE (G_ENCODE_VERSION (DSPY_MAJOR_VERSION, DSPY_MINOR_VERSION))
#endif

#if (DSPY_MINOR_VERSION == 99)
# define DSPY_VERSION_PREV_STABLE (G_ENCODE_VERSION (DSPY_MAJOR_VERSION + 1, 0))
#elif (DSPY_MINOR_VERSION % 2)
# define DSPY_VERSION_PREV_STABLE (G_ENCODE_VERSION (DSPY_MAJOR_VERSION, DSPY_MINOR_VERSION - 1))
#else
# define DSPY_VERSION_PREV_STABLE (G_ENCODE_VERSION (DSPY_MAJOR_VERSION, DSPY_MINOR_VERSION - 2))
#endif

/**
 * DSPY_VERSION_MIN_REQUIRED:
 *
 * A macro that should be defined by the user prior to including
 * the dspying.h header.
 *
 * The definition should be one of the predefined D-Spy version
 * macros: %DSPY_VERSION_42_0, ...
 *
 * This macro defines the lower bound for the D-Spy API to use.
 *
 * If a function has been deprecated in a newer version of D-Spy,
 * it is possible to use this symbol to avoid the compiler warnings
 * without disabling warning for every deprecated function.
 */
#ifndef DSPY_VERSION_MIN_REQUIRED
# define DSPY_VERSION_MIN_REQUIRED (DSPY_VERSION_CUR_STABLE)
#endif

/**
 * DSPY_VERSION_MAX_ALLOWED:
 *
 * A macro that should be defined by the user prior to including
 * the dspying.h header.

 * The definition should be one of the predefined D-Spy version
 * macros: %DSPY_VERSION_42_0, %DSPY_VERSION_1_2,...
 *
 * This macro defines the upper bound for the D-Spy API to use.
 *
 * If a function has been introduced in a newer version of D-Spy,
 * it is possible to use this symbol to get compiler warnings when
 * trying to use that function.
 */
#ifndef DSPY_VERSION_MAX_ALLOWED
# if DSPY_VERSION_MIN_REQUIRED > DSPY_VERSION_PREV_STABLE
#  define DSPY_VERSION_MAX_ALLOWED (DSPY_VERSION_MIN_REQUIRED)
# else
#  define DSPY_VERSION_MAX_ALLOWED (DSPY_VERSION_CUR_STABLE)
# endif
#endif

#if DSPY_VERSION_MAX_ALLOWED < DSPY_VERSION_MIN_REQUIRED
#error "DSPY_VERSION_MAX_ALLOWED must be >= DSPY_VERSION_MIN_REQUIRED"
#endif
#if DSPY_VERSION_MIN_REQUIRED < DSPY_VERSION_42_0
#error "DSPY_VERSION_MIN_REQUIRED must be >= DSPY_VERSION_42_0"
#endif

#define DSPY_AVAILABLE_IN_ALL                  _DSPY_EXTERN

#if DSPY_VERSION_MIN_REQUIRED >= DSPY_VERSION_42_0
# define DSPY_DEPRECATED_IN_42_0                DSPY_DEPRECATED
# define DSPY_DEPRECATED_IN_42_0_FOR(f)         DSPY_DEPRECATED_FOR(f)
#else
# define DSPY_DEPRECATED_IN_42_0                _DSPY_EXTERN
# define DSPY_DEPRECATED_IN_42_0_FOR(f)         _DSPY_EXTERN
#endif

#if DSPY_VERSION_MAX_ALLOWED < DSPY_VERSION_42_0
# define DSPY_AVAILABLE_IN_42_0                 DSPY_UNAVAILABLE(42, 0)
#else
# define DSPY_AVAILABLE_IN_42_0                 _DSPY_EXTERN
#endif
