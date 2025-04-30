/* dspy-view.c
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
#include "dspy-interface.h"
#include "dspy-introspection.h"
#include "dspy-introspectable-row.h"
#include "dspy-method-view.h"
#include "dspy-name-marquee.h"
#include "dspy-name-row.h"
#include "dspy-node.h"
#include "dspy-pattern-spec.h"
#include "dspy-property.h"
#include "dspy-simple-popover.h"
#include "dspy-titled-model.h"
#include "dspy-tree-view.h"
#include "dspy-view.h"

struct _DspyView
{
  GtkWidget parent_instance;
};

typedef struct
{
  GCancellable          *cancellable;
  GtkFilterListModel    *filter_model;
  GListModel            *model;

  /* Template widgets */
  GtkTreeView             *introspection_tree_view;
  GtkListBox              *names_list_box;
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
  AdwNavigationSplitView  *paned;
  AdwNavigationPage       *bus_navigation_page;
  GtkRevealer             *bottom_revealer;
  AdwStatusPage           *status_page;
  GtkListView             *list_view;

  guint                    destroyed : 1;
} DspyViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DspyView, dspy_view, GTK_TYPE_WIDGET)

static void dspy_view_set_model (DspyView   *self,
                                 GListModel *model);

/**
 * dspy_view_new:
 *
 * Create a new #DspyView.
 *
 * This widget contains the window contents beneath the headerbar.
 *
 * Returns: (transfer full): a newly created #DspyView
 */
GtkWidget *
dspy_view_new (void)
{
  return g_object_new (DSPY_TYPE_VIEW, NULL);
}

static void
dspy_view_list_names_cb (GObject      *object,
                         GAsyncResult *result,
                         gpointer      user_data)
{
  DspyConnection *conn = (DspyConnection *)object;
  g_autoptr(DspyView) self = user_data;
  g_autoptr(GListModel) model = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (DSPY_IS_VIEW (self));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (DSPY_IS_CONNECTION (conn));

  if (!(model = dspy_connection_list_names_finish (conn, result, &error)))
    g_warning ("Failed to list names: %s", error->message);

  dspy_view_set_model (self, model);
}

static void
radio_button_toggled_cb (DspyView             *self,
                         DspyConnectionButton *button)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  DspyConnection *connection;

  g_assert (DSPY_IS_VIEW (self));
  g_assert (DSPY_IS_CONNECTION_BUTTON (button));

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return;

  gtk_stack_set_visible_child (priv->stack, gtk_stack_page_get_child (priv->empty));
  adw_navigation_page_set_title (priv->bus_navigation_page, _("Bus"));

  connection = dspy_connection_button_get_connection (button);
  dspy_connection_list_names_async (connection,
                                    NULL,
                                    dspy_view_list_names_cb,
                                    g_object_ref (self));
}

static void
connect_address_changed_cb (DspyView          *self,
                            DspySimplePopover *popover)
{
  const gchar *text;

  g_assert (DSPY_IS_VIEW (self));
  g_assert (DSPY_IS_SIMPLE_POPOVER (popover));

  text = dspy_simple_popover_get_text (popover);
  dspy_simple_popover_set_ready (popover, text && *text);
}

static void
connection_got_error_cb (DspyView       *self,
                         const GError   *error,
                         DspyConnection *connection)
{
  AdwDialog *dialog;
  const gchar *title;
  AdwApplicationWindow *window;

  g_assert (DSPY_IS_VIEW (self));
  g_assert (error != NULL);
  g_assert (DSPY_IS_CONNECTION (connection));

  window  = ADW_APPLICATION_WINDOW (gtk_widget_get_root (GTK_WIDGET (self)));

  /* Only show one dialog at a time */
  if (adw_application_window_get_visible_dialog (window) != NULL)
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

  dialog = adw_alert_dialog_new (title, error->message);
  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog),
                                 "close",  _("_Close"));
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "close");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "close");
  adw_dialog_present (dialog, GTK_WIDGET (window));
}

