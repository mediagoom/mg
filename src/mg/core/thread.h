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
#include <uv.h>

__MGCORE_BEGIN_NAMESPACE

class Ilock
{
public:
	virtual void lock() = 0;
	virtual void unLock() = 0;
};

class signal_event;

class critical_section: public Ilock
{
	friend signal_event;

private:
		uv_mutex_t _mutex;
protected:

public:
		
	virtual void lock();

	virtual bool try_lock();

	virtual void unLock();
		
	critical_section();

	virtual ~critical_section();

};

class signal_event
{
private:
	critical_section _crit_sec;
	bool             _signed;
	uv_cond_t        _cond_t;
public:

	signal_event();

	virtual ~signal_event();

	virtual void signal();
	virtual void wait();
	
	virtual bool wait(uint32_t millyseconds);

	virtual bool is_signaled();

	virtual void reset();
};

class auto_lock
{
	Ilock & _lock;
	public:
		auto_lock(Ilock &lock):_lock(lock)
		{
			_lock.lock();
		}

		~auto_lock()
		{
			_lock.unLock();
		}
};

class cthread
{
	uv_thread_t _thread_t;
	bool _joined;
private:

	signal_event _event;
	signal_event _ended;

	static void startThread(void* thread);

	void _run()
	{
		run();

		_ended.signal();
	}

protected:
	virtual void run() = 0;

	bool should_stop(uint32_t millyseconds = 0)
	{
		return _event.wait(millyseconds);
	}

public:
	virtual void start();
	virtual void stop();

	bool is_done(uint32_t millyseconds = 0) { return _ended.wait(millyseconds); }

	void join();	

	cthread() :_joined(false) {}

};

__MGCORE_END_NAMESPACE

