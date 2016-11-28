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
#include <time.h>
#include "../../exception.h"
#include "../ctime.h"
#include "nx-util.h"

__ALX_BEGIN_NAMESPACE

inline void tm_to_sm( tm & origin, SYSTEMTIME & sm )
{
	sm.wYear = origin.tm_year;
	sm.wMonth = origin.tm_mon;
	sm.wDayOfWeek = origin.tm_wday;
	sm.wDay = origin.tm_mday;
	sm.wHour = origin.tm_hour;
	sm.wMinute = origin.tm_min;
	sm.wSecond = origin.tm_sec;
	sm.wMilliseconds = 0;

}


inline void sm_to_tm(const SYSTEMTIME & origin, tm & tms)
{
	tms.tm_year = origin.wYear;
	tms.tm_mon = origin.wMonth;
	tms.tm_wday = origin.wDayOfWeek;
	tms.tm_mday = origin.wDay;
	tms.tm_hour = origin.wHour;
	tms.tm_min = origin.wMinute;
	tms.tm_sec = origin.wSecond;
	tms.tm_isdst = -1;
}
inline uint64_t total_seconds(const SYSTEMTIME & systime)
{
	tm tms;
		sm_to_tm(systime, tms);
		return mktime(&tms); 

}
inline uint64_t total_hnano(const SYSTEMTIME & systime)
{
	if(UINT64_MAX == systime.total_hnano)
	{
		return total_seconds(systime) * 10000000ULL;
	}
	else
	{
		return systime.total_hnano;
	}
}


int64_t Ctime::_GetUTCOffset()
{

	time_t rawtime = time(NULL);
	    struct tm *ptm = gmtime(&rawtime);
        // Request that mktime() looksup dst in timezone database
	    ptm->tm_isdst = -1;                
	time_t gmt = mktime(ptm);
	    double offset = difftime(rawtime, gmt) / 60;




	return static_cast<int64_t>(offset); 

}

int64_t  Ctime::GetLocalOffset(){return _GetUTCOffset();}

void  Ctime::ResetToNow(bool local)
{
	_local = local;
	_offset = 0;
	
	time_t tt = time(NULL);

	timespec ts;

	int r = clock_gettime(CLOCK_REALTIME, &ts);
	MGCHECK(r);

	_systime.total_hnano = ts.tv_sec * 10000000ull + ts.tv_nsec / 100ull;

	tm stm;
        tm * pstm(0);

	if(_local)
	{
		
		pstm = localtime_r(&tt, &stm);
		_offset = _GetUTCOffset();
	}
	else
		pstm = gmtime_r(&tt, &stm);


	tm_to_sm(stm, _systime);	

}

int64_t Ctime::GetUTCOffset() const { return _offset; }

Ctime::Ctime(bool local)
{
	ResetToNow(local);
}


Ctime::Ctime(uint16_t wYear
	, uint16_t wMonth
	, uint16_t wDay)
{
	_systime.wYear = wYear;
	_systime.wMonth = wMonth;
	_systime.wDay = wDay;
	_systime.wDayOfWeek = 0;
	_systime.wHour = 0;
	_systime.wMinute = 0;
	_systime.wSecond = 0;
	_systime.wMilliseconds = 0;
	_systime.total_hnano = UINT64_MAX;
}



Ctime::Ctime(const TCHAR * str_time
	, bool local
	, const TCHAR * str_format)
{

	//_stscanf(str_time
	//TODO: _stscanf_s
	sscanf(str_time
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
	if((UINT64_MAX == psysBase->total_hnano
		&& UINT64_MAX == _systime.total_hnano)
		|| (UINT64_MAX != psysBase->total_hnano
		   && UINT64_MAX != _systime.total_hnano)
		   )
		   {
		   	return (total_hnano(_systime) - total_hnano((*psysBase))) / 10000UL;
		   }
		   else
		   {
		   	return (total_seconds(_systime) - total_seconds((*psysBase))) * 1000; 

		   }
	
}


int64_t Ctime::TotalHNano() const
{
	if(UINT64_MAX == _systime.total_hnano)
	{
		tm tms;
		sm_to_tm(_systime, tms);
		return mktime(&tms) * 10000000ULL;
	}
	else
	{
		return _systime.total_hnano;
	}
}


int64_t Ctime::TotalMilliseconds() const { return TotalHNano() / 10000ULL; }
int64_t Ctime::AddMilliSeconds(int64_t milly_to_add) const
{

	return TotalMilliseconds() + milly_to_add;

}
/*
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
*/

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

/*
Ctime::Ctime(FILETIME * pfiletime)
{
	if (!FileTimeToSystemTime(pfiletime, &_systime))
		ALXTHROW_LASTERR;
}
*/

Cstring Ctime::ToTimeString(const TCHAR * lpFormat, LCID Locale, uint16_t dwFlags) const
{
	Cstring time;
	size_t max = 1024;

	TCHAR ff[] = _T("%H:%M:%S");

	if(NULL == lpFormat)
	{
		lpFormat = ff;
	}


	tm ktime;

	sm_to_tm(_systime, ktime);

	size_t size = strftime(time.GetBuffer(max, false)
		, max
		, lpFormat
		, &ktime);



	time.CommitBuffer(size );

	if (0 == size)
		ALXTHROW_LASTERR;

	return time;

}

Cstring Ctime::ToDateString(const TCHAR * lpFormat, LCID Locale, uint16_t dwFlags) const
{

	if(NULL != lpFormat)
		return ToTimeString(lpFormat, Locale, dwFlags);
	else
		return ToTimeString(_T("%Y-%m-%d"), Locale, dwFlags);


}

Cstring Ctime::get_MillisecondsString() const
{
	Cstring t(_T(""));
	
	uint64_t mx = total_hnano(_systime);
		 mx %= 10000000ull; //find hnano
		 mx /= 10000ull;    //transform to milly

	if (mx < 100)
		t += _T("0");
	if (mx < 10)
		t += _T("0");

	t += mx; 

	return t;
}

const SYSTEMTIME & Ctime::get_Time() const
{
	return _systime;
}
/*
void Ctime::set_Time(const SYSTEMTIME * psystime)
{
	memcpy(&_systime, psystime, sizeof(SYSTEMTIME));
}


Ctime::Ctime(SYSTEMTIME * psystime)
{
	set_Time(psystime);
}
*/

Cstring Ctime::ToXml() const
{
	Cstring tmp = ToDateString(NULL);
	tmp += _T("T");
	tmp += ToTimeString(NULL);
	tmp += _T(".");
	tmp += get_MillisecondsString();

	uint64_t offset = GetUTCOffset() / 60;

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

