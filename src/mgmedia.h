/*****************************************************************************
 * Copyright (C) 2016 mg project
 *
 * Authors: MediaGoom <admin@mediagoom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE or NONINFRINGEMENT. 
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * For more information, contact us at info@mediagoom.com.
 *****************************************************************************/
#include <mg/media/TBitstream.h>


#ifdef HAVE_LIBGYPAES
#include "aes.h"
#endif

#ifndef FRAGMENTEDSTYP
#ifdef DEBUG
#define FRAGMENTEDSTYPTRUE
#else
#define FRAGMENTEDSTYPFALSE
#endif
#endif

#include <mg/media/mp4/cenc.h>
#include <mg/media/mp4parse.h>
#include <mg/media/mp4write.h>
#include <mg/media/mp4fragmented.h>
#include <mg/media/mp4edit.h>
#include <mg/media/MOOFReader.h>
#include <mg/media/mpd_renderer.h>
#include <mg/media/tsinfo.h>





