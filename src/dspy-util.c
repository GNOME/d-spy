/*
 * dspy-util.c
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

#include "dspy-util.h"

static void
_g_subprocess_communicate_utf8_cb (GObject      *object,
                                   GAsyncResult *result,
                                   gpointer      user_data)
{
  GSubprocess *subprocess = (GSubprocess *)object;
  g_autoptr(DexPromise) promise = user_data;
  g_autoptr(GError) error = NULL;
  g_autofree char *stdout_buf = NULL;

  g_assert (G_IS_SUBPROCESS (subprocess));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (DEX_IS_PROMISE (promise));

  if (!g_subprocess_communicate_utf8_finish (subprocess, result, &stdout_buf, NULL, &error))
    dex_promise_reject (promise, g_steal_pointer (&error));
  else
    dex_promise_resolve_string (promise, g_steal_pointer (&stdout_buf));
}

static DexFuture *
_g_subprocess_communicate_utf8 (GSubprocess *subprocess,
                                const char  *stdin_buf)
{
  DexPromise *promise;

  dex_return_error_if_fail (G_IS_SUBPROCESS (subprocess));

  promise = dex_promise_new_cancellable ();
  g_subprocess_communicate_utf8_async (subprocess,
                                       stdin_buf,
                                       dex_promise_get_cancellable (promise),
                                       _g_subprocess_communicate_utf8_cb,
                                       dex_ref (promise));
  return DEX_FUTURE (promise);
}

static DexFuture *
parse_a11y_result (DexFuture *completed,
                   gpointer   user_data)
{
  g_autofree char *stdout_buf = NULL;
  g_autoptr(GVariant) variant = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree char *a11y_bus = NULL;

  stdout_buf = dex_await_string (dex_ref (completed), NULL);

  if (!(variant = g_variant_parse (G_VARIANT_TYPE ("(s)"), stdout_buf, NULL, NULL, &error)))
    return dex_future_new_for_error (g_steal_pointer (&error));

  g_variant_take_ref (variant);
  g_variant_get (variant, "(s)", &a11y_bus, NULL);

  return dex_future_new_take_string (g_steal_pointer (&a11y_bus));
}

DexFuture *
dspy_get_a11y_bus (void)
{
  static DexFuture *future;

  if (future == NULL)
    {
      g_autoptr(GSubprocessLauncher) launcher = NULL;
      g_autoptr(GSubprocess) subprocess = NULL;
      g_autoptr(GPtrArray) argv = NULL;
      g_autoptr(GError) error = NULL;

      launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE);
      argv = g_ptr_array_new_null_terminated (0, NULL, TRUE);

      if (g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS))
        {
          g_ptr_array_add (argv, (char *)"flatpak-spawn");
          g_ptr_array_add (argv, (char *)"--host");
          g_ptr_array_add (argv, (char *)"--watch-bus");
        }

      g_ptr_array_add (argv, (char *)"gdbus");
      g_ptr_array_add (argv, (char *)"call");
      g_ptr_array_add (argv, (char *)"--session");
      g_ptr_array_add (argv, (char *)"--dest=org.a11y.Bus");
      g_ptr_array_add (argv, (char *)"--object-path=/org/a11y/bus");
      g_ptr_array_add (argv, (char *)"--method=org.a11y.Bus.GetAddress");

      if (!(subprocess = g_subprocess_launcher_spawnv (launcher, (const char * const *)argv->pdata, &error)))
        future = dex_future_new_for_error (g_steal_pointer (&error));
      else
        future = dex_future_then (_g_subprocess_communicate_utf8 (subprocess, NULL),
                                  parse_a11y_result,
                                  NULL,
                                  NULL);
    }

  return dex_ref (future);
}

gboolean
dspy_parse_a11y_bus (const char  *address,
                     char       **unix_path,
                     char       **address_suffix)
{
  const char *skip;
  const char *a11y_bus_suffix;
  char *a11y_bus_path;

  g_return_val_if_fail (address != NULL, FALSE);
  g_return_val_if_fail (unix_path != NULL, FALSE);
  g_return_val_if_fail (address_suffix != NULL, FALSE);

  *unix_path = NULL;
  *address_suffix = NULL;

  if (!g_str_has_prefix (address, "unix:path="))
    return FALSE;

  skip = address + strlen ("unix:path=");

  if ((a11y_bus_suffix = strchr (skip, ',')))
    a11y_bus_path = g_strndup (skip, a11y_bus_suffix - skip);
  else
    a11y_bus_path = g_strdup (skip);

  *unix_path = g_steal_pointer (&a11y_bus_path);
  *address_suffix = g_strdup (a11y_bus_suffix);

  return TRUE;
}
