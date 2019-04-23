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
#include <glib/gi18n.h>

#include "dspy-window.h"

struct _DspyWindow
{
  GtkApplicationWindow   parent_instance;

  GCancellable          *cancellable;
  DzlListModelFilter    *filter_model;
  GListModel            *model;

  /* Template widgets */
  GtkHeaderBar          *header_bar;
  GtkTreeView           *introspection_tree_view;
  GtkListBox            *names_list_box;
  GtkButton             *refresh_button;
  DspyNameMarquee       *name_marquee;
  GtkScrolledWindow     *names_scroller;
  DspyMethodView        *method_view;
  GtkRevealer           *method_revealer;
  DspyConnectionButton  *session_button;
  DspyConnectionButton  *system_button;
  GtkSearchEntry        *search_entry;
  GtkMenuButton         *menu_button;
  GtkBox                *radio_buttons;

  guint                  destroyed : 1;
};

static void dspy_window_set_model     (DspyWindow   *self,
                                       GListModel   *model);
static void dspy_window_list_names_cb (GObject      *object,
                                       GAsyncResult *result,
                                       gpointer      user_data);

G_DEFINE_TYPE (DspyWindow, dspy_window, GTK_TYPE_APPLICATION_WINDOW)

static void
radio_button_toggled_cb (DspyWindow           *self,
                         DspyConnectionButton *button)
{
  DspyConnection *connection;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (DSPY_IS_CONNECTION_BUTTON (button));

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return;

  connection = dspy_connection_button_get_connection (button);
  dspy_connection_list_names_async (connection,
                                    NULL,
                                    dspy_window_list_names_cb,
                                    g_object_ref (self));
}

static void
connect_address_changed_cb (DspyWindow       *self,
                            DzlSimplePopover *popover)
{
  const gchar *text;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (DZL_IS_SIMPLE_POPOVER (popover));

  text = dzl_simple_popover_get_text (popover);
  dzl_simple_popover_set_ready (popover, text && *text);
}

static void
connection_got_error_cb (DspyWindow     *self,
                         const GError   *error,
                         DspyConnection *connection)
{
  GtkWidget *dialog;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (error != NULL);
  g_assert (DSPY_IS_CONNECTION (connection));

  dialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                   GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_CLOSE,
                                   "%s", _("DBus Connection Failed"));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
  g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
connect_address_activate_cb (DspyWindow       *self,
                             const gchar      *text,
                             DzlSimplePopover *popover)
{
  g_autoptr(DspyConnection) connection = NULL;
  DspyConnectionButton *button;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (DZL_IS_SIMPLE_POPOVER (popover));

  connection = dspy_connection_new_for_address (text);

  button = g_object_new (DSPY_TYPE_CONNECTION_BUTTON,
                         "group", self->session_button,
                         "connection", connection,
                         "visible", TRUE,
                         NULL);
  g_signal_connect_object (button,
                           "toggled",
                           G_CALLBACK (radio_button_toggled_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (dspy_connection_button_get_connection (button),
                           "error",
                           G_CALLBACK (connection_got_error_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_container_add (GTK_CONTAINER (self->radio_buttons), GTK_WIDGET (button));

  gtk_widget_activate (GTK_WIDGET (button));
}

static void
connect_to_bus_action (GSimpleAction *action,
                       GVariant      *params,
                       gpointer       user_data)
{
  DspyWindow *self = user_data;
  GtkPopover *popover;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (DSPY_IS_WINDOW (self));

  popover = g_object_new (DZL_TYPE_SIMPLE_POPOVER,
                          "button-text", _("Connect"),
                          "message", _("Provide the address of the message bus"),
                          "position", GTK_POS_RIGHT,
                          "title", _("Connect to Other Bus"),
                          "relative-to", self->system_button,
                          NULL);

  g_signal_connect_object (popover,
                           "changed",
                           G_CALLBACK (connect_address_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (popover,
                           "activate",
                           G_CALLBACK (connect_address_activate_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect (popover,
                    "closed",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  gtk_popover_popup (popover);
}

static GActionEntry actions[] = {
  { "connect-to-bus", connect_to_bus_action },
};

static void
dspy_window_destroy (GtkWidget *widget)
{
  DspyWindow *self = (DspyWindow *)widget;

  g_assert (DSPY_IS_WINDOW (self));

  self->destroyed = TRUE;

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->filter_model);
  g_clear_object (&self->model);

  GTK_WIDGET_CLASS (dspy_window_parent_class)->destroy (widget);
}

static void
dspy_window_class_init (DspyWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->destroy = dspy_window_destroy;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-window.ui");
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, introspection_tree_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, menu_button);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, method_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, method_revealer);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, name_marquee);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_list_box);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_scroller);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, radio_buttons);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, refresh_button);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, search_entry);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, session_button);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, system_button);

  g_type_ensure (DSPY_TYPE_METHOD_VIEW);
  g_type_ensure (DSPY_TYPE_NAME_MARQUEE);
  g_type_ensure (DSPY_TYPE_TREE_VIEW);
}

