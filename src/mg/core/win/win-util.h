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
#include "../../exception.h"

#include <windows.h>

__MGCORE_BEGIN_NAMESPACE

inline void throw_last_win32(const TCHAR* file, int line, uint32_t num = 0)
{
	if (0 == num)
		num = ::GetLastError();

	throw mgexception(num, file, line);
}

#define ALXTHROW_LASTERR       ::mg::core::throw_last_win32((_T(__FILE__)), __LINE__);
#define ALXTHROW_LASTERR1(err) ::mg::core::throw_last_win32((_T(__FILE__)), __LINE__, err);

__MGCORE_END_NAMESPACE

