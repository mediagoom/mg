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

__ALX_BEGIN_NAMESPACE

typedef critical_section CCriticalSection;
typedef auto_lock AutoLock;

/** 
* \brief This class handle a queue on a fixed memory address
* This class handle a queue on a fixed memory address with a fixed size.
* Can be used to share memory between threads and processes.
*/
template < typename T
         , typename L  = CCriticalSection
		 , typename AL = AutoLock
         , typename size_type = int>
class cfixed_queue
{
#ifdef UNITTEST
public:
#endif

	

#ifdef UNITTEST
public:
#else
protected:
#endif

    BYTE * _location_;
	L      _lock_;

	bool   _delete_on_no_space;

	struct queue_header
	{
		int starter; //0x00000100
		int full_size;
		int start_offset;
		int end_offset;
		int total_written;
		int total_deleted;
	};

	struct queue_item_header
	{
		int starter; //0x00000200
		int body_size;
		int next_item_offset; 
	};

	queue_header * get_queue_heder() const
	{   _ASSERTE(_location_);
		return reinterpret_cast<queue_header *>(_location_);
	}

	size_type get_queue_header_size() const
	{
		return sizeof(queue_header);
	}

	queue_item_header * get_queue_item_heder(size_type offset, bool new_item = false) const
	{   
		queue_item_header * phi = reinterpret_cast<queue_item_header *>(_location_ + offset);
		_ASSERTE(0x00000200 == phi->starter || new_item);
		return phi;
	}

	size_type get_queue_item_header_size() const
	{
		return sizeof(queue_item_header) + sizeof(T);
	}

	size_type get_item_full_size(const queue_item_header * phi) const
	{
		return get_queue_item_header_size() + phi->body_size;
	}

	void init_header(size_type size)
	{
		 queue_header * ph = get_queue_heder();
		 ph->starter       = 0x00000100;
		 ph->full_size     = size;
		 ph->start_offset  = INT32_MAX;
		 ph->end_offset    = INT32_MAX;
		 ph->total_written = 0;
		 ph->total_deleted = 0;
	}
    ///the space in front the first item
	size_type get_head_space()
	{
		queue_header * ph = get_queue_heder();
		_ASSERTE(0x00000100 == ph->starter);

		if(ph->total_written == ph->total_deleted)
		{
			return ph->full_size - get_queue_header_size();
		}

		_ASSERTE(ph->total_written > ph->total_deleted);
        _ASSERTE(ph->end_offset != ph->start_offset  || 1 == size());
		
		if(ph->end_offset < ph->start_offset)
		{
			return 0;
		}

		return ph->start_offset - get_queue_header_size();
		
	}
    ///the space after the last item
	size_type get_tail_space()
	{
		queue_header * ph = get_queue_heder();
		_ASSERTE(0x00000100 == ph->starter);

		if(ph->total_written == ph->total_deleted)
		{
			return 0;
		}

		_ASSERTE(ph->total_written > ph->total_deleted);
        _ASSERTE(ph->end_offset != ph->start_offset || 1 == size());
		
		queue_item_header * phi = get_queue_item_heder(ph->end_offset);

		size_type r  = ph->full_size;
		          r -= get_item_full_size(phi);
				  r -= ph->end_offset;
				  r -= (ph->end_offset < ph->start_offset)?(ph->full_size - ph->start_offset):0;


#ifdef _DEBUG
		int s = ph->full_size;
		int hs = get_queue_header_size();
		int is = get_item_full_size(phi);
		int es = ph->end_offset;
		int ss = (ph->end_offset < ph->start_offset)?(ph->full_size - ph->start_offset):0;

		int res = s /*- hs*/ - is - es - ss;
        //_ASSERTE(res > 0);
		_ASSERTE(res == r);
#endif
		
		
		return r;
	}

	virtual void no_space()
	{
#ifdef _WIN32
		_RPT1(_CRT_WARN, "QUEUE NO SPACE %d", size());
#endif
	}
	virtual bool delete_on_no_space(){return _delete_on_no_space;}
	
