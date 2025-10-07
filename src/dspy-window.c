/* dspy-window.c
 *
 * Copyright 2019 Christian Hergert <chergert@redhat.com>
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

#include <glib/gi18n.h>

#include "dspy-argument.h"
#include "dspy-connection.h"
#include "dspy-interface.h"
#include "dspy-method.h"
#include "dspy-method-argument.h"
#include "dspy-method-invocation.h"
#include "dspy-name.h"
#include "dspy-node.h"
#include "dspy-property.h"
#include "dspy-signal.h"
#include "dspy-util.h"
#include "dspy-window.h"

struct _DspyWindow
{
  AdwApplicationWindow    parent_instance;

  GListStore             *connections;
  DspyConnection         *connection;
  DspyName               *name;
  DspyNode               *node;
  DspyInterface          *interface;
  DspyIntrospectable     *member;
  GCancellable           *cancellable;

  AdwActionRow           *dur_row;
  AdwActionRow           *min_row;
  AdwActionRow           *max_row;
  AdwPreferencesGroup    *duration_group;
  AdwPreferencesGroup    *parameters_group;
  GtkStack               *call_button_stack;
  AdwEntryRow            *new_connection_entry;
  AdwNavigationView      *sidebar_view;
  AdwNavigationView      *content_view;
  AdwNavigationPage      *connections_page;
  AdwNavigationPage      *names_page;
  AdwDialog              *connection_dialog;
  GtkStack               *content_stack;
  GtkStack               *details_stack;
  GtkListBox             *in_arguments;
  AdwNavigationPage      *interfaces_page;
  AdwNavigationPage      *members_page;
  AdwNavigationPage      *objects_page;
  AdwBreakpoint          *narrow_breakpoint;
  AdwOverlaySplitView    *overlay_split_view;
  AdwNavigationSplitView *split_view;
  GtkListView            *connections_list_view;
  GtkSortListModel       *interfaces_sorted;
  GtkListView            *names_list_view;
  GtkSortListModel       *names_sorted;
  GtkCustomSorter        *names_sorter;
  GtkSortListModel       *objects_sorted;
  AdwActionRow           *property_value;
  AdwToastOverlay        *property_toast;
  GtkTextBuffer          *result_buffer;
  AdwPreferencesGroup    *result_group;
  GtkTextView            *result_view;
  GtkWidget              *result_frame;

  gint64                  last_call_at;
  gint64                  min_duration;
  gint64                  max_duration;
};

G_DEFINE_FINAL_TYPE (DspyWindow, dspy_window, ADW_TYPE_APPLICATION_WINDOW)

enum {
  PROP_0,
  PROP_CONNECTION,
  PROP_CONNECTIONS,
  PROP_INTERFACE,
  PROP_MEMBER,
  PROP_NAME,
  PROP_NODE,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
dspy_window_update_property_value (DspyWindow *self)
{
  GDBusConnection *connection = NULL;

  g_assert (DSPY_IS_WINDOW (self));

  if (self->connection != NULL)
    connection = dspy_connection_get_connection (self->connection);

  if (!DSPY_IS_PROPERTY (self->member) || connection == NULL || self->name == NULL)
    return;

  dex_future_disown (dspy_property_query_value (DSPY_PROPERTY (self->member),
                                                connection,
                                                dspy_name_get_owner (self->name)));
}

static DexFuture *
dspy_window_add_a11y_bus (DexFuture *completed,
                          gpointer   user_data)
{
  GListStore *store = user_data;
  g_autoptr(GError) error = NULL;
  g_autofree char *address = NULL;

  g_assert (DEX_IS_FUTURE (completed));
  g_assert (G_IS_LIST_STORE (store));

  if ((address = dex_await_string (dex_ref (completed), &error)))
    {
      g_autoptr(DspyConnection) connection = dspy_connection_new_for_address (address);

      dspy_connection_set_title (connection, _("Accessibility Bus"));
      g_list_store_append (store, connection);
    }

  return dex_ref (completed);
}

static int
dspy_window_sort_names (gconstpointer a,
                        gconstpointer b,
                        gpointer      user_data)
{
  DspyName *name_a = (DspyName *)a;
  DspyName *name_b = (DspyName *)b;
  const char *str_a = dspy_name_get_name (name_a);
  const char *str_b = dspy_name_get_name (name_b);

  if (str_a[0] == ':' && str_b[0] != ':')
    return 1;

  if (str_b[0] == ':' && str_a[0] != ':')
    return -11;

  if (str_a[0] == ':')
    {
      g_autofree char *collate_a = g_utf8_collate_key_for_filename (str_a, -1);
      g_autofree char *collate_b = g_utf8_collate_key_for_filename (str_b, -1);

      return strcmp (collate_a, collate_b);
    }

  return strcmp (str_a, str_b);
}

static void
dspy_window_connection_activate_cb (DspyWindow  *self,
                                    guint        position,
                                    GtkListView *list_view)
{
  g_autoptr(DspyConnection) connection = NULL;
  g_autoptr(GListModel) names = NULL;
  GtkSelectionModel *model;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_LIST_VIEW (list_view));

  model = gtk_list_view_get_model (list_view);
  connection = g_list_model_get_item (G_LIST_MODEL (model), position);
  names = dspy_connection_list_names (connection);

  gtk_sort_list_model_set_model (self->names_sorted, names);

  if (g_set_object (&self->connection, connection))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CONNECTION]);

  adw_navigation_view_pop_to_page (self->sidebar_view, self->connections_page);
  adw_navigation_view_push (self->sidebar_view, self->names_page);

  gtk_stack_set_visible_child_name (self->content_stack, "empty");
  gtk_stack_set_visible_child_name (self->details_stack, "empty");
}

static void
dspy_window_name_activate_cb (DspyWindow  *self,
                              guint        position,
                              GtkListView *list_view)
{
  g_autoptr(DspyName) name = NULL;
  GtkSelectionModel *model;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_LIST_VIEW (list_view));

  model = gtk_list_view_get_model (list_view);
  name = g_list_model_get_item (G_LIST_MODEL (model), position);

  if (g_set_object (&self->name, name))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_NAME]);

  if (g_set_object (&self->member, NULL))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MEMBER]);

  if (g_set_object (&self->interface, NULL))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INTERFACE]);

  gtk_widget_set_visible (GTK_WIDGET (self->parameters_group), FALSE);

  adw_navigation_view_pop_to_page (self->sidebar_view, self->names_page);
  adw_navigation_view_push (self->sidebar_view, self->objects_page);

  gtk_stack_set_visible_child_name (self->content_stack, "empty");
  gtk_stack_set_visible_child_name (self->details_stack, "empty");
}

static void
dspy_window_node_activate_cb (DspyWindow  *self,
                              guint        position,
                              GtkListView *list_view)
{
  g_autoptr(DspyNode) node = NULL;
  GtkSelectionModel *model;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_LIST_VIEW (list_view));

  model = gtk_list_view_get_model (list_view);
  node = g_list_model_get_item (G_LIST_MODEL (model), position);

  if (g_set_object (&self->node, node))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_NODE]);

  if (g_set_object (&self->interface, NULL))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INTERFACE]);

  if (g_set_object (&self->interface, NULL))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MEMBER]);

  adw_navigation_view_pop_to_page (self->content_view, self->interfaces_page);

  gtk_stack_set_visible_child_name (self->content_stack, "content");
  gtk_stack_set_visible_child_name (self->details_stack, "empty");

  adw_navigation_split_view_set_show_content (self->split_view, TRUE);
}

static void
dspy_window_interface_activate_cb (DspyWindow  *self,
                                   guint        position,
                                   GtkListView *list_view)
{
  g_autoptr(DspyInterface) interface = NULL;
  GtkSelectionModel *model;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_LIST_VIEW (list_view));

  model = gtk_list_view_get_model (list_view);
  interface = g_list_model_get_item (G_LIST_MODEL (model), position);

  if (g_set_object (&self->interface, interface))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INTERFACE]);

  adw_navigation_view_pop_to_page (self->content_view, self->interfaces_page);
  adw_navigation_view_push (self->content_view, self->members_page);
}

static void
update_error_css (DspyMethodArgument *arg,
                  GParamSpec         *pspec,
                  GtkWidget          *row)
{
  if (dspy_method_argument_has_error (arg))
    gtk_widget_add_css_class (row, "error");
  else
    gtk_widget_remove_css_class (row, "error");
}

static GtkWidget *
create_argument_row (gpointer item,
                     gpointer user_data)
{
  DspyArgument *arg = item;
  g_autoptr(DspyMethodArgument) method_arg = NULL;
  AdwEntryRow *row;
  GtkWidget *label;

  method_arg = dspy_method_argument_new (arg->signature, arg->name, NULL);

  label = gtk_label_new (arg->signature);
  row = g_object_new (ADW_TYPE_ENTRY_ROW,
                      "title", arg->name,
                      NULL);
  adw_entry_row_add_suffix (row, label);

  g_object_bind_property (row, "text",
                          method_arg, "value-text",
                          G_BINDING_SYNC_CREATE);

  g_signal_connect_object (method_arg,
                           "notify::has-error",
                           G_CALLBACK (update_error_css),
                           row,
                           0);

  g_object_set_data_full (G_OBJECT (row),
                          "METHOD_ARGUMENT",
                          g_object_ref (method_arg),
                          g_object_unref);

  return GTK_WIDGET (row);
}

static void
dspy_window_update_method (DspyWindow *self,
                           DspyMethod *method)
{
  g_autoptr(GListModel) model = NULL;
  guint n_items = 0;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (!method || DSPY_IS_METHOD (method));

  if (method == NULL)
    return;

  model = dspy_method_dup_in_arguments (method);
  n_items = g_list_model_get_n_items (model);

  gtk_list_box_bind_model (self->in_arguments,
                           model,
                           create_argument_row,
                           NULL, NULL);

  gtk_widget_set_visible (GTK_WIDGET (self->parameters_group), n_items > 0);
}

static void
dspy_window_member_activate_cb (DspyWindow  *self,
                                guint        position,
                                GtkListView *list_view)
{
  g_autoptr(DspyIntrospectable) member = NULL;
  GtkSelectionModel *model;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (GTK_IS_LIST_VIEW (list_view));

  model = gtk_list_view_get_model (list_view);
  member = g_list_model_get_item (G_LIST_MODEL (model), position);

  if (g_set_object (&self->member, member))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MEMBER]);

  self->last_call_at = 0;
  self->min_duration = 0;
  self->max_duration = 0;

  gtk_text_buffer_set_text (self->result_buffer, "", 0);
  gtk_widget_set_visible (GTK_WIDGET (self->result_group), FALSE);
  gtk_widget_set_visible (GTK_WIDGET (self->parameters_group), FALSE);

  if (DSPY_IS_PROPERTY (member))
    dspy_window_update_property_value (self);
  else if (DSPY_IS_METHOD (member))
    dspy_window_update_method (self, DSPY_METHOD (member));

  if (DSPY_IS_PROPERTY (member))
    gtk_stack_set_visible_child_name (self->details_stack, "property");
  else if (DSPY_IS_SIGNAL (member))
    gtk_stack_set_visible_child_name (self->details_stack, "signal");
  else if (DSPY_IS_METHOD (member))
    gtk_stack_set_visible_child_name (self->details_stack, "method");
  else
    gtk_stack_set_visible_child_name (self->details_stack, "empty");

  gtk_widget_set_visible (GTK_WIDGET (self->duration_group), FALSE);

  if (adw_overlay_split_view_get_collapsed (self->overlay_split_view))
    adw_overlay_split_view_set_show_sidebar (self->overlay_split_view, FALSE);
}

static char *
get_member_header_text (gpointer instance,
                        gpointer item)
{
  if (item == NULL)
    return NULL;

  if (DSPY_IS_METHOD (item))
    return g_strdup (_("Methods"));
  else if (DSPY_IS_PROPERTY (item))
    return g_strdup (_("Properties"));
  else if (DSPY_IS_SIGNAL (item))
    return g_strdup (_("Signals"));
  else
    g_return_val_if_reached (NULL);
}

static void
property_refresh_action (GtkWidget  *widget,
                         const char *action_name,
                         GVariant   *param)
{
  dspy_window_update_property_value (DSPY_WINDOW (widget));
}

static void
property_copy_action (GtkWidget  *widget,
                      const char *action_name,
                      GVariant   *param)
{
  DspyWindow *self = DSPY_WINDOW (widget);
  GdkClipboard *clipboard = gtk_widget_get_clipboard (widget);

  if (DSPY_IS_PROPERTY (self->member))
    {
      gdk_clipboard_set_text (clipboard, DSPY_PROPERTY (self->member)->value);

      adw_toast_overlay_add_toast (self->property_toast,
                                   g_object_new (ADW_TYPE_TOAST,
                                                 "title", _("Copied to Clipboard"),
                                                 "timeout", 2,
                                                 NULL));
    }
}

static void
focus_members_action (GtkWidget  *widget,
                      const char *action_name,
                      GVariant   *param)
{
  DspyWindow *self = DSPY_WINDOW (widget);

  g_assert (DSPY_IS_WINDOW (self));

  adw_overlay_split_view_set_show_sidebar (self->overlay_split_view, TRUE);
  adw_navigation_split_view_set_show_content (self->split_view, TRUE);
}

static void
new_connection_action (GtkWidget  *widget,
                       const char *action_name,
                       GVariant   *param)
{
  DspyWindow *self = DSPY_WINDOW (widget);

  g_assert (DSPY_IS_WINDOW (self));

  adw_dialog_present (self->connection_dialog, widget);
}

static void
add_connection_action (GtkWidget  *widget,
                       const char *action_name,
                       GVariant   *param)
{
  DspyWindow *self = DSPY_WINDOW (widget);
  g_autoptr(DspyConnection) connection = NULL;
  const char *text;

  g_assert (DSPY_IS_WINDOW (self));

  text = gtk_editable_get_text (GTK_EDITABLE (self->new_connection_entry));
  connection = dspy_connection_new_for_address (text);

  if (connection != NULL)
    g_list_store_append (self->connections, connection);

  adw_dialog_close (self->connection_dialog);
}

static void
dspy_window_method_invoke_cb (GObject      *object,
                              GAsyncResult *result,
                              gpointer      user_data)
{
  DspyMethodInvocation *invocation = (DspyMethodInvocation *)object;
  g_autoptr(DspyWindow) self = user_data;
  g_autoptr(GVariant) reply = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (DSPY_IS_METHOD_INVOCATION (invocation));
  g_assert (DSPY_IS_WINDOW (self));
  g_assert (G_IS_ASYNC_RESULT (result));

  if (!(reply = dspy_method_invocation_execute_finish (invocation, result, &error)))
    {
      g_warning ("%s", error->message);

      gtk_text_buffer_set_text (self->result_buffer, error->message, -1);
      gtk_widget_set_visible (GTK_WIDGET (self->result_group), TRUE);
      gtk_widget_add_css_class (GTK_WIDGET (self->result_frame), "error");
    }
  else
    {
      g_autofree char *res_string = NULL;
      gint64 now = g_get_monotonic_time ();
      gint64 duration = now - self->last_call_at;

      res_string = g_variant_print (reply, TRUE);
      gtk_text_buffer_set_text (self->result_buffer, res_string, -1);
      gtk_widget_set_visible (GTK_WIDGET (self->result_group), TRUE);
      gtk_widget_remove_css_class (GTK_WIDGET (self->result_frame), "error");

      if (self->min_duration == 0 || duration < self->min_duration)
        {
          double v = (double)duration / (double)G_USEC_PER_SEC;
          g_autofree char *str = g_strdup_printf ("%0.4lf %s", v, _("msec"));

          self->min_duration = duration;

          adw_action_row_set_subtitle (self->min_row, str);
        }

      if (duration > self->max_duration)
        {
          double v = (double)duration / (double)G_USEC_PER_SEC;
          g_autofree char *str = g_strdup_printf ("%0.4lf %s", v, _("msec"));

          self->max_duration = duration;

          adw_action_row_set_subtitle (self->max_row, str);
        }

        {
          double v = (double)duration / (double)G_USEC_PER_SEC;
          g_autofree char *str = g_strdup_printf ("%0.4lf %s", v, _("msec"));

          adw_action_row_set_subtitle (self->dur_row, str);
        }

      gtk_widget_set_visible (GTK_WIDGET (self->duration_group), TRUE);
    }

  g_clear_object (&self->cancellable);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "call-method", TRUE);
  gtk_stack_set_visible_child_name (self->call_button_stack, "call");

  gtk_widget_grab_focus (GTK_WIDGET (self->result_view));
}

static void
cancel_call_action (GtkWidget  *widget,
                    const char *action_name,
                    GVariant   *unused)
{
  g_cancellable_cancel (DSPY_WINDOW (widget)->cancellable);
}

static void
call_method_action (GtkWidget  *widget,
                    const char *action_name,
                    GVariant   *unused)
{
  DspyWindow *self = DSPY_WINDOW (widget);
  g_autoptr(DspyMethodInvocation) invocation = NULL;
  g_autoptr(GVariantBuilder) builder = NULL;
  g_autoptr(GVariant) params = NULL;
  g_autofree char *in_signature = NULL;
  g_autofree char *out_signature = NULL;
  GtkListBoxRow *row;
  DspyMethod *method;
  guint i = 0;

  g_assert (DSPY_IS_WINDOW (self));

  if (!DSPY_IS_METHOD (self->member) ||
      !DSPY_IS_NAME (self->name) ||
      !DSPY_IS_INTERFACE (self->interface) ||
      !DSPY_IS_NODE (self->node))
    return;

  in_signature = dspy_method_dup_in_signature (DSPY_METHOD (self->member));
  out_signature = dspy_method_dup_out_signature (DSPY_METHOD (self->member));

  builder = g_variant_builder_new (G_VARIANT_TYPE (in_signature));
  method = DSPY_METHOD (self->member);

  while ((row = gtk_list_box_get_row_at_index (self->in_arguments, i++)))
    {
      DspyMethodArgument *arg = g_object_get_data (G_OBJECT (row), "METHOD_ARGUMENT");
      g_autoptr(GVariant) param = NULL;

      g_assert (DSPY_IS_METHOD_ARGUMENT (arg));

      if (!(param = dspy_method_argument_dup_value (arg)) ||
          dspy_method_argument_has_error (arg))
        {
          gtk_widget_add_css_class (GTK_WIDGET (row), "error");
          gtk_widget_grab_focus (GTK_WIDGET (row));
          return;
        }

      g_variant_builder_add_value (builder, param);
    }

  params = g_variant_take_ref (g_variant_builder_end (builder));

  invocation = dspy_method_invocation_new ();
  dspy_method_invocation_set_name (invocation, self->name);
  dspy_method_invocation_set_method (invocation, method->name);
  dspy_method_invocation_set_object_path (invocation, self->node->path);
  dspy_method_invocation_set_interface (invocation, self->interface->name);
  dspy_method_invocation_set_timeout (invocation, G_MAXINT-1);
  dspy_method_invocation_set_signature (invocation, in_signature);
  dspy_method_invocation_set_reply_signature (invocation, out_signature);
  dspy_method_invocation_set_parameters (invocation, params);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "call-method", FALSE);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  self->cancellable = g_cancellable_new ();

  gtk_stack_set_visible_child_name (self->call_button_stack, "cancel");

  self->last_call_at = g_get_monotonic_time ();

  dspy_method_invocation_execute_async (invocation,
                                        self->cancellable,
                                        dspy_window_method_invoke_cb,
                                        g_object_ref (self));
}

static void
dspy_window_narrow_apply_cb (DspyWindow    *self,
                             AdwBreakpoint *breakpoint)
{
  g_assert (DSPY_IS_WINDOW (self));
  g_assert (ADW_IS_BREAKPOINT (breakpoint));

  if (self->node == NULL)
    {
      adw_overlay_split_view_set_show_sidebar (self->overlay_split_view, TRUE);
      adw_navigation_split_view_set_show_content (self->split_view, FALSE);
    }
}

static void
new_connection_entry_changed_cb (DspyWindow  *self,
                                 AdwEntryRow *entry)
{
  const char *text;

  g_assert (DSPY_IS_WINDOW (self));
  g_assert (ADW_IS_ENTRY_ROW (entry));

  text = gtk_editable_get_text (GTK_EDITABLE (entry));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "add-connection", text[0] != 0);
}

static char *
shorten_string (gpointer    instance,
                const char *str)
{
  g_autofree char *shorter = NULL;

  if (str == NULL)
    return NULL;

  if (g_utf8_strlen (str, -1) <= 100)
    return g_strdup (str);

  shorter = g_utf8_substring (str, 0, 100);

  return g_strdup_printf ("%s…", shorter);
}

static void
dspy_window_dispose (GObject *object)
{
  DspyWindow *self = (DspyWindow *)object;

  gtk_widget_dispose_template (GTK_WIDGET (self), DSPY_TYPE_WINDOW);

  g_clear_object (&self->cancellable);
  g_clear_object (&self->connections);
  g_clear_object (&self->connection);
  g_clear_object (&self->name);
  g_clear_object (&self->node);
  g_clear_object (&self->interface);
  g_clear_object (&self->member);

  G_OBJECT_CLASS (dspy_window_parent_class)->dispose (object);
}

static void
dspy_window_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  DspyWindow *self = DSPY_WINDOW (object);

  switch (prop_id)
    {
    case PROP_CONNECTIONS:
      g_value_set_object (value, self->connections);
      break;

    case PROP_CONNECTION:
      g_value_set_object (value, self->connection);
      break;

    case PROP_INTERFACE:
      g_value_set_object (value, self->interface);
      break;

    case PROP_MEMBER:
      g_value_set_object (value, self->member);
      break;

    case PROP_NAME:
      g_value_set_object (value, self->name);
      break;

    case PROP_NODE:
      g_value_set_object (value, self->node);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
refresh_objects_action (GtkWidget  *widget,
                        const char *action_name,
                        GVariant   *param)
{
  DspyWindow *self = DSPY_WINDOW (widget);
  g_autoptr(GListModel) new_introspection = NULL;

  g_assert (DSPY_IS_WINDOW (self));

  if (self->name == NULL)
    return;

  new_introspection = dspy_name_dup_introspection (self->name);
  gtk_sort_list_model_set_model (self->objects_sorted, new_introspection);
}

static void
dspy_window_class_init (DspyWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dspy_window_dispose;
  object_class->get_property = dspy_window_get_property;

  properties[PROP_CONNECTION] =
    g_param_spec_object ("connection", NULL, NULL,
                         DSPY_TYPE_CONNECTION,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_CONNECTIONS] =
    g_param_spec_object ("connections", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_INTERFACE] =
    g_param_spec_object ("interface", NULL, NULL,
                         DSPY_TYPE_INTERFACE,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_NAME] =
    g_param_spec_object ("name", NULL, NULL,
                         DSPY_TYPE_NAME,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_MEMBER] =
    g_param_spec_object ("member", NULL, NULL,
                         DSPY_TYPE_INTROSPECTABLE,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_NODE] =
    g_param_spec_object ("node", NULL, NULL,
                         DSPY_TYPE_NODE,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-window.ui");
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, call_button_stack);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, connection_dialog);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, connections_list_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, connections_page);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, content_stack);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, content_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, details_stack);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, in_arguments);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, interfaces_page);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, interfaces_sorted);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, members_page);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, max_row);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, min_row);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, dur_row);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, duration_group);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, parameters_group);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_list_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_page);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_sorted);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_sorter);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, narrow_breakpoint);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, new_connection_entry);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, objects_page);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, objects_sorted);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, overlay_split_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, property_toast);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, property_value);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, result_buffer);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, result_frame);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, result_group);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, result_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, sidebar_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, split_view);
  gtk_widget_class_bind_template_callback (widget_class, dspy_window_connection_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, dspy_window_name_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, dspy_window_narrow_apply_cb);
  gtk_widget_class_bind_template_callback (widget_class, dspy_window_node_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, dspy_window_interface_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, dspy_window_member_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, get_member_header_text);
  gtk_widget_class_bind_template_callback (widget_class, new_connection_entry_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, shorten_string);
  gtk_widget_class_install_action (widget_class, "property.refresh", NULL, property_refresh_action);
  gtk_widget_class_install_action (widget_class, "property.copy", NULL, property_copy_action);
  gtk_widget_class_install_action (widget_class, "focus-members", NULL, focus_members_action);
  gtk_widget_class_install_action (widget_class, "new-connection", NULL, new_connection_action);
  gtk_widget_class_install_action (widget_class, "add-connection", NULL, add_connection_action);
  gtk_widget_class_install_action (widget_class, "call-method", NULL, call_method_action);
  gtk_widget_class_install_action (widget_class, "cancel-call", NULL, cancel_call_action);
  gtk_widget_class_install_action (widget_class, "refresh-objects", NULL, refresh_objects_action);

  g_type_ensure (DSPY_TYPE_CONNECTION);
  g_type_ensure (DSPY_TYPE_NAME);
  g_type_ensure (DSPY_TYPE_NODE);
  g_type_ensure (DSPY_TYPE_PROPERTY);
  g_type_ensure (DSPY_TYPE_METHOD);
  g_type_ensure (DSPY_TYPE_SIGNAL);
}

static void
dspy_window_init (DspyWindow *self)
{
  g_autoptr(DspyConnection) session = NULL;
  g_autoptr(DspyConnection) system = NULL;

  self->connections = g_list_store_new (DSPY_TYPE_CONNECTION);

  session = dspy_connection_new_for_bus (G_BUS_TYPE_SESSION);
  system = dspy_connection_new_for_bus (G_BUS_TYPE_SYSTEM);

  g_list_store_append (self->connections, system);
  g_list_store_append (self->connections, session);

  dex_future_disown (dex_future_then (dspy_get_a11y_bus (),
                                      dspy_window_add_a11y_bus,
                                      g_object_ref (self->connections),
                                      g_object_unref));

  gtk_widget_init_template (GTK_WIDGET (self));

#if DEVELOPMENT_BUILD
  gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
#endif

  gtk_custom_sorter_set_sort_func (self->names_sorter,
                                   dspy_window_sort_names,
                                   NULL, NULL);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "add-connection", FALSE);
}

GtkWidget *
dspy_window_new (void)
{
  return g_object_new (DSPY_TYPE_WINDOW, NULL);
}
