/*
 * dspy-property.h
 *
 * Copyright 2025 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <libdex.h>

#include "dspy-introspectable.h"

G_BEGIN_DECLS

#define DSPY_TYPE_PROPERTY (dspy_property_get_type())

G_DECLARE_FINAL_TYPE (DspyProperty, dspy_property, DSPY, PROPERTY, DspyIntrospectable)

struct _DspyProperty
{
  DspyIntrospectable      parent_instance;
  char                   *name;
  char                   *signature;
  char                   *value;
  GDBusPropertyInfoFlags  flags;
};

DexFuture *dspy_property_query_value (DspyProperty    *self,
                                      GDBusConnection *connection,
                                      const char      *name) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS
