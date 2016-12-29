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

/*
#include "core.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>


#include "thread.h"
#include "char.h"
*/

#include "../exception.h"
#include <stdio.h>
#include "thread.h"

#include <type_traits>

#define TIMEOUT 10000
#define MICROTIMEOUT 1000

#ifdef _DEBUG
#define DEFAULT_FILE_PAGE 8*1024 //
#else
#define DEFAULT_FILE_PAGE 8*1024 //8k
#endif

#define BITSTREAM_READ_DEBUG 0
#define BITSTREAM_POSITION_DEBUG 0


__MGCORE_BEGIN_NAMESPACE

//typedef void(*stream_read_cb)(void * context, unsigned char * p_val, uint32_t read);

#ifndef E_END_OF_DATA
#define E_END_OF_DATA (-4095)
#endif

#ifndef E_INVALID_INPUT
#define E_INVALID_INPUT (-4071)
#endif

#ifndef E_UNKNOWN
#define E_UNKNOWN (-4094)
#endif

#ifndef E_TIMEOUT
#define E_TIMEOUT E_UNKNOWN
#endif

#ifndef E_INVALID_ALIGNEMENT
#define E_INVALID_ALIGNEMENT E_UNKNOWN
#endif

#define INVALIDELSE else{throw InvalidInput(__FILE__, __LINE__);}



class InvalidInput: public mgexception
{
public:
	InvalidInput(const TCHAR* sFile, int iLine) : mgexception(E_INVALID_INPUT, sFile, iLine)
	{}
					
};

class EndOfData : public mgexception
{
public:
	//EndOfData(void) : Error(E_END_OF_DATA) {}
	EndOfData(const TCHAR* sFile, int iLine) : mgexception(E_END_OF_DATA, sFile, iLine)
	{}
};

class Timeout : public mgexception
{
public:
	//EndOfData(void) : Error(E_END_OF_DATA) {}
	Timeout(const TCHAR* sFile, int iLine) : mgexception(E_TIMEOUT, sFile, iLine)
	{}
};

class InvalidAlignment : public mgexception
{
public:
	//EndOfData(void) : Error(E_END_OF_DATA) {}
	InvalidAlignment(const TCHAR* sFile, int iLine) : mgexception(E_TIMEOUT, sFile, iLine)
	{}
};

/*
class ReadFailed //: public Error 
{
public:
	//EndOfData(void) : Error(E_END_OF_DATA) {}
	//EndOfData(const TCHAR* sFile, int iLine) : Error(E_END_OF_DATA, sFile, iLine) {}
};
*/

class Ibitstream_cb{

public:
	virtual void read_cb(const unsigned char * p_val, uint32_t read) = 0;
	virtual void write_cb(uint32_t written) = 0;

	

	//TODO: err_cb

};

class empty_bitstream_cb: public Ibitstream_cb
{
public:
	virtual void read_cb(const unsigned char * p_val, uint32_t read) {}
	virtual void write_cb(uint32_t written) {}

};

class IBitstream {

public:
	//virtual uint64_t  getbits(short n) = 0;
	virtual void putbits(uint32_t val, short n) = 0; 
	virtual uint32_t  getbits(short n) = 0;
	virtual uint32_t  sgetbits(short n) = 0;

	// probe next n bits, do not advance
	virtual uint32_t  nextbits(short n) = 0;

	virtual void flush() = 0;
	virtual bool eof() = 0;

	virtual void skipbits(short n) = 0;
	virtual uint32_t align(int n) = 0;

	// get current bit position (both input/output)
	virtual uint64_t getpos(void) = 0;

	/* Probe next n bits (input) or return 0 (output).
	* If big=0, then the number is represented using the little-endian method; otherwise, big-endian byte-ordering.
	* If sign=0, then no sign extension; otherwise, sign extension.
	* If alen>0, then the number is probed at alen-bit boundary (alen must be multiple of 8).
	*/
	virtual uint32_t next(int n, int big, int sign, int alen) = 0;
	virtual int little_putbits(unsigned int value, int n) = 0;

	virtual uint32_t little_getbits(int n) = 0;

	virtual uint64_t get_position() const = 0;
	virtual void     set_position(uint64_t rhs) = 0;

