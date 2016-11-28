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
#pragma once

#ifndef TCHAR

#ifndef _TCHAR_DEFINED
#ifdef UNICODE
#define TCHAR wchar_t
#else
#define TCHAR char
#endif
#define _TCHAR_DEFINED
#endif


#endif

#ifndef _T
#define _T


#ifndef _WIN32

#include <string.h>

#ifdef UNICODE 

#define _tcslen     wcslen
#define _tcscpy     wcscpy
#define _tcscpy_s   wcscpy_s
#define _tcsncpy    wcsncpy
#define _tcsncpy_s  wcsncpy_s
#define _tcscat     wcscat
#define _tcscat_s   wcscat_s
#define _tcsupr     wcsupr
#define _tcsupr_s   wcsupr_s
#define _tcslwr     wcslwr
#define _tcslwr_s   wcslwr_s

#define _stprintf_s swprintf_s
#define _sntprintf_s swprintf_s
#define _stprintf   swprintf
#define _tprintf    wprintf

#define _vstprintf_s    vswprintf_s
#define _vstprintf      vswprintf

#define _tscanf     wscanf


#define _tcscmp		wstrcmp
#define _tcsstr	    wcsstr
#else

#define _tcslen     strlen
#define _tcscpy     strcpy
#define _tcscpy_s   strcpy_s
#define _tcsncpy    strncpy
#define _tcsncpy_s  strncpy_s
#define _tcscat     strcat
#define _tcscat_s   strcat_s
#define _tcsupr     strupr
#define _tcsupr_s   strupr_s
#define _tcslwr     strlwr
#define _tcslwr_s   strlwr_s

#define _stprintf_s sprintf_s
#define _sntprintf_s snprintf_s
#define _stprintf   sprintf
#define _snprintf   snprintf
#define _tprintf    printf
#define _ftprintf   fprintf
#define _vstprintf_s    vsprintf_s
#define _vstprintf      vsprintf

#define _tscanf     scanf
#define _stscanf_s  sscanf

#define _tcscmp strcmp
#define _tcsstr strstr

#define _tmain main
#endif

#ifndef _TRUNCATE
#define _TRUNCATE UINT32_MAX -1
#endif

#endif //_WIN32

#endif

