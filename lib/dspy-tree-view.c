/* dspy-tree-view.c
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

#define G_LOG_DOMAIN "dspy-tree-view"

#include "config.h"

#include <glib/gi18n.h>

#include "dspy-tree-view.h"

G_DEFINE_TYPE (DspyTreeView, dspy_tree_view, GTK_TYPE_TREE_VIEW)

GtkWidget *
dspy_tree_view_new (void)
{
  return g_object_new (DSPY_TYPE_TREE_VIEW, NULL);
}

static void
dspy_tree_view_class_init (DspyTreeViewClass *klass)
{
}

static void
dspy_tree_view_init (DspyTreeView *self)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *cell;

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self), TRUE);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Object Path"));
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), cell, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (column), cell, "markup", 0);
}
