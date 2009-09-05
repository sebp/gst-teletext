/* 
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2009 Sebastian <<user@hostname.org>>
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

#ifndef __GST_TELETEXT_H__
#define __GST_TELETEXT_H__

#include <gst/gst.h>
#include <libzvbi.h>

G_BEGIN_DECLS

#define GST_TYPE_TELETEXT \
  (gst_teletext_get_type())
#define GST_TELETEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TELETEXT,GstTeletext))
#define GST_TELETEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TELETEXT,GstTeletextClass))
#define GST_IS_TELETEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TELETEXT))
#define GST_IS_TELETEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TELETEXT))

typedef struct _GstTeletext      GstTeletext;
typedef struct _GstTeletextClass GstTeletextClass;

struct _GstTeletext {
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

struct _GstTeletextClass {
  GstElementClass parent_class;
};

GType gst_teletext_get_type (void);

G_END_DECLS

#endif /* __GST_GSTTELETEXT_H__ */
