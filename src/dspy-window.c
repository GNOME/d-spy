/* dspy-window.c
 *
 * Copyright 2019 Christian Hergert
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
 */

#include "config.h"

#include <dazzle.h>
#include <dspy.h>

#include "dspy-window.h"

struct _DspyWindow
{
  GtkApplicationWindow   parent_instance;

  GCancellable          *cancellable;

  /* Template widgets */
  GtkHeaderBar          *header_bar;
  GtkTreeView           *introspection_tree_view;
  GtkListBox            *names_list_box;
  GtkButton             *refresh_button;
  DspyNameMarquee       *name_marquee;
  GtkScrolledWindow     *names_scroller;
  DspyMethodView        *method_view;
  GtkRevealer           *method_revealer;
  GtkRadioButton        *session_button;
  GtkRadioButton        *system_button;
};

G_DEFINE_TYPE (DspyWindow, dspy_window, GTK_TYPE_APPLICATION_WINDOW)

static void
dspy_window_class_init (DspyWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-window.ui");
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, introspection_tree_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, method_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, method_revealer);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, name_marquee);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_list_box);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_scroller);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, refresh_button);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, session_button);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, system_button);

  g_type_ensure (DSPY_TYPE_METHOD_VIEW);
  g_type_ensure (DSPY_TYPE_NAME_MARQUEE);
  g_type_ensure (DSPY_TYPE_TREE_VIEW);
}

static GtkWidget *
create_name_row_cb (gpointer item,
                    gpointer user_data)
{
  DspyName *name = item;

  g_assert (DSPY_IS_NAME (name));
  g_assert (user_data == NULL);

  return dspy_name_row_new (name);
}

static void
dspy_window_list_names_cb (GObject      *object,
                           GAsyncResult *result,
                           gpointer      user_data)
{
  DspyConnection *conn = (DspyConnection *)object;
  g_autoptr(DspyWindow) self = user_data;
  g_autoptr(GListModel) model = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (DSPY_IS_CONNECTION (conn));

  model = dspy_connection_list_names_finish (conn, result, &error);

  if (error != NULL)
    g_warning ("Failed to list names: %s", error->message);

  gtk_list_box_bind_model (self->names_list_box,
                           model,
                           create_name_row_cb,
                           NULL,
                           NULL);

  gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (self->names_scroller), 0.0);
}

static void
dspy_window_introspect_cb (GObject      *object,
                           GAsyncResult *result,
                           gpointer      user_data)
{
  DspyName *name = (DspyName *)object;
  g_autoptr(GtkTreeModel) model = NULL;
  g_autoptr(DspyWindow) self = user_data;
  g_autoptr(GError) error = NULL;

  g_assert (DSPY_IS_NAME (name));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (DSPY_IS_WINDOW (self));

  if (!(model = dspy_name_introspect_finish (name, result, &error)))
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning ("Failed to introspect peer: %s", error->message);
      return;
    }

  gtk_tree_view_set_model (self->introspection_tree_view, model);
}

static void
name_row_activated_cb (DspyWindow  *self,
                       DspyNameRow *row,
                       GtkListBox  *list_box)
{
  DspyName *name;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (DSPY_IS_NAME_ROW (row));
  g_assert (GTK_IS_LIST_BOX (list_box));

  name = dspy_name_row_get_name (row);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  self->cancellable = g_cancellable_new ();

  gtk_tree_view_set_model (self->introspection_tree_view, NULL);
  dspy_name_marquee_set_name (self->name_marquee, name);

  gtk_revealer_set_reveal_child (self->method_revealer, FALSE);

  dspy_name_introspect_async (name,
                              self->cancellable,
                              dspy_window_introspect_cb,
                              g_object_ref (self));
}

static void
refresh_button_clicked_cb (DspyWindow *self,
                           GtkButton  *button)
{
  GtkListBoxRow *row;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_BUTTON (button));

  if ((row = gtk_list_box_get_selected_row (self->names_list_box)))
    name_row_activated_cb (self, DSPY_NAME_ROW (row), self->names_list_box);
}

static void
method_activated_cb (DspyWindow           *self,
                     DspyMethodInvocation *invocation,
                     DspyTreeView         *tree_view)
{
  g_assert (DSPY_IS_WINDOW (self));
  g_assert (!invocation || DSPY_IS_METHOD_INVOCATION (invocation));
  g_assert (DSPY_IS_TREE_VIEW (tree_view));

  if (DSPY_IS_METHOD_INVOCATION (invocation))
    {
      dspy_method_view_set_invocation (self->method_view, invocation);
      gtk_revealer_set_reveal_child (self->method_revealer, TRUE);
    }
}

static void
notify_child_revealed_cb (DspyWindow  *self,
                          GParamSpec  *pspec,
                          GtkRevealer *revealer)
{
  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_REVEALER (revealer));

  if (!gtk_revealer_get_child_revealed (revealer))
    dspy_method_view_set_invocation (self->method_view, NULL);
  else
    {
      GtkTreeSelection *selection;
      GtkTreeModel *model = NULL;
      GtkTreeIter iter;

      selection = gtk_tree_view_get_selection (self->introspection_tree_view);

      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
          g_autoptr(GtkTreePath) path = gtk_tree_model_get_path (model, &iter);
          GtkTreeViewColumn *column = gtk_tree_view_get_column (self->introspection_tree_view, 0);

          /* Move the selected row as far up as we can so that the revealer
           * for the method invocation does not cover the selected area.
           */
          gtk_tree_view_scroll_to_cell (self->introspection_tree_view,
                                        path,
                                        column,
                                        TRUE,
                                        0.0,
                                        0.0);
        }
    }
}

static void
session_button_toggled_cb (DspyWindow     *self,
                           GtkRadioButton *button)
{
  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_RADIO_BUTTON (button));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      g_autoptr(DspyConnection) conn = dspy_connection_new_for_bus (G_BUS_TYPE_SESSION);
      dspy_connection_list_names_async (conn,
                                        NULL,
                                        dspy_window_list_names_cb,
                                        g_object_ref (self));
    }
}

static void
system_button_toggled_cb (DspyWindow     *self,
                          GtkRadioButton *button)
{
  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_RADIO_BUTTON (button));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      g_autoptr(DspyConnection) conn = dspy_connection_new_for_bus (G_BUS_TYPE_SYSTEM);
      dspy_connection_list_names_async (conn,
                                        NULL,
                                        dspy_window_list_names_cb,
                                        g_object_ref (self));
    }
}

static void
dspy_window_init (DspyWindow *self)
{
  g_autoptr(DspyConnection) conn = dspy_connection_new_for_bus (G_BUS_TYPE_SESSION);

  gtk_widget_init_template (GTK_WIDGET (self));

  dspy_connection_list_names_async (conn,
                                    NULL,
                                    dspy_window_list_names_cb,
                                    g_object_ref (self));

  g_signal_connect_object (self,
                           "key-press-event",
                           G_CALLBACK (dzl_shortcut_manager_handle_event),
                           NULL,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->names_list_box,
                           "row-activated",
                           G_CALLBACK (name_row_activated_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->refresh_button,
                           "clicked",
                           G_CALLBACK (refresh_button_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->method_revealer,
                           "notify::child-revealed",
                           G_CALLBACK (notify_child_revealed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->introspection_tree_view,
                           "method-activated",
                           G_CALLBACK (method_activated_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->session_button,
                           "toggled",
                           G_CALLBACK (session_button_toggled_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->system_button,
                           "toggled",
                           G_CALLBACK (system_button_toggled_cb),
                           self,
                           G_CONNECT_SWAPPED);
}
