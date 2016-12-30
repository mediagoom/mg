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

#include "../exception.h"
#include <uv.h>
#include "thread.h"

#define __UV_BEGIN_NAMESPACE namespace mg {namespace uv{
#define __UV_END_NAMESPACE     }}

#define MGUV ::mg::uv

__UV_BEGIN_NAMESPACE


#define UVCHECK(num){ if (num < 0){ throw MGCORE::mgexception(ST_U32(num), uv_strerror(ST_U32(num)), _T(__FILE__), __LINE__); } }



class uvloop
{

	 uv_loop_t _loop;

	 uvloop(uvloop &rhs);
	 uvloop& operator=(uvloop &rhs);

	 void init() {
		 int r = uv_loop_init(&_loop);
		 //_loop.idle_handles
		 UVCHECK(r);

		 _loop.data = reinterpret_cast<void *>(0x1);
	 }

	 

public:

	uvloop() 
	{
		init();
	}

	void close()
	{
		if (_loop.data)
		{
			int r = uv_loop_close(&_loop);
			_ASSERTE(r != UV_EBUSY);
			//UVCHECK(r);
		}

		_loop.data = NULL;
	}

	
	virtual ~uvloop() { close(); }

	bool run(uv_run_mode mode = uv_run_mode::UV_RUN_ONCE)
	{
		int r = uv_run(&_loop, mode);
		UVCHECK(r);

		return (0 < r) ? false : true;
	}

	void stop()
	{
		uv_stop(&_loop);
	}

	uv_loop_t * get_loop() {
		return &_loop;
	}

	bool alive()
	{
		int n = uv_loop_alive(&_loop);

		return (0 == n) ? false : true;
	}

};

typedef MGCORE::CResource<uvloop> loop;

class uvloopthread : public MGCORE::cthread
{
	loop _loop;

	MGCORE::signal_event _w;

	uint32_t _wait_time;
	
	bool _exit;

	std::exception_ptr _last_exception;

	MGCORE::critical_section _cs;

protected:
	/*
	virtual void un_handled_exception(MGCORE::mgexceptionbase & ex)
	{
	 
		_ASSERTE(false);
		std::cout << ex.get_error_number() << std::endl;
		throw;
	}
	*/

	virtual void run();
	
	loop _get_loop()
	{
		//_ASSERTE(_loop);
		return _loop;
	}
public:

	MGCORE::critical_section & loop_lock() { return _cs; }

	operator uv_loop_t *() { /*_ASSERTE(_loop);*/ return _loop->get_loop(); }

	uv_loop_t * get_loop() { return operator uv_loop_t *(); }

	uvloopthread():_wait_time(100)
		, _exit(false)
	{
		_loop.Create();
	}

	void set_wait_time(uint32_t rhs) { _wait_time = rhs; }

	virtual void stop() { _loop->stop(); _exit = true;}

	void loop_now() {_w.signal();}

	void check_exception()
	{
		if (_last_exception != nullptr)
		{
			std::exception_ptr pt = _last_exception;
			_last_exception = nullptr;

			std::rethrow_exception(pt);
		}
	}

	void end()
	{		
		if (!_exit)
			stop();

		join();

		check_exception();
	}
	
	virtual ~uvloopthread()
	{
		end();
	}

	void set_exception(std::exception_ptr & eptr) { _last_exception = eptr; }
};

typedef MGCORE::CResource<uvloopthread> loopthread;

__UV_END_NAMESPACE