	virtual void read(unsigned char * pbytes, uint32_t size, uint32_t * read) = 0;
	virtual void write(const unsigned char * pbyte, uint32_t size) = 0;


	/* Search for a specified code (input); returns number of bits skipped, excluding the code.
	* If alen>0, then output bits up to the specified alen-bit boundary (output); returns number of bits written
	* The code is represented using n bits at alen-bit boundary.
	*/
	virtual unsigned int nextcode(unsigned int code, int n, int alen) = 0;

	
};

class Ibitstream_storage{

public:
	virtual void read(uint32_t to_read, Ibitstream_cb * pcb) = 0;
	virtual void write(const unsigned char * pbytes, uint32_t to_write, Ibitstream_cb * pcb) = 0;
	
	virtual bool eof()   = 0;
	virtual void flush() = 0;
	
	virtual uint64_t get_position() const = 0;
	virtual void     set_position(uint64_t rhs) = 0;

	virtual void idle() {};

};


void copy_stream(Ibitstream_storage & source, Ibitstream_storage & destination
	, uint32_t chunk = DEFAULT_FILE_PAGE
	, uint64_t startpos = 0, uint64_t endpos = 0);


// tmasks for bitstring manipulation
static const unsigned int tmask[33] = {
0x00000000, 0x00000001, 0x00000003, 0x00000007,
0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
0xffffffff
};


// complement tmasks (used for sign extension)
static const unsigned int ctmask[33] = {
	0xffffffff, 0xfffffffe, 0xfffffffc, 0xfffffff8,
	0xfffffff0, 0xffffffe0, 0xffffffc0, 0xffffff80,
	0xffffff00, 0xfffffe00, 0xfffffc00, 0xfffff800,
	0xfffff000, 0xffffe000, 0xffffc000, 0xffff8000,
	0xffff0000, 0xfffe0000, 0xfffc0000, 0xfff80000,
	0xfff00000, 0xffe00000, 0xffc00000, 0xff800000,
	0xff000000, 0xfe000000, 0xfc000000, 0xf8000000,
	0xf0000000, 0xe0000000, 0xc0000000, 0x80000000,
	0x00000000
};

// sign tmasks (used for sign extension)
static const unsigned int stmask[33] = {
	0x00000000, 0x00000001, 0x00000002, 0x00000004,
	0x00000008, 0x00000010, 0x00000020, 0x00000040,
	0x00000080, 0x00000100, 0x00000200, 0x00000400,
	0x00000800, 0x00001000, 0x00002000, 0x00004000,
	0x00008000, 0x00010000, 0x00020000, 0x00040000,
	0x00080000, 0x00100000, 0x00200000, 0x00400000,
	0x00800000, 0x01000000, 0x02000000, 0x04000000,
	0x08000000, 0x10000000, 0x20000000, 0x40000000,
	0x80000000
};


/*! TODO: Type should be IBitstream_storage.
    TODO: bitstream and bitstream_base should be collapsed in the same class
*/
template<class Type>
class bitstream_base : public Ibitstream_cb
{
	
	unsigned char *_buf;     // buffer	
	uint32_t     _buf_len;	 // usable buffer size (for partially filled buffers)
	uint64_t     _cur_bit;     // current bit position in buf
    
	uint64_t  _tot_bits;     // total bits read/written
	
	CResource< Type > _p_cb;  

	uint32_t			  _buf_size;    //full buffer size

	unsigned char *_shadow_buf;     // shadow buffer
	uint32_t         _shadow_len;     // from begging to _shadow_end available to read
	bool	       _load_shadow;

	signal_event     _load_event;
	critical_section _load_lock;

	bool _load;

	bool _pending_read;

	void wait() {
		
		if (_p_cb)
		{
			_p_cb->idle();

			if (!_load_event.wait(TIMEOUT))
				if (!_load_event.wait(MICROTIMEOUT))
					throw Timeout(__FILE__, __LINE__);
		}
	}
	
	void internal_read(uint32_t size, bool async = true)
	{
		_ASSERTE(_p_cb);

		if (_p_cb->eof())
			return;

		//if (0 < _shadow_len)
		{
			//Make sure no other request are pending
			wait();
		}
		
		FDBGC2(BITSTREAM_READ_DEBUG, "INTERNAL_READ_LOAD_RESET\t%u\t%d", size, async);
		_load_event.reset();

		_pending_read = true;
		_p_cb->read(size, this);

		if (!async)
			wait();

	}
	
