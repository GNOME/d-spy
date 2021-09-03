/* dspy-pattern-spec.h
 *
 * Copyright (C) 2015-2017 Christian Hergert <christian@hergert.me>
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

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _DspyPatternSpec DspyPatternSpec;

#define DSPY_TYPE_PATTERN_SPEC (dspy_pattern_spec_get_type())

GType           dspy_pattern_spec_get_type (void);
DspyPatternSpec *dspy_pattern_spec_new      (const gchar    *keywords);
DspyPatternSpec *dspy_pattern_spec_ref      (DspyPatternSpec *self);
void            dspy_pattern_spec_unref    (DspyPatternSpec *self);
gboolean        dspy_pattern_spec_match    (DspyPatternSpec *self,
                                           const gchar     *haystack);
const gchar    *dspy_pattern_spec_get_text (DspyPatternSpec *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DspyPatternSpec, dspy_pattern_spec_unref)

G_END_DECLS
