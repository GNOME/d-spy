/* dspy-connection.h
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

#pragma once

#include <gio/gio.h>

#include "dspy-version-macros.h"

G_BEGIN_DECLS

#define DSPY_TYPE_CONNECTION (dspy_connection_get_type())

DSPY_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (DspyConnection, dspy_connection, DSPY, CONNECTION, GObject)

DSPY_AVAILABLE_IN_ALL
DspyConnection  *dspy_connection_new_for_address   (const gchar          *address);
DSPY_AVAILABLE_IN_ALL
DspyConnection  *dspy_connection_new_for_bus       (GBusType              bus_type);
DSPY_AVAILABLE_IN_ALL
void             dspy_connection_add_error         (DspyConnection       *self,
                                                    const GError         *error);
DSPY_AVAILABLE_IN_ALL
void             dspy_connection_clear_errors      (DspyConnection       *self);
DSPY_AVAILABLE_IN_ALL
GDBusConnection *dspy_connection_get_connection    (DspyConnection       *self);
DSPY_AVAILABLE_IN_ALL
const gchar     *dspy_connection_get_address       (DspyConnection       *self);
DSPY_AVAILABLE_IN_ALL
GBusType         dspy_connection_get_bus_type      (DspyConnection       *self);
DSPY_AVAILABLE_IN_ALL
gboolean         dspy_connection_get_has_error     (DspyConnection       *self);
DSPY_AVAILABLE_IN_ALL
void             dspy_connection_open_async        (DspyConnection       *self,
                                                    GCancellable         *cancellable,
                                                    GAsyncReadyCallback   callback,
                                                    gpointer              user_data);
DSPY_AVAILABLE_IN_ALL
GDBusConnection *dspy_connection_open_finish       (DspyConnection       *self,
                                                    GAsyncResult         *result,
                                                    GError              **error);
DSPY_AVAILABLE_IN_ALL
void             dspy_connection_close             (DspyConnection       *self);
DSPY_AVAILABLE_IN_ALL
void             dspy_connection_list_names_async  (DspyConnection       *self,
                                                    GCancellable         *cancellable,
                                                    GAsyncReadyCallback   callback,
                                                    gpointer              user_data);
DSPY_AVAILABLE_IN_ALL
GListModel      *dspy_connection_list_names_finish (DspyConnection       *self,
                                                    GAsyncResult         *result,
                                                    GError              **error);

G_END_DECLS
