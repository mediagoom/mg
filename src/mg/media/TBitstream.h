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

#include <mgcore.h>

// buffer size, in bytes
//#define DEFAULT_TBS_BUF_LEN 8  

using namespace MGCORE;


#define UI64(x, y)   ((uint64_t)x) << 32 | (uint64_t)y
#define UI64BE(x, y) ((uint64_t)y) << 32 | (uint64_t)x

#define I64(x, y)   ((int64_t)x) << 32 | (int64_t)y

typedef IBitstream IBitstream3;

#ifndef _WIN32

#ifndef BYTE 
#define BYTE unsigned char
#endif

#endif

class fixed_memory_read :public Ibitstream_storage
{
private:
	const unsigned char * _buf;
	uint64_t              _buf_len;
	uint64_t	          _current_pos;
	
public:

	fixed_memory_read(const unsigned char * pbuf
		, uint32_t buf_len) :_buf(pbuf), _buf_len(buf_len)
		, _current_pos(0)
	{}

	virtual void read(uint32_t to_read, Ibitstream_cb * pcb)
	{

		uint32_t possible_read = to_read;
		if (_buf_len - _current_pos < possible_read)
			possible_read = static_cast<uint32_t>(_buf_len - _current_pos);

		uint64_t pos = _current_pos;
		_current_pos += possible_read;
		//callback
		pcb->read_cb(_buf + pos, possible_read);
	}

	virtual void write(const unsigned char * pbytes, uint32_t to_write, Ibitstream_cb * pcb)
	{
		_ASSERTE(false);

		MGCHECK(-1);
	}


	virtual bool eof() {
		
		return _current_pos >= _buf_len;
	}

	virtual void flush() {}

	virtual uint64_t get_position() const {
		return _current_pos;
	}
	virtual void     set_position(uint64_t rhs) {
		_ASSERTE(rhs < _buf_len);
		_current_pos = rhs;
	}
};

class fixed_memory_bitstream : public bitstream<fixed_memory_read>
{
	CResource<fixed_memory_read> _read;

public:

	fixed_memory_bitstream(const unsigned char * pbuf, uint32_t buf_len)
		: _read(new fixed_memory_read(pbuf, buf_len))
	{
		bitstream<fixed_memory_read>::create(_read, buf_len);
	}
};

typedef fixed_memory_bitstream FixedMemoryBitstream;

class fixed_memory_write :public Ibitstream_storage
{

private:
	CBuffer<unsigned char>      _buffer;
	ResetBuffer<unsigned char>  _reset;
	uint64_t	                _current_pos;
	uint64_t	                _size;

	void align_size()
	{
		if (_current_pos >= _size)
			_size = _current_pos /*+ 1*/;
	}


public:

	fixed_memory_write(uint32_t init_size) : _buffer(init_size)
		, _reset(_buffer)
		, _current_pos(0), _size(0)
	{}

	virtual void read(uint32_t to_read, Ibitstream_cb * pcb)
	{

		uint32_t possible_read = to_read;
		if (_size - _current_pos < possible_read)
			possible_read = static_cast<uint32_t>(_size - _current_pos);

		uint64_t pos = _current_pos;
		_current_pos += possible_read;
		align_size();

		//callback
		pcb->read_cb(_buffer.get() + pos, possible_read);

		
	}

	virtual void write(const unsigned char * pbytes, uint32_t to_write, Ibitstream_cb * pcb)
	{
		uint64_t can_write = _buffer.getSize() - _current_pos;

		if (can_write <= to_write)
		{
			_buffer.prepare(static_cast<uint32_t>((to_write - can_write) + _buffer.getSize() + 100));
		}

		can_write = _buffer.getSize() - _current_pos;

		_ASSERTE(can_write > to_write);

		memcpy(_buffer.get() + _current_pos, pbytes, to_write);

		_current_pos += to_write;
		align_size();

		pcb->write_cb(to_write);



		_buffer.updatePosition(to_write);
		_ASSERTE(_buffer.getFull() == _current_pos);

		
	}

	virtual bool eof() {
		return (_current_pos /*+ 1*/) >= _size;
	}

	virtual void flush() {}

	virtual uint64_t get_position() const {
		return _current_pos;
	}

	virtual void     set_position(uint64_t rhs) {
		_ASSERTE(rhs < _buffer.getSize());
		_current_pos = rhs;

		if (_current_pos < _buffer.getFull())
			_reset.Back(_buffer.getFull() - _current_pos);
		else
			_buffer.updatePosition(_current_pos - _buffer.getFull());

		_ASSERTE(_buffer.getFull() == _current_pos);
	    
	}

	unsigned char * get_buffer() {
		return _buffer.get();
	}

	uint64_t size() const { return _size; }

};

class write_memory_bitstream : public bitstream<fixed_memory_write>
{
	CResource<fixed_memory_write> _write;

public:

	write_memory_bitstream(uint32_t buf_len, uint32_t size = DEFAULT_FILE_PAGE)
		: _write(new fixed_memory_write(buf_len))
	{
		bitstream<fixed_memory_write>::create(_write, size, false);
	}

