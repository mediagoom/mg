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

#include "uvbase.h"
#include "thread.h"

#ifndef _WIN32
#include <pthread.h>
#endif

__MGCORE_BEGIN_NAMESPACE

void uv_thread_start_cb(void* arg)
{

}


void critical_section::lock()
{
	uv_mutex_lock(&_mutex);
}

bool critical_section::try_lock()
{
   if(0 == uv_mutex_trylock(&_mutex))
   	return true;

	return false;
}

void critical_section::unLock()
{
	uv_mutex_unlock(&_mutex);
}

critical_section::critical_section()
{
#ifdef _WIN32
	int n = uv_mutex_init(&_mutex);
	UVCHECK(n);
#else

pthread_mutexattr_t attr;
int err;

if (pthread_mutexattr_init(&attr))
abort();

if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE))
abort();

err = pthread_mutex_init(&_mutex, &attr);

if (pthread_mutexattr_destroy(&attr))
abort();

	UVCHECK(err);

#endif
}

critical_section::~critical_section()
{
	uv_mutex_destroy(&_mutex);
}


signal_event::signal_event() :_signed(false)
{
	int n = uv_cond_init(&_cond_t);
	UVCHECK(n);
}

signal_event::~signal_event()
{
	uv_cond_destroy(&_cond_t);
}

void signal_event::signal()
{
	if (_signed)
		return;

	_signed = true;
	//free anyone waiting
	uv_cond_signal(&_cond_t);

}

void signal_event::reset()
{
	_signed = false;
}

void signal_event::wait()
{
	if (_signed)
		return;
	
	_crit_sec.lock();
	uv_cond_wait(&_cond_t, &_crit_sec._mutex);
	_crit_sec.unLock();
}

bool signal_event::wait(uint32_t millyseconds)
{
	if (_signed)
	   return true;

	double t = millyseconds;
	         t *= 1e6;
	
	
	bool work = true;

	
	if(!_crit_sec.try_lock())
		return false;
	int n = uv_cond_timedwait(&_cond_t, &_crit_sec._mutex, (uint64_t)t);
	_crit_sec.unLock();
	if(0 == n)
	  return (_signed);

	if(UV_ETIMEDOUT == n)
	  return false;

	UVCHECK(n);

	return false;

}

bool signal_event::is_signaled()
{
	/*
	if (_signed)
		return true;
	
	int n = uv_cond_timedwait(&_cond_t, &_crit_sec._mutex, 0);

	if (0 == n)
		return true;

	if (UV_ETIMEDOUT == n)
		return false;

	UVCHECK(n);

	return false;
	*/

	return wait(0);
}

void cthread::startThread(void* thread)
{
	cthread *thethread = reinterpret_cast<cthread*>(thread);
	/*********************************************/
	thethread->_run();
	/*********************************************/
}

void cthread::start()
{
	int n = uv_thread_create(&_thread_t, startThread, this);
	UVCHECK(n);
}

void cthread::stop()
{
	_event.signal();
}

void cthread::join()
{
	if (_joined)
		return;

	int n = uv_thread_join(&_thread_t);
	UVCHECK(n);

	_joined = true;
}
__MGCORE_END_NAMESPACE

