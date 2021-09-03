/* dspy-simple-popover.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DSPY_TYPE_SIMPLE_POPOVER (dspy_simple_popover_get_type())

G_DECLARE_DERIVABLE_TYPE (DspySimplePopover, dspy_simple_popover, DSPY, SIMPLE_POPOVER, GtkPopover)

struct _DspySimplePopoverClass
{
  GtkPopoverClass parent;

  /**
   * DspySimplePopover::activate:
   * @self: The #DspySimplePopover instance.
   * @text: The text at the time of activation.
   *
   * This signal is emitted when the popover's forward button is activated.
   * Connect to this signal to perform your forward progress.
   */
  void (*activate) (DspySimplePopover *self,
                    const gchar      *text);

  /**
   * DspySimplePopover::insert-text:
   * @self: A #DspySimplePopover.
   * @position: the position in UTF-8 characters.
   * @chars: the NULL terminated UTF-8 text to insert.
   * @n_chars: the number of UTF-8 characters in chars.
   *
   * Use this signal to determine if text should be allowed to be inserted
   * into the text buffer. Return GDK_EVENT_STOP to prevent the text from
   * being inserted.
   */
  gboolean (*insert_text) (DspySimplePopover *self,
                           guint             position,
                           const gchar      *chars,
                           guint             n_chars);


  /**
   * DspySimplePopover::changed:
   * @self: A #DspySimplePopover.
   *
   * This signal is emitted when the entry text changes.
   */
  void (*changed) (DspySimplePopover *self);
};

GtkWidget   *dspy_simple_popover_new             (void);
const gchar *dspy_simple_popover_get_text        (DspySimplePopover *self);
void         dspy_simple_popover_set_text        (DspySimplePopover *self,
                                                 const gchar     *text);
const gchar *dspy_simple_popover_get_message     (DspySimplePopover *self);
void         dspy_simple_popover_set_message     (DspySimplePopover *self,
                                                 const gchar     *message);
const gchar *dspy_simple_popover_get_title       (DspySimplePopover *self);
void         dspy_simple_popover_set_title       (DspySimplePopover *self,
                                                 const gchar     *title);
const gchar *dspy_simple_popover_get_button_text (DspySimplePopover *self);
void         dspy_simple_popover_set_button_text (DspySimplePopover *self,
                                                 const gchar     *button_text);
gboolean     dspy_simple_popover_get_ready       (DspySimplePopover *self);
void         dspy_simple_popover_set_ready       (DspySimplePopover *self,
                                                 gboolean         ready);

G_END_DECLS
