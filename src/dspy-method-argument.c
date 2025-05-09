/*
 * dspy-method-argument.c
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

#include "dspy-method-argument.h"

struct _DspyMethodArgument
{
  GObject parent_instance;
  char *signature;
  char *name;
  GVariant *value;
  guint has_error : 1;
};

enum {
  PROP_0,
  PROP_SIGNATURE,
  PROP_NAME,
  PROP_VALUE,
  PROP_VALUE_TEXT,
  PROP_HAS_ERROR,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DspyMethodArgument, dspy_method_argument, G_TYPE_OBJECT)

static GParamSpec *properties[N_PROPS];

static void
dspy_method_argument_finalize (GObject *object)
{
  DspyMethodArgument *self = (DspyMethodArgument *)object;

  g_clear_pointer (&self->signature, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->value, g_variant_unref);

  G_OBJECT_CLASS (dspy_method_argument_parent_class)->finalize (object);
}

static void
dspy_method_argument_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  DspyMethodArgument *self = DSPY_METHOD_ARGUMENT (object);

  switch (prop_id)
    {
    case PROP_SIGNATURE:
      g_value_set_string (value, dspy_method_argument_get_signature (self));
      break;

    case PROP_NAME:
      g_value_set_string (value, dspy_method_argument_get_name (self));
      break;

    case PROP_VALUE:
      g_value_take_variant (value, dspy_method_argument_dup_value (self));
      break;

    case PROP_VALUE_TEXT:
      g_value_take_string (value, dspy_method_argument_dup_value_text (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_method_argument_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  DspyMethodArgument *self = DSPY_METHOD_ARGUMENT (object);

  switch (prop_id)
    {
    case PROP_SIGNATURE:
      self->signature = g_value_dup_string (value);
      break;

    case PROP_NAME:
      self->name = g_value_dup_string (value);
      break;

    case PROP_VALUE:
      dspy_method_argument_set_value (self, g_value_get_variant (value));
      break;

    case PROP_VALUE_TEXT:
      dspy_method_argument_set_value_text (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_method_argument_class_init (DspyMethodArgumentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dspy_method_argument_finalize;
  object_class->get_property = dspy_method_argument_get_property;
  object_class->set_property = dspy_method_argument_set_property;

  properties[PROP_SIGNATURE] =
    g_param_spec_string ("signature", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_HAS_ERROR] =
    g_param_spec_boolean ("has-error", NULL, NULL,
                          FALSE,
                          (G_PARAM_READABLE |
                           G_PARAM_STATIC_STRINGS));

  properties[PROP_VALUE] =
    g_param_spec_variant ("value", NULL, NULL,
                          G_VARIANT_TYPE_ANY,
                          NULL,
                          (G_PARAM_READWRITE |
                           G_PARAM_EXPLICIT_NOTIFY |
                           G_PARAM_STATIC_STRINGS));

  properties[PROP_VALUE_TEXT] =
    g_param_spec_string ("value-text", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_method_argument_init (DspyMethodArgument *self)
{
}

DspyMethodArgument *
dspy_method_argument_new (const char *signature,
                          const char *name,
                          GVariant   *value)
{
  return g_object_new (DSPY_TYPE_METHOD_ARGUMENT,
                       "signature", signature,
                       "name", name,
                       "value", value,
                       NULL);
}

const char *
dspy_method_argument_get_name (DspyMethodArgument *self)
{
  g_return_val_if_fail (DSPY_IS_METHOD_ARGUMENT (self), NULL);

  return self->name;
}

const char *
dspy_method_argument_get_signature (DspyMethodArgument *self)
{
  g_return_val_if_fail (DSPY_IS_METHOD_ARGUMENT (self), NULL);

  return self->signature;
}

char *
dspy_method_argument_dup_value_text (DspyMethodArgument *self)
{
  g_return_val_if_fail (DSPY_IS_METHOD_ARGUMENT (self), NULL);

  if (self->value != NULL)
    return g_variant_print (self->value, TRUE);

  return NULL;
}

GVariant *
dspy_method_argument_dup_value (DspyMethodArgument *self)
{
  g_return_val_if_fail (DSPY_IS_METHOD_ARGUMENT (self), NULL);

  if (self->value != NULL)
    return g_variant_ref (self->value);

  return NULL;
}

gboolean
dspy_method_argument_has_error (DspyMethodArgument *self)
{
  g_return_val_if_fail (DSPY_IS_METHOD_ARGUMENT (self), FALSE);

  return self->has_error;
}

void
dspy_method_argument_set_value (DspyMethodArgument *self,
                                GVariant           *value)
{
  g_return_if_fail (DSPY_IS_METHOD_ARGUMENT (self));

  if (self->value == value)
    return;

  if (value != NULL)
    g_variant_ref (value);
  g_clear_pointer (&self->value, g_variant_unref);
  self->value = value;

  if (self->has_error)
    {
      self->has_error = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HAS_ERROR]);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE_TEXT]);
}

void
dspy_method_argument_set_value_text (DspyMethodArgument *self,
                                     const char         *value_text)
{
  g_autoptr(GVariant) value = NULL;
  g_autoptr(GError) error = NULL;

  g_return_if_fail (DSPY_IS_METHOD_ARGUMENT (self));

  if (!value_text || value_text[0] == 0)
    {
      dspy_method_argument_set_value (self, NULL);
      return;
    }

  value = g_variant_parse (G_VARIANT_TYPE (self->signature), value_text, NULL, NULL, &error);

  dspy_method_argument_set_value (self, value);

  if (error != NULL)
    {
      self->has_error = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HAS_ERROR]);
    }
}
