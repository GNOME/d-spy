/*
 * dspy-node.h
 *
 * Copyright 2025 Christian Hergert <chergert@redhat.com>
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

#pragma once

#include "dspy-introspectable.h"

G_BEGIN_DECLS

#define DSPY_TYPE_NODE (dspy_node_get_type())

G_DECLARE_FINAL_TYPE (DspyNode, dspy_node, DSPY, NODE, DspyIntrospectable)

struct _DspyNode
{
  DspyIntrospectable  parent_instance;
  char               *path;
  GQueue              nodes;
  GQueue              interfaces;
};

GListModel *dspy_node_list_interfaces (DspyNode *self);

G_END_DECLS
