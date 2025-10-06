/*
 * dspy-property.c
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

#include <glib/gi18n.h>

#include "dspy-interface.h"
#include "dspy-node.h"
#include "dspy-private.h"
#include "dspy-property.h"

enum {
  PROP_0,
  PROP_NAME,
  PROP_SIGNATURE,
  PROP_VALUE,
  PROP_FLAGS,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DspyProperty, dspy_property, DSPY_TYPE_INTROSPECTABLE)

static GParamSpec *properties[N_PROPS];

static char *
dspy_property_dup_short_title (DspyIntrospectable *introspectable)
{
  DspyProperty *self = DSPY_PROPERTY (introspectable);

  return g_strdup (self->name);
}

static char *
dspy_property_dup_title (DspyIntrospectable *introspectable)
{
  DspyProperty *self = DSPY_PROPERTY (introspectable);
  GString *str = g_string_new (self->name);
  g_autofree char *sig = _dspy_signature_humanize (self->signature);
  const char *rw = NULL;

  g_string_append_printf (str, " -> <b>%s</b>", sig);

  if (self->flags == (G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE))
    rw = _("read/write");
  else if (self->flags  & G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE)
    rw = _("write-only");
  else if (self->flags  & G_DBUS_PROPERTY_INFO_FLAGS_READABLE)
    rw = _("read-only");

  if (rw != NULL)
    g_string_append_printf (str, " (%s)", rw);

  if (self->value != NULL)
    g_string_append_printf (str, " = %s", self->value);

  return g_string_free (str, FALSE);
}

static void
dspy_property_dispose (GObject *object)
{
  DspyProperty *self = (DspyProperty *)object;

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->signature, g_free);
  g_clear_pointer (&self->value, g_free);

  G_OBJECT_CLASS (dspy_property_parent_class)->dispose (object);
}

static void
dspy_property_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  DspyProperty *self = DSPY_PROPERTY (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_SIGNATURE:
      g_value_set_string (value, self->signature);
      break;

    case PROP_VALUE:
      g_value_set_string (value, self->value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dspy_property_class_init (DspyPropertyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DspyIntrospectableClass *introspectable_class = DSPY_INTROSPECTABLE_CLASS (klass);

  object_class->dispose = dspy_property_dispose;
  object_class->get_property = dspy_property_get_property;

  introspectable_class->dup_title = dspy_property_dup_title;
  introspectable_class->dup_short_title = dspy_property_dup_short_title;

  properties[PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_SIGNATURE] =
    g_param_spec_string ("signature", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_VALUE] =
    g_param_spec_string ("value", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_FLAGS] =
    g_param_spec_flags ("flags", NULL, NULL,
                         G_TYPE_DBUS_PROPERTY_INFO_FLAGS,
                         0,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dspy_property_init (DspyProperty *self)
{
}

static const char *
dspy_property_get_interface (DspyProperty *self)
{
  for (DspyIntrospectable *iter = DSPY_INTROSPECTABLE (self)->parent;
       iter != NULL;
       iter = iter->parent)
    {
      if (DSPY_IS_INTERFACE (iter))
        return DSPY_INTERFACE (iter)->name;
    }

  return NULL;
}

static const char *
dspy_property_get_object_path (DspyProperty *self)
{
  for (DspyIntrospectable *iter = DSPY_INTROSPECTABLE (self)->parent;
       iter != NULL;
       iter = iter->parent)
    {
      if (DSPY_IS_NODE (iter))
        return DSPY_NODE (iter)->path;
    }

  return NULL;
}

static DexFuture *
dspy_property_handle_value (DexFuture *completed,
                            gpointer   user_data)
{
  DspyProperty *self = user_data;
  g_autoptr(GVariant) box = NULL;
  g_autoptr(GVariant) reply = NULL;
  g_autoptr(GVariant) child = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (DEX_IS_FUTURE (completed));
  g_assert (DSPY_IS_PROPERTY (self));

  if (!(reply = dex_await_variant (dex_ref (completed), &error)))
    {
      g_warning ("Failed to fetch property value: %s", error->message);

      if (g_set_str (&self->value, _("Error")))
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);
      g_object_notify (G_OBJECT (self), "title");

      return dex_ref (completed);
    }

  box = g_variant_get_child_value (reply, 0);
  child = g_variant_get_child_value (box, 0);

  g_clear_pointer (&self->value, g_free);

  if (g_variant_is_of_type (child, G_VARIANT_TYPE_STRING) ||
      g_variant_is_of_type (child, G_VARIANT_TYPE_OBJECT_PATH))
    self->value = g_variant_dup_string (child, NULL);
  else if (g_variant_is_of_type (child, G_VARIANT_TYPE_BYTESTRING))
    self->value = g_utf8_make_valid (g_variant_get_bytestring (child), -1);
  else
    self->value = g_variant_print (child, FALSE);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);
  g_object_notify (G_OBJECT (self), "title");

  return dex_future_new_take_string (g_strdup (self->value));
}

/**
 * dspy_property_query_value:
 * @self: a [class@Dspy.Property]
 * @connection: the connection to query with
 * @name: the owner name to query
 *
 * Returns: (transfer full): a [class@Dex.Future] that resolves to
 *   a string or rejects with error.
 */
DexFuture *
dspy_property_query_value (DspyProperty    *self,
                           GDBusConnection *connection,
                           const char      *name)
{
  const char *interface;
  const char *object_path;

  dex_return_error_if_fail (DSPY_IS_PROPERTY (self));
  dex_return_error_if_fail (G_IS_DBUS_CONNECTION (connection));
  dex_return_error_if_fail (name != NULL);

  if (!(self->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE))
    return dex_future_new_reject (G_IO_ERROR,
                                  G_IO_ERROR_INVAL,
                                  "Property is read-only");

  if (!(interface = dspy_property_get_interface (self)))
    return dex_future_new_reject (G_IO_ERROR,
                                  G_IO_ERROR_INVAL,
                                  "Property does not belong to an interface");

  if (!(object_path = dspy_property_get_object_path (self)))
    return dex_future_new_reject (G_IO_ERROR,
                                  G_IO_ERROR_INVAL,
                                  "Property does not belong to an object");

  return dex_future_finally (dex_dbus_connection_call (connection,
                                                       name,
                                                       object_path,
                                                       "org.freedesktop.DBus.Properties",
                                                       "Get",
                                                       g_variant_new ("(ss)", interface, self->name),
                                                       G_VARIANT_TYPE ("(v)"),
                                                       G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
                                                       -1),
                             dspy_property_handle_value,
                             g_object_ref (self),
                             g_object_unref);
}
