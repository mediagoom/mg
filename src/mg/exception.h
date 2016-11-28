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
#include "alxstring.h"

#pragma once

__MGCORE_BEGIN_NAMESPACE

class mgexception: public mgexceptionbase
{

	Cstring _file;
	int     _line;
	
	Cstring _msg;
	CstringT<char> _what; //TODO: test unicode
	
	void update_what()
	{
	      _what  = _T("Exception: ");
	      _what += this->get_error_number();
	      _what += _T("\r\n");
	      _what += _msg;
	      _what += _T("\r\n");
	      _what += _file;
	      _what += _T(":");
	      _what += _line;

	}

public:

    mgexception(int err_num, const TCHAR* file, int line)
    	:mgexceptionbase(err_num)
		, _file(file)
		, _line(line)
    {
	update_what();
    }

	mgexception(int err_num, const TCHAR* pmsg, const TCHAR* file, int line)
		:mgexceptionbase(err_num)
		, _file(file)
		, _line(line)
		, _msg(pmsg)
	{
		update_what();
	}

    virtual Cstring toString() const
    {
      return _what;
    }

    virtual const char * what() const noexcept
    {
    	return _what;
    }

	void set_message(const TCHAR* pmsg)
	{
		_msg = pmsg;
	}

};



#define MGCHECK(num){ if (num < 0){ throw MGCORE::mgexception(num, _T(__FILE__), __LINE__); } }
#define ALXTHROW_T(msg) { throw MGCORE::mgexception(-101, msg, _T(__FILE__), __LINE__ ); }
#define ALXTHROW(M)     {  throw MGCORE::mgexception(E_BASE_CUSTOM, (_T(M)), (_T(__FILE__)), __LINE__); }


__MGCORE_END_NAMESPACE

