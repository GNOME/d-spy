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

enum {
  METHOD_ACTIVATED,
  N_SIGNALS
};

static guint signals [N_SIGNALS];

GtkWidget *
dspy_tree_view_new (void)
{
  return g_object_new (DSPY_TYPE_TREE_VIEW, NULL);
}

static void
dspy_tree_view_selection_changed (DspyTreeView     *self,
                                  GtkTreeSelection *selection)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  g_assert (DSPY_IS_TREE_VIEW (self));
  g_assert (GTK_IS_TREE_SELECTION (selection));

  if (gtk_tree_selection_get_selected (selection, &model, &iter) &&
      DSPY_IS_INTROSPECTION_MODEL (model))
    {
      DspyName *name = dspy_introspection_model_get_name (DSPY_INTROSPECTION_MODEL (model));
      g_autoptr(DspyMethodInvocation) invocation = NULL;
      DspyNode *node = iter.user_data;

      g_assert (node != NULL);
      g_assert (DSPY_IS_NODE (node));

      if (node->any.kind == DSPY_NODE_KIND_METHOD)
        {
          invocation = dspy_method_invocation_new ();
          dspy_method_invocation_set_interface (invocation, _dspy_node_get_interface (node));
          dspy_method_invocation_set_method (invocation, node->method.name);

          if (node->method.in_args.length == 0)
            dspy_method_invocation_set_parameters (invocation, g_variant_new ("()"));
        }
      else if (node->any.kind == DSPY_NODE_KIND_PROPERTY)
        {
          const gchar *iface = _dspy_node_get_interface (node);

          invocation = dspy_method_invocation_new ();
          dspy_method_invocation_set_interface (invocation, "org.freedesktop.DBus.Properties");
          dspy_method_invocation_set_method (invocation, "Get");
          dspy_method_invocation_set_signature (invocation, "(ss)");
          dspy_method_invocation_set_reply_signature (invocation, "v");
          dspy_method_invocation_set_parameters (invocation,
                                                 g_variant_new ("(ss)", iface, node->property.name));
        }

      if (invocation != NULL)
        {
          dspy_method_invocation_set_object_path (invocation, _dspy_node_get_object_path (node));
          dspy_method_invocation_set_name (invocation, name);
          g_signal_emit (self, signals [METHOD_ACTIVATED], 0, invocation);
        }
    }

}

static void
dspy_tree_view_row_activated (GtkTreeView       *view,
                              GtkTreePath       *path,
                              GtkTreeViewColumn *column)
{
  g_assert (DSPY_IS_TREE_VIEW (view));
  g_assert (path != NULL);
  g_assert (!column || GTK_IS_TREE_VIEW_COLUMN (column));

  if (gtk_tree_view_row_expanded (view, path))
    gtk_tree_view_collapse_row (view, path);
  else
    gtk_tree_view_expand_row (view, path, FALSE);
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

  tree_view_class->row_activated = dspy_tree_view_row_activated;
  tree_view_class->row_expanded = dspy_tree_view_row_expanded;

  signals [METHOD_ACTIVATED] =
    g_signal_new ("method-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (DspyTreeViewClass, method_activated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, DSPY_TYPE_METHOD_INVOCATION);
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

  g_signal_connect_object (gtk_tree_view_get_selection (GTK_TREE_VIEW (self)),
                           "changed",
                           G_CALLBACK (dspy_tree_view_selection_changed),
                           self,
                           G_CONNECT_SWAPPED);
}
