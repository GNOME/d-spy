/* dspy-name-marquee.c
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

#include "config.h"

#include "dspy-name-marquee.h"

struct _DspyNameMarquee
{
  AdwPreferencesGroup  parent_instance;
  DspyName            *name;
};

G_DEFINE_FINAL_TYPE (DspyNameMarquee, dspy_name_marquee, ADW_TYPE_PREFERENCES_GROUP)

enum {
  PROP_0,
  PROP_NAME,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

/**
 * dspy_name_marquee_new:
 *
 * Create a new #DspyNameMarquee.
 *
 * Returns: (transfer full): a newly created #DspyNameMarquee
 */
GtkWidget *
dspy_name_marquee_new (void)
{
  return g_object_new (DSPY_TYPE_NAME_MARQUEE, NULL);
}

static void
dspy_name_marquee_dispose (GObject *object)
{
  DspyNameMarquee *self = (DspyNameMarquee *)object;

  g_clear_object (&self->name);

  G_OBJECT_CLASS (dspy_name_marquee_parent_class)->dispose (object);
}

static void
dspy_name_marquee_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  DspyNameMarquee *self = DSPY_NAME_MARQUEE (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_object (value, dspy_name_marquee_get_name (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_name_marquee_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  DspyNameMarquee *self = DSPY_NAME_MARQUEE (object);

  switch (prop_id)
    {
    case PROP_NAME:
      dspy_name_marquee_set_name (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_name_marquee_class_init (DspyNameMarqueeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dspy_name_marquee_dispose;
  object_class->get_property = dspy_name_marquee_get_property;
  object_class->set_property = dspy_name_marquee_set_property;

  properties [PROP_NAME] =
    g_param_spec_object ("name",
                         "Name",
                         "The DspyName to display on the marquee",
                         DSPY_TYPE_NAME,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dspy/dspy-name-marquee.ui");

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
dspy_name_marquee_init (DspyNameMarquee *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * dspy_name_marquee_get_name:
 *
 * Gets the name on the marquee
 *
 * Returns: (nullable) (transfer none): a #DspyName or %NULL
 */
DspyName *
dspy_name_marquee_get_name (DspyNameMarquee *self)
{
  g_return_val_if_fail (DSPY_IS_NAME_MARQUEE (self), NULL);

  return self->name;
}

void
dspy_name_marquee_set_name (DspyNameMarquee *self,
                            DspyName        *name)
{
  g_return_if_fail (DSPY_IS_NAME_MARQUEE (self));
  g_return_if_fail (!name || DSPY_IS_NAME (name));

  if (g_set_object (&self->name, name))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_NAME]);
}
