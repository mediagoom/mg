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

#ifdef _WIN32
#include <crtdbg.h>
#else
#include <assert.h>
#endif

#include <exception>
#include <type_traits>
#include <stdint.h>

#ifndef _ASSERTE
#if _DEBUG
#define _ASSERTE assert
#else
#define _ASSERTE
#endif
#endif

#define __ALX_BEGIN_NAMESPACE namespace mg {namespace core{
#define __ALX_END_NAMESPACE     }}
#define ALX ::mg::core

#define __MGCORE_BEGIN_NAMESPACE namespace mg {namespace core{
#define __MGCORE_END_NAMESPACE     }}
#define MGCORE ::mg::core

#ifndef _WIN32
#ifndef WSTDINT
#define WSTDINT
#include <stddef.h>
//#define uint64_t int64_t
#define DWORD uint32_t
#define ULONG unsigned long
#endif
#endif


__MGCORE_BEGIN_NAMESPACE

class mgexceptionbase: public std::exception
{
	int _err;
public:
	
	mgexceptionbase():_err(0)
	{}

	mgexceptionbase(int num) :_err(num)
	{
		
	}

        //const char * msg(); //TODO: {return uv_strerror(_err); }
	//const char * err_name(); //TODO: {return uv_err_name(_err); }

	int get_error_number() const {return _err;}

};

inline void MGBASECHECK(int num){ if (num < 0){ throw MGCORE::mgexceptionbase(num); } }

#ifndef E_BASE_CUSTOM
#define E_BASE_CUSTOM -10000
#endif

#define E_OVERFLOW (E_BASE_CUSTOM - 9)

__MGCORE_END_NAMESPACE


#ifdef _DEBUG
#define DBGC9(L, p0, p1, p2, p3, p4, p5, p6, p7, p9) _ftprintf(stdout, L "\r\n", p0, p1, p2, p3, p4, p5, p6, p7, p9)
#define DBGC8(L, p0, p1, p2, p3, p4, p5, p6, p7) _ftprintf(stdout, L "\r\n", p0, p1, p2, p3, p4, p5, p6, p7)
#define DBGC7(L, p0, p1, p2, p3, p4, p5, p6) _ftprintf(stdout, L "\r\n", p0, p1, p2, p3, p4, p5, p6)
#define DBGC6(L, p0, p1, p2, p3, p4, p5) _ftprintf(stdout, L "\r\n", p0, p1, p2, p3, p4, p5)
#define DBGC5(L, p0, p1, p2, p3, p4) _ftprintf(stdout, L "\r\n", p0, p1, p2, p3, p4)
#define DBGC4(L, p0, p1, p2, p3) _ftprintf(stdout, L "\r\n", p0, p1, p2, p3)
#define DBGC3(L, p0, p1, p2) _ftprintf(stdout, L "\r\n", p0, p1, p2)
#define DBGC2(L, p0, p1) _ftprintf(stdout, L "\r\n", p0, p1)
#define DBGC1(L, p0) _ftprintf(stdout, L "\r\n", p0)
#define DBGC0(L) _ftprintf(stdout, L "\r\n")
#else
#define DBGC9(L, p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define DBGC8(L, p0, p1, p2, p3, p4, p5, p6, p7)
#define DBGC7(L, p0, p1, p2, p3, p4, p5, p6)
#define DBGC6(L, p0, p1, p2, p3, p4, p5)
#define DBGC5(L, p0, p1, p2, p3, p4)
#define DBGC4(L, p0, p1, p2, p3)
#define DBGC3(L, p0, p1, p2)
#define DBGC2(L, p0, p1)
#define DBGC1(L, p0)
#define DBGC0(L)
#endif