static void
connect_address_activate_cb (DspyView          *self,
                             const gchar       *text,
                             DspySimplePopover *popover)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  g_autoptr(DspyConnection) connection = NULL;
  DspyConnectionButton *button;

  g_assert (DSPY_IS_VIEW (self));
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
clear_search (DspyView *self)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);

  g_assert (DSPY_IS_VIEW (self));

  if (priv->filter_model != NULL)
    gtk_filter_list_model_set_filter (priv->filter_model, NULL);
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
apply_search (DspyView    *self,
              const gchar *text)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);

  g_assert (DSPY_IS_VIEW (self));
  g_assert (text != NULL);
  g_assert (text[0] != 0);

  if (priv->filter_model != NULL)
    {
      g_autoptr(GtkCustomFilter) filter = NULL;

      filter = gtk_custom_filter_new ((GtkCustomFilterFunc) search_filter_func,
                                      dspy_pattern_spec_new (text),
                                      (GDestroyNotify) dspy_pattern_spec_unref);
      gtk_filter_list_model_set_filter (priv->filter_model, GTK_FILTER (filter));
    }
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
dspy_view_set_model (DspyView   *self,
                     GListModel *model)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  const gchar *text;
  GtkAdjustment *adj;

  g_assert (DSPY_IS_VIEW (self));
  g_assert (!model || G_IS_LIST_MODEL (model));

  /* Asynchronous completion implies that we might get here after
   * the widget has been destroyed.
   */
  if (priv->destroyed)
    return;

  gtk_list_box_bind_model (priv->names_list_box, NULL, NULL, NULL, NULL);

  g_clear_object (&priv->filter_model);
  g_clear_object (&priv->model);

  if (model != NULL)
    {
      priv->model = g_object_ref (model);
      priv->filter_model = gtk_filter_list_model_new (g_object_ref (model), NULL);
    }

  text = gtk_editable_get_text (GTK_EDITABLE (priv->search_entry));

  if (text && *text)
    apply_search (self, text);
  else
    clear_search (self);

  gtk_list_box_bind_model (priv->names_list_box,
                           G_LIST_MODEL (priv->filter_model),
                           create_name_row_cb,
                           NULL,
                           NULL);

  adj = gtk_scrolled_window_get_vadjustment (priv->names_scroller);
  gtk_adjustment_set_value (adj, 0.0);
}

static void
dspy_view_introspect_cb (GObject      *object,
                         GAsyncResult *result,
                         gpointer      user_data)
{
  DspyName *name = (DspyName *)object;
  g_autoptr(GtkTreeModel) model = NULL;
  g_autoptr(DspyView) self = user_data;
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  g_autoptr(GError) error = NULL;

  g_assert (DSPY_IS_NAME (name));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (DSPY_IS_VIEW (self));

  if (!(model = dspy_name_introspect_finish (name, result, &error)))
    {
      DspyConnection *connection = dspy_name_get_connection (name);
      dspy_connection_add_error (connection, error);
    }

  gtk_tree_view_set_model (priv->introspection_tree_view, model);
}

static GListModel *
create_child_model_cb (gpointer item,
                       gpointer user_data)
{
  if (DSPY_IS_NODE (item))
    {
      return dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (item), &DSPY_NODE (item)->interfaces);
    }
  else if (DSPY_IS_INTERFACE (item))
    {
      DspyInterface *iface = DSPY_INTERFACE (item);
      g_autoptr(GListStore) store = g_list_store_new (G_TYPE_LIST_MODEL);

      if (iface->signals.length > 0)
        {
          g_autoptr(GListModel) base = dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (item), &DSPY_INTERFACE (item)->signals);
          g_autoptr(GListModel) model = dspy_titled_model_new (base, _("Signals"));

          g_list_store_append (store, model);
        }

      if (iface->properties.length > 0)
        {
          g_autoptr(GListModel) base = dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (item), &DSPY_INTERFACE (item)->properties);
          g_autoptr(GListModel) model = dspy_titled_model_new (base, _("Properties"));

          g_list_store_append (store, model);
        }

      if (iface->methods.length > 0)
        {
          g_autoptr(GListModel) base = dspy_introspectable_queue_to_list (DSPY_INTROSPECTABLE (item), &DSPY_INTERFACE (item)->methods);
          g_autoptr(GListModel) model = dspy_titled_model_new (base, _("Methods"));

          g_list_store_append (store, model);
        }

      return G_LIST_MODEL (g_steal_pointer (&store));
    }
  else if (DSPY_IS_TITLED_MODEL (item))
    {
      return g_object_ref (G_LIST_MODEL (item));
    }

  return NULL;
}

