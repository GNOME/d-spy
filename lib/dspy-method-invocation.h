/* dspy-method-invocation.h
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

#include <gio/gio.h>

#include "dspy-name.h"
#include "dspy-version-macros.h"

G_BEGIN_DECLS

#define DSPY_TYPE_METHOD_INVOCATION (dspy_method_invocation_get_type())

DSPY_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (DspyMethodInvocation, dspy_method_invocation, DSPY, METHOD_INVOCATION, GObject)

struct _DspyMethodInvocationClass
{
  GObjectClass parent_class;

  /*< private >*/
  gpointer _reserved[8];
};

DSPY_AVAILABLE_IN_ALL
DspyMethodInvocation *dspy_method_invocation_new                 (void);
DSPY_AVAILABLE_IN_ALL
const gchar          *dspy_method_invocation_get_interface       (DspyMethodInvocation  *self);
DSPY_AVAILABLE_IN_ALL
const gchar          *dspy_method_invocation_get_object_path     (DspyMethodInvocation  *self);
DSPY_AVAILABLE_IN_ALL
const gchar          *dspy_method_invocation_get_method          (DspyMethodInvocation  *self);
DSPY_AVAILABLE_IN_ALL
const gchar          *dspy_method_invocation_get_signature       (DspyMethodInvocation  *self);
DSPY_AVAILABLE_IN_ALL
const gchar          *dspy_method_invocation_get_reply_signature (DspyMethodInvocation  *self);
DSPY_AVAILABLE_IN_ALL
GVariant             *dspy_method_invocation_get_parameters      (DspyMethodInvocation  *self);
DSPY_AVAILABLE_IN_ALL
DspyName             *dspy_method_invocation_get_name            (DspyMethodInvocation  *self);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_set_interface       (DspyMethodInvocation  *self,
                                                                  const gchar           *interface);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_set_method          (DspyMethodInvocation  *self,
                                                                  const gchar           *method);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_set_object_path     (DspyMethodInvocation  *self,
                                                                  const gchar           *object_path);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_set_signature       (DspyMethodInvocation  *self,
                                                                  const gchar           *signature);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_set_reply_signature (DspyMethodInvocation  *self,
                                                                  const gchar           *reply_signature);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_set_name            (DspyMethodInvocation  *self,
                                                                  DspyName              *name);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_set_parameters      (DspyMethodInvocation  *self,
                                                                  GVariant              *parameters);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_execute_async       (DspyMethodInvocation  *self,
                                                                  GCancellable          *cancellable,
                                                                  GAsyncReadyCallback    callback,
                                                                  gpointer               user_data);
DSPY_AVAILABLE_IN_ALL
GVariant             *dspy_method_invocation_execute_finish      (DspyMethodInvocation  *self,
                                                                  GAsyncResult          *result,
                                                                  GError               **error);
DSPY_AVAILABLE_IN_ALL
gint                  dspy_method_invocation_get_timeout         (DspyMethodInvocation  *self);
DSPY_AVAILABLE_IN_ALL
void                  dspy_method_invocation_set_timeout         (DspyMethodInvocation  *self,
                                                                  gint                   timout);

G_END_DECLS
