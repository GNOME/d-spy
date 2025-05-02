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

#include "dspy-connection.h"
#include "dspy-name.h"
#include "dspy-util.h"
#include "dspy-view.h"
#include "dspy-window.h"

struct _DspyWindow
{
  AdwApplicationWindow  parent_instance;

  GtkListView          *connections_list_view;
  GtkListView          *names_list_view;
  GtkCustomSorter      *names_sorter;
  DspyView             *view;

  GListStore           *connections;
};

G_DEFINE_FINAL_TYPE (DspyWindow, dspy_window, ADW_TYPE_APPLICATION_WINDOW)

enum {
  PROP_0,
  PROP_CONNECTIONS,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

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
dspy_window_dispose (GObject *object)
{
  DspyWindow *self = (DspyWindow *)object;

  gtk_widget_dispose_template (GTK_WIDGET (self), DSPY_TYPE_WINDOW);

  g_clear_object (&self->connections);

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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_window_class_init (DspyWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dspy_window_dispose;
  object_class->get_property = dspy_window_get_property;

  properties[PROP_CONNECTIONS] =
    g_param_spec_object ("connections", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-window.ui");
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, connections_list_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_list_view);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, names_sorter);
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, view);

  g_type_ensure (DSPY_TYPE_CONNECTION);
  g_type_ensure (DSPY_TYPE_NAME);
  g_type_ensure (DSPY_TYPE_VIEW);
}

static void
dspy_window_init (DspyWindow *self)
{
  g_autoptr(DspyConnection) session = NULL;
  g_autoptr(DspyConnection) system = NULL;

  self->connections = g_list_store_new (DSPY_TYPE_CONNECTION);

  system = dspy_connection_new_for_bus (G_BUS_TYPE_SYSTEM);
  session = dspy_connection_new_for_bus (G_BUS_TYPE_SESSION);

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
}

GtkWidget *
dspy_window_new (void)
{
  return g_object_new (DSPY_TYPE_WINDOW, NULL);
}
