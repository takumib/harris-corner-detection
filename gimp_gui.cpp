/*
 * Copyright (C) 2008, 2009, 2010 Richard Membarth <richard.membarth@cs.fau.de>
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
 * Additional permission under GNU GPL version 3 section 7
 * 
 * If you modify this Program, or any covered work, by linking or combining it
 * with NVIDIA CUDA Software Development Kit (or a modified version of that
 * library), containing parts covered by the terms of a royalty-free,
 * non-exclusive license, the licensors of this Program grant you additional
 * permission to convey the resulting work.
 */

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimp_main.hpp"
#include "gimp_gui.hpp"
#include "multi_res_cpu.hpp"
#include "defines_cpu.hpp"


gboolean multi_res_dialog (GimpDrawable *drawable) {
    GtkWidget *dialog;
    GtkWidget *main_vbox;
    GtkWidget *preview;
    gboolean   run;
    
    gimp_ui_init("Harris Corner Detector", FALSE);
    
    dialog = gimp_dialog_new("Harris Corner Detector", "Harris Corner Detector",
                              NULL, (GtkDialogFlags) 0,
                              gimp_standard_help_func, "plug-in-multi-res-filter",
                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,
                              NULL);
    
    main_vbox = gtk_vbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG (dialog)->vbox), main_vbox);
    gtk_widget_show(main_vbox);
    
    gtk_widget_show(dialog);
    
    run_multi_filter(drawable, GIMP_PREVIEW(preview));
    
    run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);
    
    gtk_widget_destroy(dialog);
    
    return run;
}

