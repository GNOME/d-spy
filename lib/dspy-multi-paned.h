/* dspy-multi-paned.h
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DSPY_TYPE_MULTI_PANED (dspy_multi_paned_get_type())

G_DECLARE_DERIVABLE_TYPE (DspyMultiPaned, dspy_multi_paned, DSPY, MULTI_PANED, GtkContainer)

struct _DspyMultiPanedClass
{
  GtkContainerClass parent;

  void (*resize_drag_begin) (DspyMultiPaned *self,
                             GtkWidget     *child);
  void (*resize_drag_end)   (DspyMultiPaned *self,
                             GtkWidget     *child);
};

GtkWidget *dspy_multi_paned_new            (void);
guint      dspy_multi_paned_get_n_children (DspyMultiPaned *self);
GtkWidget *dspy_multi_paned_get_nth_child  (DspyMultiPaned *self,
                                            guint          nth);
GtkWidget *dspy_multi_paned_get_at_point   (DspyMultiPaned *self,
                                            gint           x,
                                            gint           y);

G_END_DECLS
