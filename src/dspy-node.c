/*
 * dspy-node.c
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

#include "dspy-node.h"

enum {
  PROP_0,
  PROP_INTERFACES,
  PROP_NODES,
  PROP_PATH,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DspyNode, dspy_node, DSPY_TYPE_INTROSPECTABLE)

static GParamSpec *properties[N_PROPS];

static char *
dspy_node_dup_title (DspyIntrospectable *introspectable)
{
  return g_strdup (DSPY_NODE (introspectable)->path);
}

static void
dspy_node_dispose (GObject *object)
{
  DspyNode *self = (DspyNode *)object;

  dspy_introspectable_clear_queue (DSPY_INTROSPECTABLE (self), &self->nodes);
  dspy_introspectable_clear_queue (DSPY_INTROSPECTABLE (self), &self->interfaces);
  g_clear_pointer (&self->path, g_free);

  G_OBJECT_CLASS (dspy_node_parent_class)->dispose (object);
}

static void
dspy_node_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  DspyNode *self = DSPY_NODE (object);

  switch (prop_id)
    {
    case PROP_PATH:
      g_value_set_string (value, self->path);
      break;

    case PROP_NODES:
      g_value_take_object (value, dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->nodes));
      break;

    case PROP_INTERFACES:
      g_value_take_object (value, dspy_node_list_interfaces (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_node_class_init (DspyNodeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DspyIntrospectableClass *introspectable_class = DSPY_INTROSPECTABLE_CLASS (klass);

  object_class->dispose = dspy_node_dispose;
  object_class->get_property = dspy_node_get_property;

  introspectable_class->dup_title = dspy_node_dup_title;

  properties[PROP_PATH] =
    g_param_spec_string ("path", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_INTERFACES] =
    g_param_spec_object ("interfaces", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_NODES] =
    g_param_spec_object ("nodes", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_node_init (DspyNode *self)
{
}

/**
 * dspy_node_list_interfaces:
 * @self: a [class@Dspy.Node]
 *
 * Returns: (transfer full):
 */
GListModel *
dspy_node_list_interfaces (DspyNode *self)
{
  g_return_val_if_fail (DSPY_IS_NODE (self), NULL);

  return dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (self), &self->interfaces);
}
