/*
 * dspy-interface.c
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

#include "config.h"

#include <gtk/gtk.h>

#include "dspy-interface.h"

enum {
  PROP_0,
  PROP_NAME,
  PROP_PROPERTIES,
  PROP_SIGNALS,
  PROP_MEMBERS,
  PROP_METHODS,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DspyInterface, dspy_interface, DSPY_TYPE_INTROSPECTABLE)

static GParamSpec *properties[N_PROPS];

static char *
dspy_interface_dup_title (DspyIntrospectable *introspectable)
{
  DspyInterface *self = DSPY_INTERFACE (introspectable);

  return g_strdup (self->name);
}

static void
dspy_interface_finalize (GObject *object)
{
  DspyInterface *self = (DspyInterface *)object;

  dspy_introspectable_clear_queue (DSPY_INTROSPECTABLE (self), &self->properties);
  dspy_introspectable_clear_queue (DSPY_INTROSPECTABLE (self), &self->signals);
  dspy_introspectable_clear_queue (DSPY_INTROSPECTABLE (self), &self->methods);
  g_clear_pointer (&self->name, g_free);

  G_OBJECT_CLASS (dspy_interface_parent_class)->finalize (object);
}

static void
dspy_interface_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  DspyInterface *self = DSPY_INTERFACE (object);

  switch (prop_id)
    {
    case PROP_PROPERTIES:
      g_value_take_object (value, dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->properties));
      break;

    case PROP_MEMBERS:
      g_value_take_object (value, dspy_interface_list_members (self));
      break;

    case PROP_METHODS:
      g_value_take_object (value, dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->methods));
      break;

    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_SIGNALS:
      g_value_take_object (value, dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->signals));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_interface_class_init (DspyInterfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DspyIntrospectableClass *introspectable_class = DSPY_INTROSPECTABLE_CLASS (klass);

  object_class->finalize = dspy_interface_finalize;
  object_class->get_property = dspy_interface_get_property;

  introspectable_class->dup_title = dspy_interface_dup_title;

  properties[PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_PROPERTIES] =
    g_param_spec_object ("properties", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_MEMBERS] =
    g_param_spec_object ("members", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_METHODS] =
    g_param_spec_object ("methods", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_SIGNALS] =
    g_param_spec_object ("signals", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_interface_init (DspyInterface *self)
{
}

GListModel *
dspy_interface_list_members (DspyInterface *self)
{
  g_autoptr(GListModel) methods = NULL;
  g_autoptr(GListModel) props = NULL;
  g_autoptr(GListModel) signals = NULL;
  GListStore *store;

  g_return_val_if_fail (DSPY_IS_INTERFACE (self), NULL);

  store = g_list_store_new (G_TYPE_LIST_MODEL);

  g_object_get (self,
                "methods", &methods,
                "signals", &signals,
                "properties", &props,
                NULL);

  if (signals != NULL)
    g_list_store_append (store, signals);

  if (props != NULL)
    g_list_store_append (store, props);

  if (methods != NULL)
    g_list_store_append (store, methods);

  return G_LIST_MODEL (gtk_flatten_list_model_new (G_LIST_MODEL (store)));
}
