/* dspy-list-model-filter.c
 *
 * Copyright (C) 2016 Christian Hergert <christian@hergert.me>
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

#include "config.h"

#include "dspy-list-model-filter.h"

typedef struct
{
  GSequenceIter *child_iter;
  GSequenceIter *filter_iter;
} DspyListModelFilterItem;

typedef struct
{
  /* The list we are filtering */
  GListModel *child_model;

  /*
   * Both sequences point to the same DspyListModelFilterItem which
   * contains cross-referencing stable GSequenceIter pointers.
   * The child_seq is considered the "owner" and used to release
   * allocated resources.
   */
  GSequence *child_seq;
  GSequence *filter_seq;

  /*
   * Typical set of callback/closure/free function pointers and data.
   * Called for child items to determine visibility state.
   */
  DspyListModelFilterFunc filter_func;
  gpointer filter_func_data;
  GDestroyNotify filter_func_data_destroy;

  /*
   * If set, we will not emit items-changed. This is useful during
   * invalidation so that we can do a single emission for all items
   * that have changed.
   */
  guint supress_items_changed : 1;
} DspyListModelFilterPrivate;

struct _DspyListModelFilter
{
  GObject parent_instance;
};

static void list_model_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_EXTENDED (DspyListModelFilter, dspy_list_model_filter, G_TYPE_OBJECT, 0,
                        G_ADD_PRIVATE (DspyListModelFilter)
                        G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                               list_model_iface_init))

enum {
  PROP_0,
  PROP_CHILD_MODEL,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];
static guint signal_id;

static void
dspy_list_model_filter_item_free (gpointer data)
{
  DspyListModelFilterItem *item = data;

  g_clear_pointer (&item->filter_iter, g_sequence_remove);
  item->child_iter = NULL;
  g_slice_free (DspyListModelFilterItem, item);
}

static gboolean
dspy_list_model_filter_default_filter_func (GObject  *item,
                                           gpointer  user_data)
{
  return TRUE;
}

/*
 * Locates the next item in the filter sequence starting from
 * the cross-reference found at @iter. If none are found, the
 * end_iter for the filter sequence is returned.
 *
 * This returns an iter in the filter_sequence, not the child_seq.
 *
 * Returns: a #GSequenceIter from the filter sequence.
 */
static GSequenceIter *
find_next_visible_filter_iter (DspyListModelFilter *self,
                               GSequenceIter      *iter)
{
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);

  g_assert (DSPY_IS_LIST_MODEL_FILTER (self));
  g_assert (iter != NULL);

  for (; !g_sequence_iter_is_end (iter); iter = g_sequence_iter_next (iter))
    {
      DspyListModelFilterItem *item = g_sequence_get (iter);

      g_assert (item->child_iter == iter);
      g_assert (item->filter_iter == NULL ||
                g_sequence_iter_get_sequence (item->filter_iter) == priv->filter_seq);

      if (item->filter_iter != NULL)
        return item->filter_iter;
    }

  return g_sequence_get_end_iter (priv->filter_seq);
}

static void
dspy_list_model_filter_child_model_items_changed (DspyListModelFilter *self,
                                                 guint               position,
                                                 guint               n_removed,
                                                 guint               n_added,
                                                 GListModel         *child_model)
{
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);
  gboolean unblocked;

  g_assert (DSPY_IS_LIST_MODEL_FILTER (self));
  g_assert (G_IS_LIST_MODEL (child_model));
  g_assert (priv->child_model == child_model);
  g_assert (position <= (guint)g_sequence_get_length (priv->child_seq));
  g_assert ((g_sequence_get_length (priv->child_seq) - n_removed + n_added) ==
            g_list_model_get_n_items (child_model));

  unblocked = !priv->supress_items_changed;

  if (n_removed > 0)
    {
      GSequenceIter *iter = g_sequence_get_iter_at_pos (priv->child_seq, position);
      gint first_position = -1;
      guint count = 0;

      g_assert (!g_sequence_iter_is_end (iter));

      /* Small shortcut when all items are removed */
      if (n_removed == (guint)g_sequence_get_length (priv->child_seq))
        {
          guint n_filter_removed = g_sequence_get_length (priv->filter_seq);

          g_sequence_remove_range (g_sequence_get_begin_iter (priv->child_seq),
                                   g_sequence_get_end_iter (priv->child_seq));
          g_assert (g_sequence_is_empty (priv->child_seq));
          g_assert (g_sequence_is_empty (priv->filter_seq));

          if (unblocked)
            g_list_model_items_changed (G_LIST_MODEL (self), 0, n_filter_removed, 0);

          goto add_new_items;
        }

      for (guint i = 0; i < n_removed; i++)
        {
          GSequenceIter *to_remove = iter;
          DspyListModelFilterItem *item = g_sequence_get (iter);

          g_assert (item != NULL);
          g_assert (item->child_iter == iter);
          g_assert (item->filter_iter == NULL ||
                    g_sequence_iter_get_sequence (item->filter_iter) == priv->filter_seq);

          /* If this is visible, we need to notify about removal */
          if (unblocked && item->filter_iter != NULL)
            {
              if (first_position < 0)
                first_position = g_sequence_iter_get_position (item->filter_iter);

              count++;
            }

          /* Fetch the next while the iter is still valid */
          iter = g_sequence_iter_next (iter);

          /* Cascades into also removing from filter_seq. */
          g_sequence_remove (to_remove);
        }

      if (unblocked && first_position >= 0)
        g_list_model_items_changed (G_LIST_MODEL (self), first_position, count, 0);
    }