	uint64_t get_size()
	{
		return _write->size();
	}

	unsigned char * get_buffer() {
		return _write->get_buffer();
	}

	fixed_memory_write & storage()
	{
		return _write.GetRef();
	}
};

typedef write_memory_bitstream WriteMemoryBitstream;

/*!
This class is an internal class not intended to be used directly.
*/
class _file_bitstream : public bitstream<file_bitstream>
{
	
	CResource<file_bitstream> _stream;
	uint32_t _size;
	

public:

	_file_bitstream(::mg::uv::loopthread & loop, uint32_t size = DEFAULT_FILE_PAGE)
		: _stream(new file_bitstream(loop))
		, _size(size)
	{}

	void open(const TCHAR * path, int flags = O_RDONLY | O_EXCL) 
	{ 
		_stream->open_sync(path, flags);
		bool load = (!(flags & O_WRONLY));
		bitstream<file_bitstream>::create(_stream, _size, load);
	}

	void close() { _stream->close(false); }

};

/*!
This class is an internal class not intended to be used directly.
*/
class _sync_file_bitstream : public bitstream<sync_file_bitstream>
{

	CResource<sync_file_bitstream> _stream;
	uint32_t _size;


public:

	_sync_file_bitstream(uint32_t size = DEFAULT_FILE_PAGE)
		: _stream(new sync_file_bitstream)
		, _size(size)
	{}

	void open(const TCHAR * path, bool read = true)
	{
		_stream->open(path, read);
		bitstream<sync_file_bitstream>::create(_stream, _size, read);
	}

	void close() { _stream->close(); }
	void flush() { 
		bitstream<sync_file_bitstream>::flush();
		_stream->flush(); 
	}

};

template<class T> 
class base_file_midia_bitstream : public T
{
public:
	virtual void open(const TCHAR * path) = 0;
	virtual void close() = 0;

	virtual ~base_file_midia_bitstream()
	{}
};

/*! 
This class is a generic implementation for MP4File, TSFile etc..
*/
template<class T, int F = O_RDONLY | O_EXCL>
class file_media_bitstream : public base_file_midia_bitstream<T>
{
	_file_bitstream _fb;
	bool  _load;

public:

	file_media_bitstream(::mg::uv::loopthread & loop, uint32_t size = DEFAULT_FILE_PAGE)
		: _fb(loop, size)
	{

	}

	virtual ~file_media_bitstream() 
	{}

	void open(const TCHAR * path, int flags)
	{
		_load = (F & O_WRONLY);
		_fb.open(path, flags);

		T::_p_f = &_fb;
	}

	virtual void open(const TCHAR * path)
	{
		open(path, F);
	}

	virtual void close()
	{
		if(_load)
			_fb.flush();
		_fb.close();
	}

};

//
//template<class T>
//class file_media_bitstream_write : public T
//{
//	_file_bitstream _fb;
//
//public:
//
//	file_media_bitstream_write(::mg::uv::loopthread & loop, uint32_t size = DEFAULT_FILE_PAGE)
//		: _fb(loop, size)
//	{
//
//	}
//
//	void open(const TCHAR * path, int flags = O_CREAT | O_WRONLY | O_TRUNC)
//	{
//		_fb.open(path, flags);
//
//		T::_p_f = &_fb;
//	}
//
//	void close() 
//	{ 
//		_fb.flush();
//		_fb.close(); 
//	}
//};



template<class T>
class memory_write_media_bitstream : public T
{
	write_memory_bitstream * _write;

public:

	memory_write_media_bitstream() :_write(NULL)
	{

	}

	void close()
	{
		if (NULL != _write)
			delete _write;

		_write = NULL;
	}

	void open(uint32_t initial_size)
	{
		_ASSERTE(NULL == _write);
		close();

		_write = new write_memory_bitstream(initial_size);

		T::_p_f = _write;

	}

	uint64_t get_size() const { return _write->get_size(); }
	const unsigned char * get_buffer() const { return _write->get_buffer(); }

	

	virtual ~memory_write_media_bitstream()
	{
		close();
	}

	void flush() { _write->flush(); }

	fixed_memory_write & storage() { return _write->storage(); }

};

template<class T, bool K = true>
class sync_file_media_bitstream : public base_file_midia_bitstream<T>
{
	_sync_file_bitstream _fb;
	bool _load;

public:

	sync_file_media_bitstream(::mg::uv::loopthread & loop, uint32_t size = DEFAULT_FILE_PAGE)
		: _fb(size)
	{

	}
	
	
	sync_file_media_bitstream(uint32_t size = DEFAULT_FILE_PAGE)
		: _fb(size)
	{

	}

	virtual ~sync_file_media_bitstream()
	{}

	void open(const TCHAR * path, bool read)
	{
		_load = (!read);
		_fb.open(path, read);

		T::_p_f = &_fb;
	}
	
	void flush() { _fb.flush(); }

	virtual void open(const TCHAR * path)
	{
		open(path, K);
	}

	virtual void close() 
	{
		if (_load)
			_fb.flush();
		_fb.close(); 
	}

	
};

