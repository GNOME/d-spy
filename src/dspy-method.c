/*
 * dspy-method.c
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
#include "dspy-method.h"
#include "dspy-private.h"

enum {
  PROP_0,
  PROP_NAME,
  PROP_IN_ARGS,
  PROP_IN_SIGNATURE,
  PROP_OUT_ARGS,
  PROP_OUT_SIGNATURE,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DspyMethod, dspy_method, DSPY_TYPE_INTROSPECTABLE)

static GParamSpec *properties[N_PROPS];

char *
dspy_method_dup_in_signature (DspyMethod *self)
{
  GString *str = g_string_new ("(");

  for (const GList *iter = self->in_args.head; iter; iter = iter->next)
    {
      DspyArgument *arg = iter->data;
      g_string_append (str, arg->signature);
    }

  g_string_append (str, ")");

  return g_string_free (str, FALSE);
}

char *
dspy_method_dup_out_signature (DspyMethod *self)
{
  GString *str = g_string_new ("(");

  for (const GList *iter = self->out_args.head; iter; iter = iter->next)
    {
      DspyArgument *arg = iter->data;
      g_string_append (str, arg->signature);
    }

  g_string_append (str, ")");

  return g_string_free (str, FALSE);
}

static char *
dspy_method_dup_short_title (DspyIntrospectable *introspectable)
{
  return g_strdup (DSPY_METHOD (introspectable)->name);
}

static char *
dspy_method_dup_title (DspyIntrospectable *introspectable)
{
  DspyMethod *self = DSPY_METHOD (introspectable);
  GString *str = g_string_new (self->name);

  g_string_append (str, " (");

  for (const GList *iter = self->in_args.head; iter; iter = iter->next)
    {
      DspyArgument *arg = iter->data;
      g_autofree char *sig = _dspy_signature_humanize (arg->signature);

      if (iter->prev != NULL)
        g_string_append (str, ", ");
      g_string_append_printf (str, "<b>%s</b>", sig);
      if (!dspy_argument_name_is_generated (arg))
        g_string_append_printf (str, " %s", arg->name);
    }

  g_string_append (str, ") -> (");

  for (const GList *iter = self->out_args.head; iter; iter = iter->next)
    {
      DspyArgument *arg = iter->data;
      g_autofree char *sig = _dspy_signature_humanize (arg->signature);

      if (iter->prev != NULL)
        g_string_append (str, ", ");
      g_string_append_printf (str, "<b>%s</b>", sig);
      if (!dspy_argument_name_is_generated (arg))
        g_string_append_printf (str, " %s", arg->name);
    }

  g_string_append_c (str, ')');

  return g_string_free (str, FALSE);
}

static void
dspy_method_dispose (GObject *object)
{
  DspyMethod *self = (DspyMethod *)object;

  dspy_introspectable_clear_queue (DSPY_INTROSPECTABLE (self), &self->in_args);
  dspy_introspectable_clear_queue (DSPY_INTROSPECTABLE (self), &self->out_args);
  g_clear_pointer (&self->name, g_free);

  G_OBJECT_CLASS (dspy_method_parent_class)->dispose (object);
}

static void
dspy_method_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  DspyMethod *self = DSPY_METHOD (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_IN_ARGS:
      g_value_take_object (value, dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->in_args));
      break;

    case PROP_IN_SIGNATURE:
      g_value_take_string (value, dspy_method_dup_in_signature (self));
      break;

    case PROP_OUT_ARGS:
      g_value_take_object (value, dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->out_args));
      break;

    case PROP_OUT_SIGNATURE:
      g_value_take_string (value, dspy_method_dup_out_signature (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_method_class_init (DspyMethodClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DspyIntrospectableClass *introspectable_class = DSPY_INTROSPECTABLE_CLASS (klass);

  object_class->dispose = dspy_method_dispose;
  object_class->get_property = dspy_method_get_property;

  introspectable_class->dup_title = dspy_method_dup_title;
  introspectable_class->dup_short_title = dspy_method_dup_short_title;

  properties[PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_IN_ARGS] =
    g_param_spec_object ("in-args", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_IN_SIGNATURE] =
    g_param_spec_string ("in-signature", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_OUT_ARGS] =
    g_param_spec_object ("out-args", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_OUT_SIGNATURE] =
    g_param_spec_string ("out-signature", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_method_init (DspyMethod *self)
{
}