add_new_items:

  if (n_added > 0)
    {
      GSequenceIter *iter = g_sequence_get_iter_at_pos (priv->child_seq, position);
      GSequenceIter *filter_iter = find_next_visible_filter_iter (self, iter);
      guint filter_position = g_sequence_iter_get_position (filter_iter);
      guint count = 0;

      /* Walk backwards to insert items into the filter list so that
       * we can use the same filter_position for each items-changed
       * signal emission.
       */
      for (guint i = position + n_added; i > position; i--)
        {
          g_autoptr(GObject) instance = NULL;
          DspyListModelFilterItem *item;

          item = g_slice_new0 (DspyListModelFilterItem);
          item->filter_iter = NULL;
          item->child_iter = g_sequence_insert_before (iter, item);

          instance = g_list_model_get_item (child_model, i - 1);
          g_assert (G_IS_OBJECT (instance));

          /* Check if this item is visible */
          if (priv->filter_func (instance, priv->filter_func_data))
            {
              item->filter_iter = g_sequence_insert_before (filter_iter, item);

              /* Use this in the future for relative positioning */
              filter_iter = item->filter_iter;

              count++;
            }

          /* Insert next item before this */
          iter = item->child_iter;
        }

      if (unblocked && count)
        g_list_model_items_changed (G_LIST_MODEL (self), filter_position, 0, count);
    }

  g_assert ((guint)g_sequence_get_length (priv->child_seq) ==
            g_list_model_get_n_items (child_model));
}

static void
dspy_list_model_filter_finalize (GObject *object)
{
  DspyListModelFilter *self = (DspyListModelFilter *)object;
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);

  g_clear_pointer (&priv->child_seq, g_sequence_free);
  g_clear_pointer (&priv->filter_seq, g_sequence_free);

  if (priv->filter_func_data_destroy)
    {
      g_clear_pointer (&priv->filter_func_data, priv->filter_func_data_destroy);
      priv->filter_func_data_destroy = NULL;
    }

  g_clear_object (&priv->child_model);

  G_OBJECT_CLASS (dspy_list_model_filter_parent_class)->finalize (object);
}

static void
dspy_list_model_filter_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  DspyListModelFilter *self = DSPY_LIST_MODEL_FILTER (object);

  switch (prop_id)
    {
    case PROP_CHILD_MODEL:
      g_value_set_object (value, dspy_list_model_filter_get_child_model (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_list_model_filter_class_init (DspyListModelFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dspy_list_model_filter_finalize;
  object_class->get_property = dspy_list_model_filter_get_property;

  properties [PROP_CHILD_MODEL] =
    g_param_spec_object ("child-model",
                         "Child Model",
                         "The child model being filtered.",
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signal_id = g_signal_lookup ("items-changed", DSPY_TYPE_LIST_MODEL_FILTER);
}

static void
dspy_list_model_filter_init (DspyListModelFilter *self)
{
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);

  priv->filter_func = dspy_list_model_filter_default_filter_func;
  priv->child_seq = g_sequence_new (dspy_list_model_filter_item_free);
  priv->filter_seq = g_sequence_new (NULL);
}

static GType
dspy_list_model_filter_get_item_type (GListModel *model)
{
  DspyListModelFilter *self = (DspyListModelFilter *)model;
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);

  g_assert (DSPY_IS_LIST_MODEL_FILTER (self));

  return g_list_model_get_item_type (priv->child_model);
}

static guint
dspy_list_model_filter_get_n_items (GListModel *model)
{
  DspyListModelFilter *self = (DspyListModelFilter *)model;
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);

  g_assert (DSPY_IS_LIST_MODEL_FILTER (self));
  g_assert (priv->filter_seq != NULL);

  return g_sequence_get_length (priv->filter_seq);
}

static gpointer
dspy_list_model_filter_get_item (GListModel *model,
                                guint       position)
{
  DspyListModelFilter *self = (DspyListModelFilter *)model;
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);
  DspyListModelFilterItem *item;
  GSequenceIter *iter;
  guint child_position;

  g_assert (DSPY_IS_LIST_MODEL_FILTER (self));

  iter = g_sequence_get_iter_at_pos (priv->filter_seq, position);
  if (g_sequence_iter_is_end (iter))
    return NULL;

  item = g_sequence_get (iter);
  g_assert (item != NULL);
  g_assert (item->filter_iter == iter);
  g_assert (item->child_iter != NULL);
  g_assert (g_sequence_iter_get_sequence (item->child_iter) == priv->child_seq);

  child_position = g_sequence_iter_get_position (item->child_iter);

  return g_list_model_get_item (priv->child_model, child_position);
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = dspy_list_model_filter_get_item_type;
  iface->get_n_items = dspy_list_model_filter_get_n_items;
  iface->get_item = dspy_list_model_filter_get_item;
}

