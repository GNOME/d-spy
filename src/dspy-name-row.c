/* dspy-name-row.c
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

#include "dspy-name-row.h"

struct _DspyNameRow
{
  GtkWidget      parent_instance;

  DspyName      *name;

  GtkLabel      *title;
  GtkLabel      *subtitle;
};

G_DEFINE_FINAL_TYPE (DspyNameRow, dspy_name_row, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_NAME,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
dspy_name_row_update (DspyNameRow *self)
{
  g_autoptr(GString) str = NULL;
  GPid pid;

  g_assert (DSPY_IS_NAME_ROW (self));

  pid = dspy_name_get_pid (self->name);
  str = g_string_new (NULL);

  if (dspy_name_get_activatable (self->name))
    g_string_append_printf (str, _("%s: %s"), _("Activatable"), _("Yes"));
  else
    g_string_append_printf (str, _("%s: %s"), _("Activatable"), _("No"));

  if (pid > -1)
    {
      g_string_append (str, ", ");
      g_string_append_printf (str, _("%s: %u"), _("PID"), pid);
    }

  gtk_label_set_label (self->subtitle, str->str);

  gtk_widget_set_tooltip_text (GTK_WIDGET (self),
                               dspy_name_get_owner (self->name));
}

static void
dspy_name_row_set_name (DspyNameRow *self,
                        DspyName    *name)
{
  g_assert (DSPY_IS_NAME_ROW (self));
  g_assert (!name || DSPY_IS_NAME (name));

  if (self->name == name)
    return;

  if (self->name != NULL)
    g_signal_handlers_disconnect_by_func (self->name,
                                          G_CALLBACK (dspy_name_row_update),
                                          self);

  g_set_object (&self->name, name);

  if (self->name != NULL)
    {
      g_signal_connect_object (self->name,
                               "notify::pid",
                               G_CALLBACK (dspy_name_row_update),
                               self,
                               G_CONNECT_SWAPPED);

      g_signal_connect_object (self->name,
                               "notify::activatable",
                               G_CALLBACK (dspy_name_row_update),
                               self,
                               G_CONNECT_SWAPPED);

      gtk_label_set_label (self->title, dspy_name_get_name (self->name));

      dspy_name_row_update (self);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_NAME]);
    }
}

static void
dspy_name_row_dispose (GObject *object)
{
  DspyNameRow *self = (DspyNameRow *)object;
  GtkWidget *child;

  dspy_name_row_set_name (self, NULL);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  g_clear_object (&self->name);

  G_OBJECT_CLASS (dspy_name_row_parent_class)->dispose (object);
}

static void
dspy_name_row_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  DspyNameRow *self = DSPY_NAME_ROW (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_object (value, self->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_name_row_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  DspyNameRow *self = DSPY_NAME_ROW (object);

  switch (prop_id)
    {
    case PROP_NAME:
      dspy_name_row_set_name (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_name_row_class_init (DspyNameRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dspy_name_row_dispose;
  object_class->get_property = dspy_name_row_get_property;
  object_class->set_property = dspy_name_row_set_property;

  properties [PROP_NAME] =
    g_param_spec_object ("name",
                         "Name",
                         "The DspyName for the row",
                         DSPY_TYPE_NAME,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-name-row.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_bind_template_child (widget_class, DspyNameRow, subtitle);
  gtk_widget_class_bind_template_child (widget_class, DspyNameRow, title);
}

static void
dspy_name_row_init (DspyNameRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
