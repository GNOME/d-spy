/*
 * dspy-introspection.c
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
#include "dspy-interface.h"
#include "dspy-introspection.h"
#include "dspy-method.h"
#include "dspy-node.h"
#include "dspy-property.h"
#include "dspy-signal.h"

struct _DspyIntrospection
{
  GObject     parent_instance;
  GListStore *items;
};

static GType
dspy_introspection_get_item_type (GListModel *model)
{
  return DSPY_TYPE_INTROSPECTABLE;
}

static guint
dspy_introspection_get_n_items (GListModel *model)
{
  return g_list_model_get_n_items (G_LIST_MODEL (DSPY_INTROSPECTION (model)->items));
}

static gpointer
dspy_introspection_get_item (GListModel *model,
                             guint       position)
{
  return g_list_model_get_item (G_LIST_MODEL (DSPY_INTROSPECTION (model)->items), position);
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = dspy_introspection_get_item_type;
  iface->get_n_items = dspy_introspection_get_n_items;
  iface->get_item = dspy_introspection_get_item;
}

G_DEFINE_FINAL_TYPE_WITH_CODE (DspyIntrospection, dspy_introspection, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static void
dspy_introspection_dispose (GObject *object)
{
  DspyIntrospection *self = (DspyIntrospection *)object;

  g_clear_object (&self->items);

  G_OBJECT_CLASS (dspy_introspection_parent_class)->dispose (object);
}

static void
dspy_introspection_class_init (DspyIntrospectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dspy_introspection_dispose;
}

static void
dspy_introspection_init (DspyIntrospection *self)
{
  self->items = g_list_store_new (DSPY_TYPE_INTROSPECTABLE);
}

static int
compare_node_by_path (gconstpointer a,
                      gconstpointer b,
                      gpointer      user_data)
{
  const DspyNode *node_a = a;
  const DspyNode *node_b = b;

  return g_strcmp0 (node_a->path, node_b->path);
}

static int
compare_interface_by_name (gconstpointer a,
                           gconstpointer b,
                           gpointer      user_data)
{
  const DspyInterface *iface_a = a;
  const DspyInterface *iface_b = b;

  return g_strcmp0 (iface_a->name, iface_b->name);
}

static int
compare_property (gconstpointer a,
                  gconstpointer b,
                  gpointer      user_data)
{
  const DspyProperty *prop_a = a;
  const DspyProperty *prop_b = b;

  return g_strcmp0 (prop_a->name, prop_b->name);
}

static int
compare_method (gconstpointer a,
                gconstpointer b,
                gpointer      user_data)
{
  const DspyMethod *method_a = a;
  const DspyMethod *method_b = b;

  return g_strcmp0 (method_a->name, method_b->name);
}

static int
compare_signal (gconstpointer a,
                gconstpointer b,
                gpointer      user_data)
{
  const DspySignal *signal_a = a;
  const DspySignal *signal_b = b;

  return g_strcmp0 (signal_a->name, signal_b->name);
}

static DspyArgument *
parse_argument (GDBusArgInfo *info)
{
  DspyArgument *arg;

  arg = g_object_new (DSPY_TYPE_ARGUMENT, NULL);
  g_set_str (&arg->name, info->name);
  g_set_str (&arg->signature, info->signature);

  return arg;
}

static DspySignal *
parse_signal (GDBusSignalInfo *info)
{
  DspySignal *signal;

  signal = g_object_new (DSPY_TYPE_SIGNAL, NULL);
  g_set_str (&signal->name, info->name);

  for (guint i = 0; info->args[i]; i++)
    {
      DspyArgument *child;

      if ((child = parse_argument (info->args[i])))
        dspy_introspectable_append_queue (DSPY_INTROSPECTABLE (signal),
                                          &signal->args,
                                          DSPY_INTROSPECTABLE (child));
    }

  return signal;
}

static DspyMethod *
parse_method (GDBusMethodInfo *info)
{
  DspyMethod *method;

  method = g_object_new (DSPY_TYPE_METHOD, NULL);
  g_set_str (&method->name, info->name);

  for (guint i = 0; info->in_args[i]; i++)
    {
      DspyArgument *child;

      if ((child = parse_argument (info->in_args[i])))
        dspy_introspectable_append_queue (DSPY_INTROSPECTABLE (method),
                                          &method->in_args,
                                          DSPY_INTROSPECTABLE (child));
    }

  for (guint i = 0; info->out_args[i]; i++)
    {
      DspyArgument *child;

      if ((child = parse_argument (info->out_args[i])))
        dspy_introspectable_append_queue (DSPY_INTROSPECTABLE (method),
                                          &method->out_args,
                                          DSPY_INTROSPECTABLE (child));
    }

  return method;
}

static DspyProperty *
parse_property (GDBusPropertyInfo *info)
{
  DspyProperty *prop;

  prop = g_object_new (DSPY_TYPE_PROPERTY, NULL);
  g_set_str (&prop->name, info->name);
  g_set_str (&prop->signature, info->signature);
  prop->flags = info->flags;

  return prop;
}

static DspyInterface *
parse_interface (GDBusInterfaceInfo *info)
{
  DspyInterface *iface;

  iface = g_object_new (DSPY_TYPE_INTERFACE, NULL);
  g_set_str (&iface->name, info->name);

  for (guint i = 0; info->signals[i]; i++)
    {
      DspySignal *child;

      if ((child = parse_signal (info->signals[i])))
        dspy_introspectable_append_queue (DSPY_INTROSPECTABLE (iface),
                                          &iface->signals,
                                          DSPY_INTROSPECTABLE (child));
    }

  for (guint i = 0; info->methods[i]; i++)
    {
      DspyMethod *child;

      if ((child = parse_method (info->methods[i])))
        dspy_introspectable_append_queue (DSPY_INTROSPECTABLE (iface),
                                          &iface->methods,
                                          DSPY_INTROSPECTABLE (child));
    }

  for (guint i = 0; info->properties[i]; i++)
    {
      DspyProperty *child;

      if ((child = parse_property (info->properties[i])))
        dspy_introspectable_append_queue (DSPY_INTROSPECTABLE (iface),
                                          &iface->properties,
                                          DSPY_INTROSPECTABLE (child));
    }

  g_queue_sort (&iface->signals, compare_signal, NULL);
  g_queue_sort (&iface->methods, compare_method, NULL);
  g_queue_sort (&iface->properties, compare_property, NULL);

  return iface;
}

static DspyNode *
parse_node (GDBusNodeInfo *info)
{
  DspyNode *node;

  node = g_object_new (DSPY_TYPE_NODE, NULL);
  g_set_str (&node->path, info->path);

  for (guint i = 0; info->nodes[i]; i++)
    {
      DspyNode *child;

      if ((child = parse_node (info->nodes[i])))
        dspy_introspectable_append_queue (DSPY_INTROSPECTABLE (node),
                                          &node->nodes,
                                          DSPY_INTROSPECTABLE (child));
    }

  for (guint i = 0; info->interfaces[i]; i++)
    {
      DspyInterface *child;

      if ((child = parse_interface (info->interfaces[i])))
        dspy_introspectable_append_queue (DSPY_INTROSPECTABLE (node),
                                          &node->interfaces,
                                          DSPY_INTROSPECTABLE (child));
    }

  g_queue_sort (&node->nodes, compare_node_by_path, NULL);
  g_queue_sort (&node->interfaces, compare_interface_by_name, NULL);

  return node;
}

typedef struct
{
  DspyIntrospection *self;
  GDBusConnection   *connection;
  char              *owner;
  char              *path;
} Load;

static void
load_free (Load *state)
{
  g_clear_object (&state->self);
  g_clear_object (&state->connection);
  g_clear_pointer (&state->path, g_free);
  g_clear_pointer (&state->owner, g_free);
  g_free (state);
}

static DexFuture *
dspy_introspection_load_fiber (gpointer data)
{
  Load *state = data;
  g_autoptr(GPtrArray) paths = NULL;

  g_assert (state != NULL);
  g_assert (G_IS_DBUS_CONNECTION (state->connection));
  g_assert (state->owner != NULL);
  g_assert (state->path != NULL);
  g_assert (DSPY_IS_INTROSPECTION (state->self));

  paths = g_ptr_array_new_with_free_func (g_free);
  g_ptr_array_add (paths, g_strdup ("/"));

  for (guint pos = 0; pos < paths->len; pos++)
    {
      const char *path = g_ptr_array_index (paths, pos);
      g_autoptr(GVariant) reply = NULL;
      g_autoptr(GError) error = NULL;
      const char *xml;

      if (!(reply = dex_await_variant (dex_dbus_connection_call (state->connection,
                                                                 state->owner,
                                                                 path,
                                                                 "org.freedesktop.DBus.Introspectable",
                                                                 "Introspect",
                                                                 NULL,
                                                                 G_VARIANT_TYPE ("(s)"),
                                                                 G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
                                                                 -1),
                                       &error)))
        return dex_future_new_for_error (g_steal_pointer (&error));

      g_variant_get (reply, "(&s)", &xml);

      g_assert (xml != NULL);

      if (xml[0] != 0)
        {
          g_autoptr(GDBusNodeInfo) info = NULL;
          g_autoptr(DspyNode) node = NULL;

          if (!(info = g_dbus_node_info_new_for_xml (xml, &error)))
            return dex_future_new_for_error (g_steal_pointer (&error));

          node = parse_node (info);

          {
            g_autofree char *abs_path = g_build_path ("/", path, node->path, NULL);
            g_set_str (&node->path, abs_path);
          }

          /* If there are children nodes, we need to parse them too */
          for (const GList *iter = node->nodes.head; iter; iter = iter->next)
            {
              DspyNode *child = iter->data;
              g_autofree gchar *child_path = NULL;

              g_assert (child != NULL);
              g_assert (DSPY_IS_NODE (child));

              g_ptr_array_add (paths, g_build_path ("/", node->path, child->path, NULL));
            }

          /* Now add to the list of objects but only if there were interfaces
           * provided (thus not an intermediate node).
           */
          if (node->interfaces.length > 0)
            g_list_store_append (state->self->items, node);
        }
    }

  return dex_future_new_take_object (g_steal_pointer (&state->self));
}

DexFuture *
dspy_introspection_new (GDBusConnection *connection,
                        const char      *owner,
                        const char      *path)
{
  Load *state;

  dex_return_error_if_fail (G_IS_DBUS_CONNECTION (connection));
  dex_return_error_if_fail (path != NULL);

  state = g_new0 (Load, 1);
  state->connection = g_object_ref (connection);
  state->owner = g_strdup (owner);
  state->path = g_strdup (path);
  state->self = g_object_new (DSPY_TYPE_INTROSPECTION, NULL);

  return dex_scheduler_spawn (dex_thread_pool_scheduler_get_default (), 0,
                              dspy_introspection_load_fiber,
                              state,
                              (GDestroyNotify) load_free);
}