static void
clear_search (DspyWindow *self)
{
  g_assert (DSPY_IS_WINDOW (self));

  if (self->filter_model != NULL)
    dzl_list_model_filter_set_filter_func (self->filter_model, NULL, NULL, NULL);
}

static gboolean
search_filter_func (DspyName       *name,
                    DzlPatternSpec *spec)
{
  g_assert (DSPY_IS_NAME (name));
  g_assert (spec != NULL);

  return dzl_pattern_spec_match (spec, dspy_name_get_search_text (name));
}

static void
apply_search (DspyWindow  *self,
              const gchar *text)
{
  g_assert (DSPY_IS_WINDOW (self));
  g_assert (text != NULL);
  g_assert (text[0] != 0);

  if (self->filter_model != NULL)
    dzl_list_model_filter_set_filter_func (self->filter_model,
                                           (DzlListModelFilterFunc) search_filter_func,
                                           dzl_pattern_spec_new (text),
                                           (GDestroyNotify) dzl_pattern_spec_unref);
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
dspy_window_set_model (DspyWindow *self,
                       GListModel *model)
{
  const gchar *text;
  GtkAdjustment *adj;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (!model || G_IS_LIST_MODEL (model));

  /* Asynchronous completion implies that we might get here after
   * the widget has been destroyed.
   */
  if (self->destroyed)
    return;

  gtk_list_box_bind_model (self->names_list_box, NULL, NULL, NULL, NULL);

  g_clear_object (&self->filter_model);
  g_clear_object (&self->model);

  if (model != NULL)
    {
      self->model = g_object_ref (model);
      self->filter_model = dzl_list_model_filter_new (model);
    }

  text = gtk_entry_get_text (GTK_ENTRY (self->search_entry));

  if (text && *text)
    apply_search (self, text);
  else
    clear_search (self);

  gtk_list_box_bind_model (self->names_list_box,
                           G_LIST_MODEL (self->filter_model),
                           create_name_row_cb,
                           NULL,
                           NULL);

  adj = gtk_scrolled_window_get_vadjustment (self->names_scroller);
  gtk_adjustment_set_value (adj, 0.0);
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

  if (!(model = dspy_connection_list_names_finish (conn, result, &error)))
    g_warning ("Failed to list names: %s", error->message);

  dspy_window_set_model (self, model);
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
      DspyConnection *connection = dspy_name_get_connection (name);
      dspy_connection_add_error (connection, error);
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
search_entry_changed_cb (DspyWindow     *self,
                         GtkSearchEntry *search_entry)
{
  const gchar *text;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_SEARCH_ENTRY (search_entry));

  text = gtk_entry_get_text (GTK_ENTRY (search_entry));

  if (text == NULL || *text == 0)
    clear_search (self);
  else
    apply_search (self, text);
}

static void
dspy_window_init (DspyWindow *self)
{
  GMenu *menu;

  gtk_widget_init_template (GTK_WIDGET (self));

  g_action_map_add_action_entries (G_ACTION_MAP (self), actions, G_N_ELEMENTS (actions), self);

  menu = dzl_application_get_menu_by_id (DZL_APPLICATION (g_application_get_default ()),
                                         "connections-menu");
  gtk_menu_button_set_menu_model (self->menu_button, G_MENU_MODEL (menu));

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
                           G_CALLBACK (radio_button_toggled_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (dspy_connection_button_get_connection (self->session_button),
                           "error",
                           G_CALLBACK (connection_got_error_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->system_button,
                           "toggled",
                           G_CALLBACK (radio_button_toggled_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (dspy_connection_button_get_connection (self->system_button),
                           "error",
                           G_CALLBACK (connection_got_error_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->search_entry,
                           "changed",
                           G_CALLBACK (search_entry_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  radio_button_toggled_cb (self, self->session_button);
}
