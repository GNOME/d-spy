/* main.c
 *
 * Copyright 2019 Christian Hergert
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

#include <glib/gi18n.h>

#include <adwaita.h>
#include <libpanel.h>

#include "build-ident.h"
#include "dspy-window.h"

static const gchar *authors[] = {
  "Christian Hergert",
  NULL
};

static const gchar *artists[] = {
  "Jakub Steiner",
  NULL
};

static void
on_activate (GtkApplication *app)
{
  GtkWindow *window;

  g_assert (GTK_IS_APPLICATION (app));

  window = gtk_application_get_active_window (app);
  if (window == NULL)
    window = g_object_new (DSPY_TYPE_WINDOW,
                           "application", app,
                           "default-width", 1000,
                           "default-height", 700,
                           "icon-name", PACKAGE_ICON_NAME,
                           NULL);

  gtk_window_present (window);
}

static void
new_window_cb (GSimpleAction *action,
               GVariant      *param,
               gpointer       user_data)
{
  GtkApplication *app = user_data;
  GtkWindow *window;

  g_assert (GTK_IS_APPLICATION (app));

  window = g_object_new (DSPY_TYPE_WINDOW,
                         "application", app,
                         "default-width", 1000,
                         "default-height", 700,
                         "icon-name", PACKAGE_ICON_NAME,
                         NULL);
  gtk_window_present (window);
}

static void
about_action_cb (GSimpleAction *action,
                 GVariant      *param,
                 gpointer       user_data)
{
  GtkApplication *app = user_data;
  GtkWindow *window;
  AdwDialog *dialog;

  g_assert (GTK_IS_APPLICATION (app));

  dialog = adw_about_dialog_new ();
  adw_about_dialog_set_application_name (ADW_ABOUT_DIALOG(dialog), _("D-Spy"));
  adw_about_dialog_set_application_icon (ADW_ABOUT_DIALOG(dialog), PACKAGE_ICON_NAME);
  adw_about_dialog_set_developers (ADW_ABOUT_DIALOG(dialog), authors);
  adw_about_dialog_set_designers (ADW_ABOUT_DIALOG(dialog), artists);
#if DEVELOPMENT_BUILD
  adw_about_dialog_set_version (ADW_ABOUT_DIALOG(dialog), PACKAGE_VERSION " (" DSPY_BUILD_IDENTIFIER ")");
#else
  adw_about_dialog_set_version (ADW_ABOUT_DIALOG(dialog), PACKAGE_VERSION);
#endif
  adw_about_dialog_set_copyright (ADW_ABOUT_DIALOG(dialog), "© 2019-2024 Christian Hergert");
  adw_about_dialog_set_license_type (ADW_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_3_0);
  adw_about_dialog_set_website (ADW_ABOUT_DIALOG(dialog), PACKAGE_WEBSITE);
  adw_about_dialog_set_issue_url (ADW_ABOUT_DIALOG(dialog), "https://gitlab.gnome.org/GNOME/d-spy/-/issues/new");

  window = gtk_application_get_active_window (app);

  adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET(window));
}

static const GActionEntry actions[] = {
  { "about", about_action_cb },
  { "new-window", new_window_cb },
};

int
main (int   argc,
      char *argv[])
{
  g_autoptr(GtkApplication) app = NULL;
  int ret;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init ();
  adw_init ();
  panel_init ();

  app = g_object_new (ADW_TYPE_APPLICATION,
                      "application-id", APP_ID,
                      "flags", G_APPLICATION_DEFAULT_FLAGS,
                      "resource-base-path", "/org/gnome/dspy",
                      NULL);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  g_action_map_add_action_entries (G_ACTION_MAP (app), actions, G_N_ELEMENTS (actions), app);
  ret = g_application_run (G_APPLICATION (app), argc, argv);

  return ret;
}
