/*
 * dspy-property.c
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

#include "dspy-property.h"

enum {
  PROP_0,
  PROP_NAME,
  PROP_SIGNATURE,
  PROP_VALUE,
  PROP_FLAGS,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DspyProperty, dspy_property, DSPY_TYPE_INTROSPECTABLE)

static GParamSpec *properties[N_PROPS];

static void
dspy_property_dispose (GObject *object)
{
  DspyProperty *self = (DspyProperty *)object;

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->signature, g_free);
  g_clear_pointer (&self->value, g_free);

  G_OBJECT_CLASS (dspy_property_parent_class)->dispose (object);
}

static void
dspy_property_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  DspyProperty *self = DSPY_PROPERTY (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_SIGNATURE:
      g_value_set_string (value, self->signature);
      break;

    case PROP_VALUE:
      g_value_set_string (value, self->value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_property_class_init (DspyPropertyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dspy_property_dispose;
  object_class->get_property = dspy_property_get_property;

  properties[PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_SIGNATURE] =
    g_param_spec_string ("signature", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_VALUE] =
    g_param_spec_string ("value", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_FLAGS] =
    g_param_spec_flags ("flags", NULL, NULL,
                         G_TYPE_DBUS_PROPERTY_INFO_FLAGS,
                         0,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_property_init (DspyProperty *self)
{
}
