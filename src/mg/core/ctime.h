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

#include "core.h"
#include "char.h"
#include "../alxstring.h"

__ALX_BEGIN_NAMESPACE

#ifndef _WIN32

typedef struct _SYSTEMTIME {
	uint16_t wYear;
	uint16_t wMonth;
	uint16_t wDayOfWeek;
	uint16_t wDay;
	uint16_t wHour;
	uint16_t wMinute;
	uint16_t wSecond;
	uint16_t wMilliseconds;

	uint64_t total_hnano;
} SYSTEMTIME;

typedef struct _FILETIME {
	uint32_t dwLowDateTime;
	uint32_t dwHighDateTime;
} FILETIME;

typedef uint32_t LCID;

#endif


class Ctime
{
	SYSTEMTIME _systime;
	bool       _local;

	int64_t       _offset;

	static int64_t _GetUTCOffset();


public:

	static int64_t GetLocalOffset();

	void ResetToNow(bool local = true);
	

	int64_t GetUTCOffset() const;

	Ctime(bool local = true);
	

	
	Ctime(uint16_t wYear
		, uint16_t wMonth
		, uint16_t wDay)
		;



	Ctime(const TCHAR* str_time
		, bool local
		,  const TCHAR * str_format = _T("%4d-%2d-%2d %2d:%2d:%2d.%3d"))
		;
	
	int64_t MilliSeconds(const SYSTEMTIME * psysBase) const
		;

	int64_t TotalHNano() const;
	
	int64_t TotalMilliseconds() const;
	int64_t AddMilliSeconds(int64_t milly_to_add) const;

	void AddMilliSeconds(Ctime &rhs, int64_t milly_to_add) const;

	int64_t Seconds(const SYSTEMTIME * psysBase) const;
	

	int64_t Minutes(const SYSTEMTIME * psysBase) const;
	int64_t Hours(const SYSTEMTIME * psysBase) const;
	int64_t Hours() const;
	int64_t Days(const SYSTEMTIME * psysBase) const;

	int64_t Days() const;
	Ctime(FILETIME * pfiletime);

	Cstring ToTimeString(const TCHAR * lpFormat = NULL, LCID Locale = 0, uint16_t dwFlags = 0) const
		;

	Cstring ToDateString(const TCHAR * lpFormat = NULL, LCID Locale = 0, uint16_t dwFlags = 0) const
		;

	Cstring get_MillisecondsString() const
		;

	const SYSTEMTIME & get_Time() const
		;
	void set_Time(const SYSTEMTIME * psystime)
		;


	Ctime(SYSTEMTIME * psystime)
		;

	Cstring ToXml() const
		;
};


inline uint64_t clock_elapsed(Ctime & clock, bool reset = true)
{
	Ctime t(false);

	uint64_t d = t.TotalHNano() - clock.TotalHNano();

	if (reset)
	{
		clock.ResetToNow(false);
	}

	return d;
}

#define STARTCLOCK Ctime __clock__(false);

#define TIMECLOCK clock_elapsed(__clock__)

__ALX_END_NAMESPACE

