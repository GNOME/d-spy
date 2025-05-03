/* dspy-private.h
 *
 * Copyright 2019 Christian Hergert <chergert@redhat.com>
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

#include "dspy-name.h"

G_BEGIN_DECLS

void  _dspy_name_clear_pid          (DspyName                 *name);
void  _dspy_name_refresh_pid        (DspyName                 *name,
                                     GDBusConnection          *connection);
void  _dspy_name_refresh_owner      (DspyName                 *name,
                                     GDBusConnection          *connection);
void  _dspy_name_set_owner          (DspyName                 *self,
                                     const gchar              *owner);
void  _dspy_name_set_activatable    (DspyName                 *name,
                                     gboolean                  is_activatable);
char *_dspy_signature_humanize      (const gchar              *signature);

G_END_DECLS
