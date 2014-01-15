/* gnome-rr-x11.h
 *
 * Copyright 2011, Red Hat, Inc.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Soren Sandmann <sandmann@redhat.com>
 *          Marc-André Lureau <marcandre.lureau@redhat.com>
 */

#ifndef GNOME_RR_X11_H
#define GNOME_RR_X11_H

#include <glib.h>
#include "gnome-rr.h"

#define GNOME_TYPE_RR_X11_SCREEN                  (gnome_rr_x11_screen_get_type())
#define GNOME_RR_X11_SCREEN(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_RR_X11_SCREEN, GnomeRRX11Screen))
#define GNOME_IS_RR_X11_SCREEN(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_RR_X11_SCREEN))
#define GNOME_RR_X11_SCREEN_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_RR_X11_SCREEN, GnomeRRX11ScreenClass))
#define GNOME_IS_RR_X11_SCREEN_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_RR_X11_SCREEN))
#define GNOME_RR_X11_SCREEN_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_RR_X11_SCREEN, GnomeRRX11ScreenClass))

typedef struct GnomeRRX11ScreenPrivate GnomeRRX11ScreenPrivate;

typedef struct {
	GnomeRRScreen parent;

	GnomeRRX11ScreenPrivate* priv;
} GnomeRRX11Screen;

typedef struct {
	GnomeRRScreenClass parent_class;

        void (* changed) (void);
} GnomeRRX11ScreenClass;

GType gnome_rr_x11_screen_get_type (void);

G_GNUC_INTERNAL
gboolean gnome_rr_x11_screen_force_timestamp_update (GnomeRRX11Screen *screen);

#endif  /* GNOME_RR_X11_H */
