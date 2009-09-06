/*
 * GStreamer
 * Copyright (C) 2009 Sebastian <sebp@k-d-w.org>
 *
 * This library is free software; you can redistribute it and/or
 * mod1ify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef __GST_TELETEXTDEC_H__
#define __GST_TELETEXTDEC_H__

#include <gst/gst.h>
#include <libzvbi.h>

G_BEGIN_DECLS

#define GST_TYPE_TELETEXTDEC \
  (gst_teletextdec_get_type())
#define GST_TELETEXTDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TELETEXTDEC,GstTeletextDec))
#define GST_TELETEXTDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TELETEXTDEC,GstTeletextDecClass))
#define GST_IS_TELETEXTDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TELETEXTDEC))
#define GST_IS_TELETEXTDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TELETEXTDEC))

typedef struct _GstTeletextDec      GstTeletextDec;
typedef struct _GstTeletextDecClass GstTeletextDecClass;

struct _GstTeletextDec {
  GstElement element;

  GstPad *sinkpad;
  GstPad *srcpad;

  GstClockTime in_timestamp;
  GstClockTime in_duration;
  gint rate_numerator;
  gint rate_denominator;
  gint pageno;
  gint subno;

  vbi_dvb_demux * demux;
  vbi_decoder * decoder;
  GQueue *queue;
  GMutex *queue_lock;
};

struct _GstTeletextDecClass {
  GstElementClass parent_class;
};

GType gst_teletextdec_get_type (void);

G_END_DECLS

#endif /* __GST_TELETEXTDEC_H__ */
