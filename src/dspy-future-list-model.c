/*
 * dspy-future-list-model.c
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

#include "config.h"

#include "dspy-future-list-model.h"

struct _DspyFutureListModel
{
  GObject     parent_instance;
  DexFuture  *future;
  GListModel *model;
};

static GType
dspy_future_list_model_get_item_type (GListModel *model)
{
  return G_TYPE_OBJECT;
}

static guint
dspy_future_list_model_get_n_items (GListModel *model)
{
  if (DSPY_FUTURE_LIST_MODEL (model)->model)
    return g_list_model_get_n_items (DSPY_FUTURE_LIST_MODEL (model)->model);
  return 0;
}

static gpointer
dspy_future_list_model_get_item (GListModel *model,
                                 guint       position)
{
  if (DSPY_FUTURE_LIST_MODEL (model)->model)
    return g_list_model_get_item (DSPY_FUTURE_LIST_MODEL (model)->model, position);
  return NULL;
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item = dspy_future_list_model_get_item;
  iface->get_item_type = dspy_future_list_model_get_item_type;
  iface->get_n_items = dspy_future_list_model_get_n_items;
}

G_DEFINE_FINAL_TYPE_WITH_CODE (DspyFutureListModel, dspy_future_list_model, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static void
dspy_future_list_model_finalize (GObject *object)
{
  DspyFutureListModel *self = (DspyFutureListModel *)object;

  g_clear_object (&self->model);
  dex_clear (&self->future);

  G_OBJECT_CLASS (dspy_future_list_model_parent_class)->finalize (object);
}

static void
dspy_future_list_model_class_init (DspyFutureListModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dspy_future_list_model_finalize;
}

static void
dspy_future_list_model_init (DspyFutureListModel *self)
{
}

static DexFuture *
dspy_future_list_model_handle_future (DexFuture *future,
                                      gpointer   user_data)
{
  DspyFutureListModel *self = user_data;
  g_autoptr(GObject) object = NULL;

  g_assert (DEX_IS_FUTURE (future));
  g_assert (dex_future_is_resolved (future));
  g_assert (DSPY_IS_FUTURE_LIST_MODEL (self));

  if ((object = dex_await_object (dex_ref (future), NULL)) && G_IS_LIST_MODEL (object))
    {
      guint n_items;

      self->model = g_object_ref (G_LIST_MODEL (object));

      g_signal_connect_object (self->model,
                               "items-changed",
                               G_CALLBACK (g_list_model_items_changed),
                               self,
                               G_CONNECT_SWAPPED);

      if ((n_items = g_list_model_get_n_items (self->model)))
        g_list_model_items_changed (self->model, 0, 0, n_items);
    }

  return dex_future_new_true ();
}

/**
 * dspy_future_list_model_new:
 * @future: (transfer full): a [class@Dspy.Future] that resolves to a [iface@Gio.ListModel]
 *
 * Creates a new [iface@Gio.ListModel] that will populate with the contents of
 * @future after it resolves to a [iface@Gio.ListModel].
 *
 * Returns: (transfer full): a [iface@Gio.ListModel]
 */
GListModel *
dspy_future_list_model_new (DexFuture *future)
{
  DspyFutureListModel *self;

  g_return_val_if_fail (DEX_IS_FUTURE (future), NULL);

  self = g_object_new (DSPY_TYPE_FUTURE_LIST_MODEL, NULL);
  self->future = future;

  dex_future_disown (dex_future_then (dex_ref (self->future),
                                      dspy_future_list_model_handle_future,
                                      g_object_ref (self),
                                      g_object_unref));

  return G_LIST_MODEL (self);
}
