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

#include "win-util.h"

#include "../ctime.h"

__ALX_BEGIN_NAMESPACE

int64_t Ctime::_GetUTCOffset()
{
	TIME_ZONE_INFORMATION tzi;
	::ZeroMemory(&tzi, sizeof(TIME_ZONE_INFORMATION));
	DWORD dword = GetTimeZoneInformation(&tzi);
		
	if(TIME_ZONE_ID_INVALID == dword)
	{
		ALXTHROW_LASTERR;
	}
	else if(TIME_ZONE_ID_UNKNOWN == dword)
	{
		//ALXTHROW_T(_T("UNKNOWN TIME ZONE"));
		return 0;
	}
	else if(TIME_ZONE_ID_STANDARD == dword)
	{
		return -1 * tzi.Bias;
	}
	else if(TIME_ZONE_ID_DAYLIGHT == dword)
	{
		return -1 * (tzi.Bias + tzi.DaylightBias);
	}
	else
	{
			ALXTHROW_T(_T("UNKNOWN ERROR"));
	}

	return UINT32_MAX;
}

int64_t Ctime::GetLocalOffset(){return _GetUTCOffset();}

void  Ctime::ResetToNow(bool local)
	{
		_local = local;
		_offset = 0;
		if(_local)
		{
			::GetLocalTime(&_systime);
			_offset = _GetUTCOffset();
		}
		else
			::GetSystemTime(&_systime);

	}

int64_t Ctime::GetUTCOffset() const { return _offset; }

Ctime::Ctime(bool local)
{
	ResetToNow(local);
}


Ctime::Ctime(WORD wYear
	, WORD wMonth
	, WORD wDay)
{
	_systime.wYear = wYear;
	_systime.wMonth = wMonth;
	_systime.wDay = wDay;
	_systime.wDayOfWeek = 0;
	_systime.wHour = 0;
	_systime.wMinute = 0;
	_systime.wSecond = 0;
	_systime.wMilliseconds = 0;
}



Ctime::Ctime(LPCTSTR str_time
	, bool local
	, LPCTSTR str_format)
{

	//_stscanf(str_time
	_stscanf_s(str_time
		, str_format
		, &_systime.wYear
		, &_systime.wMonth
		, &_systime.wDay
		, &_systime.wHour
		, &_systime.wMinute
		, &_systime.wSecond
		, &_systime.wMilliseconds
	);

	_local = local;
	_offset = 0;

	if (_local)
		_offset = _GetUTCOffset();
}

int64_t Ctime::MilliSeconds(const SYSTEMTIME * psysBase) const
{
	FILETIME filetime;
	if (!SystemTimeToFileTime(psysBase, &filetime))
		ALXTHROW_LASTERR;

	ULARGE_INTEGER u;

	u.HighPart = filetime.dwHighDateTime;
	u.LowPart = filetime.dwLowDateTime;

	int64_t a = u.QuadPart;

	if (!SystemTimeToFileTime(&_systime, &filetime))
		ALXTHROW_LASTERR;

	u.HighPart = filetime.dwHighDateTime;
	u.LowPart = filetime.dwLowDateTime;

	int64_t b = u.QuadPart;

	return (b - a) / 10000LL;

}

int64_t Ctime::TotalHNano() const
{
	FILETIME filetime;
	if (!SystemTimeToFileTime(&_systime, &filetime))
		ALXTHROW_LASTERR;

	ULARGE_INTEGER u;

	u.HighPart = filetime.dwHighDateTime;
	u.LowPart = filetime.dwLowDateTime;

	return u.QuadPart;
}
int64_t Ctime::TotalMilliseconds() const { return TotalHNano() / 10000ULL; }
int64_t Ctime::AddMilliSeconds(int64_t milly_to_add) const
{

	return TotalMilliseconds() + milly_to_add;

}

void Ctime::AddMilliSeconds(Ctime &rhs, int64_t milly_to_add) const
{
	//ULARGE_INTEGER u;
	//u.QuadPart = AddMilliSeconds(milly_to_add);

	uint64_t t = AddMilliSeconds(milly_to_add) * 10000LL;

	FILETIME filetime = { static_cast<uint32_t>(t) , static_cast<uint32_t>(t >> 32) };
	//filetime.dwHighDateTime = u.HighPart;
	//filetime.dwLowDateTime  = u.LowPart;



	if (!FileTimeToSystemTime(&filetime,
		&rhs._systime))
		ALXTHROW_LASTERR;
}

