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
#include <glib/gi18n.h>

#include "dspy-method-view.h"

typedef struct
{
  DspyMethodInvocation *invocation;
  DzlBindingGroup      *bindings;
  GCancellable         *cancellable;

  GtkLabel             *label_interface;
  GtkLabel             *label_object_path;
  GtkLabel             *label_method;
  GtkButton            *button;
  GtkTextBuffer        *buffer_params;
  GtkTextBuffer        *buffer_reply;
  GtkTextView          *textview_params;

  guint                 busy : 1;
} DspyMethodViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DspyMethodView, dspy_method_view, DZL_TYPE_BIN)

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

static gboolean
variant_to_string_transform (GBinding     *binding,
                             const GValue *from_value,
                             GValue       *to_value,
                             gpointer      user_data)
{
  GVariant *v = g_value_get_variant (from_value);
  if (v != NULL)
    g_value_take_string (to_value, g_variant_print (v, FALSE));
  else
    g_value_set_string (to_value, "");
  return TRUE;
}

static void
dspy_method_view_execute_cb (GObject      *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  DspyMethodInvocation *invocation = (DspyMethodInvocation *)object;
  g_autoptr(DspyMethodView) self = user_data;
  DspyMethodViewPrivate *priv = dspy_method_view_get_instance_private (self);
  g_autoptr(GVariant) reply = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (DSPY_IS_METHOD_INVOCATION (invocation));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (DSPY_IS_METHOD_VIEW (self));

  priv->busy = FALSE;

  if (!(reply = dspy_method_invocation_execute_finish (invocation, result, &error)))
    {
      if (priv->invocation == invocation)
        gtk_text_buffer_set_text (priv->buffer_reply, error->message, -1);
    }
  else
    {
      if (priv->invocation == invocation)
        {
          g_autofree gchar *replystr = g_variant_print (reply, TRUE);
          gtk_text_buffer_set_text (priv->buffer_reply, replystr, -1);
        }
    }

  gtk_button_set_label (priv->button, _("Execute"));
}

static void
dspy_method_view_button_clicked_cb (DspyMethodView *self,
                                    GtkButton      *button)
{
  DspyMethodViewPrivate *priv = dspy_method_view_get_instance_private (self);

  g_assert (DSPY_IS_METHOD_VIEW (self));
  g_assert (GTK_IS_BUTTON (button));

  /* Always cancel anything in flight */
  g_cancellable_cancel (priv->cancellable);
  g_clear_object (&priv->cancellable);

  if (priv->busy)
    return;

  if (priv->invocation == NULL)
    return;

  g_assert (priv->busy == FALSE);
  g_assert (priv->cancellable == NULL);

  priv->busy = TRUE;
  priv->cancellable = g_cancellable_new ();

  gtk_text_buffer_set_text (priv->buffer_reply, "", -1);

  dspy_method_invocation_execute_async (priv->invocation,
                                        priv->cancellable,
                                        dspy_method_view_execute_cb,
                                        g_object_ref (self));

  gtk_button_set_label (priv->button, _("Cancel"));
}

static void
dspy_method_view_invoke_method (GtkWidget *widget,
                                gpointer   user_data)
{
  DspyMethodView *self = user_data;
  DspyMethodViewPrivate *priv = dspy_method_view_get_instance_private (self);

  g_assert (DSPY_IS_METHOD_VIEW (self));

  gtk_widget_activate (GTK_WIDGET (priv->button));
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
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, buffer_params);
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, buffer_reply);
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, label_interface);
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, label_method);
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, label_object_path);
  gtk_widget_class_bind_template_child_private (widget_class, DspyMethodView, textview_params);
}

static void
dspy_method_view_init (DspyMethodView *self)
{
  DspyMethodViewPrivate *priv = dspy_method_view_get_instance_private (self);
  DzlShortcutController *controller;

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->bindings = dzl_binding_group_new ();
  dzl_binding_group_bind (priv->bindings, "interface", priv->label_interface, "label", 0);
  dzl_binding_group_bind (priv->bindings, "method", priv->label_method, "label", 0);
  dzl_binding_group_bind (priv->bindings, "object-path", priv->label_object_path, "label", 0);
  dzl_binding_group_bind_full (priv->bindings, "parameters", priv->buffer_params, "text", 0,
                               variant_to_string_transform, NULL, NULL, NULL);

  g_signal_connect_object (priv->button,
                           "clicked",
                           G_CALLBACK (dspy_method_view_button_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  controller = dzl_shortcut_controller_find (GTK_WIDGET (priv->textview_params));

  dzl_shortcut_controller_add_command_callback (controller,
                                                "org.gnome.dspy.invoke-method",
                                                "<Primary>Return",
                                                DZL_SHORTCUT_PHASE_DISPATCH,
                                                dspy_method_view_invoke_method,
                                                self,
                                                NULL);
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
      g_cancellable_cancel (priv->cancellable);
      g_clear_object (&priv->cancellable);

      dzl_binding_group_set_source (priv->bindings, invocation);
      gtk_text_buffer_set_text (priv->buffer_reply, "", -1);

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
