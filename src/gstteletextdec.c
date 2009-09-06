/*
 * GStreamer
 * Copyright (C) 2009 Sebastian <sebp@k-d-w.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-teletextdec
 *
 * Decode PES stream containing teletext information to RGBA stream
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m filesrc location=recording.mpeg ! mpegtsdemux ! private/teletext ! teletextdec ! ffmpegcolorspace ! ximagesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>

#include "gstteletextdec.h"

GST_DEBUG_CATEGORY_STATIC (gst_teletextdec_debug);
#define GST_CAT_DEFAULT gst_teletextdec_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PAGENO,
  PROP_SUBNO
};

typedef struct {
  int pgno;
  int subno;
} page_info;

static GstStaticPadTemplate sink_template =
GST_STATIC_PAD_TEMPLATE (
  "sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("private/teletext")
);

static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE (
  "src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (GST_VIDEO_CAPS_RGBA)
);

/* debug category for filtering log messages */
#define DEBUG_INIT(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_teletextdec_debug, "teletext", 0, "Teletext decoder");

GST_BOILERPLATE_FULL (GstTeletextDec, gst_teletextdec, GstElement,
    GST_TYPE_ELEMENT, DEBUG_INIT);

static void gst_teletextdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_teletextdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_teletextdec_finalize (GObject *object);

static GstStateChangeReturn gst_teletextdec_change_state (GstElement * element,
    GstStateChange transition);

static GstFlowReturn gst_teletextdec_chain (GstPad * pad, GstBuffer * buf);
static gboolean gst_teletextdec_sink_event (GstPad * pad, GstEvent * event);

static vbi_bool gst_teletextdec_convert (vbi_dvb_demux *	dx, gpointer user_data,
    const vbi_sliced * sliced, guint n_lines, gint64 pts);
static void event_handler (vbi_event * ev, void * user_data);

static GstFlowReturn gst_teletextdec_push_page (GstTeletextDec *teletext, vbi_page *page);
static void gst_teletextdec_zvbi_init (GstTeletextDec * teletext);
static void gst_teletextdec_zvbi_clear (GstTeletextDec * teletext);

/* GObject vmethod implementations */

static void
gst_teletextdec_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_set_details_simple (element_class,
    "Teletext decoder",
    "Decoder",
    "Decode PES stream containing teletext information to RGBA stream",
    "Sebastian Pölsterl <sebp@k-d-w.org>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
}