int64_t Ctime::Seconds(const SYSTEMTIME * psysBase) const
{
	return MilliSeconds(psysBase) / 1000LL;
}

int64_t Ctime::Minutes(const SYSTEMTIME * psysBase) const
{
	return Seconds(psysBase) / 60;
}
int64_t Ctime::Hours(const SYSTEMTIME * psysBase) const
{
	return Minutes(psysBase) / 60;
}
int64_t Ctime::Hours() const
{
	Ctime t(_local);
	return t.Hours(&get_Time());
}
int64_t Ctime::Days(const SYSTEMTIME * psysBase) const
{
	return Minutes(psysBase) / 1440;
}
int64_t Ctime::Days() const
{
	Ctime t(_local);
	return t.Days(&get_Time());
}


Ctime::Ctime(FILETIME * pfiletime)
{
	if (!FileTimeToSystemTime(pfiletime, &_systime))
		ALXTHROW_LASTERR;
}

Cstring Ctime::ToTimeString(const TCHAR * lpFormat, LCID Locale, uint16_t dwFlags) const
{
	int size = GetTimeFormat(Locale,              // locale
		dwFlags,            // options
		&_systime, // time
		lpFormat,         // time format string
		NULL,         // formatted string buffer
		0               // size of string buffer
	);

	Cstring time;

	//size += 1;

	size = GetTimeFormat(Locale,              // locale
		dwFlags,            // options
		&_systime, // time
		lpFormat,         // time format string
		time.GetBuffer(size, false),         // formatted string buffer
		size               // size of string buffer
	);

	time.CommitBuffer(size - 1);

	if (0 == size)
		ALXTHROW_LASTERR;

	return time;



}

Cstring Ctime::ToDateString(const TCHAR * lpFormat, LCID Locale, uint16_t dwFlags) const
{
	int size = GetDateFormat(Locale,              // locale
		dwFlags,            // options
		&_systime, // time
		lpFormat,         // time format string
		NULL,         // formatted string buffer
		0               // size of string buffer
	);

	Cstring time;

	//size += 1;

	size = GetDateFormat(Locale,              // locale
		dwFlags,            // options
		&_systime, // time
		lpFormat,         // time format string
		time.GetBuffer(size, false),         // formatted string buffer
		size               // size of string buffer
	);

	time.CommitBuffer(size - 1);

	if (0 == size)
		ALXTHROW_LASTERR;

	return time;


}

Cstring Ctime::get_MillisecondsString() const
{
	Cstring t(_T(""));
	if (_systime.wMilliseconds < 100)
		t += _T("0");
	if (_systime.wMilliseconds < 10)
		t += _T("0");

	t += _systime.wMilliseconds;

	return t;
}

const SYSTEMTIME & Ctime::get_Time() const
{
	return _systime;
}

void Ctime::set_Time(const SYSTEMTIME * psystime)
{
	memcpy(&_systime, psystime, sizeof(SYSTEMTIME));
}


Ctime::Ctime(SYSTEMTIME * psystime)
{
	set_Time(psystime);
}

Cstring Ctime::ToXml() const
{
	Cstring tmp = ToDateString(_T("yyyy-MM-dd"));
	tmp += _T("T");
	tmp += ToTimeString(_T("HH:mm:ss"));
	tmp += _T(".");
	tmp += get_MillisecondsString();

	int64_t offset = GetUTCOffset() / 60;

	if (!_local)
		offset = 0;

	if (0 != offset)
	{
		if (offset > 0)
			tmp += _T("+");
		else
		{
			offset = -1 * offset;
			tmp += _T("-");
		}


		if (offset < 10)
			tmp += _T("0");

		tmp += offset;

		tmp += _T(":00");
	}
	else
	{
		tmp += _T("Z");
	}

	return tmp;

}

__ALX_END_NAMESPACE

