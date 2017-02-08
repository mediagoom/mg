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
#include "bitstream.h"

#define COPY_STREAM_DEBUG 0
#define COPY_STREAM_WAIT 0

#define SMALL_WAIT 50

__MGCORE_BEGIN_NAMESPACE

class _ccopy_stream :public Ibitstream_cb
{
	signal_event         _read_event;
	signal_event         _write_event;

	

	uint32_t _read;
	uint32_t _write;
	uint32_t _traveling;
	uint32_t _max_out;


	bool _wait_end;

	void wait(signal_event & ev) {

		if (!ev.wait(TIMEOUT))
			if (!ev.wait(MICROTIMEOUT))
				throw Timeout(__FILE__, __LINE__);
	}

public:

	

	Ibitstream_storage & _destination;
	
	void wait_read()
	{
		while (_read - _write > _max_out)
		{
			_read_event.reset();
			FDBGC3(COPY_STREAM_WAIT, "wait-read\t%u\t%u\t%u", _read, _write, _traveling);
			//wait(_read_event);
			
			_destination.idle();

			_read_event.wait(SMALL_WAIT);
		}
	}

	void wait_end()
	{
		_wait_end = true;

		if (_read == _write)
		{
			//wait for the last call to read_cb to end
			if(!_write_event.wait(SMALL_WAIT))
			{
			   _ASSERTE(false);

		       //MGCHECK(E_TIMEOUT);
			}

			return;
		}

		while (!_write_event.wait(SMALL_WAIT))
		{

			FDBGC4(COPY_STREAM_DEBUG, "wait-end\t%u\t%u\t%u\t%u", _read, _write, _traveling, 0);

			if (_read == _write)
			{
				//wait for the last call to read_cb to end
				if (!_write_event.wait(SMALL_WAIT))
				{
					_ASSERTE(false);

					MGCHECK(E_TIMEOUT);
				}

				return;
			}

			
		}

		_write_event.wait();

	}


	_ccopy_stream(Ibitstream_storage & destination)
		:_destination(destination)
		, _read(0)
		, _write(0)
		, _traveling(0)
		, _max_out(10)
		, _wait_end(false)

	{
		_write_event.reset();
	}

	
	void read(Ibitstream_storage & source, uint32_t chunk)
	{
		_read++;

		
		source.read(chunk, this);

		
	}

	virtual void read_cb(const unsigned char * p_val, uint32_t read) override
	{

		//DBGC0(_T("-COPY-WAIT-READ-SIGNAL"));
		
		
		_traveling++;
		

		FDBGC3(COPY_STREAM_DEBUG, "read-cb\t%u\t%u\t%u", _read, _write, _traveling);

				
		_destination.write(const_cast<unsigned char*>(p_val), read, this);

		//std::cout << "read_cb" << std::endl;
	}
	
	virtual void write_cb(uint32_t written) override
	{	

		_write++;
		
		if (_read - _write <= _max_out)
		{
			_read_event.signal();
			FDBGC4(COPY_STREAM_DEBUG, "read-signal\t%u\t%u\t%u\t%u", (_read - _write), _read, _write, _traveling);
		}

		FDBGC3(COPY_STREAM_DEBUG, "write-cb\t%u\t%u\t%u", _read, _write, _traveling);

		if (_wait_end)
		{
			if (_read == _write)
			{
				FDBGC3(COPY_STREAM_WAIT, "write-signal\t%u\t%u\t%u", _read, _write, _traveling);
				_write_event.signal();
			}
		}

	}
};

void copy_stream(
	  Ibitstream_storage & source
	, Ibitstream_storage & destination
	, uint32_t chunk
	, uint64_t startpos
	, uint64_t endpos
	)
{
	
	//if (0 != startpos)
		source.set_position(startpos);

	_ccopy_stream ccs(destination);

	///ccs._write_event.reset();

	while (!source.eof() && (endpos == 0 || endpos > source.get_position()))
	{
	
		ccs.wait_read();
		
		FDBGC0(COPY_STREAM_DEBUG, "read");

		ccs.read(source, chunk);
		
	}

	
	FDBGC0(COPY_STREAM_WAIT, "ending");

	ccs.wait_end();

	FDBGC0(COPY_STREAM_WAIT, "ended");

	destination.flush();

	
}


__MGCORE_END_NAMESPACE