#ifdef _DEBUG
#define FDBGC9(B, L, p0, p1, p2, p3, p4, p5, p6, p7, p9) if(B){_ftprintf(stdout, _T( L "\r\n" ), p0, p1, p2, p3, p4, p5, p6, p7, p9);}
#define FDBGC8(B, L, p0, p1, p2, p3, p4, p5, p6, p7) if(B){_ftprintf(stdout, _T( L "\r\n" ), p0, p1, p2, p3, p4, p5, p6, p7);}
#define FDBGC7(B, L, p0, p1, p2, p3, p4, p5, p6) if(B){_ftprintf(stdout, _T( L "\r\n" ), p0, p1, p2, p3, p4, p5, p6);}
#define FDBGC6(B, L, p0, p1, p2, p3, p4, p5) if(B){_ftprintf(stdout, _T( L "\r\n" ), p0, p1, p2, p3, p4, p5);}
#define FDBGC5(B, L, p0, p1, p2, p3, p4) if(B){_ftprintf(stdout, _T( L "\r\n" ), p0, p1, p2, p3, p4);}
#define FDBGC4(B, L, p0, p1, p2, p3) if(B){_ftprintf(stdout, _T( L "\r\n" ), p0, p1, p2, p3);}
#define FDBGC3(B, L, p0, p1, p2) if(B){_ftprintf(stdout,( _T( L "\r\n" ) ), p0, p1, p2);}
#define FDBGC2(B, L, p0, p1) if(B){_ftprintf(stdout, _T( L "\r\n" ), p0, p1);}
#define FDBGC1(B, L, p0) if(B){_ftprintf(stdout, _T( L "\r\n" ), p0);}
#define FDBGC0(B, L) if(B){_ftprintf(stdout, _T( L "\r\n" ) );}
#else
#define FDBGC9(B, L, p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define FDBGC8(B, L, p0, p1, p2, p3, p4, p5, p6, p7)
#define FDBGC7(B, L, p0, p1, p2, p3, p4, p5, p6)
#define FDBGC6(B, L, p0, p1, p2, p3, p4, p5)
#define FDBGC5(B, L, p0, p1, p2, p3, p4)
#define FDBGC4(B, L, p0, p1, p2, p3)
#define FDBGC3(B, L, p0, p1, p2)
#define FDBGC2(B, L, p0, p1)
#define FDBGC1(B, L, p0)
#define FDBGC0(B, L)
#endif

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

inline int64_t U64_i64(uint64_t rhs)
{
	_ASSERTE(rhs <= INT64_MAX);
	if (rhs > INT64_MAX)
	{
		throw MGCORE::mgexceptionbase(E_OVERFLOW);
	}

	return static_cast<int64_t>(rhs);
}

inline uint32_t U64_U32(uint64_t rhs)
{
	_ASSERTE(rhs <= UINT32_MAX);
	if (rhs > UINT32_MAX)
	{
		throw MGCORE::mgexceptionbase(E_OVERFLOW);
	}

	return static_cast<uint32_t>(rhs);
}

inline int32_t ST_i32(size_t rhs)
{
	_ASSERTE(rhs <= INT32_MAX);
	if (rhs > INT32_MAX)
	{
		throw MGCORE::mgexceptionbase(E_OVERFLOW);
	}

	return static_cast<int32_t>(rhs);
}

#define ST_I32(K) ST_i32(K)

#ifdef ENVIRONMENT64


#define U64_ST


inline uint32_t ST_u32(size_t rhs)
{
	_ASSERTE(rhs <= UINT32_MAX);
	if (rhs > UINT32_MAX)
	{
		throw MGCORE::mgexceptionbase(E_OVERFLOW);
	}

	return static_cast<uint32_t>(rhs);
}
#define ST_U32(K) ST_u32(K)
#define SIGNEDSIZET int64_t

#else


inline size_t u64_ST(uint64_t rhs)
{
	_ASSERTE(rhs <= UINT32_MAX);
	if (rhs > UINT32_MAX)
	{
		throw MGCORE::mgexceptionbase(E_OVERFLOW);
	}

	return static_cast<size_t>(rhs);
}
#define U64_ST(K) u64_ST(K)
#define ST_U32
#define SIGNEDSIZET int32_t

#endif


inline uint32_t is_u32(SIGNEDSIZET rhs)
{
	_ASSERTE(rhs <= INT32_MAX);
	if (rhs > INT32_MAX)
	{
		throw MGCORE::mgexceptionbase(E_OVERFLOW);
	}

	return static_cast<int32_t>(rhs);
}

#define IS_U32(K) is_u32(K)

