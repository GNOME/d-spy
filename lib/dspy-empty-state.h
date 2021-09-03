/* dspy-empty-state.h
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
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
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DSPY_TYPE_EMPTY_STATE (dspy_empty_state_get_type())

G_DECLARE_DERIVABLE_TYPE (DspyEmptyState, dspy_empty_state, DSPY, EMPTY_STATE, GtkBin)

struct _DspyEmptyStateClass
{
  GtkBinClass parent_class;
};

GtkWidget    *dspy_empty_state_new          (void);
const gchar  *dspy_empty_state_get_icon_name (DspyEmptyState *self);
void          dspy_empty_state_set_icon_name (DspyEmptyState *self,
                                             const gchar   *icon_name);
void          dspy_empty_state_set_resource  (DspyEmptyState *self,
                                             const gchar   *resource);
const gchar  *dspy_empty_state_get_title     (DspyEmptyState *self);
void          dspy_empty_state_set_title     (DspyEmptyState *self,
                                             const gchar   *title);
const gchar  *dspy_empty_state_get_subtitle  (DspyEmptyState *self);
void          dspy_empty_state_set_subtitle  (DspyEmptyState *self,
                                             const gchar   *title);

G_END_DECLS