	void internal_write(unsigned char * pbytes, uint32_t size, bool async = true)
	{
		/*
		_ASSERTE(_p_cb);

		_ASSERTE(0 < size  );

		DBGC2("WRITE\tINTERNAL\t%d\t%d", _shadow_len, size);
		
		if (0 < _shadow_len)
		{
			//Make sure no other request are pending
			wait();
		}

		_ASSERTE(0 == _shadow_len);

		if (0 == _shadow_len)
		{
			auto_lock l(_load_lock);

			::memcpy(_shadow_buf, pbytes, size);
			_shadow_len = size;

			DBGC0("INTERNAL_WRITE_LOAD_RESET");
			_load_event.reset();

			DBGC2("WRITE\tINTERNAL-2\t%d\t%d", _shadow_len, size);

		
		}INVALIDELSE;

		_p_cb->write(_shadow_buf, _shadow_len, this);

		DBGC2("WRITE\tINTERNAL-3\t%d\t%d", _shadow_len, _shadow_len);
		
		if (!async)
			_load_event.wait();

		*/

		_p_cb->write(pbytes, size, this);
	}


	void fill_buf();

	void reload()
	{
		if (!_load)
		{
			::memset(_buf, 0, _buf_size);
			return;
		}

		wait();

		_buf_len = 0;
		_shadow_len = 0;
		_cur_bit = 0;

			//do a first sync read in _buf
			_load_shadow = false;
			internal_read(_buf_size, false);

			_load_shadow = true;
			internal_read(_buf_size, true);
	}

	void init(uint32_t buf_len, bool load = true)
	{
		_buf        = new unsigned char[buf_len];
		_buf_size   = buf_len;
        _shadow_buf = new unsigned char[buf_len];
		_shadow_len = 0;
		_buf_len    = 0;

		_cur_bit = 0;
		_load = load;

		reload();
		

	}

protected:

	// returns 'n' bits as unsigned int; does not advance bit pointer
	virtual uint32_t _nextbits(int n);
	
	// returns 'n' bits as unsigned int; advances bit pointer
	virtual uint32_t  _getbits(short n)
	{
		if (n > 32 || n < 0)
			throw InvalidInput(__FILE__, __LINE__);

		register unsigned int x = _nextbits(n);
		_ASSERTE(((_cur_bit + n) >> 3) <= _buf_len);
		_cur_bit += n;
		_tot_bits += n;
		return (x & tmask[n]);
	}

	// output the buffer excluding the left-over bits.
	virtual void _flush_buf()
	{
		uint32_t l = static_cast<uint32_t>(_cur_bit >> 3);     // number of bytes written already
		unsigned long n(0);

		internal_write(_buf, l);
				
		// are there any left-over bits?
		if (_cur_bit & 0x7) {
			_buf[0] = _buf[l];                // copy the left-over bits
			memset(_buf + 1, 0, _buf_size - 1); // zero-out rest of buffer
		}
		else memset(_buf, 0, _buf_size);    // zero-out entire buffer
		
		_cur_bit &= 7;
		
	}

	virtual void _putbits(uint32_t value, short n)
	{
		int delta;          // required input shift amount
		unsigned char *v;   // current byte
		unsigned int tmp;   // temp value for shifted bits
		unsigned int val;   // the n-bit value

		val = value & tmask[n];

		if (_cur_bit + n > (_buf_size << 3)) _flush_buf();

		delta = 32 - n - (_cur_bit % 8);
		v = _buf + (_cur_bit >> 3);
		if (delta >= 0) {
			
			tmp = val << delta;
			v[0] |= tmp >> 24;
			v[1] |= tmp >> 16;
			v[2] |= tmp >> 8;
			v[3] |= tmp;
		}
		else {

			tmp = val >> (-delta);
			v[0] |= tmp >> 24;
			v[1] |= tmp >> 16;
			v[2] |= tmp >> 8;
			v[3] |= tmp;
			v[4] |= value << (8 + delta);
		}

		_ASSERTE(((_cur_bit + n) >> 3) <= _buf_size);

		_cur_bit  += n;
		_tot_bits += n;
		
		//return value;
	}

