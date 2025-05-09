/*
 * dspy-introspectable.h
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

#include <gio/gio.h>

#include "dspy-types.h"

G_BEGIN_DECLS

#define DSPY_TYPE_INTROSPECTABLE (dspy_introspectable_get_type())

DSPY_DECLARE_INTERNAL_TYPE (DspyIntrospectable, dspy_introspectable, DSPY, INTROSPECTABLE, GObject)

struct _DspyIntrospectable
{
  GObject             parent_instance;
  GList               link;
  DspyIntrospectable *parent;
};

struct _DspyIntrospectableClass
{
  GObjectClass parent_class;

  char *(*dup_title)       (DspyIntrospectable *self);
  char *(*dup_short_title) (DspyIntrospectable *self);
};

DspyIntrospectable *dspy_introspectable_get_parent      (DspyIntrospectable *self);
char               *dspy_introspectable_dup_title       (DspyIntrospectable *self);
char               *dspy_introspectable_dup_short_title (DspyIntrospectable *self);

/**
 * dspy_introspectable_append_queue:
 * @self: a [class@Dspy.Introspectable]
 * @queue: the queue
 * @child: (transfer full):
 *
 */
static inline void
dspy_introspectable_append_queue (DspyIntrospectable *self,
                                  GQueue             *queue,
                                  DspyIntrospectable *child)
{
  g_assert (DSPY_IS_INTROSPECTABLE (self));
  g_assert (queue != NULL);
  g_assert (DSPY_IS_INTROSPECTABLE (child));
  g_assert (child->parent == NULL);
  g_assert (child->link.data == child);
  g_assert (child->link.prev == NULL);
  g_assert (child->link.next == NULL);

  g_queue_push_tail_link (queue, &child->link);
  child->parent = self;
}

/**
 * dspy_introspectable_prepend_queue:
 * @self: a [class@Dspy.Introspectable]
 * @queue: the queue
 * @child: (transfer full):
 *
 */
static inline void
dspy_introspectable_prepend_queue (DspyIntrospectable *self,
                                   GQueue             *queue,
                                   DspyIntrospectable *child)
{
  g_assert (DSPY_IS_INTROSPECTABLE (self));
  g_assert (queue != NULL);
  g_assert (DSPY_IS_INTROSPECTABLE (child));
  g_assert (child->parent == NULL);
  g_assert (child->link.data == child);
  g_assert (child->link.prev == NULL);
  g_assert (child->link.next == NULL);

  g_queue_push_head_link (queue, &child->link);
  child->parent = self;
}

static inline void
dspy_introspectable_clear_queue (DspyIntrospectable *self,
                                 GQueue             *queue)
{
  g_assert (DSPY_IS_INTROSPECTABLE (self));
  g_assert (queue != NULL);

  while (queue->length > 0)
    {
      DspyIntrospectable *child = g_queue_peek_head (queue);

      g_assert (DSPY_IS_INTROSPECTABLE (child));
      g_assert (child->link.data == child);
      g_assert (child->parent == (gpointer)self);

      g_queue_unlink (queue, &child->link);
      child->parent = NULL;
      g_object_unref (child);
    }
}

static inline GListModel *
dspy_introspectable_queue_to_list (DspyIntrospectable *self,
                                   GQueue             *queue)
{
  GListStore *store = g_list_store_new (DSPY_TYPE_INTROSPECTABLE);

  for (const GList *iter = queue->head; iter; iter = iter->next)
    g_list_store_append (store, iter->data);

  return G_LIST_MODEL (store);
}

G_END_DECLS
