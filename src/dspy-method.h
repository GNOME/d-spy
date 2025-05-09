/*
 * dspy-method.h
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

#include "dspy-introspectable.h"

G_BEGIN_DECLS

#define DSPY_TYPE_METHOD (dspy_method_get_type())

G_DECLARE_FINAL_TYPE (DspyMethod, dspy_method, DSPY, METHOD, DspyIntrospectable)

struct _DspyMethod
{
  DspyIntrospectable  parent_instance;
  char               *name;
  GQueue              in_args;
  GQueue              out_args;
};

static inline GListModel *
dspy_method_dup_in_arguments (DspyMethod *self)
{
  return dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->in_args);
}

static inline GListModel *
dspy_method_dup_out_arguments (DspyMethod *self)
{
  return dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->out_args);
}

char *dspy_method_dup_in_signature  (DspyMethod *self);
char *dspy_method_dup_out_signature (DspyMethod *self);

G_END_DECLS
