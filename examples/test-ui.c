#include <gst/gst.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gst/interfaces/xoverlay.h>

#define LIVE_PIPELINE \
  "dvbbasebin frequency=754000000 modulation=\"QAM 16\" trans-mode=8k code-rate-hp=2/3 " \
  "code-rate-lp=1/2 guard=4 bandwidth=8 hierarchy=0 inversion=AUTO " \
  "program-numbers=47 .program_47"
#define FILE_PIPELINE \
  "filesrc location=/home/sebp/Videos/Recordings/ProSieben/2009-8-19_22-25/001.mpeg"

static void change_page (gint pgno);

static GtkWindow *window;
static GtkWidget *video;
static GstElement *teletext;

static gboolean
delete_event (GtkWidget *widget,
              GdkEvent *event,
              gpointer data)
{
    return FALSE;
}

static void
destroy_event (GtkWidget *widget,
         gpointer data)
{
    gtk_main_quit ();
}

static void
on_spin_changed (GtkSpinButton * button, gpointer user_data)
{
  gint pgno;

  pgno = (gint)gtk_spin_button_get_value (button);
  change_page (pgno);
}

static void
create_window (void)
{
  GtkWidget *vbox;
  GtkWidget *spin;

  window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));

  g_signal_connect (G_OBJECT (window), "delete-event",
      G_CALLBACK (delete_event), NULL);
  g_signal_connect (G_OBJECT (window), "destroy",
      G_CALLBACK (destroy_event), NULL);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  spin = gtk_spin_button_new_with_range (0x100, 0x8FF, 1.0);
  g_signal_connect (G_OBJECT (spin), "value-changed",
      G_CALLBACK (on_spin_changed), NULL);
  gtk_box_pack_end (GTK_BOX (vbox), spin, FALSE, FALSE, 0);
  gtk_widget_show (spin);

  video = gtk_drawing_area_new ();
  gtk_box_pack_start (GTK_BOX (vbox), video, TRUE, TRUE, 0);
  gtk_widget_set_size_request (video, 500, 500);
  gtk_widget_show (video);

  gtk_widget_show (GTK_WIDGET (window));
}

static void 
change_page (gint pgno)
{
  GValue val = {0,};

  g_message ("Changing page to %03x", pgno); 

  g_value_init (&val, G_TYPE_INT);
  g_value_set_int (&val, pgno);
  g_object_set_property (G_OBJECT (teletext), "page", &val);

  g_value_unset (&val);

  return;
}

int
main (int argv, char **argc)
{
    GstElement *pipeline = NULL;
    GError *error = NULL;
    GstElement *videosink;

    gst_init (&argv, &argc);
    gtk_init (&argv, &argc);

    create_window ();

    pipeline = gst_parse_launch (LIVE_PIPELINE
        " ! mpegtsdemux ! private/teletext ! teletext name=tele ! ffmpegcolorspace ! ximagesink name=xsink",
        &error);
    if (pipeline == NULL) {
        g_critical ("%s\n", error->message);
        g_error_free (error);
        return 1;
    }

    teletext = gst_bin_get_by_name (GST_BIN (pipeline), "tele");
    if (teletext == NULL) {
      g_print ("NO teletext element\n");
      return 1;
    }

    videosink = gst_bin_get_by_name (GST_BIN (pipeline), "xsink");
    if (videosink == NULL) {
        g_print ("NO sink\n");
        return 1;
    }

    gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (videosink), GDK_WINDOW_XWINDOW (video->window));
    gst_x_overlay_handle_events (GST_X_OVERLAY (videosink), FALSE);

    gst_object_unref (videosink);

    g_message ("Set PLAYING");
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_message ("Running ...");

    gtk_main ();

    gst_object_unref (teletext);

    return 0;
}