/* initialize the gstteletext's class */
static void
gst_teletextdec_class_init (GstTeletextDecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = gst_teletextdec_set_property;
  gobject_class->get_property = gst_teletextdec_get_property;
  gobject_class->finalize = gst_teletextdec_finalize;

  gstelement_class = GST_ELEMENT_CLASS (klass);
  gstelement_class->change_state = gst_teletextdec_change_state;

  g_object_class_install_property (gobject_class, PROP_PAGENO,
      g_param_spec_int ("page", "Page number",
        "Number of page that should displayed",
        0x100, 0x8FF, 0x100, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SUBNO,
      g_param_spec_int ("subpage", "Sub-page number",
        "Number of sub-page that should displayed (-1 for all)",
        -1, 0x99, -1, G_PARAM_READWRITE));
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_teletextdec_init (GstTeletextDec *teletext, GstTeletextDecClass * klass)
{
  teletext->sinkpad = gst_pad_new_from_static_template (&sink_template, "sink");
  gst_pad_set_chain_function (teletext->sinkpad,
                              GST_DEBUG_FUNCPTR (gst_teletextdec_chain));
  gst_pad_set_event_function (teletext->sinkpad,
                              GST_DEBUG_FUNCPTR (gst_teletextdec_sink_event));

  teletext->srcpad = gst_pad_new_from_static_template (&src_template, "src");
  gst_pad_use_fixed_caps (teletext->srcpad);

  gst_element_add_pad (GST_ELEMENT (teletext), teletext->sinkpad);
  gst_element_add_pad (GST_ELEMENT (teletext), teletext->srcpad);

  teletext->demux = NULL;
  teletext->decoder = NULL;
  teletext->pageno = 0x100;
  teletext->subno = -1;

  teletext->in_timestamp = GST_CLOCK_TIME_NONE;
  teletext->in_duration = GST_CLOCK_TIME_NONE;

  teletext->rate_numerator = 0;
  teletext->rate_denominator = 1;

  teletext->queue = NULL;
  teletext->queue_lock = g_mutex_new ();
}

static void
gst_teletextdec_finalize (GObject *object)
{
  GstTeletextDec *teletext = GST_TELETEXTDEC (object);

  g_mutex_free (teletext->queue_lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_teletextdec_zvbi_init (GstTeletextDec * teletext)
{
  g_return_if_fail (teletext != NULL);

  GST_LOG_OBJECT (teletext, "Initializing structures");

  teletext->demux = vbi_dvb_pes_demux_new (gst_teletextdec_convert, teletext);

  teletext->decoder = vbi_decoder_new ();

  vbi_event_handler_register (teletext->decoder, VBI_EVENT_TTX_PAGE, event_handler,
    teletext);

  g_mutex_lock (teletext->queue_lock);
  teletext->queue = g_queue_new ();
  g_mutex_unlock (teletext->queue_lock);
}

static void
gst_teletextdec_zvbi_clear (GstTeletextDec * teletext)
{
  g_return_if_fail (teletext != NULL);

  GST_LOG_OBJECT (teletext, "Clearing structures");

  if (teletext->demux != NULL) {
    vbi_dvb_demux_delete (teletext->demux);
    teletext->demux = NULL;
  }
  if (teletext->decoder != NULL) {
    vbi_decoder_delete (teletext->decoder);
    teletext->decoder = NULL;
  }

  g_mutex_lock (teletext->queue_lock);
  if (teletext->queue != NULL) {
    g_queue_free (teletext->queue);
    teletext->queue = NULL;
  }
  g_mutex_unlock (teletext->queue_lock);

  teletext->in_timestamp = GST_CLOCK_TIME_NONE;
  teletext->in_duration = GST_CLOCK_TIME_NONE;
  teletext->pageno = 0x100;
  teletext->subno = -1;
}

static void
gst_teletextdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTeletextDec *teletext = GST_TELETEXTDEC (object);

  switch (prop_id) {
    case PROP_PAGENO:
      teletext->pageno = g_value_get_int (value);
      break;
    case PROP_SUBNO:
      teletext->subno = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_teletextdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstTeletextDec *teletext = GST_TELETEXTDEC (object);

  switch (prop_id) {
    case PROP_PAGENO:
      g_value_set_int (value, teletext->pageno);
      break;
    case PROP_SUBNO:
      g_value_set_int (value, teletext->subno);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* this function does the actual processing
 */
static GstFlowReturn
gst_teletextdec_chain (GstPad * pad, GstBuffer * buf)
{
  GstTeletextDec *teletext = GST_TELETEXTDEC (gst_pad_get_parent (pad));
  gboolean success;
  GstFlowReturn result = GST_FLOW_OK;

  teletext->in_timestamp = GST_BUFFER_TIMESTAMP (buf);
  teletext->in_duration = GST_BUFFER_DURATION (buf);

  GST_LOG_OBJECT (teletext, "Feeding %u bytes to VBI demuxer",
    GST_BUFFER_SIZE (buf));

  success = vbi_dvb_demux_feed (teletext->demux,
    GST_BUFFER_DATA (buf),
    GST_BUFFER_SIZE (buf));
 
  g_mutex_lock (teletext->queue_lock);
  if (!g_queue_is_empty (teletext->queue)) {
    vbi_page page;
    page_info *pi;

    pi = (page_info *) g_queue_pop_head (teletext->queue);

    GST_INFO_OBJECT (teletext, "Fetching teletext page %03x.%02x",
        pi->pgno, pi->subno);

    success = vbi_fetch_vt_page (teletext->decoder, &page, pi->pgno, pi->subno,
        VBI_WST_LEVEL_3p5,
        /* n_rows */ 25,
        /* navigation */ FALSE);
    if (G_UNLIKELY (!success))
      goto fetch_page_failed;

    result = gst_teletextdec_push_page (teletext, &page);

    vbi_unref_page (&page);
  }
  g_mutex_unlock (teletext->queue_lock);

  gst_object_unref (teletext);
  gst_buffer_unref (buf);

  return result;

  /* ERRORS */
fetch_page_failed:
  {
    GST_ELEMENT_ERROR (teletext, RESOURCE, READ, (NULL), (NULL));
    return GST_FLOW_ERROR;
  }
}

static gboolean
gst_teletextdec_sink_event (GstPad * pad, GstEvent * event)
{
  gboolean ret;
  GstTeletextDec *teletext = GST_TELETEXTDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (teletext, "got event %s",
      gst_event_type_get_name (GST_EVENT_TYPE (event)));
  
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NEWSEGMENT:
      /* maybe save and/or update the current segment (e.g. for output
       * clipping) or convert the event into one in a different format
       * (e.g. BYTES to TIME) or drop it and set a flag to send a newsegment
       * event in a different format later */
      ret = gst_pad_push_event (teletext->srcpad, event);
      break;
    case GST_EVENT_EOS:
      /* end-of-stream, we should close down all stream leftovers here */
      gst_teletextdec_zvbi_clear (teletext);
      ret = gst_pad_push_event (teletext->srcpad, event);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_teletextdec_zvbi_clear (teletext);
      gst_teletextdec_zvbi_init (teletext);
      ret = gst_pad_push_event (teletext->srcpad, event);
      break;      
    default:
      ret = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (teletext);

  return ret;
}

static GstStateChangeReturn
gst_teletextdec_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstTeletextDec *teletext;

  teletext = GST_TELETEXTDEC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_teletextdec_zvbi_init (teletext);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret != GST_STATE_CHANGE_SUCCESS)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_teletextdec_zvbi_clear (teletext);
      break;
    default:
      break;
  }

  return ret;
}

static vbi_bool
gst_teletextdec_convert (vbi_dvb_demux * dx,
    gpointer user_data,
    const vbi_sliced * sliced,
    guint n_lines,
    gint64 pts)
{
  GstTeletextDec *teletext = GST_TELETEXTDEC (user_data);

  GST_DEBUG_OBJECT (teletext, "Converting %u lines", n_lines);

  gdouble sample_time;
  vbi_sliced * s;

  sample_time = pts * (1 / 90000.0);

  s = g_memdup (sliced, n_lines * sizeof(vbi_sliced));
  vbi_decode (teletext->decoder, s, n_lines, sample_time);
  g_free (s);

  return GST_FLOW_OK;
}

static void
event_handler (vbi_event * ev, void * user_data)
{
  vbi_pgno pgno;
  vbi_subno subno;

  GstTeletextDec *teletext = GST_TELETEXTDEC (user_data);

  switch (ev->type) {
    case VBI_EVENT_TTX_PAGE:
      pgno = ev->ev.ttx_page.pgno;
      subno = ev->ev.ttx_page.subno;
      
      if (pgno != teletext->pageno
          || (teletext->subno != -1 && subno != teletext->subno))
        return;

      GST_DEBUG_OBJECT (teletext, "Received teletext page %03x.%02x",
        pgno, subno);

      page_info pi = {pgno, subno};

      g_mutex_lock (teletext->queue_lock);
      g_queue_push_tail (teletext->queue, &pi);
      g_mutex_unlock (teletext->queue_lock);
      break;

    default:
    break;
  }

  return;
}

static GstFlowReturn
gst_teletextdec_push_page (GstTeletextDec * teletext, vbi_page * page)
{
  GstBuffer *buf;
  guint size;
  GstCaps *res, *caps;
  GstFlowReturn result;
  gint width, height;
  GstPadTemplate *templ;

  /* one character occupies 12 x 10 pixels */
  width = page->columns * 12;
  height = page->rows * 10;

  caps = gst_caps_new_simple ("video/x-raw-rgb",
    "width", G_TYPE_INT, width, 
    "height", G_TYPE_INT, height, 
    "framerate", GST_TYPE_FRACTION, teletext->rate_numerator,
    teletext->rate_denominator, NULL);

  templ = gst_static_pad_template_get (&src_template);
  res = gst_caps_intersect (caps, gst_pad_template_get_caps (templ));
  gst_caps_unref (caps);
  gst_object_unref (templ);

  if (!gst_pad_set_caps (teletext->srcpad, res))
    goto not_negotiated;

  gst_caps_unref (res);

  size = (guint)width * (guint)height * sizeof(vbi_rgba);

  buf = gst_buffer_try_new_and_alloc (size);
  if (G_UNLIKELY (buf == NULL))
    goto no_buffer;

  gst_buffer_set_caps (buf, res);

  if (GST_CLOCK_TIME_IS_VALID (teletext->in_timestamp))
    GST_BUFFER_TIMESTAMP (buf) = teletext->in_timestamp;
  if (GST_CLOCK_TIME_IS_VALID (teletext->in_duration))
    GST_BUFFER_DURATION (buf) = teletext->in_duration;

  GST_DEBUG_OBJECT (teletext, "Creating image with %d rows and %d cols",
    page->rows, page->columns);

  vbi_draw_vt_page (page, VBI_PIXFMT_RGBA32_LE,
    (vbi_rgba *) GST_BUFFER_DATA (buf),
    FALSE, TRUE);

  GST_DEBUG_OBJECT (teletext, "Pushing buffer of size %u", size);

  result = gst_pad_push (teletext->srcpad, buf);
  if (result != GST_FLOW_OK)
    goto push_failed;
    
  return result;

  /* ERRORS */
not_negotiated:
  {
    gst_caps_unref (res);
    return GST_FLOW_NOT_NEGOTIATED;
  }
no_buffer:
  {
    GST_DEBUG_OBJECT (teletext, "could not allocate buffer");
    return GST_FLOW_ERROR;
  }
push_failed:
  {
    GST_INFO_OBJECT (teletext, "Pushing buffer failed, reason %s",
        gst_flow_get_name (result));
    if (GST_FLOW_IS_FATAL (result) || result == GST_FLOW_NOT_LINKED) {
      GST_ELEMENT_ERROR (teletext, STREAM, FAILED,
          ("Internal data stream error."),
          ("stream stopped, reason %s", gst_flow_get_name (result)));
      gst_pad_push_event (teletext->srcpad, gst_event_new_eos ());
    }
    return result;
  }
}