static DexFuture *
handle_introspection (DexFuture *completed,
                      gpointer   user_data)
{
  DspyView *self = user_data;
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  g_autoptr(DspyIntrospection) introspection = NULL;
  g_autoptr(GtkTreeListModel) tree = NULL;
  g_autoptr(GtkNoSelection) model = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (DSPY_IS_VIEW (self));

  if (!(introspection = dex_await_object (dex_ref (completed), &error)))
    {
      g_warning ("Failed to get introspection: %s", error->message);
      return NULL;
    }

  tree = gtk_tree_list_model_new (g_object_ref (G_LIST_MODEL (introspection)),
                                  FALSE, FALSE,
                                  create_child_model_cb,
                                  NULL, NULL);
  model = gtk_no_selection_new (g_object_ref (G_LIST_MODEL (tree)));
  gtk_list_view_set_model (priv->list_view, GTK_SELECTION_MODEL (model));

  return NULL;
}

static void
name_row_activated_cb (DspyView    *self,
                       DspyNameRow *row,
                       GtkListBox  *list_box)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  DspyName *name;

  g_assert (DSPY_IS_VIEW (self));
  g_assert (DSPY_IS_NAME_ROW (row));
  g_assert (GTK_IS_LIST_BOX (list_box));

  name = dspy_name_row_get_name (row);

  g_cancellable_cancel (priv->cancellable);
  g_clear_object (&priv->cancellable);
  priv->cancellable = g_cancellable_new ();

  gtk_tree_view_set_model (priv->introspection_tree_view, NULL);
  dspy_name_marquee_set_name (priv->name_marquee, name);

  adw_navigation_page_set_title (priv->bus_navigation_page, dspy_name_get_name (name));
  gtk_revealer_set_reveal_child (priv->bottom_revealer, FALSE);

  dspy_name_introspect_async (name,
                              priv->cancellable,
                              dspy_view_introspect_cb,
                              g_object_ref (self));

  adw_navigation_split_view_set_show_content (priv->paned, TRUE);

  gtk_stack_set_visible_child (priv->stack, gtk_stack_page_get_child (priv->introspect));

  dex_future_disown (dex_future_finally (dspy_introspection_new (dspy_connection_get_connection (dspy_name_get_connection (name)),
                                                                 dspy_name_get_owner (name),
                                                                 "/"),
                                         handle_introspection,
                                         g_object_ref (self),
                                         g_object_unref));

}

static void
refresh_button_clicked_cb (DspyView  *self,
                           GtkButton *button)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  GtkListBoxRow *row;

  g_assert (DSPY_IS_VIEW (self));
  g_assert (GTK_IS_BUTTON (button));

  if ((row = gtk_list_box_get_selected_row (priv->names_list_box)))
    name_row_activated_cb (self, DSPY_NAME_ROW (row), priv->names_list_box);
}

static void
method_activated_cb (DspyView             *self,
                     DspyMethodInvocation *invocation,
                     DspyTreeView         *tree_view)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);

  g_assert (DSPY_IS_VIEW (self));
  g_assert (!invocation || DSPY_IS_METHOD_INVOCATION (invocation));
  g_assert (DSPY_IS_TREE_VIEW (tree_view));

  if (DSPY_IS_METHOD_INVOCATION (invocation))
    {
      dspy_method_view_set_invocation (priv->method_view, invocation);
      gtk_revealer_set_reveal_child (priv->bottom_revealer, TRUE);
    }
}

static void
notify_child_revealed_cb (DspyView       *self,
                          GParamSpec     *pspec,
                          GtkRevealer    *revealer)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);

  g_assert (DSPY_IS_VIEW (self));
  g_assert (GTK_IS_REVEALER (revealer));

  if (!gtk_revealer_get_reveal_child (revealer))
    {
      dspy_method_view_set_invocation (priv->method_view, NULL);
    }
  else
    {
      GtkTreeSelection *selection;
      GtkTreeModel *model = NULL;
      GtkTreeIter iter;

      selection = gtk_tree_view_get_selection (priv->introspection_tree_view);

      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
          g_autoptr(GtkTreePath) path = gtk_tree_model_get_path (model, &iter);
          GtkTreeViewColumn *column = gtk_tree_view_get_column (priv->introspection_tree_view, 0);

          /* Move the selected row as far up as we can so that the bottom bars
           * for the method invocation does not cover the selected area.
           */
          gtk_tree_view_scroll_to_cell (priv->introspection_tree_view,
                                        path,
                                        column,
                                        TRUE,
                                        0.0,
                                        0.0);
        }
    }
}

