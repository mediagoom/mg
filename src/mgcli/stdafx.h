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
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#include "targetver.h"

#include <stdio.h>
//#include <tchar.h>


#include <mgmedia.h>

#include <iostream>
//#define MP4INFO
//#include "mp4parse.h"
//#include "mp4write.h"

#ifdef BASE64
#include <cencode.h>
#include <cdecode.h>
#endif

#ifdef _WIN32
#include <wmsdkidl.h>
//#include "MP4EditAACReplace.h"
//#include "atlenc.h"
#else


typedef struct _WMT_TIMECODE_EXTENSION_DATA
{
	uint16_t wRange;
	uint32_t dwTimecode;
	uint32_t dwUserbits;
	uint32_t dwAmFlags;
} WMT_TIMECODE_EXTENSION_DATA;


#endif

#define ATL_BASE64_FLAG_NOPAD  0
#define ATL_BASE64_FLAG_NOCRLF 0

