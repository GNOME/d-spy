/*
 * dspy-argument.c
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

#include "dspy-argument.h"

enum {
  PROP_0,
  PROP_NAME,
  PROP_SIGNATURE,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DspyArgument, dspy_argument, DSPY_TYPE_INTROSPECTABLE)

static GParamSpec *properties[N_PROPS];

static void
dspy_argument_dispose (GObject *object)
{
  DspyArgument *self = (DspyArgument *)object;

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->signature, g_free);

  G_OBJECT_CLASS (dspy_argument_parent_class)->dispose (object);
}

static void
dspy_argument_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  DspyArgument *self = DSPY_ARGUMENT (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_SIGNATURE:
      g_value_set_string (value, self->signature);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_argument_class_init (DspyArgumentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dspy_argument_dispose;
  object_class->get_property = dspy_argument_get_property;

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

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_argument_init (DspyArgument *self)
{
}

gboolean
dspy_argument_name_is_generated (DspyArgument *self)
{
  const char *str;

  g_return_val_if_fail (DSPY_IS_ARGUMENT (self), FALSE);

  str = self->name;

  if (str == NULL)
    return TRUE;

  if (g_str_has_prefix (str, "arg_"))
    {
      gchar *endptr = NULL;
      gint64 val;

      str += strlen ("arg_");
      errno = 0;
      val = g_ascii_strtoll (str, &endptr, 10);

      if (val >= 0 && errno == 0 && *endptr == 0)
        return TRUE;
    }

  return FALSE;
}