	virtual void _flush()
	{
		_ASSERTE(!_load);

		if (_cur_bit > 7)
		{
			_flush_buf();
		}
	}

	virtual uint32_t _sgetbits(int n) {
		
		register unsigned int x = _getbits(n);
		
		if (n>1 && (x & stmask[n])) return x | ctmask[n];
		else return x;

	}
	
	virtual uint32_t _align(int n)
	{
		int s = 0;
		// we only allow alignment on multiples of bytes
		if (n % 8) {
			throw InvalidAlignment(__FILE__, __LINE__);
			return 0;
		}
		// align on next byte
		if (_tot_bits % 8) {
			s = 8 - (_tot_bits % 8);
			_skipbits(s);
		}
		while (_tot_bits % n) {
			s += 8;
			_skipbits(8);
		}
		return s;
	}

	CResource< Type > & storage() { return _p_cb; }

public:

	virtual void read_cb(const unsigned char * p_val, uint32_t read);
	virtual void write_cb(uint32_t read);

	void create(CResource<Type > & p_cb
		, uint32_t size = DEFAULT_FILE_PAGE
	    , bool load = true)
	{
		_p_cb = p_cb;

		_buf = NULL;
		_tot_bits = 0;
		
		//_ASSERTE(size > 4);

		bool is_base_of_v = std::is_base_of<Ibitstream_storage, Type>::value;
		_ASSERTE(is_base_of_v);

		uint32_t x = size;

		_load_event.signal(); //start in signal mode

		init(x, load);
	}

	bitstream_base(CResource<Type > & p_cb, uint32_t size = DEFAULT_FILE_PAGE, bool load = true)
	{
		create(p_cb, size, load);
	}

	bitstream_base():_buf(NULL), _shadow_buf(NULL)
	{
		
	}

	virtual ~bitstream_base()
	{
		wait();

		if (_buf)
			delete[] _buf;

		if(_shadow_buf)
			delete[] _shadow_buf;
	}
		
	virtual bool atend()
	{
		if (_p_cb->eof())
			if(!_shadow_len)
				if ((_buf_len << 3) <= _cur_bit)
					return true;

		return false;
	}
	
	inline bool has_bits(int nbit)
	{
		if (_p_cb->eof())
		{
			uint32_t available = (_buf_len << 3) - _cur_bit;
			//if (((_buf_len * 8 - _cur_bit) - nbit) >= 0)
			if(available >= nbit)
				return true;

			return false;
		}

		return true;
	}
	
	uint64_t _getpos(void) const { return _tot_bits; }

	uint32_t _snextbits(int n)
	{
		register unsigned int x = _nextbits(n);
		if (n>1 && (x & stmask[n])) return x | ctmask[n];
		else return x;
	}

	/* Probe next n bits (input) or return 0 (output); in either case, the bitstream is (alen-bit) aligned.
	* If big=0, then the number is represented using the little-endian method; otherwise, big-endian byte-ordering.
	* If sign=0, then no sign extension; otherwise, sign extension.
	* If alen>0, then the number is probed at alen-bit boundary (alen must be multiple of 8).
	*/
	uint32_t _next(int n, int big, int sign, int alen)
	{
		if (alen > 0) _align(alen);
		
		/*if (type == BS_INPUT) */
		{
			if (big) {
				if (sign) return _snextbits(n);
				else return _nextbits(n);
			}
			else return 0;

			/*
			else {
				if (sign) return little_snextbits(n);
				else return little_nextbits(n);
			}
			*/
		}
		//else return 0;
	}

	virtual int _little_putbits(unsigned int value, int n)
	{
		unsigned int tmp = value;
		int bytes = n >> 3;         // number of bytes to write
		int leftbits = n % 8;       // number of bits to write
		unsigned int byte_x = 0;
		int i = 0;
		for (; i < bytes; i++) {
			byte_x = (value >> (8 * i)) & tmask[8];
			_putbits(byte_x, 8);
		}
		if (leftbits > 0) {
			byte_x = (value >> (8 * i)) & tmask[leftbits];
			_putbits(byte_x, leftbits);
		}
		return value;
	}

