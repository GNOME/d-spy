/*
 * dspy-introspectable.c
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

#include "dspy-introspectable.h"

enum {
  PROP_0,
  PROP_PARENT,
  PROP_SHORT_TITLE,
  PROP_TITLE,
  N_PROPS
};

G_DEFINE_ABSTRACT_TYPE (DspyIntrospectable, dspy_introspectable, G_TYPE_OBJECT)

static GParamSpec *properties[N_PROPS];

/**
 * dspy_introspectable_get_parent:
 * @self: a [class@Dspy.Introspectable]
 *
 * Returns: (transfer none) (nullable):
 */
DspyIntrospectable *
dspy_introspectable_get_parent (DspyIntrospectable *self)
{
  g_return_val_if_fail (DSPY_IS_INTROSPECTABLE (self), NULL);

  return self->parent;
}

static void
dspy_introspectable_dispose (GObject *object)
{
  DspyIntrospectable *self = (DspyIntrospectable *)object;

  g_warn_if_fail (self->parent == NULL);
  g_warn_if_fail (self->link.prev == NULL);
  g_warn_if_fail (self->link.next == NULL);

  G_OBJECT_CLASS (dspy_introspectable_parent_class)->dispose (object);
}

static void
dspy_introspectable_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  DspyIntrospectable *self = DSPY_INTROSPECTABLE (object);

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, self->parent);
      break;

    case PROP_SHORT_TITLE:
      g_value_take_string (value, dspy_introspectable_dup_short_title (self));
      break;

    case PROP_TITLE:
      g_value_take_string (value, dspy_introspectable_dup_title (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_introspectable_class_init (DspyIntrospectableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dspy_introspectable_dispose;
  object_class->get_property = dspy_introspectable_get_property;

  properties[PROP_PARENT] =
    g_param_spec_object ("parent", NULL, NULL,
                         DSPY_TYPE_INTROSPECTABLE,
                         (G_PARAM_READABLE  |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_SHORT_TITLE] =
    g_param_spec_string ("short-title", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_introspectable_init (DspyIntrospectable *self)
{
  self->link.data = self;
}

char *
dspy_introspectable_dup_title (DspyIntrospectable *self)
{
  g_return_val_if_fail (DSPY_IS_INTROSPECTABLE (self), NULL);

  if (DSPY_INTROSPECTABLE_GET_CLASS (self)->dup_title)
    return DSPY_INTROSPECTABLE_GET_CLASS (self)->dup_title (self);

  return NULL;
}

char *
dspy_introspectable_dup_short_title (DspyIntrospectable *self)
{
  g_return_val_if_fail (DSPY_IS_INTROSPECTABLE (self), NULL);

  if (DSPY_INTROSPECTABLE_GET_CLASS (self)->dup_short_title)
    return DSPY_INTROSPECTABLE_GET_CLASS (self)->dup_short_title (self);

  return dspy_introspectable_dup_title (self);
}
