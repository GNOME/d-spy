/*
 * dspy-method-argument.h
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

#include <glib-object.h>

G_BEGIN_DECLS

#define DSPY_TYPE_METHOD_ARGUMENT (dspy_method_argument_get_type())

G_DECLARE_FINAL_TYPE (DspyMethodArgument, dspy_method_argument, DSPY, METHOD_ARGUMENT, GObject)

DspyMethodArgument *dspy_method_argument_new            (const char         *signature,
                                                         const char         *name,
                                                         GVariant           *value);
const char         *dspy_method_argument_get_name       (DspyMethodArgument *self);
const char         *dspy_method_argument_get_signature  (DspyMethodArgument *self);
GVariant           *dspy_method_argument_dup_value      (DspyMethodArgument *self);
void                dspy_method_argument_set_value      (DspyMethodArgument *self,
                                                         GVariant           *value);
gboolean            dspy_method_argument_has_error      (DspyMethodArgument *self);
char               *dspy_method_argument_dup_value_text (DspyMethodArgument *self);
void                dspy_method_argument_set_value_text (DspyMethodArgument *self,
                                                         const char         *value_text);

G_END_DECLS
