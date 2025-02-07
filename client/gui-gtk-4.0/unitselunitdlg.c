/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"

/* common */
#include "game.h"
#include "movement.h"
#include "unit.h"

/* client */
#include "control.h"
#include "tilespec.h"

/* client/gui-gtk-4.0 */
#include "gui_main.h"
#include "gui_stuff.h"
#include "sprite.h"
#include "unitselect.h"

#include "unitselunitdlg.h"

struct unit_sel_unit_cb_data {
  GtkWidget *dlg;
  int tp_id;
};

/************************************************************************//**
  Callback to handle toggling of one of the target unit buttons.
****************************************************************************/
static void unit_sel_unit_toggled(GtkToggleButton *tb, gpointer userdata)
{
  struct unit_sel_unit_cb_data *cbdata
          = (struct unit_sel_unit_cb_data *)userdata;

  if (gtk_toggle_button_get_active(tb)) {
    g_object_set_data(G_OBJECT(cbdata->dlg), "target",
                      GINT_TO_POINTER(cbdata->tp_id));
  }
}

/************************************************************************//**
  Callback to handle destruction of one of the target unit buttons.
****************************************************************************/
static void unit_sel_unit_destroyed(GtkWidget *radio, gpointer userdata)
{
  free(userdata);
}

/************************************************************************//**
  Create a dialog where a unit select what other unit to act on.
****************************************************************************/
bool select_tgt_unit(struct unit *actor, struct tile *ptile,
                     struct unit_list *potential_tgt_units,
                     struct unit *suggested_tgt_unit,
                     const gchar *dlg_title,
                     const gchar *actor_label,
                     const gchar *tgt_label,
                     const gchar *do_label,
                     GCallback do_callback)
{
  GtkWidget *dlg;
  GtkWidget *main_box;
  GtkWidget *box;
  GtkWidget *icon;
  GtkWidget *lbl;
  GtkWidget *sep;
  GtkWidget *radio;
  GtkWidget *default_option = NULL;
  GtkWidget *first_option = NULL;
  struct sprite *spr;
  const struct unit_type *actor_type = unit_type_get(actor);
  int tcount;
  const struct unit *default_unit = NULL;
  int main_row = 0;

  dlg = gtk_dialog_new_with_buttons(dlg_title, NULL, 0,
                                    _("Close"), GTK_RESPONSE_NO,
                                    do_label, GTK_RESPONSE_YES,
                                    NULL);
  setup_dialog(dlg, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_NO);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dlg), TRUE);

  main_box = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(main_box),
                                 GTK_ORIENTATION_VERTICAL);
  box = gtk_grid_new();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(box),
                                 GTK_ORIENTATION_HORIZONTAL);

  lbl = gtk_label_new(actor_label);
  gtk_grid_attach(GTK_GRID(box), lbl, 0, 0, 1, 1);

  spr = get_unittype_sprite(tileset, actor_type, direction8_invalid());
  if (spr != NULL) {
    icon = gtk_image_new_from_pixbuf(sprite_get_pixbuf(spr));
  } else {
    icon = gtk_image_new();
  }
  gtk_grid_attach(GTK_GRID(box), icon, 1, 0, 1, 1);

  lbl = gtk_label_new(utype_name_translation(actor_type));
  gtk_grid_attach(GTK_GRID(box), lbl, 2, 0, 1, 1);

  gtk_grid_attach(GTK_GRID(main_box), box, 0, main_row++, 1, 1);

  sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach(GTK_GRID(main_box), sep, 0, main_row++, 1, 1);

  lbl = gtk_label_new(tgt_label);
  gtk_grid_attach(GTK_GRID(main_box), lbl, 0, main_row++, 1, 1);

  box = gtk_grid_new();

  tcount = 0;
  unit_list_iterate(potential_tgt_units, ptgt) {
    GdkPixbuf *tubuf;

    struct unit_sel_unit_cb_data *cbdata
            = fc_malloc(sizeof(struct unit_sel_unit_cb_data));

    cbdata->tp_id = ptgt->id;
    cbdata->dlg = dlg;

    radio = gtk_check_button_new();
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio),
                               GTK_CHECK_BUTTON(first_option));
    if (first_option == NULL) {
      first_option = radio;
      default_option = first_option;
      default_unit = ptgt;
    }
    g_signal_connect(radio, "toggled",
                     G_CALLBACK(unit_sel_unit_toggled), cbdata);
    g_signal_connect(radio, "destroy",
                     G_CALLBACK(unit_sel_unit_destroyed), cbdata);
    if (ptgt == suggested_tgt_unit) {
      default_option = radio;
      default_unit = suggested_tgt_unit;
    }
    gtk_grid_attach(GTK_GRID(box), radio, 0, tcount, 1, 1);

    tubuf = usdlg_get_unit_image(ptgt);
    if (tubuf != NULL) {
      icon = gtk_image_new_from_pixbuf(tubuf);
      g_object_unref(tubuf);
    } else {
      icon = gtk_image_new();
    }
    gtk_grid_attach(GTK_GRID(box), icon, 1, tcount, 1, 1);

    lbl = gtk_label_new(usdlg_get_unit_descr(ptgt));
    gtk_grid_attach(GTK_GRID(box), lbl, 2, tcount, 1, 1);

    tcount++;
  } unit_list_iterate_end;
  gtk_grid_attach(GTK_GRID(main_box), box, 0, main_row++, 1, 1);

  fc_assert_ret_val(default_option, FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(default_option), TRUE);

  gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))),
                 main_box);

  g_object_set_data(G_OBJECT(dlg), "actor", GINT_TO_POINTER(actor->id));
  g_object_set_data(G_OBJECT(dlg), "tile", ptile);

  /* This function should never be called so that there would be no unit to select,
   * and where there is unit to select, one of them gets selected as the default. */
  fc_assert(default_unit != NULL);
  if (default_unit != NULL) { /* Compiler still wants this */
    g_object_set_data(G_OBJECT(dlg), "target", GINT_TO_POINTER(default_unit->id));
  }

  g_signal_connect(dlg, "response", do_callback, actor);

  gtk_widget_show(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
  gtk_widget_show(dlg);

  return TRUE;
}
