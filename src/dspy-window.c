/* dspy-window.c
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

#include "config.h"

#include <glib/gi18n.h>

#include <adwaita.h>

#include "dspy-connection-button.h"
#include "dspy-list-model-filter.h"
#include "dspy-method-view.h"
#include "dspy-name-marquee.h"
#include "dspy-name-row.h"
#include "dspy-pattern-spec.h"
#include "dspy-simple-popover.h"
#include "dspy-tree-view.h"
#include "dspy-window.h"

struct _DspyWindow
{
  AdwApplicationWindow parent_instance;
};

typedef struct
{
  GCancellable          *cancellable;
  DspyListModelFilter   *filter_model;
  GListModel            *model;

  /* Template widgets */
  GtkTreeView             *introspection_tree_view;
  GtkListView             *names_list_view;
  GtkButton               *refresh_button;
  DspyNameMarquee         *name_marquee;
  GtkScrolledWindow       *names_scroller;
  DspyMethodView          *method_view;
  DspyConnectionButton    *session_button;
  DspyConnectionButton    *system_button;
  GtkSearchEntry          *search_entry;
  GtkMenuButton           *menu_button;
  GtkBox                  *radio_buttons;
  GtkStack                *stack;
  GtkStackPage            *introspect;
  GtkStackPage            *empty;
  AdwStatusPage           *status_page;
  AdwBottomSheet          *bottom_sheet;

  guint                    destroyed : 1;
} DspyWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DspyWindow, dspy_window, ADW_TYPE_APPLICATION_WINDOW)

static void dspy_window_set_model (DspyWindow   *self,
                                   GListModel *model);

/**
 * dspy_window_new:
 *
 * Create a new #DspyWindow.
 *
 * This widget contains the window contents beneath the headerbar.
 *
 * Returns: (transfer full): a newly created #DspyWindow
 */
GtkWidget *
dspy_window_new (void)
{
  return g_object_new (DSPY_TYPE_WINDOW, NULL);
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
radio_button_toggled_cb (DspyWindow           *self,
                         DspyConnectionButton *button)
{
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);
  DspyConnection *connection;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (DSPY_IS_CONNECTION_BUTTON (button));

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return;

  gtk_stack_set_visible_child (priv->stack, gtk_stack_page_get_child (priv->empty));

  connection = dspy_connection_button_get_connection (button);
  dspy_connection_list_names_async (connection,
                                    NULL,
                                    dspy_window_list_names_cb,
                                    g_object_ref (self));
}

static void
connect_address_changed_cb (DspyWindow        *self,
                            DspySimplePopover *popover)
{
  const gchar *text;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (DSPY_IS_SIMPLE_POPOVER (popover));

  text = dspy_simple_popover_get_text (popover);
  dspy_simple_popover_set_ready (popover, text && *text);
}

static void
connection_got_error_cb (DspyWindow     *self,
                         const GError   *error,
                         DspyConnection *connection)
{
  static GtkWidget *dialog;
  const gchar *title;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (error != NULL);
  g_assert (DSPY_IS_CONNECTION (connection));

  /* Only show one dialog at a time */
  if (dialog != NULL)
    return;

  if (g_error_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED))
    title = _("Access Denied by Peer");
  else if (g_error_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_AUTH_FAILED))
    title = _("Authentication Failed");
  else if (g_error_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_TIMEOUT))
    title = _("Operation Timed Out");
  else if (g_error_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_DISCONNECTED))
    title = _("Lost Connection to Bus");
  else
    title = _("D-Bus Connection Failed");

  dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (self))),
                                   GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_CLOSE,
                                   "%s", title);
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
  g_signal_connect (dialog, "response", G_CALLBACK (gtk_window_destroy), NULL);
  g_signal_connect_swapped (dialog, "response", G_CALLBACK (g_nullify_pointer), &dialog);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
connect_address_activate_cb (DspyWindow        *self,
                             const gchar       *text,
                             DspySimplePopover *popover)
{
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);
  g_autoptr(DspyConnection) connection = NULL;
  DspyConnectionButton *button;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (DSPY_IS_SIMPLE_POPOVER (popover));

  connection = dspy_connection_new_for_address (text);

  button = g_object_new (DSPY_TYPE_CONNECTION_BUTTON,
                         "group", priv->session_button,
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
  gtk_box_append (priv->radio_buttons, GTK_WIDGET (button));

  gtk_widget_activate (GTK_WIDGET (button));
}

