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
#include <mg/core/bitstream.h>
#include <mg/core/thread.h>
#include <mg/core/uvbase.h>
#include <mg/core/file.h>
#include <mg/core/util.h>
#include <mg/core/ctime.h>
#include <mg/cmdline.h>
#include <mg/splitter.h>

#ifdef _WIN32
#include <mg/core/win/console_color.h>
#else
#include <mg/core/nx/console_color.h>
#endif




