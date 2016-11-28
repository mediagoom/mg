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

__UV_BEGIN_NAMESPACE

void uvloopthread::run()
{
	if (!_loop)
		 _loop.Create();

	try {

		bool do_run = true;

		while (do_run) {

			_w.reset();

			do {

				MGCORE::auto_lock l(_cs);
				do_run = _loop->run(UV_RUN_DEFAULT);

			} while (0);
			
			if ( (do_run && (!_exit)) /*|| _loop->alive()*/ )
			{
				if (!should_stop())
					_w.wait(_wait_time);
			}
			else
			{
				/*
				while (_loop->alive())
				{
					_w.wait(_wait_time);

					std::cout << "loop alive: " << _loop->alive() << std::endl;
				}
				*/
				do {

					MGCORE::auto_lock l(_cs);
					if (_loop->alive())
						_loop->run(UV_RUN_DEFAULT);

				} while (0);

				//std::cout << "ending loop thread: " << _loop->alive() << std::endl;

				MGCORE::cthread::stop();
				break;
			}
		}

	}
	//catch (MGCORE::mgexceptionbase & ex)
	catch (...)
	{
		//un_handled_exception(ex);

		std::cout << "ending loop thread handled_exception" << std::endl;

		_last_exception = std::current_exception();

		//std::cout << "last_exception:\t" << _last_exception.w
	}
}

__UV_END_NAMESPACE

