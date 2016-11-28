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

#include <iostream>
#include <mgmedia.h>

using namespace MGCORE;

#include "test_base.h"
#include "test_thread.h"


int test_thread_locking()
{
	test_lock t;

	bool s = t._ended.is_signaled();
	CHECK(false, s, _T("thread status not correct"));

	t.start();

	int prog = 0;

	while(!t._ended.wait(10))
	{
		auto_lock l(t._lock);

		if(  prog < (t._progressive))
		{
			if(t._progressive != 2)
				CHECK(1, t._progressive - prog, _T("invalid progression"));
			
			prog = ++t._progressive;
		}

		/*std::cout << "<< " << prog << std::endl;

		if(t._progressive > 98)
		{
			std::cout << "insert break mdthread.cpp:72" << std::endl;
		}
		*/

	}

	t.join();

	//std::cout << "<< ended" << std::endl;
	
	TEST_OK;
}

int test_thread_signaling()
{
	test_th t;

	t.start();
	bool s = t._started.wait(10000); //make sure the thread is correctly running.
	CHECK(true, s, _T("thread not started correctly"));


	//CHECK(false, t._im_running.is_signaled(), _T("thread is not waiting"));
	CHECK(0, t._count, _T("thread is not waiting correctly"));

	for (int i = 0; i < 10; i++)
	{
		
		//CHECK(false, t._im_running.is_signaled(), _T("thread is not running"));
		CHECK(i, t._count, _T("thread is not waiting correctly"));
		
		//std::cout << "signal _run_thread " << std::endl;

		t._run_thread.signal();

		CHECK(true, t._run_thread.is_signaled(), _T("signaling not working correctly"));

		//std::cout << "start wait" << std::endl;

		s = t._im_running.wait(10000);
		CHECK(true, s, _T("deadlock"));


                t._im_running.reset();
		
		

	}

	t.stop(); //signal the threads to exit
	s = t._ended.wait(10000); //make sure the thread has correctly ended.

	t.join();

	CHECK(true, s, _T("thread failed ending"));
	CHECK(true, t._exited, _T("thread failed to exit correctly"));

	TEST_OK;
}

int test_signal_event()
{
	signal_event se;

	bool res = se.is_signaled();

	CHECK(false, res, "startup is signaled");

	se.signal();

	res = se.is_signaled();

	CHECK(true, res, "signal() not working");

	res = se.is_signaled();

	CHECK(true, res, "signal() wrongly autorest");

	TEST_OK;
}