	virtual uint32_t _little_getbits(int n)
	{
		register unsigned int x = 0;        // the value we will return
		int bytes = n >> 3;                 // number of bytes to read 
		int leftbits = n % 8;               // number of bits to read
		unsigned int byte_x = 0;
		int i = 0;
		for (; i < bytes; i++) {
			byte_x = _getbits(8);
			byte_x <<= (8 * i);
			x |= byte_x;
		}
		if (leftbits > 0) {
			byte_x = _getbits(leftbits);
			byte_x <<= (8 * i);
			x |= byte_x;
		}
		return x;
	}

	
	virtual uint64_t _get_position() const {

		uint64_t spos = _getpos() >> 3;

#ifdef _xDEBUG
		{
			const_cast<mg::core::bitstream_base<Type> *>(this)->wait();
			
			mg::core::auto_lock l(const_cast<mg::core::critical_section&>(_load_lock));

			uint64_t pos = _p_cb->get_position();
			if (_load)
			{
				pos -= _buf_len;
				pos -= _shadow_len;
				pos += (_cur_bit >> 3);

			}else
			{

			//if (pos != spos) //we are writing
			//{
				pos = _p_cb->get_position();
				pos += (_cur_bit >> 3);
				pos += _shadow_len;

				_ASSERTE(pos == spos);
			}
		}
#endif

		return spos;
	}
	
	virtual void _set_position_read(uint64_t rhs) {

		bool do_reload = false;
		{
			if (0 == _shadow_len && ((!_p_cb->eof()) || _pending_read) )
			{
				wait();
			}

			mg::core::auto_lock l(_load_lock);
			
			//buffer start
			uint64_t epos = _p_cb->get_position();
			uint64_t kpos  = epos;
			kpos -= _buf_len;
			kpos -= _shadow_len;
			uint64_t pos = kpos + (_cur_bit >> 3);

			_ASSERTE((_cur_bit >> 3) <= _buf_size);

			FDBGC6(BITSTREAM_POSITION_DEBUG
				, "SET-POSITION\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu32 "\t%" PRIu32 "\t%" PRIu32 ""
				, rhs, epos, pos, _buf_len, _shadow_len, _buf_size);

			if ((rhs < epos) //already read
				&& (kpos <= rhs)
				)
			{



				//we can move in buffer
				uint64_t cur_ava = (_buf_len - (_cur_bit >> 3));
				uint64_t advance = 0;
				
				if(rhs > pos)
					advance = rhs - pos;

				if (advance > cur_ava)
				{
					

					_ASSERTE((advance - cur_ava) <= _shadow_len);

					_cur_bit  += (cur_ava << 3);
					_tot_bits += (cur_ava << 3);

					//force buffer switch
					this->_nextbits(32);

					_ASSERTE(0 == _cur_bit);
					//_ASSERTE(0 == _shadow_len);

					uint64_t n = ((advance - cur_ava) << 3);

					_ASSERTE(((_cur_bit + n) >> 3) <= _buf_len);

					_cur_bit  += n;
					_tot_bits += n;

					_ASSERTE((_cur_bit >> 3) <= _buf_size);
				}
				else if ((rhs < pos) && rhs >= (kpos))
				{
					//move backward
					uint64_t n = pos - rhs;

					_ASSERTE((_cur_bit >> 3) >= n);
					_ASSERTE((_tot_bits >> 3) >= n);

					_cur_bit -= (n << 3);

					if (_tot_bits < (n << 3))
						_tot_bits = 0;
					else
						_tot_bits -= (n << 3);

					_ASSERTE((_cur_bit >> 3) <= _buf_size);

				}
				else
				{
					//move in current buffer
					uint64_t n = (advance << 3);

					_ASSERTE(((_cur_bit + n) >> 3) <= _buf_len);

					_cur_bit += n;
					_tot_bits += n;

					_ASSERTE((_cur_bit >> 3) <= _buf_size);

				}
			}
			else
			{
				//reset
				_p_cb->set_position(rhs);
				_tot_bits = rhs << 3;

				do_reload = true;

				FDBGC3(BITSTREAM_READ_DEBUG
					, "SET-POSITION-RELOAD\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 ""
					, rhs, epos, pos);

			}

		}

		if(do_reload)
			reload();

		FDBGC5(BITSTREAM_POSITION_DEBUG
			, "SET-POSITION-END\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu32 "\t%" PRIu32 "\t%d"
			, rhs, _cur_bit, _shadow_len, _buf_len, do_reload);

	}

