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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstteletextdec.h"

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
teletext_init (GstPlugin * teletext)
{
  /* Helps debugging. */
  vbi_set_log_fn ((VBI_LOG_NOTICE |
        VBI_LOG_WARNING |
        VBI_LOG_ERROR |
        VBI_LOG_INFO),
      vbi_log_on_stderr,
      /* user_data */ NULL);

  return gst_element_register (teletext, "teletext", GST_RANK_NONE,
      GST_TYPE_TELETEXTDEC);
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "teletext",
    "Teletext plugin",
    teletext_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
