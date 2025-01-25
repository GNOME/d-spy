/* dspy-binding-group.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 * Copyright (C) 2015 Garrett Regier <garrettregier@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define DSPY_TYPE_BINDING_GROUP (dspy_binding_group_get_type())

G_DECLARE_FINAL_TYPE (DspyBindingGroup, dspy_binding_group, DSPY, BINDING_GROUP, GObject)

DspyBindingGroup *dspy_binding_group_new                (void);
GObject         *dspy_binding_group_get_source         (DspyBindingGroup       *self);
void             dspy_binding_group_set_source         (DspyBindingGroup       *self,
                                                       gpointer               source);
void             dspy_binding_group_bind               (DspyBindingGroup       *self,
                                                       const gchar           *source_property,
                                                       gpointer               target,
                                                       const gchar           *target_property,
                                                       GBindingFlags          flags);
void             dspy_binding_group_bind_full          (DspyBindingGroup       *self,
                                                       const gchar           *source_property,
                                                       gpointer               target,
                                                       const gchar           *target_property,
                                                       GBindingFlags          flags,
                                                       GBindingTransformFunc  transform_to,
                                                       GBindingTransformFunc  transform_from,
                                                       gpointer               user_data,
                                                       GDestroyNotify         user_data_destroy);
void             dspy_binding_group_bind_with_closures (DspyBindingGroup       *self,
                                                       const gchar           *source_property,
                                                       gpointer               target,
                                                       const gchar           *target_property,
                                                       GBindingFlags          flags,
                                                       GClosure              *transform_to,
                                                       GClosure              *transform_from);

G_END_DECLS
