/* Copyright (C)
* 2015 - John Melton, G0ORX/N6LYT
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

#include <gtk/gtk.h>
#include <semaphore.h>
#include <stdio.h>

#include "band.h"
#include "channel.h"
#include "discovered.h"
#include "new_protocol.h"
#include "radio.h"
#include "version.h"
#include "wdsp.h"

static GtkWidget *parent_window;

static GtkWidget *box;
static GtkWidget *menu;

static void cw_keyer_internal_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_internal=cw_keyer_internal==1?0:1;
  cw_changed();
}

static void cw_keyer_speed_value_changed_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_speed=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  cw_changed();
}

static void cw_breakin_cb(GtkWidget *widget, gpointer data) {
  cw_breakin=cw_breakin==1?0:1;
  cw_changed();
}

static void cw_keyer_hang_time_value_changed_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_hang_time=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  cw_changed();
}

static void cw_keys_reversed_cb(GtkWidget *widget, gpointer data) {
  cw_keys_reversed=cw_keys_reversed==1?0:1;
  cw_changed();
}

static void cw_keyer_mode_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_mode=(int)data;
  cw_changed();
}

static void vfo_divisor_value_changed_cb(GtkWidget *widget, gpointer data) {
  vfo_encoder_divisor=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void panadapter_high_value_changed_cb(GtkWidget *widget, gpointer data) {
  panadapter_high=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void panadapter_low_value_changed_cb(GtkWidget *widget, gpointer data) {
  panadapter_low=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void waterfall_high_value_changed_cb(GtkWidget *widget, gpointer data) {
  waterfall_high=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void waterfall_low_value_changed_cb(GtkWidget *widget, gpointer data) {
  waterfall_low=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void waterfall_automatic_cb(GtkWidget *widget, gpointer data) {
  waterfall_automatic=waterfall_automatic==1?0:1;
}

static void linein_cb(GtkWidget *widget, gpointer data) {
  mic_linein=mic_linein==1?0:1;
}

static void micboost_cb(GtkWidget *widget, gpointer data) {
  mic_boost=mic_boost==1?0:1;
}

static void ptt_cb(GtkWidget *widget, gpointer data) {
  mic_ptt_enabled=mic_ptt_enabled==1?0:1;
}

static void ptt_ring_cb(GtkWidget *widget, gpointer data) {
  mic_ptt_tip_bias_ring=0;
}

static void ptt_tip_cb(GtkWidget *widget, gpointer data) {
  mic_ptt_tip_bias_ring=1;
}

static void bias_cb(GtkWidget *widget, gpointer data) {
  mic_bias_enabled=mic_bias_enabled==1?0:1;
}

static void apollo_cb(GtkWidget *widget, gpointer data);

static void alex_cb(GtkWidget *widget, gpointer data) {
  if(filter_board==ALEX) {
    filter_board=NONE;
  } else if(filter_board==NONE) {
    filter_board=ALEX;
  } else if(filter_board==APOLLO) {
    GtkWidget *w=(GtkWidget *)data;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
    filter_board=ALEX;
  }

  if(protocol==NEW_PROTOCOL) {
    filter_board_changed();
  }
}

static void apollo_cb(GtkWidget *widget, gpointer data) {
  if(filter_board==APOLLO) {
    filter_board=NONE;
  } else if(filter_board==NONE) {
    filter_board=APOLLO;
  } else if(filter_board==ALEX) {
    GtkWidget *w=(GtkWidget *)data;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
    filter_board=APOLLO;
  }
  if(protocol==NEW_PROTOCOL) {
    filter_board_changed();
  }
}

static void apollo_tuner_cb(GtkWidget *widget, gpointer data) {
  apollo_tuner=apollo_tuner==1?0:1;
  if(protocol==NEW_PROTOCOL) {
    tuner_changed();
  }
}

static void pa_cb(GtkWidget *widget, gpointer data) {
  pa=pa==1?0:1;
  if(protocol==NEW_PROTOCOL) {
    pa_changed();
  }
}


static void tx_out_of_band_cb(GtkWidget *widget, gpointer data) {
  tx_out_of_band=tx_out_of_band==1?0:1;
}

static void pa_value_changed_cb(GtkWidget *widget, gpointer data) {
  BAND *band=(BAND *)data;
  band->pa_calibration=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static gboolean exit_pressed_event_cb (GtkWidget *widget,
               GdkEventButton *event,
               gpointer        data)
{
  gpio_close();
  if(protocol==ORIGINAL_PROTOCOL) {
    old_protocol_stop();
  } else {
    new_protocol_stop();
  }
  _exit(0);
}

static void oc_rx_cb(GtkWidget *widget, gpointer data) {
  int b=((int)data)>>4;
  int oc=((int)data)&0xF;
  BAND *band=band_get_band(b);
  int mask=0x01<<oc;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    band->OCrx|=mask;
  } else {
    band->OCrx&=~mask;
  }
}

static void oc_tx_cb(GtkWidget *widget, gpointer data) {
  int b=((int)data)>>4;
  int oc=((int)data)&0xF;
  BAND *band=band_get_band(b);
  int mask=0x01<<oc;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    band->OCtx|=mask;
  } else {
    band->OCtx&=~mask;
  }
}

static void detector_mode_cb(GtkWidget *widget, gpointer data) {
  display_detector_mode=(int)data;
  SetDisplayDetectorMode(CHANNEL_RX0, 0, display_detector_mode);
}

static void average_mode_cb(GtkWidget *widget, gpointer data) {
  display_average_mode=(int)data;
  SetDisplayAverageMode(CHANNEL_RX0, 0, display_average_mode);
}

static void time_value_changed_cb(GtkWidget *widget, gpointer data) {
  display_average_time=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  calculate_display_average();
  SetDisplayAvBackmult(CHANNEL_RX0, 0, display_avb);
  SetDisplayNumAverage(CHANNEL_RX0, 0, display_average);

}

static void filled_cb(GtkWidget *widget, gpointer data) {
  display_filled=display_filled==1?0:1;
}


static gboolean menu_pressed_event_cb (GtkWidget *widget,
               GdkEventButton *event,
               gpointer        data)
{
  int i, j, id;

  GtkWidget *dialog=gtk_dialog_new_with_buttons("Menu",GTK_WINDOW(parent_window),GTK_DIALOG_DESTROY_WITH_PARENT,NULL,NULL);

  GtkWidget *content=gtk_dialog_get_content_area(GTK_DIALOG(dialog));



  GtkWidget *notebook=gtk_notebook_new();


  GtkWidget *display_label=gtk_label_new("Display");
  GtkWidget *display_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(display_grid),TRUE);

  GtkWidget *filled_b=gtk_check_button_new_with_label("Filled");
  //gtk_widget_override_font(filled_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filled_b), display_filled);
  gtk_widget_show(filled_b);
  gtk_grid_attach(GTK_GRID(display_grid),filled_b,0,0,1,1);
  g_signal_connect(filled_b,"toggled",G_CALLBACK(filled_cb),NULL);

  GtkWidget *panadapter_high_label=gtk_label_new("Panadapter High: ");
  //gtk_widget_override_font(panadapter_high_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(panadapter_high_label);
  gtk_grid_attach(GTK_GRID(display_grid),panadapter_high_label,0,1,1,1);

  GtkWidget *panadapter_high_r=gtk_spin_button_new_with_range(-220.0,100.0,1.0);
  //gtk_widget_override_font(panadapter_high_r, pango_font_description_from_string("Arial 18"));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(panadapter_high_r),(double)panadapter_high);
  gtk_widget_show(panadapter_high_r);
  gtk_grid_attach(GTK_GRID(display_grid),panadapter_high_r,1,1,1,1);
  g_signal_connect(panadapter_high_r,"value_changed",G_CALLBACK(panadapter_high_value_changed_cb),NULL);

  GtkWidget *panadapter_low_label=gtk_label_new("Panadapter Low: ");
  //gtk_widget_override_font(panadapter_low_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(panadapter_low_label);
  gtk_grid_attach(GTK_GRID(display_grid),panadapter_low_label,0,2,1,1);

  GtkWidget *panadapter_low_r=gtk_spin_button_new_with_range(-220.0,100.0,1.0);
  //gtk_widget_override_font(panadapter_low_r, pango_font_description_from_string("Arial 18"));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(panadapter_low_r),(double)panadapter_low);
  gtk_widget_show(panadapter_low_r);
  gtk_grid_attach(GTK_GRID(display_grid),panadapter_low_r,1,2,1,1);
  g_signal_connect(panadapter_low_r,"value_changed",G_CALLBACK(panadapter_low_value_changed_cb),NULL);

  GtkWidget *waterfall_automatic_label=gtk_label_new("Waterfall Automatic: ");
  //gtk_widget_override_font(waterfall_automatic_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(waterfall_automatic_label);
  gtk_grid_attach(GTK_GRID(display_grid),waterfall_automatic_label,0,3,1,1);

  GtkWidget *waterfall_automatic_b=gtk_check_button_new();
  //gtk_widget_override_font(waterfall_automatic_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (waterfall_automatic_b), waterfall_automatic);
  gtk_widget_show(waterfall_automatic_b);
  gtk_grid_attach(GTK_GRID(display_grid),waterfall_automatic_b,1,3,1,1);
  g_signal_connect(waterfall_automatic_b,"toggled",G_CALLBACK(waterfall_automatic_cb),NULL);

  GtkWidget *waterfall_high_label=gtk_label_new("Waterfall High: ");
  //gtk_widget_override_font(waterfall_high_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(waterfall_high_label);
  gtk_grid_attach(GTK_GRID(display_grid),waterfall_high_label,0,4,1,1);

  GtkWidget *waterfall_high_r=gtk_spin_button_new_with_range(-220.0,100.0,1.0);
  //gtk_widget_override_font(waterfall_high_r, pango_font_description_from_string("Arial 18"));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(waterfall_high_r),(double)waterfall_high);
  gtk_widget_show(waterfall_high_r);
  gtk_grid_attach(GTK_GRID(display_grid),waterfall_high_r,1,4,1,1);
  g_signal_connect(waterfall_high_r,"value_changed",G_CALLBACK(waterfall_high_value_changed_cb),NULL);

  GtkWidget *waterfall_low_label=gtk_label_new("Waterfall Low: ");
  //gtk_widget_override_font(waterfall_low_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(waterfall_low_label);
  gtk_grid_attach(GTK_GRID(display_grid),waterfall_low_label,0,5,1,1);

  GtkWidget *waterfall_low_r=gtk_spin_button_new_with_range(-220.0,100.0,1.0);
  //gtk_widget_override_font(waterfall_low_r, pango_font_description_from_string("Arial 18"));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(waterfall_low_r),(double)waterfall_low);
  gtk_widget_show(waterfall_low_r);
  gtk_grid_attach(GTK_GRID(display_grid),waterfall_low_r,1,5,1,1);
  g_signal_connect(waterfall_low_r,"value_changed",G_CALLBACK(waterfall_low_value_changed_cb),NULL);

  GtkWidget *detector_mode_label=gtk_label_new("Detector: ");
  //gtk_widget_override_font(detector_mode_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(detector_mode_label);
  gtk_grid_attach(GTK_GRID(display_grid),detector_mode_label,3,0,1,1);

  GtkWidget *detector_mode_peak=gtk_radio_button_new_with_label(NULL,"Peak");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (detector_mode_peak), display_detector_mode==DETECTOR_MODE_PEAK);
  gtk_widget_show(detector_mode_peak);
  gtk_grid_attach(GTK_GRID(display_grid),detector_mode_peak,4,0,1,1);
  g_signal_connect(detector_mode_peak,"pressed",G_CALLBACK(detector_mode_cb),(gpointer *)DETECTOR_MODE_PEAK);

  GtkWidget *detector_mode_rosenfell=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(detector_mode_peak),"Rosenfell");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (detector_mode_rosenfell), display_detector_mode==DETECTOR_MODE_ROSENFELL);
  gtk_widget_show(detector_mode_rosenfell);
  gtk_grid_attach(GTK_GRID(display_grid),detector_mode_rosenfell,4,1,1,1);
  g_signal_connect(detector_mode_rosenfell,"pressed",G_CALLBACK(detector_mode_cb),(gpointer *)DETECTOR_MODE_ROSENFELL);

  GtkWidget *detector_mode_average=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(detector_mode_rosenfell),"Average");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (detector_mode_average), display_detector_mode==DETECTOR_MODE_AVERAGE);
  gtk_widget_show(detector_mode_average);
  gtk_grid_attach(GTK_GRID(display_grid),detector_mode_average,4,2,1,1);
  g_signal_connect(detector_mode_average,"pressed",G_CALLBACK(detector_mode_cb),(gpointer *)DETECTOR_MODE_AVERAGE);

  GtkWidget *detector_mode_sample=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(detector_mode_average),"Sample");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (detector_mode_sample), display_detector_mode==DETECTOR_MODE_SAMPLE);
  gtk_widget_show(detector_mode_sample);
  gtk_grid_attach(GTK_GRID(display_grid),detector_mode_sample,4,3,1,1);
  g_signal_connect(detector_mode_sample,"pressed",G_CALLBACK(detector_mode_cb),(gpointer *)DETECTOR_MODE_SAMPLE);


  GtkWidget *average_mode_label=gtk_label_new("Averaging: ");
  //gtk_widget_override_font(average_mode_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(average_mode_label);
  gtk_grid_attach(GTK_GRID(display_grid),average_mode_label,3,4,1,1);

  GtkWidget *average_mode_none=gtk_radio_button_new_with_label(NULL,"None");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (average_mode_none), display_detector_mode==AVERAGE_MODE_NONE);
  gtk_widget_show(average_mode_none);
  gtk_grid_attach(GTK_GRID(display_grid),average_mode_none,4,4,1,1);
  g_signal_connect(average_mode_none,"pressed",G_CALLBACK(average_mode_cb),(gpointer *)AVERAGE_MODE_NONE);

  GtkWidget *average_mode_recursive=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(average_mode_none),"Recursive");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (average_mode_recursive), display_average_mode==AVERAGE_MODE_RECURSIVE);
  gtk_widget_show(average_mode_recursive);
  gtk_grid_attach(GTK_GRID(display_grid),average_mode_recursive,4,5,1,1);
  g_signal_connect(average_mode_recursive,"pressed",G_CALLBACK(average_mode_cb),(gpointer *)AVERAGE_MODE_RECURSIVE);

  GtkWidget *average_mode_time_window=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(average_mode_recursive),"Time Window");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (average_mode_time_window), display_average_mode==AVERAGE_MODE_TIME_WINDOW);
  gtk_widget_show(average_mode_time_window);
  gtk_grid_attach(GTK_GRID(display_grid),average_mode_time_window,4,6,1,1);
  g_signal_connect(average_mode_time_window,"pressed",G_CALLBACK(average_mode_cb),(gpointer *)AVERAGE_MODE_TIME_WINDOW);

  GtkWidget *average_mode_log_recursive=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(average_mode_time_window),"Log Recursive");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (average_mode_log_recursive), display_average_mode==AVERAGE_MODE_LOG_RECURSIVE);
  gtk_widget_show(average_mode_log_recursive);
  gtk_grid_attach(GTK_GRID(display_grid),average_mode_log_recursive,4,7,1,1);
  g_signal_connect(average_mode_log_recursive,"pressed",G_CALLBACK(average_mode_cb),(gpointer *)AVERAGE_MODE_LOG_RECURSIVE);


  GtkWidget *time_label=gtk_label_new("Time (ms): ");
  //gtk_widget_override_font(average_mode_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(time_label);
  gtk_grid_attach(GTK_GRID(display_grid),time_label,3,8,1,1);

  GtkWidget *time_r=gtk_spin_button_new_with_range(1.0,9999.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(time_r),(double)display_average_time);
  gtk_widget_show(time_r);
  gtk_grid_attach(GTK_GRID(display_grid),time_r,4,8,1,1);
  g_signal_connect(time_r,"value_changed",G_CALLBACK(time_value_changed_cb),NULL);

  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),display_grid,display_label);



  GtkWidget *tx_label=gtk_label_new("TX");
  GtkWidget *tx_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(tx_grid),TRUE);
  gtk_grid_set_column_spacing (GTK_GRID(tx_grid),10);

  for(i=0;i<HAM_BANDS;i++) {
    BAND *band=band_get_band(i);

    GtkWidget *band_label=gtk_label_new(band->title);
    //gtk_widget_override_font(band_label, pango_font_description_from_string("Arial 18"));
    gtk_widget_show(band_label);
    gtk_grid_attach(GTK_GRID(tx_grid),band_label,(i/6)*2,i%6,1,1);

    GtkWidget *pa_r=gtk_spin_button_new_with_range(0.0,100.0,1.0);
    //gtk_widget_override_font(pa_r, pango_font_description_from_string("Arial 18"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pa_r),(double)band->pa_calibration);
    gtk_widget_show(pa_r);
    gtk_grid_attach(GTK_GRID(tx_grid),pa_r,((i/6)*2)+1,i%6,1,1);
    g_signal_connect(pa_r,"value_changed",G_CALLBACK(pa_value_changed_cb),band);
  }

  GtkWidget *tx_out_of_band_b=gtk_check_button_new_with_label("Transmit out of band");
  //gtk_widget_override_font(tx_out_of_band_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tx_out_of_band_b), tx_out_of_band);
  gtk_widget_show(tx_out_of_band_b);
  gtk_grid_attach(GTK_GRID(tx_grid),tx_out_of_band_b,0,7,4,1);
  g_signal_connect(tx_out_of_band_b,"toggled",G_CALLBACK(tx_out_of_band_cb),NULL);

  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),tx_grid,tx_label);

  GtkWidget *cw_label=gtk_label_new("CW");
  GtkWidget *cw_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(cw_grid),TRUE);

  GtkWidget *cw_keyer_internal_b=gtk_check_button_new_with_label("CW Internal - Speed (WPM)");
  //gtk_widget_override_font(cw_keyer_internal_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_keyer_internal_b), cw_keyer_internal);
  gtk_widget_show(cw_keyer_internal_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_internal_b,0,0,1,1);
  g_signal_connect(cw_keyer_internal_b,"toggled",G_CALLBACK(cw_keyer_internal_cb),NULL);

  GtkWidget *cw_keyer_speed_b=gtk_spin_button_new_with_range(1.0,60.0,1.0);
  //gtk_widget_override_font(cw_keyer_speed_b, pango_font_description_from_string("Arial 18"));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_speed_b),(double)cw_keyer_speed);
  gtk_widget_show(cw_keyer_speed_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_speed_b,1,0,1,1);
  g_signal_connect(cw_keyer_speed_b,"value_changed",G_CALLBACK(cw_keyer_speed_value_changed_cb),NULL);

  GtkWidget *cw_breakin_b=gtk_check_button_new_with_label("CW Break In - Delay (ms)");
  //gtk_widget_override_font(cw_breakin_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_breakin_b), cw_breakin);
  gtk_widget_show(cw_breakin_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_breakin_b,0,1,1,1);
  g_signal_connect(cw_breakin_b,"toggled",G_CALLBACK(cw_breakin_cb),NULL);

  GtkWidget *cw_keyer_hang_time_b=gtk_spin_button_new_with_range(0.0,1000.0,1.0);
  //gtk_widget_override_font(cw_keyer_hang_time_b, pango_font_description_from_string("Arial 18"));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_hang_time_b),(double)cw_keyer_hang_time);
  gtk_widget_show(cw_keyer_hang_time_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_hang_time_b,1,1,1,1);
  g_signal_connect(cw_keyer_hang_time_b,"value_changed",G_CALLBACK(cw_keyer_hang_time_value_changed_cb),NULL);
 
  GtkWidget *cw_keyer_straight=gtk_radio_button_new_with_label(NULL,"CW KEYER STRAIGHT");
  //gtk_widget_override_font(cw_keyer_straight, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_keyer_straight), cw_keyer_mode==KEYER_STRAIGHT);
  gtk_widget_show(cw_keyer_straight);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_straight,0,2,1,1);
  g_signal_connect(cw_keyer_straight,"pressed",G_CALLBACK(cw_keyer_mode_cb),(gpointer *)KEYER_STRAIGHT);

  GtkWidget *cw_keyer_mode_a=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cw_keyer_straight),"CW KEYER MODE A");
  //gtk_widget_override_font(cw_keyer_mode_a, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_keyer_mode_a), cw_keyer_mode==KEYER_MODE_A);
  gtk_widget_show(cw_keyer_mode_a);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_mode_a,0,3,1,1);
  g_signal_connect(cw_keyer_mode_a,"pressed",G_CALLBACK(cw_keyer_mode_cb),(gpointer *)KEYER_MODE_A);

  GtkWidget *cw_keyer_mode_b=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cw_keyer_mode_a),"CW KEYER MODE B");
  //gtk_widget_override_font(cw_keyer_mode_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_keyer_mode_b), cw_keyer_mode==KEYER_MODE_B);
  gtk_widget_show(cw_keyer_mode_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_mode_b,0,4,1,1);
  g_signal_connect(cw_keyer_mode_b,"pressed",G_CALLBACK(cw_keyer_mode_cb),(gpointer *)KEYER_MODE_B);

  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),cw_grid,cw_label);


  GtkWidget *encoder_label=gtk_label_new("VFO Encoder");
  GtkWidget *encoder_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(encoder_grid),TRUE);

  GtkWidget *vfo_divisor_label=gtk_label_new("Divisor: ");
  //gtk_widget_override_font(vfo_divisor_label, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(vfo_divisor_label);
  gtk_grid_attach(GTK_GRID(encoder_grid),vfo_divisor_label,0,0,1,1);

  GtkWidget *vfo_divisor=gtk_spin_button_new_with_range(1.0,60.0,1.0);
  //gtk_widget_override_font(vfo_divisor, pango_font_description_from_string("Arial 18"));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(vfo_divisor),(double)vfo_encoder_divisor);
  gtk_widget_show(vfo_divisor);
  gtk_grid_attach(GTK_GRID(encoder_grid),vfo_divisor,1,0,1,1);
  g_signal_connect(vfo_divisor,"value_changed",G_CALLBACK(vfo_divisor_value_changed_cb),NULL);

  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),encoder_grid,encoder_label);

  GtkWidget *microphone_label=gtk_label_new("Microphone");
  GtkWidget *microphone_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(microphone_grid),TRUE);

  GtkWidget *linein_b=gtk_check_button_new_with_label("Line In");
  //gtk_widget_override_font(linein_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (linein_b), mic_linein);
  gtk_widget_show(linein_b);
  gtk_grid_attach(GTK_GRID(microphone_grid),linein_b,1,1,1,1);
  g_signal_connect(linein_b,"toggled",G_CALLBACK(linein_cb),NULL);

  GtkWidget *micboost_b=gtk_check_button_new_with_label("Boost");
  //gtk_widget_override_font(micboost_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (micboost_b), mic_boost);
  gtk_widget_show(micboost_b);
  gtk_grid_attach(GTK_GRID(microphone_grid),micboost_b,1,2,1,1);
  g_signal_connect(micboost_b,"toggled",G_CALLBACK(micboost_cb),NULL);


  if((protocol==NEW_PROTOCOL && device==NEW_DEVICE_ORION) || (protocol==ORIGINAL_PROTOCOL && device==DEVICE_ORION)) {

    GtkWidget *ptt_ring_b=gtk_radio_button_new_with_label(NULL,"PTT On Ring, Mic and Bias on Tip");
    //gtk_widget_override_font(ptt_ring_b, pango_font_description_from_string("Arial 18"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ptt_ring_b), mic_ptt_tip_bias_ring==0);
    gtk_widget_show(ptt_ring_b);
    gtk_grid_attach(GTK_GRID(microphone_grid),ptt_ring_b,1,3,1,1);
    g_signal_connect(ptt_ring_b,"pressed",G_CALLBACK(ptt_ring_cb),NULL);

    GtkWidget *ptt_tip_b=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ptt_ring_b),"PTT On Tip, Mic and Bias on Ring");
    //gtk_widget_override_font(ptt_tip_b, pango_font_description_from_string("Arial 18"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ptt_tip_b), mic_ptt_tip_bias_ring==1);
    gtk_widget_show(ptt_tip_b);
    gtk_grid_attach(GTK_GRID(microphone_grid),ptt_tip_b,1,4,1,1);
    g_signal_connect(ptt_tip_b,"pressed",G_CALLBACK(ptt_tip_cb),NULL);

    GtkWidget *ptt_b=gtk_check_button_new_with_label("PTT Enabled");
    //gtk_widget_override_font(ptt_b, pango_font_description_from_string("Arial 18"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ptt_b), mic_ptt_enabled);
    gtk_widget_show(ptt_b);
    gtk_grid_attach(GTK_GRID(microphone_grid),ptt_b,1,5,1,1);
    g_signal_connect(ptt_b,"toggled",G_CALLBACK(ptt_cb),NULL);

    GtkWidget *bias_b=gtk_check_button_new_with_label("BIAS Enabled");
    //gtk_widget_override_font(bias_b, pango_font_description_from_string("Arial 18"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bias_b), mic_bias_enabled);
    gtk_widget_show(bias_b);
    gtk_grid_attach(GTK_GRID(microphone_grid),bias_b,1,6,1,1);
    g_signal_connect(bias_b,"toggled",G_CALLBACK(bias_cb),NULL);
  }

  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),microphone_grid,microphone_label);

  GtkWidget *filters_label=gtk_label_new("Filters/PA");
  GtkWidget *filters_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(filters_grid),TRUE);

  GtkWidget *alex_b=gtk_check_button_new_with_label("ALEX");
  //gtk_widget_override_font(alex_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (alex_b), filter_board==ALEX);
  gtk_widget_show(alex_b);
  gtk_grid_attach(GTK_GRID(filters_grid),alex_b,2,1,1,1);

  GtkWidget *apollo_b=gtk_check_button_new_with_label("APOLLO");
  //gtk_widget_override_font(apollo_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (apollo_b), filter_board==APOLLO);
  gtk_widget_show(apollo_b);
  gtk_grid_attach(GTK_GRID(filters_grid),apollo_b,2,2,1,1);

  g_signal_connect(alex_b,"toggled",G_CALLBACK(alex_cb),apollo_b);
  g_signal_connect(apollo_b,"toggled",G_CALLBACK(apollo_cb),alex_b);

  GtkWidget *apollo_tuner_b=gtk_check_button_new_with_label("Auto Tuner");
  //gtk_widget_override_font(apollo_tuner_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (apollo_tuner_b), apollo_tuner);
  gtk_widget_show(apollo_tuner_b);
  gtk_grid_attach(GTK_GRID(filters_grid),apollo_tuner_b,2,3,1,1);
  g_signal_connect(apollo_tuner_b,"toggled",G_CALLBACK(apollo_tuner_cb),NULL);

  GtkWidget *pa_b=gtk_check_button_new_with_label("PA");
  //gtk_widget_override_font(pa_b, pango_font_description_from_string("Arial 18"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pa_b), pa);
  gtk_widget_show(pa_b);
  gtk_grid_attach(GTK_GRID(filters_grid),pa_b,2,4,1,1);
  g_signal_connect(pa_b,"toggled",G_CALLBACK(pa_cb),NULL);

  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),filters_grid,filters_label);

  GtkWidget *oc_label=gtk_label_new("OC");
  GtkWidget *oc_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(oc_grid),TRUE);
  gtk_grid_set_column_spacing (GTK_GRID(oc_grid),15);

  GtkWidget *band_title=gtk_label_new("Band");
  //gtk_widget_override_font(band_title, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(band_title);
  gtk_grid_attach(GTK_GRID(oc_grid),band_title,0,0,1,1);

  GtkWidget *rx_title=gtk_label_new("Rx");
  //gtk_widget_override_font(rx_title, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(rx_title);
  gtk_grid_attach(GTK_GRID(oc_grid),rx_title,4,0,1,1);

  GtkWidget *tx_title=gtk_label_new("Tx");
  //gtk_widget_override_font(tx_title, pango_font_description_from_string("Arial 18"));
  gtk_widget_show(tx_title);
  gtk_grid_attach(GTK_GRID(oc_grid),tx_title,11,0,1,1);

  for(i=1;i<8;i++) {
    char oc_id[8];
    sprintf(oc_id,"%d",i);
    GtkWidget *oc_rx_title=gtk_label_new(oc_id);
    //gtk_widget_override_font(oc_rx_title, pango_font_description_from_string("Arial 18"));
    gtk_widget_show(oc_rx_title);
    gtk_grid_attach(GTK_GRID(oc_grid),oc_rx_title,i,1,1,1);
    GtkWidget *oc_tx_title=gtk_label_new(oc_id);
    //gtk_widget_override_font(oc_tx_title, pango_font_description_from_string("Arial 18"));
    gtk_widget_show(oc_tx_title);
    gtk_grid_attach(GTK_GRID(oc_grid),oc_tx_title,i+7,1,1,1);
  }

  for(i=0;i<HAM_BANDS;i++) {
    BAND *band=band_get_band(i);

    GtkWidget *band_label=gtk_label_new(band->title);
    //gtk_widget_override_font(band_label, pango_font_description_from_string("Arial 18"));
    gtk_widget_show(band_label);
    gtk_grid_attach(GTK_GRID(oc_grid),band_label,0,i+2,1,1);

    int mask;
    for(j=1;j<8;j++) {
      mask=0x01<<j;
      GtkWidget *oc_rx_b=gtk_check_button_new();
      //gtk_widget_override_font(oc_rx_b, pango_font_description_from_string("Arial 18"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (oc_rx_b), (band->OCrx&mask)==mask);
      gtk_widget_show(oc_rx_b);
      gtk_grid_attach(GTK_GRID(oc_grid),oc_rx_b,j,i+2,1,1);
      g_signal_connect(oc_rx_b,"toggled",G_CALLBACK(oc_rx_cb),(gpointer)(j+(i<<4)));

      GtkWidget *oc_tx_b=gtk_check_button_new();
      //gtk_widget_override_font(oc_tx_b, pango_font_description_from_string("Arial 18"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (oc_tx_b), (band->OCtx&mask)==mask);
      gtk_widget_show(oc_tx_b);
      gtk_grid_attach(GTK_GRID(oc_grid),oc_tx_b,j+7,i+2,1,1);
      g_signal_connect(oc_tx_b,"toggled",G_CALLBACK(oc_tx_cb),(gpointer)(j+(i<<4)));
    }
  }


  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),oc_grid,oc_label);


  GtkWidget *exit_label=gtk_label_new("Exit");
  GtkWidget *exit_grid=gtk_grid_new();
  //gtk_grid_set_row_homogeneous(GTK_GRID(exit_grid),TRUE);

  GtkWidget *exit_button=gtk_button_new_with_label("Exit PiHPSDR");
  g_signal_connect (exit_button, "pressed", G_CALLBACK(exit_pressed_event_cb), NULL);
  gtk_grid_attach(GTK_GRID(exit_grid),exit_button,0,0,1,1);

  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),exit_grid,exit_label);



/*
  GtkWidget *about_label=gtk_label_new("About");
  GtkWidget *about_grid=gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(about_grid),TRUE);

  char build[64];

  sprintf(build,"build: %s %s",build_date, build_time);
  GtkWidget *pi_label=gtk_label_new("pihpsdr by John Melton g0orx/n6lyt");
  gtk_widget_show(pi_label);
  gtk_grid_attach(GTK_GRID(about_grid),pi_label,0,0,1,1);
  GtkWidget *filler_label=gtk_label_new("");
  gtk_widget_show(filler_label);
  gtk_grid_attach(GTK_GRID(about_grid),filler_label,0,1,1,1);
  GtkWidget *build_date_label=gtk_label_new(build);
  gtk_widget_show(build_date_label);
  gtk_grid_attach(GTK_GRID(about_grid),build_date_label,0,2,1,1);
   
  id=gtk_notebook_append_page(GTK_NOTEBOOK(notebook),about_grid,about_label);
*/
  gtk_container_add(GTK_CONTAINER(content),notebook);

  GtkWidget *close_button=gtk_dialog_add_button(GTK_DIALOG(dialog),"Close Dialog",GTK_RESPONSE_OK);
 // gtk_widget_override_font(close_button, pango_font_description_from_string("Arial 18"));

  gtk_widget_show_all(dialog);

  int result=gtk_dialog_run(GTK_DIALOG(dialog));

  radioSaveState();

  gtk_widget_destroy (dialog);

  return TRUE;

}

GtkWidget* menu_init(int width,int height,GtkWidget *parent) {

  GdkRGBA black;
  black.red=0.0;
  black.green=0.0;
  black.blue=0.0;
  black.alpha=0.0;

  fprintf(stderr,"menu_init: width=%d height=%d\n",width,height);

  parent_window=parent;

  box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
  gtk_widget_set_size_request (box, width, height);
  gtk_widget_override_background_color(box, GTK_STATE_NORMAL, &black);

  menu=gtk_button_new_with_label("Menu");
 // gtk_widget_set_size_request (menu, width, height);
  g_signal_connect (menu, "pressed", G_CALLBACK(menu_pressed_event_cb), NULL);
  gtk_box_pack_start (GTK_BOX(box),menu,TRUE,TRUE,0);

  gtk_widget_show_all(box);

  return box;
}
