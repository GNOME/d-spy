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
#include <libdex.h>

#include "build-ident.h"
#include "dspy-util.h"
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

static int
on_command_line (GApplication            *app,
                 GApplicationCommandLine *command_line,
                 gpointer                 user_data)
{
  GVariantDict *options;
  gboolean new_window = FALSE;

  g_assert (G_IS_APPLICATION (app));
  g_assert (G_IS_APPLICATION_COMMAND_LINE (command_line));

  options = g_application_command_line_get_options_dict (command_line);

  if (g_variant_dict_lookup (options, "new-window", "b", &new_window) && new_window)
    g_action_group_activate_action (G_ACTION_GROUP (app), "new-window", NULL);
  else
    g_application_activate (app);

  return EXIT_SUCCESS;
}

static int
on_handle_local_options (GApplication *app,
                         GVariantDict *options,
                         gpointer      user_data)
{
  gboolean version = FALSE;

  g_assert (G_IS_APPLICATION (app));

  if (g_variant_dict_lookup (options, "version", "b", &version) && version)
    {
      g_print ("%s - %s\n", _("D-Spy"), PACKAGE_VERSION);
      return 0;
    }
  return -1;
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

static const GOptionEntry options[] = {
    {"new-window", 'w', 0, G_OPTION_ARG_NONE, NULL, N_("Open a new window"), NULL},
    {"version", 'v', 0, G_OPTION_ARG_NONE, NULL, N_("Print version information and exit"), NULL},
    {NULL}
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

  dex_init ();

  /* Discover a11y bus early so it is ready when we are */
  dex_future_disown (dspy_get_a11y_bus ());

  app = g_object_new (ADW_TYPE_APPLICATION,
                      "application-id", APP_ID,
                      "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                      "resource-base-path", "/org/gnome/dspy",
                      NULL);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  g_signal_connect (app, "command-line", G_CALLBACK (on_command_line), NULL);
  g_signal_connect (app, "handle-local-options", G_CALLBACK (on_handle_local_options), NULL);
  g_action_map_add_action_entries (G_ACTION_MAP (app), actions, G_N_ELEMENTS (actions), app);
  g_application_add_main_option_entries (G_APPLICATION (app), options);
  ret = g_application_run (G_APPLICATION (app), argc, argv);

  return ret;
}