static void
clear_search (DspyWindow *self)
{
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);

  g_assert (DSPY_IS_WINDOW (self));

  if (priv->filter_model != NULL)
    dspy_list_model_filter_set_filter_func (priv->filter_model, NULL, NULL, NULL);
}

static gboolean
search_filter_func (DspyName        *name,
                    DspyPatternSpec *spec)
{
  g_assert (DSPY_IS_NAME (name));
  g_assert (spec != NULL);

  return dspy_pattern_spec_match (spec, dspy_name_get_search_text (name));
}

static void
apply_search (DspyWindow *self,
              const char *text)
{
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (text != NULL);
  g_assert (text[0] != 0);

  if (priv->filter_model != NULL)
    dspy_list_model_filter_set_filter_func (priv->filter_model,
                                            (DspyListModelFilterFunc) search_filter_func,
                                            dspy_pattern_spec_new (text),
                                            (GDestroyNotify) dspy_pattern_spec_unref);
}

static void
dspy_window_set_model (DspyWindow *self,
                       GListModel *model)
{
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);
  GtkSelectionModel *selection;
  const gchar *text;
  GtkAdjustment *adj;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (!model || G_IS_LIST_MODEL (model));

  /* Asynchronous completion implies that we might get here after
   * the widget has been destroyed.
   */
  if (priv->destroyed)
    return;

  gtk_list_view_set_model (priv->names_list_view, NULL);

  g_clear_object (&priv->filter_model);
  g_clear_object (&priv->model);

  if (model != NULL)
    {
      priv->model = g_object_ref (model);
      priv->filter_model = dspy_list_model_filter_new (model);
    }

  text = gtk_editable_get_text (GTK_EDITABLE (priv->search_entry));

  if (text && *text)
    apply_search (self, text);
  else
    clear_search (self);

  selection = g_object_new (GTK_TYPE_SINGLE_SELECTION,
                            "model", priv->filter_model,
                            NULL);

  gtk_list_view_set_model (priv->names_list_view, selection);

  adj = gtk_scrolled_window_get_vadjustment (priv->names_scroller);
  gtk_adjustment_set_value (adj, 0.0);
}

static void
dspy_window_introspect_cb (GObject      *object,
                           GAsyncResult *result,
                           gpointer      user_data)
{
  DspyName *name = (DspyName *)object;
  g_autoptr(GtkTreeModel) model = NULL;
  g_autoptr(DspyWindow) self = user_data;
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);
  g_autoptr(GError) error = NULL;

  g_assert (DSPY_IS_NAME (name));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (DSPY_IS_WINDOW (self));

  if (!(model = dspy_name_introspect_finish (name, result, &error)))
    {
      DspyConnection *connection = dspy_name_get_connection (name);
      dspy_connection_add_error (connection, error);
    }

  gtk_tree_view_set_model (priv->introspection_tree_view, model);
}

static void
name_row_activate_cb (DspyWindow  *self,
                      guint        position,
                      GtkListView *list_view)
{
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);
  g_autoptr(DspyName) name = NULL;
  GtkSelectionModel *model;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_LIST_VIEW (list_view));

  model = gtk_list_view_get_model (list_view);
  name = g_list_model_get_item (G_LIST_MODEL (model), position);

  g_cancellable_cancel (priv->cancellable);
  g_clear_object (&priv->cancellable);
  priv->cancellable = g_cancellable_new ();

  gtk_tree_view_set_model (priv->introspection_tree_view, NULL);
  dspy_name_marquee_set_name (priv->name_marquee, name);

  dspy_name_introspect_async (name,
                              priv->cancellable,
                              dspy_window_introspect_cb,
                              g_object_ref (self));

  gtk_stack_set_visible_child (priv->stack, gtk_stack_page_get_child (priv->introspect));
  adw_bottom_sheet_set_open (priv->bottom_sheet, TRUE);
}

static void
refresh_button_clicked_cb (DspyWindow *self,
                           GtkButton  *button)
{
  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_BUTTON (button));

  g_warning ("TODO: refresh button");
}

static void
method_activated_cb (DspyWindow           *self,
                     DspyMethodInvocation *invocation,
                     DspyTreeView         *tree_view)
{
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (!invocation || DSPY_IS_METHOD_INVOCATION (invocation));
  g_assert (DSPY_IS_TREE_VIEW (tree_view));

  if (DSPY_IS_METHOD_INVOCATION (invocation))
    dspy_method_view_set_invocation (priv->method_view, invocation);
}

