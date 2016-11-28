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

#include "cresource.h"
#include <functional>
#include <exception>

__MGCORE_BEGIN_NAMESPACE

template<class T>
class CPromise
{
	
	std::function<void(CResource<T> & pres)> _cb;
	std::function<bool(std::exception_ptr eptr)> _er;

	CResource<CPromise<T> > _forward;

	CPromise<T> & operator=(CPromise<T> & rhs);
	CPromise<T>(CPromise<T> & rhs);

	//store value in case you are called before function setting
	CResource<T>        _value;
	std::exception_ptr  _eptr;

	critical_section    _cs;

public:
	
	
	//CPromise<T> & set_cb(std::function<void(CResource<T> & pres)> cb) { _cb = cb; return (*this); }

	CPromise<T> & set_er()
	{
		return set_er([](std::exception_ptr eptr) -> bool
			{ 
				std::rethrow_exception(eptr); 
				return false;
			}
		);
	}

	void call_cb(CResource<T> & pres)
	{
		auto_lock l(_cs);

		if (!_cb)
		{
		  _value = pres;
		  //std::cout << _T("call_cb race!!") << std::endl;
		  return;
		}

		try {

			_cb(pres);

			if (_forward)
				_forward->call_cb(pres);
		}
		catch (...)
		{
			call_er(std::current_exception());
		}
	}

	void call_er(std::exception_ptr  eptr)
	{
		auto_lock l(_cs);
	
		if (!_er)
		{
			_eptr = eptr;
			return;
		}

		bool process_further = _er(eptr);

		if (!process_further)
			return;

		if (_forward)
			_forward->call_er(eptr);
	}

	CResource<CPromise<T> > forward(bool auto_set_er = false)
	{
		auto_lock l(_cs);

		if (!_forward)
			_forward.Create();

		if(auto_set_er)
			_forward->set_er();

		return _forward;
	}

	CPromise<T>(){}
	
	CPromise<T> & set_cb(const std::function<void(CResource<T> & pres)> & cb) 
	{ 
		auto_lock l(_cs);

		_ASSERTE(!_cb);

		_cb = cb; 
		
		if (_value)
			call_cb(_value);

		return (*this); 
	}
	CPromise<T> & set_er(const std::function<bool(std::exception_ptr eptr)> & er) 
	{ 
		auto_lock l(_cs);

		//_ASSERTE(!_er);

		_er = er; 

		if (_eptr)
			call_er(_eptr);

		return (*this);
	}

	bool has_value() { return (_value) ? true : false; }

};


__MGCORE_END_NAMESPACE

