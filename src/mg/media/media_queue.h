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


#include "fixed_queue.h"
//#include <trcdbg.h>
#include "hnano.h"
//#include <Cexception.h>



__ALX_BEGIN_NAMESPACE

struct media_sample
{
	int64_t composition_time;
	int64_t decoding_time;
	int64_t duration;
	bool             discontinuity;
	bool             bIsSyncPoint; 

	media_sample(
		  int64_t composition_time_init = 0
		, int64_t decoding_time_init = 0
		, int64_t duration_init = 0
		, bool             discontinuity_init = false
		, bool             bIsSyncPoint_init = false
		):
		  composition_time(composition_time_init)
		, decoding_time(decoding_time_init)
		, duration(duration_init)
		, discontinuity(discontinuity_init)
		, bIsSyncPoint(bIsSyncPoint_init)
		{
		}
};


#define LOG_MEDIA_SAMPLE(PTRACE, PMD) /*ALX_TRACE5_EX_P(PTRACE, \
	_T("MEDIA SAMPLE:\tcomposition\t%s\tdecoding\t%s\tduration\t%s\tdiscontinuity\t%d\tSyncPoint\t%d\t") \
	, HNS(PMD->composition_time) \
	, HNS(PMD->decoding_time) \
	, HNS(PMD->duration) \
	, PMD->discontinuity \
	, PMD->bIsSyncPoint \
	);*/


template < typename T> class grow_queue;
template < typename T>
class temp_queue: public cfixed_queue<T>
{
	friend grow_queue<T>;
//public:
//	virtual ~temp_queue()
//	{
//		_location_ = NULL;
//	}
};

template <typename T = media_sample >
class grow_queue: public cfixed_queue<T>
{
	

protected:

	virtual bool delete_on_no_space(){return false;}

	virtual bool failed_to_grow_should_drop()
	{
		return false;
	}

	virtual bool failed_to_grow_should_throw()
	{
		return true;
	}
	
	virtual void no_space()
	{
		temp_queue<T> owner;

		owner._location_ = cfixed_queue<T>::_location_;
		      cfixed_queue<T>::_location_ = NULL;

//#ifdef ALX_WRN1
//	   ALX_WRN1(L">>>QUEUE NO SPACE %d\r\n", owner.get_queue_heder()->full_size);
//#endif

			  try{
					   //double the space
					   cfixed_queue<T>::allocate(true, owner.get_queue_heder()->full_size * 2);

			  }catch(std::bad_alloc &b)
			  {
				  ///warning C4101: b : unreferenced local variable
				  const char * pwhat = b.what();

				  try{
					  
					    

						cfixed_queue<T>::allocate(true, owner.get_queue_heder()->full_size + 3000000);
				  
				  }catch(std::bad_alloc &b)
				  { 
					  ///warning C4101: b : unreferenced local variable
					  const char * pwhat = b.what();
					   

					   cfixed_queue<T>::_location_ = owner._location_;
						owner._location_ = NULL;

					  if(failed_to_grow_should_drop())
					  {
						    //ok there nothing todo but drop items
							 cfixed_queue<T>::delete_item();
							 return;
					  }
					  else
					  {
						   if(failed_to_grow_should_throw())
							   ALXTHROW_T(_T("QUEUE RUN OUT OF MEMORY"));

						   //_ASSERTE(false);
						   return;
					  }
						
						
				  }
			  }

	   //now copy everything
	   while(owner.size())
	   {
		  int read(0);
		  BYTE * pb(0);

		  const T* pt = owner.pick_next(&read, &pb);

		  cfixed_queue<T>::push(*pt, pb, read);

		  owner.delete_item();
	   }

//#ifdef ALX_WRN1
//	   ALX_WRN1(L"<<<QUEUE NO SPACE %d\r\n", owner.get_queue_heder()->full_size);
//#endif

	   owner.de_allocate();
	   
	}

	typedef typename cfixed_queue<T>::_my_auto_lock lock_type;
public:
	virtual int size() const
	{
		lock_type l(const_cast<grow_queue<T> * >(this)->get_lock());

		return cfixed_queue<T>::size();
	}

	const T* locked_pick_next(int * pread, BYTE ** pp_data) const
	{
		lock_type l(const_cast<grow_queue<T> * >(this)->get_lock());

		return cfixed_queue<T>::pick_next(pread, pp_data);
	}

	grow_queue()
	{
	}
};

__ALX_END_NAMESPACE

