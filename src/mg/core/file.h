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

#include "bitstream.h"
#include "thread.h"
#include "uvbase.h"

#include "../promises.h"

#include <fcntl.h>

#include <stdio.h>

#include <forward_list>

__MGCORE_BEGIN_NAMESPACE




struct struct_file_cb: CPromise<uv_fs_t>
{
	CResource<uv_fs_t>   _uv_fs_t;
	
	uint32_t             _sequence;

#ifdef _DEBUG
	uint32_t*			 _p_arrive;
	uint64_t             _expt;
#endif

	uint32_t			* _p_served;
	//uint32_t            * _p_outstanding;

	std::forward_list<struct_file_cb *> * _p_forward;
};

class cfile
{
	
	MGUV::loopthread _loop;

	static void uv_file_cb(uv_fs_t* req);

	//static void process_cb(struct_file_cb * fcb);

	Cstring   _path;

	uv_stat_t _file_stat;
	uv_file   _file;

	uint64_t _position;

	std::forward_list<struct_file_cb *> _unordered_buffer;

#ifdef _DEBUG
	//uint32_t      _leave_r_sequence;
	uint32_t      _arrive_r_sequence;
	uint32_t      _leave_wr_sequence;
	uint32_t      _arrive_wr_sequence;
#endif

	uint32_t            _outstanding;
	uint32_t            _served;

	signal_event        _ended;

	 void do_outstanding(struct_file_cb * fcb)
	 {
		 fcb->_p_served = &_served;
		 //fcb->_p_outstanding = &_outstanding;

		 //(*fcb->_p_outstanding)++;
		 fcb->_sequence = _outstanding;
		 fcb->_p_forward = &_unordered_buffer;

		 _outstanding++;
		 //_ASSERTE(fcb->_p_outstanding >= 0);
	 }
protected:
	void loop_now() {
		_loop->loop_now();
	}
public:
	cfile(MGUV::loopthread & loop);

	~cfile();

	CResource<CPromise<uv_fs_t> > open(const TCHAR* path, int flags = O_RDONLY | O_EXCL, bool async = true );
	void open_sync(const TCHAR * path, int flags = O_RDONLY | O_EXCL) { open(path, flags, false); }
	void open_write_sync(const TCHAR * path) { open_sync(path, O_CREAT | O_WRONLY | O_TRUNC); }
	
	CResource<CPromise<uv_fs_t> > write(const unsigned char * p_buffer, uint32_t size);

	CResource<CPromise<CBuffer< unsigned char  > > > read(uint32_t size);

	uint64_t size() const { 
		
		if (_position >= _file_stat.st_size && 0 < _position) //we are writing
			return _position /*+ 1*/;

		return _file_stat.st_size; 
	}

	CResource<CPromise<uv_fs_t> > close(bool async = true);

	virtual uint64_t get_position() const { return _position; }
	virtual void     set_position(uint64_t rhs) { _position = rhs; }

	virtual bool eof() {
		return (size())?(get_position() >= ( size() /*-1*/ )):true;
	}

	virtual int32_t outstanding() const { return _outstanding - _served; }
	
};

class file_bitstream : public Ibitstream_storage, public cfile
{
public:

	file_bitstream(::mg::uv::loopthread & loop) :cfile(loop)
	{}

	virtual void read(uint32_t to_read, Ibitstream_cb * pcb)
	{
		cfile::read(to_read)->set_er().set_cb(
			[pcb](CResource<CBuffer<unsigned char> > & buf) mutable
		{
			pcb->read_cb(buf->get(), ST_U32(buf->size()));
		});
	}

	virtual bool eof() {
		return cfile::eof();
	}

	virtual void write(const unsigned char * pbytes, uint32_t to_write, Ibitstream_cb * pcb)
	{
		cfile::write(pbytes, to_write)->set_er().set_cb(
			[pcb](CResource<uv_fs_t> & r)
		{
			pcb->write_cb(ST_U32(r->result));
		}
		);
	}
	
	virtual void flush()
	{}

	virtual uint64_t get_position() const { return cfile::get_position(); }
	virtual void     set_position(uint64_t rhs) { cfile::set_position(rhs); }

	virtual void idle() { this->loop_now(); };
};

class sync_file_bitstream : public Ibitstream_storage
{
	FILE * _f;
	CBuffer<unsigned char> _buffer;

	uint64_t _size;

public:

	sync_file_bitstream():_f(NULL), _buffer(64 * 1024)
	{
		
	}

	void open(const TCHAR * pszName, bool read = true)
	{
		_f = ::fopen(pszName, (read) ? "rb" : "wb");
		if (NULL == _f)
		{
			MGCHECK(-1);
		}
		
		uv_fs_t req;

		int r = uv_fs_stat(uv_default_loop(), &req, pszName, NULL); //sinc
		UVCHECK(r);

		_size = req.statbuf.st_size;
		
	}

	sync_file_bitstream(const TCHAR * pszName, bool read = true)
		:_buffer(64 * 1024)
	{
		open(pszName, read);
	}


	size_t read(uint32_t to_read, unsigned char * pbyte)
	{
		return ::fread(pbyte, 1, to_read, _f);
	}

	
	virtual void read(uint32_t to_read, Ibitstream_cb * pcb)
	{
		_buffer.prepare(to_read);

		size_t r = ::fread(_buffer.get(), 1, to_read, _f);

		pcb->read_cb(_buffer.get(), ST_U32(r));
	}

	virtual bool eof() {
		return (0 != feof(_f));
	}

	size_t write(const unsigned char * pbytes, uint32_t to_write)
	{
		return ::fwrite(pbytes, 1, to_write, _f);
	}

	virtual void write(const unsigned char * pbytes, uint32_t to_write, Ibitstream_cb * pcb)
	{
		//size_t r = ::fwrite(pbytes, 1, to_write, _f);
		size_t r = write(pbytes, to_write);

		pcb->write_cb(ST_U32(r));
	}

	virtual void flush()
	{
		size_t r = ::fflush(_f);
	}

	virtual uint64_t get_position() const { return ::ftell(_f); }

	virtual void     set_position(uint64_t rhs) {
		::fseek(_f, static_cast<uint32_t>(rhs), SEEK_SET);
	}

	void close()
	{
		if(_f)
			::fclose(_f);

		_f = NULL;
	}

	virtual ~sync_file_bitstream()
	{
		close();
	}

	virtual uint64_t size()
	{ 
		return _size;		
	}
};

__MGCORE_END_NAMESPACE


