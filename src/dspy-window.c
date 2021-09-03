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

#include "dspy-window.h"

struct _DspyWindow
{
  GtkApplicationWindow  parent_instance;
  DspyView             *view;
};

G_DEFINE_TYPE (DspyWindow, dspy_window, GTK_TYPE_APPLICATION_WINDOW)

static void
dspy_window_class_init (DspyWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-window.ui");
  gtk_widget_class_bind_template_child (widget_class, DspyWindow, view);

  g_type_ensure (DSPY_TYPE_VIEW);
}

static void
dspy_window_init (DspyWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
dspy_window_new (void)
{
  return g_object_new (DSPY_TYPE_WINDOW, NULL);
}