	virtual void delete_direct()
	{
		queue_header      * ph  = get_queue_heder();
		queue_item_header * phi = get_queue_item_heder(ph->start_offset);

#ifdef _DEBUG
		//validate next item
		if(INT32_MIN != phi->next_item_offset)		
			queue_item_header * phi2 = get_queue_item_heder(phi->next_item_offset);
#endif

		_ASSERTE(ph->total_written > ph->total_deleted);

		ph->total_deleted++;
		
		if(size())
		{
			_ASSERTE(INT32_MIN != phi->next_item_offset);
			ph->start_offset = phi->next_item_offset;
		}
		

		_ASSERTE(ph->start_offset >= 0);
	}

public:

	typedef  L _my_lock;
	typedef AL _my_auto_lock;

	cfixed_queue():_location_(NULL), _delete_on_no_space(true)
	{
	}

	cfixed_queue(bool delete_on_no_space):_location_(NULL), _delete_on_no_space(delete_on_no_space)
	{
	}


	virtual void de_allocate()
	{
		AL l(_lock_);

		if(_location_)
			delete[] _location_;

		_location_ = NULL;
	}
	virtual void allocate(bool server = true, size_type queue_size = 3000000)
	{
		_ASSERTE(server);//default implementation cannot be used as client server
        
		de_allocate();

		AL l(_lock_);
		
		
		_ASSERTE(queue_size > sizeof(queue_header));
		
		_ASSERTE((6*4) == sizeof(queue_header));
		_ASSERTE((3*4) == sizeof(queue_item_header));

		_ASSERTE(INT32_MAX > queue_size);

		_location_ = new BYTE[queue_size];

		init_header(queue_size);
	}
	