static void
search_entry_changed_cb (DspyView       *self,
                         GtkSearchEntry *search_entry)
{
  const gchar *text;

  g_assert (DSPY_IS_VIEW (self));
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
  DspyView *self = user_data;
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  GtkPopover *popover;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (DSPY_IS_VIEW (self));

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

static void
dspy_view_list_view_activate_cb (DspyView    *self,
                                 guint        position,
                                 GtkListView *list_view)
{
  GtkSelectionModel *model;
  g_autoptr(GtkTreeListRow) row = NULL;
  g_autoptr(GObject) item = NULL;

  g_assert (DSPY_IS_VIEW (self));
  g_assert (GTK_IS_LIST_VIEW (list_view));

  model = gtk_list_view_get_model (list_view);
  row = g_list_model_get_item (G_LIST_MODEL (model), position);
  item = gtk_tree_list_row_get_item (row);

  if (DSPY_IS_PROPERTY (item))
    dspy_property_query_value (DSPY_PROPERTY (item));
}

static GActionEntry action_entries[] = {
  { "connect-to-bus", connect_to_bus_action },
};

static void
dspy_view_constructed (GObject *object)
{
  DspyView *self = (DspyView *)object;
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->session_button), TRUE);

  G_OBJECT_CLASS (dspy_view_parent_class)->constructed (object);

  adw_status_page_set_icon_name (priv->status_page, APP_ID "-symbolic");
}

static void
dspy_view_dispose (GObject *object)
{
  DspyView *self = (DspyView *)object;
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  GtkWidget *child;

  priv->destroyed = TRUE;

  g_cancellable_cancel (priv->cancellable);
  g_clear_object (&priv->cancellable);
  g_clear_object (&priv->filter_model);
  g_clear_object (&priv->model);

  gtk_widget_dispose_template (GTK_WIDGET (self), DSPY_TYPE_VIEW);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (dspy_view_parent_class)->dispose (object);
}

static void
dspy_view_class_init (DspyViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dspy_view_dispose;
  object_class->constructed = dspy_view_constructed;

  gtk_widget_class_set_css_name (widget_class, "dspyview");
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-view.ui");
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, introspection_tree_view);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, list_view);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, menu_button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, method_view);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, name_marquee);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, names_list_box);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, names_scroller);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, radio_buttons);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, refresh_button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, search_entry);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, session_button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, stack);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, system_button);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, introspect);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, empty);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, paned);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, bus_navigation_page);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, bottom_revealer);
  gtk_widget_class_bind_template_child_private (widget_class, DspyView, status_page);
  gtk_widget_class_bind_template_callback (widget_class, dspy_view_list_view_activate_cb);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  g_type_ensure (ADW_TYPE_STATUS_PAGE);
  g_type_ensure (DSPY_TYPE_CONNECTION_BUTTON);
  g_type_ensure (DSPY_TYPE_METHOD_VIEW);
  g_type_ensure (DSPY_TYPE_NAME_MARQUEE);
  g_type_ensure (DSPY_TYPE_TREE_VIEW);
  g_type_ensure (DSPY_TYPE_INTROSPECTABLE_ROW);
}

static void
dspy_view_init (DspyView *self)
{
  DspyViewPrivate *priv = dspy_view_get_instance_private (self);
  g_autoptr(GSimpleActionGroup) actions = g_simple_action_group_new ();
  GMenu *menu;

  gtk_widget_init_template (GTK_WIDGET (self));

  g_action_map_add_action_entries (G_ACTION_MAP (actions),
                                   action_entries,
                                   G_N_ELEMENTS (action_entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "dspy", G_ACTION_GROUP (actions));

  menu = gtk_application_get_menu_by_id (GTK_APPLICATION (g_application_get_default ()),
                                         "dspy-connections-menu");
  gtk_menu_button_set_menu_model (priv->menu_button, G_MENU_MODEL (menu));

  g_signal_connect_object (priv->names_list_box,
                           "row-activated",
                           G_CALLBACK (name_row_activated_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->refresh_button,
                           "clicked",
                           G_CALLBACK (refresh_button_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->bottom_revealer,
                           "notify::reveal-child",
                           G_CALLBACK (notify_child_revealed_cb),
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
