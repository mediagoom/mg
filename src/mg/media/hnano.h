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

//#include "splitter.h"

__ALX_BEGIN_NAMESPACE

#define HNS(x) static_cast<const TCHAR*>(ALX::hns(x))
#define HNSF(x) static_cast<const TCHAR*>(ALX::hnsf(x))

class HundredNano
{
    int64_t _nano;

#define MILLY 1000
#define HU    10000
#define SEC   (MILLY * HU)
    
public:

    HundredNano(int64_t H, int64_t M, int64_t S, int64_t ML)
    {
        _nano = 0;
        _nano = H * 60 * 60 * SEC;
        _nano += M * 60 * SEC;
        _nano += S * SEC;
        _nano += ML * HU;
    }

	HundredNano(int64_t ML):_nano(ML)
    {
        
    }

	int64_t H()
	{
		return (_nano / (SEC * 60)) / 60; 
	}

	int64_t M()
	{
		return (_nano / (SEC * 60)) % 60 ; 
	}

	int64_t S()
	{
		return (_nano / SEC ) % 60; 
	}

	int64_t ML()
	{
		//return _nano % MILLY; 
		return (_nano / HU) % MILLY;
	}

	

    operator int64_t()
    {
        return _nano;
    }

	Cstring ToString()
	{
		Cstring tmp;

		//tmp += _T(":");

		if(10 > H())
			tmp += _T("0");
		tmp += H();
		tmp += _T(":");
		if(10 > M())
			tmp += _T("0");
		tmp += M();
		tmp += _T(":");
		if(10 > S())
			tmp += _T("0");
		tmp += S();
		tmp += _T(".");
		if(10 > ML())
			tmp += _T("0");
		if(100 > ML())
			tmp += _T("0");
		tmp += ML();
		
		return tmp;
	}
};


inline Cstring hns(const int64_t x)
{
	Cstring t(_T(""));
	int64_t p = 1;
	if(0 > x)
	{
		t += _T("-");
		p = -1;
	}

	HundredNano h(x * p);
	t += h.ToString();
	return t;
}

/*
inline Cstring hnsf(const int64_t x)
{
	HundredNano h(x);
	Cstring t = alx::replace(h.ToString(), _T("_"), _T("'"));
	t = alx::replace(t, _T("_"), _T(":"));

	return t;
}
*/
__ALX_END_NAMESPACE

