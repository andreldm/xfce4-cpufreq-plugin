/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
 *  Copyright (c) 2010,2011 Florian Rivoal <frivoal@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#define BORDER 		1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>
#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-configure.h"

#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

static void
check_button_changed (GtkWidget *button, CpuFreqPluginConfigure *configure)
{
	if (button == configure->display_icon)
	{
		cpuFreq->options->show_icon = 
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
		cpufreq_update_icon (cpuFreq);
		return;
	}

	else if (button == configure->display_freq)
		cpuFreq->options->show_label_freq =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

	else if (button == configure->display_governor)
		cpuFreq->options->show_label_governor =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

	else if (button == configure->keep_compact) {
		cpuFreq->options->keep_compact =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
		if (cpuFreq->options->keep_compact)
			cpuFreq->icon_size -= 4;
		else
			cpuFreq->icon_size += 4;
		cpufreq_update_icon (cpuFreq);
	}

	cpufreq_prepare_label (cpuFreq);
	cpufreq_update_plugin ();
}

static void
combo_changed (GtkWidget *combo, CpuFreqPluginConfigure *configure)
{
	guint selected = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

	if (GTK_WIDGET (combo) == configure->combo_cpu)
	{
		cpuFreq->options->show_cpu = selected;
		cpufreq_update_plugin ();
	}
}

static void
spinner_changed (GtkWidget *spinner, CpuFreqPluginConfigure *configure)
{
	cpuFreq->options->timeout =gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	
	cpufreq_restart_timeout ();
}

static void
cpufreq_configure_response (GtkWidget *dialog, int response, CpuFreqPluginConfigure *configure)
{
	g_object_set_data (G_OBJECT (cpuFreq->plugin), "configure", NULL);
	xfce_panel_plugin_unblock_menu (cpuFreq->plugin);
	gtk_widget_destroy (dialog);

	cpufreq_write_config (cpuFreq->plugin);

	g_free (configure);
}

void
cpufreq_configure (XfcePanelPlugin *plugin)
{
	gint i;
	gchar *cpu_name;
	GtkWidget *dialog, *dialog_vbox;
	GtkWidget *frame, *align, *label, *vbox, *hbox;
	GtkWidget *combo, *spinner, *button;
	CpuFreqPluginConfigure *configure;

	configure = g_new0 (CpuFreqPluginConfigure, 1);

	xfce_panel_plugin_block_menu (cpuFreq->plugin);

	dialog = xfce_titled_dialog_new_with_buttons (_("Configure CPU Frequency Monitor"),
		 	 NULL, GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
	xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog), _("Configure the CPU frequency plugin"));

	gtk_window_set_position   (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name  (GTK_WINDOW (dialog), "xfce4-cpufreq-plugin");
	gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
	gtk_window_stick          (GTK_WINDOW (dialog));

	g_object_set_data (G_OBJECT (cpuFreq->plugin), "configure", dialog);

	dialog_vbox = GTK_DIALOG (dialog)->vbox;


	/* monitor behaviours */
	frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	
	label = gtk_label_new (_("<b>Monitor</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

	align = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame), align);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, BORDER*3, 0);

	vbox = gtk_vbox_new (FALSE, BORDER);
	gtk_container_add (GTK_CONTAINER (align), vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
	
	hbox = gtk_hbox_new (FALSE, BORDER);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Timeout Interval:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	spinner = configure->spinner_timeout = 
		gtk_spin_button_new_with_range (TIMEOUT_MIN, TIMEOUT_MAX, TIMEOUT_STEP);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), (gdouble)cpuFreq->options->timeout);
	gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (spinner), "value-changed", G_CALLBACK (spinner_changed), configure);


	/* panel behaviours */
	frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

	label = gtk_label_new (_("<b>Panel</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

	align = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame), align);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, BORDER * 3, 0);

	vbox = gtk_vbox_new (FALSE, BORDER);
	gtk_container_add (GTK_CONTAINER (align), vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);


	/* which cpu to show in panel */
	hbox = gtk_hbox_new (FALSE, BORDER);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Display CPU:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	combo = configure->combo_cpu = gtk_combo_box_new_text ();
	gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);

	for (i = 0; i < cpuFreq->cpus->len; ++i)
	{
		cpu_name = g_strdup_printf ("%d", i);
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), cpu_name);
		g_free (cpu_name);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), cpuFreq->options->show_cpu);
	g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (combo_changed), configure);


	/* check buttons for display widgets in panel */
	button = configure->keep_compact = gtk_check_button_new_with_mnemonic (_("_Keep compact"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), cpuFreq->options->keep_compact);
	g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);

	button = configure->display_icon = gtk_check_button_new_with_mnemonic (_("Show CPU icon"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), cpuFreq->options->show_icon);
	g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);

	button = configure->display_freq = gtk_check_button_new_with_mnemonic (_("Show CPU frequency"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), cpuFreq->options->show_label_freq);
	g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);

	button = configure->display_governor = gtk_check_button_new_with_mnemonic (_("Show CPU governor"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), cpuFreq->options->show_label_governor);
	g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);


	g_signal_connect(G_OBJECT (dialog), "response", G_CALLBACK(cpufreq_configure_response), configure);

	gtk_widget_show_all (dialog);
}
