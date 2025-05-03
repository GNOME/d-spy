/*
 * dspy-titled-model.c
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

#include "dspy-introspectable.h"
#include "dspy-titled-model.h"

struct _DspyTitledModel
{
  GObject parent_instance;
  GListModel *model;
  char *title;
};

enum {
  PROP_0,
  PROP_MODEL,
  PROP_TITLE,
  N_PROPS
};

static GType
dspy_titled_model_get_item_type (GListModel *model)
{
  return DSPY_TYPE_INTROSPECTABLE;
}

static guint
dspy_titled_model_get_n_items (GListModel *model)
{
  return g_list_model_get_n_items (DSPY_TITLED_MODEL (model)->model);
}

static gpointer
dspy_titled_model_get_item (GListModel *model,
                            guint       position)
{
  return g_list_model_get_item (DSPY_TITLED_MODEL (model)->model, position);
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = dspy_titled_model_get_item_type;
  iface->get_n_items = dspy_titled_model_get_n_items;
  iface->get_item = dspy_titled_model_get_item;
}

G_DEFINE_FINAL_TYPE_WITH_CODE (DspyTitledModel, dspy_titled_model, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static GParamSpec *properties[N_PROPS];

static void
dspy_titled_model_dispose (GObject *object)
{
  DspyTitledModel *self = (DspyTitledModel *)object;

  g_clear_object (&self->model);
  g_clear_pointer (&self->title, g_free);

  G_OBJECT_CLASS (dspy_titled_model_parent_class)->dispose (object);
}

static void
dspy_titled_model_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  DspyTitledModel *self = DSPY_TITLED_MODEL (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;

    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_titled_model_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  DspyTitledModel *self = DSPY_TITLED_MODEL (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      if ((self->model = g_value_dup_object (value)))
        g_signal_connect_object (self->model,
                                 "items-changed",
                                 G_CALLBACK (g_list_model_items_changed),
                                 self,
                                 G_CONNECT_SWAPPED);
      break;

    case PROP_TITLE:
      self->title = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_titled_model_class_init (DspyTitledModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dspy_titled_model_dispose;
  object_class->get_property = dspy_titled_model_get_property;
  object_class->set_property = dspy_titled_model_set_property;

  properties[PROP_MODEL] =
    g_param_spec_object ("model", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_titled_model_init (DspyTitledModel *self)
{
}

GListModel *
dspy_titled_model_new (GListModel *model,
                       const char *title)
{
  g_return_val_if_fail (G_IS_LIST_MODEL (model), NULL);
  g_return_val_if_fail (title != NULL, NULL);

  return g_object_new (DSPY_TYPE_TITLED_MODEL,
                       "model", model,
                       "title", title,
                       NULL);
}

const char *
dspy_titled_model_get_title (DspyTitledModel *self)
{
  g_return_val_if_fail (DSPY_IS_TITLED_MODEL (self), NULL);

  return self->title;
}