	virtual ~cfixed_queue()
	{
		de_allocate();
	}
	void delete_item()
	{
		AL l(_lock_);
		delete_direct();
	
	}
	///Look at the next item in the queue
	const T* pick_next(size_type * pread, BYTE ** pp_data) const
	{
		_ASSERTE(size());

		queue_header      * ph  = get_queue_heder();
		queue_item_header * phi = get_queue_item_heder(ph->start_offset);
		
		const T* pt = reinterpret_cast<const T*>(_location_ + ph->start_offset + sizeof(queue_item_header));
		*pp_data = reinterpret_cast<BYTE*>(_location_ + ph->start_offset + get_queue_item_header_size());

		*pread = phi->body_size;

		return pt;

	}	
    ///remove the next item in the queue
	T pop(BYTE *p_data
		  , size_type length
		  , size_type * pread)
	{
		size_type read(0);
		BYTE * pb(0);

		AL l(_lock_);

		const T* pt = pick_next(&read, &pb);
		//size_type space = get_queue_item_header_size() + length;

		*pread  = read;
				
		if(NULL != p_data && length >= read)
		{
			memcpy(p_data, pb, read);

			delete_direct();
		}
		
		return *pt;

	}
	///insert an item in the queue
	bool push(const T t
		    , const BYTE* p_data
		    , unsigned int length
			, bool return_on_failure = false)
	{
		
		AL l(_lock_);

		bool tail = false;

#ifdef _DEBUG
		bool moved = false;
#endif 

		queue_header      * ph  = get_queue_heder();

		_ASSERTE(ph->start_offset >= 0);

		size_type space = get_queue_item_header_size() + length;

		size_type begin_space = get_tail_space() + get_head_space();
		size_type begin_size  = size();

	
		while(get_tail_space() < space && get_head_space() < space && size())
		{
			tail = false;

			if( get_tail_space() >= space)
			{
				tail = true;
			}

			if(!tail && get_head_space() < space)
			{
				no_space();
				if(delete_on_no_space())
				   delete_item();	
				else
				{
					ph  = get_queue_heder();
					_ASSERTE(ph->start_offset >= 0);

#ifdef _DEBUG
	        moved = true;
#endif 
					if(begin_space == (get_tail_space() + get_head_space())
						&& size() == begin_size)
					{
						//we did not deleted any item and no_space did not achived anything.
						break;
					}

					begin_space = get_tail_space() + get_head_space();
					begin_size  = size();

				}
			}

		}

		_ASSERTE(ph->start_offset >= 0);

	    if( get_tail_space() >= space)
		{
			tail = true;
		}

		//we do not have enough storage for the item
		if(get_tail_space() < space && get_head_space() < space)
		{
			
			size_type available_space = get_tail_space() + get_head_space();
			size_type missing_space = available_space;
			
			while(available_space < space
				|| (get_tail_space() < space && get_head_space() < space)
				) //carry-on as long as no_space will free something 
				  //and as long as we cannot write
			{
				no_space();
				available_space = get_tail_space() + get_head_space();
				if(missing_space == available_space)
				{
					if(!return_on_failure)
					{
						//_ASSERTE(false);
						ALXTHROW_T(_T("NOT ENOUGH STORAGE IS AVAILABLE")); //cannot grow further
					}
					else
					{
						//_ASSERTE(false);
						return false;
					}
				}

				ph  = get_queue_heder();

				missing_space = get_tail_space() + get_head_space();

			}

			
		}

		queue_item_header * phi = NULL;
			
		if(size())
			phi = get_queue_item_heder(ph->end_offset);

		size_type offset  =  get_queue_header_size();

		if(tail && phi)
		{
		   _ASSERTE(phi);
		   offset = ph->end_offset + get_item_full_size(phi);
		}

#ifdef _DEBUG
		//validate first item
		if(INT32_MAX != ph->start_offset && size())	
			queue_item_header * phi5 = get_queue_item_heder(ph->start_offset);
#endif

		queue_item_header * pnew = get_queue_item_heder(offset, true);

		pnew->starter = 0x00000200;
		pnew->body_size = length;
		pnew->next_item_offset = INT32_MIN;

		_ASSERTE( (!tail) ||
			( tail || !size() ) && ( (ph->full_size - offset)  >= static_cast<int>(length)) 
			);
		_ASSERTE( tail || (
			!tail && ( ( offset + static_cast<int>(length)) <= ph->start_offset || ph->start_offset == INT32_MAX || (!size()) )
			)
		);
/*
#ifdef _DEBUG
		if(ph->start_offset == 14404)
		{
			_RPT4(_CRT_WARN, "  PROBLEM 14404 %d %d %d %d \r\n", size(), get_head_space(), get_tail_space(),  tail);
			_RPT4(_CRT_WARN, "\tPROBLEM 14404 %d %d %d %d \r\n", offset, get_queue_item_header_size(), length, ph->end_offset);
		}
#endif
*/

		memcpy(_location_ + offset + sizeof(queue_item_header), &t, sizeof(t));
		memcpy(_location_ + offset + get_queue_item_header_size(), p_data, length);

		_ASSERTE(offset >= 0);

		if(phi)
			phi->next_item_offset = offset;
		else
		{
			ph->start_offset = offset;

#ifdef _DEBUG
		//validate first item
		queue_item_header * phi3 = get_queue_item_heder(ph->start_offset);
#endif
		}
        
		ph->end_offset = offset;
		ph->total_written++;

		if(ph->total_written > (INT32_MAX - 10))
		{
			int diff = ph->total_written - ph->total_deleted;

			ph->total_written = diff;
			ph->total_deleted = 0;
		}

		_ASSERTE(ph->start_offset >= 0);

#ifdef _DEBUG
		//validate last item
		queue_item_header * phi2 = get_queue_item_heder(ph->end_offset);
		//validate first item
		queue_item_header * phi4 = get_queue_item_heder(ph->start_offset);
#endif

		return true;
	}
	///return how many items are available
	virtual size_type size() const
	{
		 queue_header * ph = get_queue_heder();
		 _ASSERTE(ph);
		 _ASSERTE(0x00000100 == ph->starter);
         _ASSERTE(ph->total_written >= ph->total_deleted);
		
		 return ph->total_written - ph->total_deleted;
	}


	 L & get_lock() {return _lock_;}

    void clear()
	{
		AL l(_lock_);

		queue_header      * ph  = get_queue_heder();

		ph->total_deleted = ph->total_written;
	}
};

__ALX_END_NAMESPACE