static void
search_entry_changed_cb (DspyWindow     *self,
                         GtkSearchEntry *search_entry)
{
  const gchar *text;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_SEARCH_ENTRY (search_entry));

  text = gtk_editable_get_text (GTK_EDITABLE (search_entry));

  if (text == NULL || *text == 0)
    clear_search (self);
  else
    apply_search (self, text);
}

static void
connect_to_bus_action (GSimpleAction *action,
                       GVariant      *params,
                       gpointer       user_data)
{
  DspyWindow *self = user_data;
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);
  GtkPopover *popover;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (DSPY_IS_WINDOW (self));

  popover = g_object_new (DSPY_TYPE_SIMPLE_POPOVER,
                          "button-text", _("Connect"),
                          "message", _("Provide the address of the message bus"),
                          "position", GTK_POS_RIGHT,
                          "title", _("Connect to Other Bus"),
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
                    G_CALLBACK (gtk_widget_unparent),
                    NULL);

  gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (priv->system_button));

  gtk_popover_popup (popover);
}

static GActionEntry action_entries[] = {
  { "connect-to-bus", connect_to_bus_action },
};

static void
dspy_window_constructed (GObject *object)
{
  DspyWindow *self = (DspyWindow *)object;
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);

  G_OBJECT_CLASS (dspy_window_parent_class)->constructed (object);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->session_button), TRUE);

  adw_status_page_set_icon_name (priv->status_page, APP_ID "-symbolic");
}

static void
dspy_window_dispose (GObject *object)
{
  DspyWindow *self = (DspyWindow *)object;
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);

  priv->destroyed = TRUE;

  g_cancellable_cancel (priv->cancellable);

  gtk_widget_dispose_template (GTK_WIDGET (self), DSPY_TYPE_WINDOW);

  g_clear_object (&priv->cancellable);
  g_clear_object (&priv->filter_model);
  g_clear_object (&priv->model);

  G_OBJECT_CLASS (dspy_window_parent_class)->dispose (object);
}

static void
dspy_window_class_init (DspyWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dspy_window_dispose;
  object_class->constructed = dspy_window_constructed;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-window.ui");

  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, bottom_sheet);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, introspection_tree_view);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, menu_button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, method_view);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, name_marquee);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, names_list_view);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, names_scroller);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, radio_buttons);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, refresh_button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, search_entry);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, session_button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, stack);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, system_button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, introspect);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, empty);
  gtk_widget_class_bind_template_child_private (widget_class, DspyWindow, status_page);

  g_type_ensure (ADW_TYPE_STATUS_PAGE);
  g_type_ensure (DSPY_TYPE_CONNECTION_BUTTON);
  g_type_ensure (DSPY_TYPE_METHOD_VIEW);
  g_type_ensure (DSPY_TYPE_NAME_MARQUEE);
  g_type_ensure (DSPY_TYPE_NAME_ROW);
  g_type_ensure (DSPY_TYPE_TREE_VIEW);
}

static void
dspy_window_init (DspyWindow *self)
{
  DspyWindowPrivate *priv = dspy_window_get_instance_private (self);
  g_autoptr(GSimpleActionGroup) actions = g_simple_action_group_new ();
  GMenu *menu;

  gtk_widget_init_template (GTK_WIDGET (self));

#if DEVELOPMENT_BUILD
  gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
#endif

  g_action_map_add_action_entries (G_ACTION_MAP (actions),
                                   action_entries,
                                   G_N_ELEMENTS (action_entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "dspy", G_ACTION_GROUP (actions));

  menu = gtk_application_get_menu_by_id (GTK_APPLICATION (g_application_get_default ()),
                                         "dspy-connections-menu");
  gtk_menu_button_set_menu_model (priv->menu_button, G_MENU_MODEL (menu));

  g_signal_connect_object (priv->names_list_view,
                           "activate",
                           G_CALLBACK (name_row_activate_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->refresh_button,
                           "clicked",
                           G_CALLBACK (refresh_button_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->introspection_tree_view,
                           "method-activated",
                           G_CALLBACK (method_activated_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->session_button,
                           "toggled",
                           G_CALLBACK (radio_button_toggled_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (dspy_connection_button_get_connection (priv->session_button),
                           "error",
                           G_CALLBACK (connection_got_error_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->system_button,
                           "toggled",
                           G_CALLBACK (radio_button_toggled_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (dspy_connection_button_get_connection (priv->system_button),
                           "error",
                           G_CALLBACK (connection_got_error_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->search_entry,
                           "changed",
                           G_CALLBACK (search_entry_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  radio_button_toggled_cb (self, priv->session_button);
}
