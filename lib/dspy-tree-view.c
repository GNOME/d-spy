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

#include "dspy-private.h"
#include "dspy-tree-view.h"

G_DEFINE_TYPE (DspyTreeView, dspy_tree_view, GTK_TYPE_TREE_VIEW)

GtkWidget *
dspy_tree_view_new (void)
{
  return g_object_new (DSPY_TYPE_TREE_VIEW, NULL);
}

static void
dspy_tree_view_row_expanded (GtkTreeView *view,
                             GtkTreeIter *iter,
                             GtkTreePath *path)
{
  DspyNode *node = NULL;
  GtkTreeModel *model;

  g_assert (GTK_IS_TREE_VIEW (view));
  g_assert (iter != NULL);
  g_assert (path != NULL);

  if (GTK_TREE_VIEW_CLASS (dspy_tree_view_parent_class)->row_expanded)
    GTK_TREE_VIEW_CLASS (dspy_tree_view_parent_class)->row_expanded (view, iter, path);

  if (!(model = gtk_tree_view_get_model (view)) ||
      !DSPY_IS_INTROSPECTION_MODEL (model))
    return;

  node = iter->user_data;

  g_assert (node != NULL);
  g_assert (DSPY_IS_NODE (node));

  /* Expand children too if there are fixed number of children */
  if (node->any.kind == DSPY_NODE_KIND_NODE ||    /* path node */
      node->any.kind == DSPY_NODE_KIND_INTERFACE) /* iface node */
    {
      GtkTreeIter child;

      if (gtk_tree_model_iter_children (model, &child, iter))
        {
          g_autoptr(GtkTreePath) copy = gtk_tree_path_copy (path);

          gtk_tree_path_down (copy);

          do
            {
              gtk_tree_view_expand_row (view, copy, FALSE);
              gtk_tree_path_next (copy);
            }
          while (gtk_tree_model_iter_next (model, &child));
        }
    }
}

static void
dspy_tree_view_class_init (DspyTreeViewClass *klass)
{
  GtkTreeViewClass *tree_view_class = GTK_TREE_VIEW_CLASS (klass);

  tree_view_class->row_expanded = dspy_tree_view_row_expanded;
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