	virtual void _set_position_write(uint64_t rhs) {

		bool do_reload = false;
		{
			/*
			if (0 != _shadow_len)
			{
				wait();
			}

			mg::core::auto_lock l(_load_lock);
			*/

			this->_flush();

			_ASSERTE(0 == _cur_bit);

			_p_cb->set_position(rhs);

			_tot_bits = (rhs << 3);

			DBGC5(_T("SET-POSITION-WRITE-END\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu32 "\t%" PRIu32 "\t%d"), rhs, _cur_bit, _shadow_len, _buf_len, do_reload);

		}

	}



	virtual void _set_position(uint64_t rhs) {
		if (_load)
		{
			_set_position_read(rhs);
		}
		else
		{
			_set_position_write(rhs);
		}
	}
	
	virtual void _write_direct(const unsigned char * pbyte, uint32_t size)
	{
		_flush();
		

		_ASSERTE(0 == _buf_len);
		_ASSERTE(0 == _cur_bit);

		/*
		if (0 < _shadow_len)
		{
			//Make sure no other request are pending
			wait();
		}
		*/

		_p_cb->write(pbyte, size, this); //fire and forget

		_tot_bits += (size << 3);
	}
	
	
protected:

	virtual void _skipbits(short n)
	{
		//_getbits(n); 

		mg::core::auto_lock l(_load_lock);

		uint64_t bit_len = _buf_len << 3;

		uint64_t bit_ava = bit_len - _cur_bit;

		if (bit_ava < n)
		{
			//we do not have enougth bits

			uint64_t bit_sha = _shadow_len << 3;

			if ((bit_ava + bit_sha) > n)
			{
				_cur_bit += bit_ava;
				_nextbits(32); //move at the beginnig of the _shadow and async reload shadow
				_ASSERTE(_cur_bit == 0);
				_ASSERTE((bit_sha >> 3) == _buf_len);
				_ASSERTE((n - bit_ava) < (_buf_len << 3));
				_cur_bit += (n - bit_ava);
				_tot_bits += n;
			}
			else
			{
				uint64_t pos = _getpos();
				         pos += n;

						 uint64_t remain = pos % 8;

						 _set_position((pos + n) >> 3);

						 _ASSERTE(_cur_bit == 0);

						 _cur_bit += remain;
						 _tot_bits += remain;
						 
			}

		}
		else
		{
			_cur_bit += n;
			_tot_bits += n;
		}

	}
private:

	virtual void __read(unsigned char * pbytes, uint32_t size, uint32_t * read)
	{
		uint32_t used = static_cast<uint32_t>((this->_cur_bit >> 3));
		uint32_t available = this->_buf_len - used;

		_ASSERTE((_cur_bit >> 3) <= _buf_size);
		
		
		if (available > size)
		{
			(*read) = size;
			
			::memcpy(pbytes, this->_buf + used, size);

			_ASSERTE((_cur_bit >> 3) <= _buf_size);
			_ASSERTE(((_cur_bit >> 3) + size) <= _buf_size);

			this->_cur_bit += (size << 3);
			this->_tot_bits += (size << 3);

			this->_nextbits(32); //reload buffer if needed
		}
		else /*if ((available + this->_shadow_len) > size)*/
		{
			
			::memcpy(pbytes, this->_buf + used, available);

			this->_cur_bit += (available << 3);
			this->_tot_bits += (available << 3);

			if(!this->atend())
				this->_nextbits(32); //reload buffer if needed
			else
			{
				//we are at the end
				(*read) = available;
				return;
			}

			uint32_t todo = size - available;

			if (todo > this->_buf_len)
			{
				todo = this->_buf_len;
			}

			_ASSERTE(this->_buf_len >= todo);
			_ASSERTE(0 == this->_cur_bit);

			::memcpy(pbytes + available, this->_buf, todo);

			this->_cur_bit += (todo << 3);
			this->_tot_bits += (todo << 3);

			this->_nextbits(32); //reload buffer if needed

			(*read) = available + todo;

		}
	}

protected:

	virtual void _read(unsigned char * pbytes, uint32_t size, uint32_t * read)
	{
		uint32_t done(0);
		unsigned char * pmybytes = pbytes;
		uint32_t mysize = size;

		_ASSERTE((_cur_bit >> 3) <= _buf_size);

		while (done < size && (!this->atend()))
		{
			if(done)
				wait();

			_ASSERTE((_cur_bit >> 3) <= _buf_size);

			uint32_t working(0);
			__read(pmybytes, mysize, &working);

			done += working;
			pmybytes += working;
			mysize -= working;


		}

		(*read) = done;

		
	}

	virtual uint32_t _nextcode(unsigned int code, int n, int alen)
	{
		unsigned int s = 0;

		if (_load) {
			if (!alen) {
				while (code != _nextbits(n)) {
					s += 1;
					_skipbits(1);
				}
			}
			else {
				s += _align(alen);
				while (code != _nextbits(n)) {
					s += alen;
					_skipbits(alen);
				}
			}
		}
		else 
			s += _align(alen);

		return s;
	}
};



template<class Type>
class bitstream: public bitstream_base<Type>, public IBitstream
{

public:

	bitstream(CResource<Type > & p_cb, uint32_t size = DEFAULT_FILE_PAGE) 
		: bitstream_base<Type>(p_cb, size)
	{

	}

	bitstream(CResource<Type > & p_cb, bool load, uint32_t size = DEFAULT_FILE_PAGE)
		: bitstream_base<Type>(p_cb, size, load)
	{

	}

	bitstream() {}

	

	virtual uint32_t   getbits(short n)  { return bitstream_base<Type>::_getbits(n);  }
	virtual uint32_t  sgetbits(short n) { return bitstream_base<Type>::_sgetbits(n); }
	virtual uint32_t  nextbits(short n) { return bitstream_base<Type>::_nextbits(n); }
	
	virtual void skipbits(short n) { return bitstream_base<Type>::_skipbits(n); }

	virtual void putbits(uint32_t val, short n) { bitstream_base<Type>::_putbits(val, n); }

	virtual void flush() { bitstream_base<Type>::_flush(); }

	virtual uint32_t align(int n) { return bitstream_base<Type>::_align(n); }
	virtual uint64_t getpos() { return bitstream_base<Type>::_getpos(); }
	virtual uint32_t next(int n, int big, int sign, int alen)
	{
		return bitstream_base<Type>::_next(n, big, sign, alen);
	}
	virtual int little_putbits(unsigned int value, int n){ return bitstream_base<Type>::_little_putbits(value, n); }
	virtual uint32_t little_getbits(int n) { return bitstream_base<Type>::_little_getbits(n); }
	
	virtual bool eof() { return bitstream_base<Type>::atend(); }
	
	virtual uint64_t get_position() const {
		return bitstream_base<Type>::_get_position();
	}
	virtual void     set_position(uint64_t rhs) {
		bitstream_base<Type>::_set_position(rhs);
	}
	
	virtual ~bitstream(){}

	virtual void read(unsigned char * pbytes, uint32_t size, uint32_t * read)
	{
		bitstream_base<Type>::_read(pbytes, size, read);
	}

	virtual void write(const unsigned char * pbytes, uint32_t size)
	{
		bitstream_base<Type>::_write_direct(pbytes, size);
	}

	virtual uint32_t nextcode(unsigned int code, int n, int alen)
	{
		return bitstream_base<Type>::_nextcode(code, n, alen);
	}

};  

template<class Type> void bitstream_base<Type>::fill_buf()
{
	if ((!_shadow_len) && _p_cb->eof())
		throw EndOfData(_T(__FILE__), __LINE__);

	_ASSERTE((_cur_bit >> 3) <= _buf_size);

	if (_shadow_len != _buf_size) {
		wait();
	}

	{
		auto_lock l(_load_lock);

		//DBGC2("READ\tENTER-FILL-BUFFER\t%d\t%d", _shadow_len, _buf_size);
		
		_ASSERTE( ( _shadow_len == _buf_size) || _p_cb->eof() );
		_ASSERTE((_cur_bit >> 3) <= _buf_size);

		uint32_t	n;	// how many bytes we must fetch (already read)
					//uint32_t	l(0);	// how many bytes we will fetch (available)
		uint32_t	u;	// how many are still unread

		n = static_cast<uint32_t>(_cur_bit >> 3);

		_ASSERTE(n > 0);
		_ASSERTE(n <= _buf_size);

		u = _buf_len - n;

		// move unread contents to the beginning of the buffer
		memmove(_buf, _buf + n, u);
		// clear the rest of buf
		memset(_buf + u, 0, n);

		// now we are at the first byte
		_cur_bit &= 7;
		//we have u bytes in buffer yet to read
		_buf_len = u;

		_ASSERTE( ( n == _buf_size - _buf_len ) ||  _p_cb->eof() );
				
		//move as much as we can from _shadow
		_ASSERTE(_shadow_len);
		_ASSERTE( ( n <= _shadow_len) || _p_cb->eof());

		//DBGC3("READ\tFILL-BUFFER\t%d\t%d\t%d", _shadow_len, n, _buf_size - _shadow_len);

		if (_shadow_len)
		{
			if (_shadow_len > n)
			{
				memcpy(_buf + u, _shadow_buf, n);
				_shadow_len -= n;
				_buf_len += n;
			}
			else
			{
				_ASSERTE( ( n == _shadow_len) || _p_cb->eof() );

				memcpy(_buf + u, _shadow_buf, _shadow_len);
				_buf_len += _shadow_len;
				_shadow_len = 0;
			}

		}INVALIDELSE;

		_ASSERTE(_buf_size > _shadow_len);
	}

	_load_shadow = true;
	internal_read(_buf_size - _shadow_len, true);

}

template<class Type> void bitstream_base<Type>::read_cb(const unsigned char * p_val, uint32_t read)
{
	FDBGC4(BITSTREAM_READ_DEBUG, "READ_CB\t%d\t%d\t%d\t%d"
		, _shadow_len, read, _buf_size - _shadow_len, _load_shadow);

	if (_load_shadow)
	{
		if (read)
		{
			//DBGC3("READ\tCB\t%d\t%d\t%d", _shadow_len, read, _buf_size - _shadow_len);

			auto_lock l(_load_lock);

			_ASSERTE( (read <= (_buf_size - _shadow_len)) || _p_cb->eof() );

			if (read)
			{
				
				if (_shadow_len)
				{
					//copy end to beginning
					memcpy(_shadow_buf, _shadow_buf + (_buf_size - _shadow_len), _shadow_len);
				}

				memcpy(_shadow_buf + _shadow_len, p_val, read);

				_shadow_len += read;
			}

			//_ASSERTE( (_shadow_len == _buf_size) || _p_cb->eof() );
		
		}INVALIDELSE;
	}
	else
	{
		auto_lock l(_load_lock);

		//_ASSERTE(read == _buf_size - _buf_len);

		_ASSERTE((_buf_len + read) <= _buf_size);

		memcpy(_buf + _buf_len, p_val, read);


		_buf_len += read;
	}

	DBGC0("READ_CB_SIGNAL");
	_pending_read = false;
	_load_event.signal();
}

template<class Type> void bitstream_base<Type>::write_cb(uint32_t written)
{
	/*
	DBGC0("WRITE_CB");

	{
	auto_lock l(_load_lock);

	//DBGC2("WRITE\tCB\t%d\t%d", _shadow_len, written);
	
	_shadow_len = 0;
	}
	 
	DBGC0("WRITE_CB_SIGNAL");
	_load_event.signal();
	*/
}

template<class Type> uint32_t  bitstream_base<Type>::_nextbits(int n)
{
	register unsigned int x(0);    // the value we will return
	unsigned char *v;           // the byte where cur_bit points to
	register int s;             // number of bits to shift 

	if (n > 32 || n < 0)  //TODO: 64
		throw InvalidInput(__FILE__, __LINE__);

	_ASSERTE((_cur_bit >> 3) <= _buf_size);

	// make sure we have enough data
	if (_cur_bit + n >(_buf_len << 3))
	{
		fill_buf();
		_ASSERTE(_cur_bit == 0);
	}

	// starting byte in buffer
	v = _buf + (_cur_bit >> 3);
	// load 4 bytes, a byte at a time - this way endianess is automatically taken care of
	x = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];

	// figure out how much shifting is required
	s = 32 - ((_cur_bit % 8) + n);

	if (s >= 0) x = (x >> s);   // need right adjust 
	else {                      // shift left and read an extra byte
		x = x << -s;
		x |= v[4] >> (8 + s);
	}

	return (x & tmask[n]);

}


__MGCORE_END_NAMESPACE