DspyListModelFilter *
dspy_list_model_filter_new (GListModel *child_model)
{
  DspyListModelFilter *ret;
  DspyListModelFilterPrivate *priv;

  g_return_val_if_fail (G_IS_LIST_MODEL (child_model), NULL);

  ret = g_object_new (DSPY_TYPE_LIST_MODEL_FILTER, NULL);
  priv = dspy_list_model_filter_get_instance_private (ret);
  priv->child_model = g_object_ref (child_model);

  g_signal_connect_object (child_model,
                           "items-changed",
                           G_CALLBACK (dspy_list_model_filter_child_model_items_changed),
                           ret,
                           G_CONNECT_SWAPPED);

  dspy_list_model_filter_invalidate (ret);

  return ret;
}

/**
 * dspy_list_model_filter_get_child_model:
 * @self: A #DspyListModelFilter
 *
 * Gets the child model that is being filtered.
 *
 * Returns: (transfer none): A #GListModel.
 */
GListModel *
dspy_list_model_filter_get_child_model (DspyListModelFilter *self)
{
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);

  g_return_val_if_fail (DSPY_IS_LIST_MODEL_FILTER (self), NULL);

  return priv->child_model;
}

void
dspy_list_model_filter_invalidate (DspyListModelFilter *self)
{
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);
  guint n_items;

  g_return_if_fail (DSPY_IS_LIST_MODEL_FILTER (self));

  /* We block emission while in invalidate so that we can use
   * a single larger items-changed rather lots of small emissions.
   */
  priv->supress_items_changed = TRUE;

  /* First determine how many items we need to synthesize as a removal */
  n_items = g_sequence_get_length (priv->filter_seq);

  /*
   * If we have a child store, we want to rebuild our list of items
   * from scratch, so just remove everything.
   */
  if (!g_sequence_is_empty (priv->child_seq))
    g_sequence_remove_range (g_sequence_get_begin_iter (priv->child_seq),
                             g_sequence_get_end_iter (priv->child_seq));

  g_assert (g_sequence_is_empty (priv->child_seq));
  g_assert (g_sequence_is_empty (priv->filter_seq));
  g_assert (!priv->child_model || G_IS_LIST_MODEL (priv->child_model));

  /*
   * Now add the new items by synthesizing the addition of all the
   * itmes in the list.
   */
  if (priv->child_model != NULL)
    {
      guint child_n_items;

      /*
       * Now add all the items as one shot to our list so that
       * we get populate our sequence and filter sequence.
       */
      child_n_items = g_list_model_get_n_items (priv->child_model);
      dspy_list_model_filter_child_model_items_changed (self, 0, 0, child_n_items, priv->child_model);

      g_assert ((guint)g_sequence_get_length (priv->child_seq) == child_n_items);
      g_assert ((guint)g_sequence_get_length (priv->filter_seq) <= child_n_items);
    }

  priv->supress_items_changed = FALSE;

  /* Now that we've updated our sequences, notify of all the changes
   * as a single series of updates to the consumers.
   */
  if (n_items > 0 || !g_sequence_is_empty (priv->filter_seq))
    g_list_model_items_changed (G_LIST_MODEL (self),
                                0,
                                n_items,
                                g_sequence_get_length (priv->filter_seq));
}

void
dspy_list_model_filter_set_filter_func (DspyListModelFilter     *self,
                                       DspyListModelFilterFunc  filter_func,
                                       gpointer                filter_func_data,
                                       GDestroyNotify          filter_func_data_destroy)
{
  DspyListModelFilterPrivate *priv = dspy_list_model_filter_get_instance_private (self);

  g_return_if_fail (DSPY_IS_LIST_MODEL_FILTER (self));
  g_return_if_fail (filter_func || (!filter_func_data && !filter_func_data_destroy));

  if (priv->filter_func_data_destroy != NULL)
    g_clear_pointer (&priv->filter_func_data, priv->filter_func_data_destroy);

  if (filter_func != NULL)
    {
      priv->filter_func = filter_func;
      priv->filter_func_data = filter_func_data;
      priv->filter_func_data_destroy = filter_func_data_destroy;
    }
  else
    {
      priv->filter_func = dspy_list_model_filter_default_filter_func;
      priv->filter_func_data = NULL;
      priv->filter_func_data_destroy = NULL;
    }

  dspy_list_model_filter_invalidate (self);
}
