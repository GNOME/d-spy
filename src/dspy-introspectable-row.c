/*
 * dspy-introspectable-row.c
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

#include "dspy-interface.h"
#include "dspy-introspectable-row.h"
#include "dspy-method.h"
#include "dspy-node.h"
#include "dspy-property.h"
#include "dspy-signal.h"
#include "dspy-titled-model.h"

struct _DspyIntrospectableRow
{
  GtkWidget      parent_instance;

  GObject       *item;
  GBindingGroup *bindings;

  GtkLabel      *title;
};

enum {
  PROP_0,
  PROP_ITEM,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DspyIntrospectableRow, dspy_introspectable_row, GTK_TYPE_WIDGET)

static GParamSpec *properties[N_PROPS];

static void
dspy_introspectable_row_dispose (GObject *object)
{
  DspyIntrospectableRow *self = (DspyIntrospectableRow *)object;
  GtkWidget *child;

  gtk_widget_dispose_template (GTK_WIDGET (self), DSPY_TYPE_INTROSPECTABLE_ROW);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  g_clear_object (&self->bindings);
  g_clear_object (&self->item);

  G_OBJECT_CLASS (dspy_introspectable_row_parent_class)->dispose (object);
}

static void
dspy_introspectable_row_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  DspyIntrospectableRow *self = DSPY_INTROSPECTABLE_ROW (object);

  switch (prop_id)
    {
    case PROP_ITEM:
      g_value_set_object (value, dspy_introspectable_row_get_item (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_introspectable_row_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  DspyIntrospectableRow *self = DSPY_INTROSPECTABLE_ROW (object);

  switch (prop_id)
    {
    case PROP_ITEM:
      dspy_introspectable_row_set_item (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_introspectable_row_class_init (DspyIntrospectableRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dspy_introspectable_row_dispose;
  object_class->get_property = dspy_introspectable_row_get_property;
  object_class->set_property = dspy_introspectable_row_set_property;

  properties[PROP_ITEM] =
    g_param_spec_object ("item", NULL, NULL,
                         G_TYPE_OBJECT,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-introspectable-row.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_bind_template_child (widget_class, DspyIntrospectableRow, title);
}

static void
dspy_introspectable_row_init (DspyIntrospectableRow *self)
{
  self->bindings = g_binding_group_new ();

  gtk_widget_init_template (GTK_WIDGET (self));

  g_binding_group_bind (self->bindings, "title",
                        self->title, "label",
                        G_BINDING_SYNC_CREATE);
}

/**
 * dspy_introspectable_row_get_Item:
 * @self: a [class@Dspy.IntrospectableRow]
 *
 * Returns: (transfer none) (nullable):
 */
GObject *
dspy_introspectable_row_get_item (DspyIntrospectableRow *self)
{
  g_return_val_if_fail (DSPY_IS_INTROSPECTABLE_ROW (self), NULL);

  return self->item;
}

void
dspy_introspectable_row_set_item (DspyIntrospectableRow *self,
                                  GObject               *item)
{
  g_return_if_fail (DSPY_IS_INTROSPECTABLE_ROW (self));

  if (g_set_object (&self->item, item))
    {
      g_binding_group_set_source (self->bindings, item);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ITEM]);
    }
}
