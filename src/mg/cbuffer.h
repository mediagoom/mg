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
#include "core/core.h"
#include <stddef.h>
#include <new>
#include <string.h>
//#include <crtdbg.h>

#include <inttypes.h>

#ifndef __ALX__BUFFER_
#define __ALX__BUFFER_


__ALX_BEGIN_NAMESPACE

template <typename type, typename sizetype = size_t>
class ResetBuffer;

template <typename type, typename sizetype = size_t>
class CBuffer{
private:
	//avoid usage 
	//To Have these functionality use CResource
	CBuffer(const CBuffer& orig);
	CBuffer& operator=(CBuffer& rhs);
	
	friend ResetBuffer<type, sizetype>;

protected:

	void growth(sizetype size)
	{
		//either growth enough or growth as much 
		//as the beginning
		sizetype t_nsize = m_iOrigin > size?m_iOrigin:size;
		type *temp = m_p;
		m_p = new type[m_iSize +  t_nsize];
		if(0 == m_p) throw std::bad_alloc();
		memset(m_p, 0, (m_iSize +  t_nsize) * sizeof(type));
		memcpy(m_p, temp, m_iCurrent * sizeof(type));
		m_iSize += t_nsize;
		delete[] (temp);
		//Check we have enough space
		prepare(size);
	}

	//The pointer the beginning of the buffer. This can 
	//change during the life of the class!. Do not store it.
		type *m_p;
	//The next available position in the buffer.
		sizetype m_iCurrent;
	//The original size of the buffer. This also specified 
	//how much the buffer growth when it needs.
		sizetype m_iOrigin; //keep this just to know how much grow
	//The size of the buffer: how many types it contain
		sizetype m_iSize;
public:
	CBuffer(sizetype size = 1024)
	:m_iCurrent(0), m_iSize(size), m_iOrigin(size)
	{ 
		m_p = new type[size];
		if(0 == m_p) throw std::bad_alloc();
		//DOUBLE CHECK BUT THIS SHOULD NEVER HAPPEN SINCE WE HAVE JUST ALLOCATED IT
		if(size < m_iSize) throw std::bad_alloc();
		//----------------------------------//
        memset(m_p, 0, size * sizeof(type));
	}

	virtual ~CBuffer(){delete[] (m_p);}
	//Return the address of the next available space in  the 
	//memory managed by this class.
	type *getCurrent(){return &m_p[m_iCurrent];};
	void add(const type *source, sizetype size)
	{
		_ASSERTE(0 <= size);
		if(0 == size)
			return;

		prepare(size);

		//DOUBLE CHECK BUT THIS SHOULD NEVER HAPPEN SINCE PREPARE DOES THE SAME CHECK
		if((m_iSize - m_iCurrent) < size)throw std::bad_alloc();
		//----------------------------------//

		::memcpy(&m_p[m_iCurrent], source, size * sizeof(type));
			m_iCurrent += size;
	};
	//Simply return the address of the buffer. Remember that 
	//the address is not guaranteed to stay the same.
	type *get(){return m_p;};
	//Call this function after accessing directly the buffer. 
	//This make sure that the Current position is update 
	//and that no data is overwritten.
	void updatePosition(sizetype size)
	{
		_ASSERTE(size >= 0);
		_ASSERTE(size <= getFree());
		m_iCurrent += size;
	};
	//Tells the class how many space is needed before 
	//accessing the buffer directly. Calling this function 
	//make sure that the space is available. You do not need 
	//to call prepare if you call add.
	void prepare(sizetype size)
	{	
		_ASSERTE(0 != size);
		if((m_iSize - m_iCurrent) <= size) 
		growth(size);
	};
	//Return the size of the buffer.
	sizetype getSize() const
	{return m_iSize;};
	//Return the amount of free storage available in the 
	//buffer
	sizetype getFree() const
	{return m_iSize - m_iCurrent;}
	//Return the amount of the buffer that has been committed.
	sizetype getFull() const
	{return m_iCurrent;}
	//The same as above
	sizetype size() const
	{return getFull();}

	type &getAt(uint32_t index){return m_p[index];};

	//Write The element at that point
	void WriteAt(uint32_t index, type * source)
	{
		_ASSERTE(index < m_iSize);
		::memcpy(&m_p[index], source, 1 * sizeof(type));
	}

	//READ A SPECIFIED NUMBER OF TYPES FROM THE SPECIFIED POSITION
	bool ReadBuffer(type *buffer, unsigned long dwsize, unsigned long &dwread, sizetype position)
	{
				dwread = m_iCurrent - position;
				if(0 > dwread) //did not have nothing to read
					return false;

				if(dwsize < dwread) //we still have space
					dwread = dwsize;

				memcpy(buffer, &m_p[position], dwread * sizeof(type));
				return true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	//OPERATORS
	/////////////////////////////////////////////////////////////////////////////////////////////
#if _MSC_VER < 1300 //VC.NET
	type &operator[](sizetype index){return m_p[index];};
#endif

	operator type*(){return get();}

	const CBuffer& operator+=(CBuffer &addbuf)
	{
		add(addbuf.get(), addbuf.getFull());
		return *this;
	}
	
};

template <typename type, typename sizetype>
class ResetBuffer
{
protected:
	CBuffer<type, sizetype> &m_Buffer;
public:
	ResetBuffer(CBuffer<type, sizetype> &Buffer):m_Buffer(Buffer)
	{

	}

	ResetBuffer(CBuffer<type, sizetype> *Buffer):m_Buffer(*Buffer)
	{

	}

	void Reset()
	{
		memset(m_Buffer.m_p, 0, m_Buffer.m_iSize * sizeof(type));
		m_Buffer.m_iCurrent = 0;
		//m_Buffer.m_iSize = 0;
	}

	void Back(sizetype size, bool zero = true)
	{
		_ASSERTE(m_Buffer.m_iCurrent >= size);
		m_Buffer.m_iCurrent -= size;
		if(zero)
			memset(m_Buffer.m_p + m_Buffer.m_iCurrent, 0, size * sizeof(type));
	}

	void MoveBack(sizetype size)
	{
		Back(size, false);
	}
};

__ALX_END_NAMESPACE

#endif // __ALX__BUFFER_

