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

//test class 
class fixed_memory :public Ibitstream_storage
{
public:
	unsigned char * _buf;
	uint64_t        _buf_len;
	uint64_t		_current_pos;

	bool           _write;

	fixed_memory(bool write = false) :_buf(NULL), _buf_len(0), _current_pos(0), _write(write)
	{}

	virtual void read(uint32_t to_read, Ibitstream_cb * pcb)
	{
		_ASSERTE(!_write);

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
		_ASSERTE(_write);
		_ASSERTE(_current_pos <= _buf_len);
		uint64_t can_write = _buf_len - _current_pos;
		uint32_t size = to_write;

		if (to_write > can_write)
			size = static_cast<uint32_t>(can_write);;

		::memcpy(_buf + _current_pos, pbytes, size);

		_ASSERTE( (_current_pos + size) <= _buf_len);

		_current_pos += size;

		pcb->write_cb(size);
	}

	virtual bool eof() {
		
		if (_write)
			return true;

		return _current_pos >= _buf_len;
	}

	virtual void flush() {}

	virtual uint64_t get_position() const { return _current_pos; };
	virtual void     set_position(uint64_t rhs) {
		_current_pos = rhs;
	}
};

#define CHAR4 0x0f, 0x0f, 0x01, 0x02 

//////////INTERNAL TEST FUNCTION/////////
int check_char4_bitstream(MGCORE::IBitstream & bs);
int check_bitset_bitream(MGCORE::IBitstream & bs);
void fill_bitset(unsigned char(&mem)[4][4]);
int transfer_bits(MGCORE::IBitstream & ins, MGCORE::IBitstream & outs);
int compare_buffer(const unsigned char * src, const unsigned char * dst, uint32_t size);



int test_fixed_memory();
int test_bitset();

