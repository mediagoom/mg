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

#include <mgcore.h>

#ifndef MG_ERRROR_LOG
#define MG_ERROR(err) throw CMediaParserErr (_T(__FILE__), __LINE__, err);
#define MG_ERROR1(err, K) throw CMediaParserErr (_T(__FILE__), __LINE__, err);
#else
#define MG_ERROR(err) fprintf(stdout, "MG ERROR %s %s %d\r\n", err, _T(__FILE__), __LINE__);
#define MG_ERROR1(err, K) fprintf(stdout, "MG ERROR %s %s %d %d\r\n", err, _T(__FILE__), __LINE__, K);

#endif

#define MG_WARNING(err) fprintf(stdout, "MG WARNING %s %s %d\r\n", err, _T(__FILE__), __LINE__); 


class CMediaParserErr : public mgexception
{
public:
	CMediaParserErr(const TCHAR* sFile, int iLine, const TCHAR* sDescription) : mgexception(E_INVALID_INPUT, sDescription, sFile, iLine)
	{}

};

