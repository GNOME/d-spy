/* dspy-method-view.c
 *
 * Copyright 2019 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#define G_LOG_DOMAIN "dspy-method-view"

#include "config.h"

#include <dazzle.h>

#include "dspy-method-view.h"

typedef struct
{
  DspyMethodInvocation *invocation;
  DzlBindingGroup *bindings;

  GtkLabel *label_interface;
  GtkLabel *label_object_path;
  GtkLabel *label_method;
} DspyMethodViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DspyMethodView, dspy_method_view, GTK_TYPE_BIN)

enum {
  PROP_0,
  PROP_INVOCATION,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

/**
 * dspy_method_view_new:
 *
 * Create a new #DspyMethodView.
 *
 * Returns: (transfer full): a newly created #DspyMethodView
 */
GtkWidget *
dspy_method_view_new (void)
{
  return g_object_new (DSPY_TYPE_METHOD_VIEW, NULL);
}

static void
dspy_method_view_finalize (GObject *object)
{
  DspyMethodView *self = (DspyMethodView *)object;
  DspyMethodViewPrivate *priv = dspy_method_view_get_instance_private (self);

  dzl_binding_group_set_source (priv->bindings, NULL);
  g_clear_object (&priv->bindings);
  g_clear_object (&priv->invocation);

  G_OBJECT_CLASS (dspy_method_view_parent_class)->finalize (object);
}

static void
dspy_method_view_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  DspyMethodView *self = DSPY_METHOD_VIEW (object);

  switch (prop_id)
    {
    case PROP_INVOCATION:
      g_value_set_object (value, dspy_method_view_get_invocation (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_method_view_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  DspyMethodView *self = DSPY_METHOD_VIEW (object);

  switch (prop_id)
    {
    case PROP_INVOCATION:
      dspy_method_view_set_invocation (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_method_view_class_init (DspyMethodViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = dspy_method_view_finalize;
  object_class->get_property = dspy_method_view_get_property;
  object_class->set_property = dspy_method_view_set_property;

  properties [PROP_INVOCATION] =
    g_param_spec_object ("invocation",
                         "Invocation",
                         "The method invocation to view",
                         DSPY_TYPE_METHOD_INVOCATION,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-method-view.ui");
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, label_interface);
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, label_method);
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, label_object_path);
}

static void
dspy_method_view_init (DspyMethodView *self)
{
  DspyMethodViewPrivate *priv = dspy_method_view_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->bindings = dzl_binding_group_new ();
  dzl_binding_group_bind (priv->bindings, "interface", priv->label_interface, "label", 0);
  dzl_binding_group_bind (priv->bindings, "method", priv->label_method, "label", 0);
  dzl_binding_group_bind (priv->bindings, "object-path", priv->label_object_path, "label", 0);
}

void
dspy_method_view_set_invocation (DspyMethodView       *self,
                                 DspyMethodInvocation *invocation)
{
  DspyMethodViewPrivate *priv = dspy_method_view_get_instance_private (self);

  g_return_if_fail (DSPY_IS_METHOD_VIEW (self));
  g_return_if_fail (!invocation || DSPY_IS_METHOD_INVOCATION (invocation));

  if (g_set_object (&priv->invocation, invocation))
    {
      dzl_binding_group_set_source (priv->bindings, invocation);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_INVOCATION]);
    }
}

/**
 * dspy_method_view_get_invocation:
 *
 * Returns: (transfer none) (nullable): a #DspyMethodInvocation or %NULL
 */
DspyMethodInvocation *
dspy_method_view_get_invocation (DspyMethodView *self)
{
  DspyMethodViewPrivate *priv = dspy_method_view_get_instance_private (self);

  g_return_val_if_fail (DSPY_IS_METHOD_VIEW (self), NULL);

  return priv->invocation;
}
