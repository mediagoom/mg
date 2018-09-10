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

class test_th : public cthread
{
public:

	int          _pre_count;
	int          _count;
	
	signal_event _run_thread;
	signal_event _im_running;

	signal_event _auto_wait;
        
	signal_event _started;
	signal_event _ended;

	bool _exited;
	bool _success;

	test_th() :_count(0), _pre_count(0), _exited(false), _success(false)
	{
	
		//_started.reset();
		//_ended.reset();
	}


protected:

	virtual void run()
	{
		_success = true;

		_started.signal();

		_run_thread.wait();
		_run_thread.wait(); //check double wait auto-reset is false

		while (!this->should_stop())
		{
		//	_im_running.reset();

			/*
			std::cout << "_run_thread: " << _run_thread.is_signaled() << std::endl;
			std::cout << "_run_thread1: " << _run_thread.is_signaled() << std::endl;
			std::cout << "_run_thread2: " << _run_thread.wait(0) << std::endl;
			std::cout << "_run_thread3: " << _run_thread.wait(0) << std::endl;
			std::cout << "_run_thread4: " << _run_thread.wait(5) << std::endl;
			*/

			     if(!_run_thread.wait(10000))
			     {
				     std::cout << "failed to run" << std::endl;
				     _success = false;
				     break;
			     }


			     _count++;
		
			     _run_thread.reset();
				 _im_running.signal(); //free up waiting thread

			     //make sure the other thread has time to run
				 if (_auto_wait.wait(500))
			     {
				     std::cout << "failed to auto-wait" << std::endl;
				     _success = false;
				     break;
			     }
				
			

		}

		_exited = true;
		_ended.signal();
	
	}
};

class test_lock : public cthread
{
public:
	critical_section _lock;
	int _progressive;

	signal_event _ended;

    test_lock():_progressive(0)
    {}

protected:

	virtual void run()
	{
		
		_progressive = 1;
		int last = 0;

		while(100 > _progressive)
		{
		
			//auto-wait
			_ended.wait(10);
			auto_lock l(_lock);
		
			if(last <  _progressive)
			{
				//CHECK(1, _progressive - last, _T("invalid progression worker"));
				_ASSERTE(1 == (_progressive - last) || _progressive == 2);
				last = ++_progressive;
				
			}

			//std::cout << ">> " << _progressive << " " << last << std::endl;

		
		}

		std::cout << ">> ending" << std::endl;

		_ended.signal();

		return;
	}
};
int test_signal_event();
int test_thread_signaling();
int test_thread_locking();

